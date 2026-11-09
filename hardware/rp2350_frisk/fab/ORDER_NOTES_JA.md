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

`jlc_bom.csv` の LCSC 型番は JLC の在庫とパッケージで確認済みです。

- U1 RP2350A: C42411118
- U2 NCP1117ST33T3G: C26537
- U3 W25Q128JVSIQ: C97521
- U4 CH345T: C5246448（在庫 120 個。5 枚なら足りますが extended）
- U5 TPS2051BDBVR: C24593（旧 C51187 は JLC のカタログから消えています）
- X1 12 MHz 10 pF: C50430
- X2 12 MHz 12 pF: C70565
- L1 3.3 uH 0806: C98336
- R1 10 k: C25744、R3 33Ω: C25105、R7/R8 27Ω: C25100、
  R9/R10 0Ω: C17168、R15/R16 22Ω: C23345
- C3/C4 15 pF: C1548、C26/C27 22 pF: C1653、C6/C7/C9 4.7 uF: C23733
- 0402 の 100 nF: C307331、0402 の 1 k: C11702
  （0603 の C14663 / C21190 は 0402 パッドに合いません）

J1 と J2 は JLC に在庫がありません。BOM では `TBD` のままにして、JLC 側で
"Do not place" にして手はんだしてください。

- J1: Amphenol 10103594-0001LF（LCSC C9900136151 は取り寄せ・在庫 0）
- J2: Neltron 5075AR-04-WH（USB-A 基板用、THT）

J3 は HRO TYPE-C-31-M-12 (C165948) です。

## CH345 のボーレート

CH345 の UART は 31250 bps 固定です。ファームは生 MIDI をそのまま流します。
