#ifndef HID_DEVICE_H
#define HID_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

// ネイティブ USB 側 = NKRO HID キーボードデバイス。
// core0 (tud_task を回すコア) からのみ呼ぶこと。内部状態に排他は無い。

void hid_device_init(void);

// MIDI Note On / Off を NKRO レポート状態へ反映する。
void hid_device_note_on(uint8_t midi_note);
void hid_device_note_off(uint8_t midi_note);

// 全キー解放 (MIDI デバイス切断時など)。
void hid_device_release_all(void);

// 保留中のレポートを送信可能なら送る。core0 のメインループから毎回呼ぶ。
void hid_device_task(void);

#endif // HID_DEVICE_H
