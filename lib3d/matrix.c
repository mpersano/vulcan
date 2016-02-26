/* matrix.c -- part of vulcan
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
#include "matrix.h"

static const struct matrix identity = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f
};

void
mat_copy(struct matrix *r, const struct matrix *m)
{
	*r = *m;
}

void
mat_make_identity(struct matrix *m)
{
	memcpy(m, &identity, sizeof *m);
}

void
mat_make_rotation_around_x(struct matrix *m, const float ang)
{
	float c, s;

	c = cos(ang);
	s = sin(ang);

	mat_make_identity(m);

	m->m22 = c; m->m23 = -s;
	m->m32 = s; m->m33 = c; 
}

void
mat_make_rotation_around_y(struct matrix *m, const float ang)
{
	float c, s;

	c = cos(ang);
	s = sin(ang);

	mat_make_identity(m);

	m->m11 = c; m->m13 = -s;
	m->m31 = s; m->m33 = c;
}

void
mat_make_rotation_around_z(struct matrix *m, const float ang)
{
	float c, s;

	c = cos(ang);
	s = sin(ang);

	mat_make_identity(m);

	m->m11 = c; m->m12 = -s;
	m->m21 = s; m->m22 = c;
}

void
mat_make_translation(struct matrix *m, const float x, const float y,
  const float z)
{
	mat_make_identity(m);

	m->m14 = x;
	m->m24 = y;
	m->m34 = z;
}

void
mat_make_translation_from_vec(struct matrix *m, const struct vector *pos)
{
	mat_make_identity(m);

	m->m14 = pos->x;
	m->m24 = pos->y;
	m->m34 = pos->z;
}

void
mat_make_scale(struct matrix *m, const float s)
{
	mat_make_identity(m);

	m->m11 = m->m22 = m->m33 = s;
}

void
mat_make_scale_xyz(struct matrix *m, const float sx, const float sy,
  const float sz)
{
	mat_make_identity(m);

	m->m11 = sx;
	m->m22 = sy;
	m->m33 = sz;
}

void
mat_mul_copy(struct matrix *r, const struct matrix *a, const struct matrix *b)
{
	float l11, l12, l13, l14;
	float l21, l22, l23, l24;
	float l31, l32, l33, l34;

	l11 = a->m11*b->m11 + a->m12*b->m21 + a->m13*b->m31;
	l12 = a->m11*b->m12 + a->m12*b->m22 + a->m13*b->m32;
	l13 = a->m11*b->m13 + a->m12*b->m23 + a->m13*b->m33;
	l14 = a->m11*b->m14 + a->m12*b->m24 + a->m13*b->m34 + a->m14;

	l21 = a->m21*b->m11 + a->m22*b->m21 + a->m23*b->m31;
	l22 = a->m21*b->m12 + a->m22*b->m22 + a->m23*b->m32;
	l23 = a->m21*b->m13 + a->m22*b->m23 + a->m23*b->m33;
	l24 = a->m21*b->m14 + a->m22*b->m24 + a->m23*b->m34 + a->m24;

	l31 = a->m31*b->m11 + a->m32*b->m21 + a->m33*b->m31;
	l32 = a->m31*b->m12 + a->m32*b->m22 + a->m33*b->m32;
	l33 = a->m31*b->m13 + a->m32*b->m23 + a->m33*b->m33;
	l34 = a->m31*b->m14 + a->m32*b->m24 + a->m33*b->m34 + a->m34;

	r->m11 = l11; r->m12 = l12; r->m13 = l13; r->m14 = l14;
	r->m21 = l21; r->m22 = l22; r->m23 = l23; r->m24 = l24;
	r->m31 = l31; r->m32 = l32; r->m33 = l33; r->m34 = l34;
}

void
mat_mul(struct matrix *a, const struct matrix *b)
{
	mat_mul_copy(a, a, b);
}

void
mat_transpose_copy(struct matrix *r, const struct matrix *m)
{
	float l11, l12, l13;
	float l21, l22, l23;
	float l31, l32, l33;

	l11 = m->m11; l12 = m->m21; l13 = m->m31; 
	l21 = m->m12; l22 = m->m22; l23 = m->m32;
	l31 = m->m13; l32 = m->m23; l33 = m->m33;

	r->m11 = l11; r->m12 = l12; r->m13 = l13; r->m14 = 0.f;
	r->m21 = l21; r->m22 = l22; r->m23 = l23; r->m24 = 0.f;
	r->m31 = l31; r->m32 = l32; r->m33 = l33; r->m34 = 0.f;
}

void
mat_transpose(struct matrix *m)
{
	mat_transpose_copy(m, m);
}

void
mat_invert_copy(struct matrix *r, const struct matrix *m)
{
	float l11, l12, l13, l14;
	float l21, l22, l23, l24;
	float l31, l32, l33, l34;
	float d;

	l11 = m->m11; l12 = m->m12; l13 = m->m13; l14 = m->m14; 
	l21 = m->m21; l22 = m->m22; l23 = m->m23; l24 = m->m24;
	l31 = m->m31; l32 = m->m32; l33 = m->m33; l34 = m->m34;

        d = 1.f / (l11*(l22*l33 - l32*l23) - (l21*(l12*l33 - l32*l13) +
	  l31*(l22*l13 - l12*l23)));

        r->m11 = d*(l22*l33 - l32*l23);
        r->m21 = d*(l31*l23 - l21*l33);
        r->m31 = d*(l21*l32 - l31*l22);

        r->m12 = d*(l32*l13 - l12*l33);
        r->m22 = d*(l11*l33 - l31*l13);
        r->m32 = d*(l31*l12 - l11*l32);

        r->m13 = d*(l12*l23 - l22*l13);
        r->m23 = d*(l21*l13 - l11*l23);
        r->m33 = d*(l11*l22 - l21*l12);

        r->m14 = d*(l22*(l13*l34 - l33*l14) + l32*(l23*l14 - l13*l24) -
	  l12*(l23*l34 - l33*l24));
        r->m24 = d*(l11*(l23*l34 - l33*l24) + l21*(l33*l14 - l13*l34) +
	  l31*(l13*l24 - l23*l14));
        r->m34 = d*(l21*(l12*l34 - l32*l14) + l31*(l22*l14 - l12*l24) -
	  l11*(l22*l34 - l32*l24));
}

void
mat_invert(struct matrix *m)
{
	mat_invert_copy(m, m);
}

void
mat_get_col(struct matrix *m, struct vector *v, const int col)
{
	switch (col) {
		case 1:
			v->x = m->m11;
			v->y = m->m21;
			v->z = m->m31;
			break;

		case 2:
			v->x = m->m12;
			v->y = m->m22;
			v->z = m->m32;
			break;

		case 3:
			v->x = m->m13;
			v->y = m->m23;
			v->z = m->m33;
			break;
	}
}

void
mat_get_row(struct matrix *m, struct vector *v, const int row)
{
	switch (row) {
		case 1:
			v->x = m->m11;
			v->y = m->m12;
			v->z = m->m13;
			break;

		case 2:
			v->x = m->m21;
			v->y = m->m22;
			v->z = m->m23;
			break;

		case 3:
			v->x = m->m31;
			v->y = m->m32;
			v->z = m->m33;
			break;
	}
}

void
mat_transform_copy(struct vector *r, const struct matrix *m,
  const struct vector *v)
{
	float x, y, z;

	x = m->m11*v->x + m->m12*v->y + m->m13*v->z + m->m14;
	y = m->m21*v->x + m->m22*v->y + m->m23*v->z + m->m24;
	z = m->m31*v->x + m->m32*v->y + m->m33*v->z + m->m34;

	r->x = x;
	r->y = y;
	r->z = z;
}

void
mat_transform(struct vector *v, const struct matrix *m)
{
	mat_transform_copy(v, m, v);
}

void
mat_rotate_copy(struct vector *r, const struct matrix *m,
  const struct vector *v)
{
	float x, y, z;

	x = m->m11*v->x + m->m12*v->y + m->m13*v->z;
	y = m->m21*v->x + m->m22*v->y + m->m23*v->z;
	z = m->m31*v->x + m->m32*v->y + m->m33*v->z;

	r->x = x;
	r->y = y;
	r->z = z;
}

void
mat_rotate(struct vector *v, const struct matrix *m)
{
	mat_rotate_copy(v, m, v);
}

void
mat_make_look_at(struct matrix *m, const struct vector *o,
  const struct vector *p)
{
	struct vector n, u, v, t;
	struct matrix a;

	vec_sub(&n, p, o);

	vec_normalize(&n);

	/* v' = (0, 1, 0) */

	vec_set(&v, 0.f, 1.f, 0.f);

	/* v = v' - (v'*n)*n */

	vec_scalar_mul_copy(&t, &n, vec_dot_product(&v, &n));
	vec_sub_from(&v, &t);
	vec_normalize(&v);

	vec_cross_product(&u, &v, &n);
	vec_normalize(&u);

	m->m11 = u.x; m->m12 = u.y; m->m13 = u.z; m->m14 = 0.f;
	m->m21 = v.x; m->m22 = v.y; m->m23 = v.z; m->m24 = 0.f;
	m->m31 = n.x; m->m32 = n.y; m->m33 = n.z; m->m34 = 0.f;

	mat_make_translation(&a, -o->x, -o->y, -o->z);

	mat_mul(m, &a);
}

