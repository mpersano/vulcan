/* game.c -- part of vulcan
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
#include "vector.h"
#include "move.h"
#include "game.h"

const int piece_values[NUM_PIECES] = {
	10,	/* pawn */
	50,	/* rook */
	20,	/* knight */
	30,	/* bishop */
	300,	/* queen */
	30000,	/* king */
};

static struct board_state initial_board_state = {
	/* board */
  { { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, BLACK_ROOK, BLACK_QUEEN, INVALID, INVALID, BLACK_KING, BLACK_ROOK, INVALID,
      INVALID, BLACK_PAWN, BLACK_PAWN, EMPTY, EMPTY, BLACK_PAWN, BLACK_PAWN, INVALID, 
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID },

    { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, BLACK_KNIGHT, BLACK_BISHOP, BLACK_BISHOP, BLACK_KNIGHT, EMPTY, INVALID,
      INVALID, EMPTY, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID },

    { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID },

    { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID },

    { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, WHITE_PAWN, WHITE_PAWN, EMPTY, EMPTY, WHITE_PAWN, WHITE_PAWN, INVALID,
      INVALID, WHITE_ROOK, WHITE_QUEEN, INVALID, INVALID, WHITE_KING, WHITE_ROOK, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID },

    { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, EMPTY, INVALID,
      INVALID, EMPTY, WHITE_KNIGHT, WHITE_BISHOP, WHITE_BISHOP, WHITE_KNIGHT, EMPTY, INVALID, 
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID },

    { INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, INVALID,
      INVALID, EMPTY, EMPTY, INVALID, INVALID, EMPTY, EMPTY, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID,
      INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID, INVALID } },

	/* attack board flags */
	(1UL<<AB_ABOVE_LEFT_UP)|(1UL<<(AB_ABOVE_RIGHT_UP))|
	  (1UL<<(AB_ABOVE_LEFT_DOWN + 16))|(1UL<<(AB_ABOVE_RIGHT_DOWN + 16)),

	/* colors */
	(1UL<<AB_ABOVE_LEFT_UP)|(1UL<<AB_ABOVE_RIGHT_UP),

	/* material imbalance */
	0,

	/* castling rights */
	CR_WHITE_KINGSIDE|CR_WHITE_QUEENSIDE|
	CR_BLACK_KINGSIDE|CR_BLACK_QUEENSIDE,

	/* cur ply */
	0,
};

static void
init_undo_info(struct undo_move_info *undo_info,
  const struct board_state *state)
{
	undo_info->num_squares_touched = 0;
	undo_info->prev_attack_board_bits = state->attack_board_bits;
	undo_info->prev_attack_board_side = state->attack_board_side;
	undo_info->prev_material_imbalance = state->material_imbalance;
	undo_info->prev_castling_rights = state->castling_rights;
}

static void
do_piece_move(struct board_state *state, const struct piece_move *move,
  struct undo_move_info *undo_info)
{
	unsigned char *from, *to;
	int piece, side;

	from = &state->board[move->from.level][move->from.square];
	to = &state->board[move->to.level][move->to.square];

	undo_info->num_squares_touched = 2;

	undo_info->squares_touched[0].level = move->from.level;
	undo_info->squares_touched[0].square = move->from.square;
	undo_info->prev_states[0] = *from;

	undo_info->squares_touched[1].level = move->to.level;
	undo_info->squares_touched[1].square = move->to.square;
	undo_info->prev_states[1] = *to;

	if (*to != EMPTY) {
		if (*to & BLACK_FLAG) {
			state->material_imbalance +=
			  piece_values[(*to & PIECE_MASK) - PAWN];
		} else {
			state->material_imbalance -=
			  piece_values[(*to & PIECE_MASK) - PAWN];
		}
	}

	side = *from & BLACK_FLAG;
	piece = *from & PIECE_MASK;

	*to = *from;

