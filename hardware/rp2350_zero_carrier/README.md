# RP2350-Zero carrier 組立

> **2026-08-30電源制御版:** 発注判定と実測waiverは [ORDER_DECISION_JA.md](ORDER_DECISION_JA.md) と [VALIDATION_WAIVERS_JA.md](VALIDATION_WAIVERS_JA.md) を正本とします。TPS2553はGP9 EN / GP10 FAULT_N、FAULT 3V3 pull-up、EN 100kΩ pull-down、OUT 100nF、5測定padの構成です。外部電源入力とSW3流用は禁止します。

## J1/J3の向きは必ず確認する

J1/J3の穴は左右対称に見えるため、RP2350-Zeroを裏返しても仮置きでは穴が合います。しかしその向きで半田付けすると、5VとGPIOなどが鏡映され、重大な誤配線になります。ピンソケットは取り外せない前提で、次の確認を半田付け前に必ず行ってください。

1. キャリアの **B.Cu（裏面）** を上にし、B.Silkscreenの `USB-C / BOOT`、太い矢印、V字ノッチ、`1=5V` から **+Y側（横5穴列の反対端）** とpin 1を確認します。
2. +Y側を見失わないように基板を裏返して **F.Cu（表面）** を上にし、メスソケットをその表面に仮置きします。RP2350-ZeroのUSB-C/BOOT/RUN側を、手順1で確認した+Y側へ向けます。
3. Zeroは、USB-C・BOOT/RUN・LEDがある**部品面をキャリアから外側（上）**、オスピンを下にして仮挿入します。USB-C/BOOT/RUNが+Y側にあり、BOOT/RUNとLEDが上から見えることを確認します。
4. 反対面から見てBOOT/RUNがキャリア側に隠れる場合は中止してください。その姿勢は穴が合っても誤りです。
5. 仮挿入を外し、メスソケットをF.Cu面から片端の1ピンだけ仮止めします。向きと直角を再確認してから残りを半田付けします。

実装後は、J1/J3ともUSB-Cが矢印側に出ていてBOOT/RUNを押せることを確認してください。

## USB-A VBUS保護回路

USB-Aの電源は `J1 5V → F1 → U1 (TPS2553) → SW4 → C2 → J2 VBUS` の順に通ります。F1はリセッタブルヒューズ、U1は電流制限・短絡保護付きのハイサイドスイッチ、SW4はUSB-Aへの手動給電スイッチです。

- TI Rev.F Table 2の500mA行はideal 52.5kΩ、closest 1% 52.3kΩ、actual min/nom/max 448.3/501.6/562.4mAです。本設計は`R3 = 52.3kΩ 1%`を採用します。これは異常時の保護閾値であり、接続したMIDI機器へ500mAを保証するものではありません。通常消費の小さいUSB MIDIキーボードを対象にし、不明な機器は消費電流を確認してください。
- ボード1のUSBコンフィグレーションディスクリプタは、RP2350本体とUSB-A接続機器を合わせた上流側の最大値として500 mAを申告します。電源供給元の能力を増やす設定ではないため、無給電ハブや大電流機器は接続しないでください。
- `USB_PWR_FAULT_N` はU1のオープンドレインFAULTを100kΩで3V3へプルアップし、J1 pad22=GP10へ接続します。ENはJ1 pad23=GP9で制御し、R5=100kΩによりboot default offです。
- `C1` はU1入力の直近に置く1uFセラミックコンデンサです。IN/GNDの両枝はF.CuだけでU1へ接続し、局所バイパスloopにviaを含めません。
- `C3` はU1 OUT-GND間の100nF X7Rセラミックコンデンサです。TP_EN/TP_FAULT/TP_IN/TP_OUT/TP_GNDを受入測定に使います。
- `C2` はUSB-Aコネクタ付近の220uF電解コンデンサです。**pad 1 / `+` を `VBUS_USB_A_SW`、pad 2 / `-` をGND**へ実装してください。逆挿しは禁止です。

