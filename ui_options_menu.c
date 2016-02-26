#include <stdio.h>

#include "ui.h"
#include "menu.h"
#include "ui_options_menu.h"

#define FLAG_AS_STR(v) ((v) ? "On" : "Off")

static void
menu_toggle_texture_flag(void *extra)
{
	ui.do_textures ^= 1;
}

static const char *
menu_get_texture_flag(void *extra)
{
	return FLAG_AS_STR(ui.do_textures);
}

static void
menu_toggle_reflection_flag(void *extra)
{
	ui.do_reflections ^= 1;
}

static const char *
menu_get_reflection_flag(void *extra)
{
	return FLAG_AS_STR(ui.do_reflections);
}

static void
menu_toggle_move_history_flag(void *extra)
{
	ui.do_move_history ^= 1;
}

static const char *
menu_get_move_history_flag(void *extra)
{
	return FLAG_AS_STR(ui.do_move_history);
}

void
add_options_menu(struct menu *menu)
{
	struct menu *options_menu;

	options_menu = menu_add_submenu_item(menu, "Options");

	menu_add_toggle_item(options_menu, "Textures:",
	  menu_toggle_texture_flag, NULL,
	  menu_get_texture_flag, NULL);

	menu_add_toggle_item(options_menu, "Reflections:",
	  menu_toggle_reflection_flag, NULL,
	  menu_get_reflection_flag, NULL);

	menu_add_toggle_item(options_menu, "Move History:",
	  menu_toggle_move_history_flag, NULL,
	  menu_get_move_history_flag, NULL);
}
