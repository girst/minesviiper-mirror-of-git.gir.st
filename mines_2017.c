/*******************************************************************************
 minesviiper 0.3.145926
 By Tobias Girstmair, 2015 - 2018

 ./minesviiper 16x16x40
 (see ./minesviiper -h for full list of options)

 MOUSE MODE: - left click to open and choord
             - right click to flag/unflag
 VI MODE:    - hjkl to move cursor left/down/up/right
             - o/space to open and choord
             - i to flag/unflag
             - (see `./minesviiper -h' for all keybindings)

 GNU GPL v3, see LICENSE or https://www.gnu.org/licenses/gpl-3.0.txt
*******************************************************************************/


#define _POSIX_C_SOURCE 2 /*for getopt, sigaction in c99*/
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "schemes.h"

#define LINE_OFFSET 3
#define COL_OFFSET 2
#define BIG_MOVE 5

#define MIN(a,b) (a>b?b:a)
#define MAX(a,b) (a>b?a:b)
#define CLAMP(a,m,M) (a<m?m:(a>M?M:a))
#define printm(n, s) for (int _loop = 0; _loop < n; _loop++) fputs (s, stdout)
#define print(str) fputs (str?str:"", stdout)
#define EMOT(e) op.scheme->emoticons[EMOT_ ## e]
#define BORDER(l, c) op.scheme->border[B_ ## l][B_ ## c]

struct minecell {
	unsigned m:2; /* mine?1:killmine?2:0 */
	unsigned o:1; /* open?1:0 */
	unsigned f:2; /* flagged?1:questioned?2:0 */
	unsigned n:4; /* 0<= neighbours <=8 */
};
struct minefield {
	struct minecell **c;
	int w; /* width */
	int h; /* height */
	int m; /* number of mines */

	int f; /* flags counter */
	int t; /* time of game start */
	int p[2]; /* cursor position {line, col} */
	int s; /* space mode */
	int o; /* mode */
} f;

struct opt {
	struct minescheme* scheme;
	int mode; /* allow flags? quesm? */
} op;

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
void signal_handler (int signum);
void timer_setup (int);

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

#define CW op.scheme->cell_width
void signal_handler (int signum) {
	int dtime;
	switch (signum) {
	case SIGALRM:
		dtime = difftime (time(NULL), f.t);
		move_ph (1, f.w*CW-(CW%2)-3-(dtime>999));
		printf ("[%03d]", f.t?dtime:0);
		break;
	case SIGINT:
		exit(128+SIGINT);
	}
}
#undef CW

/* http://users.csc.calpoly.edu/~phatalsk/357/lectures/code/sigalrm.c */
struct termios saved_term_mode;
struct termios set_raw_term_mode() {
	struct termios cur_term_mode, raw_term_mode;

	tcgetattr(STDIN_FILENO, &cur_term_mode);
	raw_term_mode = cur_term_mode;
	raw_term_mode.c_lflag &= ~(ICANON | ECHO);
	raw_term_mode.c_cc[VMIN] = 1 ;
	raw_term_mode.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_term_mode);

	return cur_term_mode;
}
void restore_term_mode(struct termios saved_term_mode) {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_term_mode);
}


