# PIO-MIDI → CH9329 キーボード ガジェット (RP2350A)

USB MIDI キーボードの演奏を RP2350 で受け、**CH9329 経由の USB HID
キーボード**として操作対象 PC（原神の演奏 UI）へ送るガジェットです。
同時に RP2350 自身は **USB-MIDI ガジェット**として PC/DAW に MIDI を
そのまま出力します。

```
[MIDI キーボード] --USB-A--> PIO-USB host (RP2350)
                                  |
                                  +--> native USB-C = USB-MIDI ガジェット → PC/DAW
                                  |
                                  +--> UART1 9600 8N1 --> CH9329 --> USB-C HID → 操作対象 PC
```

- 3 オクターブ × ダイアトニック 7 音 = 21 キー固定
- 和音対応（CH9329 の 6KRO レポート。6 音まで）
- ベロシティは無視
- MIDI の生バイト列は USB-MIDI ガジェットへパススルー（CC、ピッチベンド、SysEx も通る）
- MIDI を見せる PC と、CH9329 で操作する PC を物理的に分けられる

---

## 1. ハードウェア

新しい基板は `hardware/rp2350_frisk/` にあります。

- **外形 57.6 × 31.4 mm、四隅 2 mm 45° パンチカット**
  （`nananek/serial-km-emulator` と同じ。旧型フリスクケース向け）
- **RP2350A QFN-60 ベアチップ**（ソケット／モジュールのスタックではない）
- 12 MHz 水晶、W25Q128 QSPI フラッシュ、3.3 V LDO、公式 Minimal Design の
  デカップリング構成
- **USB-C ×2 + USB-A ×1**
  - J1: native USB-MIDI ガジェット（PC/DAW）
  - J2: USB-A ホスト（MIDI キーボード）
  - J3: CH9329 の USB HID（操作対象 PC）
- CH9329 は UART1 / GP4 / GP5 / 9600 8N1 で接続

> **注意:** `hardware/rp2350_frisk/` の Gerber / BOM / CPL は `fab/` にあります。
> DRC の物理エラーは 0 ですが、`/QSPI_SCLK`、`Net-(U1-USB_DP)`、
> `Net-(C4-Pad1)` の 3 ネットだけ未接続で、組立後に細いワイヤで手配線する
> 必要があります。詳細は `hardware/rp2350_frisk/fab/ORDER_NOTES_JA.md` を
> 参照してください。

旧設計 `hardware/rp2350_zero_carrier/`（RP2350-Zero を 2 枚ソケットで載せる
2 ボード構成）は履歴として残しています。新しいビルド対象ではありません。

### ピン割り当て（新基板）

| 機能 | RP2350A pin | GPIO | 接続 |
|------|-------------|------|------|
| PIO-USB D+ | 16 | GP12 | 22Ω → ESD → USB-A J2 D+ |
| PIO-USB D- | 17 | GP13 | 22Ω → ESD → USB-A J2 D- |
| CH9329 RXD | 7 | GP4 | CH9329 TXD (pin 7) |
| CH9329 TXD | 8 | GP5 | CH9329 RXD (pin 8) |
| native USB D- | 51 | USB_DM | USB-C J1 D- |
| native USB D+ | 52 | USB_DP | USB-C J1 D+ |
| 原神モード（任意） | 42 | GP28 | TP3 ↔ GND |
| 楽器モード（任意） | 43 | GP29 | TP4 ↔ GND |

デバッグログは **UART0 (GP0=TX / GP1=RX, 115200)**。CH9329 は UART1 なので
衝突しません。native USB は USB-MIDI が占有します。

---

## 2. ソフトウェア構成

- **core1**: PIO-USB ホスト（`tuh_midi`）。MIDI キーボードの Note On/Off を
  パースし、生 MIDI バイト列を USB-MIDI ガジェット用キューへ流す。
- **core0**: native USB の USB-MIDI デバイス（`tud_midi`）と CH9329 UART。
  Note イベントを 6KRO キーボードレポートへ変換し、UART へ積む。
