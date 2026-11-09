# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

RP2350A ファームウェア。USB MIDI キーボードの演奏を解析し、2 系統へ出力する:

1. **native USB = USB-MIDI ガジェット**（PC/DAW へ生 MIDI をパススルー）
2. **UART1 = CH9329**（操作対象 PC へ 6KRO HID キーボードを送信）

旧構成の「RP2350 2 枚 + HID デバイス + UART ミラー」は廃止。ビルド成果物は
`build/midi_ch9329_gadget.uf2` の 1 本だけ。

## コマンド

### ホスト側ユニットテスト(ARM 不要・最速の検証手段)
純粋ロジック 4 モジュール(`note_mapper` / `midi_parse` / `ch9329_protocol` /
`ch9329_keyboard`)をホスト `cc` で検証する。触ったらまずここを回す。
```sh
cd tests && make
make t_ch9329_keyboard && ./t_ch9329_keyboard
# WRAP ポリシー専用ビルド:
make t_ch9329_keyboard_wrap && ./t_ch9329_keyboard_wrap
```

### ファームのビルド
```sh
./scripts/build_docker.sh   # 推奨。build/midi_ch9329_gadget.uf2 を生成
# ネイティブ:
export PICO_SDK_PATH=/path/to/pico-sdk
./scripts/fetch_deps.sh && cmake -B build -DPICO_BOARD=pico2 && cmake --build build -j
```
デバッグ出力は **UART0**(GP0/GP1, 115200)。native USB は USB-MIDI が占有する。
CH9329 は **UART1**(TX=GP4 / RX=GP5, 9600 8N1)。

## アーキテクチャ

**デュアルコアの一方向パイプライン。** `main.c` が起点:
- **core1** = PIO-USB ホスト (`tuh_midi`, rhport 1)。`midi_host.c` がコールバック。
- **core0** = native USB-MIDI デバイス (`tud_midi`, rhport 0) + CH9329 UART。
- core1 で受けた生 MIDI バイト列は `midi_bridge_send()` でキューへ積み、
  core0 の `midi_bridge_task()` が `tud_midi_stream_write()` で PC へ送る。
- Note On/Off は `app_on_midi_note` / `app_on_midi_disconnect`（`midi_host.h`）で
  core0 のキューへ渡す。

**モジュールは「純粋 / ハード依存」で二層**:
- 純粋・テスト可能:
  - `note_mapper`: MIDI ノート → 21 キースロット（黒鍵ドロップ、移調、スメール音階）。
  - `midi_parse`: 生 MIDI → Note On/Off（ランニングステータス / SysEx 対応）。
  - `ch9329_protocol`: CH9329 フレーム組み立てとチェックサム。
  - `ch9329_keyboard`: 6KRO キーボードレポート状態（参照カウント、release_all）。
- ハード依存:
  - `midi_host` / `midi_bridge` / `ch9329_uart` / `usb_descriptors` / `main`。
  - `hid_mute`（GP28）/ `mirror_filter_switch`（GP29）/ `debounced_switch`。
- 新しいロジックは可能な限り純粋層へ置き、`tests/` を足すこと。

**CH9329 の送信**(`ch9329_uart.c`):
- フレームは `57 AB 00 02 08 <modifier> 00 <key1..6> <sum>` の 14 バイト。
- `sum` は checksum 自身を除く全バイトの単純和の下位 8 bit。
- 9600 bps なので 1 フレーム約 15 ms。`uart_write_blocking` は使わず、
  `uart_is_writable` の範囲で少しずつ書き、送信中に新しいレポートが来たら
  「最新の状態」だけを次に送る（古い中間状態は捨てる）。
- CH9329 は 6KRO。7 音以上は表現できない分を落とす。既存キーは固着させない。

**CH9329 のキーボード状態**(`ch9329_keyboard.c`):
- MIDI ノート単位の `note_active[128]` と、キースロット単位の参照カウントを持つ。
- `OUT_OF_RANGE_POLICY=WRAP` で 2 つのノートが同じキーに丸まる場合、
  片方を離してもキーを保持する。`tests/Makefile` の `t_ch9329_keyboard_wrap`
  だけがこの経路をカバーする。
- 6 キー満杯で新しいキーが来たら、そのノートは落とす（押下状態も立てない）。

**MIDI パススルー**(`midi_bridge.c`):
- `tud_midi_stream_write` は部分書き込みを返すため、core0 の `s_carry` に
  未送信分を持ち越す。
- USB 未列挙時はキューを捨てる（再接続時に古いノートが一気に流れて固着するのを防ぐ）。
- MIDI ストリーム状態機械は core0 だけが触る。他の送信者を追加しないこと。

**スイッチ**:
- **GP28 = 原神モード**（LOW アクティブ、内部プルアップ）。ミュート中は
  CH9329 出力を停止し、USB-MIDI パススルーは継続する。突入時に全キー解放、
  解除時に保持ノートを再アサートする。
- **GP29 = 楽器モード**（LOW アクティブ）。スメール音階に切り替える。
  切替時に全キー解放 → 再配置する。
- どちらも `config.h` で無効化可。デバウンス本体は `debounced_switch.c` に共通化。

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
- J1 = native USB-MIDI ガジェット（USB-C、左短辺）。
- J2 = USB-A ホスト（MIDI キーボード、上長辺）。VBUS は TPS2051B。
- J3 = CH9329 の USB HID（USB-C、右短辺）。
- J1/J3 の VBUS は BAT54WS でダイオード OR。
- DRC は物理エラー 0 / unconnected 3。残る 3 ネット
  (`/QSPI_SCLK` / `Net-(U1-USB_DP)` / `Net-(C4-Pad1)`) は
  `fab/ORDER_NOTES_JA.md` の座標間を細いワイヤで手配線する。
- 発注ファイル: `fab/rp2350_frisk_gerbers_JLCPCB.zip`、
  `fab/jlc_bom.csv`、`fab/jlc_cpl.csv`。BOM の `TBD` は発注前に確定する。
- 旧 `hardware/rp2350_zero_carrier/` は履歴。新しい契約テストや CI を作る場合は
  `hardware/rp2350_frisk/` 向けに作り直すこと。

## キャリブレーション

暫定 C4=MIDI60 基準。実機で原神と鳴らし比べてズレたら `config.h` の
`OCTAVE_OFFSET`（半音単位、12=1oct）で移調補正。黒鍵は常にドロップ。
手順は `README.md` §7。
