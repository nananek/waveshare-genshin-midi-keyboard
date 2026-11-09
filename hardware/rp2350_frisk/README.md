# RP2350 Frisk MIDI/CH9329 gadget 基板

旧型フリスクケースに収めるための、**RP2350A ベアチップ + CH9329** の
1 枚基板です。従来の「RP2350-Zero を 2 枚ソケットでスタックする」構成を
置き換える **発注データ付きの試作基板** です。

> **CH345 版の発注データを `fab/` に生成済みです。**
> DRC は 0 violations / 0 unconnected です。ただし U1 pin22 と J3 の
> A7/B7 に via-in-pad を使っているため、JLCPCB の発注時に resin fill /
> via-in-pad の要否を DFM プレビューで確認してください。
>
> 発注ファイル:
> - `fab/rp2350_frisk_ch345_gerbers_JLCPCB.zip`
> - `fab/jlc_bom.csv`
> - `fab/jlc_cpl.csv`
> - `fab/ORDER_NOTES_JA.md`

## 外形

`nananek/serial-km-emulator` と同じ外形に合わせています。

- **57.6 × 31.4 mm、t1.6 mm**
- 四隅 **2 mm の 45° パンチカット**
- **6 層**: `F.Cu` / `In1.Cu` / `In2.Cu` / `In3.Cu` / `In4.Cu` / `B.Cu`
- 固定穴なし（紙ケース／両面テープ／スポンジ固定を想定）

外形は `rp2350_frisk.kicad_pcb` の `Edge.Cuts` が正本です。
`docs/placement.pdf` / `docs/placement.png` に配置図があります。

## 構成

```
[MIDI キーボード] --USB-A(J2)--> PIO-USB host (GP12/13)
                                      |
                                      +--> native USB-C(J1) = USB-MIDI ガジェット
                                      |
                                      +--> UART1 GP4/GP5 9600 8N1 --> CH9329(U4)
                                                                        |
                                                                        +--> USB-C(J3) = HID → 操作対象 PC
```

- **J1 (左短辺, USB-C)**: RP2350A の native USB。PC/DAW には USB-MIDI
  ガジェットとして見える。5 V 供給元にもなる。
- **J2 (上長辺, USB-A)**: PIO-USB ホスト。MIDI キーボードを接続する。
  VBUS は TPS2051B (U5) の電流制限付きスイッチ経由。
- **J3 (右短辺, USB-C)**: CH9329 の USB HID。操作対象 PC（原神を動かす PC）
  に接続する。CC は 5.1 kΩ の Rd。
- **U1**: RP2350A QFN-60 ベアチップ。12 MHz 水晶 (X1)、QSPI フラッシュ
  (U3, W25Q128JVSIQ)、3.3 V LDO (U2, NCP1117)、公式 Minimal Design の
  デカップリング／内部レギュレータ構成を踏襲。
- **U4**: CH9329 SOP-16。VCC=5 V、V33 に 100 nF。MODE0/1、CFG0/1、DEF、
  RST は未接続（内蔵プルで Mode 0 / プロトコルモード / 9600 bps）。
  9600 bps は 1 レポート約 14.6 ms なので、高速なトリルまで詰めたい場合は
  **TP1 (SET) と TP2 (GND) を短絡して WCH 設定ツールで 115200 bps に変更**する。
  ファームは起動時に GET_INFO を複数ボーレートで試して自動判定するため、
  再ビルドは不要。出荷時 9600 のままでも通常演奏は可能。
- **U5**: TPS2051BDBVR。USB-A 側 VBUS の 500 mA 電流制限。EN は GND
  （常時 ON）。J2 の VBUS はここから供給。
- **D1/D2**: BAT54WS。J1 VBUS と J3 VBUS のダイオード OR。どちらの
  USB-C から給電されても `+5V` が立ち、相互逆流を防ぐ。
- **U6**: USBLC6-2SC6。USB-A ホスト D+/D- の ESD 保護。
- **TP1/TP2**: CH9329 の SET/GND。設定ツールを使うときだけ短絡する。
- **TP3/TP4/TP5**: GP28（原神モード）/ GP29（楽器モード）/
  GP27（予備スイッチ）。GP27 は旧ボード2のサステインスイッチの移設先で、
  既定では状態を読むだけ。`SPARE_SWITCH_CC=64` にするとサステイン相当。

## RP2350A ピン割り当て

| 機能 | RP2350A QFN-60 pin | GPIO | 接続先 |
|------|--------------------|------|--------|
| PIO-USB D+ | 16 | GP12 | R15 22Ω → U6 → J2 D+ |
| PIO-USB D- | 17 | GP13 | R16 22Ω → U6 → J2 D- |
| CH9329 RX (RP2350 TX) | 7 | GP4 | U4 pin 8 (RXD) |
| CH9329 TX (RP2350 RX) | 8 | GP5 | U4 pin 7 (TXD) |
| native USB D- | 51 | USB_DM | J1 D- |
| native USB D+ | 52 | USB_DP | J1 D+ |
| 原神モードスイッチ | 42 | GP28 | TP3 ↔ GND（任意） |
| 楽器モードスイッチ | 43 | GP29 | TP4 ↔ GND（任意） |
| QSPI | 55–60 | — | U3 |
| 水晶 | 21/22 | XIN/XOUT | X1 12 MHz |
| RUN / SWD | 26 / 24/25 | — | テストパッド未実装（必要なら追加） |

