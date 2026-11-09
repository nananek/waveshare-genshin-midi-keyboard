#include "ch9329_keyboard.h"

#include <string.h>

#include "note_mapper.h"

static bool s_note_active[128];
static uint8_t s_slot_ref[NOTE_MAP_KEY_COUNT];
static uint8_t s_keys[CH9329_KEYBOARD_KEY_COUNT];
static bool s_sumeru_mode;

static bool keys_contains(uint8_t code) {
    for (uint8_t i = 0; i < CH9329_KEYBOARD_KEY_COUNT; i++) {
        if (s_keys[i] == code) {
            return true;
        }
    }
    return false;
}

static bool keys_add(uint8_t code) {
    if (code == 0 || keys_contains(code)) {
        return false;
    }
    for (uint8_t i = 0; i < CH9329_KEYBOARD_KEY_COUNT; i++) {
        if (s_keys[i] == 0) {
            s_keys[i] = code;
            return true;
        }
    }
    return false; // 6 キー満杯
}

static bool keys_remove(uint8_t code) {
    for (uint8_t i = 0; i < CH9329_KEYBOARD_KEY_COUNT; i++) {
        if (s_keys[i] == code) {
            // 詰めて、未使用スロットを常に末尾の 0 に保つ。
            for (uint8_t j = i; j + 1u < CH9329_KEYBOARD_KEY_COUNT; j++) {
                s_keys[j] = s_keys[j + 1u];
            }
            s_keys[CH9329_KEYBOARD_KEY_COUNT - 1u] = 0;
            return true;
        }
    }
    return false;
}

// 同じ HID コードを参照している別スロットが残っているか。
static bool code_still_referenced(uint8_t code) {
    for (uint8_t slot = 0; slot < NOTE_MAP_KEY_COUNT; slot++) {
        if (s_slot_ref[slot] > 0 && note_map_hid_codes[slot] == code) {
            return true;
        }
    }
    return false;
}

void ch9329_keyboard_init(void) {
    memset(s_note_active, 0, sizeof(s_note_active));
    memset(s_slot_ref, 0, sizeof(s_slot_ref));
    memset(s_keys, 0, sizeof(s_keys));
    s_sumeru_mode = false;
}

void ch9329_keyboard_set_sumeru_mode(bool enabled) {
    s_sumeru_mode = enabled;
}

bool ch9329_keyboard_note_on(uint8_t midi_note) {
    if (midi_note > 127 || s_note_active[midi_note]) {
        return false;
    }
    uint8_t slot = note_to_key_index_mode(midi_note, s_sumeru_mode);
    if (slot == NOTE_MAP_NONE) {
        return false;
    }
    uint8_t code = note_map_hid_codes[slot];

    // 新しいコードを追加できない（6 キー満杯）なら、このノートは落とす。
    // 押下状態も立てないので、後続の Note Off で誤って他キーを消さない。
    bool new_code = !keys_contains(code);
    if (new_code && !keys_add(code)) {
        return false;
    }

    s_note_active[midi_note] = true;
    s_slot_ref[slot]++;
    return new_code;
}

bool ch9329_keyboard_note_off(uint8_t midi_note) {
    if (midi_note > 127 || !s_note_active[midi_note]) {
        return false;
    }
    uint8_t slot = note_to_key_index_mode(midi_note, s_sumeru_mode);
    if (slot == NOTE_MAP_NONE) {
        // Note On を通したノートなので通常は到達しない。
        s_note_active[midi_note] = false;
        return false;
    }
    uint8_t code = note_map_hid_codes[slot];
    s_note_active[midi_note] = false;
    if (s_slot_ref[slot] > 0) {
        s_slot_ref[slot]--;
    }
    if (s_slot_ref[slot] == 0 && !code_still_referenced(code)) {
        return keys_remove(code);
    }
    return false;
}

bool ch9329_keyboard_release_all(void) {
    bool changed = false;
    for (uint8_t i = 0; i < CH9329_KEYBOARD_KEY_COUNT; i++) {
        if (s_keys[i] != 0) {
            s_keys[i] = 0;
            changed = true;
        }
    }
    memset(s_note_active, 0, sizeof(s_note_active));
    memset(s_slot_ref, 0, sizeof(s_slot_ref));
    return changed;
}

const uint8_t *ch9329_keyboard_keys(void) {
    return s_keys;
}

uint8_t ch9329_keyboard_modifier(void) {
    return 0;
}
