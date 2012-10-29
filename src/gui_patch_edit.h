/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2011, 2012 Sean Bolton.
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

#ifndef _GUI_PATCH_EDIT_H
#define _GUI_PATCH_EDIT_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "hexter_types.h"

#define PEPT_Name     1
#define PEPT_0_99     2
#define PEPT_0_7      3
#define PEPT_Mode     4   /* R,F */
#define PEPT_FC       5   /* 0-31 */
#define PEPT_FF       6   /* 0-99 */
#define PEPT_Detune   7   /* 0-14, as -7 to +7 */
#define PEPT_0_3      8
#define PEPT_Alg      9   /* 0-31, as 1-32 */
#define PEPT_Curve   10   /* 0-3, scaling curve */
#define PEPT_BkPt    11   /* 0-99, breakpoint */
#define PEPT_Trans   12   /* 0-48, transpose, as -24 to +24 */
#define PEPT_OnOff   13
#define PEPT_LFOWave 14
#define PEPT_Env     15   /* 0-99, envelope */
#define PEPT_Scal99  16   /* 0-99, for parameters that affect level scaling */
#define PEPT_COUNT   17

extern GtkWidget *editor_window;
extern GtkWidget *editor_name_entry;

/* It is possible to have three editors active simultaneously (widgy, retro,
 * and MIDI), so EVERYTHING in a patch (except the name) is represented by an
 * adjustment, stored in edit_adj[]. This model and each editor connect to the
 * adjustments, and are then notified when any of the other connectees makes a
 * change.
 */
extern GtkObject *edit_adj[DX7_VOICE_PARAMETERS - 1]; /* omitting name */

void  patch_edit_create_edit_adjs(void);
void  patch_edit_connect_to_model(void);
void  patch_edit_update_status(void);
void  patch_edit_copy_name_to_utf8(char *buffer, uint8_t *patch, int packed);
void  patch_edit_update_editors(void);
int   patch_edit_get_edit_parameter(int offset);
char *patch_edit_NoteText(int note);

extern unsigned short peFFF[100];

extern int sysex_receive_channel;
void on_sysex_receipt(unsigned int length, unsigned char *data);

GtkWidget *create_editor_window(const char *tag);

/* in gui_retro_editor.c: */
extern GtkWidget *retro_widget;
GtkWidget *create_retro_editor(const char *tag);

/* in gui_widgy_editor.c: */
extern GtkWidget *widgy_widget;
GtkWidget *create_widgy_editor(const char *tag);

/* in gui_widgy_alg.c: */
extern GtkWidget *we_alg_drawing_area;
void we_alg_drawing_area_new(void);
void we_alg_draw(cairo_t *cr, int alg);

#endif /* _GUI_PATCH_EDIT_H */

