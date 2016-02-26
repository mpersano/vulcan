/* move.c -- part of vulcan
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "game.h"
#include "move.h"

static const int knight_moves[] = {
	2*BCOLS+1, 2*BCOLS-1, -2*BCOLS+1, -2*BCOLS-1,
	BCOLS+2, BCOLS-2, -BCOLS+2, -BCOLS-2 };

static const int king_moves[] = { -BCOLS-1, -BCOLS, -BCOLS+1, 1,
	BCOLS+1, BCOLS, BCOLS-1, -1 };

static int
is_in_check_from(const struct board_state *state, int side, int king_square);

static int
find_king_square(const struct board_state *state, int side);

static void
find_pins(unsigned char pins[BLEVELS][BAREA], const struct board_state *state,
  int side, int king_square);

static unsigned long amask_table[BLEVELS][BAREA];

static void
init_amask_table(void)
{
	int i, j, r, c;
	struct position first_pos;
	unsigned long amask;
	volatile unsigned long *p;

	memset(amask_table, 0xff, sizeof amask_table);

	for (i = 0; i < NUM_MAIN_BOARDS; i++) {
		get_main_board_position(&first_pos, i);

		p = &amask_table[first_pos.level][first_pos.square];

		for (r = 0; r < 4; r++) {
			for (c = 0; c < 4; c++)
				*p++ = 0UL;

			p += BCOLS - 4;
		}

		for (j = 0; j < 8; j++) {
			get_attack_board_position(&first_pos, i, j);

			p = &amask_table[first_pos.level][first_pos.square];

			amask = 1UL<<(j + 8*i);

			for (r = 0; r < 2; r++) {
				for (c = 0; c < 2; c++)
					*p++ = amask;

				p += BCOLS - 2;
			}
		}
	}
}

struct move_gen_context {
	struct board_state *state;
	int king_square;
	int is_in_check;
	int is_pinned;
	union move *last_move;
	struct position from;
	int side;
	unsigned char pins[BLEVELS][BAREA];
};

static inline void
append_move(struct move_gen_context *ctx, int level, int square)
{
	union move *last_move = ctx->last_move;

	last_move->type = PIECE_MOVE;
	last_move->piece_move.from = ctx->from;
	last_move->piece_move.to.level = level;
	last_move->piece_move.to.square = square;

	ctx->last_move++;
}

static inline void
append_move_if_legal(struct move_gen_context *ctx, int level, int square)
{
	unsigned char *from, *to;
	unsigned char prev_to, prev_from;
	int ks;

	/* try move */

	from = &ctx->state->board[ctx->from.level][ctx->from.square];
	to = &ctx->state->board[level][square];

	prev_from = *from;
	prev_to = *to;

	*from = EMPTY;
	*to = prev_from;

	/* leads to check? */

	ks = ((prev_from & PIECE_MASK) == KING) ? square : ctx->king_square;

/*
	if (ks != find_king_square(ctx->state, ctx->side)) {
		printf("%d --> %d, %d, side = %d, ks = %d, square = %d | %d %d\n", ctx->from.square, square, prev_from, ctx->side, ks, square, ctx->king_square,
		  find_king_square(ctx->state, ctx->side));
		printf("%d %d\n", *from, *to);
		assert(0);
	}
*/

	if (!is_in_check_from(ctx->state, ctx->side, ks))
		append_move(ctx, level, square);

	/* undo move */

	*from = prev_from;
	*to = prev_to;
}

static inline int
append_squares_at(struct move_gen_context *ctx, int square)
{
	int blocked;
	int l;
	unsigned long amask;
	struct board_state *state;

	state = ctx->state;
	amask = state->attack_board_bits;

	blocked = 0;

	for (l = 0; l < BLEVELS; l++) {
		const unsigned long m = amask_table[l][square];

		if (m != ~0UL && (amask & m) == m) {
			const unsigned char s = state->board[l][square];

			assert(s != INVALID);

			if (s == EMPTY || (s & BLACK_FLAG) != ctx->side) {
				if (!ctx->is_in_check && !ctx->is_pinned)
					append_move(ctx, l, square);
				else
					append_move_if_legal(ctx, l, square);
			}

			if (s != EMPTY)
				blocked = 1;
		}
	}

	return blocked;
}

