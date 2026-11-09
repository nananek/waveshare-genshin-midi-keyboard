#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

// ===========================================================================
//  TinyUSB 設定 — RP2350 デュアルロール
//    device (native USB, rhport 0) = USB-MIDI ガジェット
//    host   (PIO-USB,   rhport 1) = USB MIDI クラスホスト
//
//  MIDI ホストには upstream hathach/tinyusb 0.20.0+ が必要
//  (pico-sdk 同梱版には midi_host が無い)。CMake の PICO_TINYUSB_PATH を
//  upstream に向けること。
// ===========================================================================

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_PICO
#endif

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

#define CFG_TUD_ENABLED     1
#define CFG_TUH_ENABLED     1
#define CFG_TUH_RPI_PIO_USB 1

// RootHub ポート割り当て: device=0 (native)、host=1 (PIO-USB)
#define BOARD_TUD_RHPORT 0
#define BOARD_TUH_RHPORT 1

// ---------------------------------------------------------------------------
//  Device (native USB, rhport 0): USB-MIDI
// ---------------------------------------------------------------------------
#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUD_CDC            0
#define CFG_TUD_HID            0
#define CFG_TUD_MSC            0
#define CFG_TUD_MIDI           1
#define CFG_TUD_VENDOR         0
#define CFG_TUD_MIDI_RX_BUFSIZE 256
#define CFG_TUD_MIDI_TX_BUFSIZE 256

// ---------------------------------------------------------------------------
//  Host (PIO-USB, rhport 1): USB MIDI クラス
// ---------------------------------------------------------------------------
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#define CFG_TUH_HUB          0
#define CFG_TUH_DEVICE_MAX   (CFG_TUH_HUB ? 4 : 1)
#define CFG_TUH_MIDI         1
#define CFG_TUH_MIDI_RX_BUFSIZE 64
#define CFG_TUH_MIDI_TX_BUFSIZE 64
#define CFG_TUH_HID          0
#define CFG_TUH_CDC          0
#define CFG_TUH_MSC          0
#define CFG_TUH_VENDOR       0

#endif // TUSB_CONFIG_H
