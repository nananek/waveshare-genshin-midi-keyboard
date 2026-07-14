#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/clocks.h"

#include "tusb.h"
#include "pio_usb.h"

#include "config.h"
#include "hid_device.h"
#include "midi_host.h"

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

    // stdio はデバッグログ用に UART へ (native USB は TinyUSB が占有)。
    stdio_init_all();
    printf("\r\n[boot] Genshin MIDI keyboard: D+=GP%d\r\n", PIN_USB_HOST_DP);

    queue_init(&s_evt_queue, sizeof(midi_evt_t), 128);

    // 先に core1 で PIO-USB ホストを起動。
    multicore_launch_core1(core1_main);

    // core0 でデバイススタックを起動。
    tud_init(BOARD_TUD_RHPORT);
    hid_device_init();

    for (;;) {
        tud_task();

        // core1 から届いた MIDI イベントを反映。
        midi_evt_t e;
        while (queue_try_remove(&s_evt_queue, &e)) {
            if (e.note == EVT_RELEASE_ALL) {
                hid_device_release_all();
            } else if (e.on) {
                hid_device_note_on(e.note);
            } else {
                hid_device_note_off(e.note);
            }
        }

        hid_device_task();
    }
}
