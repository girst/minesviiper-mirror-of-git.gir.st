/*******************************************************************************
 minesviiper 0.5
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
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "mines.h"
#include "schemes.h"

#define LINE_OFFSET 3
#define LINES_AFTER 2
#define COL_OFFSET 2
#define BIG_MOVE 5
#define MOUSE_MAX 231
#define STOMP_TIMEOUT 150 /*ms*/

#define MIN(a,b) (a>b?b:a)
#define MAX(a,b) (a>b?a:b)
#define CLAMP(a,m,M) (a<m?m:(a>M?M:a))
#define printm(n, s) for (int _loop = 0; _loop < n; _loop++) fputs (s, stdout)
#define print(str) fputs (str?str:"", stdout)
#define EMOT(e) op.scheme->emoticons[EMOT_ ## e]
#define BORDER(l, c) op.scheme->border[B_ ## l][B_ ## c]
#define CW op.scheme->cell_width /* for brevity */
#define DW op.scheme->display_width /* ditto. */
#define HI_CELL f.c[g.p[0]][g.p[1]]
#define LINE_BELOW LINE_OFFSET+f.h-1+LINES_AFTER
#define CTRL_ 0x1F &

#define AR_CELL f.c[ROW][COL] /* helper for AROUND() */
#define AROUND(l, c) /* gives us loop-variables ROW, COL, AR_CELL */ \
	for (int ROW = MAX(l-1, 0); ROW <= MIN(l+1, f.h-1); ROW++) \
		for (int COL = MAX(c-1, 0); COL <= MIN(c+1, f.w-1); COL++) \
			if (!(ROW == l && COL == c)) /* skip itself */

#define PAUSE_TIMER \
	for (int _break=(timer_setup(0),1); _break; _break=0, timer_setup(g.t))

struct minefield f;
struct game g;
struct opt op;

int main (int argc, char** argv) {
	/* defaults: */
	f.w = 30;
	f.h = 16;
	f.m = 99;
	op.scheme = &symbols_mono;
	op.mode = FLAG;

	int optget;
	opterr = 0; /* don't print message on unrecognized option */
	while ((optget = getopt (argc, argv, "+hnfqbcd")) != -1) {
		switch (optget) {
		case 'n': op.mode    = NOFLAG; break;
		case 'f': op.mode    = FLAG; break; /*default*/
		case 'q': op.mode    = QUESM; break; /*WARN:implicitly sets -f*/
		case 'b': op.scheme  = &symbols_mono; break;
		case 'c': op.scheme  = &symbols_col1; break;
		case 'd': op.scheme  = &symbols_doublewidth; break;
		case 'h':
		default: 
			fprintf (stderr, SHORTHELP LONGHELP KEYHELP, argv[0]);
			return !(optget=='h');
		}
	} if (optind < argc) { /* parse Fieldspec */
		if (parse_fieldspec (argv[optind])) {
			fprintf (stderr, SHORTHELP FIELDHELP, argv[0]);
			return 1;
		}
	}

	signal_setup();
	atexit (*quit);

newgame:
	screen_setup(1);
	switch (g.o = minesviiper()) {
	case GAME_NEW: goto newgame;
	case GAME_WON: goto game_end;
	case GAME_LOST:goto game_end;
	case GAME_QUIT:goto quit;
	}

game_end:
	timer_setup(0); /* stop timer */
	int show_how = g.o==GAME_WON?SHOWFLAGS:SHOWMINES;
	show_minefield (show_how);
	for(;;) {
		switch(getch_wrapper()) {
		case WRAPPER_EMOTICON:
		case 'r': goto newgame;
		case 'q': goto quit;
		case CTRL_'L':
			/*WARN: updates the timer (would require fix in struct*/
			screen_setup(1);
			show_minefield (show_how);
			break;
		case CTRL_'R':
			interactive_resize();
			goto newgame;
		}
	}

quit:
	return 0;
}