static inline void
follow_dir(struct move_gen_context *ctx, int delta)
{
	int cur;
	struct board_state *state;

	state = ctx->state;
	cur = ctx->from.square + delta;

	while (state->board[0][cur] != INVALID) {
		if (append_squares_at(ctx, cur))
			break;

		cur += delta;
	}
}

static inline void
walk_king(struct move_gen_context *ctx)
{
	int i, square;

	for (i = 0; i < sizeof king_moves / sizeof *king_moves; i++) {
		square = ctx->from.square + king_moves[i];

		if (ctx->state->board[0][square] != INVALID)
			append_squares_at(ctx, square);
	}

	/* castling */

	if (!ctx->is_in_check && ctx->state->cur_ply > 1) {
		struct board_state *state = ctx->state;
		union move *last_move = ctx->last_move;
		const unsigned castling_rights = state->castling_rights;

		if (ctx->side == BLACK_FLAG) {
			if ((castling_rights & CR_BLACK_KINGSIDE) &&
			  !is_in_check_from(state, BLACK_FLAG, 2*BCOLS+6)) {
				last_move->type = BLACK_KINGSIDE_CASTLING;
				++last_move;
			}

			if ((castling_rights & CR_BLACK_QUEENSIDE) &&
			  state->board[0][2*BCOLS+2] == EMPTY &&
			  !is_in_check_from(state, BLACK_FLAG, 2*BCOLS+2)) {
				last_move->type = BLACK_QUEENSIDE_CASTLING;
				++last_move;
			}
		} else {
			if ((castling_rights & CR_WHITE_KINGSIDE) &&
			  !is_in_check_from(state, 0, 11*BCOLS+6)) {
				last_move->type = WHITE_KINGSIDE_CASTLING;
				++last_move;
			}

			if (castling_rights & CR_WHITE_QUEENSIDE &&
			  state->board[4][11*BCOLS+2] == EMPTY &&
			  !is_in_check_from(state, 0, 11*BCOLS+2)) {
				last_move->type = WHITE_QUEENSIDE_CASTLING;
				++last_move;
			}
		}

		ctx->last_move = last_move;
	}
}

static inline void
walk_knight(struct move_gen_context *ctx)
{
	int i, square;

	for (i = 0; i < sizeof knight_moves / sizeof *knight_moves; i++) {
		square = ctx->from.square + knight_moves[i];

		if (ctx->state->board[0][square] != INVALID)
			append_squares_at(ctx, square);
	}
}

static inline int
append_squares_no_capture(struct move_gen_context *ctx, int square)
{
	int blocked;
	struct board_state *state;
	unsigned long amask;
	int l;

	state = ctx->state;
	amask = state->attack_board_bits;

	blocked = 0;

	for (l = 0; l < BLEVELS; l++) {
		const unsigned long m = amask_table[l][square];

		if (m != ~0UL && (amask & m) == m) {
			const unsigned char s = state->board[l][square];

			assert(s != INVALID);

			if (s == EMPTY)
				append_move_if_legal(ctx, l, square);

			if (s != EMPTY)
				blocked = 1;
		}
	}

	return blocked;
}

static inline void
append_squares_capture_only(struct move_gen_context *ctx, int square, int side)
{
	int l;
	struct board_state *state;

	state = ctx->state;

	for (l = 0; l < BLEVELS; l++) {
		const unsigned long m = amask_table[l][square];

		if (m != ~0UL && (state->attack_board_bits & m) == m) {
			const unsigned char s = state->board[l][square];

			assert(s != INVALID);

			if (s != EMPTY && (s & BLACK_FLAG) != side)
				append_move_if_legal(ctx, l, square);
		}
	}
}

static inline void
walk_black_pawn(struct move_gen_context *ctx, int moved)
{
	int blocked;
	struct board_state *state;
	int from;

	state = ctx->state;
	from = ctx->from.square;

	blocked = 1;

	if (state->board[0][from + BCOLS] != INVALID)
		blocked = append_squares_no_capture(ctx, from + BCOLS);

	if (!moved && !blocked && state->board[0][from + 2*BCOLS] != INVALID)
		append_squares_no_capture(ctx, from + 2*BCOLS);

	if (state->board[0][from + BCOLS + 1] != INVALID)
		append_squares_capture_only(ctx, from + BCOLS + 1,
		  BLACK_FLAG);

	if (state->board[0][from + BCOLS - 1] != INVALID)
		append_squares_capture_only(ctx, from + BCOLS - 1,
		  BLACK_FLAG);
}

