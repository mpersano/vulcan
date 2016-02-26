/* render.h -- part of vulcan
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

#ifndef RENDER_H_
#define RENDER_H_

struct matrix;
struct vector;
struct board_state;
struct selected_squares;

void
init_render(void);

void
render_piece_at(const struct matrix *mv, const struct vector *pos, int piece,
  int flags, int reflected);

void
render_board_state(const struct board_state *state,
  const struct selected_squares *flags, const struct matrix *mv);

#endif /* RENDER_H_ */
