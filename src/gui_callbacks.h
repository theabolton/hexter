/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2004, 2009, 2012 Sean Bolton and others.
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

#ifndef _GUI_CALLBACKS_H
#define _GUI_CALLBACKS_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdint.h>
#include <gtk/gtk.h>

#include "hexter.h"

void on_menu_import_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_export_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_quit_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_edit_activate(GtkMenuItem *menuitem, gpointer user_data);
void on_menu_about_activate(GtkMenuItem *menuitem, gpointer user_data);
gint on_delete_event_wrapper(GtkWidget *widget, GdkEvent *event, gpointer data);
void on_import_file_ok(GtkWidget *widget, gpointer data);
void on_import_file_cancel(GtkWidget *widget, gpointer data);
void on_position_change(GtkWidget *widget, gpointer data);
void on_import_file_position_ok(GtkWidget *widget, gpointer data);
void on_import_file_position_cancel(GtkWidget *widget, gpointer data);
void on_export_file_type_press(GtkWidget *widget, gpointer data);
void on_export_file_position_change(GtkWidget *widget, gpointer data);
void on_export_file_type_ok(GtkWidget *widget, gpointer data);
void on_export_file_type_cancel(GtkWidget *widget, gpointer data);
void on_export_file_ok(GtkWidget *widget, gpointer data);
void on_export_file_cancel(GtkWidget *widget, gpointer data);
void on_about_dismiss(GtkWidget *widget, gpointer data);
void on_patches_selection(GtkWidget *clist, gint row, gint column,
                          GdkEventButton *event, gpointer data);
void on_tuning_change(GtkWidget *widget, gpointer data);
void on_volume_change(GtkWidget *widget, gpointer data);
void on_polyphony_change(GtkWidget *widget, gpointer data);
void on_mono_mode_activate(GtkWidget *widget, gpointer data);
void on_compat059_toggled(GtkWidget *widget, gpointer data);
#ifdef HEXTER_DEBUG_CONTROL
void on_test_changed(GtkObject *adj, gpointer data);
#endif
#ifdef MIDI_ALSA
void on_sysex_enable_toggled(GtkWidget *widget, gpointer data);
void on_sysex_channel_change(GtkWidget *widget, gpointer data);
#endif /* MIDI_ALSA */
void on_edit_save_position_ok(GtkWidget *widget, gpointer data);
void on_edit_save_position_cancel(GtkWidget *widget, gpointer data);
void send_performance(void);
void on_performance_spin_change(GtkWidget *widget, gpointer data);
void on_performance_assign_toggled(GtkWidget *widget, gpointer data);
void on_test_note_slider_change(GtkWidget *widget, gpointer data);
void release_test_note(void);
void on_test_note_button_press(GtkWidget *widget, gpointer data);
void display_notice(char *message1, char *message2);
void on_notice_dismiss(GtkWidget *widget, gpointer data);
void update_voice_widget(int port, float value);
void update_from_program_select(unsigned long bank, unsigned long program);
void update_patches(const char *key, const char *value);
void update_edit_buffer(const char *value);
void update_performance_widgets(uint8_t *performance);
void update_performance(const char *value);
void update_monophonic(const char *value);
void update_polyphony(const char *value);
void update_global_polyphony(const char *value);
void patches_clist_set_program(void);
void rebuild_patches_clist(void);

#endif  /* _GUI_CALLBACKS_H */

