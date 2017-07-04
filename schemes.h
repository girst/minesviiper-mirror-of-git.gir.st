/*
contains color/monchrome schemes for tty-mines. 
*/
#ifndef __SCHEMES_H__
#define __SCHEMES_H__

struct minescheme {
	char* number[9];
	char* field_closed;
	char* field_flagged;
	char* field_question;
	char* mouse_highlight;
	char* mine_normal;
	char* mine_death;
	char* mine_wrongf;
	char* mine_wrongq;

	char* border_top_l;
	char* border_top_m;
	char* border_top_r;

	char* border_status_l;
	//TODO: better define status line
	char* border_status_r;

	char* border_spacer_l;
	char* border_spacer_m;
	char* border_spacer_r;

	char* border_field_l;
	char* border_field_r;

	char* border_bottom_l;
	char* border_bottom_m;
	char* border_bottom_r;

	int cell_width;
	char* init_seq;
	char* reset_seq;
};

struct minescheme symbols_mono = {
	.number = {"　", "１", "２", "３", "４", "５", "６", "７", "８"},
	.field_closed = "░░",
	.field_flagged = "▒▒",
	.field_question = "？",
	.mouse_highlight = "▓▓",
	.mine_normal = "＊",
	.mine_death = "＃",
	.mine_wrongf = "／",
	.mine_wrongq = "＼",

	.border_top_l = "╔═",
	.border_top_m = "═",
	.border_top_r = "═╗",

	.border_status_l = "║ ",
	.border_status_r = " ║",

	.border_spacer_l = "╟─",
	.border_spacer_m = "─",
	.border_spacer_r = "─╢",

	.border_field_l = "║ ",
	.border_field_r = " ║",

	.border_bottom_l = "╚═",
	.border_bottom_m = "═",
	.border_bottom_r = "═╝",

	.cell_width = 2,
};

struct minescheme symbols_col1 = {
	.number = {"　", "\033[94m１\033[m", "\033[32m２\033[m", "\033[31m３\033[m", "\033[34m４\033[m", "\033[33m５\033[m", "\033[36m６\033[m", "\033[30m７\033[m", "\033[97m８\033[m"},
	.field_closed = "░░",
	.field_flagged = "▒▒",
	.field_question = "？",
	.mouse_highlight = "▓▓",
	.mine_normal = "＊",
	.mine_death = "\033[31m＊\033[m",
	.mine_wrongf = "／",
	.mine_wrongq = "＼",

	.border_top_l = "╔═",
	.border_top_m = "═",
	.border_top_r = "═╗",

	.border_status_l = "║ ",
	.border_status_r = " ║",

	.border_spacer_l = "╟─",
	.border_spacer_m = "─",
	.border_spacer_r = "─╢",

	.border_field_l = "║ ",
	.border_field_r = " ║",

	.border_bottom_l = "╚═",
	.border_bottom_m = "═",
	.border_bottom_r = "═╝",

	.cell_width = 2,
};

struct minescheme symbols_doublewidth = {
	/* vt220 multilingual character set, see http://vt100.net/docs/vt220-rm/table2-4.html */
	.number = {" ", "1", "2", "3", "4", "5", "6", "7", "8"},
	.field_closed = "\x61",
	.field_flagged = "F",
	.field_question = "?",
	.mouse_highlight = "@",
	.mine_normal = "*",
	.mine_death = "#",
	.mine_wrongf = "/",
	.mine_wrongq = "\\",

	.border_top_l = "\033#6\x6c",
	.border_top_m = "\x71",
	.border_top_r = "\x6b",

	.border_status_l = "\033#6\x78",
	.border_status_r = "\x78",

	.border_spacer_l = "\033#6\x74",
	.border_spacer_m = "\x71",
	.border_spacer_r = "\x75",

	.border_field_l = "\033#6\x78",
	.border_field_r = "\x78",

	.border_bottom_l = "\033#6\x6d",
	.border_bottom_m = "\x71",
	.border_bottom_r = "\x6a",

	.cell_width = 1,
	.init_seq = "\033(0", /* enable DEC Special Graphics Character Set */
	.reset_seq = "\033(B", /* enable DEC Multinational Character Set (TODO: check) */
};
#endif
