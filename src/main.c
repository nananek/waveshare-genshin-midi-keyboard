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
#include "midi_uart.h"
#include "mirror_filter_switch.h"
#include "spare_switch.h"

// ===========================================================================
//  MIDI keyboard (PIO-USB host) → NKRO HID (native USB)
//  + RAW MIDI (UART1 31250) → CH345 → USB-MIDI
//
//  core1: PIO-USB ホスト (MIDI キーボード入力、RAW MIDI を CH345 へ)
//  core0: native USB = NKRO HID キーボード (操作対象 PC)
//
//  HID 状態は core0 だけが触る。MIDI ノートイベントはロックフリーキューで
//  core1 → core0 へ渡す。
// ===========================================================================

#define EVT_RELEASE_ALL 0xFFu // note フィールドのセンチネル (有効 MIDI ノートは 0..127)

typedef struct {
    uint8_t note;
    uint8_t on;
} midi_evt_t;

static queue_t s_evt_queue;

// MIDI 側から見た「現在押されているノート」(ミュート中も含め常時更新)。
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

static void reassert_held_notes(void) {
    hid_device_release_all();
    for (int i = 0; i < 128; i++) {
        if (s_note_held[i]) {
            hid_device_note_on((uint8_t)i);
        }
    }
}

int main(void) {
    // PIO-USB は 120MHz (または 240MHz) のシステムクロックを要求する。
    set_sys_clock_khz(120000, true);

    // stdio はデバッグログ用に UART0 (GP0/GP1) へ。
    // native USB は NKRO HID が占有するため stdio-usb は使わない。
    stdio_init_all();
    printf("\r\n[boot] MIDI→NKRO HID + CH345: D+=GP%d UART%d TX=GP%d\r\n",
           PIN_USB_HOST_DP, MIDI_UART_INDEX, MIDI_UART_TX_PIN);

    queue_init(&s_evt_queue, sizeof(midi_evt_t), 128);

    // native USB = NKRO HID キーボード。
    tud_init(BOARD_TUD_RHPORT);
    hid_device_init();
    midi_uart_init();
    hid_mute_init();
    mirror_filter_switch_init();
    spare_switch_init();
    hid_device_set_sumeru_mode(mirror_filter_switch_is_enabled());

    multicore_launch_core1(core1_main);

    for (;;) {
        tud_task();

        // --- 原神モードスイッチ: HID 出力のミュート ---
        hid_mute_edge_t edge = hid_mute_poll();
        if (edge == HID_MUTE_ENTER) {
            printf("[mute] ON: NKRO HID output disabled\r\n");
            hid_device_release_all();
        } else if (edge == HID_MUTE_EXIT) {
            printf("[mute] OFF: reassert held notes\r\n");
            reassert_held_notes();
        }

        // --- 予備スイッチ: 既定では状態を読むだけ。SPARE_SWITCH_CC を
        //     設定すると CH345 経由で CC を送る ---
        spare_switch_edge_t spare_edge = spare_switch_poll();
        if (spare_edge != SPARE_SWITCH_NO_CHANGE) {
#if SPARE_SWITCH_CC >= 0
            uint8_t value = (spare_edge == SPARE_SWITCH_ENTER) ? 127u : 0u;
            midi_uart_send_cc(SPARE_SWITCH_MIDI_CHANNEL,
                              (uint8_t)SPARE_SWITCH_CC, value);
            printf("[spare] CC%d=%u\r\n", SPARE_SWITCH_CC, value);
#else
            printf("[spare] %s\r\n",
                   spare_edge == SPARE_SWITCH_ENTER ? "ON" : "OFF");
#endif
        }

        // --- 楽器モードスイッチ: 押下中ノートを新しい音階で再配置 ---
        bool old_sumeru_mode = mirror_filter_switch_is_enabled();
        mirror_filter_switch_poll();
        bool new_sumeru_mode = mirror_filter_switch_is_enabled();
        if (new_sumeru_mode != old_sumeru_mode) {
            if (!hid_mute_is_muted()) {
                hid_device_release_all();
                hid_device_set_sumeru_mode(new_sumeru_mode);
                for (int i = 0; i < 128; i++) {
                    if (s_note_held[i]) {
                        hid_device_note_on((uint8_t)i);
                    }
                }
            } else {
                hid_device_set_sumeru_mode(new_sumeru_mode);
            }
        }

        // core1 から届いた MIDI イベントを反映。
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

        hid_device_task();
    }
}
