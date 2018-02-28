/*******************************************************************************
 minesviiper 0.3.145
 By Tobias Girstmair, 2015 - 2018

 ./minesviiper 16x16x40
 (see ./minesviiper -h for full list of options)

 MOUSE MODE: - left click to open and choord
             - right click to flag/unflag
 VI MODE:    - hjkl to move cursor left/down/up/right
             - bduw to jump left/down/up/right by 5 cells
             - o to open and choord
             - i to flag/unflag

 GNU GPL v3, see LICENSE or https://www.gnu.org/licenses/gpl-3.0.txt
*******************************************************************************/


#define _POSIX_C_SOURCE 2 /*for getopt, sigaction in c99*/
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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
#define printm(num, str) for (int i = 0; i < num; i++) fputs (str, stdout)
#define print(str) fputs (str, stdout)

struct minecell {
	unsigned m:2; /* mine?1:killmine?2:0 */
	unsigned o:1; /* open?1:0 */
	unsigned f:2; /* flagged?1:questioned?2:0 */
	unsigned n:4; /* 0<= neighbours <=8 */
};
struct minefield {
	struct minecell **c;
	//TODO: rename w, h to L, C
	int w; /* width */
	int h; /* height */
	int m; /* number of mines */

	int f; /* flags counter */
	int t; /* time of game start */
	int p[2]; /* cursor position {line, col} */
	int s; /* space mode */
} f;

struct opt {
	struct minescheme* scheme;
	int mode; /* allow flags? quesm? */
} op;

int alt_screen = 0;

struct line_col {
	int l;
	int c;
};

void fill_minefield (int, int);
void move (int, int);
void cursor_move (int, int);
void to_next_boundary (int l, int c, char direction);
int getch (unsigned char*);
int getctrlseq (unsigned char*);
int everything_opened ();
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
void free_field ();
int screen2field_l (int);
int screen2field_c (int);
int field2screen_l (int);
int field2screen_c (int);
void quit();
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
};
enum mine_types {
	NO_MINE,
	STD_MINE,
	DEATH_MINE,
};

