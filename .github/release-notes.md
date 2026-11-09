## Firmware

- `midi_nkro_ch345.uf2` は新しい 1 枚構成用です。
  - PIO-USB ホストで MIDI キーボードを受信
  - native USB を NKRO HID キーボードとして操作対象 PC へ接続
  - UART1 (GP4/GP5, 31250 8N1) から CH345 へ生 MIDI を送り、制御 PC/DAW へ USB-MIDI を出力

旧 `genshin_midi_kbd.uf2` / `serial_midi_device.uf2` の 2 ボード構成は廃止しました。

## Hardware

`hardware/rp2350_frisk/` に新基板の外形・配置ドラフトがあります。

- 57.6 × 31.4 mm、四隅 2 mm 45° パンチカット（旧型フリスクケース向け）
- RP2350A QFN-60 ベアチップ（ソケット/モジュールスタックではない）
- USB-C ×2 + USB-A ×1、CH345 を UART 接続（基板は CH345 版へ再設計中）

**注意:** ファームは NKRO HID + CH345 に切り替わりました。現在の
`hardware/rp2350_frisk/fab/` は旧 CH9329 試作のデータで、このファームとは
互換性がありません。CH345 のピン配置を確定して基板を再配線するまで、
`fab/` を発注しないでください。旧 `rp2350_zero_carrier` の製造データは
履歴として同梱しています。
