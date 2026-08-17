# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

RP2350 ファームウェア。USB MIDI キーボードの演奏を解析し、NKRO USB HID キーボードとして
PC に送って原神「諧律のチェンバロ」を弾けるようにする。3 オクターブ × ダイアトニック 7 音
= 21 キー固定、和音対応、ベロシティ無視。

同一ビルドで **2 本の uf2** を生成する:
- `build/genshin_midi_kbd.uf2` = ボード1: MIDI→HID コンバータ + RAW MIDI の UART ミラー出力
  (`midi_mirror.c`、UART1/GP4/31250、`config.h` で無効化可)。
- `build/serial_midi_device.uf2` = ボード2: シリアル(UART1/GP5/31250)→ USB-MIDI ブリッジ。

## コマンド

### ホスト側ユニットテスト(ARM 不要・最速の検証手段)
純粋ロジック 3 モジュール(`note_mapper` / `midi_parse` / `nkro_report`)はホスト `cc` で検証する。
これがこのリポジトリで最も速く回せる検証ループなので、それらを触ったらまずここを回す。
```sh
cd tests && make            # 3 スイートをビルド&実行
make t_note_mapper && ./t_note_mapper   # 単体スイートだけ
# ポリシー別ビルドは config.h の既定値を -D で上書き:
cc -I../src -I. -DCHROMATIC_SNAP_POLICY=1 -DOUT_OF_RANGE_POLICY=1 \
   test_note_mapper.c ../src/note_mapper.c -o t && ./t
```

### ファームのビルド
```sh
./scripts/build_docker.sh   # 推奨。ホストに ARM toolchain 不要。両 uf2 を生成:
                            #   build/genshin_midi_kbd.uf2 (ボード1)
                            #   build/serial_midi_device.uf2 (ボード2)
# ネイティブ:
export PICO_SDK_PATH=/path/to/pico-sdk
./scripts/fetch_deps.sh && cmake -B build -DPICO_BOARD=pico2 && cmake --build build -j
```
`scripts/fetch_deps.sh` が依存を `lib/`(gitignored)へ取得する。`lib/` を消せばクリーンビルド。
デバッグ出力は **UART**(GP0/GP1, 115200)。native USB は HID/MIDI が占有するので stdio-USB は使えない。

## アーキテクチャ(複数ファイルにまたがる要点)

**二系統 USB / 二コアの一方向パイプライン。** `main.c` が起点:
- **core1** = PIO-USB ホスト (`tuh_midi`, rhport 1)。`midi_host.c` が `tuh_midi_*` コールバックを実装。
- **core0** = native USB デバイス (`tud_hid`, rhport 0)。`hid_device.c` が NKRO レポートを送出。
- 受け渡しは `pico/util/queue.h` のロックフリーキュー。**HID 状態は core0 のみ、MIDI パースは
  core1 のみが触る**(排他は持たない前提)。コア間のシームは `app_on_midi_note` /
  `app_on_midi_disconnect`(`midi_host.h` で宣言、`main.c` が実装してキューへ push)。

**モジュールは「純粋 / TinyUSB 依存」で二層に分けている**(だからテストが速い):
- 純粋・テスト可能: `note_mapper`(ノート→キースロット変換)、`midi_parse`(生 MIDI バイト列→
  Note On/Off、running status / realtime / SysEx 対応)、`nkro_report`(レポート状態)。
- TinyUSB 依存(実機/Docker ビルドでのみ検証): `hid_device` / `midi_host` / `usb_descriptors` / `main`。
- **`midi_mirror` もハード依存層**(`hardware/uart.h` を直接使う)なのでホストテスト対象外。
  無効時(`MIDI_UART_MIRROR_ENABLE=0`)は関数本体ごと `#if` でコンパイルアウトされる。
- **`hid_mute` / `mirror_filter_switch` もハード依存層**(`hardware/gpio.h` を直接使う)なので
  ホストテスト対象外。無効時(`MUTE_SWITCH_ENABLE=0` / `MIRROR_FILTER_SWITCH_ENABLE=0`)は
  no-op になり、`#if` でコンパイルアウトされる。
- 新しいロジックは可能な限り純粋層へ置き、`tests/` を足すこと。

**変換パイプライン**(`note_mapper.c`):移調(`OCTAVE_OFFSET`)→ 範囲外ポリシー → 黒鍵スナップ
→ スロット算出。全ポリシーは `config.h` のコンパイル時定数。対応表・キーコードもここに集約。

