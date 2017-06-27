.PHONY: all clean run
all: 2017mines

2017mines: mines_2017.c schemes.h
	gcc mines_2017.c -o 2017mines

run: 2017mines
	./2017mines

clean:
	rm -f 2017mines
