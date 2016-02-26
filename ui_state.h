/* ui_state.h -- part of vulcan
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

#ifndef UI_STATE_H_
#define UI_STATE_H_

union ui_state;

struct menu;

enum ui_state_type {
	STATE_MAIN_MENU,
	STATE_WAITING_FOR_MOVE,
	STATE_ANIMATION,
	STATE_IN_GAME_MENU,
	STATE_GAME_OVER,
};

/*	
 *	STATES:
 *
 *	1. Main Menu
 *	2. Waiting for Move
 *	3. Piece Animation
 *	4. Game Over
 *	5. In-Game Menu
 */

struct state_common {
	void (*on_idle)(union ui_state *);
	void (*on_worker_reply)(union ui_state *, union move *move);
	void (*on_motion_notify)(union ui_state *, int button, int x, int y);
	void (*on_drag)(union ui_state *, int button, int x, int y);
	void (*on_button_press)(union ui_state *, int button, int x, int y);
	void (*on_button_release)(union ui_state *, int button, int x, int y);
	void (*on_escape_press)(union ui_state *);
	void (*on_escape_release)(union ui_state *);
	void (*redraw)(union ui_state *);
	void (*initialize)(union ui_state *);
	void (*on_state_switch)(union ui_state *);
};

/* main menu */

struct state_main_menu {
	struct state_common common;
	struct menu *menu;
	unsigned long start_t;
};

extern struct state_main_menu state_main_menu;

/* waiting for move */

struct state_waiting_for_move {
	struct state_common common;
};

extern struct state_waiting_for_move state_waiting_for_move;

/* piece animation */

struct state_animation {
	struct state_common common;
	int piece;
	struct vector from, speed, cur_pos;
	unsigned long start_t;
	unsigned long duration;
	struct board_state prev_board_state;
};

extern struct state_animation state_animation;

/* game over */

struct state_game_over {
	struct state_common common;
};

extern struct state_game_over state_game_over;

/* in-game menu */

struct state_in_game_menu {
	struct state_common common;
	struct menu *menu;
};

extern struct state_in_game_menu state_in_game_menu;

/* ui state */

union ui_state {
	struct state_common common;
	struct state_main_menu main_menu;
	struct state_waiting_for_move waiting_for_move;
	struct state_animation animation;
	struct state_game_over game_over;
	struct state_in_game_menu in_game_menu;
};

#endif /* UI_STATE_H_ */
