#include "note_mapper.h"
#include "config.h"

// ---------------------------------------------------------------------------
//  スロット割り当て (指示書 3.1)
//    slot 0..6   低音域  ド レ ミ ファ ソ ラ シ  →  Z X C V B N M
//    slot 7..13  中音域                          →  A S D F G H J
//    slot 14..20 高音域                          →  Q W E R T Y U
//  各スロットの USB HID キーコード (Keyboard/Keypad Page 0x07)。
// ---------------------------------------------------------------------------
const uint8_t note_map_hid_codes[NOTE_MAP_KEY_COUNT] = {
    // 低音域: Z     X     C     V     B     N     M
    0x1D, 0x1B, 0x06, 0x19, 0x05, 0x11, 0x10,
    // 中音域: A     S     D     F     G     H     J
    0x04, 0x16, 0x07, 0x09, 0x0A, 0x0B, 0x0D,
    // 高音域: Q     W     E     R     T     Y     U
    0x14, 0x1A, 0x08, 0x15, 0x17, 0x1C, 0x18,
};

// pitch class (0=C … 11=B) → ダイアトニック音度 0..6。黒鍵は -1。
static const int8_t DEGREE_OF_PC[12] = {
    0,  // 0  C
    -1, // 1  C#
    1,  // 2  D
    -1, // 3  D#
    2,  // 4  E
    3,  // 5  F
    -1, // 6  F#
    4,  // 7  G
    -1, // 8  G#
    5,  // 9  A
    -1, // 10 A#
    6,  // 11 B
};

uint8_t key_index_to_hid_code(uint8_t key_index) {
    if (key_index >= NOTE_MAP_KEY_COUNT) {
        return 0;
    }
    return note_map_hid_codes[key_index];
}

uint8_t note_to_key_index(uint8_t midi_note) {
    // 1. 移調 (キャリブレーション)
    int n = (int)midi_note + (OCTAVE_OFFSET);

    // 2. 範囲外ポリシー
    if (n < NOTE_RANGE_LOW || n > NOTE_RANGE_HIGH) {
#if OUT_OF_RANGE_POLICY == OUT_OF_RANGE_WRAP
        while (n < NOTE_RANGE_LOW)  { n += 12; }
        while (n > NOTE_RANGE_HIGH) { n -= 12; }
        // 3 オクターブ幅なので 12 単位の折り返しで必ず収まるが、保険。
        if (n < NOTE_RANGE_LOW || n > NOTE_RANGE_HIGH) {
            return NOTE_MAP_NONE;
        }
#else
        return NOTE_MAP_NONE;
#endif
    }

    // 3. 半音 (黒鍵) ポリシー。ここで n は必ず [48,83] に収まっている。
    int pc = n % 12; // 48 % 12 == 0 なので pc は C 基準で正しい
    if (DEGREE_OF_PC[pc] < 0) {
#if CHROMATIC_SNAP_POLICY == CHROMATIC_SNAP_UP
        n += 1;
#elif CHROMATIC_SNAP_POLICY == CHROMATIC_SNAP_IGNORE
        return NOTE_MAP_NONE;
#else // CHROMATIC_SNAP_DOWN (デフォルト)
        n -= 1;
#endif
        pc = n % 12;
        // スナップ先は必ず白鍵かつ [48,83] 内 (境界: 49→48, 82→83)。念のため確認。
        if (DEGREE_OF_PC[pc] < 0) {
            return NOTE_MAP_NONE;
        }
    }

    // 4. スロット算出
    int octave = (n - NOTE_RANGE_LOW) / 12; // 0..2
    int degree = DEGREE_OF_PC[pc];          // 0..6
    int slot = octave * 7 + degree;
    if (slot < 0 || slot >= NOTE_MAP_KEY_COUNT) {
        return NOTE_MAP_NONE; // 到達不能だが保険
    }
    return (uint8_t)slot;
}