int minesviiper(void) {
	f.c = alloc_array (f.h, f.w);
	g = (const struct game){0}; /* reset all game-specific parameters */

	show_minefield (NORMAL);

	for(;;) {
		int actor = KEYBOARD; /* WARN: feels like a hack :| */
		switch (getch_wrapper()) {
		case ' ':
			if (HI_CELL.o == OPENED) goto open_cell;
			if (g.s == MODE_OPEN)  goto open_cell;
			if (g.s == MODE_FLAG)  goto flag_cell;
			if (g.s == MODE_QUESM) goto quesm_cell;
			break;
		case 's':
			g.s = (g.s+1)%(op.mode+1);
			show_minefield (g.c?SHOWMINES:NORMAL);
			break;
		case CTRSEQ_MOUSE_LEFT:
			actor = MOUSE;
			/* fallthrough */
		open_cell:
		case 'o':
			switch (do_uncover(&(g.n), actor)) {
				case GAME_LOST: return GAME_LOST;
				case GAME_WON:  return GAME_WON;
			}
			break;
		flag_cell:
		case CTRSEQ_MOUSE_RIGHT:
		case 'i': flag_square (g.p[0], g.p[1]); break;
		quesm_cell:
		case '?':quesm_square (g.p[0], g.p[1]); break;
		case CTRSEQ_CURSOR_LEFT:
		case 'h': move_hi (g.p[0],   g.p[1]-1); break;
		case CTRSEQ_CURSOR_DOWN:
		case 'j': move_hi (g.p[0]+1, g.p[1]  ); break;
		case CTRSEQ_CURSOR_UP:
		case 'k': move_hi (g.p[0]-1, g.p[1]  ); break;
		case CTRSEQ_CURSOR_RIGHT:
		case 'l': move_hi (g.p[0],   g.p[1]+1); break;
		//TODO: W, B, U, D (bigword: only consider open/close state)
		case 'w': to_next_boundary (g.p[0], g.p[1], '>'); break;
		case 'b': to_next_boundary (g.p[0], g.p[1], '<'); break;
		case 'u': to_next_boundary (g.p[0], g.p[1], '^'); break;
		case 'd': to_next_boundary (g.p[0], g.p[1], 'v'); break;
		case 'e': to_next_boundary (g.p[0], g.p[1], '>'); break;
		case '0': /* fallthrough */
		case '^': move_hi (g.p[0], 0     ); break;
		case '$': move_hi (g.p[0], f.w-1 ); break;
		case 'g': move_hi (0,      g.p[1]); break;
		case 'G': move_hi (f.h-1,  g.p[1]); break;
		case 'z': move_hi (f.h/2,  f.w/2 ); break;
		case 'm': set_mark(); break;
		case'\'': /* fallthrough */
		case '`': jump_mark(); break;
		case 'f': find(getch_wrapper(), '>', 0); break;
		case 'F': find(getch_wrapper(), '<', 0); break;
		case 't': find(getch_wrapper(), '>',-1); break;
		case 'T': find(getch_wrapper(), '<',-1); break;
		case 'a': find(getch_wrapper(), '>',+1); break;
		case 'A': find(getch_wrapper(), '<',+1); break;
		//TODO: HJKL, ^H^J^K^L: <spacemode> up/down/left/right/45deg
		//TODO: ;, (repeat last find/till/after)
		case '!': grand_opening(&(g.n)); break;
		case WRAPPER_EMOTICON:
		case ':':
			switch (ex_cmd()) {
			case EX_QUIT: return GAME_QUIT;
			case EX_NEW:  timer_setup(0); return GAME_NEW;
			case EX_HELP: printf (KEYHELP); break; //TODO: charset problems with vt220
			case EX_RESZ:
				timer_setup(0);
				interactive_resize();
				return GAME_NEW;
			default:
				printf ("\rinvalid command");
				PAUSE_TIMER ungetc(getchar(), stdin);
				printf("\033[2K"); /* clear line */
			}
			break;
		case 'q': //TODO: redefine `q' as macro functionality
			break;
		case CTRL_'L':
			screen_setup(1);
			show_minefield (NORMAL);
			break;
		case '\\':
			if (g.n == GAME_NEW) break; /* must open a cell first */
			g.c = !g.c;
			show_minefield (g.c?SHOWMINES:NORMAL);
			break;
		}
	}

}

