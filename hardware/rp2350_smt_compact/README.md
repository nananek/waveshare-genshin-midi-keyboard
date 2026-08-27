# RP2350 SMT Compact 1-Board (50×35mm)

現行 `../rp2350_zero_carrier/` (56.5×59.5mm, 2層, THTピンソケット2枚)を、**物理1枚・中身RP2350 2チップ集約**のSMTコンパクト試作として再設計した基板。見た目確認用（試作レベル）だが回路上はJLCでそのまま発注可能。

## 特徴

- **サイズ 50×35mm (1750mm²)** = 現行比48%削減。2層 1.6mm FR4 1oz維持
- **RP2350-Zero 2個を半穴SMT直付け（キャスタレーション）で縦並び**。J1=ボード1(上)、J3=ボード2(下)。J3は180°回転でUSB-Cを外側へ
- **全て手ハンダ可能パッケージ**: 0603/0805, SOT23-5/6, SMA, SOIC-8相当のみ。QFN/BGA/0201不使用
- **USB逆電流対策**: U1 TPS2051B SOT23-5 (J1 5V→J2 VBUS, 70mΩ, ENはGNDプルで常時ON) + D1 SS14 SMA (J3側理想ダイオード代替) + F1/F2 MF-R050 PTC
- **VBUSバルク**: C1/C3 10µF 0805 + C2/C4 0.1µF 0603 を各VBUSに
- **ESD**: U2/U3 USBLC6-2SC6 SOT23-6 をUSB-Aと将来USB-C(D+/D-予備)に各1
- **スイッチ**: THTスライド×5＋タクト×1を **SMD SS-12D00G3 + PTS645 6×6** に置換
- **抵抗**: 22Ωを **0603 SMD** に。D+/D-は差動90Ωで5mm以内長合わせ（FS 12Mbpsで余裕）
- **USB Type-C**: Zero内蔵のまま（QFN直載せ第2弾ではGCT USB4105-GF-Aを検討、今回はZero直付けのため追加不要）
- JLCPCB Basic部品優先 (TPS2051B C51187, SS14 C57595, USBLC6 C7516, 0603/0805はBasic)

## 配置概要 (上面 F.Cu, 原点=基板中央)

```
 +Y (USB-C top)
 ┌─────────────────────────────┐
 │ H1(22,14.5)  SW1 SW2 SW3  H2 │  Y=+17.5 Edge
 │          [  J1  RP2350-Zero  ] │  J1 at (0,8)  USB-C↑
 │ C3 C4  [F2]  [D1]  [U1]      │  C3/C4=J3 bulk, D1=SMA, U1=SOT23-5
 │  U2(ESD)   R1 R2  U3(ESD)   │  R1/R2=0603 22Ω near USB diff
 │          [  J3  RP2350-Zero  ] │  J3 at (0,-8) USB-C↓ (180°)
 │ SW6   TP1   [  J2 USB-A  ]  SW4 │  J2 at (0,-14.2) bottom edge
 │ H3                  H4      │  Y=-17.5 Edge
 └─────────────────────────────┘
 -X=-25                      +X=25
```
- 縦並び: J1とJ3の間隔は約16mm（モジュール間クリアランス2mm）。UARTミラーは基板内配線 GP4(J1 pad14)→GP5(J3 pad15相当)で直結（従来通り）
- VBUS幅 1.0mm以上、GNDはベタ。ネジ穴は四隅 M2.5 2.7mm
- 全SMDはTop面に集約（JLC PCBA Topのみ）

## 回路メモ

```
J1(Zero1) 5V ── F1(MF-R050) ── U1(TPS2051B) ── J2 USB-A VBUS
                │              ├── EN=GND(10k PD, 常時ON)
                │              └── GND
                └─ C1 10µF + C2 0.1µF (VBUS-GND)
J3(Zero2) 5V ── F2(MF-R050) ── D1(SS14) ── (将来iPad用USB-C VBUS, 現状NC)
                └─ C3 10µF + C4 0.1µF
J1 GP12→R1(22Ω)→J2 D+  (U2 ESD: D+→I/O1, D-→I/O2, VBUS/GND)
J1 GP13→R2(22Ω)→J2 D-  U3はZero内蔵USB-CのD+/D-予備（将来QFN時にGCT USB4105のD+/D-へ）
```

- SS14は外部→基板への逆流のみブロック（Vf 0.35VでもHost給電4.4V許容）
- TPS2051BはHigh-sideスイッチで過電流自動遮断、EN=LowでON
- 現行のSW4はU1のEN制御に置換可能だが、互換のためSW4をU1前段に残す選択も可（BOMではSW4を残置）

## 発注手順 (JLCPCB)

1. KiCadで Gerber (F.Cu/B.Cu/F.Mask/B.Mask/F.Silk/B.Silk/Edge.Cuts) + Drill + Pos(CPL) + BOM(CSV) を出力
   - Plot: `File → Plot → Gerber`, `Generate Drill Files`, `Fabrication Outputs → BOM/CPL`
   - 本リポジトリの `bom.csv` はLCSC型番併記済み
2. JLCPCBで `PCB + PCBA` を選択、50×35mm 2層 1.6mm HASL or ENIG
3. SMT Assembly: Top Sideのみ。BOM/CPLアップロード。Zero 2個は `Customer Supplied Part` または在庫確認（無ければ手ハンダ）
   - U1/D1/U2/U3/C/R/SWは全てJLC Basicなので追加料金最小
   - USB-A(J2)とF1/F2はTHTのためPCBA対象外→手ハンダ（JLCのTHT Assemblyを選ぶ手もあり）
4. JLCオプション: Castellated Holes=Yes (Zero半穴)
5. 5枚発注で基板$2 + PCBA $8 + 部品$5 ≈ $15 (¥2200)。Zero持ち込みなら別途

## レンダリング/3D確認

- KiCad 7/8で `View → 3D Viewer` (Alt+3) または `kicad-cli pcb render` で確認
- `kicad-cli pcb export --format step` でSTEP出力可
- Edge.Cuts寸法は `Inspect → Measure` で 50×35mmを確認

## 手ハンダメモ

全パッケージが0603/0805/SOT23-5/6/SMA/radial PTCで、フラックス＋鏝先0.8mmで手直し可能。QFN/BGA/0201は使っていない。半穴は鏝を横から当ててフィレットを作る。U1の1pinマーク(ドット)をF.SilkSで確認。

## 今後の拡張 (第2弾 QFN案)

- RP2350 QFN60直載せ + W25Q128 SOIC-8 + 12MHz 3225水晶 + RT9013 SOT23-5 LDO + GCT USB4105-GF-A Type-C (JLCで委託)
- さらに40×30mmを目指せるが、手ハンダ難易度が上がるため本基板で量産可否をまず検証する
