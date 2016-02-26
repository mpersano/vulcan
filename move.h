/* move.h -- part of vulcan
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

#ifndef MOVE_H_
#define MOVE_H_

#define MAX_MOVES 100

struct position;
union move;
struct board_state;

void
init_move_tables(void);

int
get_legal_moves_for_position(union move *moves,
  const struct position *from_pos, struct board_state *state, int side);

int
get_legal_moves(union move *moves, struct board_state *state, int side);

int
is_in_check(const struct board_state *state, int side);

#endif /* MOVE_H_ */
