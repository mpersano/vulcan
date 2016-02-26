/* engine.h -- part of vulcan
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

#ifndef ENGINE_H_
#define ENGINE_H_

void
get_best_move(union move *move, struct board_state *state, int side,
  int max_depth);

void
init_engine(void);

#endif /* ENGINE_H_ */
