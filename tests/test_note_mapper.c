// note_mapper.c のホスト側ユニットテスト。
// ARM ツールチェーン不要。次で実行:
//   cc -I../src -I. test_note_mapper.c ../src/note_mapper.c -o t && ./t
// ポリシー別ビルドは -D で config.h の既定値を上書きして確認する。
#include <stdio.h>
#include "note_mapper.h"
#include "config.h"

static int failures = 0;

static char code_to_letter(uint8_t code) {
    // 0x04..0x1D が A..Z に対応 (HID Keyboard/Keypad Page)
    if (code >= 0x04 && code <= 0x1D) {
        return (char)('A' + (code - 0x04));
    }
    return '?';
}

static void expect_key(uint8_t note, char want) {
    uint8_t idx = note_to_key_index(note);
    char got = (idx == NOTE_MAP_NONE) ? '.' : code_to_letter(key_index_to_hid_code(idx));
    if (got != want) {
        printf("  FAIL note %3u: want '%c' got '%c' (slot=%u)\n", note, want, got, idx);
        failures++;
    }
}

static void expect_none(uint8_t note) {
    uint8_t idx = note_to_key_index(note);
    if (idx != NOTE_MAP_NONE) {
        printf("  FAIL note %3u: want NONE got slot=%u ('%c')\n",
               note, idx, code_to_letter(key_index_to_hid_code(idx)));
        failures++;
    }
}

int main(void) {
    printf("config: OCTAVE_OFFSET=%d OUT_OF_RANGE_POLICY=%d CHROMATIC_SNAP_POLICY=%d\n",
           OCTAVE_OFFSET, OUT_OF_RANGE_POLICY, CHROMATIC_SNAP_POLICY);

#if OCTAVE_OFFSET == 0
    // --- 指示書 3.1 の対応表 (デフォルト設定でのみ厳密一致) ---
    // 低音域
    expect_key(48,'Z'); expect_key(50,'X'); expect_key(52,'C'); expect_key(53,'V');
    expect_key(55,'B'); expect_key(57,'N'); expect_key(59,'M');
    // 中音域
    expect_key(60,'A'); expect_key(62,'S'); expect_key(64,'D'); expect_key(65,'F');
    expect_key(67,'G'); expect_key(69,'H'); expect_key(71,'J');
    // 高音域
    expect_key(72,'Q'); expect_key(74,'W'); expect_key(76,'E'); expect_key(77,'R');
    expect_key(79,'T'); expect_key(81,'Y'); expect_key(83,'U');

    // --- 黒鍵ポリシー ---
    // C#4=61, D#4=63, F#4=66, G#4=68, A#4=70
#  if CHROMATIC_SNAP_POLICY == CHROMATIC_SNAP_DOWN
    expect_key(61,'A'); expect_key(63,'S'); expect_key(66,'F');
    expect_key(68,'G'); expect_key(70,'H');
#  elif CHROMATIC_SNAP_POLICY == CHROMATIC_SNAP_UP
    expect_key(61,'S'); expect_key(63,'D'); expect_key(66,'G');
    expect_key(68,'H'); expect_key(70,'J');
    // 上端の A#5=82 は UP で B5→U に収まる
    expect_key(82,'U');
#  elif CHROMATIC_SNAP_POLICY == CHROMATIC_SNAP_IGNORE
    expect_none(61); expect_none(63); expect_none(66);
    expect_none(68); expect_none(70);
#  endif

    // --- 範囲外ポリシー ---
    // 47 以下 / 84 以上
#  if OUT_OF_RANGE_POLICY == OUT_OF_RANGE_IGNORE
    expect_none(47); expect_none(36); expect_none(84); expect_none(96);
#  elif OUT_OF_RANGE_POLICY == OUT_OF_RANGE_WRAP
    // 36 (C2) → +12 → 48 (C) = Z, 84 (C6) → -12 → 72 (C) = Q
    expect_key(36,'Z'); expect_key(84,'Q');
    // 24 (C1) → +12+12 → 48 = Z
    expect_key(24,'Z');
    // 85 (C#6) は WRAP→73(C#5) 後に黒鍵ポリシー適用
#    if CHROMATIC_SNAP_POLICY == CHROMATIC_SNAP_DOWN
    expect_key(85,'Q'); // C#5→C5=Q
#    endif
#  endif
#endif // OCTAVE_OFFSET == 0

    if (failures == 0) {
        printf("ALL PASS\n");
        return 0;
    }
    printf("%d FAILURE(S)\n", failures);
    return 1;
}
