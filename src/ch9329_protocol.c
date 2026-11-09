#include "ch9329_protocol.h"

#include <string.h>

uint8_t ch9329_checksum(const uint8_t *frame, size_t length) {
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum = (uint8_t)(sum + frame[i]);
    }
    return sum;
}

uint8_t ch9329_build_frame(uint8_t addr, uint8_t command,
                           const uint8_t *data, uint8_t length,
                           uint8_t *out) {
    out[0] = CH9329_HEADER_0;
    out[1] = CH9329_HEADER_1;
    out[2] = addr;
    out[3] = command;
    out[4] = length;
    if (length > 0 && data != NULL) {
        memcpy(&out[5], data, length);
    }
    uint8_t frame_len = (uint8_t)(5u + length);
    out[frame_len] = ch9329_checksum(out, frame_len);
    return (uint8_t)(frame_len + 1u);
}

uint8_t ch9329_build_keyboard_frame(uint8_t modifier,
                                    const uint8_t keys[CH9329_KEY_COUNT],
                                    uint8_t *out) {
    uint8_t data[CH9329_KB_DATA_LEN] = {0};
    data[0] = modifier;
    data[1] = 0;
    for (uint8_t i = 0; i < CH9329_KEY_COUNT; i++) {
        data[2u + i] = keys[i];
    }
    return ch9329_build_frame(CH9329_ADDR_DEFAULT, CH9329_CMD_SEND_KB,
                              data, CH9329_KB_DATA_LEN, out);
}

uint8_t ch9329_build_get_info_frame(uint8_t addr, uint8_t *out) {
    return ch9329_build_frame(addr, CH9329_CMD_GET_INFO, NULL, 0, out);
}

bool ch9329_frame_valid(const uint8_t *frame, size_t length) {
    if (frame == NULL || length < 6) {
        return false;
    }
    if (frame[0] != CH9329_HEADER_0 || frame[1] != CH9329_HEADER_1) {
        return false;
    }
    uint8_t data_len = frame[4];
    size_t total = (size_t)5u + data_len + 1u;
    if (length < total) {
        return false;
    }
    return frame[total - 1u] == ch9329_checksum(frame, total - 1u);
}
