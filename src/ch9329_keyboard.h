#ifndef CH9329_KEYBOARD_H
#define CH9329_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// CH9329 へ送る 6KRO キーボードレポートの純粋な状態管理。
// TinyUSB / UART 非依存なのでホスト側で単体テストできる。
//
// CH9329 のキーボードレポートは「修飾キー 1 バイト + 予約 1 バイト +
// HID Usage 6 バイト」の固定長。原神の演奏は最大 6 和音を想定する。
// 7 音以上を同時に押した場合は、表現できない分を落とす (固着はさせない)。

#define CH9329_KEYBOARD_KEY_COUNT 6

void ch9329_keyboard_init(void);

// スメール音階 (古びたライアー) のマッピングを使う。
// 呼び出し側は押下中ノートを解放してから切り替えること。
void ch9329_keyboard_set_sumeru_mode(bool enabled);

// MIDI Note On / Off を反映し、レポートが変化したら true を返す。
// 同じノートの多重 Note On はべき等。範囲外・黒鍵は false。
bool ch9329_keyboard_note_on(uint8_t midi_note);
bool ch9329_keyboard_note_off(uint8_t midi_note);

// 全キー解放。変化があれば true。
bool ch9329_keyboard_release_all(void);

// 現在のレポート (HID Usage 6 個、未使用は 0)。
const uint8_t *ch9329_keyboard_keys(void);

// 現在の修飾キービットマップ (本用途では常に 0)。
uint8_t ch9329_keyboard_modifier(void);

#endif // CH9329_KEYBOARD_H
