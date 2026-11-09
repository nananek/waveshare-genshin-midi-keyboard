// ch9329_keyboard.c のホスト側ユニットテスト。
//   cc -I../src -I. test_ch9329_keyboard.c ../src/ch9329_keyboard.c ../src/note_mapper.c -o t && ./t
// OUT_OF_RANGE_POLICY=WRAP のエイリアス経路は t_ch9329_keyboard_wrap で検証する。
#include <stdio.h>
#include <stdbool.h>

#include "ch9329_keyboard.h"
#include "config.h"

static int failures;

static void expect_bool(const char *name, bool got, bool want) {
    if (got != want) {
        printf("  FAIL %s: want %s got %s\n", name,
               want ? "true" : "false", got ? "true" : "false");
        failures++;
    }
}

static bool has_key(uint8_t code) {
    const uint8_t *keys = ch9329_keyboard_keys();
    for (int i = 0; i < CH9329_KEYBOARD_KEY_COUNT; i++) {
        if (keys[i] == code) {
            return true;
        }
    }
    return false;
}

static void expect_key(const char *name, uint8_t code, bool want) {
    if (has_key(code) != want) {
        printf("  FAIL %s: key 0x%02X want %s\n", name, code,
               want ? "present" : "absent");
        failures++;
    }
}

static void expect_empty(const char *name) {
    const uint8_t *keys = ch9329_keyboard_keys();
    for (int i = 0; i < CH9329_KEYBOARD_KEY_COUNT; i++) {
        if (keys[i] != 0) {
            printf("  FAIL %s: keys[%d]=0x%02X (expected empty)\n",
                   name, i, keys[i]);
            failures++;
            return;
        }
    }
}

int main(void) {
    // --- 単音: note60 → A(0x04) ---
    ch9329_keyboard_init();
    expect_bool("on60", ch9329_keyboard_note_on(60), true);
    expect_key("on60", 0x04, true);
    expect_bool("off60", ch9329_keyboard_note_off(60), true);
    expect_empty("off60");
    expect_bool("modifier", ch9329_keyboard_modifier() == 0, true);

    // --- 和音と 6KRO 上限 ---
    ch9329_keyboard_init();
    expect_bool("on60", ch9329_keyboard_note_on(60), true); // A
    expect_bool("on62", ch9329_keyboard_note_on(62), true); // S
    expect_bool("on64", ch9329_keyboard_note_on(64), true); // D
    expect_bool("on65", ch9329_keyboard_note_on(65), true); // F
    expect_bool("on67", ch9329_keyboard_note_on(67), true); // G
    expect_bool("on69", ch9329_keyboard_note_on(69), true); // H
    expect_bool("on71-full", ch9329_keyboard_note_on(71), false); // J は 7 音目
    expect_key("on71-full", 0x0D, false);
    expect_bool("off71-dropped", ch9329_keyboard_note_off(71), false);
    expect_bool("off60", ch9329_keyboard_note_off(60), true);
    expect_key("off60", 0x04, false);
    expect_key("off60-keep-S", 0x16, true);

    // --- べき等: 同じノートの二重 On/Off ---
    ch9329_keyboard_init();
    expect_bool("idem-on1", ch9329_keyboard_note_on(72), true);
    expect_bool("idem-on2", ch9329_keyboard_note_on(72), false);
    expect_bool("idem-off1", ch9329_keyboard_note_off(72), true);
    expect_bool("idem-off2", ch9329_keyboard_note_off(72), false);

    // --- 範囲外・黒鍵 ---
    ch9329_keyboard_init();
    expect_bool("black-61", ch9329_keyboard_note_on(61), false);
#if OUT_OF_RANGE_POLICY == OUT_OF_RANGE_IGNORE
    expect_bool("oor-47", ch9329_keyboard_note_on(47), false);
#endif
    expect_bool("oor-200", ch9329_keyboard_note_on(200), false);

    // --- スメール音階: 低音 Eb(note51) → C(0x06) ---
    ch9329_keyboard_init();
    ch9329_keyboard_set_sumeru_mode(true);
    expect_bool("sumeru-on51", ch9329_keyboard_note_on(51), true);
    expect_key("sumeru-on51", 0x06, true);

    // --- release_all ---
    ch9329_keyboard_init();
    ch9329_keyboard_note_on(60);
    ch9329_keyboard_note_on(83);
    expect_bool("relall-1", ch9329_keyboard_release_all(), true);
    expect_empty("relall-clear");
    expect_bool("relall-2", ch9329_keyboard_release_all(), false);
    expect_bool("relall-reon", ch9329_keyboard_note_on(60), true);

#if OUT_OF_RANGE_POLICY == OUT_OF_RANGE_WRAP
    // --- エイリアス: note24 と note36 は同じ Z(0x1D) に丸まる ---
    ch9329_keyboard_init();
    expect_bool("alias-on24", ch9329_keyboard_note_on(24), true);
    expect_bool("alias-on36", ch9329_keyboard_note_on(36), false);
    expect_key("alias-set", 0x1D, true);
    expect_bool("alias-off24", ch9329_keyboard_note_off(24), false);
    expect_key("alias-hold", 0x1D, true);
    expect_bool("alias-off36", ch9329_keyboard_note_off(36), true);
    expect_empty("alias-clear");
#endif

    if (failures == 0) {
        printf("ALL PASS\n");
        return 0;
    }
    printf("%d FAILURE(S)\n", failures);
    return 1;
}
