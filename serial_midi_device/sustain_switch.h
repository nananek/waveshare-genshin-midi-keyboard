#ifndef SUSTAIN_SWITCH_H
#define SUSTAIN_SWITCH_H

// サステインペダル (CC64) 用スライドスイッチの入力管理 (issue #8、ハード依存層)。
// ON/OFF状態が変化した瞬間だけエッジを返す。CC64の値(127/0)の送信は呼び出し側で行う。

typedef enum {
    SUSTAIN_SWITCH_NO_CHANGE = 0,
    SUSTAIN_SWITCH_ENTER, // OFF→ON (呼び出し側がCC64=127を送る)
    SUSTAIN_SWITCH_EXIT,  // ON→OFF (呼び出し側がCC64=0を送る)
} sustain_switch_edge_t;

void sustain_switch_init(void);

// 各メインループイテレーションで呼ぶこと (デバウンスの時間計測がこれを前提とする)。
sustain_switch_edge_t sustain_switch_poll(void);

#endif // SUSTAIN_SWITCH_H
