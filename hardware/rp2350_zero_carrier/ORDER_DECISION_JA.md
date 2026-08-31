# RP2350-Zero carrier 発注判断書

## 結論

**試作発注は条件付きGO、量産発注はNO-GO**とする。静的設計、host test、RP2350実ビルド、Gerber/BOM/CPL生成は発注可能な水準まで完了した。一方、実機を必要とする列挙前電流、基板全体の500mA予算、短絡・温度、PC/iPad相互接続、JLC実プレビューは未実施であり、`VALIDATION_WAIVERS_JA.md` のwaiverを残したまま量産へ移行してはならない。

- 再検証基準SHA: `d9b0c0b75329aca04e70e7a3ddf5180ae0a3d1ac`（最新 `origin/main` をread-only fetchして確定）
- origin: `https://github.com/nananek/waveshare-genshin-midi-keyboard.git`
- 判定日: 2026-08-30 JST
- 対象: board 1のTPS2553電源制御、board 2とのVBUS分離、スイッチ、ファーム、JLC製造データ
- 禁止事項: 外部電源入力を追加しない。SW3を電源制御へ流用しない。

```text
git ls-remote --exit-code origin refs/heads/main
d9b0c0b75329aca04e70e7a3ddf5180ae0a3d1ac  refs/heads/main
```

## 今回確定・是正した不具合

| 分類 | 基準版の問題 | 是正と機械的根拠 |
|---|---|---|
| 確定不具合 | TPS2553 EN(pin3)が入力5Vへ直結され、USB列挙前から常時ON | ENを`USB_PWR_EN`へ分離しJ1 pad23=GP9へ接続。R5=100kΩをGNDへ追加。PCB `(9.7,-16.35)`、TP_EN `(8,-12.5)` |
| 確定不具合 | FAULT pull-upが5VでRP2350 GPIOへ直結不能 | R4=100kΩを3V3へ変更しJ1 pad22=GP10へ接続。R4中心 `(18,-12)`、TP_FAULT `(14.5,-11.5)` |
| 確定不具合 | TPS2553 OUT直近の高周波バイパスが無い | C3=100nF X7RをOUT-GND間へ追加。中心 `(15.25,-20.8)`、TP_OUT `(18,-20.05)`、TP_GND `(12,-21.5)` |
| 確定不具合 | 回路図がJ1/J3を20ピンで代用し、SW1–SW6の線端がピンに載っていなかった | project-local専用23-pin symbolへ変更し、J1 pad3/22/23、J3、SW1–SW6を実パッド番号どおりに再結線。KiCad schematic parity 0件 |
| 確定不具合 | 初回FAULTを100ms無視し、2回自動再投入し、detach/suspendでlatchを消していた | blanking/retry/counterを削除。最初のFAULT_N LowをGPIO IRQでEN Lowにし、RAM latchは実rebootだけで初期化。detach/suspend/resume/reconfigure、BOOT、SW3では解除しないhost testを追加 |
| 計画不適合 | C1局所loopが2 via/B.Cu迂回、入力測定padなし | C1を移動しIN=3.386mm/GND=5.260mmをF.Cuだけで接続。TP_IN `(6,-21.5)`を短いstubで追加 |
| 確定不具合 | 回路図のoff-grid端点59件と、D+/D−直列抵抗間で重なるラベル線 | 全端点をconnection gridへ整列し、R1/R2は各pinへ直接label。`endpoint_off_grid` / `multiple_net_names` / local symbol mismatchはいずれも0 |

変更前後の電源・制御ブロックは次のとおり。矢印は電力経路、縦線は制御または受入測定点を表す。

```text
変更前: J1 5V ── F1 ── U1 ── SW4 ── C2/J2 VBUS
                         EN=5V固定
                         FAULT──5V pull-up──GPIO（非許容）

変更後: J1 5V ── F1 ── U1 ── SW4 ── C2/J2 VBUS
                 C1      │OUT─C3
                         ├─ EN ── GP9  ── TP_EN
                         │        └─ R5 100kΩ ── GND
                         └─ FAULT_N ── GP10 ── TP_FAULT
                              └─ R4 100kΩ ── 3V3

J3: pad1(VBUS)=NC、pad3(3V3)=NC（J1/J2 VBUSへ接続しない）
```

## TPS2553一次資料との照合

