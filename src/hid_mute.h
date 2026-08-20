#ifndef HID_MUTE_H
#define HID_MUTE_H

#include <stdbool.h>

// ミュートスイッチの入力管理 (core0 のメインループからのみ呼ぶこと)。
// ハード依存層 (GPIO 直接操作) なのでホスト単体テスト対象外。

// デバウンス確定後のミュート状態遷移。
//   HID_MUTE_ENTER : ミュート開始 (スイッチ ON)。このとき全キー解放+空レポート送信を行う
//   HID_MUTE_EXIT  : ミュート解除 (スイッチ OFF)
typedef enum {
    HID_MUTE_NO_CHANGE = 0,
    HID_MUTE_ENTER,
    HID_MUTE_EXIT,
} hid_mute_edge_t;

void hid_mute_init(void);

// 各メインループイテレーションで呼ぶ。内部でデバウンスを行い、
// 状態遷移が確定したら HID_MUTE_ENTER / HID_MUTE_EXIT を返す (それ以外は NO_CHANGE)。
hid_mute_edge_t hid_mute_poll(void);

// 現在のデバウンス済みミュート状態 (true = ミュート中)。
bool hid_mute_is_muted(void);

// core1 のミラーから読む。原神演奏中 (false) のときだけフィルターを有効にする。
bool hid_mute_should_filter_mirror(void);

#endif // HID_MUTE_H
