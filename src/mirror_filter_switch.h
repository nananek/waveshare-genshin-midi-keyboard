#ifndef LYRE_SWITCH_H
#define LYRE_SWITCH_H

#include <stdbool.h>

// 古びたライアー (スメール音階) モードスイッチの入力管理。
// デバウンス済み状態は core0/core1 から参照する。

void mirror_filter_switch_init(void);
void mirror_filter_switch_poll(void);
bool mirror_filter_switch_is_enabled(void); // true = スメール音階

#endif // LYRE_SWITCH_H
