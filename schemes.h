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
	char* init_seq;
	char* reset_seq;
};

struct minescheme symbols_mono = {
	.number = {"　", "１", "２", "３", "４", "５", "６", "７", "８"},
	.field_closed = "░░",
	.field_flagged = "▕▀",
	.field_question = "？",
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

struct minescheme symbols_doublewidth = {
	/* for the vt220. 
	DEC Special Graphics Character Set:
	    http://vt100.net/docs/vt220-rm/table2-4.html
	Dynamically Redefinable Character Set:
	    https://vt100.net/docs/vt220-rm/chapter4.html#S4.16 */
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
	.field_flagged = SGR(BOLD,"\eO!"),
	.field_question = SGR(BOLD,"?"),
	.mouse_highlight = SGR(BLINK,"@"),
	.mine_normal = SGR(BOLD,"*"),
	.mine_death = SGR(BOLD,"#"),
	.mine_wrongf = SGR(BOLD,"/"),
	.mine_wrongq = SGR(BOLD,"\\"),

	.emoticons = {":)", ":(", ":D", "\033(B:o\033(0"},

	.border = {{"\033#6\x6c","\x71","\x6b"},
	           {"\033#6\x78","    ","\x78"},
	           {"\033[?25l\033#6\x74","\x71","\x75"},
	           {"\033#6\x78","    ","\x78"},
	           {"\033#6\x6d","\x71","\x6a"}},

	.cell_width = 1,
	.init_seq = "\eP0;1;0;4;1;1{P" /* config for DRCS "P": 7x10,erase-all */
	            "??~^^^^/??N????\e\\" /* flag at '!' resembling ▕▀ */
	            "\e(0\e+P\x0f" /* G0=Graphics, G3="P", lock charset to G0 */
	            "\033[?3l",  /* switch to 80 column mode */
	.reset_seq = "\033(B"    /* reset to DEC Multinational Character Set */
	             "\033[?3h", /* switch back to 132 column mode (TODO: shouldn't be hardcoded) */
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