	if (piece == PAWN) {
		int row, col;
		int promoted;

		*to |= MOVED_FLAG;

		/* pawn promotion */

		row = move->to.square/BCOLS;
		col = move->to.square%BCOLS;

		promoted = 0;

		if (side == BLACK_FLAG) {
			/* black */

			if (row == 11) {
				promoted = 1;
			} else if (row == 10) {
				if (col == 3 || col == 4) {
					promoted = 1;
				} else {
					if (col == 2 &&
					  !ATTACK_BOARD_IS_ACTIVE(state, 2,
					    AB_ABOVE_LEFT_DOWN) &&
					  !ATTACK_BOARD_IS_ACTIVE(state, 2,
					    AB_BELOW_LEFT_DOWN)) {
						promoted = 1;
					} else if (col == 5 &&
					  !ATTACK_BOARD_IS_ACTIVE(state, 2,
					    AB_ABOVE_RIGHT_DOWN) &&
					  !ATTACK_BOARD_IS_ACTIVE(state, 2,
					    AB_BELOW_RIGHT_DOWN)) {
						promoted = 1;
					}
				}
			}
	
			if (promoted) {
				state->material_imbalance +=
				  piece_values[QUEEN] - piece_values[PAWN];
			}
		} else {
			/* white */

			if (row == 2) {
				promoted = 1;
			} else if (row == 3) {
				if (col == 3 || col == 4) {
					promoted = 1;
				} else {
					if (col == 2 &&
					  !ATTACK_BOARD_IS_ACTIVE(state, 0,
					    AB_ABOVE_LEFT_UP) &&
					  !ATTACK_BOARD_IS_ACTIVE(state, 0,
					    AB_BELOW_LEFT_UP)) {
						promoted = 1;
					} else if (col == 5 &&
					  !ATTACK_BOARD_IS_ACTIVE(state, 0,
					    AB_ABOVE_RIGHT_UP) &&
					  !ATTACK_BOARD_IS_ACTIVE(state, 0,
					    AB_BELOW_RIGHT_UP)) {
						promoted = 1;
					}
				}
			}

			if (promoted) {
				state->material_imbalance +=
				  piece_values[QUEEN] - piece_values[PAWN];
			}
		}

		if (promoted)
			*to = (*to & BLACK_FLAG)|QUEEN;
	} else if (piece == KING) {
		if (side == BLACK_FLAG) {
			state->castling_rights &=
			  ~(CR_BLACK_KINGSIDE|CR_BLACK_QUEENSIDE);
		} else {
			state->castling_rights &=
			  ~(CR_WHITE_KINGSIDE|CR_WHITE_QUEENSIDE);
		}
	} else if (piece == ROOK) {
		if (side == BLACK_FLAG) {
			if ((state->castling_rights & CR_BLACK_QUEENSIDE) &&
			  move->from.square == 2*BCOLS + 1)
				state->castling_rights &= ~CR_BLACK_QUEENSIDE;
			else if ((state->castling_rights & CR_BLACK_KINGSIDE) &&
			  move->from.square == 2*BCOLS + 6)
				state->castling_rights &= ~CR_BLACK_KINGSIDE;
		} else {
			if ((state->castling_rights & CR_WHITE_QUEENSIDE) &&
			  move->from.square == 11*BCOLS + 1)
				state->castling_rights &= ~CR_WHITE_QUEENSIDE;
			else if ((state->castling_rights & CR_WHITE_KINGSIDE) &&
			  move->from.square == 11*BCOLS + 6)
				state->castling_rights &= ~CR_WHITE_KINGSIDE;
		}
	}

	*from = EMPTY;
}

