/* font_render.h -- part of vulcan
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

#ifndef FONT_RENDER_H_
#define FONT_RENDER_H_

#include <GL/gl.h>
#include "font.h"

enum {
	MAX_GLYPH_CODE = 127
};

struct font_render_info {
  	const struct glyph_info *glyph_table[MAX_GLYPH_CODE + 1];
	const struct font_info *font_info;
  	int texture_width;
	int texture_height;
	int char_height;
	GLuint texture_id;
	GLuint display_list_base;
};

struct font_render_info *
font_render_info_make(const struct font_info *info);

void
render_string(const struct font_render_info *fri, const char *str, int x, int y);

void
render_string_scaled(const struct font_render_info *fri, const char *str, int x, int y, float scale);

int
string_width_in_pixels(const struct font_render_info *fri, const char *fmt, ...);

#endif /* FONT_RENDER_H_ */