void quit (void) {
	screen_setup(0);
	free_field ();
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
		/* remove keyboard's cursor (if it's there): */
		redraw_cell (g.p[0], g.p[1], NORMAL);
		/* show a pushed-in button if cursor is on minefield */
		redraw_cell (l, c, HIGHLIGHT);

		show_stomp(1, l, c);
	}

	while (level > 0) {
		if (getctrlseq (mouse2) == CTRSEQ_MOUSE) {
			/* ignore mouse wheel events: */
			if (mouse2[0] & 0x40) continue;

			else if((mouse2[0]&3) == 3) level--; /* release event */
			else level++; /* another button pressed */
		}
	}

	move_ph (1, field2screen_c (f.w/2)-1); print (get_emoticon());

	if (!(l < 0 || l >= f.h || c < 0 || c >= f.w)) {
                redraw_cell (l, c, g.o?SHOWMINES:NORMAL);
		show_stomp(0, l, c);
	}
	c2 = screen2field_c(mouse2[1]);
	l2 = screen2field_l(mouse2[2]);
	return ((l2 == l) && (c2 == c));
}

int choord_square (int line, int col) {
	AROUND(line,col)
		if (AR_CELL.f != FLAG) {
			if (uncover_square (ROW, COL))
				 return 1;
		}

	return 0;
}

int uncover_square (int l, int c) {
	if (f.c[l][c].o == OPENED)
		return 0; /* nothing to do */

	f.c[l][c].o = OPENED;
	f.c[l][c].f = NOFLAG; /*must not be QUESM, otherwise rendering issues*/
	redraw_cell (l, c, NORMAL);

	if (f.c[l][c].m) {
		f.c[l][c].m = DEATH_MINE;
		return 1;
	}

	/* check for chording */
	if (f.c[l][c].n == 0) {
		AROUND(l, c)
			if (uncover_square (ROW, COL)) return 1;
	}

	return 0;
}

void flag_square (int l, int c) {
	static char modechar[] = {'*', '!', '?'};

	if (f.c[l][c].o != CLOSED) return;
	/* cycle through flag/quesm/noflag: */
	f.c[l][c].f = (f.c[l][c].f + 1) % (op.mode + 1);
	if (f.c[l][c].f==FLAG) g.f++;
	else if ((op.mode==FLAG  && f.c[l][c].f==NOFLAG) ||
	         (op.mode==QUESM && f.c[l][c].f==QUESM)) g.f--;
	redraw_cell (l, c, NORMAL);
	move_ph (1, op.scheme->cell_width);
	printf ("[%03d%c]", f.m - g.f, modechar[g.s]);
}

void quesm_square (int l, int c) {
	/* toggle question mark / none. won't turn flags into question marks.
	unlike flag_square, this function doesn't respect `-q'. */
	if      (f.c[l][c].o != CLOSED) return;
	else if (f.c[l][c].f == NOFLAG) f.c[l][c].f = QUESM;
	else if (f.c[l][c].f == QUESM)  f.c[l][c].f = NOFLAG;
	redraw_cell (l, c, NORMAL);
}

