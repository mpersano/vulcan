/* quaternion.c -- part of vulcan
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
#include "quaternion.h"

void
quat_copy(struct quaternion *r, struct quaternion *a)
{
	memcpy(r, a, sizeof *r);
}

void
quat_set(struct quaternion *r, float x, float y, float z, float w)
{
	r->x = x;
	r->y = y;
	r->z = z;
	r->w = w;
}

void
quat_from_axis_and_angle(struct quaternion *r, struct vector *v, float a)
{
	float s, c;
	struct vector axis;

	s = sin(a*.5);
	c = cos(a*.5);

	vec_copy(&axis, v);
	vec_normalize(&axis);

	r->x = axis.x*s;
	r->y = axis.y*s;
	r->z = axis.z*s;
	r->w = c;
}

void
quat_add(struct quaternion *r, struct quaternion *a, struct quaternion *b)
{
	r->x = a->x + b->x;
	r->y = a->y + b->y;
	r->z = a->z + b->z;
	r->w = a->w + b->w;
}

void
quat_add_to(struct quaternion *a, struct quaternion *b)
{
	a->x += b->x;
	a->y += b->y;
	a->z += b->z;
	a->w += b->w;
}

void
quat_sub(struct quaternion *r, struct quaternion *a, struct quaternion *b)
{
	r->x = a->x - b->x;
	r->y = a->y - b->y;
	r->z = a->z - b->z;
	r->w = a->w - b->w;
}

void
quat_sub_from(struct quaternion *a, struct quaternion *b)
{
	a->x -= b->x;
	a->y -= b->y;
	a->z -= b->z;
	a->w -= b->w;
}

float
quat_length(struct quaternion *a)
{
	return sqrt(a->x*a->x + a->y*a->y + a->z*a->z + a->w*a->w);
}

void
quat_normalize(struct quaternion *a)
{
	float l;
	
	l = quat_length(a);

	if (l == 0.f) {
		a->x = 1.f;
		a->y = a->z = a->w = 0.f;
	} else {
		float il = 1.f/l;

		a->x *= il;
		a->y *= il;
		a->z *= il;
		a->w *= il;
	}
}
