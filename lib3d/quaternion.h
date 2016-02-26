/* quaternion.h -- part of vulcan
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

#ifndef QUATERNION_H_
#define QUATERNION_H_

struct quaternion {
	float x;
	float y;
	float z;
	float w;
};

void
quat_copy(struct quaternion *r, struct quaternion *a);

void
quat_set(struct quaternion *r, float x, float y, float z, float w);

void
quat_from_axis_and_angle(struct quaternion *r, struct vector *v, float a);

void
quat_add(struct quaternion *r, struct quaternion *a, struct quaternion *b);

void
quat_add_to(struct quaternion *a, struct quaternion *b);

void
quat_sub(struct quaternion *r, struct quaternion *a, struct quaternion *b);

void
quat_sub_from(struct quaternion *a, struct quaternion *b);

float
quat_length(struct quaternion *a);

void
quat_normalize(struct quaternion *a);

#endif /* QUATERNION_H_ */
