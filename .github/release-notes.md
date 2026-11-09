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

**注意:** 新基板の Gerber / BOM / CPL は `hardware/rp2350_frisk/fab/` に
あります。DRC の物理エラーは 0 ですが、`/QSPI_SCLK`、`Net-(U1-USB_DP)`、
`Net-(C4-Pad1)` の 3 ネットは未接続で、組立後に細いワイヤで手配線する必要が
あります。詳細は `ORDER_NOTES_JA.md` を参照してください。旧
`rp2350_zero_carrier` の製造データは履歴として同梱しています。
