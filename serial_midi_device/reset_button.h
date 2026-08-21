#ifndef RESET_BUTTON_H
#define RESET_BUTTON_H

#include <stdbool.h>

// MIDI RESETタクトスイッチの入力管理 (issue #6、ハード依存層)。
// デバウンス本体は src/debounced_switch.c を流用する (ボード1と共用、CLAUDE.md参照)。

void reset_button_init(void);

// 各メインループイテレーションで呼ぶこと (デバウンスの時間計測がこれを前提とする)。
// 押下が確定した瞬間だけ true を返す (one-shot)。離す (EXIT) は無視する。
bool reset_button_poll(void);

#endif // RESET_BUTTON_H