static inline void
walk_white_pawn(struct move_gen_context *ctx, int moved)
{
	int blocked;
	struct board_state *state;
	int from;

	state = ctx->state;
	from = ctx->from.square;

	blocked = 1;

	if (state->board[0][from - BCOLS] != INVALID)
		blocked = append_squares_no_capture(ctx, from - BCOLS);

	if (!moved && !blocked && state->board[0][from - 2*BCOLS] != INVALID)
		append_squares_no_capture(ctx, from - 2*BCOLS);

	if (state->board[0][from - BCOLS + 1] != INVALID)
		append_squares_capture_only(ctx, from - BCOLS + 1, 0);

	if (state->board[0][from - BCOLS - 1] != INVALID)
		append_squares_capture_only(ctx, from - BCOLS - 1, 0);
}

void
init_move_tables(void)
{
	init_amask_table();
}

static inline void
get_next_moves_for_piece(struct move_gen_context *ctx, int piece, int moved)
{
	struct board_state *state;
	int from;
	int side;

	state = ctx->state;
	from = ctx->from.square;
	side = ctx->side;

	switch (piece) {
		case ROOK:
		case QUEEN:
  			follow_dir(ctx, -BCOLS);
  			follow_dir(ctx, BCOLS);
  			follow_dir(ctx, 1);
  			follow_dir(ctx, -1);

			if (piece == ROOK)
				break;

			/* fallthrough for queen */

		case BISHOP:
  			follow_dir(ctx, -BCOLS - 1);
  			follow_dir(ctx, -BCOLS + 1);
  			follow_dir(ctx, BCOLS - 1);
  			follow_dir(ctx, BCOLS + 1);
			break;

		case KING:
			walk_king(ctx);
			break;

		case KNIGHT:
			walk_knight(ctx);
			break;

		case PAWN:
			if (side == BLACK_FLAG)
				walk_black_pawn(ctx, moved);
			else
				walk_white_pawn(ctx, moved);
			break;

		default:
			assert(0 && "invalid piece type");
	}
}

int
get_next_positions_for_attack_board(struct attack_board *positions, 
  const struct board_state *state, const struct attack_board *from, int side)
{
	int last_move;

	if (!attack_board_can_move(state, from->main_board, from->position))
		return 0;

	last_move = 0;

	if (!ATTACK_BOARD_IS_ACTIVE(state, from->main_board,
	  from->position ^ AB_UP_FLAG)) {
		positions[last_move].main_board = from->main_board;
		positions[last_move].position = from->position ^ AB_UP_FLAG;
		++last_move;
	}

	if (!ATTACK_BOARD_IS_ACTIVE(state, from->main_board,
	  from->position ^ AB_RIGHT_FLAG)) {
		positions[last_move].main_board = from->main_board;
		positions[last_move].position = from->position ^ AB_RIGHT_FLAG;
		++last_move;
	}

	if (attack_board_is_empty(state, from->main_board, from->position)) {
		if (!ATTACK_BOARD_IS_ACTIVE(state, from->main_board,
		  from->position ^ AB_ABOVE_FLAG)) {
			positions[last_move].main_board = from->main_board;
			positions[last_move].position =
			  from->position ^ AB_ABOVE_FLAG;
			++last_move;
		}

		if (from->main_board > 0 &&
		  ATTACK_BOARD_IS_ABOVE(from->position)) {
			  if (!ATTACK_BOARD_IS_ACTIVE(state,
			    from->main_board - 1,
			    from->position ^ AB_ABOVE_FLAG)) {
				positions[last_move].main_board =
				  from->main_board - 1;
				positions[last_move].position =
				  from->position ^ AB_ABOVE_FLAG;
				++last_move;
			}

			  if (!ATTACK_BOARD_IS_ACTIVE(state,
			    from->main_board - 1,
			    (from->position ^ AB_ABOVE_FLAG) ^ AB_UP_FLAG)) {
				positions[last_move].main_board =
				  from->main_board - 1;
				positions[last_move].position =
				  (from->position ^ AB_ABOVE_FLAG) ^
				    AB_UP_FLAG;
				++last_move;
			}
		}

		if (from->main_board < NUM_MAIN_BOARDS - 1 &&
		  ATTACK_BOARD_IS_BELOW(from->position)) {
			if (!ATTACK_BOARD_IS_ACTIVE(state,
			  from->main_board + 1,
			  from->position ^ AB_ABOVE_FLAG)) {
				positions[last_move].main_board =
				  from->main_board + 1;
				positions[last_move].position =
				  from->position ^ AB_ABOVE_FLAG;
				++last_move;
			}

			if (!ATTACK_BOARD_IS_ACTIVE(state,
			  from->main_board + 1,
			  (from->position ^ AB_ABOVE_FLAG) ^ AB_UP_FLAG)) {
				positions[last_move].main_board =
				  from->main_board + 1;
				positions[last_move].position =
				  (from->position ^ AB_ABOVE_FLAG) ^
				    AB_UP_FLAG;
				++last_move;
			}
		}
	}

	return last_move;
}

