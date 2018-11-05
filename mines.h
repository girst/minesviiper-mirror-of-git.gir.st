#ifndef __MINES_H__
#define __MINES_H__

#define SHORTHELP "%s [OPTIONS] [FIELDSPEC]\n"
#define FIELDHELP "FIELDSPEC: WxH[xM] (width 'x' height 'x' mines)\n"
#define LONGHELP \
	"OPTIONS:\n" \
	"    -n(o flagging)\n" \
	"    -f(lagging)\n" \
	"    -q(uestion marks; implies -f)\n" \
	"    -b(land symbols)\n" \
	"    -c(olorful symbols)\n" \
	"    -d(ec charset symbols)\n" \
	"FIELDSPEC:\n" \
	"    WxH[xM] (width 'x' height 'x' mines)\n" \
	"    defaults to 30x16x99; mines default to ~20%%\n" \
	"\n"
#define KEYHELP \
	"Keybindings:\n" \
	"    hjkl :move left/down/up/right\n" \
	"    bduw :move to next boundary\n" \
	"    ^Gg$ :move to the left/bottom/top/right\n" \
	"    z    :center cursor on minefield\n" \
	"    o    :open/choord\n" \
	"    i    :flag/unflag\n" \
	"    space:modeful cursor (either open or flag)\n" \
	"    s    :toggle mode for space (open/flag)\n" \
	"    mX   :set a mark for letter X\n" \
	"    `X   :move to mark X (aliased to ')\n" \
	"    f/F x:find forward/backward [012345678 ocfq]\n" \
	"    t/T x:'til -\"-\n" \
	"    a/A x:after -\"-\n" \
	"    r    :start a new game\n" \
	"    :h   :help\n" \
	"    :r   :resize\n" \
	"    :q   :quit\n"

struct minefield {
	struct minecell {
		unsigned m:2; /* mine?1:killmine?2:0 */
		unsigned o:1; /* open?1:0 */
		unsigned f:2; /* flagged?1:questioned?2:0 */
		unsigned n:4; /* 0<= neighbours <=8 */
	} **c;
	int w; /* width */
	int h; /* height */
	int m; /* number of mines */
};

struct game {
	int f; /* flags counter */
	int t; /* time of game start */
	int p[2]; /* cursor position {line, col} */
	int s; /* space mode */
	int o; /* mode */
	int n; /* new game? */
	int c; /* cheat mode? */
	struct markers {
		int s; /* set? */
		int l; /* line */
		int c; /* col */
	} m[26]; /* a-z */
};

struct opt {
	struct minescheme* scheme;
	int mode; /* allow flags? quesm? */
};

int minesviiper(void);
void fill_minefield (int, int);
void move_ph (int, int);
void move_hi (int, int);
void to_next_boundary (int, int, char, int);
int find (int, char, int);
int getch (unsigned char*);
int getch_wrapper (void);
int getctrlseq (unsigned char*);
int everything_opened (void);
int wait_mouse_up (int, int);
void redraw_cell (int, int, int);
void show_minefield (int);
void show_stomp (int, int, int);
void wait_stomp (void);
int get_neighbours (int, int, int);
int uncover_square (int, int);
void flag_square (int, int);
void quesm_square (int, int);
int choord_square (int, int);
int do_uncover (int*, int);
int ex_cmd(void);
void set_mark(void);
void jump_mark(void);
void interactive_resize(void);
struct minecell** alloc_array (int, int);
void free_field (void);
char* get_emoticon(void);
int screen2field_l (int);
int screen2field_c (int);
int field2screen_c (int);
int clicked_emoticon (unsigned char*);
void quit(void);
void clamp_fieldsize (void);
int mines_percentage(int, int);
int parse_fieldspec(char*);
void signal_handler (int);
void signal_setup (void);
void timer_setup (int);
void screen_setup (int);
void raw_mode(int);

enum modes {
	NORMAL,
	REDUCED,
	SHOWMINES,
	SHOWFLAGS,
	HIGHLIGHT,
	RESIZEMODE,
};
enum flagtypes {
	NOFLAG,
	FLAG,
	QUESM,
};
enum fieldopenstates {
	CLOSED,
	OPENED,
};
enum game_states {
	GAME_NEW = 0, /* expected zero while resetting `g' */
	GAME_INPROGRESS,
	GAME_WON,
	GAME_LOST,
	GAME_QUIT,
};
enum space_modes {
	MODE_OPEN = 0, /* expected zero while resetting `g' */
	MODE_FLAG,
	MODE_QUESM,
};
enum event {
	/* for getctrlseq() */
	CTRSEQ_NULL    =  0,
	CTRSEQ_EOF     = -1,
	CTRSEQ_INVALID = -2,
	CTRSEQ_MOUSE   = -3,
	/* for getch() */
	CTRSEQ_MOUSE_LEFT   = -4,
	CTRSEQ_MOUSE_MIDDLE = -5,
	CTRSEQ_MOUSE_RIGHT  = -6,
	CTRSEQ_CURSOR_LEFT  = -7,
	CTRSEQ_CURSOR_DOWN  = -8,
	CTRSEQ_CURSOR_UP    = -9,
	CTRSEQ_CURSOR_RIGHT = -10,
	/* for getch_wrapper() */
	WRAPPER_EMOTICON    = -11,
};
enum actor {
	KEYBOARD,
	MOUSE,
};
enum colon {
	EX_INVALID,
	EX_QUIT,
	EX_HELP,
	EX_RESZ,
};
enum mine_types {
	NO_MINE,
	STD_MINE,
	DEATH_MINE,
};
#endif
