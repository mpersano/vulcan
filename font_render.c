/* font_render.c -- part of vulcan
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
#include <stdlib.h>
#include <stdarg.h>

#include <assert.h>

#include <GL/gl.h>
#include <png.h>

#include "font.h"
#include "panic.h"
#include "dict.h"
#include "image.h"
#include "gl_util.h"
#include "font_render.h"

struct font_render_info *
font_render_info_make(const struct font_info *info)
{
	struct font_render_info *fri;
	struct image *img;
	const struct glyph_info *g;
	GLuint texture_id;
	
	img = image_make_from_png(info->texture_filename);

	texture_id = image_to_opengl_texture(img);

	glDisable(GL_TEXTURE_2D);

	fri = calloc(1, sizeof *fri);

	fri->char_height = info->size;
	fri->texture_id = texture_id;
	fri->font_info = info;

	/* create display lists */

	float ds = 1.f/img->width;
	float dt = 1.f/img->height;

	fri->display_list_base = glGenLists(MAX_GLYPH_CODE + 1);

	for (g = info->glyphs; g != &info->glyphs[info->num_glyphs]; g++) {
		float s0, s1, t0, t1;

		assert(g->code >= 0 && g->code <= MAX_GLYPH_CODE);

		fri->glyph_table[g->code] = g;

		glNewList(fri->display_list_base + g->code, GL_COMPILE);

		glBindTexture(GL_TEXTURE_2D, fri->texture_id);

		glBegin(GL_QUADS);

		s0 = ds*g->texture_x;
		s1 = ds*(g->texture_x + (g->transposed ? g->height : g->width)); 
		t0 = dt*g->texture_y;
		t1 = dt*(g->texture_y + (g->transposed ? g->width : g->height));

		glTexCoord2f(s0, t0);
		glVertex2f(g->left, -g->top);

		if (g->transposed)
			glTexCoord2f(s0, t1);
		else
			glTexCoord2f(s1, t0);
		glVertex2f(g->left + g->width, -g->top);

		glTexCoord2f(s1, t1);
		glVertex2f(g->left + g->width, g->height - g->top);

		if (g->transposed)
			glTexCoord2f(s1, t0);
		else
			glTexCoord2f(s0, t1);
		glVertex2f(g->left, g->height - g->top);

		glEnd();

		glTranslatef(g->advance_x, 0, 0);

		glEndList();
	}

	image_free(img);

	return fri;
}

void
render_string_scaled(const struct font_render_info *fri, const char *str, int x, int y, float scale)
{
	GLint viewport[4];
	const char *p;

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_FLAT);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindTexture(GL_TEXTURE_2D, fri->texture_id);

	glGetIntegerv(GL_VIEWPORT, viewport);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.f, viewport[2], viewport[3], 0.f, -1.f, 1.f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glTranslatef(x, y, 0.f);

	glScalef(scale, scale, 1.f);

	if (str == NULL)
		str = "(nil)";

	for (p = str; *p; p++)
		glCallList(fri->display_list_base + *p);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
}

void
render_string(const struct font_render_info *fri, const char *str, int x, int y)
{
	render_string_scaled(fri, str, x, y, 1.f);
}

int
string_width_in_pixels(const struct font_render_info *fri, const char *fmt, ...)
{
	static char str[80];
	va_list ap;
	int width;
	const struct glyph_info *g;
	const char *p;

	va_start(ap, fmt);
	vsnprintf(str, sizeof str, fmt, ap);
	va_end(ap);

	width = 0;
	g = NULL;

	for (p = str; *p != '\0'; ++p) {
		g = fri->glyph_table[(int)*p];
		width += g != NULL ? g->advance_x : fri->char_height;
	}

	return width;
}