static int
is_legal_move(const union move *move, struct board_state *state, int side)
{
	struct undo_move_info undo_info;
	struct board_state temp;
	int r;

	temp = *state;

	do_move(state, move, &undo_info);
	r = !is_in_check(state, side);
	undo_move(state, &undo_info);

	return r;
}

int
get_legal_moves_for_position(union move *moves,
  const struct position *from_pos, struct board_state *state, int side)
{
	int p, npos, i;
	union move *last_move;

	p = state->board[from_pos->level][from_pos->square];

	last_move = moves;

	if (p != EMPTY) {
		int piece, moved;
		static struct move_gen_context ctx;

		/* piece moves */
		if ((p & BLACK_FLAG) == side) {
			piece = p & PIECE_MASK;
			moved = p & MOVED_FLAG;

			ctx.state = state;
			ctx.king_square = find_king_square(state, side);
			ctx.is_in_check = is_in_check_from(state, side,
			  ctx.king_square);
			find_pins(ctx.pins, state, side, ctx.king_square);
			ctx.last_move = moves;
			ctx.side = side;
			ctx.from = *from_pos;
			ctx.is_pinned = (piece == KING) ||
			  ctx.pins[from_pos->level][from_pos->square];

			get_next_moves_for_piece(&ctx, piece, moved);

			last_move = ctx.last_move;
		}
	} else {
		struct attack_board from, positions[MAX_MOVES];

		/* attack board moves */
		if (get_attack_board_for_position(&from, state, from_pos)) {

			if (ATTACK_BOARD_SIDE(state, from.main_board,
			  from.position) == side) {
				npos = get_next_positions_for_attack_board(
				  positions, state, &from, side);

				for (i = 0; i < npos; i++) {
					last_move->type = ATTACK_BOARD_MOVE;
					last_move->attack_board_move.from =
					  from;
					last_move->attack_board_move.to =
					  positions[i];

					if (is_legal_move(last_move, state,
					  side))
						++last_move;
				}
			}
		}
	}

	return last_move - moves;
}

static void
get_legal_piece_moves_for_board(struct move_gen_context *ctx,
  struct position *first, int size)
{
	int i, j;
	int square, level;
	unsigned char p;
	unsigned char *from;
	struct board_state *state;
	int king_square;
	int side;

	state = ctx->state;
	king_square = ctx->king_square;
	side = ctx->side;

	level = first->level;
	square = first->square;

	for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
			from = &state->board[level][square];
			p = *from;

			if (p != EMPTY && (p & BLACK_FLAG) == side) {
				ctx->from.level = level;
				ctx->from.square = square;
				ctx->is_pinned = ((p&PIECE_MASK) == KING) ||
				  ctx->pins[level][square];

				get_next_moves_for_piece(ctx, p&PIECE_MASK,
				  p&MOVED_FLAG);
			}

			++square;
		}

		square += BCOLS - size;
	}
}