- コア間はロックフリーキュー。CH9329 のキーボード状態は core0 だけが触る。

### ソースツリー

```
src/
  main.c              # デュアルコア起動、キュー、スイッチ、CH9329 出力
  usb_descriptors.c   # native USB = USB-MIDI デバイス
  tusb_config.h       # device=USB-MIDI / host=PIO-USB MIDI
  midi_host.c/.h      # tuh_midi コールバック（idx ベース）
  midi_parse.c/.h     # 生 MIDI → Note On/Off（純粋・テスト可能）
  midi_bridge.c/.h    # PIO-USB → USB-MIDI パススルー
  note_mapper.c/.h    # MIDI ノート → キースロット（純粋・テスト可能）
  ch9329_protocol.c/.h# CH9329 フレーム組み立て（純粋・テスト可能）
  ch9329_keyboard.c/.h# CH9329 6KRO レポート状態（純粋・テスト可能）
  ch9329_uart.c/.h    # UART1 9600 8N1 送信（ノンブロッキング）
  hid_mute.c/.h       # GP28 原神モード（CH9329 出力ミュート）
  mirror_filter_switch.c/.h # GP29 楽器モード（スメール音階）
  debounced_switch.c/.h
  config.h            # 設定の一元管理
hardware/rp2350_frisk/ # 新基板（外形・配置ドラフト）
hardware/rp2350_zero_carrier/ # 旧 2 ボード構成（履歴）
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
# => build/midi_ch9329_gadget.uf2
```

### ネイティブ

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
./scripts/fetch_deps.sh
cmake -B build -DPICO_BOARD=pico2
cmake --build build -j
```

### 書き込み

BOOTSEL を押しながら USB-C 接続し、`build/midi_ch9329_gadget.uf2` を
RPI-RP2 ドライブへコピーします（または `picotool load ...`）。

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
| `CH9329_UART_INDEX` | `1` | CH9329 に使う UART（0=uart0 / 1=uart1） |
| `CH9329_UART_TX_PIN` | `4` | RP2350 TX → CH9329 RXD |
| `CH9329_UART_RX_PIN` | `5` | RP2350 RX ← CH9329 TXD |
| `CH9329_UART_BAUD` | `9600` | 自動判定が失敗したときのフォールバック |
| `CH9329_AUTO_BAUD` | `1` | 起動時に GET_INFO でボーレートを自動判定する |
| `MUTE_SWITCH_ENABLE` | `1` | GP28 原神モード（CH9329 出力ミュート） |
| `MUTE_SWITCH_PIN` | `28` | スイッチ GPIO |
| `LYRE_SWITCH_ENABLE` | `1` | GP29 楽器モード（スメール音階） |
| `LYRE_SWITCH_PIN` | `29` | スイッチ GPIO |

### キー対応表（既定）

```
低音域 48=Z 50=X 52=C 53=V 55=B 57=N 59=M   (ド レ ミ ファ ソ ラ シ)
中音域 60=A 62=S 64=D 65=F 67=G 69=H 71=J
高音域 72=Q 74=W 76=E 77=R 79=T 81=Y 83=U
```

黒鍵は常にドロップします（原神の楽器に黒鍵が無いため）。

---

## 5. CH9329 プロトコル

CH9329 のキーボード送信フレーム（14 バイト）:

```
57 AB 00 02 08 <modifier> 00 <key1> <key2> <key3> <key4> <key5> <key6> <sum>
```

- `sum` = checksum 自身を除く全バイトの単純和の下位 8 bit
- `key*` は USB HID Keyboard/Keypad Usage（a=0x04 … z=0x1D）
- CH9329 は 6KRO。7 音以上同時に押した場合は、表現できないノートを落とす
  （既存キーを固着させない）

送信は 9600 bps のため 1 フレーム約 15 ms です。UART はノンブロッキングで、
送信中に新しいレポートが来た場合は「最新の状態」を次に送ります。

### ボーレートは起動時に自動判定する

通常の演奏（和音、数音/秒のパッセージ）では 9600 bps でも実用になります。
ただし 1 レポート 14.6 ms なので、15 ms 以内の Note On/Off が重なると
中間状態を間引きます。高速なトリルや非常に細かい和音変化まで残したい場合は、
CH9329 を 115200 bps に設定してください。

ファームは起動時に **GET_INFO を 115200 / 57600 / 38400 / 19200 / 9600 の順で
送信し、正しい応答が返った速度を採用**します。CH9329 の設定を変えても
ファームの再ビルドは不要です。どれにも応答がない場合は
`CH9329_UART_BAUD`（既定 9600）で動作します。

1. 基板の **TP1 (SET) と TP2 (GND)** を短絡したまま、J1 側から WCH の設定
   ツールで CH9329 のボーレートを 115200 に変更する。
2. 短絡を外して再起動する。起動ログに `[ch9329] UART1 @115200 bps` が出る。

自動判定を無効化して固定する場合:

```sh
cmake -B build -DPICO_BOARD=pico2 \
  -DCMAKE_C_FLAGS="-DCH9329_AUTO_BAUD=0 -DCH9329_UART_BAUD=9600"
