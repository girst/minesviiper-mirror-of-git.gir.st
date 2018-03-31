/*
contains color/monchrome schemes for tty-mines. 
*/
#ifndef __SCHEMES_H__
#define __SCHEMES_H__

#define C(color, string) "\033[" #color "m" #string "\033[0m"

enum e_emoticons {
	EMOT_SMILE,
	EMOT_DEAD,
	EMOT_WON,
	EMOT_OHH,
	NUM_EMOT,
};

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

	char* emoticons[NUM_EMOT];

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
	int flag_offset;
	char* init_seq;
	char* reset_seq;
};

struct minescheme symbols_mono = {
	.number = {"　", "１", "２", "３", "４", "５", "６", "７", "８"},
	.field_closed = "░░",
	.field_flagged = "\033[7m！\033[0m",
	.field_question = "\033[7m？\033[0m",
	.mouse_highlight = "▓▓",
	.mine_normal = "＊",
	.mine_death = "＃",
	.mine_wrongf = "／",
	.mine_wrongq = "＼",

	.emoticons = {":)", ":(", ":D", ":o"},

	.border_top_l = "╔═",
	.border_top_m = "══",
	.border_top_r = "═╗",

	.border_status_l = "║ ",
	.border_status_r = " ║",

	.border_spacer_l = "╟─",
	.border_spacer_m = "──",
	.border_spacer_r = "─╢",

	.border_field_l = "║ ",
	.border_field_r = " ║",

	.border_bottom_l = "╚═",
	.border_bottom_m = "═",
	.border_bottom_r = "═╝",

	.cell_width = 2,
	.flag_offset = 4, /* length of the escape sequece infront of .field_flagged and .field_question for cursor highlighting */
};

struct minescheme symbols_col1 = {
	.number = {"　", C(94,１), C(32,２), C(31,３), C(34,４), C(33,５), C(36,６), C(30,７), C(97,８)},
	.field_closed = "░░",
	.field_flagged = "\033[37m▕\033[91m▀\033[m",
	.field_question = "？",
	.mouse_highlight = "▓▓",
	.mine_normal = "＊",
	.mine_death = C(31,＊),
	.mine_wrongf = "／",
	.mine_wrongq = "＼",

	.emoticons = {":)", ":(", ":D", ":o"},

	.border_top_l = "╔═",
	.border_top_m = "══",
	.border_top_r = "═╗",

	.border_status_l = "║ ",
	.border_status_r = " ║",

	.border_spacer_l = "╟─",
	.border_spacer_m = "──",
	.border_spacer_r = "─╢",

	.border_field_l = "║ ",
	.border_field_r = " ║",

	.border_bottom_l = "╚═",
	.border_bottom_m = "═",
	.border_bottom_r = "═╝",

	.cell_width = 2,
};

struct minescheme symbols_doublewidth = {
	/* vt220 multilingual character set,
	see http://vt100.net/docs/vt220-rm/table2-4.html */
	.number = {" ", C(1,1), C(1,2), C(1,3), C(1,4), C(1,5), C(1,6), C(1,7), C(1,8)},
	.field_closed = "\x61",
	.field_flagged = C(1,!),
	.field_question = C(1,?),
	.mouse_highlight = C(5,@),
	.mine_normal = C(1,*),
	.mine_death = C(1,#),
	.mine_wrongf = C(1,/),
	.mine_wrongq = C(1,\\),

	.emoticons = {":)", ":(", ":D", ":o"},

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

#undef C
#endif
