#ifndef NKRO_REPORT_H
#define NKRO_REPORT_H

#include <stdint.h>
#include <stdbool.h>

// NKRO レポートの純粋な状態管理 (TinyUSB 非依存、単体テスト可能)。
//
// レポート構成 (5 バイト、レポート ID 無し):
//   byte 0      : modifier ビットマップ (未使用、常に 0)
//   byte 1..4   : キービットマップ。Usage 0x04..0x1D (a..z) の 26 ビット + 6 パディング。
//                 キーコード c のビット位置 = (c - 0x04)。
//                 report[1 + bi/8] の (1 << (bi % 8)) ビット。
// この配置は usb_descriptors.c の NKRO レポートディスクリプタと一致していること。

#define NKRO_REPORT_LEN 5

void nkro_report_init(void);

// MIDI Note On / Off を反映。レポートが変化して送信が必要なら true。
// 同じノートの多重 Note On はべき等 (retrigger で二重カウントしない)。
// 別々のノートが同じゲームキーへ丸められた場合 (黒鍵スナップ / WRAP) は
// 参照カウントで管理し、片方を離しても押下が維持される。
bool nkro_report_note_on(uint8_t midi_note);
bool nkro_report_note_off(uint8_t midi_note);

// 全キー解放 (ホットプラグ切断時など)。変化があれば true。
bool nkro_report_release_all(void);

// 現在のレポート先頭 (NKRO_REPORT_LEN バイト)。
const uint8_t *nkro_report_data(void);

#endif // NKRO_REPORT_H
