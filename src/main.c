#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/clocks.h"

#include "tusb.h"
#include "pio_usb.h"

#include "config.h"
#include "hid_device.h"
#include "hid_mute.h"
#include "midi_host.h"
#include "mirror_filter_switch.h"
#include "usb_power_control.h"

// ===========================================================================
//  MIDI(host, core1) → HID(device, core0) の一方向パイプライン
//  イベントはロックフリーキューでコア間受け渡し。HID 状態は core0 のみが触る。
// ===========================================================================

#define EVT_RELEASE_ALL 0xFFu // note フィールドのセンチネル (有効 MIDI ノートは 0..127)

typedef struct {
    uint8_t note;
    uint8_t on;
} midi_evt_t;

static queue_t s_evt_queue;

// MIDI 側から見た「現在押されているノート」(ミュート中も含め常時更新)。
// ミュート解除時、押しっぱなしのまま新規 Note On が来ないノートを再送するために使う。
static bool s_note_held[128];

// ---- midi_host.c (core1) から呼ばれるシーム実装 ----
void app_on_midi_note(uint8_t note, bool on) {
    midi_evt_t e = { .note = note, .on = on ? 1u : 0u };
    // キューが溢れたら取りこぼす (演奏用途では実質発生しない深さを確保)
    queue_try_add(&s_evt_queue, &e);
}

void app_on_midi_disconnect(void) {
    midi_evt_t e = { .note = EVT_RELEASE_ALL, .on = 0 };
    queue_try_add(&s_evt_queue, &e);
}

// ---------------------------------------------------------------------------
//  core1: PIO-USB ホスト (rhport 1)
//  tuh_configure / tuh_init / tuh_task はすべてこのコアで行うこと
//  (PIO/DMA ステートマシンが起動コアに紐づくため)。
// ---------------------------------------------------------------------------
static void core1_main(void) {
    sleep_ms(10);

    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = PIN_USB_HOST_DP; // D- は自動的に pin_dp + 1
    tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    tuh_init(BOARD_TUH_RHPORT);

    midi_host_init();

    for (;;) {
        tuh_task();
    }
}

// ---------------------------------------------------------------------------
//  core0: ネイティブ USB デバイス (rhport 0) = HID キーボード
// ---------------------------------------------------------------------------
int main(void) {
    // PIO-USB は 120MHz (または 240MHz) のシステムクロックを要求する。
    set_sys_clock_khz(120000, true);

    // Establish GP9=LOW before any USB stack, second core, or application
    // peripheral starts.  The board-level pull-down independently covers the
    // reset interval before these instructions execute.
    usb_power_control_init(to_ms_since_boot(get_absolute_time()));

    // stdio はデバッグログ用に UART へ (native USB は TinyUSB が占有)。
    stdio_init_all();
    printf("\r\n[boot] Genshin MIDI keyboard: D+=GP%d\r\n", PIN_USB_HOST_DP);

    queue_init(&s_evt_queue, sizeof(midi_evt_t), 128);

    // core0 のデバイススタック・GPIO/共有フラグ初期化を終えてから core1 (PIO-USB
    // ホスト) を起動する。逆順だと、USB 列挙が早く終わった場合に core1 が
    // 楽器モードスイッチの共有値を未初期化の既定値で読む窓ができる。
    tud_init(BOARD_TUD_RHPORT);
    hid_device_init();
    hid_mute_init();
    mirror_filter_switch_init();
    hid_device_set_sumeru_mode(mirror_filter_switch_is_enabled());

    multicore_launch_core1(core1_main);

    for (;;) {
        tud_task();

        // Downstream VBUS is available only after the PC/iPad-side native USB
        // device is configured, and is removed again on detach or suspend.
        // J3 VBUS remains physically isolated; this controls only J1→J2 power.
        bool upstream_ready = tud_mounted() && !tud_suspended();
        usb_power_control_task(to_ms_since_boot(get_absolute_time()),
                               upstream_ready);

        // --- ミュートスイッチ: デバウンス済みエッジを検出 ---
        hid_mute_edge_t edge = hid_mute_poll();
        if (edge == HID_MUTE_ENTER) {
            printf("[mute] ON\r\n");
            // 全キー解放 → 空レポート送信 (原神側の固着キー解除)。
            // 送信はループ末尾の hid_device_task() が行う (busy 中は次回リトライ)。
            hid_device_release_all();
        } else if (edge == HID_MUTE_EXIT) {
            printf("[mute] OFF\r\n");
            // ミュート中も押しっぱなしのノートは MIDI 側から Note On が再送されないため、
            // 現在の保持状態を元に HID へ再アサートする (取りこぼし防止)。
            for (int i = 0; i < 128; i++) {
                if (s_note_held[i]) {
                    hid_device_note_on((uint8_t)i);
                }
            }
        }

        // --- 楽器モードスイッチ: デバウンス + 共有フラグ反映 ---
        bool old_sumeru_mode = mirror_filter_switch_is_enabled();
        mirror_filter_switch_poll();
        bool new_sumeru_mode = mirror_filter_switch_is_enabled();
        if (new_sumeru_mode != old_sumeru_mode) {
            // 押下中ノートを一度解放し、同じ保持状態を新しい音階で再配置する。
            hid_device_release_all();
            hid_device_set_sumeru_mode(new_sumeru_mode);
            for (int i = 0; i < 128; i++) {
                if (s_note_held[i]) {
                    hid_device_note_on((uint8_t)i);
                }
            }
        }

        // core1 から届いた MIDI イベントを反映。
        // ミュート中もキューは排出しつつ保持状態 (s_note_held) だけは更新し、
        // HID 状態への反映のみ止める (溢れ防止 + unmute 時の再アサートに使う)。
        bool muted = hid_mute_is_muted();
        midi_evt_t e;
        while (queue_try_remove(&s_evt_queue, &e)) {
            if (e.note == EVT_RELEASE_ALL) {
                for (int i = 0; i < 128; i++) {
                    s_note_held[i] = false;
                }
                if (!muted) {
                    hid_device_release_all();
                }
                continue;
            }
            s_note_held[e.note] = e.on;
            if (muted) {
                continue;
            }
            if (e.on) {
                hid_device_note_on(e.note);
            } else {
                hid_device_note_off(e.note);
            }
        }

        // 保留中レポートの送信。ミュート中は新規のキー出力は発生しないため、
        // 実質エッジ時点の空レポート送信のみが走り、それ以外は no-op。
        hid_device_task();
    }
}
