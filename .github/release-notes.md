## Firmware

- `midi_nkro_ch345.uf2` は新しい 1 枚構成用です。
  - PIO-USB ホストで MIDI キーボードを受信
  - native USB を NKRO HID キーボードとして操作対象 PC へ接続
  - UART1 (GP4/GP5, 31250 8N1) から CH345 へ生 MIDI を送り、制御 PC/DAW へ USB-MIDI を出力

旧 `genshin_midi_kbd.uf2` / `serial_midi_device.uf2` の 2 ボード構成は廃止しました。

## Hardware

`hardware/rp2350_frisk/` の CH345 版基板が v2.0.0 の対象です。発注データは次の
ファイルで添付しています。

- `rp2350_frisk_ch345_gerbers_JLCPCB.zip` … Gerber / Excellon（6層 + ドリル + job）
- `rp2350_frisk-jlc-bom.csv` / `rp2350_frisk-jlc-cpl.csv` … JLCPCB 発注用 BOM / CPL
- `rp2350_frisk-bom-full.csv` … 参考用の全部品表
- `rp2350_frisk-drc.rpt` … KiCad 10.0.5 の DRC（0 violations / 0 unconnected）
- `rp2350_frisk-placement.pdf` … 配置図
- `rp2350_frisk-ORDER_NOTES_JA.md` … 発注前の確認事項

基板仕様は 57.6 × 31.4 mm、四隅 2 mm 45° パンチカット、6層です。RP2350A
QFN-60 ベアチップ、CH345T を UART 31250 で接続し、USB-C ×2 + USB-A ×1 を
載せています。

U1 pin22 と J3 A7/B7 に via-in-pad を使用しています。JLCPCB の DFM で
resin fill / via-in-pad の要否を確認してください。BOM の LCSC 型番は在庫と
パッケージで確認済みです。J1 (Amphenol 10103594-0001LF) と J2 (Neltron
5075AR-04-WH) は JLC に在庫がないため、BOM では `TBD` のままにして手はんだ
してください。

旧 `rp2350_zero_carrier` の製造データは履歴として添付しています。旧基板は
廃止済みで、v2.0.0 の実装対象ではありません。
