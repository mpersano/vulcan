/* ui_main_menu.c -- part of vulcan
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
#include <assert.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "version.h"
#include "ui.h"
#include "menu.h"
#include "font.h"
#include "font_render.h"
#include "ui_util.h"
#include "ui_options_menu.h"
#include "ui_state.h"

extern unsigned long msecs(void);

static const float rotation_speed = .0002f;

/* ui_state */

static void
on_idle(union ui_state *ui_state)
{
	struct state_main_menu *state = &ui_state->main_menu;
	unsigned long dt = msecs() - state->start_t;

	ui.rotation = .5f*M_PI + dt*rotation_speed;
}

static void
on_worker_reply(union ui_state *state, union move *move)
{
	assert("worker reply on main menu?" && 0);
}

static void
on_motion_notify(union ui_state *state, int button, int x, int y)
{
	menu_on_mouse_motion(state->main_menu.menu, x, y);
}

static void
on_drag(union ui_state *state, int button, int x, int y)
{
	on_motion_notify(state, button, x, y);
}

static void
on_button_press(union ui_state *state, int button, int x, int y)
{
	menu_on_click(state->main_menu.menu, x, y);
}

static void
on_button_release(union ui_state *state, int button, int x, int y)
{
}

static void
on_escape_press(union ui_state *state)
{
}

static void
on_escape_release(union ui_state *state)
{
}

static void
redraw(union ui_state *state)
{
	int x, y;

	ui_render_board_state();

	menu_render(state->main_menu.menu);

	x = (ui.cur_width - string_width_in_pixels(ui.font_big, VERSION))/2;
	y = 2*ui.font_big->char_height;

	glColor4f(1.f, .5f, .5f, 1.f);

	render_string(ui.font_big, VERSION, x, y);
}

/* menu actions */

static void
menu_toggle_white_player_type(void *extra)
{
	ui.players[0] =
	  ui.players[0] == HUMAN_PLAYER ?  COMPUTER_PLAYER : HUMAN_PLAYER;
}

static void
menu_toggle_black_player_type(void *extra)
{
	ui.players[1] =
	  ui.players[1] == HUMAN_PLAYER ? COMPUTER_PLAYER : HUMAN_PLAYER;
}

static void
menu_toggle_max_search_depth(void *extra)
{
	ui.max_depth = 3 + (ui.max_depth - 3 + 1)%3;
}

static const char *
menu_get_white_player_type(void *extra)
{
	return ui.players[0] == HUMAN_PLAYER ? "Human" : "Computer";
}

static const char *
menu_get_black_player_type(void *extra)
{
	return ui.players[1] == HUMAN_PLAYER ? "Human" : "Computer";
}

static const char *
menu_get_max_search_depth(void *extra)
{
	static char depth_str[20];

	sprintf(depth_str, "%d", ui.max_depth);

	return depth_str;
}

#if 0
static void
menu_about(void *extra)
{
	printf("about\n");
}
#endif

static void
menu_quit(void *extra)
{
	/* printf("quit\n"); */
	ui.to_quit = 1;
}

static void
menu_play(union ui_state *state)
{
	struct state_main_menu *state_main_menu = &state->main_menu;

	menu_on_hide(state_main_menu->menu);

	ui_reset_game();

	ui_set_state(STATE_WAITING_FOR_MOVE);
}

static void
initialize(union ui_state *state)
{
	struct menu *main_menu;

	main_menu = menu_make("main", ui.font_medium);

	menu_add_action_item(main_menu, "Play",
	  (void(*)(void *))menu_play, state);

	menu_add_toggle_item(main_menu, "White player:", 
	  menu_toggle_white_player_type, NULL,
	  menu_get_white_player_type, NULL);

	menu_add_toggle_item(main_menu, "Black player:",
	  menu_toggle_black_player_type, NULL,
	  menu_get_black_player_type, NULL);

	menu_add_toggle_item(main_menu, "Search depth:",
	  menu_toggle_max_search_depth, NULL,
	  menu_get_max_search_depth, NULL);

	add_options_menu(main_menu);

	menu_add_action_item(main_menu, "Quit", menu_quit, NULL);

	state->main_menu.menu = main_menu;
}

static void
on_state_switch(union ui_state *state)
{
	struct state_main_menu *main_menu_state = &state->main_menu;

	ui_reset_game();

	menu_reset(main_menu_state->menu);

	ui.distance = 300.f;
	ui.tilt = .5f;

	main_menu_state->start_t = msecs();
}

struct state_main_menu state_main_menu = {
	{ 
		on_idle,
		on_worker_reply,
		on_motion_notify,
		on_drag,
		on_button_press,
		on_button_release,
		on_escape_press,
		on_escape_release,
		redraw,
		initialize,
		on_state_switch,
	}
};
