/* menu.h -- part of vulcan
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

#ifndef MENU_H_
#define MENU_H_

struct menu;

struct font_render_info;

struct menu *
menu_make(const char *name, const struct font_render_info *fri);

void
menu_free(struct menu *menu);

struct menu *
menu_add_submenu_item(struct menu *menu, const char *name);

void
menu_add_action_item(struct menu *menu, const char *text,
  void(*on_action)(void *extra), void *extra);

void
menu_add_toggle_item(struct menu *menu, const char *text,
  void(*on_action)(void *extra), void *extra_on_action,
  const char *(*get_toggle_status)(void *extra_on_get_toggle),
  void *extra_on_get_toggle);

void
menu_render(const struct menu *menu);

int
menu_on_mouse_motion(struct menu *menu, int x, int y);

int
menu_on_click(struct menu *menu, int x, int y);

void
menu_on_hide(struct menu *menu);

void
menu_on_show(struct menu *menu);

void
menu_reset(struct menu *menu);

#endif /* MENU_H_ */
