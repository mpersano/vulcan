/* graphics.h -- part of vulcan
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

#ifndef GRAPHICS_H_
#define GRAPHICS_H_

struct vector;
struct position;

void
get_position_xyz(struct vector *p, const struct position *pos);

void
get_main_board_xyz(struct vector *p, int main_board);

void
get_attack_board_xyz(struct vector *p, int main_board, int attack_board);

#endif /* GRAPHICS_H_ */
