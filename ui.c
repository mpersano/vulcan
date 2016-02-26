/* ui.c -- part of vulcan
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
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <sys/param.h>
#include <assert.h>

#include "version.h"
#include "panic.h"
#include "list.h"
#include "dict.h"
#include "render.h"
#include "vector.h"
#include "matrix.h"
#include "mesh.h"
#include "bsptree.h"
#include "model.h"
#include "list.h"
#include "pick.h"
#include "move.h"
#include "game.h"
#include "engine.h"
#include "font.h"
#include "font_render.h"
#include "menu.h"
#include "pathnames.h"
#include "ui_util.h"
#include "ui_state.h"
#include "ui.h"

enum {
	NUM_FONTS = 4
};

static struct {
	const char *file;
	const char *name;
	struct font_render_info **render_info_ptr;
} fonts[NUM_FONTS] = {
	{ "vera-small.fontdef", "vera-small", &ui.font_small },
	{ "vera-medium.fontdef", "vera-medium", &ui.font_medium },
	{ "vera-big.fontdef", "vera-big", &ui.font_big },
	{ "vera-mono-digits.fontdef", "vera-mono-digits", &ui.font_digits },
};

extern long msecs(void);

struct ui ui;

enum {
	MIN_CLICK_INTERVAL = 100,	/* in msecs */
	NUM_UI_STATES = 4
};

static union ui_state *ui_states[NUM_UI_STATES] = {
	(union ui_state *)&state_main_menu,
	(union ui_state *)&state_waiting_for_move,
	(union ui_state *)&state_animation,
	(union ui_state *)&state_in_game_menu,
};

extern void swap_buffers(void);

void
ui_redraw(void)
{
	unsigned clear_bits;

	glClearColor(0.f, 0.f, 0.f, 0.f);

	clear_bits = GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT;
	if (ui.do_reflections)
		clear_bits |= GL_STENCIL_BUFFER_BIT;
	glClear(clear_bits);

	ui.cur_state->common.redraw(ui.cur_state);

	swap_buffers();
}

void
ui_reshape(int width, int height)
{
	glViewport(0, 0, width, height);

	ui.cur_width = width;
	ui.cur_height = height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45., (GLdouble)width/(GLdouble)height, 1., 1000.);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static void
init_opengl_state(void)
{
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClearDepth(1.f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
}

static void
init_font(void)
{
	struct dict *font_dict;
	struct font_info *fi;
	static char cur_dir[MAXPATHLEN];
	int i;

	font_dict = dict_make();

	getcwd(cur_dir, sizeof cur_dir);

	if (chdir(FONT_DIR) != 0)
		panic("chdir to %s failed: %s", FONT_DIR, strerror(errno));

	for (i = 0; i < NUM_FONTS; i++) {
		fontdef_parse_file(font_dict, fonts[i].file);

		if ((fi = dict_get(font_dict, fonts[i].name)) == NULL)
			panic("font %s found in fontdef file %s?",
			  fonts[i].name, fonts[i].file);

		*fonts[i].render_info_ptr = font_render_info_make(fi);
	}

	chdir(cur_dir);

	dict_free(font_dict, NULL);
}

static void
init_ui_states(void)
{
	int i;

	for (i = 0; i < NUM_UI_STATES; i++)
		ui_states[i]->common.initialize(ui_states[i]);
}

void
gl_initialize(void)
{
	init_opengl_state();
	init_render();
	init_font();
	init_ui_states();
}

void
ui_set_state(enum ui_state_type state_type)
{
	assert(state_type >= 0 && state_type < NUM_UI_STATES);

	ui.cur_state = ui_states[state_type];

	ui.cur_state->common.on_state_switch(ui.cur_state);
}

void
ui_reset_game(void)
{
	init_board_state(&ui.board_state);

	memset(&ui.selected_squares, UNSELECTED_SQUARE,
	  sizeof ui.selected_squares);

	ui.computer_player_state = IDLE;
	ui.cur_side = 0;
	ui.start_position_selected = 0;
	ui.pos_highlighted = 0;

	if (ui.move_history)
		list_free(ui.move_history, free);

	ui.move_history = list_make();

	ui.ply_prev_msecs = 0;
	ui.white_player_msecs = 0;
	ui.black_player_msecs = 0;

	/* mat_make_rotation_around_z(&ui.rmatrix, -M_PI/2.); */
	mat_make_identity(&ui.rmatrix);
	/* trackball(&ui.rotation, 0, 0, 0, 0); */

	ui.rotation = .5f*M_PI;
	ui.tilt = .3f;
	ui.distance = 350.f;
}

void
ui_initialize(enum player_type white_player, enum player_type black_player,
  int max_depth, int width, int height)
{
	init_move_tables();
	init_engine();

	ui.players[0] = white_player;
	ui.players[1] = black_player;
	ui.max_depth = max_depth;

	ui.cur_width = width;
	ui.cur_height = height;

	ui.to_quit = 0;

	ui.left_button_down = 0;
	ui.right_button_down = 0;
	ui.control_key_down = 0;
	ui.last_clicked_x = -1;
	ui.last_clicked_y = -1;

	ui.do_reflections = 0;
	ui.do_textures = 0;
	ui.do_move_history = 1;

	ui_reset_game();
}

void
ui_on_worker_reply(union move *next_move)
{
	ui.cur_state->common.on_worker_reply(ui.cur_state, next_move);
}

void
ui_on_button_press(int button, int x, int y)
{		
	static long last_click_msecs = 0;
	long now;

	if (button == BTN_LEFT || button == BTN_RIGHT) {
		if (button == BTN_LEFT)
			ui.left_button_down = 1;
		else if (button == BTN_RIGHT)
			ui.right_button_down = 1;

		ui.last_clicked_x = x;
		ui.last_clicked_y = y;

		now = msecs();

		if (last_click_msecs == 0
		  || now - last_click_msecs > MIN_CLICK_INTERVAL) {
			last_click_msecs = now;

			ui.cur_state->common.on_button_press(ui.cur_state,
			  button, x, y);
		}
	}
}

void
ui_on_button_release(int button, int x, int y)
{
	static long last_click_msecs = 0;
	long now;

	if (button == BTN_LEFT)
		ui.left_button_down = 0;
	else if (button == BTN_RIGHT)
		ui.right_button_down = 0;

	now = msecs();

	if (last_click_msecs == 0 ||
	  now - last_click_msecs > MIN_CLICK_INTERVAL) {
		last_click_msecs = now;
	
		ui.cur_state->common.on_button_release(ui.cur_state, button, x,
		  y);
	}
}

void
ui_on_motion_notify(int button, int x, int y)
{
	if (ui.left_button_down || ui.right_button_down) {
		ui.cur_state->common.on_drag(ui.cur_state,
		  ui.left_button_down ? BTN_LEFT : BTN_RIGHT, x, y);
	} else {
		ui.cur_state->common.on_motion_notify(
		  ui.cur_state, button, x, y);
	}
}

void
ui_on_idle(void)
{
	ui.cur_state->common.on_idle(ui.cur_state);
}

void
ui_on_escape_press(void)
{
	ui.cur_state->common.on_escape_press(ui.cur_state);
}

void
ui_on_escape_release(void)
{
	ui.cur_state->common.on_escape_release(ui.cur_state);
}
