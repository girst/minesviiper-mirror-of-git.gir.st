/*
contains color/monchrome schemes for tty-mines. 
*/
#ifndef __SCHEMES_H__
#define __SCHEMES_H__

#define SGR(color, string) "\033[" color "m" string "\033[0m"
#define BOLD    "1"
#define BLINK   "5"
#define REV     "7"
#define RED    "31"
#define GREEN  "32"
#define YELLOW "33"
#define BLUE   "34"
#define CYAN   "36"
#define GREY   "37"
#define BRED   "91"
#define BBLUE  "94"
#define WHITE  "97"

enum e_emoticons {
	EMOT_SMILE,
	EMOT_DEAD,
	EMOT_WON,
	EMOT_OHH,
	NUM_EMOT,
};

enum e_border_lines {
	B_TOP,
	B_STATUS,
	B_DIVIDER,
	B_FIELD,
	B_BOTTOM,
};
enum e_border_cols {
	B_LEFT,
	B_MIDDLE,
	B_RIGHT,
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

	char* border[5][3];

	int cell_width;
	int flag_offset;
	char* init_seq;
	char* reset_seq;
};

struct minescheme symbols_mono = {
	.number = {"　", "１", "２", "３", "４", "５", "６", "７", "８"},
	.field_closed = "░░",
	.field_flagged = SGR(REV,"！"),
	.field_question = SGR(REV,"？"),
	.mouse_highlight = "▓▓",
	.mine_normal = "＊",
	.mine_death = "＃",
	.mine_wrongf = "／",
	.mine_wrongq = "＼",

	.emoticons = {":)", ":(", ":D", ":o"},

	.border = {{"╔═","══","═╗"},
	           {"║ ","  "," ║"},
	           {"╟─","──","─╢"},
	           {"║ ","  "," ║"},
	           {"╚═","══","═╝"}},

	.cell_width = 2,
	.flag_offset = 4, /* length of the escape sequece infront of .field_flagged and .field_question for cursor highlighting */
};

struct minescheme symbols_col1 = {
	.number = {"　",
	           SGR(BBLUE, "１"),
	           SGR(GREEN, "２"),
	           SGR(RED,   "３"),
	           SGR(BLUE,  "４"),
	           SGR(YELLOW,"５"),
	           SGR(CYAN,  "６"),
	           SGR(GREY,  "７"),
	           SGR(WHITE, "８")},
	.field_closed = "░░",
	.field_flagged = SGR(GREY,"▕\033["BRED"m▀"),
	.field_question = "？",
	.mouse_highlight = "▓▓",
	.mine_normal = "＊",
	.mine_death = SGR(RED,"＊"),
	.mine_wrongf = "／",
	.mine_wrongq = "＼",

	.emoticons = {":)", ":(", ":D", ":o"},

	.border = {{"╔═","══","═╗"},
	           {"║ ","  "," ║"},
	           {"╟─","──","─╢"},
	           {"║ ","  "," ║"},
	           {"╚═","══","═╝"}},

	.cell_width = 2,
};

//TODO: numbers aren't BOLD after {partial_,}show_minefield()
struct minescheme symbols_doublewidth = {
	/* vt220 multilingual character set,
	see http://vt100.net/docs/vt220-rm/table2-4.html */
	.number = {" ",
	           SGR(BOLD,"1"),
	           SGR(BOLD,"2"),
	           SGR(BOLD,"3"),
	           SGR(BOLD,"4"),
	           SGR(BOLD,"5"),
	           SGR(BOLD,"6"),
	           SGR(BOLD,"7"),
	           SGR(BOLD,"8")},
	.field_closed = "\x61",
	.field_flagged = SGR(BOLD,"!"),
	.field_question = SGR(BOLD,"?"),
	.mouse_highlight = SGR(BLINK,"@"),
	.mine_normal = SGR(BOLD,"*"),
	.mine_death = SGR(BOLD,"#"),
	.mine_wrongf = SGR(BOLD,"/"),
	.mine_wrongq = SGR(BOLD,"\\"),

	.emoticons = {":)", ":(", ":D", ":o"},

	.border = {{"\033#6\x6c","\x71","\x6b"},
	           {"\033#6\x78","    ","\x78"},
	           {"\033#6\x74","\x71","\x75"},
	           {"\033#6\x78","    ","\x78"},
	           {"\033#6\x6d","\x71","\x6a"}},

	.cell_width = 1,
	.init_seq = "\033(0"     /* enable DEC Special Graphics Character Set */
	            "\033[?3l",  /* switch to 80 column mode */
	.reset_seq = "\033(B"    /* enable DEC Multinational Character Set (TODO: check) */
	             "\033[?3h", /* switch to 132 column mode */
};

#undef SGR
#undef BOLD
#undef BLINK
#undef RED
#undef GREEN
#undef YELLOW
#undef BLUE
#undef CYAN
#undef GREY
#undef BRED
#undef BBLUE
#undef WHITE
#endif
