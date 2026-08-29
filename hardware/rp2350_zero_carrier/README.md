# RP2350-Zero carrier 組立

## J1/J3の向きは必ず確認する

J1/J3の穴は左右対称に見えるため、RP2350-Zeroを裏返しても仮置きでは穴が合います。しかしその向きで半田付けすると、5VとGPIOなどが鏡映され、重大な誤配線になります。ピンソケットは取り外せない前提で、次の確認を半田付け前に必ず行ってください。

1. キャリアの **B.Cu（裏面）** を上にし、B.Silkscreenの `USB-C / BOOT`、太い矢印、V字ノッチ、`1=5V` から **+Y側（横5穴列の反対端）** とpin 1を確認します。
2. +Y側を見失わないように基板を裏返して **F.Cu（表面）** を上にし、メスソケットをその表面に仮置きします。RP2350-ZeroのUSB-C/BOOT/RUN側を、手順1で確認した+Y側へ向けます。
3. Zeroは、USB-C・BOOT/RUN・LEDがある**部品面をキャリアから外側（上）**、オスピンを下にして仮挿入します。USB-C/BOOT/RUNが+Y側にあり、BOOT/RUNとLEDが上から見えることを確認します。
4. 反対面から見てBOOT/RUNがキャリア側に隠れる場合は中止してください。その姿勢は穴が合っても誤りです。
5. 仮挿入を外し、メスソケットをF.Cu面から片端の1ピンだけ仮止めします。向きと直角を再確認してから残りを半田付けします。

実装後は、J1/J3ともUSB-Cが矢印側に出ていてBOOT/RUNを押せることを確認してください。

## USB-A VBUS保護回路

USB-Aの電源は `J1 5V → F1 → U1 (TPS2553) → SW4 → COUT → J2 VBUS` の順に通ります。F1はリセッタブルヒューズ、U1は電流制限・短絡保護付きのハイサイドスイッチ、SW4はUSB-Aへの手動給電スイッチです。

- `R_ILIM = 52.3kΩ 1%` のとき、TPS2553データシート表の電流制限実力値は最小448.3 mA、公称501.6 mA、最大562.4 mAです。これは異常時の保護上限であり、接続したMIDI機器へ500 mAを保証するものではありません。通常消費の小さいUSB MIDIキーボードを対象にし、不明な機器は消費電流を確認してください。
- ボード1のUSBコンフィグレーションディスクリプタは、RP2350本体とUSB-A接続機器を合わせた上流側の最大値として500 mAを申告します。電源供給元の能力を増やす設定ではないため、無給電ハブや大電流機器は接続しないでください。
- `USB_PWR_FAULT_N` はU1のオープンドレインFAULTを100kΩで入力側5Vへプルアップした観測用ネットです。現版ではRP2350へ接続していません。
- `CIN` はU1入力の直近に置く1uFセラミックコンデンサです。
- `COUT` はUSB-Aコネクタ付近の220uF電解コンデンサです。**pad 1 / `+` を `VBUS_USB_A_SW`、pad 2 / `-` をGND**へ実装してください。逆挿しは禁止です。

