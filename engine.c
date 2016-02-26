/* engine.c -- part of vulcan
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
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "move.h"
#include "game.h"
#include "engine.h"

enum {
	INFINITY = 1000000
};

static int square_scores[] = {
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 0, 0, -1, -1, 0, 0, -1,
  -1, 0, 0, 0, 0, 0, 0, -1,
  -1, 0, 0, 1, 1, 0, 0, -1,
  -1, 0, 1, 2, 2, 1, 0, -1,
  -1, 0, 1, 3, 3, 1, 0, -1,
  -1, 0, 1, 3, 3, 1, 0, -1,
  -1, 0, 1, 2, 2, 1, 0, -1,
  -1, 0, 0, 1, 1, 0, 0, -1,
  -1, 0, 0, 0, 0, 0, 0, -1,
  -1, 0, 0, -1, -1, 0, 0, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
};

static int
get_score_for_board(const struct board_state *state, struct position *first,
  int size)
{
	int i, j, s;
	int square, score;
	const unsigned char *board;

	score = 0;

	board = state->board[first->level];
	square = first->square;

	for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
			s = board[square];

			if (s != EMPTY) {
				if (s & BLACK_FLAG)
					score -= square_scores[square];
				else
					score += square_scores[square];
			}

			++square;
		}

		square += BCOLS - size;
	}

	return score;
}

static int
get_score(const struct board_state *state, int side)
{
	int i, j;
	struct position first;
	int score;

	score = state->material_imbalance;

	for (i = 0; i < NUM_MAIN_BOARDS; i++) {
		get_main_board_position(&first, i);

		score += get_score_for_board(state, &first, MAIN_BOARD_SIZE);

		for (j = 0; j < 8; j++) {
			if (ATTACK_BOARD_IS_ACTIVE(state, i, j)) {
				get_attack_board_position(&first, i, j);

				score += get_score_for_board(state, &first,
				  ATTACK_BOARD_SIZE);
			}
		}
	}

	return score*(side == BLACK_FLAG ? -1 : 1);
}

static void
rank_moves(int *rank, const struct board_state *state, union move *moves,
  int num_moves)
{
	int i;
	const union move *end;

	end = &moves[num_moves];

	for (i = 0; i < num_moves; i++) {
		const union move *p = &moves[i];

		rank[i] = 0;

		if (p->type == PIECE_MOVE) {
			const struct position *to = &p->piece_move.to;
			const int s = state->board[to->level][to->square];

			if (s != EMPTY)
				rank[i] = piece_values[(s & PIECE_MASK) - PAWN];
		}
	}
}

static void
best_move_first(int *rank, union move *moves, int num_moves)
{
	int best_rank, best_move;
	int i;

	best_rank = rank[0];
	best_move = 0;

	for (i = 1; i < num_moves; i++)
		if (rank[i] > best_rank)
			best_move = i;

	if (best_move > 0) {
		union move tm;
		int tr;
		
		tm = moves[0];
		moves[0] = moves[best_move];
		moves[best_move] = tm;

		tr = rank[0];
		rank[0] = rank[best_move];
		rank[best_move] = tr;
	}
}

static int
get_best_move_r(union move *move, struct board_state *state, int side,
  int depth, int alpha, int beta)
{
	union move moves[MAX_MOVES];
	int move_rank[MAX_MOVES], *r;
	union move *best_move;
	union move *p;
	const union move *end;
	int n, score, best_score;
	union move next_move;
	struct undo_move_info undo_info;

	best_score = -INFINITY;
	best_move = NULL;

	n = get_legal_moves(moves, state, side);

	rank_moves(move_rank, state, moves, n);

	r = move_rank;
	end = &moves[n];

	for (p = moves; p != end; p++) {
		best_move_first(r, p, n);

		do_move(state, p, &undo_info);

		if (depth == 0) {
			score = get_score(state, side);
		} else {
			score = -get_best_move_r(&next_move,
			  state, side^BLACK_FLAG, depth - 1,
			  -beta, -alpha);
		}

		undo_move(state, &undo_info);

		if (score > best_score) {
			best_score = score;
			best_move = p;
		
			if (best_score > alpha)
				alpha = best_score;

			if (best_score >= beta)
				break;
		}

		n--;
		r++;
	}

	if (best_move != NULL)
		*move = *best_move;

	return best_score;
}

void
get_best_move(union move *move, struct board_state *state, int side,
  int max_depth)
{
	int score;

	score = get_best_move_r(move, state, side, max_depth, -INFINITY,
	  INFINITY);

	assert(score > INT_MIN);
	fprintf(stderr, "score: %d\n", score);
}

void
init_engine(void)
{
}