int main (int argc, char** argv) {
	struct sigaction saction;

	saction.sa_handler = signal_handler;
	sigemptyset(&saction.sa_mask);
	saction.sa_flags   = 0;
	if (sigaction(SIGALRM, &saction, NULL) < 0 ) {
		perror("SIGALRM");
		exit(1);
	}

	if (sigaction(SIGINT, &saction, NULL) < 0 ) {
		perror ("SIGINT");
		exit (1);
	}
	/* end screen setup */

	op.scheme = &symbols_mono;
	op.mode = FLAG;
	/* end defaults */
	/* parse options */
	int optget;
	opterr = 0; /* don't print message on unrecognized option */
	while ((optget = getopt (argc, argv, "+hnfqcdx")) != -1) {
		switch (optget) {
		case 'n': op.mode    = NOFLAG; break;
		case 'f': op.mode    = FLAG; break; /*default*/
		case 'q': op.mode    = QUESM; break;
		case 'c': op.scheme  = &symbols_col1; break;
		case 'd': op.scheme  = &symbols_doublewidth; break;
		case 'h':
		default:
			fprintf (stderr, "%s [OPTIONS] [FIELDSPEC]\n"
			"OPTIONS:\n"
			"    -n(o flagging)\n"
			"    -f(lagging)\n"
			"    -q(uestion marks)\n"
			"    -c(olored symbols)\n"
			"    -d(ec charset symbols)\n"
			"FIELDSPEC:\n"
			"    WxH[xM] (width 'x' height 'x' mines)\n"
			"    defaults to 30x16x99; mines default to ~20%%\n"
			"\n"
			"Keybindings:\n"
			"    hjkl: move left/down/up/right\n"
			"    bduw: move to next boundary\n"
			"    ^Gg$: move to the left/bottom/top/right\n"
			"    z:    center cursor on minefield\n"
			"    o:    open/choord\n"
			"    i:    flag/unflag\n"
			"    space:modeful cursor (either open or flag)\n"
			"    a:    toggle mode for space (open/flag)\n"
			"    mX:   set a mark for letter X\n"
			"    `X:   move to mark X (aliased to ')\n"
			"    r:    start a new game\n"
			"    q:    quit\n", argv[0]);
			_exit(optget=='h'?0:1);
		}
	}
	/* end parse options*/
	if (optind < argc) { /* parse Fieldspec */
		int n = sscanf (argv[optind], "%dx%dx%d", &f.w, &f.h, &f.m);
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
#define CW op.scheme->cell_width
#define WW w.ws_col /*window width  */
#define WH w.ws_row /*window height */
#define FW f.w      /* field width  */
#define FH f.h      /* field height */
#define MM 231      /* mouse maximum*/
#define LB 2        /*  left border */
#define RB 2        /* right border */
#define TB 3        /*   top border */
#define BB 2        /*bottom border */

		if (LB + FW*CW + RB > WW) FW = WW/CW - (LB+RB);
		if (TB + FH    + BB > WH) FH = WH    - (TB+BB);
		if (LB + FW*CW      > MM) FW = MM/CW - LB;
		if (TB + FH         > MM) FH = MM    - TB;

		if (n < 2) {
			fprintf (stderr, "FIELDSPEC: WxH[xM]"
			" (width 'x' height 'x' mines)\n");
			return 1;
		} else if (n == 2) {
			if (f.w < 30) f.m = f.w*f.h*.15625;
			else f.m = f.w*f.h*.20625;
		}
#undef CW
#undef WW
#undef WH
#undef FW
#undef FH
#undef MM
#undef LB
#undef RB
#undef TB
#undef BB
	} else { /* use defaults */
		f.w = 30;
		f.h = 16;
		f.m = 99;
	}
	/* check boundaries */
	if (f.m > (f.w-1) * (f.h-1)) {
		f.m = (f.w-1) * (f.h-1);
		fprintf (stderr, "too many mines. reduced to %d.\r\n", f.m);
	}
	/* end check */

newgame:
	f.c = alloc_array (f.h, f.w);

	f.f = 0;
	f.t = 0;
	f.p[0] = 0;
	f.p[1] = 0;

	int is_newgame = 1;
	int cheatmode = 0;
	f.s = MODE_OPEN;
	f.o = GAME_NEW;
	struct line_col markers[26];
	for (int i=26; i; markers[--i].l = -1);

	/* setup the screen: */
	saved_term_mode = set_raw_term_mode();
	atexit (*quit);
	/* switch to alternate screen */
	printf ("\033[?47h");
	/* reset cursor, clear screen */
	printf ("\033[H\033[J");

	/* swich charset, if necessary */
	if (op.scheme->init_seq != NULL) print (op.scheme->init_seq);

	show_minefield (NORMAL);

	/* enable mouse, hide cursor */
	printf ("\033[?1000h\033[?25l");

	while (1) {
		int action;
		unsigned char mouse[3];

		action = getch(mouse);
		switch (action) {
		case ' ':
			if (f.s == MODE_OPEN ||
			    f.c[f.p[0]][f.p[1]].o == OPENED) {
				switch (do_uncover(&is_newgame)) {
					case GAME_LOST: goto lose;
					case GAME_WON:  goto win;
				}
			} else if (f.s == MODE_FLAG) {
				flag_square (f.p[0], f.p[1]);
			} else if (f.s ==  MODE_QUESM) {
				quesm_square (f.p[0], f.p[1]);
			}
			break;
		case 'a':
			f.s = (f.s+1)%(op.mode+1);
			show_minefield (cheatmode?SHOWMINES:NORMAL);
			break;
		case CTRSEQ_MOUSE_LEFT:
			if (clicked_emoticon(mouse)) {
				free_field ();
				goto newgame;
			}
			if (screen2field_c (mouse[1]) < 0    ||
			    screen2field_c (mouse[1]) >= f.w ||
			    screen2field_l (mouse[2]) <  0   ||
			    screen2field_l (mouse[2]) >= f.h)   break;
			f.p[0] = screen2field_l (mouse[2]);
			f.p[1] = screen2field_c (mouse[1]);
			/* fallthrough */
		case 'o':
			switch (do_uncover(&is_newgame)) {
				case GAME_LOST: goto lose;
				case GAME_WON:  goto win;
			}
			break;
		case CTRSEQ_MOUSE_RIGHT:
			if (screen2field_c (mouse[1]) < 0    ||
			    screen2field_c (mouse[1]) >= f.w ||
			    screen2field_l (mouse[2]) <  0   ||
			    screen2field_l (mouse[2]) >= f.h)   break;
			f.p[0] = screen2field_l (mouse[2]);
			f.p[1] = screen2field_c (mouse[1]);
			/* fallthrough */
		case 'i': flag_square (f.p[0], f.p[1]); break;
		case '?':quesm_square (f.p[0], f.p[1]); break;
		case CTRSEQ_CURSOR_LEFT:
		case 'h': move_hi (f.p[0],   f.p[1]-1 ); break;
		case CTRSEQ_CURSOR_DOWN:
		case 'j': move_hi (f.p[0]+1, f.p[1]   ); break;
		case CTRSEQ_CURSOR_UP:
		case 'k': move_hi (f.p[0]-1, f.p[1]   ); break;
		case CTRSEQ_CURSOR_RIGHT:
		case 'l': move_hi (f.p[0],   f.p[1]+1 ); break;
		case 'w': to_next_boundary (f.p[0], f.p[1], '>'); break;
		case 'b': to_next_boundary (f.p[0], f.p[1], '<'); break;
		case 'u': to_next_boundary (f.p[0], f.p[1], '^'); break;
		case 'd': to_next_boundary (f.p[0], f.p[1], 'v'); break;
		case '0': /* fallthrough */
		case '^': move_hi (f.p[0],    0        ); break;
		case '$': move_hi (f.p[0],    f.w-1    ); break;
		case 'g': move_hi (0,         f.p[1]   ); break;
		case 'G': move_hi (f.h-1,     f.p[1]   ); break;
		case 'z': move_hi (f.h/2, f.w/2); break;
		case 'm':
			action = tolower(getch(mouse));
			if (action < 'a' || action > 'z') break;/*out of bound*/
			markers[action-'a'].l = f.p[0];
			markers[action-'a'].c = f.p[1];
			break;
		case'\'': /* fallthrough */
		case '`':
			action = tolower(getch(mouse));
			if (action < 'a' || action > 'z' /* out of bound or */
			    || markers[action-'a'].l == -1) break; /* unset */
			move_hi (markers[action-'a'].l, markers[action-'a'].c);
			break;
		case 'r': /* start a new game */
			free_field ();
			goto newgame;
		case 'q':
			goto quit;
		case '\014': /* Ctrl-L -- redraw */
			show_minefield (NORMAL);
			break;
		case '\\':
			if (is_newgame) {
				is_newgame = 0;
				fill_minefield (-1, -1);
				timer_setup(1);
			}
			show_minefield (cheatmode?NORMAL:SHOWMINES);
			cheatmode = !cheatmode;
			break;
		}
	}

win:	f.o = GAME_WON;  goto endgame;
lose:	f.o = GAME_LOST; goto endgame;
endgame:
	timer_setup(0); /* stop timer */
	show_minefield (SHOWMINES);
	int gotaction;
	do {
		unsigned char mouse[3];
		gotaction = getch(mouse);
		if (gotaction==CTRSEQ_MOUSE_LEFT && clicked_emoticon(mouse)) {
			free_field ();
			goto newgame;
		} else if (gotaction == 'r') {
			free_field ();
			goto newgame;
		} else if (gotaction == 'q') {
			goto quit;
		}
	} while (1);

quit:
	return 0;
}