TI `TPS2552/53 SLVS841F`: https://www.ti.com/lit/ds/symlink/tps2553.pdf

TI product/ordering page: https://www.ti.com/product/TPS2553/part-details/TPS2553DBVR

- DBV top viewはpin1=IN、2=GND、3=EN、4=FAULT、5=ILIM、6=OUT。PCB U1も同じネット順である。
- 発注型番`TPS2553DBVR`は2026-08-30確認時点でACTIVE、active-high、DBV/SOT-23-6、3000個large T&R、MSL1/260℃、top marking=`2553`。`TPS2553DBVR-1`（latch-off、別marking）への代替は禁止する。DBVに露出pad/pad7は無い。
- TPS2553のENはactive-high。R5によりリセット中はLow、ファームはGP9をUSB device configured後だけHighにする。
- FAULTはactive-low open-drain。R4=100kΩで3V3へpull-upしGP10で読む。RP2350内部pullは使用しない。
- IN-GNDには0.1µF以上をIC近傍に置く要求に対しC1=1µF X7R/16V。OUT側はC3=100nFと、SW4後段のC2=220µFを持つ。
- TI Rev.F Table 2の「Desired Nominal Current Limit 500mA」行は、ideal resistor=`52.5kΩ`、closest 1% resistor=`52.3kΩ`、actual min/nom/max=`448.3/501.6/562.4mA`である。本設計の`R3=52.3kΩ 1%`はこのclosest 1%値を採用する。これは短絡保護閾値のばらつきであり、基板全体500mAの保証値ではない。
- DBVの`rDS(on)`は25℃で85mΩ typ/95mΩ max、全温度135mΩ max。500mA時のIC導通損失概算は21.3mW typ、33.8mW worst、`RθJA=182.6℃/W`単純積で約3.9～6.2℃上昇。ただし電流制限中・周囲条件は実測waiver対象。
- TIのレイアウト指針に従いIN/OUT/GNDを幅広配線とし、C1/C3、ILIM、FAULTをU1周辺に集約した。C1 pad2 `(7.75,-21.00)`→U1 IN `(9.70,-18.25)`は3.386mm、C1 pad1 `(9.25,-21.00)`→U1 GND `(9.70,-17.30)`は5.260mmで、どちらもF.Cuだけを通り局所loopにvia/B.Cuを含まない。U1 OUT側 `(12.30,-18.50)`→C3 pad1 `(15.25,-20.05)`は4.282mm。`hardware_contract.py`は各経路のlayerと上限4/6/5mmを固定し、zone refill込みDRCでshort/clearance/unrouted 0を実証する。TP_INは `(6,-21.5)` から2.1mmのF.Cu stubとviaでF1後段のB.Cu幹線へ接続し、局所C1 loopへ枝を作らない。

DBV0006A機械図: https://www.ti.com/lit/ml/mpds026v/mpds026v.pdf

## ファームウェア契約

| 状態 | EN | WS2812(GPIO16) | 遷移 |
|---|---:|---|---|
| `WAIT_UPSTREAM` | Low | 消灯 | boot/detach/suspend |
| `ON` | High | 緑 | FAULT Lowで即OFF |
| `FAULT_LATCHED` | Low | 赤1Hz/25% duty | 最初のFAULT Lowでlatch。実reboot以外の出口なし |

WS2812はWaveshare RP2350-Zero一次資料のGPIO16をPIO2で駆動する。PIO-USBは固定依存のdefaultでPIO0/SM0–2を使うため競合しない。

- Waveshare製品/wiki: https://www.waveshare.com/product/rp2350-zero.htm / https://www.waveshare.com/wiki/RP2350-Zero
- Waveshare回路図: https://files.waveshare.com/wiki/RP2350-Zero/RP2350_Zero.pdf
- WS2812 PIOの由来: https://github.com/raspberrypi/pico-examples/tree/master/pio/ws2812

USB configuration descriptorは`bMaxPower=500mA`を申告する。EN判定は`tud_mounted() && !tud_suspended()`であり、vendored TinyUSB 0.20.0の`usbd.c`で`tud_mounted()`は`cfg_num != 0`、`usbd.h`でconnected-and-configuredと定義される。列挙前・detach・suspendではUSB-A VBUSを切る。FAULT falling-edge IRQはmain-loop処理より先にGP9をLowにし、boot時にもGP10をsampleする。電源制御initはBSS one-shot guard付きで通常実行中の再呼出しではlatchをclearできず、clear API・BOOT/SW3入力も存在しない。

