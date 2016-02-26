/* graphics.c -- part of vulcan
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

#include "vector.h"
#include "game.h"
#include "graphics.h"

void
get_position_xyz(struct vector *p, const struct position *pos)
{
	int i, j, br, bc, r, c;
	struct position bp;

	vec_zero(p);

	r = pos->square/BCOLS;
	c = pos->square%BCOLS;

	for (i = NUM_MAIN_BOARDS - 1; i >= 0; i--) {
		get_main_board_position(&bp, i);

		if (pos->level == bp.level) {
			br = bp.square/BCOLS;
			bc = bp.square%BCOLS;

			if (r >= br && r - br < MAIN_BOARD_SIZE &&
			  c >= bc && c - bc < MAIN_BOARD_SIZE) {
				struct vector board_center;

				get_main_board_xyz(&board_center, i);

				p->x = board_center.x
				  - .5f*MAIN_BOARD_SIZE*SQUARE_SIZE
				  + (r - br + .5f)*SQUARE_SIZE;
				p->y = board_center.y
				  - .5f*MAIN_BOARD_SIZE*SQUARE_SIZE
				  + (c - bc + .5f)*SQUARE_SIZE;
				p->z = board_center.z;

				break;
			}
		}

		for (j = 0; j < 8; j++) {
			get_attack_board_position(&bp, i, j);

			if (pos->level == bp.level) {
				br = bp.square/BCOLS;
				bc = bp.square%BCOLS;

				if (r >= br
				  && r - br < ATTACK_BOARD_SIZE
				  && c >= bc
				  && c - bc < ATTACK_BOARD_SIZE) {
					struct vector board_center;

					get_attack_board_xyz(&board_center, i,
					  j);

					p->x = board_center.x
					  - .5f*ATTACK_BOARD_SIZE*SQUARE_SIZE
					  + (r - br + .5f)*SQUARE_SIZE;

					p->y = board_center.y
					  - .5f*ATTACK_BOARD_SIZE*SQUARE_SIZE
					  + (c - bc + .5f)*SQUARE_SIZE;

					p->z = board_center.z;

					goto done;
				}
			}
		}
	}
  done:
  	;
}

/*
 * get_main_board_xyz --
 *	Fill in a vector with the position of the center of a main board,
 *	in world coordinates.
 */
void
get_main_board_xyz(struct vector *p, int main_board)
{
	vec_set(p,
	  -2.f*SQUARE_SIZE + main_board*2.f*SQUARE_SIZE,
	  0.f,
	  MAIN_BOARD_DISTANCE - main_board*MAIN_BOARD_DISTANCE);
}

/*
 * get_attack_board_xyz --
 *	Fill in a vector with the position of the center of an attack board,
 *	in world coordinates.
 */
void
get_attack_board_xyz(struct vector *p, int main_board, int attack_board)
{
	float x, y, z;

	if (ATTACK_BOARD_IS_UP(attack_board))
		x = -2.f*SQUARE_SIZE;
	else
		x = 2.f*SQUARE_SIZE;

	if (ATTACK_BOARD_IS_RIGHT(attack_board))
		y = 2.f*SQUARE_SIZE;
	else
		y = -2.f*SQUARE_SIZE;

	if (ATTACK_BOARD_IS_ABOVE(attack_board))
		z = ATTACK_BOARD_DISTANCE;
	else
		z = -ATTACK_BOARD_DISTANCE;

	z += MAIN_BOARD_DISTANCE - MAIN_BOARD_DISTANCE*main_board;

	x -= 2.f*SQUARE_SIZE;
	x += 2.f*SQUARE_SIZE*main_board;

	vec_set(p, x, y, z);
}