int do_uncover (int* is_newgame, int actor) {
	if (*is_newgame == GAME_NEW) {
		*is_newgame = GAME_INPROGRESS;
		fill_minefield (g.p[0], g.p[1]);
		timer_setup(1);
	}

#ifdef COORD_ON_FLAG
	if (HI_CELL.f == FLAG  ) {
		if (choord_square(g.p[0], g.p[1])) return GAME_LOST;
		else return GAME_INPROGRESS;
	}
#else
	if (HI_CELL.f == FLAG  ) return GAME_INPROGRESS;
#endif
	if (HI_CELL.o == CLOSED) {
		if (uncover_square (g.p[0], g.p[1])) return GAME_LOST;
	} else if (get_neighbours (g.p[0], g.p[1], 1) == 0) {
		if (choord_square (g.p[0], g.p[1])) return GAME_LOST;
	} else if (actor != MOUSE) {
		show_stomp(1, g.p[0], g.p[1]);

		wait_stomp();

		show_stomp(0, g.p[0], g.p[1]);
	}
	if (everything_opened()) return GAME_WON;

	return GAME_INPROGRESS;
}

#define for_each_cell(r, c) \
	for (int r = 0; r < f.h; r++) for (int c = 0; c < f.w; c++)
int grand_opening(int* is_newgame) {
	/* Find the largest cluster of zero-cells and open it. This is a bad
	implementation of a connected-component labelling algorithm and also
	uses (gasp!) VLAs! */

	if (*is_newgame == GAME_NEW) {
		*is_newgame = GAME_INPROGRESS;
		fill_minefield (-1, -1);
		timer_setup(1);
	} else return GAME_INPROGRESS; /* only allow this as the first move */

	int clusters[f.h*f.w]; /* each 0-cell gets labelled with a cluster */
	int cluster_id = 1;    /* number. connected cells get the same.    */
	int largest = 1;       /* the largest cluster will then be opened. */
	memset(clusters, 0, f.h*f.w * sizeof(int));

	/* assign cluster numbers: */
	for_each_cell(r, c) {
		if (assign_cluster(r, c, cluster_id, clusters))
			cluster_id++;
	}

	/* calculate cluster sizes: */
	int id_size[cluster_id];
	memset(id_size, 0, cluster_id * sizeof(int));
	for_each_cell(r, c)
		id_size[ clusters[r*f.w+c] ]++;

	/* find largest cluster and open it: */
	for (int i = 1; i < cluster_id; i++)
		if (id_size[i] > id_size[largest])
			largest = i;
	for_each_cell(r, c) {
		if (clusters[r*f.w+c] == largest) {
			g.p[0] = r;
			g.p[1] = c;
			return do_uncover(is_newgame, KEYBOARD);
		}
	}

	return GAME_INPROGRESS; /* that shouln't ever happen(TM) */
}
#undef for_each_cell
int assign_cluster(int r, int c, int id, int *clusters) {
	int tagged = 0;

	if (clusters[r*f.w+c] > 0    /* already tagged    */
	||  f.c[r][c].n       > 0   /* cell not empty    */
	||  f.c[r][c].m            /* cell has mine     */
	||  f.c[r][c].o         ) /* cell already open */
		return tagged;

	/* else, tag the cell and its neighbours */

	clusters[r*f.w+c] = id; tagged++;
	AROUND(r, c)
		tagged+= assign_cluster(ROW, COL, id, clusters);

	return tagged;
}

int ex_cmd(void) {
	char what[256];
	printf ("\033[?1000l\033[?25h"); /* disable mouse, show cursor */
	PAUSE_TIMER { /* timer spams ASCII STX (0x02) chars */
		move_ph(LINE_BELOW, 0);
		putchar(':'); /* prompt */

		raw_mode(0);/*use cooked mode, so we get line editing for free*/
		fgets(what, 256, stdin); //TODO: cancel on ^G, ESC, ^C, ...
		raw_mode(1);

		move_ph(LINE_BELOW, 0);
		printf ("\033[0J"); /* clear to the end of the screen */
	}
	printf ("\033[?1000h\033[?25l"); /* enable mouse, hide cursor */

	switch (what[0]) { //TODO: urgh.
	case 'q': return EX_QUIT;
	case 'n': return EX_NEW;
	case 'h': return EX_HELP;
	case 'r': return EX_RESZ;
	default:  return EX_INVALID;
	}
}
void set_mark(void) {
	int mark = tolower(getch_wrapper());
	if (mark < 'a' || mark > 'z') return; /*out of bound*/

	g.m[mark-'a'].l = g.p[0];
	g.m[mark-'a'].c = g.p[1];
	g.m[mark-'a'].s = 1;
}

