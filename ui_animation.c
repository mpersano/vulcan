/* ui_animation.c -- part of vulcan
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
#include <assert.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "panic.h"
#include "vector.h"
#include "matrix.h"
#include "pick.h"
#include "game.h"
#include "move.h"
#include "font.h"
#include "font_render.h"
#include "graphics.h"
#include "render.h"
#include "ui.h"
#include "ui_util.h"
#include "ui_state.h"

extern unsigned long msecs(void);

static const float animation_speed = .25f;

static void
on_idle(union ui_state *state)
{
	struct state_animation *animation = &state->animation;

	unsigned long dt = msecs() - animation->start_t;

	if (dt >= animation->duration) {
		ui_set_state(STATE_WAITING_FOR_MOVE);
	} else {
		struct vector temp;

		vec_scalar_mul_copy(&temp, &state->animation.speed, dt);

		vec_add(&state->animation.cur_pos, &state->animation.from,
		  &temp);
	}
}

static void
on_worker_reply(union ui_state *state, union move *move)
{
}

static void
on_motion_notify(union ui_state *state, int button, int x, int y)
{
}

static void
on_drag(union ui_state *state, int button, int x, int y)
{
	state_waiting_for_move.common.on_drag(state, button, x, y);
}

static void
on_button_press(union ui_state *state, int button, int x, int y)
{
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
	struct matrix mv;
	struct state_animation *animation = &state->animation;

	ui_set_modelview_matrix(&mv);

	render_board_state(&animation->prev_board_state,
	  &ui.selected_squares, &mv);

	render_piece_at(&mv, &animation->cur_pos, animation->piece, 0, 0);

	ui_render_text();
}

static void
initialize(union ui_state *state)
{
}

static void
on_state_switch(union ui_state *state)
{
	struct state_animation *animation = &state->animation;
	union move *move = &ui.animation_move;
	struct vector from, to, dir;
	float dist;

	assert(move->type == PIECE_MOVE);

	get_position_xyz(&from, &move->piece_move.from);
	get_position_xyz(&to, &move->piece_move.to);
	vec_sub(&dir, &to, &from);

	dist = vec_length(&dir);

	vec_scalar_mul_copy(&animation->speed, &dir, animation_speed/dist);
	vec_copy(&animation->cur_pos, &from);
	vec_copy(&animation->from, &from);

	animation->duration = dist/animation_speed;
	animation->start_t = msecs();
	animation->piece = ui.board_state.board[move->piece_move.to.level]
	  [move->piece_move.to.square];

	animation->prev_board_state = ui.board_state;

	undo_move(&animation->prev_board_state, &ui.last_undo_info);

	animation->prev_board_state.board
	  [ui.animation_move.piece_move.from.level]
	  [ui.animation_move.piece_move.from.square] = EMPTY;
}

struct state_animation state_animation = {
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
