/* plane.c -- part of vulcan
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
#include <stdio.h>
#include "vector.h"
#include "bsptree.h"
#include "plane.h"

#define RP_EPSILON 1e-2

enum rpos
plane_classify_point(const struct plane *p, const struct vector *a)
{
	struct vector d;
	float s;

	vec_sub(&d, a, &p->pt);
	s = vec_dot_product(&d, &p->normal);

	if (s > RP_EPSILON)
		return RP_FRONT;
	else if (s < -RP_EPSILON)
		return RP_BACK;
	else
		return RP_ON;
}

float
plane_point_distance(const struct plane *p, const struct vector *a)
{
	struct vector d;

	vec_sub(&d, a, &p->pt);

	return vec_dot_product(&d, &p->normal);
}

void
plane_intersect(struct vector *r, const struct plane *p,
  const struct vector *a, const struct vector *b)
{
	float s;
	struct vector u, v;

	vec_sub(&u, &p->pt, a);
	vec_sub(&v, b, a);

	assert(vec_length(&v) > RP_EPSILON);

	s = vec_dot_product(&u, &p->normal)/vec_dot_product(&v, &p->normal);

	vec_scalar_mul_copy(r, &v, s);
	vec_add_to(r, a);
}
