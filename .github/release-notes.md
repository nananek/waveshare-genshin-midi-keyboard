## Firmware

- `midi_ch9329_gadget.uf2` は新しい 1 枚構成用です。
  - PIO-USB ホストで MIDI キーボードを受信
  - native USB を USB-MIDI ガジェットとして PC/DAW へパススルー
  - UART1 (GP4/GP5, 9600 8N1) で CH9329 を駆動し、操作対象 PC へ HID キーボードを送信

旧 `genshin_midi_kbd.uf2` / `serial_midi_device.uf2` の 2 ボード構成は廃止しました。

## Hardware

`hardware/rp2350_frisk/` に新基板の外形・配置ドラフトがあります。

- 57.6 × 31.4 mm、四隅 2 mm 45° パンチカット（旧型フリスクケース向け）
- RP2350A QFN-60 ベアチップ（ソケット/モジュールスタックではない）
- USB-C ×2 + USB-A ×1、CH9329 を UART 接続

**注意:** 新基板は外形・配置・ネット割り当てのドラフトで、配線と回路図は
未完了です。製造発注には使用しないでください。旧 `rp2350_zero_carrier` の
製造データは履歴として同梱しています。
