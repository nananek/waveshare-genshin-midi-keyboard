# 原神「諧律のチェンバロ」MIDI → HID キーボード変換ファーム (RP2350)

USB MIDI キーボードの演奏を RP2350 上で解析し、USB HID キーボード信号へ変換して
PC 上の原神・演奏 UI(諧律のチェンバロ等)を弾けるようにするファームウェア。

```
[MIDI キーボード] --USB--> (PIO-USB host) RP2350 (native USB device) --USB--> [PC / 原神]
                         core1: tuh_midi                core0: tud_hid (NKRO)
```

- 3 オクターブ × ダイアトニック 7 音 = 21 キー固定
- 和音対応(NKRO ビットマップ、最大 21 同時押し)
- ベロシティは無視(ゲーム側に強弱概念が無いため)

---

## 1. ハードウェア

対象ボード: **Waveshare RP2350-USB-A**(RP2350 + USB-A ホストポート)。素の Pico 2 +
自作 D+/D− 配線でも可。

| 役割 | 接続 | 備考 |
|------|------|------|
| MIDI 入力 (ホスト) | USB-A ポート = **D+ = GP12 / D− = GP13** | PIO-USB。`PIN_USB_HOST_DP` で変更可 |
| PC 出力 (デバイス) | USB-C(ネイティブ USB) | HID キーボードとして認識される |

### ⚠️ 必須ハードウェア改造(Waveshare RP2350-USB-A)

USB-A ポートの **D+ にはデバイス動作用のプルアップ抵抗 R13 (1.5kΩ)** が載っている。
これはホスト動作を妨げる(ロースピード機器が繋がらない・ホットプラグ検出が効かない)。
**R13 を除去する**こと。除去しないと MIDI キーボードが列挙されない/挿抜検出できない。

- D+/D− は隣接ペアであること(GP12/GP13 は条件を満たす)。
- 素のボードで自作配線する場合、ホスト側は D+/D− に強いプルアップを付けないこと。

> 指示書 1章の「D+ のプルダウン抵抗を外す改造」は、実際には当ボードでは
> **D+ のプルアップ R13 の除去**に相当する。

---

## 2. ソフトウェア構成

- **core0**: ネイティブ USB = NKRO HID キーボードデバイス (`tud_hid`)
- **core1**: PIO-USB = USB MIDI ホスト (`tuh_midi`)
- MIDI 受信 → ノート変換 → HID キーコード → NKRO レポート送信の一方向パイプライン。
  コア間はロックフリーキュー(`pico/util/queue.h`)で受け渡し、HID 状態は core0 のみが触る。

### ソースツリー

```
src/
  main.c            # デュアルコア起動、コア間キュー、ドレインループ
  usb_descriptors.c # デバイス/コンフィグ/NKRO レポートディスクリプタ
  hid_device.c/.h   # tud_hid グルー・NKRO レポート送信
  nkro_report.c/.h  # NKRO レポート状態(純粋・単体テスト可能)
  midi_host.c/.h    # tuh_midi コールバック(idx ベース)
  midi_parse.c/.h   # 生 MIDI バイト列 → Note On/Off(純粋・単体テスト可能)
  note_mapper.c/.h  # MIDI ノート → キースロット変換(3 章の仕様、純粋・単体テスト可能)
  config.h          # 全設定パラメータ(4 章)
  tusb_config.h     # TinyUSB デュアルロール設定
tests/              # ホスト側ユニットテスト(ARM 不要、cc で実行)
lib/                # 外部ライブラリ(fetch_deps.sh が配置)
scripts/fetch_deps.sh
CMakeLists.txt
pico_sdk_import.cmake
```

### 依存ライブラリとバージョン(重要)

- **pico-sdk 2.1.1 / 2.2.0**(RP2350 対応)。環境変数 `PICO_SDK_PATH` を設定。
- **hathach/tinyusb `0.20.0`**(⚠️ pico-sdk 同梱の raspberrypi/tinyusb では不可)
  - 同梱版には `midi_host` が無く `tuh_midi_*` をリンクできない。
  - `0.20.0` は idx ベースの `tuh_midi` API・`hcd_edpt_abort_xfer`/`hcd_frame_number`
    実装済み(→ 指示書 2.3 の未実装リンクエラー回避策は不要)。
- **sekigon-gonnoc/Pico-PIO-USB `main`**(RP2350 対応)。

`scripts/fetch_deps.sh` がこれらを `lib/` に取得する。CMake は `PICO_TINYUSB_PATH`
を upstream に向けてビルドする。

---

## 3. ビルド & 書き込み

