/* ui_in_game_menu.c -- part of vulcan
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

#include "ui.h"
#include "menu.h"
#include "font.h"
#include "font_render.h"
#include "ui_util.h"
#include "ui_options_menu.h"
#include "ui_state.h"

/* ui_state */

static void
on_idle(union ui_state *ui_state)
{
}

static void
on_worker_reply(union ui_state *state, union move *move)
{
	/* HACK */
	state_waiting_for_move.common.on_worker_reply(state, move);
}

static void
on_motion_notify(union ui_state *state, int button, int x, int y)
{
	menu_on_mouse_motion(state->in_game_menu.menu, x, y);
}

static void
on_drag(union ui_state *state, int button, int x, int y)
{
	on_motion_notify(state, button, x, y);
}

static void
on_button_press(union ui_state *state, int button, int x, int y)
{
	menu_on_click(state->in_game_menu.menu, x, y);
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
	ui_set_state(STATE_WAITING_FOR_MOVE);
}

static void
redraw(union ui_state *state)
{
	ui_render_board_state();

	ui_render_text();

	menu_render(state->in_game_menu.menu);
}

static void
menu_restart(void *extra)
{
	ui_set_state(STATE_MAIN_MENU);
}

static void
menu_quit(void *extra)
{
	ui.to_quit = 1;
}

static void
menu_back(void *extra)
{
	ui_set_state(STATE_WAITING_FOR_MOVE);
}

static void
initialize(union ui_state *state)
{
	struct menu *menu;

	menu = menu_make("main", ui.font_medium);

	menu_add_action_item(menu, "Resume Game", menu_back, NULL);
	menu_add_action_item(menu, "Restart", menu_restart, state);
	add_options_menu(menu);
	menu_add_action_item(menu, "Quit", menu_quit, NULL);

	state->in_game_menu.menu = menu;
}

static void
on_state_switch(union ui_state *state)
{
}

struct state_in_game_menu state_in_game_menu = {
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