void quit (void) {
	move_ph(0,0);
	printf ("\033[?9l\033[?25h"); /* disable mouse, show cursor */
	print (op.scheme->reset_seq); /* reset charset, if necessary */
	printf ("\033[?47l");         /* revert to primary screen */
	free_field ();
	restore_term_mode(saved_term_mode);
}

/* I haven't won as long as a cell exists, that
    - I haven't opened, and
    - is not a mine */
int everything_opened (void) {
	for (int row = 0; row < f.h; row++)
		for (int col = 0; col < f.w; col++)
			if (f.c[row][col].o == CLOSED &&
			    f.c[row][col].m == NO_MINE  ) return 0;
	return 1;
}

int wait_mouse_up (int l, int c) {
	unsigned char mouse2[3];
	int level = 1;
	int l2, c2;

	/* show :o face */
	move_ph (1, field2screen_c (f.w/2)-1); print (EMOT(OHH));

	if (!(l < 0 || l >= f.h || c < 0 || c >= f.w)) {
		/* show a pushed-in button if cursor is on minefield */
		move_ph (l+LINE_OFFSET, field2screen_c(c));
		fputs (op.scheme->mouse_highlight, stdout);
	}

	while (level > 0) {
		if (getctrlseq (mouse2) == CTRSEQ_MOUSE) {
			/* ignore mouse wheel events: */
			if (mouse2[0] & 0x40) continue;

			else if (mouse2[0]&3 == 3) level--; /* release event */
			else level++; /* another button pressed */
		}
	}

	move_ph (1, field2screen_c (f.w/2)-1); print (get_emoticon());

	if (!(l < 0 || l >= f.h || c < 0 || c >= f.w)) {
		partial_show_minefield (l, c, NORMAL);
	}
	c2 = screen2field_c(mouse2[1]);
	l2 = screen2field_l(mouse2[2]);
	return ((l2 == l) && (c2 == c));
}

