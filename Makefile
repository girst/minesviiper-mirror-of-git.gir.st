.PHONY: all clean run test
all: 2017mines

2017mines: mines.c mines.h schemes.h
	gcc mines.c -o 2017mines

run: 2017mines
	./2017mines

clean:
	rm -f 2017mines

define TESTS
	echo -e '\033[7mTODOs:\033[0m'
	grep -ni --color=always 'xxx\|note\|warn\|todo\|[^:]\/\/' *.c *.h

	echo -e '\n\033[7m>80:\033[0m'
	for myFILE in *.c *.h
	do sed 's/\t/        /g' < $$myFILE|grep -En --color=always '.{81}'|sed "s/^/\x1B[35m$$myFILE:/"
	done

	echo -e '\n\033[7m-Wall:\033[0m'
	gcc mines.c -o 2017mines -Wall -Werror -Wextra -pedantic -fdiagnostics-color=always
endef
export TESTS
test:
	bash -c "$$TESTS" 2>&1| less --RAW-CONTROL-CHARS --chop-long-lines
