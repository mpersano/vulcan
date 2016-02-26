/* matrix.h -- part of vulcan
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

#ifndef MATRIX_H_
#define MATRIX_H_

struct vector;
struct quaternion;

struct matrix {
	float m11, m12, m13, m14;
	float m21, m22, m23, m24;
	float m31, m32, m33, m34;
};

void
mat_copy(struct matrix *r, const struct matrix *m);

void
mat_make_identity(struct matrix *m);

void
mat_make_rotation_around_x(struct matrix *m, const float ang);

void
mat_make_rotation_around_y(struct matrix *m, const float ang);

void
mat_make_rotation_around_z(struct matrix *m, const float ang);

void
mat_make_translation(struct matrix *m, const float x, const float y,
  const float z);

void
mat_make_translation_from_vec(struct matrix *m, const struct vector *pos);

void
mat_make_scale(struct matrix *m, const float s);

void
mat_make_scale_xyz(struct matrix *m, const float sx, const float sy,
  const float sz);

void
mat_mul(struct matrix *a, const struct matrix *b);

void
mat_mul_copy(struct matrix *r, const struct matrix *a, const struct matrix *b);

void
mat_invert(struct matrix *m);

void
mat_invert_copy(struct matrix *r, const struct matrix *m);

void
mat_transpose(struct matrix *m);

void
mat_transpose_copy(struct matrix *r, const struct matrix *m);

void
mat_get_col(struct matrix *m, struct vector *v, const int col);

void
mat_get_row(struct matrix *m, struct vector *v, const int col);

void
mat_transform(struct vector *v, const struct matrix *m);

void
mat_transform_copy(struct vector *r, const struct matrix *m,
  const struct vector *v);

void
mat_rotate(struct vector *v, const struct matrix *m);

void
mat_rotate_copy(struct vector *r, const struct matrix *m,
  const struct vector *v);

void
mat_make_look_at(struct matrix *m, const struct vector *o,
  const struct vector *p);

void
mat_from_quaternion(struct matrix *m, struct quaternion *q);

void
mat_print(const struct matrix *m);

#endif /* MATRIX_H_ */
