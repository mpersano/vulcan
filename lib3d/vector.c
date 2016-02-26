/* vector.c -- part of vulcan
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
#include <string.h>
#include <math.h>
#include "vector.h"

void
vec_copy(struct vector *r, const struct vector *a)
{
	*r = *a;
}

void
vec_set(struct vector *r, float x, float y, float z)
{
	r->x = x;
	r->y = y;
	r->z = z;
}

void
vec_zero(struct vector *r)
{
	r->x = r->y = r->z = 0.f;
}

void
vec_neg_copy(struct vector *r, const struct vector *a)
{
	r->x = -a->x;
	r->y = -a->y;
	r->z = -a->z;
}

void
vec_neg(struct vector *r)
{
	vec_neg_copy(r, r);
}

void
vec_add(struct vector *r, const struct vector *a, const struct vector *b)
{
	r->x = a->x + b->x;
	r->y = a->y + b->y;
	r->z = a->z + b->z;
}

void
vec_add_to(struct vector *a, const struct vector *b)
{
	a->x += b->x;
	a->y += b->y;
	a->z += b->z;
}

void
vec_sub(struct vector *r, const struct vector *a, const struct vector *b)
{
	r->x = a->x - b->x;
	r->y = a->y - b->y;
	r->z = a->z - b->z;
}

void
vec_sub_from(struct vector *a, const struct vector *b)
{
	a->x -= b->x;
	a->y -= b->y;
	a->z -= b->z;
}

float
vec_dot_product(const struct vector *a, const struct vector *b)
{
	return a->x*b->x + a->y*b->y + a->z*b->z;
}

float
vec_length_squared(const struct vector *a)
{
	return vec_dot_product(a, a);
}

float
vec_length(const struct vector *a)
{
	return sqrt(vec_length_squared(a));
}

void
vec_cross_product(struct vector *r, const struct vector *a,
  const struct vector *b)
{
	r->x = a->y*b->z - a->z*b->y;
	r->y = a->z*b->x - a->x*b->z;
	r->z = a->x*b->y - a->y*b->x;
}

void
vec_scalar_mul(struct vector *a, const float s)
{
	a->x *= s;
	a->y *= s;
	a->z *= s;
}

void
vec_scalar_mul_copy(struct vector *r, const struct vector *a, const float s)
{
	r->x = a->x*s;
	r->y = a->y*s;
	r->z = a->z*s;
}

void
vec_normalize(struct vector *a)
{
	float l;

	l = vec_length(a);

	if (l == 0.f)
		a->x = 1.f, a->y = 0.f, a->z = 0.f;
	else
		vec_scalar_mul(a, 1.f/l);
}

void
vec_print(const struct vector *v)
{
	printf("[ %7.4f %7.4f %7.4f ]\n", v->x, v->y, v->z);
}
