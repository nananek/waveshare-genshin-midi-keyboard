# MIDI → NKRO HID + CH345 USB-MIDI ガジェット (RP2350A)

USB MIDI キーボードの演奏を RP2350 で受け、**NKRO HID キーボード**として
操作対象 PC（原神の演奏 UI）へ送ります。同時に、生 MIDI を **CH345** 経由で
USB-MIDI として制御 PC/DAW へ出力します。

```
[MIDI キーボード] --USB-A--> PIO-USB host (RP2350)
                                  |
                                  +--> native USB-C = NKRO HID → 操作対象 PC
                                  |
                                  +--> UART1 31250 --> CH345 --> USB-C = USB-MIDI → 制御 PC/DAW
```

- 3 オクターブ × ダイアトニック 7 音 = 21 キー固定
- **NKRO**（21 同時押しまで。HID レポートは 26 ビットのキービットマップ）
- 和音対応、ベロシティ無視
- MIDI の生バイト列は CH345 へそのまま流す（CC、ピッチベンド、SysEx も通る）
- HID を見せる PC と、USB-MIDI を見せる PC を物理的に分けられる

---

## 1. ハードウェア

新基板は `hardware/rp2350_frisk/` にあります。

> **注意:** `fab/` は CH345 版の発注データです。DRC は 0 violations /
> 0 unconnected ですが、U1 pin22 と J3 A7/B7 に via-in-pad を使っているため、
> JLCPCB の DFM プレビューで resin fill / via-in-pad の要否を確認してください。

- **外形 57.6 × 31.4 mm、四隅 2 mm 45° パンチカット**（旧型フリスクケース向け）
- **RP2350A QFN-60 ベアチップ**（ソケット／モジュールのスタックではない）
- 12 MHz 水晶、W25Q128 QSPI フラッシュ、3.3 V LDO
- **USB-C ×2 + USB-A ×1**
  - J1: native USB = NKRO HID（操作対象 PC）
  - J2: USB-A ホスト（MIDI キーボード）
  - J3: CH345 の USB-MIDI（制御 PC/DAW）
- CH345 は UART1 / GP4 / GP5 / 31250 8N1 で接続

### ピン割り当て（新基板）

| 機能 | RP2350A pin | GPIO | 接続 |
|------|-------------|------|------|
| PIO-USB D+ | 16 | GP12 | 22Ω → ESD → USB-A J2 D+ |
| PIO-USB D- | 17 | GP13 | 22Ω → ESD → USB-A J2 D- |
| CH345 MIN (RP2350 TX) | 7 | GP4 | CH345 の UART RX |
| CH345 MOUT (RP2350 RX) | 8 | GP5 | CH345 の UART TX |
| native USB D- | 51 | USB_DM | USB-C J1 D- |
| native USB D+ | 52 | USB_DP | USB-C J1 D+ |
| 原神モード（任意） | 42 | GP28 | TP3 ↔ GND |
| 楽器モード（任意） | 43 | GP29 | TP4 ↔ GND |
| 予備スイッチ（任意） | 41 | GP27 | TP5 ↔ GND（旧サステインスイッチの移設先） |

デバッグログは **UART0 (GP0=TX / GP1=RX, 115200)**。CH345 は UART1 なので
衝突しません。native USB は NKRO HID が占有します。

---

## 2. ソフトウェア構成

- **core1**: PIO-USB ホスト（`tuh_midi`）。MIDI キーボードの Note On/Off を
  パースし、生 MIDI バイト列を CH345 へ UART 出力する。
- **core0**: native USB の NKRO HID キーボード（`tud_hid`）。
  Note イベントを 26 ビットのキービットマップへ変換し、HID レポートを送る。
- コア間はロックフリーキュー。HID 状態は core0 だけが触る。

### ソースツリー

```
src/
  main.c              # デュアルコア起動、キュー、スイッチ、HID 出力
  usb_descriptors.c   # native USB = NKRO HID キーボード
  tusb_config.h       # device=HID / host=PIO-USB MIDI
  midi_host.c/.h      # tuh_midi コールバック（idx ベース）
  midi_parse.c/.h     # 生 MIDI → Note On/Off（純粋・テスト可能）
  midi_uart.c/.h      # CH345 への RAW MIDI UART 出力（31250）
  note_mapper.c/.h    # MIDI ノート → キースロット（純粋・テスト可能）
  nkro_report.c/.h    # NKRO 26 ビットレポート状態（純粋・テスト可能）
  hid_device.c/.h     # tud_hid グルー
  hid_mute.c/.h       # GP28 原神モード（HID 出力ミュート）
  mirror_filter_switch.c/.h # GP29 楽器モード（スメール音階）
  debounced_switch.c/.h
  config.h            # 設定の一元管理
hardware/rp2350_frisk/ # 基板（CH345 版、6層）
tests/                 # ホスト側ユニットテスト（ARM 不要）
scripts/               # ビルド・依存取得・製造データ
```

### 依存ライブラリ

- **pico-sdk 2.1.1**（RP2350 対応）
- **hathach/tinyusb 0.20.0**
  （pico-sdk 同梱版には `midi_host` が無いため upstream が必須）
- **sekigon-gonnoc/Pico-PIO-USB**（RP2350 対応コミット固定）

`scripts/fetch_deps.sh` が `lib/` に取得します。

---

## 3. ビルド & 書き込み

### Docker（推奨）

```sh
./scripts/build_docker.sh
# => build/midi_nkro_ch345.uf2
```

> ターゲット名は移行途中のため `midi_nkro_ch345` のままです。