**RAW MIDI ミラー**(`midi_host.c` → `midi_mirror.c`):`tuh_midi_rx_cb` の読出しループで
`tuh_midi_stream_read` が返す**生バイト列**を `uart_write_blocking` で別 UART へ流す
(パース・変換は通さない。CIN 除去済みの MIDI 1.0 ストリーム)。ブロッキング送信のため
大きな SysEx 中は core1 の `tuh_task` が数十 ms 止まり得る(既知のトレードオフ)。

**ボード2**(`serial_midi_device/`):PIO-USB・デュアルコア不要のシングルコア・デバイスのみ。
UART1(RX=GP5/31250)で受けた生 MIDI バイト列を `tud_midi_stream_write` で USB-MIDI へ。
`tud_midi` は FIFO 満杯時に部分書き込みを返すため、`s_carry[64]` に未送信分を持ち越す
(2 回連続で満杯なら新しい分は捨てる。ミラー用途として許容)。`set_sys_clock_khz` は不要。

**NKRO レポートのビット配置は 2 ファイルで一致させること**(暗黙結合):
- `usb_descriptors.c` の HID レポートディスクリプタ = modifier 1 バイト + Usage 0x04..0x1D(a..z)
  の 26 ビット + 6 パディング = 全 5 バイト。
- `nkro_report.c` は「キーコード `c` のビット位置 = `c - 0x04`」で同じ配置に書く。
  片方を変えたら必ずもう片方も直す。`nkro_report` は MIDI ノート単位のべき等(`note_active[128]`)
  + スロット単位の参照カウント(黒鍵スナップ/WRAP で 2 音が同一ゲームキーに丸まる場合の対策)を持つ。

## ビルド依存で必ず踏む落とし穴

- **MIDI ホストには upstream `hathach/tinyusb` 0.20.0 が必須**。pico-sdk 同梱の
  `raspberrypi/tinyusb` には `midi_host` が無く `tuh_midi_*` をリンクできない。`CMakeLists.txt` は
  `PICO_TINYUSB_PATH` を upstream に向け、SDK の `tinyusb_host` が列挙しない
  `midi_host.c` / `dcd_pio_usb.c` / `hcd_pio_usb.c` を **手動でソースに追加**している。
- `tuh_midi` は **idx ベース API**(旧 `dev_addr` ベースではない)。mount cb は
  `tuh_midi_mount_cb_t*` 構造体を取る。古いチュートリアルの `dev_addr` 版に引きずられないこと。
- **rootless docker** では `docker run --user` を付けると bind mount に書けない
  (container-root が呼び出しユーザにマップされる)。`scripts/build_docker.sh` が rootless を検出して
  付け分ける。手で `docker run` する場合も同様に注意。
- **`serial_midi_device/` は独立した include 空間を持つ**(src/ を include パスに足さない)。
  `src/tusb_config.h`(HID 用)や `src/config.h` を拾うと衝突する。`pico_sdk_import.cmake` の
  再 include / `pico_sdk_init` 二重呼び出しも禁止(ルートが済ませている)。

## ハードウェア(Waveshare RP2350-USB-A 前提)

- MIDI 入力の USB-A ポート = **D+ = GP12 / D− = GP13**(`config.h` の `PIN_USB_HOST_DP`)。
- **ホスト動作には D+ プルアップ抵抗 R13(1.5kΩ)の除去が必須**。除去しないと MIDI キーボードが
  列挙されない/ホットプラグ検出できない。詳細は `README.md` §1。
- ネイティブ USB(USB-C)= HID デバイス側。2 ポートは物理的に別。
- ボード1 のミラー出力は **UART1 TX=GP4**、ボード2 のミラー入力は **UART1 RX=GP5**、両者を直結
  (+GND 共有)。ボード2 のデバッグログは UART0 なので衝突しない。GP12/13(PIO-USB)は使わない。
- 任意スイッチ: **GP28 = ミュートスイッチ** (LOW アクティブ、内部プルアップ、閉=HID 出力 OFF。
  UART ミラーは継続)、**GP29 = フィルタースイッチ** (LOW アクティブ、閉=ミラーを原神鍵盤の
  Note On/Off のみに絞る)。どちらも `config.h` で無効化可。

## キャリブレーション

暫定 C4=MIDI60 基準。実機で原神と鳴らし比べてズレたら `config.h` の `OCTAVE_OFFSET`(半音単位、
12=1oct)で移調補正。黒鍵の丸め方は `CHROMATIC_SNAP_POLICY` を実際に弾いて選ぶ。手順は `README.md` §6。
