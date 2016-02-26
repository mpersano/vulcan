/* menu.c -- part of vulcan
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
#include <string.h>
#include <assert.h>
#include <GL/gl.h>

#include "gl_util.h"
#include "list.h"
#include "font_render.h"
#include "menu.h"

enum menu_item_type {
	MI_SUBMENU,
	MI_ACTION,
	MI_TOGGLE,
};

struct menu {
	char *name;
	const struct font_render_info *fri;
	int max_text_width;
	int item_height;
	struct list *items;
	struct menu_item *cur_selection;
	const struct menu_item *hilite;
	struct menu *parent;
	struct menu *cur_submenu;
};

struct menu_item {
	enum menu_item_type type;
	char *text;

	/* if toggle or action */
	void (*on_action)(void *extra_on_action);
	void *extra_on_action;

	/* if toggle */
	const char *(*get_toggle_status)(void *extra_on_get_toggle);
	void *extra_on_get_toggle;

	/* if submenu */
	struct menu *submenu;
};

static void
get_viewport_dimensions(int *width, int *height)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	*width = viewport[2];
	*height = viewport[3];
}

static void
menu_on_back(struct menu *menu)
{
	assert(menu->parent);
	menu->parent->cur_submenu = NULL;
	menu->hilite = menu->parent->hilite = NULL;
}

static const struct menu_item menu_item_back = {
	MI_ACTION,
	"Back",
	(void(*)(void *))menu_on_back, NULL,
	NULL, NULL,
	NULL
};

struct menu *
menu_make(const char *name, const struct font_render_info *fri)
{
	struct menu *m;

	m = malloc(sizeof *m);

	m->name = strdup(name);
	m->fri = fri;
	m->items = list_make();
	m->cur_selection = NULL;
	m->max_text_width = 0;
	m->hilite = NULL;
	m->parent = NULL;
	m->cur_submenu = NULL;
	m->item_height = 3*fri->char_height/2;

	return m;
}

void menu_free(struct menu *);

static void
menu_item_free(struct menu_item *item)
{
	if (item->submenu)
		menu_free(item->submenu);

	free(item->text);
}

void
menu_free(struct menu *menu)
{
	list_free(menu->items, (void(*)(void *))menu_item_free);
	free(menu->name);
	free(menu);
}

static struct menu_item *
menu_item_make(enum menu_item_type type, const char *text)
{
	struct menu_item *mi;

	mi = malloc(sizeof *mi);

	mi->type = type;
	mi->text = strdup(text);

	mi->on_action = NULL;
	mi->get_toggle_status = NULL;
	mi->submenu = NULL;

	return mi;
}

static void
menu_add_item(struct menu *menu, struct menu_item *mi)
{
	int w;

	list_append(menu->items, mi);

	w = string_width_in_pixels(menu->fri, mi->text);

	if (w > menu->max_text_width)
		menu->max_text_width = w;
}

struct menu *
menu_add_submenu_item(struct menu *menu, const char *name)
{
	struct menu_item *mi;

	mi = menu_item_make(MI_SUBMENU, name);
	mi->submenu = menu_make(name, menu->fri);
	mi->submenu->parent = menu;

	menu_add_item(menu, mi);

	return mi->submenu;
}

void
menu_add_action_item(struct menu *menu, const char *text,
  void(*on_action)(void *extra), void *extra)
{
	struct menu_item *mi;

	mi = menu_item_make(MI_ACTION, text);
	mi->on_action = on_action;
	mi->extra_on_action = extra;

	menu_add_item(menu, mi);
}

void
menu_add_toggle_item(struct menu *menu, const char *text,
  void(*on_action)(void *extra), void *extra_on_action,
  const char *(*get_toggle_status)(void *extra_on_get_toggle),
  void *extra_on_get_toggle)
{
	struct menu_item *mi;

	mi = menu_item_make(MI_TOGGLE, text);

	mi->on_action = on_action;
	mi->extra_on_action = extra_on_action;

	mi->get_toggle_status = get_toggle_status;
	mi->extra_on_get_toggle = extra_on_get_toggle;

	menu_add_item(menu, mi);
}

static const char *
menu_item_text(const struct menu *menu, const struct menu_item *mi)
{
	char *text;

	if (mi->type == MI_TOGGLE) {
		static char str[256];

		snprintf(str, sizeof str, "%s %s", mi->text,
		  mi->get_toggle_status(mi->extra_on_get_toggle));

		text = str;
	} else {
		text = mi->text;
	}

	return text;
}

static int
menu_item_index(const struct menu *menu, const struct menu_item *mi)
{
	struct list_node *ln;
	int idx = -1, i;

	for (i = 0, ln = menu->items->first; ln; ++i, ln = ln->next) {
		if ((const struct menu_item *)ln->data == mi) {
			idx = i;
			break;
		}
	}

	if (idx == -1 && menu->parent && mi == &menu_item_back)
		idx = i;

	assert(idx != -1);

	return idx;
}