static int
get_legal_piece_moves(union move *moves, struct board_state *state, int side)
{
	int i, j;
	struct position first;
	static struct move_gen_context ctx;

	ctx.state = state;
	ctx.king_square = find_king_square(state, side);
	ctx.is_in_check = is_in_check_from(state, side, ctx.king_square);
	find_pins(ctx.pins, state, side, ctx.king_square);
	ctx.last_move = moves;
	ctx.side = side;

	for (i = 0; i < NUM_MAIN_BOARDS; i++) {
		get_main_board_position(&first, i);

		get_legal_piece_moves_for_board(&ctx, &first, MAIN_BOARD_SIZE);

		for (j = 0; j < 8; j++) {
			if (ATTACK_BOARD_IS_ACTIVE(state, i, j)) {
				get_attack_board_position(&first, i, j);

				get_legal_piece_moves_for_board(&ctx, &first,
				  ATTACK_BOARD_SIZE);
			}
		}
	}

	return ctx.last_move - moves;
}

static int
get_legal_attack_board_moves(union move *moves, struct board_state *state,
  int side)
{
	struct attack_board from, positions[MAX_MOVES];
	int i, j, k, npos;
	union move *last_move;

	last_move = moves;

	for (i = 0; i < NUM_MAIN_BOARDS; i++) {
		for (j = 0; j < 8; j++) {
			if (ATTACK_BOARD_IS_ACTIVE(state, i, j) &&
			  ATTACK_BOARD_SIDE(state, i, j) == side) {
			  	from.main_board = i;
				from.position = j;

				npos = get_next_positions_for_attack_board(
				  positions, state, &from, side);

				for (k = 0; k < npos; k++) {
					last_move->type = ATTACK_BOARD_MOVE;
					last_move->attack_board_move.from =
					  from;
					last_move->attack_board_move.to =
					  positions[k];

					if (is_legal_move(last_move, state,
					  side))
						++last_move;
				}
	
			
			}
		}
	}

	return last_move - moves;
}

static int
eval_legal_moves(union move *moves, struct board_state *state, int side)
{
	int n;

	n = get_legal_piece_moves(moves, state, side);
	n += get_legal_attack_board_moves(&moves[n], state, side);

/*
{
	int i;

	for (i = 0; i < n; i++) {
		if (!is_legal_move(&moves[i], state, side)) {
			printf("illegal move: %s - %s\n", moves[i].type == PIECE_MOVE ? "piece move" : "attack board move", move_as_string(state, &moves[i]));
			assert(0);
		}
	}
}
*/

	assert(n < MAX_MOVES);

	return n;
}

int
get_legal_moves(union move *moves, struct board_state *state, int side)
{
	return eval_legal_moves(moves, state, side);
}

static int
find_piece_at(const struct board_state *state, int square, int piece, int side)
{
	int blocked;
	int l;

	blocked = 0;

	for (l = 0; l < BLEVELS; l++) {
		const unsigned char s = state->board[l][square] & ~MOVED_FLAG;

		if (s == (piece|side) || s == (QUEEN|side) /* HACK */)
			return 1;

		if (s != EMPTY)
			blocked = -1;
	}

	return blocked;
}

static int
find_piece_on_dir(const struct board_state *state, int from, int delta,
  int piece, int side)
{
	int cur, s;

	cur = from + delta;

	while (state->board[0][cur] != INVALID) {
		s = find_piece_at(state, cur, piece, side);

		if (s == 1)
			return 1;

		if (s == -1)
			break;

		cur += delta;
	}

	return 0;
}

int
is_in_check_expensive(struct board_state *state, int side)
{
	union move moves[600];
	int i, n;

	n = get_legal_piece_moves(moves, state, side ^ BLACK_FLAG);

	for (i = 0; i < n; i++) {
		assert((state->board[moves[i].piece_move.to.level]
		  [moves[i].piece_move.to.square] & ~MOVED_FLAG)
		    != (KING|side));
	}

	return 0;
}

