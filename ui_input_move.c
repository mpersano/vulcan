/* ui_input_move.c -- part of vulcan
 *
 * This program is copyright (C) 2006 Mauro Persano, and is free
 * software which is freely distributable under the terms of the
 * GNU public license, included as the file COPYING in this
 * distribution.  It is NOT public domain software, and any
 * redistribution not permitted by the GNU General Public License is
 * expressly forbidden without prior written permission from
 * the author.
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "panic.h"
#include "list.h"
#include "vector.h"
#include "matrix.h"
#include "quaternion.h"
#include "pick.h"
#include "game.h"
#include "move.h"
#include "font.h"
#include "font_render.h"
#include "ui.h"
#include "ui_util.h"
#include "ui_state.h"

extern int send_worker_request(void);
extern long msecs(void);

static void
update_timers(void)
{
	if (ui.computer_player_state != DONE_MATE_FOR_BLACK &&
	  ui.computer_player_state != DONE_MATE_FOR_WHITE) {
		long delta, now;

		now = msecs();
		delta = now - ui.ply_prev_msecs;

		if (ui.cur_side == BLACK_FLAG)
			ui.black_player_msecs += delta;
		else
			ui.white_player_msecs += delta;

		ui.ply_prev_msecs = now;
	}
}

static void
game_do_move(union move *move)
{
	static char last_move_buf[120];

	update_timers();

	sprintf(last_move_buf, move_as_string(&ui.board_state, move));

	do_move(&ui.board_state, move, &ui.last_undo_info);

	if (get_game_status(&ui.board_state) != IN_PLAY) {
		ui.computer_player_state =
		  ui.cur_side == BLACK_FLAG ?
		    DONE_MATE_FOR_BLACK : DONE_MATE_FOR_WHITE;

		strcat(last_move_buf, "++");
	} else {
		ui.cur_side ^= BLACK_FLAG;
		ui.computer_player_state = IDLE;

		if (is_in_check(&ui.board_state, ui.cur_side))
			strcat(last_move_buf, "+");
	}

	list_append(ui.move_history, strdup(last_move_buf));

	if (move->type == PIECE_MOVE) {
		if (ui.cur_state ==
		  (union ui_state *)&state_waiting_for_move) { /* HACK */
			ui.animation_move = *move;
			ui_set_state(STATE_ANIMATION);
		}
	}
}

static enum player_type
cur_player_type(void)
{
	return ui.players[ui.cur_side == BLACK_FLAG];
}

static void
on_idle(union ui_state *state)
{
	if (ui.computer_player_state != IDLE) {
		update_timers();
	} else {
		if (cur_player_type() == COMPUTER_PLAYER) {
			if (send_worker_request() != 0)
				panic("couldn't send worker request?");
			ui.computer_player_state = THINKING;
		} else {
			ui.computer_player_state =
			  WAITING_FOR_USER_INPUT;
		}

		ui.ply_prev_msecs = msecs();
	}
}

static void
mark_selected_square(struct selected_squares *selected,
  const struct position *pos, enum selection_type t)
{
	selected->selection[pos->level][pos->square] = t;
}

static void
mark_selected_attack_board(struct selected_squares *selected,
  const struct attack_board *pos, enum selection_type t)
{
	int i, j;
	struct position first_pos;
	unsigned char *p;

	get_attack_board_position(&first_pos, pos->main_board, pos->position);

	p = &selected->selection[first_pos.level][first_pos.square];

	for (i = 0; i < ATTACK_BOARD_SIZE; i++) {
		for (j = 0; j < ATTACK_BOARD_SIZE; j++)
			*p++ = t;

		p += BCOLS - ATTACK_BOARD_SIZE;
	}
}

static void
mark_selected_squares(struct selected_squares *selected,
  const struct board_state *state, 
  int side, union move *moves, int num_moves)
{
	union move *move, *end;
	const struct position *pos;
	unsigned char s;
	enum selection_type t;
	struct position closest;

	end = &moves[num_moves];

	for (move = moves; move != end; move++) {
		switch (move->type) {
			case PIECE_MOVE:
				mark_selected_square(selected,
				  &move->piece_move.from, SELECTED_SQUARE);

				pos = &move->piece_move.to;

				s = state->board[pos->level][pos->square];

				if (s == EMPTY || (s & BLACK_FLAG) == side)
					t = SELECTED_SQUARE;
				else
					t = ATTACKED_SQUARE;

				mark_selected_square(selected, pos, t);
				break;

			case ATTACK_BOARD_MOVE:
				mark_selected_attack_board(selected,
				  &move->attack_board_move.from,
				    SELECTED_SQUARE);

				/* mark clickable destination square */

				get_closest_main_board_square_for_attack_board(
				  &closest, &move->attack_board_move.to);

				mark_selected_square(selected, &closest,
				  ATTACKED_SQUARE);
				break;

			case WHITE_KINGSIDE_CASTLING:
				selected->selection[4][11*BCOLS + 5] =
				  SELECTED_SQUARE;
				selected->selection[4][11*BCOLS + 6] =
				  ATTACKED_SQUARE;
				break;

			case WHITE_QUEENSIDE_CASTLING:
				selected->selection[4][11*BCOLS + 5] =
				  SELECTED_SQUARE;
				selected->selection[4][11*BCOLS + 2] =
				  SELECTED_SQUARE;
				break;

			case BLACK_KINGSIDE_CASTLING:
				selected->selection[0][2*BCOLS + 5] =
				  SELECTED_SQUARE;
				selected->selection[0][2*BCOLS + 6] =
				  ATTACKED_SQUARE;
				break;

			case BLACK_QUEENSIDE_CASTLING:
				selected->selection[0][2*BCOLS + 5] =
				  SELECTED_SQUARE;
				selected->selection[0][2*BCOLS + 2] =
				  SELECTED_SQUARE;
				break;

			default:
				assert(0);
		}
	}
}

