/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2004 Sean Bolton and others.
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
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _GUI_INTERFACE_H
#define _GUI_INTERFACE_H

#include <gtk/gtk.h>

extern GtkWidget *main_window;
extern GtkObject *tuning_adj;
extern GtkObject *volume_adj;
extern GtkObject *polyphony_instance_adj;
extern GtkObject *polyphony_global_adj;
extern GtkWidget *monophonic_option_menu;
extern GtkWidget *sysex_channel_label;
extern GtkWidget *sysex_channel_spin;
extern GtkWidget *sysex_status_label;
extern GtkWidget *sysex_discard_button;
extern GtkWidget *sysex_save_button;

extern GtkWidget *about_window;
extern GtkWidget *about_label;

extern GtkWidget *import_file_selection;
extern GtkWidget *import_file_position_window;
extern GtkObject *import_file_position_spin_adj;
extern GtkWidget *import_file_position_name_label;

extern GtkWidget *notice_window;
extern GtkWidget *notice_label_1;
extern GtkWidget *notice_label_2;

extern GtkWidget *export_file_type_window;
extern GtkWidget *export_file_type_sysex;
extern GtkObject *export_file_start_spin_adj;
extern GtkWidget *export_file_start_name;
extern GtkWidget *export_file_end_label;
extern GtkWidget *export_file_end_spin;
extern GtkObject *export_file_end_spin_adj;
extern GtkWidget *export_file_end_name;
extern GtkWidget *export_file_selection;

extern GtkWidget *edit_save_position_window;
extern GtkObject *edit_save_position_spin_adj;
extern GtkWidget *edit_save_position_name_label;

extern GtkWidget *patches_clist;

void create_windows(const char *instance_tag);

#endif /* _GUI_INTERFACE_H */

