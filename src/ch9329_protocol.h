#ifndef CH9329_PROTOCOL_H
#define CH9329_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// CH9329 シリアルプロトコルの純粋な組み立て処理 (UART 非依存・単体テスト可能)。
//
// フレーム:
//   0x57 0xAB | addr | command | length | data[length] | checksum
//   checksum = 全バイト (checksum 自身を除く) の単純和の下位 8 bit
//
// キーボードレポート (command 0x02, length 0x08):
//   data[0] = modifier (HID Usage 0xE0..0xE7 のビットマップ)
//   data[1] = reserved (0)
//   data[2..7] = 押下キーの HID Usage (最大 6 個、未使用は 0)
//
// CH9329 出荷時設定: 9600 bps / 8N1 / プロトコルモード / Mode 0。

#define CH9329_HEADER_0 0x57u
#define CH9329_HEADER_1 0xABu
#define CH9329_ADDR_DEFAULT 0x00u
#define CH9329_CMD_GET_INFO 0x01u
#define CH9329_CMD_SEND_KB 0x02u
#define CH9329_KB_DATA_LEN 0x08u
#define CH9329_KEY_COUNT 6u
#define CH9329_FRAME_MAX 16u
// header(2) + addr(1) + command(1) + length(1) + data(8) + checksum(1)
#define CH9329_KEYBOARD_FRAME_LEN 14u

// キーボードレポート 1 個分の CH9329 フレームを out へ書き、フレーム長を返す。
// out は少なくとも CH9329_FRAME_MAX バイト必要。
// keys は HID Usage の配列 (CH9329_KEY_COUNT 個)。0 は「空き」を表す。
uint8_t ch9329_build_keyboard_frame(uint8_t modifier,
                                    const uint8_t keys[CH9329_KEY_COUNT],
                                    uint8_t *out);

// 任意のコマンドフレームを組み立てる。data は NULL 可 (length=0)。
// 戻り値はフレーム長。out は 5 + length + 1 バイト必要。
uint8_t ch9329_build_frame(uint8_t addr, uint8_t command,
                           const uint8_t *data, uint8_t length,
                           uint8_t *out);

// GET_INFO (command 0x01) フレーム。応答のヘッダ検出でボーレートを
// 自動判定するために使う。out は CH9329_FRAME_MAX バイト必要。
uint8_t ch9329_build_get_info_frame(uint8_t addr, uint8_t *out);

// 受信バッファの先頭が完全な CH9329 フレームなら true。
// ヘッダ (57 AB)、長さ、チェックサムを検証する。
bool ch9329_frame_valid(const uint8_t *frame, size_t length);

// フレームのチェックサム (checksum 自身を含めない単純和) を返す。
uint8_t ch9329_checksum(const uint8_t *frame, size_t length);

#endif // CH9329_PROTOCOL_H