void jump_mark(void) {
	int mark = tolower(getch_wrapper());
	/* check bounds and if set: */
	if (mark < 'a' || mark > 'z' || !g.m[mark-'a'].s) return;
	move_hi (g.m[mark-'a'].l, g.m[mark-'a'].c);
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

	redraw_cell (g.p[0], g.p[1], NORMAL);
	/* update g.p */
	g.p[0] = CLAMP(l, 0, f.h-1);
	g.p[1] = CLAMP(c, 0, f.w-1);
	move_ph (g.p[0]+LINE_OFFSET, field2screen_c(g.p[1]));

	redraw_cell (g.p[0], g.p[1], HIGHLIGHT);
}

/* to_next_boundary(): move into the supplied direction until a change in open-
state or flag-state is found and move there. prefer landing on a closed cell by
advancing once more when landing on an opened cell. falls back to BIG_MOVE. */
#define FIND_BOUND(X, L, C, L1, C1, MAX, OP) do {\
	new_ ## X OP ## = BIG_MOVE;\
	for (int i = X OP 1; i > 0 && i < f.MAX-1; i OP ## OP)\
		if (((f.c[L ][C ].o<<2) + f.c[L ][C ].f) \
		 != ((f.c[L1][C1].o<<2) + f.c[L1][C1].f)) {\
			new_ ## X = i OP !f.c[L1][C1].o;\
			break;\
		}\
	} while(0)
void to_next_boundary (int l, int c, char direction) {
	int new_l = l;
	int new_c = c;
	switch (direction) {
	case '>': FIND_BOUND(c, l, i, l, i+1, w, +); break;
	case '<': FIND_BOUND(c, l, i, l, i-1, w, -); break;
	case '^': FIND_BOUND(l, i, c, i-1, c, h, -); break;
	case 'v': FIND_BOUND(l, i, c, i+1, c, h, +); break;
	}

	move_hi (new_l, new_c);
}
#undef FIND_BOUND

#define CELL f.c[g.p[0]][c]
#define FIND_NEXT(CONDITION, DIRECTION) do {\
	int plusminus = DIRECTION-'='; /* -1 if '<', 1 if '>' */ \
	for (int c = g.p[1]+plusminus; c >= 0 && c < f.w; c+=plusminus) \
		if (CONDITION) { \
			move_hi (g.p[0], c+(till_after*plusminus)); \
			return 1; /* NOTE: break didn't work */ \
		} \
		return 0; \
	} while(0)
int find (int what, char direction, int till_after) {
	switch (what) {
	case ' ': what = '0'; /* fallthrough */
	case '0': case '1': case '2':
	case '3': case '4': case '5':
	case '6': case '7': case '8': /* numbers, opened */
	                    FIND_NEXT(CELL.o && CELL.n == what-'0', direction);
	case 'o':           FIND_NEXT(CELL.o, direction);       /* any opened */
	case 'c':           FIND_NEXT(!CELL.o, direction);      /* any closed */
	case 'f': case 'i': FIND_NEXT(CELL.f==FLAG, direction); /* is flagged */
	case 'q': case '?': FIND_NEXT(CELL.f==QUESM, direction);/* questioned */
	}

	return 0;
}
#undef FIND_NEXT
#undef CELL