void signal_handler (int signum) {
	switch (signum) {
	case SIGALRM:
		move (1, f.w*op.scheme->cell_width-(op.scheme->cell_width%2)-3);
		printf ("[%03d]", f.t?(int)difftime (time(NULL), f.t):0);
		break;
	case SIGINT:
		exit(128+SIGINT);
	}
}

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
	saved_term_mode = set_raw_term_mode();

	atexit (*quit);

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

	/* setup defaults */
	f.w = 30;
	f.h = 16;
	f.m = 99;
	f.c = NULL; /*to not free() array before it is allocated*/

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
			"    WxHxM (width 'x' height 'x' mines)\n"
			"    defaults to 30x16x99\n"
			"\n"
			"hjkl: move 1 left/down/up/right\n"
			"bduw: move 5 left/down/up/right\n"
			"^Gg$: move to the left/bottom/top/right\n"
			"left mouse/o: open/choord\n"
			"right mouse/i: flag/unflag\n"
			"space: modeful cursor (either open or flag)\n"
			"a: toggle mode for space (open/flag)\n"
			":D / r: start a new game\n"
			"q: quit\n", argv[0]);
			_exit(0);
		}
	}
	/* end parse options*/
	if (optind < argc) {
		sscanf (argv[optind], "%dx%dx%d", &(f.w), &(f.h), &(f.m));
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
	struct line_col markers[26];
	for (int i=26; i; markers[--i].l = -1);

	/* switch to alternate screen */
	printf ("\033[?47h");
	alt_screen = 1;
	/* reset cursor, clear screen */
	printf ("\033[H\033[J");

	/* swich charset, if necessary */
	if (op.scheme->init_seq != NULL) print (op.scheme->init_seq);

	show_minefield (NORMAL);

	/* enable mouse, hide cursor */
	printf ("\033[?1000h\033[?25l");

	while (1) {
		int l, c;
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
			f.p[0] = screen2field_l (mouse[2]);
			f.p[1] = screen2field_c (mouse[1]);
			/* :D clicked: TODO: won't work in single-width mode! */
			if (mouse[2] == LINE_OFFSET-1 && 
			   (mouse[1] == f.w+COL_OFFSET || 
			    mouse[1] == f.w+COL_OFFSET+1)) {
				free_field ();
				goto newgame;
			}
			if (f.p[1] < 0 || f.p[1] >= f.w || 
			    f.p[0] < 0 || f.p[0] >= f.h) break; /*out of bound*/
			/* fallthrough */
		case 'o':
			switch (do_uncover(&is_newgame)) {
				case GAME_LOST: goto lose;
				case GAME_WON:  goto win;
			}
			break;
		case CTRSEQ_MOUSE_RIGHT:
			f.p[0] = screen2field_l (mouse[2]);
			f.p[1] = screen2field_c (mouse[1]);
			if (f.p[1] < 0 || f.p[1] >= f.w || 
			    f.p[0] < 0 || f.p[0] >= f.h) break; /*out of bound*/
			/* fallthrough */
		case 'i': flag_square (f.p[0], f.p[1]); break;
		case '?':quesm_square (f.p[0], f.p[1]); break;
		case 'h': cursor_move (f.p[0],    f.p[1]-1 ); break;
		case 'j': cursor_move (f.p[0]+1,  f.p[1]   ); break;
		case 'k': cursor_move (f.p[0]-1,  f.p[1]   ); break;
		case 'l': cursor_move (f.p[0],    f.p[1]+1 ); break;
		case 'w': to_next_boundary (f.p[0], f.p[1], '>'); break;
		case 'b': to_next_boundary (f.p[0], f.p[1], '<'); break;
		case 'u': to_next_boundary (f.p[0], f.p[1], '^'); break;
		case 'd': to_next_boundary (f.p[0], f.p[1], 'v'); break;
		case '0': /* fallthrough */
		case '^': cursor_move (f.p[0],    0        ); break;
		case '$': cursor_move (f.p[0],    f.w-1    ); break;
		case 'g': cursor_move (0,         f.p[1]   ); break;
		case 'G': cursor_move (f.h-1,     f.p[1]   ); break;
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
			cursor_move (markers[action-'a'].l, markers[action-'a'].c);
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

win:
lose:
	timer_setup(0); /* stop timer */
	show_minefield (SHOWMINES);
	int gotaction;
	do {
		unsigned char mouse[3];
		gotaction = getch(mouse);
		/* :D clicked: TODO: won't work in single-width mode! */
		if (gotaction==CTRSEQ_MOUSE_LEFT && mouse[2]==LINE_OFFSET-1 &&
		   (mouse[1]==f.w+COL_OFFSET || mouse[1]==f.w+COL_OFFSET+1)) {
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

void quit () {
	move(0,0); //move(f.h+LINE_OFFSET+2, 0);
	/* disable mouse, show cursor */
	printf ("\033[?9l\033[?25h");
	/* reset charset, if necessary */
	if (op.scheme && op.scheme->reset_seq) print (op.scheme->reset_seq);
	/* revert to primary screen */
	if (alt_screen) printf ("\033[?47l");
	free_field ();
	restore_term_mode(saved_term_mode);
}

/* I haven't won as long as a cell exists, that
    - I haven't opened, and
    - is not a mine */
int everything_opened () {
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
	move (1, field2screen_c (f.w/2)-1); print (":o");

	if (!(l < 0 || l >= f.h || c < 0 || c >= f.w)) {
		/* show a pushed-in button if cursor is on minefield */
		move (l+LINE_OFFSET, field2screen_c(c));
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

	move (1, field2screen_c (f.w/2)-1); print (":D");
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
	if (f.c[l][c].o != CLOSED) return;
	/* cycles through flag/quesm/noflag (uses op.mode to detect which ones 
	are allowed) */
	f.c[l][c].f = (f.c[l][c].f + 1) % (op.mode + 1);
	if (f.c[l][c].f==FLAG) f.f++;
	else f.f--; //WARN: breaks on `-q'!
	partial_show_minefield (l, c, NORMAL);
	move (1, op.scheme->cell_width);
	printf ("[%03d]", f.f);
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

void move (int line, int col) {
	printf ("\033[%d;%dH", line+1, col+1);
}

/* absolute coordinates! */
void cursor_move (int l, int c) {
	partial_show_minefield (f.p[0], f.p[1], NORMAL);
	/* update f.p */
	f.p[0] = CLAMP(l, 0, f.h-1);
	f.p[1] = CLAMP(c, 0, f.w-1);
	move (f.p[0]+LINE_OFFSET, field2screen_c(f.p[1]));
	//fputs (op.scheme->mouse_highlight, stdout);

	if (!f.c[f.p[0]][f.p[1]].f) print("\033[7m");//invert unless ! or ?
	partial_show_minefield (f.p[0], f.p[1], HIGHLIGHT);
	print("\033[0m");//un-invert
}

/* to_next_boundary(): move into the supplied direction until a change in open-
state or flag-state is found and move there. falls back to BIG_MOVE. */
#define FIND_NEXT(X, L, C, L1, C1, MAX, OP) do {\
	new_ ## X OP ## = BIG_MOVE;\
	for (int i = X OP 1; i > 0 && i < f.MAX-1; i OP ## OP)\
		if ((f.c[L ][C ].o<<2 + f.c[L ][C ].f) \
		 != (f.c[L1][C1].o<<2 + f.c[L1][C1].f)) {\
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

	cursor_move (new_l, new_c);
}
#undef FIND_NEXT

char* cell2schema (int l, int c, int mode) {
	struct minecell cell = f.c[l][c];
	/* move past invert-ctrlsequence when highlighting the cursor: */
	int offset = ((mode==HIGHLIGHT)*op.scheme->flag_offset);

	if (mode == SHOWMINES) return (
		cell.f == FLAG &&  cell.m ? op.scheme->field_flagged+offset:
		cell.f == FLAG && !cell.m ? op.scheme->mine_wrongf+offset:
		cell.m == STD_MINE        ? op.scheme->mine_normal:
		cell.m == DEATH_MINE      ? op.scheme->mine_death:
		cell.o == CLOSED          ? op.scheme->field_closed:
		/*.......................*/ op.scheme->number[f.c[l][c].n]);
	 else return (
		cell.f == FLAG            ? op.scheme->field_flagged+offset:
		cell.f == QUESM           ? op.scheme->field_question+offset:
		cell.o == CLOSED          ? op.scheme->field_closed:
		cell.m == STD_MINE        ? op.scheme->mine_normal:
		cell.m == DEATH_MINE      ? op.scheme->mine_death:
		/*.......................*/ op.scheme->number[f.c[l][c].n]);
}

void partial_show_minefield (int l, int c, int mode) {
	move (l+LINE_OFFSET, field2screen_c(c));

	print (cell2schema(l, c, mode));
}

void show_minefield (int mode) {
	int dtime;
	static char modechar[] = {'*', '!', '?'};

	move (0,0);

	if (f.t == 0) {
		dtime = 0;
	} else {
		dtime = difftime (time(NULL), f.t);
	}

	/* first line */
	print (op.scheme->border_top_l);
	printm (f.w*op.scheme->cell_width,op.scheme->border_top_m);
	printf ("%s\r\n", op.scheme->border_top_r);
	/* second line */
	print (op.scheme->border_status_l);
	printf("[%03d]", f.f);
	printm (f.w*op.scheme->cell_width/2-6, " ");
	printf ("%s", mode==SHOWMINES?":C":":D");
	printm (f.w*op.scheme->cell_width/2-6-4, " ");
	printf ("[%c] [%03d]", modechar[f.s], dtime);
	print (op.scheme->border_status_r);
	print ("\r\n");
	/* third line */
	print (op.scheme->border_spacer_l);
	printm (f.w*op.scheme->cell_width,op.scheme->border_spacer_m);
	print (op.scheme->border_spacer_r);
	print ("\r\n");
	/* main body */
	for (int l = 0; l < f.h; l++) {
		print (op.scheme->border_field_l);
		for (int c = 0; c < f.w; c++) {
			print (cell2schema(l, c, mode));
		}
		print (op.scheme->border_field_r); print ("\r\n");
	}
	/* last line */
	print (op.scheme->border_bottom_l);
	printm (f.w*op.scheme->cell_width,op.scheme->border_bottom_m);
	print (op.scheme->border_bottom_r);
	print ("\r\n");
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

void free_field () {
	if (f.c == NULL) return;
	for (int l = 0; l < f.h; l++) {
		free (f.c[l]);
	}
	free (f.c);
}

int screen2field_l (int l) {
	return (l-LINE_OFFSET) - 1;
}
/* some trickery is required to extract the mouse position from the cell width, 
depending on wheather we are using full width characters or double line width. 
WARN: tested only with scheme.cell_width = 1 and scheme.cell_width = 2. */
int screen2field_c (int c) {
	return (c-COL_OFFSET+1 - 2*(op.scheme->cell_width%2))/2 - op.scheme->cell_width/2;
}
int field2screen_l (int l) {
	return 0; //TODO: is never used, therefore not implemented
}
int field2screen_c (int c) {
	return (op.scheme->cell_width*c+COL_OFFSET - (op.scheme->cell_width%2));
}

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
