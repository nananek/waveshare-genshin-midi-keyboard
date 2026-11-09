# ホスト側ロジックのユニットテスト (ARM ツールチェーン不要)。
#   make        … 全テストをビルドして実行
#   make clean
CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Wno-unused-function -O2 -I../src -I.
SRC      = ../src

TESTS = t_note_mapper t_midi_parse t_ch9329_protocol t_ch9329_keyboard
# OUT_OF_RANGE_POLICY=WRAP でのみ到達する CH9329 キーボードのエイリアス
# 参照カウント経路を検証する専用ビルド。
TESTS_WRAP = t_ch9329_keyboard_wrap

.PHONY: all clean
all: run

t_note_mapper: test_note_mapper.c $(SRC)/note_mapper.c
	$(CC) $(CFLAGS) $^ -o $@

t_midi_parse: test_midi_parse.c $(SRC)/midi_parse.c
	$(CC) $(CFLAGS) $^ -o $@

t_ch9329_protocol: test_ch9329_protocol.c $(SRC)/ch9329_protocol.c
	$(CC) $(CFLAGS) $^ -o $@

t_ch9329_keyboard: test_ch9329_keyboard.c $(SRC)/ch9329_keyboard.c $(SRC)/note_mapper.c
	$(CC) $(CFLAGS) $^ -o $@

t_ch9329_keyboard_wrap: test_ch9329_keyboard.c $(SRC)/ch9329_keyboard.c $(SRC)/note_mapper.c
	$(CC) $(CFLAGS) -DOUT_OF_RANGE_POLICY=1 $^ -o $@

run: $(TESTS) $(TESTS_WRAP)
	@rc=0; for t in $(TESTS) $(TESTS_WRAP); do echo "=== $$t ==="; ./$$t || rc=1; done; exit $$rc

clean:
	rm -f $(TESTS) $(TESTS_WRAP)
