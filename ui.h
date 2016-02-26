/* ui.h -- part of vulcan
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

#ifndef UI_H_
#define UI_H_

#include "game.h"
#include "vector.h"
#include "matrix.h"
#include "ui_state.h"

struct ui;
union ui_state;

enum computer_player_state {
	IDLE,
	WAITING_FOR_USER_INPUT,
	ANIMATING,
	THINKING,
	DONE_MATE_FOR_BLACK,
	DONE_MATE_FOR_WHITE,
};

struct list;

struct ui {
	struct board_state board_state;
	struct selected_squares selected_squares;

	int cur_side;
	int max_depth;				/* max AI search depth */
	enum computer_player_state computer_player_state;
	enum player_type players[2];

	struct list *move_history;

	struct matrix rmatrix; 			/* rotation matrix */


	float rotation;				/* 0 - 2*M_PI */
	float tilt;				/* 0 - .5*M_PI */
	float distance;

	int do_reflections;
	int do_textures;
	int do_move_history;

	struct position last_selected_pos;	/* last square selected by
						 * user  */
	int left_button_down;
	int right_button_down;
	int control_key_down;
	int start_position_selected;

	struct position last_highlighted_pos;
	int pos_highlighted;

	int last_clicked_x;
	int last_clicked_y;

	int cur_width;
	int cur_height;

	struct font_render_info *font_small;
	struct font_render_info *font_medium;
	struct font_render_info *font_big;
	struct font_render_info *font_digits;

	union ui_state *cur_state;

	struct undo_move_info last_undo_info;

	union move animation_move;

	int to_quit;

	long ply_prev_msecs;
	long white_player_msecs;
	long black_player_msecs;
};

enum {
	BTN_LEFT = 1,
	BTN_RIGHT,
};

extern struct ui ui;

void
ui_on_worker_reply(union move *move);

void
ui_on_motion_notify(int button, int x, int y);

void
ui_on_button_press(int button, int x, int y);

void
ui_on_button_release(int button, int x, int y);

void
ui_redraw(void);

void
ui_reshape(int width, int height);

void
ui_reset_game(void);

void
ui_initialize(enum player_type white_player,
  enum player_type black_player, int max_depth, int width, int height);

void
gl_initialize(void);

void
ui_on_idle(void);

void
ui_on_escape_press(void);

void
ui_on_escape_release(void);

enum ui_state_type;

void
ui_set_state(enum ui_state_type state_type);

#endif /* UI_H_ */
