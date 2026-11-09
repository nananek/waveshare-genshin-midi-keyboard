## Firmware

- `midi_nkro_ch345.uf2` は新しい 1 枚構成用です。
  - PIO-USB ホストで MIDI キーボードを受信
  - native USB を NKRO HID キーボードとして操作対象 PC へ接続
  - UART1 (GP4/GP5, 31250 8N1) から CH345 へ生 MIDI を送り、制御 PC/DAW へ USB-MIDI を出力

旧 `genshin_midi_kbd.uf2` / `serial_midi_device.uf2` の 2 ボード構成は廃止しました。

## Hardware

`hardware/rp2350_frisk/` に CH345 版の基板と発注データがあります。

- 57.6 × 31.4 mm、四隅 2 mm 45° パンチカット、**6層**
- RP2350A QFN-60 ベアチップ（ソケット/モジュールスタックではない）
- USB-C ×2 + USB-A ×1、CH345T を UART 31250 で接続
- Gerber / BOM / CPL は `hardware/rp2350_frisk/fab/`。DRC は 0 violations /
  0 unconnected
- U1 pin22 と J3 A7/B7 に via-in-pad を使用。JLCPCB の DFM で resin fill /
  via-in-pad の要否を確認してください。
- BOM の `TBD`（CH345T、RP2350A、W25Q128、NCP1117、水晶、USB-A など）は
  発注前に確定してください。
- 旧 `rp2350_zero_carrier` の製造データは履歴として同梱しています。
