// midi_note_filter.c のホスト側ユニットテスト。
//   cc -Wall -Wextra -I../src test_midi_note_filter.c ../src/midi_note_filter.c
//       ../src/note_mapper.c -o t && ./t
// ポリシー別ビルドは -D で config.h の既定値を上書きして確認する
// (例: -DOCTAVE_OFFSET=12)。
#include <stdio.h>
#include <string.h>
#include "midi_note_filter.h"
#include "config.h"

static int failures = 0;

static void expect_output(const char *name,
                          const uint8_t *in, uint32_t len,
                          const uint8_t *want, uint32_t want_len) {
    midi_note_filter_t f;
    midi_note_filter_init(&f);
    uint8_t out[128];
    uint32_t n = midi_note_filter_process(&f, in, len, out, sizeof(out));
    if (n != want_len || (n > 0 && memcmp(out, want, n) != 0)) {
        printf("  FAIL %s: want %u bytes got %u\n", name, want_len, n);
        printf("    want:");
        for (uint32_t i = 0; i < want_len; i++) printf(" %02X", want[i]);
        printf("\n    got: ");
        for (uint32_t i = 0; i < n; i++) printf(" %02X", out[i]);
        printf("\n");
        failures++;
    }
}

// 入力を 2 チャンクに分断して渡し、状態が保持されることを確認する。
static void expect_split(const char *name,
                         const uint8_t *a, uint32_t alen,
                         const uint8_t *b, uint32_t blen,
                         const uint8_t *want, uint32_t want_len) {
    midi_note_filter_t f;
    midi_note_filter_init(&f);
    uint8_t out[128];
    uint32_t n1 = midi_note_filter_process(&f, a, alen, out, sizeof(out));
    uint32_t n2 = midi_note_filter_process(&f, b, blen, out, sizeof(out));
    uint32_t n = n1 + n2;
    if (n1 != 0) {
        printf("  FAIL %s: first chunk produced %u bytes (want 0)\n", name, n1);
        failures++;
        return;
    }
    if (n != want_len || (n > 0 && memcmp(out, want, n) != 0)) {
        printf("  FAIL %s: want %u bytes got %u\n", name, want_len, n);
        printf("    want:");
        for (uint32_t i = 0; i < want_len; i++) printf(" %02X", want[i]);
        printf("\n    got: ");
        for (uint32_t i = 0; i < n; i++) printf(" %02X", out[i]);
        printf("\n");
        failures++;
    }
}

int main(void) {
    printf("config: OCTAVE_OFFSET=%d\n", OCTAVE_OFFSET);

    // 1. ダイアトニック Note On の素通し
    { uint8_t b[] = {0x90, 0x3C, 0x64}; uint8_t w[] = {0x90, 0x3C, 0x64};
      expect_output("noteon", b, sizeof b, w, sizeof w); }

    // 2. ダイアトニック Note Off の素通し
    { uint8_t b[] = {0x80, 0x3C, 0x40}; uint8_t w[] = {0x80, 0x3C, 0x40};
      expect_output("noteoff", b, sizeof b, w, sizeof w); }

    // 3. 範囲外ノート (36 / 84) の破棄 (既定 OCTAVE_OFFSET=0 でのみ成立)
#if OCTAVE_OFFSET == 0
    { uint8_t b[] = {0x90, 0x24, 0x64}; expect_output("out-low", b, sizeof b, NULL, 0); }
    { uint8_t b[] = {0x90, 0x54, 0x64}; expect_output("out-high", b, sizeof b, NULL, 0); }
#endif

    // 範囲内の黒鍵 (C#4=61) はスナップされてゲームキーにマップされるため通過する
    { uint8_t b[] = {0x90, 0x3D, 0x64}; uint8_t w[] = {0x90, 0x3D, 0x64};
      expect_output("blackkey-snap", b, sizeof b, w, sizeof w); }

    // 4. ランニングステータス: 2 音目が明示ステータス 3 バイトで再構成される
    { uint8_t b[] = {0x90, 0x3C, 0x64, 0x3E, 0x40};
      uint8_t w[] = {0x90, 0x3C, 0x64, 0x90, 0x3E, 0x40};
      expect_output("running-status", b, sizeof b, w, sizeof w); }

    // 5. ランニングステータス中の非対応ノート: 破棄され後続は正常再同期
    //    (既定 OCTAVE_OFFSET=0 でのみ 36 が非対応)
#if OCTAVE_OFFSET == 0
    { uint8_t b[] = {0x90, 0x3C, 0x64, 0x24, 0x40}; uint8_t w[] = {0x90, 0x3C, 0x64};
      expect_output("running-drop", b, sizeof b, w, sizeof w); }
#endif

    // 6. CC がランニングステータスを解除: 後の 3E はステータス無しとして破棄
    { uint8_t b[] = {0x90, 0x3C, 0x64, 0xB0, 0x07, 0x7F, 0x3E, 0x40};
      uint8_t w[] = {0x90, 0x3C, 0x64};
      expect_output("cc-breaks-running", b, sizeof b, w, sizeof w); }

    // 7. ピッチベンド / プログラムチェンジの破棄
    { uint8_t b[] = {0xE0, 0x00, 0x40}; expect_output("pitch-bend", b, sizeof b, NULL, 0); }
    { uint8_t b[] = {0xC0, 0x05};       expect_output("program-change", b, sizeof b, NULL, 0); }

    // 8. SysEx の破棄と状態リセット: 直後の Note は正常に通る
    { uint8_t b[] = {0xF0, 0x01, 0x02, 0x03, 0xF7, 0x90, 0x3C, 0x64};
      uint8_t w[] = {0x90, 0x3C, 0x64};
      expect_output("sysex-then-note", b, sizeof b, w, sizeof w); }

    // 9. リアルタイム (F8) の混入: 破棄されつつメッセージは正しく再構成
    { uint8_t b[] = {0x90, 0xF8, 0x3C, 0x64}; uint8_t w[] = {0x90, 0x3C, 0x64};
      expect_output("realtime-mix", b, sizeof b, w, sizeof w); }

    // 10. チャンク分断: status+note と velocity が別チャンクに来ても状態保持
    { uint8_t a[] = {0x90, 0x3C}; uint8_t b[] = {0x64}; uint8_t w[] = {0x90, 0x3C, 0x64};
      expect_split("chunk-split", a, sizeof a, b, sizeof b, w, sizeof w); }

    // 11. Note On vel=0 (実質 Note Off) の素通し
    { uint8_t b[] = {0x90, 0x3C, 0x00}; uint8_t w[] = {0x90, 0x3C, 0x00};
      expect_output("noteon-vel0", b, sizeof b, w, sizeof w); }

#if OCTAVE_OFFSET == 12
    // 12. 移調オフセット適用時は 36 (C2) が +12 でゲームキーに一致して通過する
    { uint8_t b[] = {0x90, 0x24, 0x64}; uint8_t w[] = {0x90, 0x24, 0x64};
      expect_output("offset12-pass", b, sizeof b, w, sizeof w); }
#endif

    if (failures == 0) {
        printf("ALL PASS\n");
        return 0;
    }
    printf("%d FAILURE(S)\n", failures);
    return 1;
}