```sh
# 0) ARM ツールチェーン + pico-sdk が必要
#    Arch: pacman -S arm-none-eabi-gcc arm-none-eabi-newlib cmake
#    PICO_SDK_PATH を pico-sdk のパスに設定しておく
export PICO_SDK_PATH=/path/to/pico-sdk

# 1) 依存ライブラリ取得
./scripts/fetch_deps.sh

# 2) ビルド
cmake -B build -DPICO_BOARD=pico2
cmake --build build -j

# 3) 書き込み: BOOTSEL を押しながら USB-C 接続 → build/genshin_midi_kbd.uf2 を
#    RPI-RP2 ドライブへコピー(または picotool load build/genshin_midi_kbd.uf2)
```

デバッグログは **UART**(GP0=TX, GP1=RX, 115200)に出る(native USB は HID が占有)。

---

## 4. 設定(`src/config.h`)

すべて `#ifndef` ガード付き。`cmake -DCMAKE_C_FLAGS="-DOCTAVE_OFFSET=12"` 等でも上書き可。

| パラメータ | 既定 | 意味 |
|-----------|------|------|
| `OCTAVE_OFFSET` | `0` | 入力ノートに加算する**半音数**(12=1oct)。実機キャリブレーション用 |
| `OUT_OF_RANGE_POLICY` | `IGNORE` | 範囲外(47以下/84以上): `IGNORE`=無視 / `WRAP`=最寄りオクターブへ折返し |
| `CHROMATIC_SNAP_POLICY` | `DOWN` | 黒鍵: `DOWN`=直下白鍵 / `UP`=直上白鍵 / `IGNORE`=無視 |
| `PIN_USB_HOST_DP` | `12` | PIO-USB ホストの D+ GPIO(D− は +1) |
| `MIDI_CHANNEL_FILTER` | `0` | 0=全ch / 1..16=指定chのみ |

### キー対応表(既定・指示書 3.1)

```
低音域 48=Z 50=X 52=C 53=V 55=B 57=N 59=M   (ド レ ミ ファ ソ ラ シ)
中音域 60=A 62=S 64=D 65=F 67=G 69=H 71=J
高音域 72=Q 74=W 76=E 77=R 79=T 81=Y 83=U
```

---

## 5. ホスト側ユニットテスト(ARM ツールチェーン不要)

純粋ロジック(ノート変換・MIDI パース・NKRO レポート状態)は PC の `cc` で検証できる。

```sh
cd tests
make          # 3 スイートをビルドして実行 (全て ALL PASS になるはず)
make clean
```

カバー範囲:
- `note_mapper`: 3.1 対応表、黒鍵 3 ポリシー、範囲外 2 ポリシー、移調オフセット
- `midi_parse`: Note On/Off、vel0=Off、ランニングステータス、リアルタイム混在、SysEx、和音
- `nkro_report`: 単音/和音のビット配置、エイリアス参照カウント(黒鍵スナップで同一キーに
  丸まった 2 音)、retrigger べき等、range 外無視、`release_all`

---

## 6. 実機キャリブレーション手順(指示書 6章)

1. UART ログで MIDI キーボードの Note On/Off が出ることを確認。
2. PC で HID キーボードとして認識され、キーボードテスター等でキーが出ることを確認。
3. 原神の演奏画面を開き、MIDI で「ド」を弾いて画面上の対応キーと発音が一致するか確認。
   ズレていたら `OCTAVE_OFFSET` を ±12(オクターブ)や ±1(半音)で調整。
4. 3 和音以上を弾いて全音が発音される(NKRO)ことを確認。
5. 黒鍵を弾いて `CHROMATIC_SNAP_POLICY` の挙動が好みか確認、必要なら変更。
6. MIDI キーボードを抜き差ししても復帰する(切断時は全キー解放される)ことを確認。

---

## 7. 既知の制約 / 残リスク

- **TinyUSB × Pico-PIO-USB のリンク統合**が唯一の要実機確認ポイント。本 CMake は
  Pico-PIO-USB 公式サンプル(native device + PIO host)と同じ方式:
  `tinyusb_device` / `tinyusb_host` をリンクしつつ `dcd_pio_usb.c` / `hcd_pio_usb.c`
  と(SDK が列挙しない)`midi_host.c` を手動追加する。
  もし `hcd_*` の重複定義や未定義参照が出た場合は、TinyUSB / Pico-PIO-USB の
  バージョン組み合わせ(§2)を疑い、各 GitHub Issue を確認すること。
- ダイアトニック 21 音固定。原神側に半音/移調キーが増えたらマッピング再設計が必要。
- PIO-USB の MIDI クラス相性検証事例は少ない。列挙に失敗する機種があれば
  `tuh_midi` のデバッグログを有効化して切り分ける。

## 8. 受け入れ基準(指示書 7章)

- [ ] 3 オクターブのダイアトニック音が、原神の楽譜表示・発音と対応キー通り一致
- [ ] 3 和音以上を同時に弾いても全音が正しく発音(NKRO)
- [ ] MIDI キーボードのホットプラグ後も動作継続 / 再認識(切断時は全キー解放)
- [ ] 範囲外・黒鍵の挙動が設定通り
