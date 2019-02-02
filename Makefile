.PHONY: all clean

CFLAGS := -Wall -Werror -Wextra -pedantic

all: mines

mines: mines.c mines.h schemes.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f mines

.PHONY: test_todo test_long test
test_todo:
	@echo -e '\033[7mTODOs:\033[0m'
	@grep -ni --color=always 'xxx\|todo\|[^:]\/\/' *.c *.h
	# grep -ni --color=always 'note\|warn' *.c *.h

test_long:
	@echo -e '\n\033[7m>80:\033[0m'
	@for myFILE in *.c *.h; do sed 's/\t/        /g' < $$myFILE|grep -En --color=always '.{81}'|sed "s/^/\x1B[35m$$myFILE\x1B[36m:/"; done

test: test_todo test_long