static void
do_attack_board_move(struct board_state *state,
  const struct attack_board_move *move, struct undo_move_info *undo_info)
{
	int i, j;
	struct position from_pos, to_pos;
	struct position *touched;
	unsigned char *from, *to, *prev_state;
	int from_square, to_square;

	undo_info->num_squares_touched = 8;

	get_attack_board_position(&from_pos, move->from.main_board,
	  move->from.position);

	get_attack_board_position(&to_pos, move->to.main_board,
	  move->to.position);

	from_square = from_pos.square;
	to_square = to_pos.square;

	from = &state->board[from_pos.level][from_square];
	to = &state->board[to_pos.level][to_square];

	touched = undo_info->squares_touched;
	prev_state = undo_info->prev_states;

	for (i = 0; i < ATTACK_BOARD_SIZE; i++) {
		for (j = 0; j < ATTACK_BOARD_SIZE; j++) {
			touched->level = from_pos.level;
			touched->square = from_square;
			++touched;
			*prev_state++ = *from;

			touched->level = to_pos.level;
			touched->square = to_square;
			++touched;
			*prev_state++ = *to;

			if (((*to = *from) & PIECE_MASK) == PAWN)
				*to |= MOVED_FLAG;
			*from = EMPTY;
			++from;
			++to;
			++from_square;
			++to_square;
		}

		from += BCOLS - ATTACK_BOARD_SIZE;
		to += BCOLS - ATTACK_BOARD_SIZE;
		from_square += BCOLS - ATTACK_BOARD_SIZE;
		to_square += BCOLS - ATTACK_BOARD_SIZE;
	}

	state->attack_board_bits ^=
	  1UL << (move->from.position + 8*move->from.main_board);

	state->attack_board_bits ^=
	  1UL << (move->to.position + 8*move->to.main_board);

	if (state->attack_board_side &
	  (1UL << (8*move->from.main_board + move->from.position))) {
		state->attack_board_side &=
		  ~(1UL << (8*move->from.main_board + move->from.position));
		state->attack_board_side |=
		  1UL << (8*move->to.main_board + move->to.position);
	} else {
		state->attack_board_side &=
		  ~(1UL << (8*move->to.main_board + move->to.position));
	}

	if ((state->castling_rights & CR_BLACK_QUEENSIDE) &&
	  move->from.main_board == 0 &&
	  move->from.position == AB_ABOVE_LEFT_UP) {
		state->castling_rights &= ~CR_BLACK_QUEENSIDE;
	}

	if ((state->castling_rights & CR_BLACK_KINGSIDE) &&
	  move->from.main_board == 0 &&
	  move->from.position == AB_ABOVE_RIGHT_UP) {
		state->castling_rights &=
		  ~(CR_BLACK_KINGSIDE|CR_BLACK_QUEENSIDE);
	}

	if ((state->castling_rights & CR_WHITE_QUEENSIDE) &&
	  move->from.main_board == 2 &&
	  move->from.position == AB_ABOVE_LEFT_DOWN) {
		state->castling_rights &= ~CR_WHITE_QUEENSIDE;
	}

	if ((state->castling_rights & CR_WHITE_KINGSIDE) &&
	  move->from.main_board == 2 &&
	  move->from.position == AB_ABOVE_RIGHT_DOWN) {
		state->castling_rights &=
		  ~(CR_WHITE_KINGSIDE|CR_WHITE_QUEENSIDE);
	}
}

void
do_move(struct board_state *state, const union move *move,
  struct undo_move_info *undo_info)
{
	init_undo_info(undo_info, state);

