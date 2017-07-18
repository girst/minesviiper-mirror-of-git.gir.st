# minesVIiper

minesVIiper is a clone of Minesweeper, which runs in the terminal and can be
controlled by either `vi` style keybindings, or the mouse. 

Multiple character and color schemes are available, but more can be added
easily. 

Complete documentation and screenshots can be found on the
[**Homepage**](https://gir.st/mines.htm). 

## Keybindings and mouse

| Key         | Action                    |
| ----------- | ------------------------- |
| `h`/`b`/`^` | move 1/5/all to the left  |
| `j`/`d`/`G` | move 1/5/all down         |
| `k`/`u`/`g` | move 1/5/all up           |
| `l`/`w`/`$` | move 1/5/all to the right |
| `i`         | flag / unflag             |
| space       | open / chord              |
| `r`         | restart game              |
| Ctrl-L      | redraw screen             |
| `q`         | quit                      |

Use the left mouse button to open or chord a cell, and the right button to flag.

A new game can be started by clicking on the `:D` icon. 

## Command line arguments

| Arg.   | Description                      |
| ------ | -------------------------------  |
| `-h`   | quick help                       |
| `-w N` | set field width to N cells       |
| `-h N` | set field height to N cells      |
| `-m N` | set number of mines to N         |
| `-n`   | disable flagging                 |
| `-f`   | enable flagging (default)        |
| `-q`   | enable question marks            |
| `-c`   | switch to the colored scheme     |
| `-d`   | switch to the DEC charset scheme |

## Character Schemes

By default, minesVIiper comes with three schemes; black-and-white, color, and
DECTerm. The first two use unicode characters and therefore require a modern
software terminal emulator like GNOME Terminal, XTerm, or others.    
The DEC color scheme uses Digital, Inc.'s proprietary *Special Graphics Character
Set*, which is implemented on the VT220 and later models. (full support for this mode
in terminal emulation software is rare)

## Compiling and extending

To compile minesVIiper, just run `make`. The phony target `run` will compile and
execute the program with the default settings (monochrome scheme, 30x16x99
field). 

While the main source of the program (`mines_2017.c`) is pure ASCII, `schemes.h`
is UTF-8-encoded. Therefore, care must be taken when editing character schemes.

## License

This program is released under the terms of the GNU GPL version 3.    
&copy; 2015-2017 Tobias Girstmair