## スイッチとVBUS分離

| 部品 | 役割 | 実ネット |
|---|---|---|
| SW1 | board 1 mute/game mode | pad1 NC、pad2 GP28、pad3 GND |
| SW2 | board 1 lyre mode | pad1 NC、pad2 GP29、pad3 GND |
| SW3 | 予備。今回も未使用 | pad1 NC、pad2 GP27、pad3 GND |
| SW4 | USB-A VBUSの手動遮断 | pad1 NC、pad2 TPS OUT、pad3 J2/C2 |
| SW5 | board 2 sustain | pad1 NC、pad2 J3 GP29、pad3 GND |
| SW6 | board 2 reset | pad1/2 GND、pad3/4 J3 GP28 |

- J1の上流VBUSはpad1=`VBUS_5V`で、`F1→U1→SW4→J2`の一方向経路だけを持つ。
- J3 pad1(VBUS)とpad3(3V3)はNCを維持する。J1/J3間はGND/UART/役割信号のみで、PCとiPadのVBUSを直結しない。
- 外部電源コネクタ、外部5V注入、SW3流用は存在しない。

## 静的・host・firmware試験結果

```text
python3 scripts/hardware_contract.py
  HARDWARE CONTRACT PASS

cd tests && make clean && make
  t_note_mapper/t_midi_parse/t_nkro_report/t_midi_note_filter/
  t_usb_power_fsm/t_nkro_report_wrap: ALL PASS

./scripts/build_docker.sh
  genshin_midi_kbd.elf/uf2: PASS
  serial_midi_device.elf/uf2: PASS

kicad-cli pcb drc --schematic-parity --refill-zones --severity-all \
  --all-track-errors --format json ... rp2350_zero_carrier.kicad_pcb
  DRC 0 / unrouted 0 / schematic parity 0

kicad-cli sch erc --severity-error --exit-code-violations ...kicad_sch
  error 0

kicad-cli sch export netlist / export bom + schematic_contract.py
  standard reference 19点 / `?` 0 / 23-pin symbol名・型一致

schematic_visual_audit.py
  native A4 / field proximity / rendered golden一致
```

全ERC警告レポートは19件である。`lib_symbol_issues=15`（Device 8、Switch 5、Connector_Generic 2）はこの検証コンテナにKiCad標準symbol libraryが無いという実在の環境依存警告で、誤検知とは分類しない。`isolated_pin_label=4`は、回路図に置かないPCB-only UART test headerへJ1/J3のGP0/GP1を渡す単独端点である。J1/J3の専用23-pin、TPS2553、USB-Aはproject-local library化済みで、`endpoint_off_grid`、`multiple_net_names`、`lib_symbol_mismatch`は0。同じ回路図からのKiCad native parityと独立pad-map契約も0差分である。電気的ERC errorは0だが、warning 19件は不可視化も「error 0」への読み替えもしない。ERCの既存ignore（single global label、four-way junction、simulation model、footprint filter）を検証用copyでwarning化すると、追加は同じUART単独label 4件だけで他3種は0だった。

## 再現コマンド

```sh
git ls-remote --exit-code origin refs/heads/main
python3 scripts/hardware_contract.py
(cd tests && make clean && make)
./scripts/build_docker.sh
kicad-cli sch erc --severity-error --exit-code-violations \
  --output /tmp/rp2350-erc-errors.rpt \
  hardware/rp2350_zero_carrier/rp2350_zero_carrier.kicad_sch
kicad-cli sch export netlist --output /tmp/rp2350-native.net \
  hardware/rp2350_zero_carrier/rp2350_zero_carrier.kicad_sch
kicad-cli sch export bom --output /tmp/rp2350-native-bom.csv \
  hardware/rp2350_zero_carrier/rp2350_zero_carrier.kicad_sch
kicad-cli pcb drc --schematic-parity --refill-zones --severity-all \
  --all-track-errors --exit-code-violations \
  --output /tmp/rp2350-drc-parity.rpt \
  hardware/rp2350_zero_carrier/rp2350_zero_carrier.kicad_pcb
./scripts/build_gerbers.sh
(cd build/release && sha256sum --check SHA256SUMS.txt)
unzip -Z1 build/release/rp2350_zero_carrier_gerbers_JLCPCB.zip
git diff --check
```

