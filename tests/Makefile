# ホスト側ロジックのユニットテスト (ARM ツールチェーン不要)。
#   make        … 全テストをビルドして実行
#   make clean
CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Wno-unused-function -O2 -I../src -I.
SRC      = ../src

TESTS = t_note_mapper t_midi_parse t_nkro_report

.PHONY: all clean
all: run

t_note_mapper: test_note_mapper.c $(SRC)/note_mapper.c
	$(CC) $(CFLAGS) $^ -o $@

t_midi_parse: test_midi_parse.c $(SRC)/midi_parse.c
	$(CC) $(CFLAGS) $^ -o $@

t_nkro_report: test_nkro_report.c $(SRC)/nkro_report.c $(SRC)/note_mapper.c
	$(CC) $(CFLAGS) $^ -o $@

run: $(TESTS)
	@rc=0; for t in $(TESTS); do echo "=== $$t ==="; ./$$t || rc=1; done; exit $$rc

clean:
	rm -f $(TESTS)
