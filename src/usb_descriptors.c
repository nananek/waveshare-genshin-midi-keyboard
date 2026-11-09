#include <string.h>
#include "tusb.h"
#include "pico/unique_id.h"

// ===========================================================================
//  native USB = USB-MIDI ガジェット
//
//  PIO-USB ホストが受けた MIDI キーボードの生バイト列を、この USB-MIDI
//  デバイスとして PC/DAW へそのまま出力する。
//
//  HID キーボード機能は CH9329 が担当するため、このファームウェアは
//  USB HID デバイスを列挙しない。これにより「MIDI を見せる PC」と
//  「CH9329 で操作する PC」を物理的に分けられる。
// ===========================================================================

#define USB_VID 0xCAFE // テスト用 (実配布時は正規に取得した VID/PID に置換)
#define USB_PID 0x4D49 // 'M''I'
#define USB_BCD 0x0200
#define USB_MAX_POWER_MA 100

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

// ---------------------------------------------------------------------------
//  コンフィグレーションディスクリプタ
//  TUD_MIDI_DESCRIPTOR は Audio Control + MIDI Streaming の 2 インタフェース。
// ---------------------------------------------------------------------------
enum { ITF_NUM_MIDI = 0, ITF_NUM_MIDI_STREAMING, ITF_NUM_TOTAL };

#define EPNUM_MIDI_OUT 0x01
#define EPNUM_MIDI_IN  0x81

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, USB_MAX_POWER_MA),
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 0, EPNUM_MIDI_OUT, EPNUM_MIDI_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

// ---------------------------------------------------------------------------
//  文字列ディスクリプタ
// ---------------------------------------------------------------------------
enum { STRID_LANGID = 0, STRID_MANUFACTURER, STRID_PRODUCT, STRID_SERIAL };

static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},        // 0: 言語 ID (英語 0x0409)
    "MIDI2Genshin",                    // 1: Manufacturer
    "PIO-MIDI CH9329 Gadget",          // 2: Product
    NULL,                              // 3: Serial (unique id から生成)
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

    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
