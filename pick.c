/* pick.c -- part of vulcan
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
#include <assert.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "game.h"
#include "vector.h"
#include "matrix.h"
#include "ray.h"
#include "mesh.h"
#include "model.h"
#include "bsptree.h"
#include "graphics.h"
#include "collision.h"
#include "pick.h"

extern struct model *piece_models[NUM_PIECES];

struct hit {
	float t;	/* distance along ray */
	int row;	/* row of square that was hit */
	int col;	/* col of square that was hit */
};

static void
get_pick_ray(struct ray *ray, int x, int y)
{
	GLdouble modelview[16], projection[16];
	GLint viewport[4];
	GLdouble from_x, from_y, from_z;
	GLdouble to_x, to_y, to_z;
	GLdouble win_x, win_y;

	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	win_x = x;
	win_y = viewport[3] - y - 1;

	gluUnProject(win_x, win_y, 0., modelview, projection, viewport,
	  &from_x, &from_y, &from_z);

	ray->from.x = from_x;
	ray->from.y = from_y;
	ray->from.z = from_z;

	gluUnProject(win_x, win_y, 1., modelview, projection, viewport,
	  &to_x, &to_y, &to_z);

	ray->to.x = to_x;
	ray->to.y = to_y;
	ray->to.z = to_z;
}

static int
intersect_board_squares(struct hit *hit, const struct ray *ray,
  const struct vector *board_pos, const unsigned char *state,
  int size)
{
	float t;
	float start_x, start_y, size_x, size_y;
	struct vector dir, hit_pos;

	/* find hit position */

	vec_sub(&dir, &ray->to, &ray->from);

	if (dir.z == 0.f)
		return 0;

	t = (board_pos->z - ray->from.z)/dir.z;

	if (t < 0.f)
		return 0;

	vec_scalar_mul_copy(&hit_pos, &dir, t);
	vec_add_to(&hit_pos, &ray->from);

	/* figure out square position */

	size_x = SQUARE_SIZE*size;
	size_y = SQUARE_SIZE*size;

	start_x = board_pos->x - .5f*size_x;
	start_y = board_pos->y - .5f*size_y;

	if (hit_pos.x < start_x || hit_pos.x > start_x + size_x)
		return 0;

	if (hit_pos.y < start_y || hit_pos.y > start_y + size_y)
		return 0;

	hit->t = t;
	hit->row = (hit_pos.x - start_x)/SQUARE_SIZE;
	hit->col = (hit_pos.y - start_y)/SQUARE_SIZE;

	return 1;
}


static int
intersect_board_pieces(struct hit *hit, const struct ray *ray,
  const struct vector *board_pos, const unsigned char *state, int size)
{
	int i, j, p, first_hit;
	float x, y, z, start_x, start_y;
	const unsigned char *ps;

	first_hit = 1;

	start_x = board_pos->x - .5f*SQUARE_SIZE*(size - 1);
	start_y = board_pos->y - .5f*SQUARE_SIZE*(size - 1);
	z = board_pos->z;

	x = start_x;
	ps = state;

	for (i = 0; i < size; i++) {
		y = start_y;

		for (j = 0; j < size; j++) {
			p = *ps++;

			if (p) {
				struct matrix m, inv_m;
				struct model *model;
				struct ray ray_model;
				float hit_t = -1.f;

				mat_make_translation(&m, x, y, z);

				if (p & BLACK_FLAG) {
					struct matrix r;
					mat_make_rotation_around_z(&r, M_PI);
					mat_mul_copy(&m, &m, &r);
				}

				model = piece_models[(p & PIECE_MASK) - PAWN];

				mat_invert_copy(&inv_m, &m);
				mat_transform_ray(&ray_model, &inv_m, ray);

				if (ray_model_find_collision(&hit_t,
				  &ray_model, model)) {
					if (first_hit || hit_t < hit->t) {
						hit->t = hit_t;
						hit->row = i;
						hit->col = j;

						first_hit = 0;
					}
				}
			}

			y += SQUARE_SIZE;
		}

		ps += BCOLS - size;
		x += SQUARE_SIZE;
	}

	return first_hit == 0;
}


static int
intersect_board(struct hit *hit, const struct ray *ray,
  const struct vector *board_pos, const unsigned char *state,
  int size)
{
	struct hit hit1 = {0}, hit2 = {0};
	int r1, r2;

	r1 = intersect_board_squares(&hit1, ray, board_pos, state, size);
	r2 = intersect_board_pieces(&hit2, ray, board_pos, state, size);

	if (r1 && (!r2 || hit1.t <= hit2.t)) {
		*hit = hit1;
		return 1;
	} else if (r2 && (!r1 || hit2.t <= hit1.t)) {
		*hit = hit2;
		return 1;
	} else {
		return 0;
	}
}


int
pick_square(struct position *pos, const struct board_state *state,
  const struct matrix *mv, int x, int y)
{
	struct ray ray_orig;	/* pick ray in world coordinates */
	struct ray ray_obj;	/* pick ray in object coordinates */
	struct matrix inv_mv;
	struct vector p;
	int i, j;
	struct position first_pos;
	const unsigned char *ps;
	struct hit hit;
	int no_hits, hit_level, hit_square;
	float closer_t;

	/* ray in world coordinates */

	get_pick_ray(&ray_orig, x, y);

	/* transform ray to game coordinates */

	mat_invert_copy(&inv_mv, mv);
	mat_transform_ray(&ray_obj, &inv_mv, &ray_orig);

	no_hits = 1;
	hit_level = hit_square = -1;
	closer_t = -1.f;

	for (i = 0; i < NUM_MAIN_BOARDS; i++) {
		/* intersection with main board */

		get_main_board_xyz(&p, i);
		get_main_board_position(&first_pos, i);

		ps = &state->board[first_pos.level][first_pos.square];

		if (intersect_board(&hit, &ray_obj, &p, ps, MAIN_BOARD_SIZE)) {
			if (no_hits || hit.t < closer_t) {
				closer_t = hit.t;
				hit_level = first_pos.level;
				hit_square = first_pos.square +
				  hit.row*BCOLS + hit.col;
				no_hits = 0;
			}
		}

		for (j = 0; j < 8; j++) {
			if (ATTACK_BOARD_IS_ACTIVE(state, i, j)) {
				/* intersection with attack board */

				hit.t = -1.f;

				get_attack_board_xyz(&p, i, j);
				get_attack_board_position(&first_pos, i, j);

				ps = &state->board
				  [first_pos.level][first_pos.square];

				if (intersect_board(&hit, &ray_obj, &p, ps,
				  ATTACK_BOARD_SIZE)) {
				  	if (no_hits || hit.t < closer_t) {
						closer_t = hit.t;
						hit_level = first_pos.level;
						hit_square = first_pos.square +
						  hit.row*BCOLS + hit.col;
						no_hits = 0;
					}
				}
			}
		}
	}

	if (no_hits)
		return 0;

	pos->level = hit_level;	
	pos->square = hit_square;

	return 1;
}