static void
find_pin_on_dir(unsigned char pins[BLEVELS][BAREA],
  const struct board_state *state, int king_square, int delta, int piece,
    int side)
{
	int cur, l;
	int pin_level, pin_square;

	cur = king_square + delta;
	pin_level = pin_square = -1;

	while (state->board[0][cur] != INVALID) {
		for (l = 0; l < BLEVELS; l++) {
			const unsigned char s = state->board[l][cur];

			if (s != EMPTY) {
				if ((s & BLACK_FLAG) == side) {
					if (pin_level != -1)
						return;

					pin_level = l;
					pin_square = cur;
				} else {
					if (pin_level != -1 &&
					  pin_square != cur &&
					  ((s & PIECE_MASK) == piece ||
					  (s & PIECE_MASK) == QUEEN)) {
						pins[pin_level][pin_square] = 1;
					}

					return;
				}
			}
		}

		cur += delta;
	}
}

static const int bishop_dirs[] = { BCOLS-1, BCOLS+1, -BCOLS-1, -BCOLS+1 };
static const int rook_dirs[] = { 1, -1, BCOLS, -BCOLS };

static void
find_pins(unsigned char pins[BLEVELS][BAREA], const struct board_state *state,
  int side, int king_square)
{
	int i;

	memset(pins, 0, sizeof pins);

	for (i = 0; i < 4; i++) {
		find_pin_on_dir(pins, state, king_square, rook_dirs[i],
		  ROOK, side);

		find_pin_on_dir(pins, state, king_square, bishop_dirs[i],
		  BISHOP, side);
	}
}

static int
is_in_check_from(const struct board_state *state, int side, int king_square)
{
	int next;
	int i, j;
	int pawn_capture_dir[2];
	int s;
	unsigned long amask;

	amask = state->attack_board_bits;

	/* check by bishop, rook or queen? */

	for (i = 0; i < 4; i++) {
		/* check straight line */
		if (find_piece_on_dir(state, king_square, rook_dirs[i],
		  ROOK, side ^ BLACK_FLAG)) {
			return 1;
		}

		/* check diagonal */
		if (find_piece_on_dir(state, king_square, bishop_dirs[i],
		  BISHOP, side ^ BLACK_FLAG)) {
			return 1;
		}
	}

	/* check by knight */

	for (i = 0; i < sizeof knight_moves/sizeof *knight_moves; i++) {
		next = king_square + knight_moves[i];

		for (j = 0; j < BLEVELS; j++) {
			s = state->board[j][next];

			if (s == INVALID)
				break;

			if ((s & ~MOVED_FLAG) == (KNIGHT|(side ^ BLACK_FLAG)))
				return 1;
		}
	}

	/* check by pawn */

	if (side == BLACK_FLAG) {
		pawn_capture_dir[0] = BCOLS+1;
		pawn_capture_dir[1] = BCOLS-1;
	} else {
		pawn_capture_dir[0] = -BCOLS+1;
		pawn_capture_dir[1] = -BCOLS-1;
	}

	for (i = 0; i < 2; i++) {
		next = king_square + pawn_capture_dir[i];

		for (j = 0; j < BLEVELS; j++) {
			s = state->board[j][next];

			if (s == INVALID)
				break;

			if ((s & ~MOVED_FLAG) == (PAWN|(side ^ BLACK_FLAG)))
				return 1;
		}
	}

	/* check by king */

	for (i = 0; i < sizeof king_moves/sizeof *king_moves; i++) {
		next = king_square + king_moves[i];

		for (j = 0; j < BLEVELS; j++) {
			s = state->board[j][next];

			if (s == INVALID)
				break;

			if ((s & ~MOVED_FLAG) == (KING|(side ^ BLACK_FLAG)))
				return 1;
		}
	}

	return 0;
}

static int
find_king_square(const struct board_state *state, int side)
{
	int king_square;
	int i, j, s;

	/* find king */

	king_square = -1;

	for (j = 0; j < BAREA; j++) {
		for (i = 0; i < BLEVELS; i++) {
			s = state->board[i][j];

			if (s == INVALID)
				break;

			if ((s & PIECE_MASK) == KING &&
			  (s & BLACK_FLAG) == side) {
				king_square = j;
				break;
			}
		}
	}

	assert(king_square != -1);

	return king_square;
}

int
is_in_check(const struct board_state *state, int side)
{
	return is_in_check_from(state, side, find_king_square(state, side));
}