char* cell2schema (int l, int c, int mode) {
	struct minecell cell = f.c[l][c];

	if (mode == SHOWMINES) return (
		cell.f == FLAG &&  cell.m ? op.scheme->field_flagged:
		cell.f == FLAG && !cell.m ? op.scheme->mine_wrongf:
		cell.m == STD_MINE        ? op.scheme->mine_normal:
		cell.m == DEATH_MINE      ? op.scheme->mine_death:
		cell.o == CLOSED          ? op.scheme->field_closed:
		/*.......................*/ op.scheme->number[f.c[l][c].n]);
	else if (mode == SHOWFLAGS) return (
		/* on win (everything opened, only correct flags left) */
		cell.m == STD_MINE        ? op.scheme->field_flagged:
		/*.......................*/ op.scheme->number[f.c[l][c].n]);
	else return (
		cell.f == FLAG            ? op.scheme->field_flagged:
		cell.f == QUESM           ? op.scheme->field_question:
		cell.o == CLOSED          ? op.scheme->field_closed:
		cell.m == STD_MINE        ? op.scheme->mine_normal:
		cell.m == DEATH_MINE      ? op.scheme->mine_death:
		/*.......................*/ op.scheme->number[f.c[l][c].n]);
}

void redraw_cell (int l, int c, int mode) {
	move_ph (l+LINE_OFFSET, field2screen_c(c));
	if (mode == HIGHLIGHT) printf ("\033[7m"); /* reverse video */
	print (cell2schema(l, c, mode));
	if (mode == HIGHLIGHT) printf ("\033[0m"); /* reset all */
}

char* get_emoticon(void) {
	return g.o==GAME_WON ? EMOT(WON):
	       g.o==GAME_LOST? EMOT(DEAD):
		EMOT(SMILE);
}

/* https://zserge.com/blog/c-for-loop-tricks.html */
#define print_line(which) \
	for (int _break = (printf("%s", BORDER(which,LEFT)), 1); _break; \
		_break = 0, printf("%s\r\n", BORDER(which,RIGHT)))
#define print_border(which, width) \
	print_line(which) printm (width, BORDER(which,MIDDLE))
void show_minefield (int mode) {
	int dtime = difftime (time(NULL), g.t)*!!g.t;
	int half_spaces = f.w*op.scheme->cell_width/2;
	int left_spaces = MAX(0,half_spaces-7-(f.m-g.f>999));
	int right_spaces = MAX(0,half_spaces-6-(dtime>999));
	static char modechar[] = {'*', '!', '?'};

	move_ph (0,0);

	print_border(TOP, f.w);
	print_line(STATUS) {
		printf("[%03d%c]%*s%s%*s[%03d]",
		  /* [ */ f.m - g.f, modechar[g.s], /* ] */
		  left_spaces,"", get_emoticon(), right_spaces,"",
		  /* [ */ dtime /* ] */);
	}
	print_border(DIVIDER, f.w);
	/* main body */
	for (int l = 0; l < f.h; l++) print_line(FIELD)
		printm (f.w, cell2schema(l, _loop, mode));
	print_border(BOTTOM, f.w);
}

void show_stomp (int enable, int l, int c) {
	if (enable) {
		if (f.c[l][c].o == OPENED && get_neighbours (l, c, 1) != 0) {
			/* show the stomp radius if we aren't fully flagged */
			AROUND(l, c)
				if (AR_CELL.o == CLOSED && AR_CELL.f != FLAG)
					redraw_cell (ROW, COL, HIGHLIGHT);
			fflush(stdout); /* won't display without */
		}
	} else {
                AROUND(l, c) redraw_cell (ROW, COL, g.o?SHOWMINES:NORMAL);
	}
}

void wait_stomp (void) {
	/* block SIGALRM, otherwise poll gets cancelled by the timer: */
	sigset_t sig;
	sigemptyset (&sig);
	sigaddset(&sig, SIGALRM);
	sigprocmask (SIG_BLOCK, &sig, NULL);

	/* wait for timout or keypress: */
	struct pollfd fds;
	fds.fd = 0; fds.events = POLLIN;
	poll(&fds, 1, STOMP_TIMEOUT);

	/* restore signal mask: */
	sigprocmask (SIG_UNBLOCK, &sig, NULL);
}

