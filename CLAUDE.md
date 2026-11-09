# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

RP2350A ファームウェア。USB MIDI キーボードの演奏を解析し、2 系統へ出力する:

1. **native USB = NKRO HID キーボード**（操作対象 PC へ）
2. **UART1 = CH345**（制御 PC/DAW へ USB-MIDI を出す UART-MIDI ブリッジ）

旧構成の「RP2350 2 枚 + CH9329」は廃止。ビルド成果物は
`build/midi_nkro_ch345.uf2` の 1 本だけ。

## コマンド

### ホスト側ユニットテスト(ARM 不要・最速の検証手段)
純粋ロジック 3 モジュール(`note_mapper` / `midi_parse` / `nkro_report`)を
ホスト `cc` で検証する。触ったらまずここを回す。
```sh
cd tests && make
# WRAP ポリシー専用ビルド:
make t_nkro_report_wrap && ./t_nkro_report_wrap
```

### ファームのビルド
```sh
./scripts/build_docker.sh   # 推奨。build/midi_nkro_ch345.uf2 を生成
# ネイティブ:
export PICO_SDK_PATH=/path/to/pico-sdk
./scripts/fetch_deps.sh && cmake -B build -DPICO_BOARD=pico2 && cmake --build build -j
```
デバッグ出力は **UART0**(GP0/GP1, 115200)。native USB は NKRO HID が占有する。
CH345 は **UART1**(TX=GP4 / RX=GP5, 31250 8N1)。

## アーキテクチャ

**デュアルコアの一方向パイプライン。** `main.c` が起点:
- **core1** = PIO-USB ホスト (`tuh_midi`, rhport 1)。`midi_host.c` がコールバック。
- **core0** = native USB NKRO HID デバイス (`tud_hid`, rhport 0)。
- core1 で受けた生 MIDI バイト列は `midi_uart_send()` で CH345 へ
  UART1 31250 のまま流す。
- Note On/Off は `app_on_midi_note` / `app_on_midi_disconnect`（`midi_host.h`）で
  core0 のキューへ渡し、`nkro_report` で 26 ビットの HID レポートにする。

**モジュールは「純粋 / ハード依存」で二層**:
- 純粋・テスト可能:
  - `note_mapper`: MIDI ノート → 21 キースロット（黒鍵ドロップ、移調、スメール音階）。
  - `midi_parse`: 生 MIDI → Note On/Off（ランニングステータス / SysEx 対応）。
  - `nkro_report`: 26 ビット NKRO レポート状態（参照カウント、release_all）。
- ハード依存:
  - `midi_host` / `midi_uart` / `hid_device` / `usb_descriptors` / `main`。
  - `hid_mute`（GP28）/ `mirror_filter_switch`（GP29）/ `debounced_switch`。
- 新しいロジックは可能な限り純粋層へ置き、`tests/` を足すこと。

**CH345 の送信**(`midi_uart.c`):
- PIO-USB ホストが受けた生 MIDI 1.0 バイト列を、UART1 31250 8N1 で
  そのまま CH345 へ送る。CH345 が USB-MIDI クラスとして PC/DAW に見える。
- パース・再ステータス付与・フィルタはしない。CC/ピッチベンド/SysEx も通る。
- `uart_write_blocking` のため、大きな SysEx 中は core1 の `tuh_task` が
  数十 ms 止まり得る（既知のトレードオフ）。

**スイッチ**:
- **GP28 = 原神モード**（LOW アクティブ、内部プルアップ）。ミュート中は
  NKRO HID 出力を停止し、CH345 への MIDI 出力は継続する。突入時に全キー解放、
  解除時に保持ノートを再アサートする。
- **GP29 = 楽器モード**（LOW アクティブ）。スメール音階に切り替える。
  切替時に全キー解放 → 再配置する。
- **GP27 = 予備スイッチ**（LOW アクティブ）。旧ボード2のサステインスイッチを
  ガジェット側へ移したもの。既定では状態を読むだけで MIDI へは送らない。
  `SPARE_SWITCH_CC` を 0..127 にすると、エッジでその CC を CH345 へ送る
  （64 にすると従来のサステイン相当）。UART は `midi_uart` の mutex で
  core1 の RAW MIDI と直列化される。
- 3つとも `config.h` で無効化可。デバウンス本体は `debounced_switch.c` に共通化。

## ビルド依存で必ず踏む落とし穴

- **MIDI ホストには upstream `hathach/tinyusb` 0.20.0 が必須**。pico-sdk 同梱の
  `raspberrypi/tinyusb` には `midi_host` が無い。`CMakeLists.txt` は
  `PICO_TINYUSB_PATH` を upstream に向け、SDK が列挙しない
  `midi_host.c` / `dcd_pio_usb.c` / `hcd_pio_usb.c` を手動でソースに追加している。
- `tuh_midi` は **idx ベース API**。mount cb は `tuh_midi_mount_cb_t*` を取る。
- **rootless docker** では `docker run --user` を付けると bind mount に書けない。
  KiCad コンテナを使うときは `--user 0:0` が必要（rootless の root が
  ホストユーザにマップされる）。
- `PICO_BOARD=pico2` は RP2350A の既定。ベアチップ基板でも SDK のボード定義は
  `pico2` を流用している（USB/PIO の設定が同じため）。

## ハードウェア

新基板は `hardware/rp2350_frisk/`。発注データは `fab/` にある。

- 外形 **57.6 × 31.4 mm**、四隅 2 mm 45° パンチカット。
- **RP2350A QFN-60 ベアチップ** + W25Q128 + 12 MHz 水晶 + 3.3 V LDO。
- J1 = native USB **NKRO HID**（USB-C、左短辺）。
- J2 = USB-A ホスト（MIDI キーボード、上長辺）。VBUS は TPS2051B。
- J3 = **CH345 の USB-MIDI**（USB-C、右短辺）。
- J1/J3 の VBUS は BAT54WS でダイオード OR。
- 現在の `fab/` は旧 CH9329 試作のデータ。CH345 版へ再設計するまで発注しない。
- CH345 のパッケージ・ピン配置・水晶要否をデータシートで確定してから、
  U4 周辺（UART/USB/水晶）を再配線する。
- 旧 `hardware/rp2350_zero_carrier/` は履歴。新しい契約テストや CI を作る場合は
  `hardware/rp2350_frisk/` 向けに作り直すこと。

## キャリブレーション

暫定 C4=MIDI60 基準。実機で原神と鳴らし比べてズレたら `config.h` の
`OCTAVE_OFFSET`（半音単位、12=1oct）で移調補正。黒鍵は常にドロップ。
手順は `README.md` §7。