## 電源

```
J1 VBUS ── D1 ──┐
                 ├── +5V ── U2 (NCP1117) ── +3V3 ── RP2350A / flash / crystal
J3 VBUS ── D2 ──┘         │
                           ├── U4 CH9329 VCC
                           └── U5 TPS2051 IN ── OUT ── J2 USB-A VBUS
```

- J1/J3 のどちらか一方だけ給電でも `+5V` が立ちます。
- J2 の MIDI キーボードへは U5 の電流制限付き 5 V を出します。
- J1/J3 の VBUS にはそれぞれ 100 nF、`+5V` と USB-A VBUS には 10 uF を
  置いています。

## BOM

- `fab/jlc_bom.csv` … JLCPCB アップロード用 BOM
- `fab/jlc_cpl.csv` … JLCPCB アップロード用 CPL（基板左下原点、Y 上向き）
- `fab/bom_full.csv` … 参考用の全部品表

`TBD` の LCSC は発注前に確定してください。とくに RP2350A、W25Q128、
NCP1117、水晶、USB-A コネクタは JLCPCB の在庫・実装可否を確認してください。

## CH345 版の変更点

NKRO 必須のため、U4 の CH9329 を **CH345T** に置き換えました。CH345 は
UART 31250 ↔ USB-MIDI の専用ICです。パッケージは **SSOP-20 5.3×7.2 mm、
0.65 mm pitch** で、WCH の CH345 データシートに基づいています。

| CH345 pin | 機能 | 接続 |
|-----------|------|------|
| 1 | RST | GND |
| 3 | MOUT | RP2350 GP5 (UART1 RX) |
| 4 | MIN | RP2350 GP4 (UART1 TX) |
| 5 | V3 | 100 nF → GND |
| 6 | UD+ | USB-C J3 D+ |
| 7 | UD- | USB-C J3 D- |
| 8, 17 | GND | GND |
| 9 | XI | 12 MHz 水晶 X2 |
| 10 | XO | 12 MHz 水晶 X2 |
| 19 | VCC | +5 V |
| 2, 11–16, 18, 20 | NC | 未接続 |

追加部品:

- X2: 12 MHz 水晶（CH345 用。RP2350 の X1 とは別）
- C26/C27: 22 pF（水晶負荷容量。データシート推奨値で確定）
- U7: USBLC6-2SC6（J3 の D+/D- ESD）
- R17/R18: 5.1 kΩ（J3 の CC1/CC2 Rd。既存 R13/R14 を流用可）

J1 は native USB の NKRO HID、J2 は USB-A ホスト、J3 は CH345 の USB-MIDI
になります。U4 周辺の UART/USB/水晶は CH345 版として再配置・再配線済みです。

## 発注前の確認事項

1. **via-in-pad / 0.2 mm ドリル**: U1 pin22 と J3 A7/B7 に小さな via-in-pad を
   使っています。JLCPCB の DFM プレビューで resin fill / via-in-pad の要否を
   確認してください。
2. **BOM の TBD**: `fab/jlc_bom.csv` の `TBD` は JLCPCB の在庫確認後に
   LCSC 型番を確定する。とくに CH345T、RP2350A、W25Q128、NCP1117、水晶、
   USB-A コネクタ。
3. **CH345 の水晶負荷容量**: C26/C27 は 22 pF の暫定値。データシート推奨値で
   確定する。
4. **ERC**: 回路図は未作成。KiCad の回路図 → PCB パリティは未確認。
5. **ケース開口**: 旧型フリスクケースの USB-C ×2 / USB-A ×1 の開口位置を
   実物で確認する。

## 検証

KiCad 10.0.5 で外形・配置の DRC を実行済みです。

```sh
kicad-cli pcb drc --severity-error rp2350_frisk.kicad_pcb
# Found 0 violations
# Found 0 unconnected items
```

`fab/drc.rpt` に結果を保存しています。

## 旧設計との関係

`../rp2350_zero_carrier/` は RP2350-Zero を 2 枚ソケットで載せる旧設計です。
本基板はそれを置き換える CH345 版で、旧基板の製造データや契約テスト
（`scripts/hardware_contract.py` など）は旧設計向けのままです。
新基板をリリース対象にするときは、契約テストと CI をこの基板向けに
作り直してください。

## ライセンス

RP2350A 周辺の初期設計は Raspberry Pi 公式
`RP2350A QFN-60 Minimal Design Example R4` を参照しています。
`official-minimal-LICENSE.txt` を同梱しています。