	switch (move->type) {
		case PIECE_MOVE:
			do_piece_move(state, &move->piece_move, undo_info);
			break;

		case ATTACK_BOARD_MOVE:
			do_attack_board_move(state, &move->attack_board_move,
			  undo_info);
			break;

		case WHITE_KINGSIDE_CASTLING:
			undo_info->num_squares_touched = 2;

			undo_info->squares_touched[0].level = 4;
			undo_info->squares_touched[0].square = 11*BCOLS + 5;
			undo_info->prev_states[0] = WHITE_KING;

			undo_info->squares_touched[1].level = 4;
			undo_info->squares_touched[1].square = 11*BCOLS + 6;
			undo_info->prev_states[1] = WHITE_ROOK;

			state->board[4][11*BCOLS + 5] = WHITE_ROOK;
			state->board[4][11*BCOLS + 6] = WHITE_KING;

			state->castling_rights &=
			  ~(CR_WHITE_KINGSIDE|CR_WHITE_QUEENSIDE);
			break;

		case WHITE_QUEENSIDE_CASTLING:
			undo_info->num_squares_touched = 3;

			undo_info->squares_touched[0].level = 4;
			undo_info->squares_touched[0].square = 11*BCOLS + 5;
			undo_info->prev_states[0] = WHITE_KING;

			undo_info->squares_touched[1].level = 4;
			undo_info->squares_touched[1].square = 11*BCOLS + 2;
			undo_info->prev_states[1] = EMPTY;

			undo_info->squares_touched[2].level = 4;
			undo_info->squares_touched[2].square = 11*BCOLS + 1;
			undo_info->prev_states[2] = WHITE_ROOK;

			state->board[4][11*BCOLS + 5] = WHITE_ROOK;
			state->board[4][11*BCOLS + 2] = WHITE_KING;
			state->board[4][11*BCOLS + 1] = EMPTY;

			state->castling_rights &=
			  ~(CR_WHITE_KINGSIDE|CR_WHITE_QUEENSIDE);
			break;

		case BLACK_KINGSIDE_CASTLING:
			undo_info->num_squares_touched = 2;

			undo_info->squares_touched[0].level = 0;
			undo_info->squares_touched[0].square = 2*BCOLS + 5;
			undo_info->prev_states[0] = BLACK_KING;

			undo_info->squares_touched[1].level = 0;
			undo_info->squares_touched[1].square = 2*BCOLS + 6;
			undo_info->prev_states[1] = BLACK_ROOK;

			state->board[0][2*BCOLS + 5] = BLACK_ROOK;
			state->board[0][2*BCOLS + 6] = BLACK_KING;

			state->castling_rights &=
			  ~(CR_BLACK_KINGSIDE|CR_BLACK_QUEENSIDE);
			break;

		case BLACK_QUEENSIDE_CASTLING:
			undo_info->num_squares_touched = 3;

			undo_info->squares_touched[0].level = 0;
			undo_info->squares_touched[0].square = 2*BCOLS + 5;
			undo_info->prev_states[0] = BLACK_KING;

			undo_info->squares_touched[1].level = 0;
			undo_info->squares_touched[1].square = 2*BCOLS + 2;
			undo_info->prev_states[1] = EMPTY;

			undo_info->squares_touched[2].level = 0;
			undo_info->squares_touched[2].square = 2*BCOLS + 1;
			undo_info->prev_states[2] = BLACK_ROOK;

			state->board[0][2*BCOLS + 5] = BLACK_ROOK;
			state->board[0][2*BCOLS + 2] = BLACK_KING;
			state->board[0][2*BCOLS + 1] = EMPTY;

			state->castling_rights &=
			  ~(CR_BLACK_KINGSIDE|CR_BLACK_QUEENSIDE);
			break;

		default:
			assert(0);
	}

	state->cur_ply++;
}

void
undo_move(struct board_state *state,
  const struct undo_move_info *undo_info)
{
	int i;

	for (i = 0; i < undo_info->num_squares_touched; i++) {
		state->board[undo_info->squares_touched[i].level]
		  [undo_info->squares_touched[i].square] =
		  undo_info->prev_states[i];
	}

	state->attack_board_bits = undo_info->prev_attack_board_bits;
	state->attack_board_side = undo_info->prev_attack_board_side;
	state->material_imbalance = undo_info->prev_material_imbalance;
	state->castling_rights = undo_info->prev_castling_rights;
	state->cur_ply--;
}


/*
 * get_main_board_position --
 *	Get the position of the top left square of a main board.
 */