static void
reset_selected_squares(void)
{
	memset(&ui.selected_squares, UNSELECTED_SQUARE,
	  sizeof ui.selected_squares);
}

static void
on_start_position_selected(struct position *pos)
{
	union move moves[MAX_MOVES];
	int n;

	reset_selected_squares();

	if ((n = get_legal_moves_for_position(moves, pos, &ui.board_state,
	  ui.cur_side)) == 0) {
		ui.start_position_selected = 0;
	} else {
		mark_selected_squares(&ui.selected_squares,
		  &ui.board_state, ui.cur_side, moves, n);

		mark_selected_square(&ui.selected_squares, pos,
		  SELECTED_SQUARE);

		ui.last_selected_pos = *pos;
		ui.start_position_selected = 1;
		ui.pos_highlighted = 0;
	}
}

static void
on_end_position_selected(struct position *pos)
{
	union move moves[MAX_MOVES];
	union move *selected_move;
	struct position closest, *start;
	int i, n;

	start = &ui.last_selected_pos;

	assert((n = get_legal_moves_for_position(moves,
	  start, &ui.board_state, ui.cur_side)) > 0);

	selected_move = NULL;

	for (i = 0; i < n && selected_move == NULL; i++) {
		switch (moves[i].type) {
			case PIECE_MOVE:
				if (SAME_POS(&moves[i].piece_move.to, pos)) {
					selected_move = &moves[i];
					break;
				}
				break;

			case ATTACK_BOARD_MOVE:
				get_closest_main_board_square_for_attack_board(
				  &closest, &moves[i].attack_board_move.to);

				if (SAME_POS(&closest, pos)) {
					selected_move = &moves[i];
					break;
				}
				break;

			case WHITE_KINGSIDE_CASTLING:
				if (start->level == 4 &&
				  start->square == 11*BCOLS+5 &&
				  pos->level == 4 &&
				  pos->square == 11*BCOLS+6) {
					selected_move = &moves[i];
					break;
				}
				break;

			case WHITE_QUEENSIDE_CASTLING:
				if (start->level == 4 &&
				  start->square == 11*BCOLS+5 &&
				  pos->level == 4 &&
				  pos->square == 11*BCOLS+2) {
					selected_move = &moves[i];
					break;
				}
				break;

			case BLACK_KINGSIDE_CASTLING:
				if (start->level == 0 &&
				  start->square == 2*BCOLS+5 &&
				  pos->level == 0 &&
				  pos->square == 2*BCOLS+6) {
					selected_move = &moves[i];
					break;
				}
				break;

			case BLACK_QUEENSIDE_CASTLING:
				if (start->level == 0 &&
				  start->square == 2*BCOLS+5 &&
				  pos->level == 0 &&
				  pos->square == 2*BCOLS+2) {
					selected_move = &moves[i];
					break;
				}
				break;
		}
	}

	if (selected_move != NULL) {
		reset_selected_squares();

		game_do_move(selected_move);

		ui.start_position_selected = 0;
	}
}

static int
is_same_position(struct position *a, struct position *b)
{
	return a->level == b->level && a->square == b->square;
}

static void
on_position_selected(struct position *pos)
{
	union move moves[MAX_MOVES];

	if (ui.start_position_selected &&
	  !is_same_position(&ui.last_selected_pos, pos) &&
	  ui.selected_squares.selection[pos->level][pos->square] !=
	    UNSELECTED_SQUARE) {
		on_end_position_selected(pos);
	}

	if (get_legal_moves_for_position(moves, pos, &ui.board_state,
	  ui.cur_side) > 0) {
		on_start_position_selected(pos);
	}
}

/* state handlers */

static void
on_worker_reply(union ui_state *state, union move *move)
{
	assert(ui.computer_player_state == THINKING);
	game_do_move(move);
}

#define MIN_TILT (0.3f)
#define MAX_TILT (.5f*M_PI - .01f)
#define MIN_DIST (100.f)
#define MAX_DIST (500.f)

