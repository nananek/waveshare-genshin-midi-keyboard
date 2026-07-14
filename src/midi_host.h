#ifndef MIDI_HOST_H
#define MIDI_HOST_H

// PIO-USB 側 = USB MIDI クラスホスト。
// TinyUSB の tuh_midi_* コールバックはこのモジュールが実装する。
// core1 (tuh_task を回すコア) からのみ呼ぶこと。

// MIDI パーサ等の初期化。core1 で tuh_init(1) の後に呼ぶ。
void midi_host_init(void);

// ---------------------------------------------------------------------------
//  アプリ側 (main.c) が実装するシーム。
//  midi_host は core1 で動くため、ここでは HID 状態を直接触らず、
//  イベントを core0 へ受け渡す実装を期待する (キュー等)。
// ---------------------------------------------------------------------------

// Note On (on=true) / Note Off (on=false)。note は 0..127。
extern void app_on_midi_note(uint8_t note, bool on);

// MIDI デバイスが切断された。押しっぱなし防止に全キー解放を促す。
extern void app_on_midi_disconnect(void);

#endif // MIDI_HOST_H