void
get_main_board_position(struct position *pos, int main_board)
{
	pos->level = 1 + 2*main_board;
	pos->square = 2 + (3 + 2*main_board)*BCOLS;
}



/*
 * get_attack_board_position --
 *	Get the position of the top left square of an attack board.
 */
void
get_attack_board_position(struct position *pos, int main_board,
  int attack_board)
{
	int row, col;

	pos->level = 2*main_board;
	if (ATTACK_BOARD_IS_BELOW(attack_board))
		pos->level += 2;

	row = 2 + 2*main_board;
	if (ATTACK_BOARD_IS_DOWN(attack_board))
		row += 4;

	col = 1;
	if (ATTACK_BOARD_IS_RIGHT(attack_board))
		col += 4;

	pos->square = row*BCOLS + col;
}


/*
 * attack_board_is_empty --
 *	Return true if the attack board has no pieces on it.
 */
int
attack_board_is_empty(const struct board_state *state,
  int main_board, int attack_board)
{
	int i, j;
	struct position pos;
	const unsigned char *p;

	get_attack_board_position(&pos, main_board, attack_board);

	p = &state->board[pos.level][pos.square];

	for (i = 0; i < ATTACK_BOARD_SIZE; i++) {
		for (j = 0; j < ATTACK_BOARD_SIZE; j++) {
			if (*p++ != EMPTY)
				return 0;
		}

		p += BCOLS - ATTACK_BOARD_SIZE;
	}

	return 1;
}


/*
 * attack_board_can_move --
 *	Return true if the attack board is either empty or has a single piece
 *	on it.
 */
int
attack_board_can_move(const struct board_state *state,
  int main_board, int attack_board)
{
	int i, j, piece_count;
	struct position pos;
	unsigned char s;
	const unsigned char *p;
	int side;

	side = ATTACK_BOARD_SIDE(state, main_board, attack_board);

	get_attack_board_position(&pos, main_board, attack_board);

	piece_count = 0;
	p = &state->board[pos.level][pos.square];

	for (i = 0; i < ATTACK_BOARD_SIZE; i++) {
		for (j = 0; j < ATTACK_BOARD_SIZE; j++) {
			s = *p++;

			if (s != EMPTY) {
				if ((s & BLACK_FLAG) != side)
					return 0;

				if (++piece_count > 1)
					return 0;

				++piece_count;
			}
		}

		p += BCOLS - ATTACK_BOARD_SIZE;
	}

	return 1;
}


/*
 * get_attack_board_for_position --
 *	Fill in attack_board structure for attack board that contains
 *	square specified by pos. Return 0 if pos is not on an attack board.
 */
int
get_attack_board_for_position(struct attack_board *aboard,
  const struct board_state *state, const struct position *pos)
{
	int i, j, r, c;
	int br, bc;
	struct position first_pos;

	r = pos->square/BCOLS;
	c = pos->square%BCOLS;

	for (i = 0; i < NUM_MAIN_BOARDS; i++) {
		for (j = 0; j < 8; j++) {
			if (ATTACK_BOARD_IS_ACTIVE(state, i, j)) {
				get_attack_board_position(&first_pos, i, j);

				if (first_pos.level == pos->level) {
					br = first_pos.square/BCOLS;
					bc = first_pos.square%BCOLS;

					if (r - br >= 0 &&
					  r - br < ATTACK_BOARD_SIZE &&
					  c - bc >= 0 &&
					  c - bc < ATTACK_BOARD_SIZE) {
						aboard->main_board = i;
						aboard->position = j;
						return 1;
					}
				}
			}
		}
	}

	return 0;
}

void
get_closest_main_board_square_for_attack_board(struct position *pos,
  const struct attack_board *aboard)
{
	struct position first_pos;
	int r, c;

	r = ATTACK_BOARD_IS_UP(aboard->position) ? 0 : 3;
	c = ATTACK_BOARD_IS_LEFT(aboard->position) ? 0 : 3;

	get_main_board_position(&first_pos, aboard->main_board);

	pos->level = first_pos.level;
	pos->square = first_pos.square + r*BCOLS + c;
}

