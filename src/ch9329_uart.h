#ifndef CH9329_UART_H
#define CH9329_UART_H

#include <stdint.h>
#include <stdbool.h>

#include "ch9329_protocol.h"

// CH9329 との UART 接続 (ハード依存層)。
// core0 のメインループからのみ呼ぶこと。送信はノンブロッキングで、
// 最新のキーボードレポートを優先する (古い中間状態は捨てる)。

// UART を初期化し、必要なら CH9329 のボーレートを自動判定する。
void ch9329_uart_init(void);

// 実際に使用しているボーレート。
uint32_t ch9329_uart_baud(void);

// 送信待ちのレポートを置き換える。UART が空いていれば次の task で送られる。
void ch9329_uart_queue_report(uint8_t modifier,
                              const uint8_t keys[CH9329_KEY_COUNT]);

// TX FIFO へ書ける分だけ書く。メインループから毎回呼ぶ。
void ch9329_uart_task(void);

// 送信中または送信待ちがある。
bool ch9329_uart_busy(void);

#endif // CH9329_UART_H