static int
menu_item_width(const struct menu *menu, const struct menu_item *mi)
{
	return string_width_in_pixels(menu->fri, menu_item_text(menu, mi));
}

static int
menu_width(const struct menu *menu)
{
	struct list_node *ln;
	int width = 0;

	for (ln = menu->items->first; ln; ln = ln->next) {
		int w = menu_item_width(menu,
		  (const struct menu_item *)ln->data);

		if (w > width)
			width = w;
	}

	return width;
}

static int
menu_num_items(const struct menu *menu)
{
	int nitems = menu->items->length;

	if (menu->parent)
		nitems++; /* "back" option */

	return nitems;
}

static void
menu_get_position(const struct menu *menu, int *px, int *py)
{
	int viewport_width, viewport_height;

	get_viewport_dimensions(&viewport_width, &viewport_height);

	*px = .5*(viewport_width - menu_width(menu));
	*py = .5*(viewport_height - menu_num_items(menu)*menu->item_height);
}

static void
menu_render_item(const struct menu *menu, const struct menu_item *mi, int y)
{
	int x, viewport_width, dummy;
	float scale;

	get_viewport_dimensions(&viewport_width, &dummy);

	if (mi == menu->hilite) {
		glColor4f(1.f, 1.f, .1f, 1.f);
		scale = 1.2f;
	} else {
		glColor4f(1.f, 1.f, 1.f, 1.f);
		scale = 1.f;
	}

	x = .5*(viewport_width - scale*menu_item_width(menu, mi));
	y += .5f*menu->item_height*(scale - 1.);

	render_string_scaled(menu->fri, menu_item_text(menu, mi), x, y, scale);
}

enum {
	MENU_WINDOW_BORDER = 80
};

static void
menu_render_background(const struct menu *menu)
{
	GLint viewport[4];

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_FLAT);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGetIntegerv(GL_VIEWPORT, viewport);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.f, viewport[2], viewport[3], 0.f, -1.f, 1.f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor4f(.3f, .3f, .3f, .8f);

	glBegin(GL_QUADS);
	glVertex2f(MENU_WINDOW_BORDER, 0);
	glVertex2f(viewport[2] - MENU_WINDOW_BORDER, 0);
	glVertex2f(viewport[2] - MENU_WINDOW_BORDER, viewport[3]);
	glVertex2f(MENU_WINDOW_BORDER, viewport[3]);
	glEnd();

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
}

void
menu_render(const struct menu *menu)
{
	const struct list_node *ln;
	int x, y;

	while (menu->cur_submenu)
		menu = menu->cur_submenu;

	menu_get_position(menu, &x, &y);

	menu_render_background(menu);

	y += 26; /* HACK */

	for (ln = menu->items->first; ln; ln = ln->next) {
		menu_render_item(menu, (const struct menu_item *)ln->data, y);
		y += menu->item_height;
	}

	if (menu->parent)
		menu_render_item(menu, &menu_item_back, y);
}


static const struct menu_item *
menu_item_at(const struct menu *menu, int x, int y)
{
	int idx;
	int menu_x, menu_y, num_items;
	const struct menu_item *mi;

	menu_get_position(menu, &menu_x, &menu_y);

	num_items = menu_num_items(menu);

	mi = NULL;

	if (y >= menu_y && y <= menu_y + menu->item_height*num_items) {
		idx = (y - menu_y)/menu->item_height;

		if (idx >= 0 && idx < num_items) {
			int viewport_width, item_width, dummy;

			get_viewport_dimensions(&viewport_width, &dummy);

			if (menu->parent && idx == num_items - 1)
				mi = &menu_item_back;
			else
				mi = (struct menu_item *)
				  list_element_at(menu->items, idx);

			item_width = menu_item_width(menu, mi);

			if (x < .5*(viewport_width - item_width) ||
			  x > .5*(viewport_width + item_width))
				mi = NULL;
		}
	}

	return mi;
}

int
menu_on_mouse_motion(struct menu *menu, int x, int y)
{
	const struct menu_item *prev;

	while (menu->cur_submenu)
		menu = menu->cur_submenu;

	prev = menu->hilite;

	menu->hilite = menu_item_at(menu, x, y);

	return menu->hilite != prev;
}

int
menu_on_click(struct menu *menu, int x, int y)
{
	const struct menu_item *mi;

	while (menu->cur_submenu)
		menu = menu->cur_submenu;

	if ((mi = menu_item_at(menu, x, y)) == NULL)
		return 0;

	if (mi->type == MI_SUBMENU)
		menu->cur_submenu = mi->submenu;
	else
		mi->on_action(mi == &menu_item_back ? menu :
		  mi->extra_on_action);

	menu_on_mouse_motion(menu, x, y);

	return 1;
}

void
menu_on_hide(struct menu *menu)
{
	menu->hilite = NULL;
}

void
menu_on_show(struct menu *menu)
{
	menu->hilite = NULL;
}

void
menu_reset(struct menu *menu)
{
	menu->cur_submenu = NULL;

	menu_on_show(menu);
}
