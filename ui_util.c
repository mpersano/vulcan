/* ui_util.c -- part of vulcan
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
#include <math.h>

#include "render.h"
#include "ui_util.h"
#include "font_render.h"
#include "list.h"
#include "matrix.h"

void
ui_set_modelview_matrix(struct matrix *mv)
{
	struct matrix ry, rx, rm;
	struct vector p, o;
	struct matrix c, t, r, v;

	mat_make_rotation_around_y(&ry, ui.rotation);
	mat_make_rotation_around_x(&rx, ui.tilt);
	mat_mul_copy(&rm, &ry, &rx);

	vec_zero(&o); /* look at */

	vec_set(&p, 0.f, 0.f, -ui.distance); /* camera position */
	mat_rotate(&p, &rm);

	mat_make_look_at(&c, &o, &p);

	vec_neg(&p);
	mat_make_translation_from_vec(&t, &p);
	mat_mul_copy(&v, &c, &t);

	mat_make_rotation_around_x(&r, -M_PI/2.);

	mat_mul_copy(mv, &v, &r);
}

void
ui_render_board_state(void)
{
	struct matrix mv;

	ui_set_modelview_matrix(&mv);

	render_board_state(&ui.board_state, &ui.selected_squares,
	  &mv);
}

enum {
	MOVE_TABLE_ROWS = 6,
	MOVE_HISTORY_BORDER = 4
};

static void
format_time(char *str, long msecs)
{
	sprintf(str, "%02ld:%02ld", msecs/1000/60, msecs/1000%60);
}

void
ui_render_clock(void)
{
	const struct font_render_info *font_digits = ui.font_digits;
	const struct font_render_info *font_medium = ui.font_small;
	static char white_time[20], black_time[20];
	static char white_title[120], black_title[120];

	glColor4f(1.f, 1.f, 1.f, 1.f);

	sprintf(white_title, "White Player - %s",
	  ui.players[0] == HUMAN_PLAYER ? "Human" : "Computer");
	sprintf(black_title, "Black Player - %s",
	  ui.players[1] == HUMAN_PLAYER ? "Human" : "Computer");

	format_time(white_time, ui.white_player_msecs);
	format_time(black_time, ui.black_player_msecs);

	render_string(font_medium, white_title, 2, 30);
	render_string(font_digits, white_time, 2, 60);

	render_string(font_medium, black_title, ui.cur_width - 6 -
	  string_width_in_pixels(font_medium, black_title), 30);
	render_string(font_digits, black_time, ui.cur_width - 4 -
	  string_width_in_pixels(font_digits, "00:00"), 60);
}

void
ui_render_move_history(void)
{
	const struct font_render_info *const font = ui.font_small;
	const struct list *const move_history = ui.move_history;
	static int cur_move, i, y, x, x0, y0, x1, y1;
	char str[20];
	static int row_width_0, row_width_1;

	glColor4f(1.f, 1.f, 1.f, 1.f);

	row_width_0 = string_width_in_pixels(font, "XXX");
	row_width_1 = string_width_in_pixels(font, "XXXXXXXXX");

	x = ui.cur_width - (row_width_0 + 2*row_width_1 +
	  3*MOVE_HISTORY_BORDER);
	y = ui.cur_height - (MOVE_TABLE_ROWS + 1)*
	  (font->char_height + 2);

	/* render background */

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_FLAT);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.f, ui.cur_width, ui.cur_height, 0.f, -1.f,
	  1.f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor4f(.4f, .4f, .4f, .5f);

	x0 = x - MOVE_HISTORY_BORDER;
	x1 = x + row_width_0 + 2*row_width_1 + MOVE_HISTORY_BORDER;

	/* HACK */
	y0 = y - font->char_height;
	y1 = y + font->char_height*MOVE_TABLE_ROWS + 2*MOVE_HISTORY_BORDER;

	glBegin(GL_QUADS);
	glVertex2f(x0, y0);
	glVertex2f(x1, y0);
	glVertex2f(x1, y1);
	glVertex2f(x0, y1);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();

	/* render text */

	cur_move = (ui.board_state.cur_ply - 1)/2;

	i = cur_move < MOVE_TABLE_ROWS - 1 ? 0 :
	  cur_move - (MOVE_TABLE_ROWS - 1);

	for (; i <= cur_move; i++) {
		sprintf(str, "%d.", i + 1);
		render_string(font, str, x, y);

		if (i*2 < ui.board_state.cur_ply)
			render_string(font,
			  (char *)list_element_at(move_history, i*2),
			  x + row_width_0, y);

		if (i*2 + 1 < ui.board_state.cur_ply)
			render_string(font,
			  (char *)list_element_at(move_history, i*2 + 1),
			  x + row_width_0 + row_width_1, y);

		y += font->char_height + 2;
	}
}

void
ui_render_text(void)
{
	ui_render_clock();

	if (ui.do_move_history)
		ui_render_move_history();
}
