/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2011 Sean Bolton.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#ifndef GUI_RETRO_CONSOLE_H
#define GUI_RETRO_CONSOLE_H

#include <gtk/gtk.h>

#define RC_USE_BIG_FONT

#ifndef RC_USE_BIG_FONT
#define RC_FONT_WIDTH       7
#define RC_FONT_HEIGHT     13
#else
#define RC_FONT_WIDTH       9
#define RC_FONT_HEIGHT     18
#endif

#define RC_COLOR_WHITE  0
#define RC_COLOR_BLACK  1
#define RC_COLOR_GREEN  2
#define RC_COLOR_BLUE   3
#define RC_COLOR_YELLOW 4
#define RC_COLOR_CYAN   5
#define RC_COLOR_COUNT  6

#define RC_MAX_COLOR_PAIRS  8

extern GtkWidget *rc_drawing_area;

GtkWidget *retro_console_init(int width, int height);  /* returns the console GtkDrawingArea */
void rc_console_set_button_press_callback(void (*cb)(guint button, guint x, guint y));
void rc_console_set_key_press_callback(void (*cb)(gboolean pressed, guint key, guint state));
void rc_console_addch(char c);
void rc_console_addstr(const char *t);
void rc_console_move(int y, int x);
void rc_console_clrtoeol(void);
void rc_console_erase(void);
void rc_console_init_color_pair(int pair, int fg, int bg);
void rc_console_set_color_pair(int pair);

#endif  /* GUI_RETRO_CONSOLE_H */
