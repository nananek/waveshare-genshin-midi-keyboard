// ch9329_protocol.c のホスト側ユニットテスト。
//   cc -I../src -I. test_ch9329_protocol.c ../src/ch9329_protocol.c -o t && ./t
#include <stdio.h>
#include <string.h>

#include "ch9329_protocol.h"

static int failures;

static void expect_u8(const char *name, uint8_t got, uint8_t want) {
    if (got != want) {
        printf("  FAIL %s: want 0x%02X got 0x%02X\n", name, want, got);
        failures++;
    }
}

int main(void) {
    // --- 空レポート ---
    uint8_t keys[CH9329_KEY_COUNT] = {0};
    uint8_t frame[CH9329_FRAME_MAX];
    uint8_t len = ch9329_build_keyboard_frame(0, keys, frame);
    expect_u8("empty-len", len, 14);
    expect_u8("empty-h0", frame[0], 0x57);
    expect_u8("empty-h1", frame[1], 0xAB);
    expect_u8("empty-addr", frame[2], 0x00);
    expect_u8("empty-cmd", frame[3], 0x02);
    expect_u8("empty-dlen", frame[4], 0x08);
    expect_u8("empty-mod", frame[5], 0x00);
    expect_u8("empty-rsv", frame[6], 0x00);
    for (int i = 0; i < 6; i++) {
        expect_u8("empty-key", frame[7 + i], 0x00);
    }
    uint8_t sum = 0;
    for (int i = 0; i < 13; i++) {
        sum = (uint8_t)(sum + frame[i]);
    }
    expect_u8("empty-sum", frame[13], sum);

    // --- 'A' (HID Usage 0x04) 押下 ---
    // CH9329 の資料にある例: 57 AB 00 02 08 00 00 04 00 00 00 00 00 10
    keys[0] = 0x04;
    len = ch9329_build_keyboard_frame(0, keys, frame);
    expect_u8("A-len", len, 14);
    const uint8_t press_a[] = {
        0x57, 0xAB, 0x00, 0x02, 0x08, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
    };
    for (int i = 0; i < 14; i++) {
        expect_u8("A-example", frame[i], press_a[i]);
    }
    expect_u8("A-key0", frame[7], 0x04);
    expect_u8("A-key1", frame[8], 0x00);

    // --- modifier の反映 ---
    keys[0] = 0;
    keys[1] = 0x05; // B
    len = ch9329_build_keyboard_frame(0x02, keys, frame);
    expect_u8("mod-len", len, 14);
    expect_u8("mod", frame[5], 0x02);
    expect_u8("mod-key1", frame[8], 0x05);

    // --- 任意コマンド ---
    uint8_t payload[3] = {0x11, 0x22, 0x33};
    len = ch9329_build_frame(0x00, 0x01, payload, 3, frame);
    expect_u8("custom-len", len, 9);
    expect_u8("custom-cmd", frame[3], 0x01);
    expect_u8("custom-len-field", frame[4], 0x03);
    expect_u8("custom-data", frame[5], 0x11);
    expect_u8("custom-sum", frame[8], ch9329_checksum(frame, 8));

    // --- checksum は 8 bit で折り返す ---
    uint8_t many[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    expect_u8("sum-wrap", ch9329_checksum(many, sizeof(many)), 0xFC);

    if (failures == 0) {
        printf("ALL PASS\n");
        return 0;
    }
    printf("%d FAILURE(S)\n", failures);
    return 1;
}
