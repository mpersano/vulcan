/* gl_util.h -- part of vulcan
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

#ifndef GL_UTIL_H_
#define GL_UTIL_H_

#include <GL/gl.h>

struct matrix;
struct image;

void
mat_to_opengl(float *r, const struct matrix *m);

void
set_opengl_matrix(const struct matrix *m);

GLuint
image_to_opengl_texture(const struct image *img);

#endif /* GL_UTIL_H_ */
