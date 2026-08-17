#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "midi_mirror.h"
#include "midi_note_filter.h"
#include "mirror_filter_switch.h"
#include "config.h"

// ===========================================================================
//  RAW MIDI の UART ミラー出力 (ハード依存層)。
//  tuh_midi_stream_read が返す生 MIDI バイト列をそのまま別 UART へ流す。
//
//  ※ ブロッキングのトレードオフ: uart_write_blocking は TX FIFO に詰めるまで
//    コア1 (tuh_task) を止める。31250 baud では 1 バイト ≈ 320µs。
//    - Note On/Off (3 バイト) や CC クラス: 1ms 未満。演奏用途で問題なし。
//    - 大きな SysEx ダンプ: 64 バイトで ~20ms ほど止まるが、ミラー用途として許容。
// ===========================================================================

#if MIDI_UART_MIRROR_ENABLE

// UART インスタンス選択 (コンパイル時分岐で uart0/uart1 を解決)
#if MIDI_UART_MIRROR_UART == 0
#define MIRROR_UART uart0
#else
#define MIRROR_UART uart1
#endif

// ランニングステータス展開 (最大 1.5 倍) + 前回呼び出しからの持ち越し 1 note 分を
// 見込んだ安全な上限。MIDI_STREAM_CHUNK_MAX (= tuh_midi_stream_read の 1 回の
// 最大読み出しバイト数、midi_host.c と共有) に対して計算する。
#define MIRROR_FILTERED_BUF_LEN ((MIDI_STREAM_CHUNK_MAX * 3) / 2 + 3)

// フィルターは入力チャンクをまたいで状態を保つため、ミラー全体で 1 個保持する。
static midi_note_filter_t s_filter;

// 実際に適用中のモード (true = フィルター済み 3 バイト再構成 / false = 完全パススルー)。
// mirror_filter_switch_is_enabled() の生値をそのまま使わず、s_filter がメッセージ
// 境界 (READY) にあるときだけここへ反映する。メッセージ途中で切り替えると
// バッファ済みのステータス+note が宙に浮いてバイト列が破損するため。
static bool s_filter_active;

static void sync_filter_mode(void) {
    if (midi_note_filter_is_ready(&s_filter)) {
        s_filter_active = mirror_filter_switch_is_enabled();
    }
}

void midi_mirror_init(void) {
    uart_init(MIRROR_UART, MIDI_UART_MIRROR_BAUD); // 8N1 がデフォルト
    gpio_set_function(MIDI_UART_MIRROR_TX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(MIRROR_UART, true);
    midi_note_filter_init(&s_filter);
    s_filter_active = mirror_filter_switch_is_enabled();
}

// デバイス再マウント時に呼ぶ (midi_host の mount cb から)。ランニングステータス等をリセット。
void midi_mirror_reset(void) {
    midi_note_filter_init(&s_filter);
    s_filter_active = mirror_filter_switch_is_enabled();
}

void midi_mirror_send(const uint8_t *data, uint32_t len) {
    if (len == 0) {
        return;
    }
    // モード切替はメッセージ境界でのみ反映する (上の s_filter_active コメント参照)。
    sync_filter_mode();

    // パススルー中もランニングステータス等の内部状態を追跡し続けるため、
    // モードに関わらず毎回 process を通す (境界判定を常に正確に保つ)。
    uint8_t filtered[MIRROR_FILTERED_BUF_LEN];
    uint32_t n = midi_note_filter_process(&s_filter, data, len, filtered, sizeof(filtered));

    if (s_filter_active) {
        if (n > 0) {
            uart_write_blocking(MIRROR_UART, filtered, n);
        }
    } else {
        uart_write_blocking(MIRROR_UART, data, len); // 完全パススルー (バイト同一)
    }
}

#else // 無効時は no-op (コードは残すが何もしない)

void midi_mirror_init(void) {}
void midi_mirror_send(const uint8_t *data, uint32_t len) { (void)data; (void)len; }
void midi_mirror_reset(void) {}

#endif
