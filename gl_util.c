/* gl_util.c -- part of vulcan
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

#include <GL/gl.h>
#include <GL/glu.h>

#include "matrix.h"
#include "panic.h"
#include "image.h"
#include "gl_util.h"

void
mat_to_opengl(float *r, const struct matrix *m)
{
	r[0] = m->m11;  r[4] = m->m12;  r[8] = m->m13; r[12] = m->m14;
	r[1] = m->m21;  r[5] = m->m22;  r[9] = m->m23; r[13] = m->m24;
	r[2] = m->m31;  r[6] = m->m32; r[10] = m->m33; r[14] = m->m34;
	r[3] = 0.f;     r[7] = 0.f;    r[11] = 0.f;    r[15] = 1.f;
}

void
set_opengl_matrix(const struct matrix *m)
{
	static float fm[16];

	glLoadIdentity();
	mat_to_opengl(fm, m);
        glMultMatrixf(fm);
}

GLuint
image_to_opengl_texture(const struct image *img)
{
	GLuint texture_id;

	glEnable(GL_TEXTURE_2D);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &texture_id);

	if (glGetError() != GL_NO_ERROR)
		panic("glGenTextures failed");

	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height,
	  0, GL_RGBA, GL_UNSIGNED_BYTE, img->bits);

	return texture_id;
}
