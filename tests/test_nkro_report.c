// nkro_report.c のホスト側ユニットテスト (デフォルト設定: OUT_OF_RANGE_POLICY=ignore, offset0)。
// エイリアス/参照カウントのテストは OUT_OF_RANGE_POLICY=WRAP ビルド限定
// (`make t_nkro_report_wrap`。詳細は tests/Makefile)。
//   cc -Wall -Wextra -I../src test_nkro_report.c ../src/nkro_report.c ../src/note_mapper.c -o t && ./t
#include <stdio.h>
#include "nkro_report.h"
#include "note_mapper.h"
#include "config.h"

static int failures;

static void expect_byte(const char *name, int idx, uint8_t want) {
    uint8_t got = nkro_report_data()[idx];
    if (got != want) {
        printf("  FAIL %s: report[%d] want 0x%02X got 0x%02X\n", name, idx, want, got);
        failures++;
    }
}
static void expect_bool(const char *name, bool got, bool want) {
    if (got != want) {
        printf("  FAIL %s: want %s got %s\n", name, want ? "true" : "false", got ? "true" : "false");
        failures++;
    }
}
static void expect_zero(const char *name) {
    for (int i = 0; i < NKRO_REPORT_LEN; i++) {
        if (nkro_report_data()[i] != 0) {
            printf("  FAIL %s: report[%d]=0x%02X (expected all zero)\n", name, i, nkro_report_data()[i]);
            failures++;
            return;
        }
    }
}

int main(void) {
    // --- 単音: note60 → A(0x04) → byte1 bit0 ---
    nkro_report_init();
    expect_bool("on60", nkro_report_note_on(60), true);
    expect_byte("on60", 1, 0x01);
    expect_byte("on60-mod", 0, 0x00);
    expect_bool("off60", nkro_report_note_off(60), true);
    expect_zero("off60");

    // --- スメール音階: 低音 Eb(note51) → C キー ---
    nkro_report_init();
    nkro_report_set_sumeru_mode(true);
    expect_bool("sumeru-on51", nkro_report_note_on(51), true);
    expect_byte("sumeru-on51", 1, 0x04);

    // --- 高音 note83 → U(0x18) → bi20 → byte3 bit4 (0x10) ---
    nkro_report_init();
    nkro_report_note_on(83);
    expect_byte("on83", 3, 0x10);

    // --- 最上位ビット Z(0x1D) → bi25 → byte4 bit1 (0x02) : note48 ---
    nkro_report_init();
    nkro_report_note_on(48);
    expect_byte("on48-Z", 4, 0x02);

    // --- 和音 60,64,67 → A,D,G → byte1 0x01|0x08|0x40 = 0x49 ---
    nkro_report_init();
    nkro_report_note_on(60);
    nkro_report_note_on(64);
    nkro_report_note_on(67);
    expect_byte("chord", 1, 0x49);
    nkro_report_note_off(64); // D を離す → 0x49 & ~0x08 = 0x41
    expect_byte("chord-rel-D", 1, 0x41);

#if OUT_OF_RANGE_POLICY == OUT_OF_RANGE_WRAP
    // --- エイリアス: note24 と note36 は WRAP で同じキー (48=Z) に丸まる ---
    nkro_report_init();
    expect_bool("alias-on24", nkro_report_note_on(24), true);   // Z 押下
    expect_bool("alias-on36", nkro_report_note_on(36), false);  // 同じ Z、変化なし
    expect_byte("alias-set", 4, 0x02);
    expect_bool("alias-off24", nkro_report_note_off(24), false); // まだ 36 が保持
    expect_byte("alias-hold", 4, 0x02);
    expect_bool("alias-off36", nkro_report_note_off(36), true);  // ここで解放
    expect_zero("alias-clear");
#endif

    // --- べき等: 同じノートの二重 Note On ---
    nkro_report_init();
    expect_bool("idem-on1", nkro_report_note_on(72), true);
    expect_bool("idem-on2", nkro_report_note_on(72), false);
    expect_bool("idem-off1", nkro_report_note_off(72), true);
    expect_bool("idem-off2", nkro_report_note_off(72), false);

    // --- 対象外ノート: MIDI 上限超え (200) はどのポリシーでも対象外 ---
    nkro_report_init();
#if OUT_OF_RANGE_POLICY == OUT_OF_RANGE_IGNORE
    // 範囲外 47 は IGNORE のときのみ対象外
    // (WRAP だと 59=B に丸め込まれて対象になるため、この節は IGNORE 限定)
    expect_bool("oor-on47", nkro_report_note_on(47), false);
    expect_bool("oor-off47", nkro_report_note_off(47), false);
#endif
    expect_bool("oor-on200", nkro_report_note_on(200), false);
    expect_zero("oor");

    // --- release_all ---
    nkro_report_init();
    nkro_report_note_on(60);
    nkro_report_note_on(83);
    expect_bool("relall-1", nkro_report_release_all(), true);
    expect_zero("relall-clear");
    expect_bool("relall-2", nkro_report_release_all(), false); // 既にゼロ
    // 解放後は状態もリセットされ、同じノートを再度押下できる
    expect_bool("relall-reon", nkro_report_note_on(60), true);

    if (failures == 0) { printf("ALL PASS\n"); return 0; }
    printf("%d FAILURE(S)\n", failures);
    return 1;
}
