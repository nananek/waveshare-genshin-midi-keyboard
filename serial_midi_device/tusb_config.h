#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

// ===========================================================================
//  TinyUSB 設定 — シリアル→USB-MIDI ブリッジ (ボード2)
//  デバイス専用 (native USB, rhport 0) = USB-MIDI クラスデバイス。
//  PIO-USB ホスト・デュアルコアは使わない。
//
//  CFG_TUSB_MCU / CFG_TUSB_OS は pico-sdk の CMake が設定するので原則ここでは
//  定義しない (RP2350 でも MCU 値は OPT_MCU_RP2040)。念のためのフォールバックのみ。
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

// ---- デバイスのみ有効化 (ホスト不使用) ----
#define CFG_TUD_ENABLED     1
#define CFG_TUH_ENABLED     0

// RootHub ポート割り当て: device=0 (native USB)
#define BOARD_TUD_RHPORT 0

// ---------------------------------------------------------------------------
//  Device (native USB, rhport 0): USB-MIDI
// ---------------------------------------------------------------------------
#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUD_CDC            0
#define CFG_TUD_HID            0
#define CFG_TUD_MSC            0
#define CFG_TUD_MIDI           1
#define CFG_TUD_VENDOR         0
// MIDI FIFO サイズ。UART のバーストを考慮し十分に取る。
#define CFG_TUD_MIDI_RX_BUFSIZE 256
#define CFG_TUD_MIDI_TX_BUFSIZE 256

#endif // TUSB_CONFIG_H
