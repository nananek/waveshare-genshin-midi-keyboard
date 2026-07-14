#include <string.h>
#include "tusb.h"
#include "hid_device.h"
#include "nkro_report.h"

// レポートが変化して未送信のとき true。core0 のみが触る。
static bool s_dirty = false;

static void try_send(void) {
    if (!s_dirty) {
        return;
    }
    if (!tud_hid_ready()) {
        return; // まだ前回送信中 / 未接続
    }
    // レポート ID 無し → 第1引数は 0。NKRO_REPORT_LEN バイト全体を送る。
    if (tud_hid_report(0, nkro_report_data(), NKRO_REPORT_LEN)) {
        s_dirty = false;
    }
}

void hid_device_init(void) {
    nkro_report_init();
    s_dirty = false;
}

void hid_device_note_on(uint8_t midi_note) {
    if (nkro_report_note_on(midi_note)) {
        s_dirty = true;
    }
}

void hid_device_note_off(uint8_t midi_note) {
    if (nkro_report_note_off(midi_note)) {
        s_dirty = true;
    }
}

void hid_device_release_all(void) {
    if (nkro_report_release_all()) {
        s_dirty = true;
    }
}

void hid_device_task(void) {
    try_send();
}

// ---- TinyUSB HID デバイスコールバック (weak override) ----

// 送信完了。まだ差分があれば続けて送る (連続入力のレイテンシ低減)。
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len) {
    (void)instance;
    (void)report;
    (void)len;
    try_send();
}

// GET_REPORT: 現在の入力レポートを返す。
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    (void)instance;
    (void)report_id;
    if (report_type == HID_REPORT_TYPE_INPUT) {
        uint16_t n = (NKRO_REPORT_LEN < reqlen) ? (uint16_t)NKRO_REPORT_LEN : reqlen;
        memcpy(buffer, nkro_report_data(), n);
        return n;
    }
    return 0;
}

// SET_REPORT: 出力レポート (LED 等) は使わないので無視。
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