int choord_square (int line, int col) {
	for (int l = MAX(line-1, 0); l <= MIN(line+1, f.h-1); l++) {
		for (int c = MAX(col-1, 0); c <= MIN(col+1, f.w-1); c++) {
			if (f.c[l][c].f != FLAG) {
				if (uncover_square (l, c))
					 return 1;
			}
		}
	}

	return 0;
}

int uncover_square (int l, int c) {
	f.c[l][c].o = OPENED;
	f.c[l][c].f = NOFLAG; /*must not be QUESM, otherwise rendering issues*/
	partial_show_minefield (l, c, NORMAL);

	if (f.c[l][c].m) {
		f.c[l][c].m = DEATH_MINE;
		return 1;
	}

	/* check for chording */
	if (f.c[l][c].n == 0) {
		for (int choord_l = -1; choord_l <= 1; choord_l++) {
			for (int choord_c = -1; choord_c <= 1; choord_c++) {
				int newl = l + choord_l;
				int newc = c + choord_c;
				if (newl >= 0 && newl < f.h &&
				    newc >= 0 && newc < f.w &&
				    f.c[newl][newc].o == CLOSED &&
				    uncover_square (newl, newc)) {
					 return 1;
				}
			}
		}
	}

	return 0;
}

void flag_square (int l, int c) {
	static char modechar[] = {'*', '!', '?'};

	if (f.c[l][c].o != CLOSED) return;
	/* cycle through flag/quesm/noflag: */
	f.c[l][c].f = (f.c[l][c].f + 1) % (op.mode + 1);
	if (f.c[l][c].f==FLAG) f.f++;
	else if ((op.mode==FLAG  && f.c[l][c].f==NOFLAG) ||
	         (op.mode==QUESM && f.c[l][c].f==QUESM)) f.f--;
	partial_show_minefield (l, c, NORMAL);
	move_ph (1, op.scheme->cell_width);
	printf ("[%03d%c]", f.m - f.f, modechar[f.s]);
}

