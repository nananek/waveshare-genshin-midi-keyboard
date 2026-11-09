#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

// ===========================================================================
//  TinyUSB 設定 — RP2350 デュアルロール
//    device (native USB, rhport 0) = HID キーボード
//    host   (PIO-USB,   rhport 1) = USB MIDI クラスホスト
//
//  注意: このプロジェクトは MIDI ホスト対応の upstream hathach/tinyusb
//  (0.20.0+) を前提とする。pico-sdk 同梱の raspberrypi/tinyusb には
//  midi_host が無いため tuh_midi_* がリンクできない。CMake の
//  PICO_TINYUSB_PATH で upstream を指すこと (README 参照)。
// ===========================================================================

// CFG_TUSB_MCU / CFG_TUSB_OS は pico-sdk の CMake が設定するので原則ここでは
// 定義しない (RP2350 でも MCU 値は OPT_MCU_RP2040)。念のためのフォールバックのみ。
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_PICO
#endif

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#endif

// ---- 両スタック有効化 ----
#define CFG_TUD_ENABLED     1
#define CFG_TUH_ENABLED     1
#define CFG_TUH_RPI_PIO_USB 1  // ホスト側を PIO-USB で駆動

// RootHub ポート割り当て: device=0 (native)、host=1 (PIO-USB)
#define BOARD_TUD_RHPORT 0
#define BOARD_TUH_RHPORT 1

// ---------------------------------------------------------------------------
//  Device (native USB, rhport 0): HID キーボード
// ---------------------------------------------------------------------------
#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUD_HID            1
#define CFG_TUD_CDC            0
#define CFG_TUD_MSC            0
#define CFG_TUD_MIDI          0
#define CFG_TUD_VENDOR        0
// NKRO レポート (5 バイト) を余裕を持って収める
#define CFG_TUD_HID_EP_BUFSIZE 16

// ---------------------------------------------------------------------------
//  Host (PIO-USB, rhport 1): USB MIDI クラス
// ---------------------------------------------------------------------------
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#define CFG_TUH_HUB          0   // ハブ経由で繋ぐ場合のみ 1
#define CFG_TUH_DEVICE_MAX   (CFG_TUH_HUB ? 4 : 1)
#define CFG_TUH_MIDI         1   // MIDI ホストインタフェース数
#define CFG_TUH_MIDI_RX_BUFSIZE 64
#define CFG_TUH_MIDI_TX_BUFSIZE 64
// HID/CDC/MSC ホストは不要
#define CFG_TUH_HID          0
#define CFG_TUH_CDC          0
#define CFG_TUH_MSC          0
#define CFG_TUH_VENDOR       0

#endif // TUSB_CONFIG_H