int get_neighbours (int line, int col, int reduced_mode) {
	/* counts mines surrounding a square
	   modes: 0=normal; 1=reduced */

	int count = 0;
	AROUND(line, col) {
		count += !!AR_CELL.m;
		count -= reduced_mode * AR_CELL.f==FLAG;
	}
	return count;
}

void interactive_resize(void) {
	unsigned char buf[3];
	int old_w = f.w;
	int old_h = f.h;
	for(;;) {
		screen_setup(1); /* clears the screen */
		show_minefield (RESIZEMODE);
		/* show the new field size in the corner: */
		move_ph(f.h+LINE_OFFSET-1,COL_OFFSET);
		printf("%d x %d", f.w, f.h);
		move_ph(LINE_BELOW, 0);
		printf("Resize Mode: Enter to set, Q to abort");
		//TODO: need escape seq. to print lowercase in `-d' mode!

		free_field(); /* must free before resizing! */
		switch (getctrlseq(buf)) {
		case CTRSEQ_CURSOR_LEFT: case 'h': f.w--; break;
		case CTRSEQ_CURSOR_DOWN: case 'j': f.h++; break;
		case CTRSEQ_CURSOR_UP:   case 'k': f.h--; break;
		case CTRSEQ_CURSOR_RIGHT:case 'l': f.w++; break;
		case 'w': f.w+=BIG_MOVE; break;
		case 'b': f.w-=BIG_MOVE; break;
		case 'u': f.h-=BIG_MOVE; break;
		case 'd': f.h+=BIG_MOVE; break;
		case 0xa: return; /* enter */
		case 'q': /* abort */
			f.w = old_w;
			f.h = old_h;
			screen_setup(1);
			return;
		}

		clamp_fieldsize();
		f.m = mines_percentage(f.w, f.h);
		f.c = alloc_array (f.h, f.w);
	}
}