static void
on_drag(union ui_state *state, int button, int x, int y)
{
	int dx, dy;
	struct matrix r, rx, ry;

	if (ui.pos_highlighted) {
		mark_selected_square(&ui.selected_squares,
		  &ui.last_highlighted_pos, UNSELECTED_SQUARE);

		ui.pos_highlighted = 0;
	}

	dx = x - ui.last_clicked_x;
	dy = y - ui.last_clicked_y;

	if (button == BTN_LEFT) {
		mat_make_rotation_around_y(&ry, (float)-dx/100.f);
		mat_make_rotation_around_x(&rx, (float)dy/100.f);
		mat_mul_copy(&r, &rx, &ry);
		mat_mul_copy(&ui.rmatrix, &r, &ui.rmatrix);

		ui.rotation += dx/100.f;
		ui.tilt += dy/100.f;

		if (ui.tilt < MIN_TILT)
			ui.tilt = MIN_TILT;

		if (ui.tilt > MAX_TILT)
			ui.tilt = MAX_TILT;
	} else {
		ui.distance += dx + dy;

		if (ui.distance < MIN_DIST)
			ui.distance = MIN_DIST;

		if (ui.distance > MAX_DIST)
			ui.distance = MAX_DIST;
	}

	ui.last_clicked_x = x;
	ui.last_clicked_y = y;
}

static void
highlight_piece_at(int x, int y)
{
	struct position pos;
	struct matrix mv;

	if (ui.pos_highlighted) {
		mark_selected_square(&ui.selected_squares,
		  &ui.last_highlighted_pos, UNSELECTED_SQUARE);

		ui.pos_highlighted = 0;
	}

	ui_set_modelview_matrix(&mv);

	if (pick_square(&pos, &ui.board_state, &mv, x, y)) {
		const int s = ui.board_state.board[pos.level]
		  [pos.square];

		if (s != EMPTY && (s & BLACK_FLAG) == ui.cur_side) {
			if (!(ui.start_position_selected &&
			  ui.selected_squares.selection
			    [pos.level][pos.square] ==
			      SELECTED_SQUARE)) {
				ui.pos_highlighted = 1;
				ui.last_highlighted_pos = pos;

				mark_selected_square(&ui.selected_squares,
				  &ui.last_highlighted_pos, HIGHLIGHTED_PIECE);
			}
		}
	}
}

static void
on_motion_notify(union ui_state *state, int button, int x, int y)
{
	if (ui.computer_player_state == WAITING_FOR_USER_INPUT)
		highlight_piece_at(x, y);
}

static void
on_button_press(union ui_state *state, int button, int x, int y)
{
	struct position pos;
	struct matrix mv;

	if (ui.computer_player_state == WAITING_FOR_USER_INPUT) {
		if (button == BTN_LEFT) {
			/* left button selects a square */

			ui_set_modelview_matrix(&mv);

			if (pick_square(&pos, &ui.board_state, &mv, x, y)) {
				/*
				 * the picked square must be:
				 *
				 * 1. a piece different than the one already
				 *    selected;
				 *
				 * 2. a square to which the piece currently
				 *    selected can move to.
				 */

				if (!ui.start_position_selected ||
				  !is_same_position(&ui.last_selected_pos,
				    &pos)) {
					on_position_selected(&pos);
				}
			}
		} else {
			/* right button unselects */

			reset_selected_squares();

			highlight_piece_at(x, y);

			ui.start_position_selected = 0;
		}
	}
}

static void
on_button_release(union ui_state *state, int button, int x, int y)
{
}

static void
on_escape_press(union ui_state *state)
{
}

static void
on_escape_release(union ui_state *state)
{
	ui_set_state(STATE_IN_GAME_MENU);
}

static void
render_text(void)
{
	int char_height;
	const int baseline = 22;
	const struct font_render_info *const font = ui.font_small;

	char_height = font->char_height;

	glColor4f(1.f, 1.f, 1.f, 1.f);

	ui_render_text();

	switch (ui.computer_player_state) {
		case DONE_MATE_FOR_WHITE:
			render_string(font, "Mate for white!",
			  5, ui.cur_height - (baseline + char_height + 2));
			break;

		case DONE_MATE_FOR_BLACK:
			render_string(font, "Mate for black!",
			  5, ui.cur_height - (baseline + char_height + 2));
			break;

		default:
			render_string(font,
			  ui.cur_side == BLACK_FLAG ?
			    "Black's turn." : "White's turn.",
			  5, ui.cur_height - (baseline + 2*(char_height + 2)));
			render_string(font,
  			  cur_player_type() == COMPUTER_PLAYER ?
			    "Computer thinking..." : "Your move?",
			  5, ui.cur_height - (baseline + char_height + 2));
			break;
	}
}

static void
redraw(union ui_state *state)
{
	ui_render_board_state();
	render_text();
}

static void
initialize(union ui_state *state)
{
}

static void
on_state_switch(union ui_state *state)
{
}

struct state_waiting_for_move state_waiting_for_move = {
	{
		on_idle,
		on_worker_reply,
		on_motion_notify,
		on_drag,
		on_button_press,
		on_button_release,
		on_escape_press,
		on_escape_release,
		redraw,
		initialize,
		on_state_switch,
	}
};
