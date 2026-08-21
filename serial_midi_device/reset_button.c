#include "reset_button.h"
#include "../src/debounced_switch.h"
#include "config.h"

// ===========================================================================
//  MIDI RESETタクトスイッチ入力 (ハード依存層、issue #6)。
//  デバウンス本体は src/debounced_switch.c を共用する (ボード1の hid_mute /
//  mirror_filter_switch と同じ実装。二重保守を避けるため複製しない)。
//  CMakeLists.txt がソースとして ../src/debounced_switch.c を追加しているが、
//  serial_midi_device のinclude directoriesには src/ を足していない
//  (このファイルだけが相対パスでヘッダを直接参照するため、tusb_config.h /
//  config.h の解決には一切影響しない)。
//  無効時 (RESET_BUTTON_ENABLE=0) は no-op になりコンパイルアウトされる。
// ===========================================================================

#if RESET_BUTTON_ENABLE

static debounced_switch_t s_sw;

void reset_button_init(void) {
    debounced_switch_init(&s_sw, RESET_BUTTON_PIN, (bool)RESET_BUTTON_ACTIVE_LEVEL,
                           (uint32_t)RESET_BUTTON_DEBOUNCE_MS);
}

bool reset_button_poll(void) {
    return debounced_switch_poll(&s_sw) == DEBOUNCED_SWITCH_ENTER;
}

#else

void reset_button_init(void) {}
bool reset_button_poll(void) { return false; }

#endif
