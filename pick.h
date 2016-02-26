/* pick.h -- part of vulcan
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

#ifndef PICK_H_
#define PICK_H_

struct matrix;
struct board_state;
struct position;

int
pick_square(struct position *pos, const struct board_state *board,
  const struct matrix *mv, int x, int y);

#endif /* PICK_H_ */
