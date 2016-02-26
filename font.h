/* font.h -- part of vulcan
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

#ifndef FONT_H_
#define FONT_H_

struct glyph_info {
	int code;
	int width;
	int height;
	int left;
	int top;
	int advance_x;
	int advance_y;
	int texture_x;
	int texture_y;
	int transposed;
};

struct font_info {
	const char *texture_filename;
	int size;
	int num_glyphs;
	struct glyph_info *glyphs;
};

struct dict;

void
fontdef_parse_file(struct dict *font_dict, const char *filename);

#endif /* FONT_H_ */