部品値・パッケージの根拠は、[TI TPS2553データシート](https://www.ti.com/lit/ds/symlink/tps2553.pdf)と[TI DBV0006Aパッケージ図](https://www.ti.com/lit/ml/mpds026v/mpds026v.pdf)です。COUTは[秋月電子のRubycon 220uF 16V品](https://akizukidenshi.com/catalog/g/g110326/)に合わせて直径6.3 mm・ピッチ2.5 mmとしています。

## R2は裏面へ実装する

USB D-の直列抵抗R2は、そのリード間のF.Cu側にU1/CINがあるため、**抵抗本体をB.Cu（裏面）側へ寝かせて挿入し、F.Cu側から半田付け**します。B.Silkscreenの `R2` referenceと抵抗本体の外形が見える側が実装面です。R1は従来どおりF.Cu側へ実装します。

## U1型番の選定理由(ラッチオフ版を採用しなかった理由)

`TPS2553DBVR`(通常版・自動リトライ)を採用し、`TPS2553DBVR-1`(ラッチオフ版)は採用していません。ラッチオフ版は過電流検出後にOUTをラッチオフし、再度ONにするにはEN再トグルまたはVIN再投入が必要です。本基板はENを常時ON固定でGPIO制御を持たないため、ラッチオフ版だと過電流1回でOUTが電源再投入以外に復帰しなくなり、無人運用に近いMIDI周辺機器用途ではリスクが高いと判断しました。通常版は過電流除去後に自動でOUT再投入を繰り返すため、接続機器の一時的な突入電流やホットプラグに対してより穏当に振る舞います。

## EN固定・ILIM設定の根拠

- `EN`(pin3)は抵抗を介さず`VBUS_TPS_IN`(F1後段、約5V)へ直結しています。TIデータシート(SLVS841F)のAbsolute Maximum RatingsはEN/IN/OUT/ILIM/FAULT共通で-0.3〜7Vであり、Recommended Operating ConditionsのV_IN上限も6.5Vです。ENとINが同一ノードでも規格内(7V上限に対し実電圧は約5V)のため、直結で問題ありません。GPIO制御化は現版では行っていません(将来GPIO化する場合は外部プルダウン必須・内部プルアップ任せ厳禁、という設計注意のみ申し送りとして残します)。
- `R_ILIM = 52.3kΩ`(1%)は、TIデータシートTable 2「Common R_ILIM Resistor Selections」の"Desired Nominal Current Limit 500 mA"行にある値をそのまま採用しています(推奨範囲15kΩ〜232kΩ内)。実力値は本文既述のとおり448.3〜562.4 mA(公称501.6 mA)です。独自に式から計算した値ではなく、データシートの実測ベースの表を直接引用しています。

## VBUS配線幅の根拠(IPC-2221)

`USB_VBUS_PWR` net class(`VBUS_5V` / `VBUS_TPS_IN` / `VBUS_USB_A` / `VBUS_USB_A_SW`)は基板既定の0.2mmから1.0mm(外層・1oz銅相当)へ拡幅しています。IPC-2221外層トレース幅の式(I = k・ΔT^0.44・A^0.725, k=0.048)で計算すると、1oz・ΔT10℃・外層条件で0.2mmは約0.74A、1.0mmは約2.4Aまで許容できます。ILIM実力値の上限(562.4 mA)に対し1.0mmは十分な余裕(約4倍)を持たせた値です。

- `J1 5V → F1 → U1 → SW4 → COUT/J2`の主電流経路は1.0mmです。従来0.2mmだった`J1 5V → F1`は、J1/J3のヘッダーパッド列の中央へ引き直してクリアランスを保ったまま1.0mm化しました。
- U1のINピン直近は、隣接するGNDピンおよびGNDビアとのクリアランスを確保するため、約1.5mmだけ0.4mmへテーパーさせています。CIN/EN/FAULTの枝も負荷電流の主経路ではないため0.4〜0.6mmです。これらを除く主電流経路は1.0mmです。

## GND強化

B.Cu全面をGNDゾーン化してリフィルしました。USBの負荷電流は`J2 GND/COUT → B.Cu GNDプレーン → J1 GND`へ戻るため、信号GNDも含むネット全体を1.0mmのネットクラスへ変更する必要はありません。GNDネットは`Default`のままですが、ゾーンのサーマルスポーク幅は0.5mm、最小スポーク数は2本で、J2のシェルへ向かうプレーン外のF.Cu枝は1.0mmです。既存のGNDビアは1本のみでしたが、U1/CIN/R_ILIM周辺への追加ステッチビアにより、現在GNDネットのビアは4本(F.Cu-B.Cu貫通)です。ゾーンリフィル後に`kicad-cli pcb drc --refill-zones --save-board`でDRCを再実行し、0違反であることを確認しています。

## D+/D-配線は現状維持(変更していません)

今回のUSB-A VBUS保護回路追加ではD+/D-ネット(`GP12_USB_DP`, `GP13_USB_DM`, `USB_A_DP`, `USB_A_DM`)には一切手を加えていません。既存のPIO-USBホスト動作(Full-Speed列挙)を壊すリスクを避けるためです。

- 現状の配線長: D+ 約33.5mm(`GP12_USB_DP` 19.6mm B.Cu + `USB_A_DP` 13.9mm F.Cu、層をまたぐ)、D- 約44.7mm(`GP13_USB_DM` 15.9mm + `USB_A_DM` 28.9mm、B.Cu内完結)。長さ差は約11.2mmで、D+のみ層をまたぐ非対称構成です(実測はworkerAの計画書による)。
- 改善(等長化・同一層化)を見送った理由: GND/VBUS強化のための再配線・ゾーン追加が、この非対称だが**現に動作しているD+/D-配線**を意図せず分断・再ルーティングしてしまうリスクの方が、USB Full-Speedの電気的マージン改善より大きいと判断したためです。
- **実機でのUSB Full-Speed列挙確認が必要です**。今回D+/D-は変更していませんが、GND/VBUS周りの改版そのものが基板改版であり、実機での再検証(USB MIDIキーボードの列挙・演奏確認)を組立後に必ず行ってください。

## SW4・ENの扱い(今回の結論)

SW4はF1〜J2間のVBUS直列スイッチとして従来どおり残置し(現在はU1のOUT側とCOUT/J2の間に位置)、GPIO制御化は行っていません。ENは前述のとおり常時ON固定(VBUS_TPS_INへ直結)です。SW4がユーザーが操作できる唯一のVBUS ON/OFF手段である点は変わりません。

## JLCPCB向けデータ

`jlc_bom.csv` / `jlc_cpl.csv` はJLCPCBのSMD自動実装(CPL)に対応する4部品(`U1`, `CIN`, `R_ILIM`, `R_FAULT`)のみを収録しています。BOMとCPLのDesignatorは完全一致します。

- **JLC実装(SMD)対象**: `U1`(TPS2553DBVR), `CIN`(0603), `R_ILIM`(0603), `R_FAULT`(0603)。
- **手はんだ(THT)対象**: `J1`〜`J3`(ピンソケット/ヘッダー), `F1`(ラジアルPTCヒューズ), `SW1`〜`SW6`(スライド/タクトスイッチ), `R1`/`R2`(axial 22Ω), `COUT`(ラジアル電解コンデンサ、220uF)。いずれもCPLには含めていません。
- **LCSC番号**: `U1`=C55266(TPS2553DBVR、TI公式ページで型番・SOT-23-6パッケージを直接確認)、`CIN`=C93816(FH 0603B105K160NT、1uF/16V/X7R、LCSC商品ページで確認)、`R_FAULT`=C14675(YAGEO RC0603FR-07100KL、100kΩ±1%、LCSC商品ページで確認)。**`R_ILIM`(52.3kΩ 1% 0603)はLCSC上で在庫確認できませんでした**(0402パッケージ品`RC0402FR-0752K3L`のみ発見、0603品は未掲載)。発注前に52.3kΩ±1% 0603(例: Yageo RC0603FR-0752K3L相当品)の入手性を別途確認してください。空欄のまま推測番号は記載していません。
- SOT-23-6(U1)のピン1向きはTIデータシートSLVS841Fの"Pin Configuration and Functions"(DBVパッケージ, top view: 左列上から IN/GND/EN、右列上から OUT/ILIM/FAULT)に基づいて自作footprintを作成しています。発注時にJLCPCBの回転補正表と必ず突き合わせてください(特にR_ILIMは180°回転で配置しているため要確認)。

## 既存由来のERC件数(ベースライン)

`kicad-cli sch erc --severity-all`はこの基板の既存回路(今回のタスク範囲外、主にJ1/J3の未使用GPIOピンや手描きシンボルのグリッド外配置に起因)で129件を検出します(内訳: pin_not_connected 37, endpoint_off_grid 54, isolated_pin_label 16, lib_symbol_issues 12, unconnected_wire_endpoint 10)。今回の追加後は132件で、差分の内訳は「新規部品5点(U1/CIN/R_ILIM/R_FAULT/COUT)ぶんの`lib_symbol_issues`+5」「`isolated_pin_label`-2(既存ラベル整理による改善)」です。`pin_not_connected`・`endpoint_off_grid`・`unconnected_wire_endpoint`は増えていません。`lib_symbol_issues`はこのプロジェクトにsym-lib-tableが存在しないこと自体に起因する既存の警告カテゴリで、新規部品もR1/R2/F1と同じ理由で警告対象になるだけであり、配線の欠陥ではありません。

PCB DRC(`kicad-cli pcb drc --refill-zones --severity-all`)は0 violations・0未配線パッドです。`--schematic-parity`を追加した検査では合計84件を検出します。ベースラインの70件から増えた14件はいずれもPCB/回路図のネット名の先頭スラッシュ差で、今回追加した5部品(`U1`/`CIN`/`R_ILIM`/`R_FAULT`/`COUT`)は全ピンを個別照合済みです。既存の70件にはpin/footprint mismatch等も含まれ、今回の変更範囲外です。

## Release PDF paper size

Run `scripts/build_gerbers.sh` to regenerate the release drawings. The
schematic is fitted to ISO A4 landscape. The five selected PCB layers are
exported on separate A4 pages at physical scale 1:1 after translating a
temporary copy into the page area, so every page is centered without clipping
or changing the board dimensions.
