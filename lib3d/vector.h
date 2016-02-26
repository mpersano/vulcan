/* vector.h -- part of vulcan
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

#ifndef VECTOR_H_
#define VECTOR_H_

struct vector {
	float x, y, z;
};

void
vec_copy(struct vector *r, const struct vector *a);

void
vec_set(struct vector *r, float x, float y, float z);

void
vec_zero(struct vector *r);

void
vec_neg(struct vector *r);

void
vec_neg_copy(struct vector *r, const struct vector *a);

void
vec_add(struct vector *r, const struct vector *a, const struct vector *b);

void
vec_add_to(struct vector *a, const struct vector *b);

void
vec_sub(struct vector *r, const struct vector *a, const struct vector *b);

void
vec_sub_from(struct vector *a, const struct vector *b);

float
vec_dot_product(const struct vector *a, const struct vector *b);

float
vec_length_squared(const struct vector *a);

float
vec_length(const struct vector *a);

void
vec_cross_product(struct vector *r, const struct vector *a,
  const struct vector *b);

void
vec_scalar_mul(struct vector *a, const float s);

void
vec_scalar_mul_copy(struct vector *r, const struct vector *a, const float s);

void
vec_normalize(struct vector *a);

void
vec_print(const struct vector *v);

#endif /* VECTOR_H_ */
