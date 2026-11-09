#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/clocks.h"

#include "tusb.h"
#include "pio_usb.h"

#include "ch9329_keyboard.h"
#include "ch9329_uart.h"
#include "config.h"
#include "hid_mute.h"
#include "midi_bridge.h"
#include "midi_host.h"
#include "mirror_filter_switch.h"

// ===========================================================================
//  PIO-MIDI gadget + CH9329 keyboard pipeline
//
//  core1: PIO-USB ホスト = MIDI キーボード入力
//  core0: native USB = USB-MIDI ガジェット (PC/DAW へ生 MIDI)
//         UART1       = CH9329 (操作対象 PC へ HID キーボード)
//
//  MIDI ノートイベントはロックフリーキューで core1 → core0 へ渡す。
//  CH9329 のキーボード状態は core0 だけが触る。
// ===========================================================================

#define EVT_RELEASE_ALL 0xFFu // note フィールドのセンチネル (有効 MIDI ノートは 0..127)

typedef struct {
    uint8_t note;
    uint8_t on;
} midi_evt_t;

static queue_t s_evt_queue;

// MIDI 側から見た「現在押されているノート」(ミュート中も含め常時更新)。
// ミュート解除時、押しっぱなしのまま新規 Note On が来ないノートを再送する。
static bool s_note_held[128];

// ---- midi_host.c (core1) から呼ばれるシーム実装 ----
void app_on_midi_note(uint8_t note, bool on) {
    midi_evt_t e = { .note = note, .on = on ? 1u : 0u };
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

static void queue_keyboard_report(void) {
    ch9329_uart_queue_report(ch9329_keyboard_modifier(),
                              ch9329_keyboard_keys());
}

static void reassert_held_notes(void) {
    ch9329_keyboard_release_all();
    for (int i = 0; i < 128; i++) {
        if (s_note_held[i]) {
            ch9329_keyboard_note_on((uint8_t)i);
        }
    }
    queue_keyboard_report();
}

int main(void) {
    // PIO-USB は 120MHz (または 240MHz) のシステムクロックを要求する。
    set_sys_clock_khz(120000, true);

    // stdio はデバッグログ用に UART0 (GP0/GP1) へ。
    // native USB は USB-MIDI が占有するため stdio-usb は使わない。
    stdio_init_all();
    printf("\r\n[boot] PIO-MIDI CH9329 gadget: D+=GP%d UART%d TX=GP%d\r\n",
           PIN_USB_HOST_DP, CH9329_UART_INDEX, CH9329_UART_TX_PIN);

    queue_init(&s_evt_queue, sizeof(midi_evt_t), 128);
    midi_bridge_init();

    // native USB = USB-MIDI デバイス。
    tud_init(BOARD_TUD_RHPORT);

    ch9329_keyboard_init();
    ch9329_uart_init();
    hid_mute_init();
    mirror_filter_switch_init();
    ch9329_keyboard_set_sumeru_mode(mirror_filter_switch_is_enabled());

    // 起動時に空レポートを一度送り、CH9329 側の固着キーを解除する。
    queue_keyboard_report();

    multicore_launch_core1(core1_main);

    for (;;) {
        tud_task();

        // USB-MIDI ガジェットへの生 MIDI パススルー。
        midi_bridge_task();

        // --- 原神モードスイッチ: CH9329 出力のミュート ---
        hid_mute_edge_t edge = hid_mute_poll();
        if (edge == HID_MUTE_ENTER) {
            printf("[mute] ON: CH9329 keyboard output disabled\r\n");
            ch9329_keyboard_release_all();
            queue_keyboard_report();
        } else if (edge == HID_MUTE_EXIT) {
            printf("[mute] OFF: reassert held notes\r\n");
            reassert_held_notes();
        }

        // --- 楽器モードスイッチ: 押下中ノートを新しい音階で再配置 ---
        bool old_sumeru_mode = mirror_filter_switch_is_enabled();
        mirror_filter_switch_poll();
        bool new_sumeru_mode = mirror_filter_switch_is_enabled();
        if (new_sumeru_mode != old_sumeru_mode) {
            // ミュート中は CH9329 に何も送らない。解除時に新しい音階で
            // 保持ノートを再アサートする。
            if (!hid_mute_is_muted()) {
                ch9329_keyboard_release_all();
                ch9329_keyboard_set_sumeru_mode(new_sumeru_mode);
                for (int i = 0; i < 128; i++) {
                    if (s_note_held[i]) {
                        ch9329_keyboard_note_on((uint8_t)i);
                    }
                }
                queue_keyboard_report();
            } else {
                ch9329_keyboard_set_sumeru_mode(new_sumeru_mode);
            }
        }

        // core1 から届いた MIDI イベントを反映。
        // ミュート中もキューは排出しつつ保持状態だけは更新する。
        bool muted = hid_mute_is_muted();
        bool keyboard_changed = false;
        midi_evt_t e;
        while (queue_try_remove(&s_evt_queue, &e)) {
            if (e.note == EVT_RELEASE_ALL) {
                for (int i = 0; i < 128; i++) {
                    s_note_held[i] = false;
                }
                if (!muted) {
                    keyboard_changed |= ch9329_keyboard_release_all();
                }
                continue;
            }
            s_note_held[e.note] = e.on;
            if (muted) {
                continue;
            }
            if (e.on) {
                keyboard_changed |= ch9329_keyboard_note_on(e.note);
            } else {
                keyboard_changed |= ch9329_keyboard_note_off(e.note);
            }
        }
        if (keyboard_changed) {
            queue_keyboard_report();
        }

        ch9329_uart_task();
    }
}
