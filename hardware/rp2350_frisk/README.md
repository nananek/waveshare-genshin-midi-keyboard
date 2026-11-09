# RP2350 Frisk MIDI/CH9329 gadget 基板

旧型フリスクケースに収めるための、**RP2350A ベアチップ + CH9329** の
1 枚基板です。従来の「RP2350-Zero を 2 枚ソケットでスタックする」構成を
置き換える設計方針を示すための **配置・外形ドラフト** です。

> **重要: この基板はまだ製造可能な完成品ではありません。**
> 外形・部品配置・ネット割り当ては確定していますが、配線（ルーティング）は
> 未実施です。`drc.rpt` の `unconnected_items` 141 件はそのためです。
> 発注前に配線、ベタ、D+/D- の長さ確認、ERC/DRC 0 件化が必要です。

## 外形

`nananek/serial-km-emulator` と同じ外形に合わせています。

- **57.6 × 31.4 mm、t1.6 mm**
- 四隅 **2 mm の 45° パンチカット**
- 4 層: `F.Cu` / `In1.Cu`（GND プレーン予定）/ `In2.Cu`（電源予定）/ `B.Cu`
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
- **U5**: TPS2051BDBVR。USB-A 側 VBUS の 500 mA 電流制限。EN は GND
  （常時 ON）。J2 の VBUS はここから供給。
- **D1/D2**: BAT54WS。J1 VBUS と J3 VBUS のダイオード OR。どちらの
  USB-C から給電されても `+5V` が立ち、相互逆流を防ぐ。
- **U6**: USBLC6-2SC6。USB-A ホスト D+/D- の ESD 保護。
- **TP1/TP2**: CH9329 の SET/GND。設定ツールを使うときだけ短絡する。
- **TP3/TP4**: GP28 / GP29。任意の原神モード／楽器モードスイッチ用。

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

`bom.csv` は KiCad の PCB から生成した参考 BOM です。`TBD` の LCSC は
発注前に確定してください。とくに RP2350A、W25Q128、NCP1117、水晶、
USB-A コネクタは JLCPCB の在庫・実装可否を確認してください。

## 既知の未完了項目（発注前に必須）

1. **配線**: 現状は配置のみ。`+3V3`、`+5V`、GND、QSPI、USB、UART を配線する。
2. **GND/電源プレーン**: 4 層の `In1.Cu` に GND、`In2.Cu` に 3V3/5V を
   置く想定。ベタの分割とビア配置を決める。
3. **D+/D-**: J1/J2/J3 の差動ペアを同一層・等長気味に配線する。
   Full-Speed なので 90Ω 制御は必須ではないが、ペア長差は詰める。
4. **USB-A VBUS**: U5 の IN/OUT と J2 の VBUS を 1.0 mm 以上で配線する。
5. **CH9329**: U4 の VCC/V33 デカップリングを直近に置く。UART は 9600 8N1。
6. **スイッチ**: TP3/TP4 を実際のスイッチにするか、テストパッドのままにするか決める。
7. **ERC**: 回路図は未作成。KiCad の回路図 → PCB パリティを通すには
   ネットリストとシンボルを作る必要がある。
8. **ケース開口**: 旧型フリスクケースの USB-C ×2 / USB-A ×1 の開口位置を
   実物で確認する。

## 検証

KiCad 10.0.5 で外形・配置の DRC を実行済みです。

```sh
kicad-cli pcb drc --severity-error rp2350_frisk.kicad_pcb
# Found 0 violations
# Found 141 unconnected items（配線未実施のため）
```

`drc.rpt` に結果を保存しています。物理的なクリアランス／短絡／
基板端／ドリル径のエラーは 0 です。`unconnected_items` は配線を入れれば
解消します。

## 旧設計との関係

`../rp2350_zero_carrier/` は RP2350-Zero を 2 枚ソケットで載せる旧設計です。
本基板はそれを置き換える新方針のドラフトであり、旧基板の製造データや
契約テスト（`scripts/hardware_contract.py` など）は旧設計向けのままです。
新基板をリリース対象にするときは、契約テストと CI をこの基板向けに
作り直してください。

## ライセンス

RP2350A 周辺の初期設計は Raspberry Pi 公式
`RP2350A QFN-60 Minimal Design Example R4` を参照しています。
`official-minimal-LICENSE.txt` を同梱しています。