enum game_status
get_game_status(struct board_state *state)
{
	union move moves[MAX_MOVES];
	int nw, nb;
	
	nw = get_legal_moves(moves, state, 0);
	nb = get_legal_moves(moves, state, BLACK_FLAG);

	if (nw == 0)
		return MATE_FOR_BLACK;

	if (nb == 0)
		return MATE_FOR_WHITE;

	return IN_PLAY;
}

void
init_board_state(struct board_state *state)
{
	memcpy(state, &initial_board_state, sizeof *state);
}

static void
attack_board_as_string(const struct attack_board *ab, char *buf)
{
	sprintf(buf, "%s%d",
	  ATTACK_BOARD_IS_LEFT(ab->position) ?  "QL" : "KL",
	  5 - ab->main_board*2 + ATTACK_BOARD_IS_UP(ab->position));
}

static void
position_as_string(const struct board_state *state, const struct position *pos,
  char *buf)
{
	char *p;
	int row, col;

	row = pos->square/BCOLS - 2;
	col = pos->square%BCOLS - 1;

	sprintf(buf, "%c%d", "zabcde"[col], 9 - row);

	p = buf + strlen(buf);

	switch (pos->level) {
		case 1:
			/* black's board */
			strcat(buf, "B");
			break;

		case 3:
			/* neutral board */
			strcat(buf, "N");
			break;

		case 5:
			/* white's board */
			strcat(buf, "W");
			break;

		default:
			{
				/* attack board */

				struct attack_board ab = {0};
				int r;

				r = get_attack_board_for_position(&ab, state,
				  pos);

				assert(r);

				attack_board_as_string(&ab, buf + strlen(buf));
			}
			break;
	}
}

static void
piece_move_as_string(const struct board_state *state,
  const struct piece_move *pm, char *buf)
{
	int piece_from, piece_to;

	piece_from = state->board[pm->from.level][pm->from.square];
	piece_to = state->board[pm->to.level][pm->to.square];

	if ((piece_from & PIECE_MASK) != PAWN)
		*buf++ = "RNBQK"[(piece_from & PIECE_MASK) - PAWN - 1];

	if (piece_to != EMPTY) {
		if ((piece_from & PIECE_MASK) == PAWN) {
			position_as_string(state, &pm->from, buf);
			buf = &buf[strlen(buf)];
		}

		*buf++ = 'x';
	}

	position_as_string(state, &pm->to, buf);
}

static void
attack_board_move_as_string(const struct board_state *state,
  const struct attack_board_move *am, char *buf)
{
	attack_board_as_string(&am->to, buf);
}

char *
move_as_string(const struct board_state *state, const union move *move)
{
	static char buf[80];

	switch (move->type) {
		case PIECE_MOVE:
			piece_move_as_string(state, &move->piece_move, buf);
			break;

		case ATTACK_BOARD_MOVE:
			attack_board_move_as_string(state,
			  &move->attack_board_move, buf);
			break;

		case WHITE_QUEENSIDE_CASTLING:
		case BLACK_QUEENSIDE_CASTLING:
			return "o-o-o";

		case WHITE_KINGSIDE_CASTLING:
		case BLACK_KINGSIDE_CASTLING:
			return "o-o";
	}

	return buf;
}

#if 0
void
print_move(union move *move)
{
	if (move->type == PIECE_MOVE) {
		const struct piece_move *pm = &move->piece_move;

		printf("piece: from %d,%d to %d,%d\n",
		  pm->from.level, pm->from.square,
		  pm->to.level, pm->to.square);
	} else {
		const struct attack_board_move *am = &move->attack_board_move;

		printf("attack board: from %d-%d to %d-%d\n",
		  am->from.main_board, am->from.position,
		  am->to.main_board, am->to.position);
	}
}
#endif