struct minecell** alloc_array (int lines, int cols) {
	free_field (); /* NOTE: this feels like a hack :| */
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

int getch_wrapper (void) {
	unsigned char mouse[3];
	int c;
	/*skip over ASCII_STX=0x02 that gets sent when returning from SIGALRM:*/
	while ( (c = getch(mouse)) == 0x02);

	if (c == CTRSEQ_MOUSE_LEFT || c == CTRSEQ_MOUSE_RIGHT) {
		if (clicked_emoticon(mouse))
			return WRAPPER_EMOTICON;

		if (screen2field_c (mouse[1]) < 0
		 || screen2field_c (mouse[1]) >= f.w
		 || screen2field_l (mouse[2]) < 0
		 || screen2field_l (mouse[2]) >= f.h)
			return CTRSEQ_INVALID;

		g.p[0] = screen2field_l (mouse[2]);
		g.p[1] = screen2field_c (mouse[1]);

		return c; /* CTRSEQ_MOUSE_LEFT || CTRSEQ_MOUSE_RIGHT */
	}

	return c;
}

void clamp_fieldsize (void) {
	/* clamp field size to terminal size and mouse maximum: */
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	/* seven is the minimum width of the status bar */
	/* NOTE: min of status in DEC mode: 13 (WONTFIX) */
	if (f.w < 7) f.w = 7;
	if (f.h < 7) f.h = 7;

	if (COL_OFFSET + f.w*CW + COL_OFFSET > w.ws_col)
		f.w = (w.ws_col - COL_OFFSET - COL_OFFSET)/DW;
	if (LINE_OFFSET + f.h + LINES_AFTER > w.ws_row)
		f.h = w.ws_row - (LINE_OFFSET+LINES_AFTER);
	if (COL_OFFSET + f.w*CW > MOUSE_MAX)
		f.w = MOUSE_MAX/CW - COL_OFFSET;
	if (LINE_OFFSET + f.h > MOUSE_MAX)
		f.h = MOUSE_MAX - LINE_OFFSET;
}

int mines_percentage(int w, int h) {
	return w*h*(w*h<30*16?.15625:.20625);
}

int parse_fieldspec(char* str) {
	/* parses the FIELDSPEC (WxHxM); returns 1 on error */
	int n = sscanf (str, "%dx%dx%d", &f.w, &f.h, &f.m);

	clamp_fieldsize();

	if (n < 2) {
		return 1; /* error */
	} else if (n == 2) {
		f.m = mines_percentage(f.w, f.h);
	}

	f.m = MIN(f.m, (f.w-1) * (f.h-1)); /* limit mines */

	return 0;
}

void timer_setup (int enable) {
	static struct itimerval tbuf;
	tbuf.it_interval.tv_sec  = 1;
	tbuf.it_interval.tv_usec = 0;

	if (enable) {
		g.t = time(NULL);
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

void signal_setup (void) {
	struct sigaction saction;

	saction.sa_handler = signal_handler;
	sigemptyset(&saction.sa_mask);
	saction.sa_flags   = 0;
	if (sigaction(SIGALRM, &saction, NULL) < 0 ) {
		perror("SIGALRM");
		exit(1);
	}

	if (sigaction(SIGTSTP, &saction, NULL) < 0 ) {
		perror ("SIGTSTP");
		exit (1);
	}

	if (sigaction(SIGCONT, &saction, NULL) < 0 ) {
		perror ("SIGCONT");
		exit (1);
	}

	if (sigaction(SIGINT, &saction, NULL) < 0 ) {
		perror ("SIGINT");
		exit (1);
	}
}

void signal_handler (int signum) {
	int dtime;
	switch (signum) {
	case SIGALRM:
		dtime = difftime (time(NULL), g.t);
		move_ph (1, f.w*CW-(CW%2)-3-(dtime>999));
		printf ("[%03d]", g.t?dtime:0);
		break;
	case SIGTSTP: /* interactive stop (C-z) */
		screen_setup(0); fflush(stdout);
		signal(SIGTSTP, SIG_DFL); /* NOTE: assumes SysV semantics! */
		raise(SIGTSTP);
		break;
	case SIGCONT:
		/* NOTE: will leave the VT220 in special graphics charset */
		screen_setup(1);
		show_minefield (g.c?SHOWMINES:NORMAL);
		break;
	case SIGINT:
		exit(128+SIGINT);
	}
}

void screen_setup (int enable) {
	static int sent_init;
	if (enable) {
		raw_mode(1);
		printf ("\033[s\033[?47h"); /* save cursor, alternate screen */
		printf ("\033[H\033[J"); /* reset cursor, clear screen */
		printf ("\033[?1000h\033[?25l"); /* enable mouse, hide cursor */
		if (!sent_init++) /* swich charset, if necessary and only once*/
			print (op.scheme->init_seq);
	} else {
		print (op.scheme->reset_seq); /* reset charset, if necessary */
		printf ("\033[?1000l\033[?25h");/* disable mouse, show cursor */
		printf ("\033[?47l\033[u"); /* primary screen, restore cursor */
		raw_mode(0);
		sent_init = 0;
	}
}

/* http://users.csc.calpoly.edu/~phatalsk/357/lectures/code/sigalrm.c */
void raw_mode(int enable) {
	static struct termios saved_term_mode;
	struct termios raw_term_mode;

	if (enable) {
		if (saved_term_mode.c_lflag == 0)/*don't overwrite stored mode*/
			tcgetattr(STDIN_FILENO, &saved_term_mode);
		raw_term_mode = saved_term_mode;
		raw_term_mode.c_lflag &= ~(ICANON | ECHO);
		raw_term_mode.c_cc[VMIN] = 1 ;
		raw_term_mode.c_cc[VTIME] = 0;
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_term_mode);
	} else {
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_term_mode);
	}
}
