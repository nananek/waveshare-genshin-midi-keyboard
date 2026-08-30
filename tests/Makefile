# ホスト側ロジックのユニットテスト (ARM ツールチェーン不要)。
#   make        … 全テストをビルドして実行
#   make clean
CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Wno-unused-function -O2 -I../src -I.
SRC      = ../src

TESTS = t_note_mapper t_midi_parse t_nkro_report t_midi_note_filter t_usb_power_fsm
# OUT_OF_RANGE_POLICY=WRAP でのみ到達する nkro_report のエイリアス/参照カウント経路を
# 検証する専用ビルド (既定ビルドの OUT_OF_RANGE_POLICY=IGNORE では黒鍵ドロップ後に
# alias 経路が到達不能になるため)。
TESTS_WRAP = t_nkro_report_wrap

.PHONY: all clean
all: run

t_note_mapper: test_note_mapper.c $(SRC)/note_mapper.c
	$(CC) $(CFLAGS) $^ -o $@

t_midi_parse: test_midi_parse.c $(SRC)/midi_parse.c
	$(CC) $(CFLAGS) $^ -o $@

t_nkro_report: test_nkro_report.c $(SRC)/nkro_report.c $(SRC)/note_mapper.c
	$(CC) $(CFLAGS) $^ -o $@

t_nkro_report_wrap: test_nkro_report.c $(SRC)/nkro_report.c $(SRC)/note_mapper.c
	$(CC) $(CFLAGS) -DOUT_OF_RANGE_POLICY=1 $^ -o $@

t_midi_note_filter: test_midi_note_filter.c $(SRC)/midi_note_filter.c $(SRC)/note_mapper.c
	$(CC) $(CFLAGS) $^ -o $@

t_usb_power_fsm: test_usb_power_fsm.c $(SRC)/usb_power_fsm.c
	$(CC) $(CFLAGS) $^ -o $@

run: $(TESTS) $(TESTS_WRAP)
	@rc=0; for t in $(TESTS) $(TESTS_WRAP); do echo "=== $$t ==="; ./$$t || rc=1; done; exit $$rc

clean:
	rm -f $(TESTS) $(TESTS_WRAP)
