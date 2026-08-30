# 実測未完了項目とwaiver台帳

この台帳は試作発注だけを許可する期限付きwaiverである。量産GOの代用にはならない。各waiverは試作実装後、判定書のGO復帰条件を満たした証跡（測定ログ、写真、JLCプレビュー）でcloseする。

| ID | 分類 | 未実施項目 | 静的代替証拠 | close条件 |
|---|---|---|---|---|
| WVR-001 | 設計上のリスク | boot/USB列挙前の実電流・TP_OUT電圧 | R5=100kΩ、GP9 latch-low、`tud_mounted && !tud_suspended`、FSM host test | PC/iPadでboot/列挙前/列挙後/detach/suspendをオシロまたはロガー測定 |
| WVR-002 | 設計上のリスク | 基板全体500mA予算 | USB descriptor 500mA、TI Rev.F Table 2: ideal 52.5kΩ/closest 1% R3=52.3kΩ/actual 448.3・501.6・562.4mA | 全許可USB-MIDI機器で上流総電流≤500mA。超過時は発注停止 |
| WVR-003 | 設計上のリスク | short/overcurrent/thermal、SW4/USB-A hot-plug時の220µF突入 | 最初のFAULTでIRQ latch-off、自動retryなし、DRC 0、DBV損失計算 | TP_OUT短絡・500mA連続30分・SW4/hot-plug・最高周囲温度でFAULT/VBUS波形と温度を記録。short除去、detach、suspend/resume、再列挙、BOOT、SW3/SW4で再投入せず、RP2350 RUN/RESETまたは電源再投入による実reboot後の再configurationだけで復帰すること |
| WVR-004 | 要メーカー確認 | JLC実装プレビュー | BOM/CPL exact-set contract PASS、native parity 0、公式orientation guide | U1 pin1 dotと6部品のrotationをJLC画面で二者確認し画像保存 |
| WVR-005 | 要メーカー確認 | J1/J3モジュールの立体的表裏・回転 | footprint pin番号、F/B silk、組立注記、Waveshare公式pinout | 無通電仮組み写真とpad1/pad23導通確認 |
| WVR-006 | 設計上のリスク | PC/iPad同時接続の逆流 | J3 pad1/pad3 NC、hardware contractで下流VBUS不在を検査 | 電源OFF導通試験と両ホスト接続時の各VBUS電圧/逆電流測定 |
| WVR-007 | 設計上のリスク | WS2812の色・FAULT表示実機確認 | GPIO16/PIO2、3状態のhost test、非blocking PIO送信、firmware link PASS | WAIT消灯、ON低輝度緑、FAULT赤1Hz/25% dutyを確認。FAULT表示はdetach/suspendでも継続し、RUN/RESET後に消える動画または試験記録 |
| WVR-008 | 要メーカー確認 | 6点の在庫・代替・実装方向 | 発注判断書のexact Manufacturer/MPN/LCSC 6行表、BOM LCSC固定 | 注文時に全代替を禁止し、U1=`TPS2553DBVR`（`-1`禁止）を含む6点のMPN/値/誘電体/許容差を確認 |
| WVR-009 | 要メーカー確認 | C1 C93816の5V DC bias後の実効容量 | FH一次ページで0603B105K160NT=1µF/X7R/16V/±10%を確認、TI最小値は0.1µF | 温度/公差込み実効容量≥0.1µFのメーカー曲線または5V bias下LCR実測を保存。未達/不明なら特性保証品へ変更して再レビュー |
| WVR-010 | 設計上のリスク | courtyard未整備21 footprintの実物立体干渉、J1/J3表裏、SW6、C2/R2実装性 | 設定DRC/silk audit 0、ignore全解除strict DRCは`missing_courtyard=21`のみ、寸法入りfootprint、組立注記 | 1:1印刷と実部品dry-fit写真、操作/嵌合/ねじ/半田ごてアクセスを二者確認 |
| WVR-011 | 設計上のリスク | 既存D+/D−非対称配線のPC/iPad実機マージン | D+/D−netは今回変更なし、DRC 0、firmware link PASS | 両ホスト、長短ケーブル、許可MIDI機器で列挙/再接続/連続演奏を合格 |

## 受入測定点

- TP_EN `(8.00,-12.50)`: GP9/TPS EN
- TP_FAULT `(14.50,-11.50)`: GP10/FAULT_N
- TP_IN `(6.00,-21.50)`: F1後段/TPS IN
- TP_OUT `(18.00,-20.05)`: TPS OUT、SW4前段
- TP_GND `(12.00,-21.50)`: 局所GND

測定器GNDはTP_GNDへ接続する。USB D+/D−、異なるホストのVBUS、J3 pad1/pad3へ測定器から電源を注入しない。

## 静的warningの扱い

ERC warning 19件は量産試験のwaiverではない。標準symbol library lookup 15件は検証コンテナの実在する環境依存警告、単独UART label 4件はPCB-only test headerとの境界を示す意図的端点であり、いずれもreleaseへ全件保存する。`endpoint_off_grid`、複数net名、project-local symbol mismatch、電気的ERC error、設定どおりのDRC、unrouted、native parityは0である。ERC ignore 4種のstrict確認は同じ単独UART label 4件だけを追加検出した。DRC ignore 5種のstrict確認はWVR-010の`missing_courtyard=21`だけを検出し、track-via中心ずれ、footprint filter/type、tuning geometryは0だった。

## waiver失効条件

- PCB/回路図/BOM/CPL/Gerberのchecksumが変化した場合
- U1、R3、C1、C3、R5、R4、J1/J3、SW3/SW4/SW6の部品・値・ネット・座標が変化した場合
- ファームのGP9/GP10/GPIO16、USB descriptor、FAULT latch/IRQ/clear方針が変化した場合
- JLCが指定部品を代替する場合

失効時は再レビューし、waiverを自動継承しない。