部品値・パッケージの根拠は、[TI TPS2553データシート](https://www.ti.com/lit/ds/symlink/tps2553.pdf)と[TI DBV0006Aパッケージ図](https://www.ti.com/lit/ml/mpds026v/mpds026v.pdf)です。C2は[秋月電子のRubycon 220uF 16V品](https://akizukidenshi.com/catalog/g/g110326/)に合わせて直径6.3 mm・ピッチ2.5 mmとしています。

## R2は裏面へ実装する

USB D-の直列抵抗R2は、そのリード間のF.Cu側にU1/C1があるため、**抵抗本体をB.Cu（裏面）側へ寝かせて挿入し、F.Cu側から半田付け**します。B.Silkscreenの `R2` referenceと抵抗本体の外形が見える側が実装面です。R1は従来どおりF.Cu側へ実装します。

## U1型番の選定理由(ラッチオフ版を採用しなかった理由)

`TPS2553DBVR`(通常版)を採用し、`TPS2553DBVR-1`(IC内ラッチオフ版)は採用していません。通常版のFAULTをGP10のfalling-edge IRQとboot時sampleで監視し、**最初のFAULT_N LowでENを即時OFFしてRAM latchします。自動再投入は一切しません。** latchはUSB detach・再列挙・suspend/resume・SW4操作・機器抜去・BOOTボタン・SW3では解除されず、RP2350のRUN/RESETまたは電源再投入による実rebootだけで初期化されます。電源制御initはBSS one-shot guard付きで、通常実行中の再呼出しでも解除できません。reboot後もnative USBが再びconfiguredになるまではEN Lowです。

## EN制御・ILIM設定の根拠

- `EN`(pin3)は`USB_PWR_EN`としてJ1 pad23=GP9へ接続し、R5=100kΩでGNDへpull-downします。ファーム初期化でもGP9の出力ラッチをLowにしてから出力化し、native USBがconfiguredかつ非suspend、かつFAULT latchなしの場合だけHighにします。
- `R3 = 52.3kΩ`(1%)は、TI Rev.F Table 2「Common RILIM Resistor Selections」の500mA行にあるideal 52.5kΩに対するclosest 1%値です。同じ行のactual値は448.3〜562.4mA（nom 501.6mA）です。独自計算値や49.9kΩ行との近似ではありません。

## VBUS配線幅の根拠(IPC-2221)

`USB_VBUS_PWR` net class(`VBUS_5V` / `VBUS_TPS_IN` / `VBUS_USB_A` / `VBUS_USB_A_SW`)は基板既定の0.2mmから1.0mm(外層・1oz銅相当)へ拡幅しています。IPC-2221外層トレース幅の式(I = k・ΔT^0.44・A^0.725, k=0.048)で計算すると、1oz・ΔT10℃・外層条件で0.2mmは約0.74A、1.0mmは約2.4Aまで許容できます。ILIM実力値の上限(562.4 mA)に対し1.0mmは十分な余裕(約4倍)を持たせた値です。

- `J1 5V → F1 → U1 → SW4 → C2/J2`の主電流経路は1.0mmです。従来0.2mmだった`J1 5V → F1`は、J1/J3のヘッダーパッド列の中央へ引き直してクリアランスを保ったまま1.0mm化しました。
- U1のINピン直近は、隣接するGNDピンおよびGNDビアとのクリアランスを確保するため、約1.5mmだけ0.4mmへテーパーさせています。C1/EN/FAULTの枝も負荷電流の主経路ではないため0.4〜0.6mmです。これらを除く主電流経路は1.0mmです。

## GND強化

B.Cu全面をGNDゾーン化してリフィルしました。USBの負荷電流は`J2 GND/C2 → B.Cu GNDプレーン → J1 GND`へ戻るため、信号GNDも含むネット全体を1.0mmのネットクラスへ変更する必要はありません。GNDネットは`Default`のままですが、ゾーンのサーマルスポーク幅は0.5mm、最小スポーク数は2本で、J2のシェルへ向かうプレーン外のF.Cu枝は1.0mmです。既存のGNDビアは1本のみでしたが、U1/C1/R3周辺への追加ステッチビアにより、現在GNDネットのビアは4本(F.Cu-B.Cu貫通)です。ゾーンリフィル後に`kicad-cli pcb drc --refill-zones --save-board`でDRCを再実行し、0違反であることを確認しています。

## D+/D-配線は現状維持(変更していません)

今回のUSB-A VBUS保護回路追加ではD+/D-ネット(`GP12_USB_DP`, `GP13_USB_DM`, `USB_A_DP`, `USB_A_DM`)には一切手を加えていません。既存のPIO-USBホスト動作(Full-Speed列挙)を壊すリスクを避けるためです。

- 現状の配線長: D+ 約33.5mm(`GP12_USB_DP` 19.6mm B.Cu + `USB_A_DP` 13.9mm F.Cu、層をまたぐ)、D- 約44.7mm(`GP13_USB_DM` 15.9mm + `USB_A_DM` 28.9mm、B.Cu内完結)。長さ差は約11.2mmで、D+のみ層をまたぐ非対称構成です(実測はworkerAの計画書による)。
- 改善(等長化・同一層化)を見送った理由: GND/VBUS強化のための再配線・ゾーン追加が、この非対称だが**現に動作しているD+/D-配線**を意図せず分断・再ルーティングしてしまうリスクの方が、USB Full-Speedの電気的マージン改善より大きいと判断したためです。
- **実機でのUSB Full-Speed列挙確認が必要です**。今回D+/D-は変更していませんが、GND/VBUS周りの改版そのものが基板改版であり、実機での再検証(USB MIDIキーボードの列挙・演奏確認)を組立後に必ず行ってください。

## SW3・SW4・ENの扱い(今回の結論)

SW4はU1 OUTとC2/J2の間にある手動VBUS遮断として維持します。ENはGP9による自動制御です。SW3はpad1 NC/pad2 GP27/pad3 GNDの予備スイッチのままで、電源制御へ流用しません。外部電源入力は追加しません。

## JLCPCB向けデータ

`jlc_bom.csv` / `jlc_cpl.csv` はJLCPCBのSMD自動実装(CPL)に対応する6部品(`U1`, `C1`, `R3`, `R4`, `R5`, `C3`)のみを収録しています。BOMとCPLのDesignatorは完全一致し、`scripts/hardware_contract.py`が値・座標・回転を検査します。

- **JLC実装(SMD)対象**: `U1`(TPS2553DBVR), `C1`(0603), `R3`(0603), `R4`(0603), `R5`(0603), `C3`(0603)。
- **手はんだ(THT)対象**: `J1`〜`J3`(ピンソケット/ヘッダー), `F1`(ラジアルPTCヒューズ), `SW1`〜`SW6`(スライド/タクトスイッチ), `R1`/`R2`(axial 22Ω), `C2`(ラジアル電解コンデンサ、220uF)。いずれもCPLには含めていません。
- **exact Manufacturer/MPN/LCSC**: `U1`=Texas Instruments/TPS2553DBVR/C55266、`C1`=FH/0603B105K160NT/C93816、`R3`=UNI-ROYAL/0603WAF5232T5E/C23198、`R4`と`R5`=YAGEO/RC0603FR-07100KL/C14675、`C3`=YAGEO/CC0603KRX7R9BB104/C14663。正式な6行表と発注チェックリストは`ORDER_DECISION_JA.md`を参照し、代替を禁止してください。
- SOT-23-6(U1)のピン1向きはTIデータシートSLVS841Fの"Pin Configuration and Functions"(DBVパッケージ, top view: 左列上から IN/GND/EN、右列上から OUT/ILIM/FAULT)に基づいて自作footprintを作成しています。発注時にJLCPCBの回転補正表と必ず突き合わせてください(特にR3は180°回転で配置しているため要確認)。

## ERC/DRC/parity

`kicad-cli sch erc --severity-error`は0 errorです。`--severity-all`は19 warningで、内訳はこのコンテナにKiCad標準ライブラリが無いことを示す`lib_symbol_issues=15`（Device 8、Switch 5、Connector_Generic 2）と、PCB-only UART test headerへ渡す意図的な単独端点`isolated_pin_label=4`です。前者は環境依存警告であり誤検知とは呼ばず、releaseへ全件保存します。J1/J3の専用23-pin、TPS2553、USB-Aシンボルはproject-local libraryと一致し、off-grid/multiple-net-name/mismatch警告は0です。設定どおりのPCB DRCは0、unrouted 0、KiCad native schematic parity 0です。既存ignore 5種を一時的にwarning化したstrict DRCは`missing_courtyard=21`だけを検出し、track-via中心ずれ、footprint filter/type、tuning geometryは0でした。courtyard未整備はWVR-010として実物dry-fitでcloseします。ERC ignore 4種のstrict確認で増えたのも同じ単独UART label 4件だけです。

## Release PDF paper size

Run `scripts/build_gerbers.sh` to regenerate the release drawings. The
schematic source and release PDF are native ISO A4 landscape. Native KiCad
netlist/BOM exports are retained in the release and rejected if annotation
produces a `?` or a nonstandard reference. The PDF is also rendered at 180dpi
and compared with the manually reviewed golden digest while critical field
positions are checked for proximity. The five selected PCB layers are
exported on separate A4 pages at physical scale 1:1 after translating a
temporary copy into the page area, so every page is centered without clipping
or changing the board dimensions.
