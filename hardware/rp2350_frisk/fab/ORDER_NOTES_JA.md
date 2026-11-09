# RP2350A + CH345 Frisk 基板 発注メモ

## 同梱ファイル

- `gerbers/` … KiCad 10.0.5 の Gerber / Excellon。6層（F.Cu, In1–In4, B.Cu）。
- `rp2350_frisk_ch345_gerbers_JLCPCB.zip` … 上記 Gerber / Drill の ZIP
- `jlc_bom.csv` … JLCPCB BOM
- `jlc_cpl.csv` … JLCPCB CPL（基板左下原点、Y 上向き）
- `bom_full.csv` … 参考用の全部品表
- `drc.rpt` … KiCad DRC 結果（0 violations / 0 unconnected）

## 基板仕様

- 57.6 × 31.4 mm、t1.6 mm、**6層**
- 四隅 2 mm 45° パンチカット
- 表面・裏面の両面 SMT
- J1 = native USB **NKRO HID**（操作対象 PC）
- J2 = USB-A ホスト（MIDI キーボード）
- J3 = CH345 の **USB-MIDI**（制御 PC/DAW）
- U4 = CH345T（SSOP-20）、X2 = 12 MHz 水晶
- TP5 = GP27 予備スイッチ（旧サステインスイッチの移設先）

## 製法上の注意

- **ビア径**: 0.45/0.2 mm と 0.3/0.2 mm を使用しています。0.2 mm ドリルは
  6層基板の標準範囲ですが、JLCPCB の見積で advanced 扱いにならないか確認してください。
- **Via-in-pad**: U1 pin22 (/XOUT) と J3 の A7/B7 (USB_HID_DM) に小さな
  ビアをパッド内に置いています。JLCPCB では resin fill / via-in-pad の
  オプションが必要になる場合があります。DFM プレビューで確認してください。
- **最小線幅/間隔**: 0.1 mm。6層基板の標準範囲です。
- CH345T は SSOP-20 5.3 × 7.2 mm、0.65 mm pitch です。
- 水晶 X2 の負荷容量 C26/C27 は 22 pF の暫定値です。CH345 データシートの
  推奨値に合わせて確定してください。

## BOM の TBD

`jlc_bom.csv` の `LCSC Part #` が `TBD` の部品は、JLCPCB の在庫確認後に
型番を確定してください。とくに以下は要確認です。

- CH345T（U4）
- RP2350A（U1）
- W25Q128JVSIQ（U3）
- NCP1117ST33T3G（U2）
- 12 MHz 水晶（X1, X2）
- USB-A コネクタ（J2）
- 15 pF / 22 pF / 4.7 uF / 22Ω / 33Ω / 27Ω / 0Ω / 10 kΩ の受動部品

確定済み: USB-C (J1/J3, C165948)、USBLC6 (U6/U7, C7519)、TPS2051B (U5, C51187)、
BAT54WS (D1/D2, C22629)、100 nF (C14663)、10 uF 0805 (C15850)、
5.1 k (C23186)、1 k (C21190)。

## CH345 のボーレート

CH345 の UART は 31250 bps 固定です。ファームは生 MIDI をそのまま流します。
