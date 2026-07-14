// midi_parse.c のホスト側ユニットテスト。
//   cc -Wall -Wextra -I../src test_midi_parse.c ../src/midi_parse.c -o t && ./t
#include <stdio.h>
#include "midi_parse.h"

typedef struct { uint8_t ch, note; int on; } ev_t;
static ev_t evs[64];
static int n_ev;
static int failures;

static void sink(uint8_t ch, uint8_t note, bool on, void *ctx) {
    (void)ctx;
    if (n_ev < 64) { evs[n_ev].ch = ch; evs[n_ev].note = note; evs[n_ev].on = on; }
    n_ev++;
}

static void run(const char *name, const uint8_t *b, uint32_t n) {
    midi_parser_t p;
    midi_parser_init(&p);
    n_ev = 0;
    midi_parser_push(&p, b, n, sink, 0);
    printf("[%s] %d event(s):", name, n_ev);
    for (int i = 0; i < n_ev && i < 64; i++)
        printf(" %s(ch%u,n%u)", evs[i].on ? "ON" : "off", evs[i].ch, evs[i].note);
    printf("\n");
}

static void expect(const char *name, int idx, uint8_t ch, uint8_t note, int on) {
    if (idx >= n_ev) { printf("  FAIL %s: missing event %d\n", name, idx); failures++; return; }
    ev_t e = evs[idx];
    if (e.ch != ch || e.note != note || e.on != on) {
        printf("  FAIL %s[%d]: want (ch%u,n%u,%s) got (ch%u,n%u,%s)\n",
               name, idx, ch, note, on ? "ON" : "off", e.ch, e.note, e.on ? "ON" : "off");
        failures++;
    }
}
static void expect_count(const char *name, int c) {
    if (n_ev != c) { printf("  FAIL %s: want %d events got %d\n", name, c, n_ev); failures++; }
}

int main(void) {
    { uint8_t b[] = {0x90, 60, 64};                 run("noteon", b, sizeof b);
      expect_count("noteon", 1); expect("noteon", 0, 0, 60, 1); }

    { uint8_t b[] = {0x90, 60, 0};                  run("noteon-vel0", b, sizeof b);
      expect_count("noteon-vel0", 1); expect("noteon-vel0", 0, 0, 60, 0); }

    { uint8_t b[] = {0x80, 60, 64};                 run("noteoff", b, sizeof b);
      expect_count("noteoff", 1); expect("noteoff", 0, 0, 60, 0); }

    { uint8_t b[] = {0x92, 60, 64};                 run("channel2", b, sizeof b);
      expect_count("channel2", 1); expect("channel2", 0, 2, 60, 1); }

    // ランニングステータス: 3 音の和音 (60,64,67)
    { uint8_t b[] = {0x90, 60, 64, 64, 64, 67, 64}; run("chord-running", b, sizeof b);
      expect_count("chord-running", 3);
      expect("chord-running", 0, 0, 60, 1);
      expect("chord-running", 1, 0, 64, 1);
      expect("chord-running", 2, 0, 67, 1); }

    // ランニングステータス + vel0 で off
    { uint8_t b[] = {0x90, 60, 64, 60, 0};          run("running-off", b, sizeof b);
      expect_count("running-off", 2);
      expect("running-off", 0, 0, 60, 1);
      expect("running-off", 1, 0, 60, 0); }

    // リアルタイムメッセージ (0xF8 clock) が途中に挟まる
    { uint8_t b[] = {0x90, 0xF8, 60, 0xF8, 64};     run("realtime-interleave", b, sizeof b);
      expect_count("realtime-interleave", 1);
      expect("realtime-interleave", 0, 0, 60, 1); }

    // Control Change は無視
    { uint8_t b[] = {0xB0, 0x07, 0x7F};             run("cc-ignored", b, sizeof b);
      expect_count("cc-ignored", 0); }

    // SysEx をまたいで復帰
    { uint8_t b[] = {0xF0, 0x7E, 0x00, 0x06, 0xF7, 0x90, 60, 64};
      run("sysex-skip", b, sizeof b);
      expect_count("sysex-skip", 1); expect("sysex-skip", 0, 0, 60, 1); }

    // Program Change (1 データ) の後に Note On
    { uint8_t b[] = {0xC0, 0x05, 0x90, 62, 100};    run("pc-then-note", b, sizeof b);
      expect_count("pc-then-note", 1); expect("pc-then-note", 0, 0, 62, 1); }

    // Note Off ステータスのランニングステータス
    { uint8_t b[] = {0x80, 60, 0, 64, 0};           run("noteoff-running", b, sizeof b);
      expect_count("noteoff-running", 2);
      expect("noteoff-running", 0, 0, 60, 0);
      expect("noteoff-running", 1, 0, 64, 0); }

    if (failures == 0) { printf("ALL PASS\n"); return 0; }
    printf("%d FAILURE(S)\n", failures);
    return 1;
}