void
mat_from_quaternion(struct matrix *m, struct quaternion *q)
{
        float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

        x2 = 2.*q->x;
	y2 = 2.*q->y;
        z2 = 2.*q->z;

        xx = q->x*x2;
	xy = q->x*y2;
	xz = q->x*z2;

        yy = q->y*y2;
	yz = q->y*z2;
	zz = q->z*z2;

        wx = q->w*x2;
	wy = q->w*y2;
	wz = q->w*z2;

        m->m11 = 1.f - (yy + zz);
	m->m12 = xy - wz;
        m->m13 = xz + wy;
	m->m14 = 0.f;

        m->m21 = xy + wz;
	m->m22 = 1.f - (xx + zz);
        m->m23 = yz - wx;
	m->m24 = 0.f;

        m->m31 = xz - wy;
	m->m32 = yz + wx;
        m->m33 = 1.f - (xx + yy);
	m->m34 = 0.f;
}

void
mat_print(const struct matrix *m)
{
	printf("[ %7.4f %7.4f %7.4f %7.4f ]\n", m->m11, m->m12, m->m13, m->m14);
	printf("[ %7.4f %7.4f %7.4f %7.4f ]\n", m->m21, m->m22, m->m23, m->m24);
	printf("[ %7.4f %7.4f %7.4f %7.4f ]\n", m->m31, m->m32, m->m33, m->m34);
	printf("[ %7.4f %7.4f %7.4f %7.4f ]\n", 0.f, 0.f, 0.f, 1.f);
}
