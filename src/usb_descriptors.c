#include <string.h>
#include "tusb.h"
#include "pico/unique_id.h"

// ===========================================================================
//  USB デバイスディスクリプタ (native USB = HID キーボード)
// ===========================================================================

#define USB_VID 0xCAFE // テスト用 (実配布時は正規に取得した VID/PID に置換)
#define USB_PID 0x4753 // 'G''S'
#define USB_BCD 0x0200

static tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

// ===========================================================================
//  HID レポートディスクリプタ (カスタム NKRO / ビットマップ)
//
//  レポート構成 (5 バイト、レポート ID 無し):
//    byte 0    : modifier ビットマップ (Usage 0xE0..0xE7、本用途では常に 0)
//    byte 1..4 : キービットマップ。Usage 0x04..0x1D (a..z) の 26 ビット + 6 パディング。
//                キーコード c のビット位置 = (c - 0x04)。
//                → nkro_report.c のビット配置と一致していること。
//
//  演奏に使う 21 キー (Q W E R T Y U / A S D F G H J / Z X C V B N M) は
//  すべて a..z の範囲に収まるため、26 ビットのビットマップで全て表現できる。
// ===========================================================================

static uint8_t const desc_hid_report[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)

    // --- Modifier キー (8 bit) ---
    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,       //   Usage Minimum (Left Control)
    0x29, 0xE7,       //   Usage Maximum (Right GUI)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x08,       //   Report Count (8)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)

    // --- キービットマップ: Usage 0x04..0x1D (a..z) = 26 bit ---
    0x05, 0x07,       //   Usage Page (Keyboard/Keypad)
    0x19, 0x04,       //   Usage Minimum (a / 0x04)
    0x29, 0x1D,       //   Usage Maximum (z / 0x1D)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x1A,       //   Report Count (26)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)

    // --- バイト境界へのパディング (6 bit) ---
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x06,       //   Report Count (6)
    0x81, 0x03,       //   Input (Constant, Variable, Absolute)

    0xC0              // End Collection
};

// instance ごとのレポートディスクリプタを返す (HID は 1 インスタンスのみ)。
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return desc_hid_report;
}

// ===========================================================================
//  コンフィグレーションディスクリプタ
// ===========================================================================

enum { ITF_NUM_HID = 0, ITF_NUM_TOTAL };

#define EPNUM_HID 0x81 // IN endpoint 1

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

static uint8_t const desc_configuration[] = {
    // Config: number, interface count, string index, total length, attribute, power (mA)
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // HID: interface, string, protocol, report desc len, EP in, size, polling(ms)
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report),
                       EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 1),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

// ===========================================================================
//  文字列ディスクリプタ
// ===========================================================================

enum { STRID_LANGID = 0, STRID_MANUFACTURER, STRID_PRODUCT, STRID_SERIAL };

static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},   // 0: 言語 ID (英語 0x0409)
    "MIDI2Genshin",               // 1: Manufacturer
    "Genshin Chamber Keyboard",   // 2: Product
    NULL,                         // 3: Serial (unique id から生成)
};

static uint16_t _desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    size_t chr_count;

    switch (index) {
        case STRID_LANGID:
            memcpy(&_desc_str[1], string_desc_arr[0], 2);
            chr_count = 1;
            break;

        case STRID_SERIAL: {
            char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
            pico_get_unique_board_id_string(serial, sizeof(serial));
            chr_count = strlen(serial);
            if (chr_count > 32) {
                chr_count = 32;
            }
            for (size_t i = 0; i < chr_count; i++) {
                _desc_str[1 + i] = (uint16_t)serial[i];
            }
            break;
        }

        default: {
            if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
                return NULL;
            }
            const char *str = string_desc_arr[index];
            chr_count = strlen(str);
            if (chr_count > 32) {
                chr_count = 32;
            }
            for (size_t i = 0; i < chr_count; i++) {
                _desc_str[1 + i] = (uint16_t)str[i];
            }
            break;
        }
    }

    // 先頭ワード: bLength と bDescriptorType(STRING)
    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