void quesm_square (int l, int c) {
	/* toggle question mark / none. won't turn flags into question marks.
	unlike flag_square, this function doesn't respect `-q'. */
	if      (f.c[l][c].o != CLOSED) return;
	else if (f.c[l][c].f == NOFLAG) f.c[l][c].f = QUESM;
	else if (f.c[l][c].f == QUESM)  f.c[l][c].f = NOFLAG;
	partial_show_minefield (l, c, NORMAL);
}

int do_uncover (int* is_newgame) {
	if (*is_newgame) {
		*is_newgame = 0;
		fill_minefield (f.p[0], f.p[1]);
		timer_setup(1);
	}

	if (f.c[f.p[0]][f.p[1]].f == FLAG  ) return GAME_INPROGRESS;
	if (f.c[f.p[0]][f.p[1]].o == CLOSED) {
		if (uncover_square (f.p[0], f.p[1])) return GAME_LOST;
	} else if (get_neighbours (f.p[0], f.p[1], 1) == 0) {
		if (choord_square (f.p[0], f.p[1])) return GAME_LOST;
	}
	if (everything_opened()) return GAME_WON;

	return GAME_INPROGRESS;
}

void fill_minefield (int l, int c) {
	srand (time(0));
	int mines_set = f.m;
	while (mines_set) {
		int line = rand() % f.h;
		int col = rand() % f.w;

		if (f.c[line][col].m) {
		/* skip if field already has a mine */
			continue;
		} else if ((line == l) && (col == c)) {
		/* don't put a mine on the already opened (first click) field */
			continue;
		} else {
			mines_set--;
			f.c[line][col].m = STD_MINE;
		}
	}

	/* precalculate neighbours */
	for (int l=0; l < f.h; l++)
		for (int c=0; c < f.w; c++)
			f.c[l][c].n = get_neighbours (l, c, NORMAL);
}

void move_ph (int line, int col) {
	/* move printhead to zero-indexed position */
	printf ("\033[%d;%dH", line+1, col+1);
}

void move_hi (int l, int c) {
	/* move cursor and highlight to absolute coordinates */

	partial_show_minefield (f.p[0], f.p[1], NORMAL);
	/* update f.p */
	f.p[0] = CLAMP(l, 0, f.h-1);
	f.p[1] = CLAMP(c, 0, f.w-1);
	move_ph (f.p[0]+LINE_OFFSET, field2screen_c(f.p[1]));

	print("\033[7m"); /* reverse video */
	partial_show_minefield (f.p[0], f.p[1], HIGHLIGHT);
	print("\033[0m"); /* un-invert */
}

/* to_next_boundary(): move into the supplied direction until a change in open-
state or flag-state is found and move there. falls back to BIG_MOVE. */
#define FIND_NEXT(X, L, C, L1, C1, MAX, OP) do {\
	new_ ## X OP ## = BIG_MOVE;\
	for (int i = X OP 1; i > 0 && i < f.MAX-1; i OP ## OP)\
		if (((f.c[L ][C ].o<<2) + f.c[L ][C ].f) \
		 != ((f.c[L1][C1].o<<2) + f.c[L1][C1].f)) {\
			new_ ## X = i OP 1;\
			break;\
		}\
	} while(0)