### ネイティブ

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
./scripts/fetch_deps.sh
cmake -B build -DPICO_BOARD=pico2
cmake --build build -j
```

### 書き込み

BOOTSEL を押しながら USB-C 接続し、`build/midi_nkro_ch345.uf2` を
RPI-RP2 ドライブへコピーします。

---

## 4. 設定（`src/config.h`）

すべて `#ifndef` ガード付き。`cmake -DCMAKE_C_FLAGS="-DOCTAVE_OFFSET=12"` 等で
上書きできます。

| パラメータ | 既定 | 意味 |
|-----------|------|------|
| `OCTAVE_OFFSET` | `0` | 入力ノートに加算する半音数（12=1oct） |
| `OUT_OF_RANGE_POLICY` | `IGNORE` | 範囲外: `IGNORE`=無視 / `WRAP`=最寄りオクターブへ折返し |
| `PIN_USB_HOST_DP` | `12` | PIO-USB ホスト D+（D− は +1） |
| `MIDI_CHANNEL_FILTER` | `0` | 0=全ch / 1..16=指定chのみ |
| `MIDI_UART_INDEX` | `1` | CH345 に使う UART（0=uart0 / 1=uart1） |
| `MIDI_UART_TX_PIN` | `4` | RP2350 TX → CH345 MIN |
| `MIDI_UART_RX_PIN` | `5` | RP2350 RX ← CH345 MOUT |
| `MIDI_UART_BAUD` | `31250` | MIDI over UART |
| `MUTE_SWITCH_ENABLE` | `1` | GP28 原神モード（HID 出力ミュート） |
| `MUTE_SWITCH_PIN` | `28` | スイッチ GPIO |
| `LYRE_SWITCH_ENABLE` | `1` | GP29 楽器モード（スメール音階） |
| `LYRE_SWITCH_PIN` | `29` | スイッチ GPIO |
| `SPARE_SWITCH_ENABLE` | `1` | GP27 予備スイッチ（旧サステインの移設先） |
| `SPARE_SWITCH_PIN` | `27` | 予備スイッチ GPIO |
| `SPARE_SWITCH_CC` | `-1` | `-1`=状態のみ / `0..127`=その CC を CH345 へ送る |

### キー対応表（既定）

```
低音域 48=Z 50=X 52=C 53=V 55=B 57=N 59=M   (ド レ ミ ファ ソ ラ シ)
中音域 60=A 62=S 64=D 65=F 67=G 69=H 71=J
高音域 72=Q 74=W 76=E 77=R 79=T 81=Y 83=U
```

黒鍵は常にドロップします（原神の楽器に黒鍵が無いため）。

---

## 5. MIDI / HID の流れ

1. PIO-USB ホストが MIDI キーボードの USB-MIDI パケットを受信する。
2. `tuh_midi_stream_read` が返す生 MIDI 1.0 バイト列を、そのまま CH345 へ
   UART1 31250 で送る。
3. 同時に `midi_parse` が Note On/Off を取り出し、core0 のキューへ積む。
4. core0 が `note_mapper` でキースロットへ変換し、`nkro_report` の
   26 ビットビットマップを更新して `tud_hid_report` で送る。

HID は 5 バイト（modifier 1 + キービットマップ 4）の NKRO レポートです。
`usb_descriptors.c` のレポートディスクリプタと `nkro_report.c` のビット配置は
必ず一致させてください。

---

## 6. ホスト側ユニットテスト

ARM ツールチェーン不要。純粋ロジックを `cc` で検証します。

```sh
cd tests
make        # note_mapper / midi_parse / nkro_report
make clean
```

カバー範囲:

- `note_mapper`: 21 キー対応、黒鍵ドロップ、範囲外 IGNORE/WRAP、移調、スメール音階
- `midi_parse`: Note On/Off、vel0=Off、ランニングステータス、リアルタイム、SysEx
- `nkro_report`: 単音/和音のビット配置、エイリアス参照カウント、retrigger べき等、
  range 外無視、release_all

---

## 7. 実機キャリブレーション

1. UART0 ログで MIDI キーボードの Note On/Off が出ることを確認。
2. 操作対象 PC で NKRO HID キーボードとして認識され、キーボードテスターで
   キーが出ることを確認。
3. 制御 PC/DAW で CH345 が USB-MIDI として見えることを確認（`aconnect -l` 等）。
4. 原神の演奏画面で「ド」を弾き、画面上のキーと発音が一致するか確認。
   ズレていたら `OCTAVE_OFFSET` を ±12 / ±1 で調整。
5. 3 和音以上を弾いて全音が発音されることを確認（NKRO）。
6. 黒鍵が無視されることを確認。
7. MIDI キーボードを抜き差しして、HID 側の全キーが解放されることを確認。

---

## 8. 既知の制約 / 残作業

- 基板は CH345 版（6層）で、`fab/` に Gerber / BOM / CPL を生成済み。
- U1 pin22 と J3 A7/B7 に via-in-pad を使用。JLCPCB の DFM で
  resin fill / via-in-pad の要否を確認すること。
- CH345 の UART は 31250 固定。CC/ピッチベンド/SysEx はそのまま通ります。
- HID は NKRO ですが、原神側が同時に受け付ける音数はゲーム側の仕様に依存します。
- 回路図と ERC は未作成。

---

## 9. 旧構成（履歴）

`hardware/rp2350_zero_carrier/` は RP2350 を 2 枚使う旧構成の名残です。
現在のビルド対象ではなく、新構成では 1 枚の RP2350A が NKRO HID と
CH345 ドライバを兼ねます。

## ライセンス

`LICENSE` を参照してください。取得・ビルド依存のライセンスは
`THIRD_PARTY_NOTICES.md` を参照してください。
