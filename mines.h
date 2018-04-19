#ifndef __MINES_H__
#define __MINES_H
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
};

struct opt {
	struct minescheme* scheme;
	int mode; /* allow flags? quesm? */
};

struct line_col {
	int l;
	int c;
};

void fill_minefield (int, int);
void move_ph (int, int);
void move_hi (int, int);
void to_next_boundary (int l, int c, char direction);
int getch (unsigned char*);
int getctrlseq (unsigned char*);
int everything_opened (void);
int wait_mouse_up (int, int);
void partial_show_minefield (int, int, int);
void show_minefield (int);
int get_neighbours (int, int, int);
int uncover_square (int, int);
void flag_square (int, int);
void quesm_square (int, int);
int choord_square (int, int);
int do_uncover (int*);
struct minecell** alloc_array (int, int);
void free_field (void);
char* get_emoticon(void);
int screen2field_l (int);
int screen2field_c (int);
int field2screen_c (int);
int clicked_emoticon (unsigned char*);
void quit(void);
int parse_fieldspec(char*);
void signal_handler (int signum);
void signal_setup (void);
void timer_setup (int);
void raw_mode(int mode);

enum modes {
	NORMAL,
	REDUCED,
	SHOWMINES,
	HIGHLIGHT,
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
	GAME_INPROGRESS,
	GAME_NEW,
	GAME_WON,
	GAME_LOST,
};
enum space_modes {
	MODE_OPEN,
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
};
enum mine_types {
	NO_MINE,
	STD_MINE,
	DEATH_MINE,
};
#endif
