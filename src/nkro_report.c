#include "nkro_report.h"
#include "note_mapper.h"

// キービットマップがカバーする Usage 範囲 (a..z)。
#define KEYMAP_USAGE_MIN 0x04u // 'a'
#define KEYMAP_USAGE_MAX 0x1Du // 'z'

static uint8_t s_report[NKRO_REPORT_LEN];
static uint8_t s_note_active[128];                  // MIDI ノートごとの押下状態 (べき等用)
static uint8_t s_slot_refcount[NOTE_MAP_KEY_COUNT]; // ゲームキーごとの参照数 (エイリアス用)

static void set_bit_for_code(uint8_t code, bool on) {
    if (code < KEYMAP_USAGE_MIN || code > KEYMAP_USAGE_MAX) {
        return;
    }
    uint8_t bi = (uint8_t)(code - KEYMAP_USAGE_MIN); // 0..25
    uint8_t byte = (uint8_t)(1 + (bi >> 3));         // report[1..4]
    uint8_t mask = (uint8_t)(1u << (bi & 7u));
    if (on) {
        s_report[byte] |= mask;
    } else {
        s_report[byte] &= (uint8_t)~mask;
    }
}

void nkro_report_init(void) {
    for (int i = 0; i < NKRO_REPORT_LEN; i++) {
        s_report[i] = 0;
    }
    for (int i = 0; i < 128; i++) {
        s_note_active[i] = 0;
    }
    for (int i = 0; i < NOTE_MAP_KEY_COUNT; i++) {
        s_slot_refcount[i] = 0;
    }
}

bool nkro_report_note_on(uint8_t midi_note) {
    if (midi_note > 127) {
        return false;
    }
    if (s_note_active[midi_note]) {
        return false; // retrigger: 二重カウントしない
    }
    uint8_t slot = note_to_key_index(midi_note);
    if (slot == NOTE_MAP_NONE) {
        return false; // 出力対象外 (範囲外 / 黒鍵 IGNORE)
    }
    s_note_active[midi_note] = 1;
    bool changed = false;
    if (s_slot_refcount[slot] == 0) {
        set_bit_for_code(note_map_hid_codes[slot], true);
        changed = true;
    }
    s_slot_refcount[slot]++;
    return changed;
}

bool nkro_report_note_off(uint8_t midi_note) {
    if (midi_note > 127) {
        return false;
    }
    if (!s_note_active[midi_note]) {
        return false; // 押されていない (対象外ノートの Note Off も含む)
    }
    s_note_active[midi_note] = 0;
    uint8_t slot = note_to_key_index(midi_note);
    if (slot == NOTE_MAP_NONE) {
        return false; // note_active を立てた時点で slot!=NONE のはずなので到達しない
    }
    bool changed = false;
    if (s_slot_refcount[slot] > 0) {
        s_slot_refcount[slot]--;
        if (s_slot_refcount[slot] == 0) {
            set_bit_for_code(note_map_hid_codes[slot], false);
            changed = true;
        }
    }
    return changed;
}

bool nkro_report_release_all(void) {
    bool changed = false;
    for (int i = 1; i < NKRO_REPORT_LEN; i++) {
        if (s_report[i] != 0) {
            s_report[i] = 0;
            changed = true;
        }
    }
    for (int i = 0; i < 128; i++) {
        s_note_active[i] = 0;
    }
    for (int i = 0; i < NOTE_MAP_KEY_COUNT; i++) {
        s_slot_refcount[i] = 0;
    }
    return changed;
}

const uint8_t *nkro_report_data(void) {
    return s_report;
}