## JLC BOM/CPL/Gerber

実装対象は`U1,C1,R3,R4,R5,C3`の6点だけ。test pad、THT、ソケット、スイッチ、電解、機構穴はBOM/CPLから除外する。

| Designator | Manufacturer | MPN | LCSC | CPL (X,Y,Top,Rotation) |
|---|---|---|---|---|
| U1 | Texas Instruments | TPS2553DBVR | C55266 | 11.00,17.30,Top,180 |
| C1 | FH (Guangdong Fenghua Advanced Tech) | 0603B105K160NT | C93816 | 8.50,21.00,Top,180 |
| R3 | UNI-ROYAL (Uniroyal Elec) | 0603WAF5232T5E | C23198 | 14.50,17.80,Top,180 |
| R4 | YAGEO | RC0603FR-07100KL | C14675 | 18.00,12.00,Top,90 |
| R5 | YAGEO | RC0603FR-07100KL | C14675 | 5.50,13.50,Top,0 |
| C3 | YAGEO | CC0603KRX7R9BB104 | C14663 | 15.25,20.80,Top,90 |

上表6点が唯一のJLC自動実装対象である。一方、JLC BOMは材料ごとの5行である。R4/R5は同じYAGEO `RC0603FR-07100KL` / `C14675`なので、BOMでは`100k 1%,"R4,R5",R_0603,C14675`という**quoted Designator 1行**に集約する。JLCが材料単位で表示する場合のC14675 Part Qty per PCB=**2**は正しい。CPLは物理搭載点なので6行を維持し、R4=`18.00,12.00,Top,90`、R5=`5.50,13.50,Top,0`を統合してはならない。特にU1は`TPS2553DBVR`通常版に固定し、`TPS2553DBVR-1`、同等品、仕様違いを含む一切の代替を禁止する。C1の公称仕様は1µF/X7R/16V/±10%/0603で一致するが、5V DC bias後の実効容量曲線は取得できていないためWVR-009とし、TI最小0.1µFを実効値で満たすことを量産前に確認する。

### 発注時チェックリスト

1. BOM Designatorをカンマ展開するとCPLと上表の6点に完全一致することを確認する。BOMは5材料行、CPLは6搭載行であり、C14675はquoted `R4,R5`の1材料行・Qty=2でなければならない。R4/R5を別BOM行へ戻す、未選択、Qty=0、重複警告は発注STOPとする。
2. 各行のManufacturer/MPN/LCSCを上表と照合し、JLCの部品代替を無効にする。U1に`-1` suffixや別MPNが表示された場合は発注を中止する。
3. U1のtop marking=`2553`、pin 1 dot、CPL rotation=180°をJLCプレビューとTI DBV top viewで二者確認する。
4. C1/C3の誘電体・容量・定格、R3/R4/R5の抵抗値・許容差をプレビューの部品詳細で再確認する。
5. THT、test pad、C2、J1–J3、F1、R1/R2、SW1–SW6、機構穴が自動実装対象へ混入していないことを確認する。
6. 6部品すべての位置・面・回転が見えるJLCプレビュー画像を保存し、WVR-004のclose証跡にする。

### BOM/CPL再アップロード手順

1. 同一commitで`./scripts/build_gerbers.sh`を実行し、`build/release/SHA256SUMS.txt`がBOM、CPL、native position CSV、Gerber ZIPを含めて全て`OK`であることを確認する。
2. 新しいJLC order/sessionへ、同じreleaseのGerber ZIP、`jlc_bom.csv`、`jlc_cpl.csv`をアップロードする。以前のsessionのBOM/CPLを上書き利用しない。
3. matching画面でC14675が一つの選択済み材料として`R4,R5`/Qty=2（designator表示ならR4=1かつR5=1）となること、重複・yellow warning・Qty=0・未選択がないことを確認する。
4. placement previewでR4=`18.00,12.00,Top,90`、R5=`5.50,13.50,Top,0`、U1=`C55266`/`TPS2553DBVR`/`11.00,17.30,Top,180`を確認し、matching画面・部品一覧・previewの画像とrelease SHA-256を保存する。
5. C14675を含む6点の在庫を同じsessionで確認する。在庫不足はQty対応とは別のNO-GOであり、代替を自動選択しない。