```

---

## 6. ホスト側ユニットテスト

ARM ツールチェーン不要。純粋ロジックを `cc` で検証します。

```sh
cd tests
make        # note_mapper / midi_parse / ch9329_protocol / ch9329_keyboard
make clean
```

カバー範囲:

- `note_mapper`: 21 キー対応、黒鍵ドロップ、範囲外 IGNORE/WRAP、移調、スメール音階
- `midi_parse`: Note On/Off、vel0=Off、ランニングステータス、リアルタイム、SysEx
- `ch9329_protocol`: フレーム長、ヘッダ、チェックサム、modifier、任意コマンド
- `ch9329_keyboard`: 単音/和音、6KRO 上限、べき等、範囲外、スメール、release_all、
  WRAP のエイリアス参照カウント

---

## 7. 実機キャリブレーション

1. UART0 ログで MIDI キーボードの Note On/Off が出ることを確認。
2. PC 側で USB-MIDI ガジェットが見えることを確認（`aconnect -l` 等）。
3. CH9329 側の PC で HID キーボードとして認識され、キーボードテスターで
   キーが出ることを確認。
4. 原神の演奏画面で「ド」を弾き、画面上のキーと発音が一致するか確認。
   ズレていたら `OCTAVE_OFFSET` を ±12 / ±1 で調整。
5. 3 和音以上を弾いて全音が発音されることを確認（6KRO の範囲）。
6. 黒鍵が無視されることを確認。
7. MIDI キーボードを抜き差しして、CH9329 側の全キーが解放されることを確認。

---

## 8. 既知の制約 / 残作業

- `hardware/rp2350_frisk/` は配置ドラフト。配線、GND/電源プレーン、ERC、
  回路図、ケース開口の確認が未完了です。
- CH9329 は 6KRO のため、7 音以上の同時押しはできません。
- CH9329 の UART は 9600 bps 固定（出荷時設定）。設定ツールで速度を変える場合は
  `CH9329_UART_BAUD` も合わせてください。
- 原神側のキーバインドが変わったら `note_mapper.c` の対応表を更新してください。

---

## 9. 旧構成（履歴）

`hardware/rp2350_zero_carrier/` は RP2350 を 2 枚使う旧構成の名残です。
旧ボード 2（UART→USB-MIDI ブリッジ）のファームは削除しました。現在の
ビルド対象は 1 枚の RP2350A が USB-MIDI ガジェットと CH9329 ドライバを
兼ねる新構成だけです。

## ライセンス

`LICENSE` を参照してください。取得・ビルド依存のライセンスは
`THIRD_PARTY_NOTICES.md` を参照してください。