void to_next_boundary (int l, int c, char direction) {
	int new_l = l;
	int new_c = c;
	switch (direction) {
	case '>': FIND_NEXT(c, l, i, l, i+1, w, +); break;
	case '<': FIND_NEXT(c, l, i, l, i-1, w, -); break;
	case '^': FIND_NEXT(l, i, c, i-1, c, h, -); break;
	case 'v': FIND_NEXT(l, i, c, i+1, c, h, +); break;
	}

	move_hi (new_l, new_c);
}
#undef FIND_NEXT

char* cell2schema (int l, int c, int mode) {
	struct minecell cell = f.c[l][c];

	if (mode == SHOWMINES) return (
		cell.f == FLAG &&  cell.m ? op.scheme->field_flagged:
		cell.f == FLAG && !cell.m ? op.scheme->mine_wrongf:
		cell.m == STD_MINE        ? op.scheme->mine_normal:
		cell.m == DEATH_MINE      ? op.scheme->mine_death:
		cell.o == CLOSED          ? op.scheme->field_closed:
		/*.......................*/ op.scheme->number[f.c[l][c].n]);
	 else return (
		cell.f == FLAG            ? op.scheme->field_flagged:
		cell.f == QUESM           ? op.scheme->field_question:
		cell.o == CLOSED          ? op.scheme->field_closed:
		cell.m == STD_MINE        ? op.scheme->mine_normal:
		cell.m == DEATH_MINE      ? op.scheme->mine_death:
		/*.......................*/ op.scheme->number[f.c[l][c].n]);
}

void partial_show_minefield (int l, int c, int mode) {
	move_ph (l+LINE_OFFSET, field2screen_c(c));

	print (cell2schema(l, c, mode));
}

char* get_emoticon(void) {
	return f.o==GAME_WON ? EMOT(WON):
	       f.o==GAME_LOST? EMOT(DEAD):
		EMOT(SMILE);
}

/* https://zserge.com/blog/c-for-loop-tricks.html */
#define print_line(which) \
	for (int _break = (printf(BORDER(which,LEFT)), 1); _break; \
		_break = 0, printf("%s\r\n", BORDER(which,RIGHT)))
#define print_border(which, width) \
	print_line(which) printm (width, BORDER(which,MIDDLE))
void show_minefield (int mode) {
	int dtime = difftime (time(NULL), f.t)*!!f.t;
	int half_spaces = f.w*op.scheme->cell_width/2;
	int left_spaces = MAX(0,half_spaces-7-(f.m-f.f>999));
	int right_spaces = MAX(0,half_spaces-6-(dtime>999));
	static char modechar[] = {'*', '!', '?'};

	move_ph (0,0);

	print_border(TOP, f.w);
	print_line(STATUS) {
		printf("[%03d%c]%*s%s%*s[%03d]",
		  /* [ */ f.m - f.f, modechar[f.s], /* ] */
		  left_spaces,"", get_emoticon(), right_spaces,"",
		  /* [ */ dtime /* ] */);
	}
	print_border(DIVIDER, f.w);
	/* main body */
	for (int l = 0; l < f.h; l++) print_line(FIELD)
		printm (f.w, cell2schema(l, _loop, mode));
	print_border(BOTTOM, f.w);
}

int get_neighbours (int line, int col, int reduced_mode) {
	/* counts mines surrounding a square
	   modes: 0=normal; 1=reduced */

	int count = 0;

	for (int l = MAX(line-1, 0); l <= MIN(line+1, f.h-1); l++) {
		for (int c = MAX(col-1, 0); c <= MIN(col+1, f.w-1); c++) {
			if (!l && !c) continue;

			count += !!f.c[l][c].m;
			count -= reduced_mode * f.c[l][c].f==FLAG;
		}
	}
	return count;
}

struct minecell** alloc_array (int lines, int cols) {
	struct minecell** a = malloc (lines * sizeof(struct minecell*));
	if (a == NULL) return NULL;
	for (int l = 0; l < lines; l++) {
		a[l] = calloc (cols, sizeof(struct minecell));
		if (a[l] == NULL) goto unalloc;
	}

