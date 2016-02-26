/* ui_util.h -- part of vulcan
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

#ifndef UI_UTIL_H_
#define UI_UTIL_H_

#include "ui.h"

void
ui_set_modelview_matrix(struct matrix *mv);

void
ui_render_board_state(void);

void
ui_render_text(void);

void
ui_render_move_history(void);

void
ui_render_clock(void);

void
ui_render_text(void);

#endif /* UI_UTIL_H_ */