2026-08-31 JSTにJLC公開部品ページで`C14675`=`RC0603FR-07100KL`（YAGEO、100kΩ、1%、0603、SMT）を再照合した時点では、表示在庫は`0`だった。従って、このBOM集約でQty対応は是正しても、現時点の実発注判定は**NO-GO**である。在庫は変動するため新規sessionで再確認するが、在庫0を理由に別LCSC/別MPNを自動選択してはならない。

- https://jlcpcb.com/partdetail/YAGEO-CC0603KRX7R9BB104/C14663
- https://www.lcsc.com/product-detail/A_C14675.html
- https://fhcomp.com/en/product_info/?prod_code=0603B105K160NT
- https://jlcpcb.com/partdetail/95014-0603B105K160NT/C93816
- JLC向き確認: https://jlcpcb.com/help/article/component-polarity-and-orientation-identification-guide

## 分類別の残項目

### 確定不具合

今回の対象について未解決の確定不具合は0。上表の確定不具合は修正済みで、host test、ERC、DRC/parity、contractにより再発を検出する。

### 設計上のリスク

1. `R3=52.3kΩ`はUSB-A下流だけで概ね500mAを許し、RP2350-Zero本体・LED・周辺を加えた上流500mA予算を静的には保証しない。
2. C1/C3は要求容量を満たすが、実装後のinrush、VBUS droop、FAULTチャタリングは配線寄生成分を含むため実測が必要。
3. DBVの通常導通損失は小さいが、電流制限開始からIRQによるlatch-offまでの温度・時間は実測が必要。
4. C93816の5V DC bias後の実効容量、SW4投入/hot-plug時の220µF突入、既存D+/D−非対称配線の実機マージンは静的検査だけでは確定しない。

### 要メーカー確認

1. JLCプレビューでU1 pin1と6点の回転・座標を確認する。
2. JLCがC55266/C93816/C23198/C14675/C14663を代替せず、指定MPN/誘電体/許容差で実装することを確認する。
3. RP2350-Zeroをキャリア表面へ、USB-C/BOOT側が基板矢印側かつ部品面が外向きになるよう実装する。

### ERC warning（未解消、誤検知扱いしない）

ERC warning 19件は前述の標準library lookup 15件と、PCB-only UART test header用の単独label 4件である。電気的ERC error、設定どおりのDRC、unrouted、native parityはすべて0だが、この結果はwarningの存在を否定しない。DRC既存ignore 5種をwarning化したstrict auditでは`missing_courtyard=21`だけが残り、track-via中心ずれ、footprint filter/type、tuning geometryは0だった。21件はWVR-010であり、release DRC reportのignored checksにも残す。警告はreleaseへ全件保存し、不可視化しない。

## 条件付きGOからGOへ戻す条件

1. JLC order previewのスクリーンショットを保存し、6点のpin1/回転/座標を本書の表と二者確認する。
2. 実装基板で、boot・列挙前・detach・suspend時にTP_OUT=0V、configured後のみ約5Vであることを測る。
3. PC/iPad双方で上流総電流を測り、許可する全USB-MIDI機器との組合せで500mA以下と確認する。超過時は下流負荷を制限するか、R3再設計を別承認する。
4. TP_FAULTを含む短絡試験で最初のFAULT_N Low直後にENがLowとなり、自動再投入しないことを確認する。short除去、detach/reattach、suspend/resume、再列挙、BOOT、SW3、SW4ではlatchが残り、RUN/RESETまたは電源再投入後の再configurationだけで復帰すること、WS2812赤点滅を確認する。
5. 500mA連続30分と短絡試験でU1/配線/コネクタ温度、VBUS droop、再起動が受入基準内であることを確認する。
6. J3 pad1/pad3とJ1/J2 VBUS間の導通なしを電源OFFで測り、PC/iPad同時接続時に逆流が無いことを確認する。
7. C93816が5V印加・温度/公差込みで0.1µF以上を保つメーカー曲線またはLCR実測を保存する。
8. 1:1印刷/実物dry-fitでJ1/J3の表裏、SW6、C2極性、R2裏面、コネクタ/スイッチ/ねじの干渉なしを二者確認する。
9. PC/iPad双方でUSB Full-Speed列挙・再接続、長短ケーブル、対象MIDI機器、連続演奏を実機合格させる。