	return a;
unalloc:
	for (int l = 0; l < lines; l++)
		free (a[l]);
	return NULL;
}

void free_field (void) {
	if (f.c == NULL) return;
	for (int l = 0; l < f.h; l++) {
		free (f.c[l]);
	}

	free (f.c);
	f.c = NULL;
}

#define CW op.scheme->cell_width
int screen2field_l (int l) {
	return (l-LINE_OFFSET) - 1;
}
int screen2field_c (int c) {
	/* this depends on the cell width and only works when it is 1 or 2. */
	return (c-COL_OFFSET+1 - 2*(CW%2))/2 - CW/2;
}
int field2screen_c (int c) {
	return (CW*c+COL_OFFSET - (CW%2));
}
int clicked_emoticon (unsigned char* mouse) {
	return (mouse[2] == LINE_OFFSET-1 && (
		mouse[1] == f.w+COL_OFFSET ||
		mouse[1] == f.w+COL_OFFSET+1));
}
#undef CW

enum esc_states {
	START,
	ESC_SENT,
	CSI_SENT,
	MOUSE_EVENT,
};
int getctrlseq (unsigned char* buf) {
	int c;
	int state = START;
	int offset = 0x20; /* never sends control chars as data */
	while ((c = getchar()) != EOF) {
		switch (state) {
		case START:
			switch (c) {
			case '\033': state=ESC_SENT; break;
			default: return c;
			}
			break;
		case ESC_SENT:
			switch (c) {
			case '[': state=CSI_SENT; break;
			default: return CTRSEQ_INVALID;
			}
			break;
		case CSI_SENT:
			switch (c) {
			case 'A': return CTRSEQ_CURSOR_UP;
			case 'B': return CTRSEQ_CURSOR_DOWN;
			case 'C': return CTRSEQ_CURSOR_RIGHT;
			case 'D': return CTRSEQ_CURSOR_LEFT;
			case 'M': state=MOUSE_EVENT; break;
			default: return CTRSEQ_INVALID;
			}
			break;
		case MOUSE_EVENT:
			buf[0] = c - offset;
			buf[1] = getchar() - offset;
			buf[2] = getchar() - offset;
			return CTRSEQ_MOUSE;
		default:
			return CTRSEQ_INVALID;
		}
	}
	return 2;
}

int getch(unsigned char* buf) {
/* returns a character, EOF, or constant for an escape/control sequence - NOT
compatible with the ncurses implementation of same name */
	int action = getctrlseq(buf);
	int l, c;
	switch (action) {
	case CTRSEQ_MOUSE:
		l = screen2field_l (buf[2]);
		c = screen2field_c (buf[1]);

		if (buf[0] > 3) break; /* ignore all but left/middle/right/up */
		int success = wait_mouse_up(l, c);

		/* mouse moved while pressed: */
		if (!success) return CTRSEQ_INVALID;

		switch (buf[0]) {
		case 0: return CTRSEQ_MOUSE_LEFT;
		case 1: return CTRSEQ_MOUSE_MIDDLE;
		case 2: return CTRSEQ_MOUSE_RIGHT;
		}
	}

	return action;
}

void timer_setup (int enable) {
	static struct itimerval tbuf;
	tbuf.it_interval.tv_sec  = 1;
	tbuf.it_interval.tv_usec = 0;

	if (enable) {
		f.t = time(NULL);
		tbuf.it_value.tv_sec  = 1;
		tbuf.it_value.tv_usec = 0;
		if (setitimer(ITIMER_REAL, &tbuf, NULL) == -1) {
			perror("setitimer");
			exit(1);
		}
	} else {
		tbuf.it_value.tv_sec  = 0;
		tbuf.it_value.tv_usec = 0;
		if ( setitimer(ITIMER_REAL, &tbuf, NULL) == -1 ) {
			perror("setitimer");
			exit(1);
		}
	}

}
