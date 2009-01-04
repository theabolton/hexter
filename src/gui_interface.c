/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2004, 2009 Sean Bolton and others.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "hexter.h"
#include "gui_callbacks.h"
#include "gui_interface.h"
#include "gui_images.h"

GtkWidget *main_window;
GtkObject *tuning_adj;
GtkObject *volume_adj;
GtkObject *polyphony_instance_adj;
GtkObject *polyphony_global_adj;
GtkWidget *monophonic_option_menu;
GtkWidget *compat059_button;
GtkWidget *sysex_channel_label;
GtkWidget *sysex_channel_spin;
GtkWidget *sysex_status_label;
GtkWidget *sysex_discard_button;
GtkWidget *sysex_save_button;
GtkWidget *performance_frame;
GtkObject *performance_spin_adjustments[6];
GtkWidget *performance_assign_widgets[4][3];

GtkWidget *about_window;
GtkWidget *about_label;

GtkWidget *import_file_selection;
GtkWidget *import_file_position_window;
GtkObject *import_file_position_spin_adj;
GtkWidget *import_file_position_name_label;

GtkWidget *notice_window;
GtkWidget *notice_label_1;
GtkWidget *notice_label_2;

GtkWidget *export_file_type_window;
GtkWidget *export_file_type_sysex;
GtkObject *export_file_start_spin_adj;
GtkWidget *export_file_start_name;
GtkWidget *export_file_end_label;
GtkWidget *export_file_end_spin;
GtkObject *export_file_end_spin_adj;
GtkWidget *export_file_end_name;
GtkWidget *export_file_selection;

GtkWidget *edit_save_position_window;
GtkObject *edit_save_position_spin_adj;
GtkWidget *edit_save_position_name_label;

GtkWidget *patches_clist;

const char *performance_spin_names[6] = {
    "pitch_bend_range",
    "portamento_time",
    "mod_wheel_sensitivity",
    "foot_sensitivity",
    "pressure_sensitivity",
    "breath_sensitivity",
};

const char *performance_assign_names[4] = {
    "mod_wheel_assign",
    "foot_assign",
    "pressure_assign",
    "breath_assign"
};

void
set_window_title(GtkWidget *window, const char *tag, const char *text)
{
    char *title = (char *)malloc(strlen(tag) + strlen(text) + 2);
    sprintf(title, "%s %s", tag, text);
    gtk_window_set_title (GTK_WINDOW (window), title);
    free(title);
}

void
create_performance_spin(GtkWidget *window, const char *text, GtkWidget *table,
                        int row, int parameter, int max, int init)
{
    GtkWidget *label;
    GtkObject *spin_button_adj;
    GtkWidget *spin_button;

    label = gtk_label_new (text);
    gtk_widget_ref (label);
    gtk_object_set_data_full (GTK_OBJECT (window), "performance label", label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label), 2, 0);

    spin_button_adj = gtk_adjustment_new (init, 0, max, 1, 10, 0);
    performance_spin_adjustments[parameter] = spin_button_adj;
    spin_button = gtk_spin_button_new (GTK_ADJUSTMENT (spin_button_adj), 1, 0);
    gtk_widget_ref (spin_button);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "performance spin", spin_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (spin_button);
    gtk_table_attach (GTK_TABLE (table), spin_button, 1, 2, row, row + 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON (spin_button), GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin_button), TRUE);

    gtk_signal_connect (GTK_OBJECT (spin_button_adj), "value_changed",
                        GTK_SIGNAL_FUNC(on_performance_spin_change),
                        (gpointer)parameter);
}

void
create_performance_assign(GtkWidget *window, const char *text,
                          GtkWidget *table, int row, int parameter, int initbits)
{
    GtkWidget *label;
    GtkWidget *hbox;
    GtkWidget *button;

    label = gtk_label_new (text);
    gtk_widget_ref (label);
    gtk_object_set_data_full (GTK_OBJECT (window), "performance label", label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label), 2, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox);
    gtk_object_set_data_full (GTK_OBJECT (window), "assign hbox",
                              hbox, (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox);
    gtk_table_attach (GTK_TABLE (table), hbox, 1, 2, row, row + 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);

    label = gtk_label_new ("P");
    gtk_widget_ref (label);
    gtk_object_set_data_full (GTK_OBJECT (window), "assign P label", label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 1);

    button = gtk_check_button_new();
    performance_assign_widgets[parameter][0] = button;
    gtk_widget_ref (button);
    gtk_object_set_data_full (GTK_OBJECT (window), "assign P button", button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), initbits & 1 ? 1 : 0);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 1);
    gtk_signal_connect (GTK_OBJECT (button), "toggled",
                        GTK_SIGNAL_FUNC (on_performance_assign_toggled),
                        (gpointer)parameter);

    label = gtk_label_new ("A");
    gtk_widget_ref (label);
    gtk_object_set_data_full (GTK_OBJECT (window), "assign A label", label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 1);

    button = gtk_check_button_new();
    performance_assign_widgets[parameter][1] = button;
    gtk_widget_ref (button);
    gtk_object_set_data_full (GTK_OBJECT (window), "assign A button", button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), initbits & 2 ? 1 : 0);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 1);
    gtk_signal_connect (GTK_OBJECT (button), "toggled",
                        GTK_SIGNAL_FUNC (on_performance_assign_toggled),
                        (gpointer)parameter);

    label = gtk_label_new ("E");
    gtk_widget_ref (label);
    gtk_object_set_data_full (GTK_OBJECT (window), "assign E label", label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 1);

    button = gtk_check_button_new();
    performance_assign_widgets[parameter][2] = button;
    gtk_widget_ref (button);
    gtk_object_set_data_full (GTK_OBJECT (window), "assign E button", button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), initbits & 4 ? 1 : 0);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 1);
    gtk_signal_connect (GTK_OBJECT (button), "toggled",
                        GTK_SIGNAL_FUNC (on_performance_assign_toggled),
                        (gpointer)parameter);
}

void
create_main_window (const char *tag)
{
    GtkWidget *vbox1;
    GtkWidget *menubar1;
    GtkWidget *file1;
    GtkWidget *file1_menu;
#if !GTK_CHECK_VERSION(2, 0, 0)
    GtkAccelGroup *file1_menu_accels;
    GtkAccelGroup *help1_menu_accels;
#endif
    GtkWidget *menu_import;
    GtkWidget *menu_export;
    GtkWidget *separator1;
    GtkWidget *menu_quit;
    GtkWidget *help1;
    GtkWidget *help1_menu;
    GtkWidget *menu_about;
    GtkWidget *notebook1;
    GtkWidget *scrolledwindow1;
    GtkWidget *label45;
    GtkWidget *label46;
    GtkWidget *patches_tab_label;
    GtkWidget *vbox2;
    GtkWidget *frame12;
    GtkWidget *table15;
    GtkWidget *label10;
    GtkWidget *label10a;
    GtkWidget *test_note_button;
    GtkWidget *test_note_key;
    GtkWidget *test_note_velocity;
    GtkWidget *frame14;
    GtkWidget *table16;
    GtkWidget *tuning;
    GtkWidget *volume;
    GtkWidget *mono_mode_off;
    GtkWidget *mono_mode_on;
    GtkWidget *mono_mode_once;
    GtkWidget *mono_mode_both;
    GtkWidget *optionmenu5_menu;
    GtkWidget *polyphony_instance;
    GtkWidget *polyphony_global;
    GtkWidget *label42;
    GtkWidget *label43;
    GtkWidget *label43a;
    GtkWidget *volume_label;
    GtkWidget *label44;
#ifdef MIDI_ALSA
    GtkWidget *sysex_enable_button;
    GtkObject *sysex_channel_spin_adj;
    GtkWidget *sysex_action_label;
    GtkWidget *frame15;
    GtkWidget *frame4;
    GtkWidget *table3;
    GtkWidget *label11;
#endif /* MIDI_ALSA */
    GtkWidget *configuration_tab_label;
    GtkWidget *performance_table;
    GtkWidget *performance_tab_label;
    GtkAccelGroup *accel_group;

    accel_group = gtk_accel_group_new ();

    main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (main_window), "main_window", main_window);
    gtk_window_set_title (GTK_WINDOW (main_window), tag);
 
    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox1);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "vbox1", vbox1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox1);
    gtk_container_add (GTK_CONTAINER (main_window), vbox1);

    menubar1 = gtk_menu_bar_new ();
    gtk_widget_ref (menubar1);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "menubar1", menubar1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (menubar1);
    gtk_box_pack_start (GTK_BOX (vbox1), menubar1, FALSE, FALSE, 0);

    file1 = gtk_menu_item_new_with_label ("File");
    gtk_widget_ref (file1);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "file1", file1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (file1);
    gtk_container_add (GTK_CONTAINER (menubar1), file1);

    file1_menu = gtk_menu_new ();
    gtk_widget_ref (file1_menu);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "file1_menu", file1_menu,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (file1), file1_menu);
#if !GTK_CHECK_VERSION(2, 0, 0)
    file1_menu_accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (file1_menu));
#endif

    menu_import = gtk_menu_item_new_with_label ("Import Patch Bank...");
    gtk_widget_ref (menu_import);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_import", menu_import,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (menu_import);
    gtk_container_add (GTK_CONTAINER (file1_menu), menu_import);
    gtk_widget_add_accelerator (menu_import, "activate", accel_group,
                                GDK_O, GDK_CONTROL_MASK,
                                GTK_ACCEL_VISIBLE);

    menu_export = gtk_menu_item_new_with_label ("Export Patch Bank...");
    gtk_widget_ref (menu_export);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_export", menu_export,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (menu_export);
    gtk_container_add (GTK_CONTAINER (file1_menu), menu_export);
    gtk_widget_add_accelerator (menu_export, "activate", accel_group,
                                GDK_S, GDK_CONTROL_MASK,
                                GTK_ACCEL_VISIBLE);

    separator1 = gtk_menu_item_new ();
    gtk_widget_ref (separator1);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "separator1", separator1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (separator1);
    gtk_container_add (GTK_CONTAINER (file1_menu), separator1);
    gtk_widget_set_sensitive (separator1, FALSE);

    menu_quit = gtk_menu_item_new_with_label ("Quit");
    gtk_widget_ref (menu_quit);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_quit", menu_quit,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (menu_quit);
    gtk_container_add (GTK_CONTAINER (file1_menu), menu_quit);
    gtk_widget_add_accelerator (menu_quit, "activate", accel_group,
                                GDK_Q, GDK_CONTROL_MASK,
                                GTK_ACCEL_VISIBLE);

    help1 = gtk_menu_item_new_with_label ("About");
    gtk_widget_ref (help1);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "help1", help1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (help1);
    gtk_container_add (GTK_CONTAINER (menubar1), help1);
    gtk_menu_item_right_justify (GTK_MENU_ITEM (help1));

    help1_menu = gtk_menu_new ();
    gtk_widget_ref (help1_menu);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "help1_menu", help1_menu,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (help1), help1_menu);
#if !GTK_CHECK_VERSION(2, 0, 0)
    help1_menu_accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (help1_menu));
#endif

    menu_about = gtk_menu_item_new_with_label ("About hexter");
    gtk_widget_ref (menu_about);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "menu_about", menu_about,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (menu_about);
    gtk_container_add (GTK_CONTAINER (help1_menu), menu_about);

    notebook1 = gtk_notebook_new ();
    gtk_widget_ref (notebook1);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "notebook1", notebook1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (notebook1);
    gtk_box_pack_start (GTK_BOX (vbox1), notebook1, TRUE, TRUE, 0);

    scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_ref (scrolledwindow1);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "scrolledwindow1", scrolledwindow1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (scrolledwindow1);
    gtk_container_add (GTK_CONTAINER (notebook1), scrolledwindow1);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    patches_clist = gtk_clist_new (2);
    gtk_widget_ref (patches_clist);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "patches_clist", patches_clist,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (patches_clist);
    gtk_container_add (GTK_CONTAINER (scrolledwindow1), patches_clist);
    gtk_clist_set_column_width (GTK_CLIST (patches_clist), 0, 50);
    gtk_clist_set_column_width (GTK_CLIST (patches_clist), 1, 80);
    gtk_clist_column_titles_show (GTK_CLIST (patches_clist));

    label45 = gtk_label_new ("Prog No");
    gtk_widget_ref (label45);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "label45", label45,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label45);
    gtk_clist_set_column_widget (GTK_CLIST (patches_clist), 0, label45);

    label46 = gtk_label_new ("Name");
    gtk_widget_ref (label46);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "label46", label46,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label46);
    gtk_clist_set_column_widget (GTK_CLIST (patches_clist), 1, label46);

    patches_tab_label = gtk_label_new ("Patches");
    gtk_widget_ref (patches_tab_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "patches_tab_label", patches_tab_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (patches_tab_label);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 0), patches_tab_label);

    /* Configuration tab */
    vbox2 = gtk_vbox_new (FALSE, 5);
    gtk_widget_ref (vbox2);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "vbox2", vbox2,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (notebook1), vbox2);
    gtk_container_set_border_width (GTK_CONTAINER (vbox2), 5);

    frame14 = gtk_frame_new ("Synthesizer");
    gtk_widget_ref (frame14);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "frame14", frame14,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (frame14);
    gtk_box_pack_start (GTK_BOX (vbox2), frame14, TRUE, TRUE, 0);

    table16 = gtk_table_new (2, 6, FALSE);
    gtk_widget_ref (table16);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "table16", table16,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (table16);
    gtk_container_add (GTK_CONTAINER (frame14), table16);
    gtk_container_set_border_width (GTK_CONTAINER (table16), 2);
    gtk_table_set_row_spacings (GTK_TABLE (table16), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table16), 5);

    label43a = gtk_label_new ("tuning");
    gtk_widget_ref (label43a);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "label43a", label43a,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label43a);
    gtk_table_attach (GTK_TABLE (table16), label43a, 0, 1, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label43a), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label43a), 2, 0);

    tuning_adj = gtk_adjustment_new (440.0, 415.3, 466.2, 0.1, 1, 0);
    tuning = gtk_spin_button_new (GTK_ADJUSTMENT (tuning_adj), 1, 1);
    gtk_widget_ref (tuning);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "tuning", tuning,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (tuning);
    gtk_table_attach (GTK_TABLE (table16), tuning, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON (tuning), GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (tuning), TRUE);

    volume_label = gtk_label_new ("volume (dB)");
    gtk_widget_ref (volume_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "volume_label", volume_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (volume_label);
    gtk_table_attach (GTK_TABLE (table16), volume_label, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (volume_label), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (volume_label), 2, 0);

    volume_adj = gtk_adjustment_new (0.0, -70.0, 20.0, 0.1, 1, 0);
    volume = gtk_spin_button_new (GTK_ADJUSTMENT (volume_adj), 1, 1);
    gtk_widget_ref (volume);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "volume", volume,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (volume);
    gtk_table_attach (GTK_TABLE (table16), volume, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON (volume), GTK_UPDATE_IF_VALID);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (volume), TRUE);

    monophonic_option_menu = gtk_option_menu_new ();
    gtk_widget_ref (monophonic_option_menu);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "monophonic_option_menu", monophonic_option_menu,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (monophonic_option_menu);
    gtk_table_attach (GTK_TABLE (table16), monophonic_option_menu, 1, 2, 4, 5,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    optionmenu5_menu = gtk_menu_new ();
    mono_mode_off = gtk_menu_item_new_with_label ("Off");
    gtk_widget_show (mono_mode_off);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_off);
    mono_mode_on = gtk_menu_item_new_with_label ("On");
    gtk_widget_show (mono_mode_on);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_on);
    mono_mode_once = gtk_menu_item_new_with_label ("Once");
    gtk_widget_show (mono_mode_once);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_once);
    mono_mode_both = gtk_menu_item_new_with_label ("Both");
    gtk_widget_show (mono_mode_both);
    gtk_menu_append (GTK_MENU (optionmenu5_menu), mono_mode_both);
    gtk_option_menu_set_menu (GTK_OPTION_MENU (monophonic_option_menu), optionmenu5_menu);

    polyphony_instance_adj = gtk_adjustment_new (HEXTER_DEFAULT_POLYPHONY, 1, HEXTER_MAX_POLYPHONY, 1, 10, 0);
    polyphony_instance = gtk_spin_button_new (GTK_ADJUSTMENT (polyphony_instance_adj), 1, 0);
    gtk_widget_ref (polyphony_instance);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "polyphony_instance", polyphony_instance,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (polyphony_instance);
    gtk_table_attach (GTK_TABLE (table16), polyphony_instance, 1, 2, 2, 3,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    label43 = gtk_label_new ("polyphony (instance)");
    gtk_widget_ref (label43);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "label43", label43,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label43);
    gtk_table_attach (GTK_TABLE (table16), label43, 0, 1, 2, 3,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label43), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label43), 2, 0);

    polyphony_global_adj = gtk_adjustment_new (HEXTER_DEFAULT_POLYPHONY, 1, HEXTER_MAX_POLYPHONY, 1, 10, 0);
    polyphony_global = gtk_spin_button_new (GTK_ADJUSTMENT (polyphony_global_adj), 1, 0);
    gtk_widget_ref (polyphony_global);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "polyphony_global", polyphony_global,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (polyphony_global);
    gtk_table_attach (GTK_TABLE (table16), polyphony_global, 1, 2, 3, 4,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    label42 = gtk_label_new ("polyphony (global)");
    gtk_widget_ref (label42);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "label42", label42,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label42);
    gtk_table_attach (GTK_TABLE (table16), label42, 0, 1, 3, 4,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label42), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label42), 2, 0);

    label44 = gtk_label_new ("monophonic mode");
    gtk_widget_ref (label44);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "label44", label44,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label44);
    gtk_table_attach (GTK_TABLE (table16), label44, 0, 1, 4, 5,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label44), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label44), 2, 0);

    label44 = gtk_label_new ("disable LFO/Mod/Perf\n"
                             "(0.5.x compatibility)");
    gtk_widget_show (label44);
    gtk_table_attach (GTK_TABLE (table16), label44, 0, 1, 5, 6,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label44), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label44), 2, 0);

    compat059_button = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compat059_button), 0);
    gtk_widget_show (compat059_button);
    gtk_table_attach (GTK_TABLE (table16), compat059_button, 1, 2, 5, 6,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

#ifdef MIDI_ALSA
    frame15 = gtk_frame_new ("Sys-Ex Patch Editing");
    gtk_widget_ref (frame15);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "frame15", frame15,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (frame15);
    gtk_box_pack_start (GTK_BOX (vbox2), frame15, TRUE, TRUE, 0);

    table3 = gtk_table_new (5, 3, FALSE);
    gtk_widget_ref (table3);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "table3", table3,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (table3);
    gtk_container_add (GTK_CONTAINER (frame15), table3);
    gtk_container_set_border_width (GTK_CONTAINER (table3), 2);
    gtk_table_set_row_spacings (GTK_TABLE (table3), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table3), 5);

    label11 = gtk_label_new ("Enable Sys-Ex Editing");
    gtk_widget_ref (label11);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "label11", label11,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label11);
    gtk_table_attach (GTK_TABLE (table3), label11, 0, 2, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label11), 0, 0.5);

    sysex_enable_button = gtk_check_button_new_with_label ("");
    gtk_widget_ref (sysex_enable_button);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "sysex_enable_button", sysex_enable_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (sysex_enable_button);
    gtk_table_attach (GTK_TABLE (table3), sysex_enable_button, 2, 3, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);

    sysex_channel_label = gtk_label_new ("Sys-Ex Receive Channel");
    gtk_widget_ref (sysex_channel_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "sysex_channel_label", sysex_channel_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (sysex_channel_label);
    gtk_table_attach (GTK_TABLE (table3), sysex_channel_label, 0, 2, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (sysex_channel_label), 0, 0.5);
    gtk_widget_set_sensitive (sysex_channel_label, FALSE);

    sysex_channel_spin_adj = gtk_adjustment_new (1, 1, 16, 1, 10, 0);
    sysex_channel_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sysex_channel_spin_adj), 1, 0);
    gtk_widget_ref (sysex_channel_spin);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "sysex_channel_spin", sysex_channel_spin,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (sysex_channel_spin);
    gtk_table_attach (GTK_TABLE (table3), sysex_channel_spin, 2, 3, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_widget_set_sensitive (sysex_channel_spin, FALSE);

    frame4 = gtk_frame_new (NULL);
    gtk_widget_ref (frame4);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "frame4", frame4,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (frame4);
    gtk_table_attach (GTK_TABLE (table3), frame4, 0, 3, 2, 3,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
    gtk_frame_set_shadow_type (GTK_FRAME (frame4), GTK_SHADOW_IN);

    sysex_status_label = gtk_label_new (" ");
    gtk_widget_ref (sysex_status_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "sysex_status_label", sysex_status_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (sysex_status_label);
    gtk_container_add (GTK_CONTAINER (frame4), sysex_status_label);
    gtk_label_set_line_wrap (GTK_LABEL (sysex_status_label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (sysex_status_label), 0, 0.5);

    sysex_action_label = gtk_label_new ("Edit Action");
    gtk_widget_ref (sysex_action_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "sysex_action_label", sysex_action_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (sysex_action_label);
    gtk_table_attach (GTK_TABLE (table3), sysex_action_label, 0, 1, 3, 4,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (sysex_action_label), 0, 0.5);

    sysex_discard_button = gtk_button_new_with_label ("Discard Changes");
    gtk_widget_ref (sysex_discard_button);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "sysex_discard_button", sysex_discard_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (sysex_discard_button);
    gtk_table_attach (GTK_TABLE (table3), sysex_discard_button, 1, 3, 3, 4,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_widget_set_sensitive (sysex_discard_button, FALSE);

    sysex_save_button = gtk_button_new_with_label ("Save Changes into Patch Bank");
    gtk_widget_ref (sysex_save_button);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "sysex_save_button", sysex_save_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (sysex_save_button);
    gtk_table_attach (GTK_TABLE (table3), sysex_save_button, 1, 3, 4, 5,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_widget_set_sensitive (sysex_save_button, FALSE);
#endif /* MIDI_ALSA */

    configuration_tab_label = gtk_label_new ("Configuration");
    gtk_widget_ref (configuration_tab_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "configuration_tab_label", configuration_tab_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (configuration_tab_label);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 1), configuration_tab_label);

    /* Performance tab */
    performance_frame = gtk_frame_new ("Global Performance Parameters");
    gtk_widget_ref (performance_frame);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "performance_frame", performance_frame,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (performance_frame);
    gtk_container_add (GTK_CONTAINER (notebook1), performance_frame);
    gtk_container_set_border_width (GTK_CONTAINER (performance_frame), 5);

    performance_table = gtk_table_new (2, 10, FALSE);
    gtk_widget_ref (performance_table);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "performance_table", performance_table,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (performance_table);
    gtk_container_add (GTK_CONTAINER (performance_frame), performance_table);
    gtk_container_set_border_width (GTK_CONTAINER (performance_table), 2);
    gtk_table_set_row_spacings (GTK_TABLE (performance_table), 5);
    gtk_table_set_col_spacings (GTK_TABLE (performance_table), 5);

    create_performance_spin  (main_window, "pitch bend range", performance_table, 0, PP_PITCH_BEND_RANGE, 12, 2);
    create_performance_spin  (main_window, "portamento time",  performance_table, 1, PP_PORTAMENTO_TIME, 99, 0);
    create_performance_spin  (main_window, "mod wheel sens.",  performance_table, 2, PP_MOD_WHEEL_SENSITIVITY, 15, 15);
    create_performance_assign(main_window, "mod wheel assign", performance_table, 3, PP_MOD_WHEEL_ASSIGN, 0x01);
    create_performance_spin  (main_window, "foot sensitivity", performance_table, 4, PP_FOOT_SENSITIVITY, 15, 0);
    create_performance_assign(main_window, "foot assign",      performance_table, 5, PP_FOOT_ASSIGN, 0x04);
    create_performance_spin  (main_window, "pressure sens.",   performance_table, 6, PP_PRESSURE_SENSITIVITY, 15, 15);
    create_performance_assign(main_window, "pressure assign",  performance_table, 7, PP_PRESSURE_ASSIGN, 0x02);
    create_performance_spin  (main_window, "breath sens.",     performance_table, 8, PP_BREATH_SENSITIVITY, 15, 15);
    create_performance_assign(main_window, "breath assign",    performance_table, 9, PP_BREATH_ASSIGN, 0x02);

    performance_tab_label = gtk_label_new ("Performance");
    gtk_widget_ref (performance_tab_label);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "performance_tab_label", performance_tab_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (performance_tab_label);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 2), performance_tab_label);

    /* Test note widgets */
    frame12 = gtk_frame_new ("Test Note");
    gtk_widget_ref (frame12);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "frame12", frame12,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (frame12);
    gtk_container_set_border_width (GTK_CONTAINER (frame12), 7);
    gtk_box_pack_start (GTK_BOX (vbox1), frame12, TRUE, TRUE, 0);

    table15 = gtk_table_new (2, 3, FALSE);
    gtk_widget_ref (table15);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "table15", table15,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (table15);
    gtk_container_add (GTK_CONTAINER (frame12), table15);
    gtk_container_set_border_width (GTK_CONTAINER (table15), 4);
    gtk_table_set_row_spacings (GTK_TABLE (table15), 5);
    gtk_table_set_col_spacings (GTK_TABLE (table15), 5);
 
    label10 = gtk_label_new ("key");
    gtk_widget_ref (label10);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "label10", label10,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label10);
    gtk_table_attach (GTK_TABLE (table15), label10, 0, 1, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label10), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label10), 2, 0);

    label10a = gtk_label_new ("velocity");
    gtk_widget_ref (label10a);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "label10a", label10a,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label10a);
    gtk_table_attach (GTK_TABLE (table15), label10a, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label10a), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label10a), 2, 0);

    test_note_button = gtk_button_new_with_label ("Send Test Note");
    gtk_widget_ref (test_note_button);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "test_note_button", test_note_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_button);
    gtk_table_attach (GTK_TABLE (table15), test_note_button, 0, 2, 2, 3,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 4, 0);
    gtk_container_set_border_width (GTK_CONTAINER (test_note_button), 2);

    test_note_key = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (60, 12, 132, 1, 12, 12)));
    gtk_widget_ref (test_note_key);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "test_note_key", test_note_key,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_key);
    gtk_table_attach (GTK_TABLE (table15), test_note_key, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_scale_set_value_pos (GTK_SCALE (test_note_key), GTK_POS_RIGHT);
    gtk_scale_set_digits (GTK_SCALE (test_note_key), 0);
    gtk_range_set_update_policy (GTK_RANGE (test_note_key), GTK_UPDATE_DELAYED);

    test_note_velocity = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (84, 1, 137, 1, 10, 10)));
    gtk_widget_ref (test_note_velocity);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "test_note_velocity", test_note_velocity,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (test_note_velocity);
    gtk_table_attach (GTK_TABLE (table15), test_note_velocity, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);
    gtk_scale_set_value_pos (GTK_SCALE (test_note_velocity), GTK_POS_RIGHT);
    gtk_scale_set_digits (GTK_SCALE (test_note_velocity), 0);
    gtk_range_set_update_policy (GTK_RANGE (test_note_velocity), GTK_UPDATE_DELAYED);

    /* connect main window */
    gtk_signal_connect(GTK_OBJECT(main_window), "destroy",
                       GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (main_window), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_menu_quit_activate);
    
    /* connect menu items */
    gtk_signal_connect (GTK_OBJECT (menu_import), "activate",
                        GTK_SIGNAL_FUNC (on_menu_import_activate),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (menu_export), "activate",
                        GTK_SIGNAL_FUNC (on_menu_export_activate),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (menu_quit), "activate",
                        GTK_SIGNAL_FUNC (on_menu_quit_activate),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (menu_about), "activate",
                        GTK_SIGNAL_FUNC (on_menu_about_activate),
                        NULL);

    /* connect patch list */
    gtk_signal_connect(GTK_OBJECT(patches_clist), "select_row",
                       GTK_SIGNAL_FUNC(on_patches_selection),
                       NULL);

    /* connect synth configuration widgets */
    gtk_signal_connect (GTK_OBJECT (tuning_adj), "value_changed",
                        GTK_SIGNAL_FUNC(on_tuning_change),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (volume_adj), "value_changed",
                        GTK_SIGNAL_FUNC(on_volume_change),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (polyphony_instance_adj), "value_changed",
                        GTK_SIGNAL_FUNC(on_polyphony_change),
                        (gpointer)0);
    gtk_signal_connect (GTK_OBJECT (polyphony_global_adj), "value_changed",
                        GTK_SIGNAL_FUNC(on_polyphony_change),
                        (gpointer)1);
    gtk_signal_connect (GTK_OBJECT (mono_mode_off), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"off");
    gtk_signal_connect (GTK_OBJECT (mono_mode_on), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"on");
    gtk_signal_connect (GTK_OBJECT (mono_mode_once), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"once");
    gtk_signal_connect (GTK_OBJECT (mono_mode_both), "activate",
                        GTK_SIGNAL_FUNC (on_mono_mode_activate),
                        (gpointer)"both");
    gtk_signal_connect (GTK_OBJECT (compat059_button), "toggled",
                        GTK_SIGNAL_FUNC (on_compat059_toggled),
                        NULL);
#ifdef MIDI_ALSA
    /* connect sys-ex widgets */
    gtk_signal_connect (GTK_OBJECT (sysex_enable_button), "toggled",
                        GTK_SIGNAL_FUNC (on_sysex_enable_toggled),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (sysex_channel_spin_adj), "value_changed",
                        GTK_SIGNAL_FUNC (on_sysex_channel_change),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (sysex_discard_button), "clicked",
                        GTK_SIGNAL_FUNC (on_sysex_discard_button_press),
                        NULL);
    gtk_signal_connect (GTK_OBJECT (sysex_save_button), "clicked",
                        GTK_SIGNAL_FUNC (on_sysex_save_button_press),
                        NULL);
#endif /* MIDI_ALSA */

    /* connect test note widgets */
    gtk_signal_connect (GTK_OBJECT (gtk_range_get_adjustment (GTK_RANGE (test_note_key))),
                        "value_changed", GTK_SIGNAL_FUNC(on_test_note_slider_change),
                        (gpointer)0);
    gtk_signal_connect (GTK_OBJECT (gtk_range_get_adjustment (GTK_RANGE (test_note_velocity))),
                        "value_changed", GTK_SIGNAL_FUNC(on_test_note_slider_change),
                        (gpointer)1);
    gtk_signal_connect (GTK_OBJECT (test_note_button), "pressed",
                        GTK_SIGNAL_FUNC (on_test_note_button_press),
                        (gpointer)1);
    gtk_signal_connect (GTK_OBJECT (test_note_button), "released",
                        GTK_SIGNAL_FUNC (on_test_note_button_press),
                        (gpointer)0);

    gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);
}

void
create_about_window (const char *tag)
{
    GtkWidget *vbox2;
    GtkWidget *frame1;
    GtkWidget *about_pixmap;
    GtkWidget *closeabout;

    about_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (about_window), "about_window", about_window);
    gtk_window_set_title (GTK_WINDOW (about_window), "About hexter");
    gtk_widget_realize(about_window);  /* window must be realized for create_about_pixmap() */

    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox2);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "vbox2", vbox2,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (about_window), vbox2);

    frame1 = gtk_frame_new (NULL);
    gtk_widget_ref (frame1);
    gtk_object_set_data_full (GTK_OBJECT (main_window), "frame1", frame1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (frame1);
    gtk_box_pack_start (GTK_BOX (vbox2), frame1, FALSE, FALSE, 0);

    about_pixmap = (GtkWidget *)create_about_pixmap (about_window);
    gtk_widget_ref (about_pixmap);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "about_pixmap", about_pixmap,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (about_pixmap);
    gtk_container_add (GTK_CONTAINER (frame1), about_pixmap);
    gtk_misc_set_padding (GTK_MISC (about_pixmap), 5, 5);

    about_label = gtk_label_new ("Some message\ngoes here");
    gtk_widget_ref (about_label);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "about_label", about_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (about_label);
    gtk_box_pack_start (GTK_BOX (vbox2), about_label, FALSE, FALSE, 0);
    // gtk_label_set_line_wrap (GTK_LABEL (about_label), TRUE);
    gtk_label_set_justify (GTK_LABEL (about_label), GTK_JUSTIFY_CENTER);
    gtk_misc_set_padding (GTK_MISC (about_label), 5, 5);

    closeabout = gtk_button_new_with_label ("Dismiss");
    gtk_widget_ref (closeabout);
    gtk_object_set_data_full (GTK_OBJECT (about_window), "closeabout", closeabout,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (closeabout);
    gtk_box_pack_start (GTK_BOX (vbox2), closeabout, FALSE, FALSE, 0);

    gtk_signal_connect (GTK_OBJECT (about_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (about_window), "delete_event",
                        GTK_SIGNAL_FUNC (on_delete_event_wrapper),
                        (gpointer)on_about_dismiss);
    gtk_signal_connect (GTK_OBJECT (closeabout), "clicked",
                        GTK_SIGNAL_FUNC (on_about_dismiss),
                        NULL);
}

void
create_import_file_selection (const char *tag)
{
    char      *title;
    GtkWidget *ok_button;
    GtkWidget *cancel_button;

    title = (char *)malloc(strlen(tag) + 21);
    sprintf(title, "%s - Import Patch Bank", tag);
    import_file_selection = gtk_file_selection_new (title);
    free(title);
    gtk_object_set_data (GTK_OBJECT (import_file_selection), "import_file_selection", import_file_selection);
    gtk_container_set_border_width (GTK_CONTAINER (import_file_selection), 10);

    ok_button = GTK_FILE_SELECTION (import_file_selection)->ok_button;
    gtk_object_set_data (GTK_OBJECT (import_file_selection), "ok_button", ok_button);
    gtk_widget_show (ok_button);
    GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);

    cancel_button = GTK_FILE_SELECTION (import_file_selection)->cancel_button;
    gtk_object_set_data (GTK_OBJECT (import_file_selection), "cancel_button", cancel_button);
    gtk_widget_show (cancel_button);
    GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);

    gtk_signal_connect (GTK_OBJECT (import_file_selection), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (import_file_selection), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_import_file_cancel);
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (import_file_selection)->ok_button),
                        "clicked", (GtkSignalFunc)on_import_file_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (import_file_selection)->cancel_button),
                        "clicked", (GtkSignalFunc)on_import_file_cancel,
                        NULL);
}

void
create_import_file_position_window (const char *tag)
{
    GtkWidget *vbox4;
    GtkWidget *position_text_label;
    GtkWidget *hbox2;
    GtkWidget *label50;
    GtkWidget *position_spin;
    GtkWidget *hbox3;
    GtkWidget *position_cancel;
    GtkWidget *position_ok;

    import_file_position_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (import_file_position_window),
                         "import_file_position_window", import_file_position_window);
    set_window_title(import_file_position_window, tag, "Import Position");

    vbox4 = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox4);
    gtk_object_set_data_full (GTK_OBJECT (import_file_position_window), "vbox4",
                              vbox4, (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox4);
    gtk_container_add (GTK_CONTAINER (import_file_position_window), vbox4);
    gtk_container_set_border_width (GTK_CONTAINER (vbox4), 6);

    position_text_label = gtk_label_new ("Select the Program Number at which you "
                                         "wish to begin importing patches (existing "
                                         "patches will be overwritten)");
    gtk_widget_ref (position_text_label);
    gtk_object_set_data_full (GTK_OBJECT (import_file_position_window),
                              "position_text_label", position_text_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (position_text_label);
    gtk_box_pack_start (GTK_BOX (vbox4), position_text_label, TRUE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (position_text_label), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap (GTK_LABEL (position_text_label), TRUE);
    gtk_misc_set_padding (GTK_MISC (position_text_label), 0, 6);

    hbox2 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox2);
    gtk_object_set_data_full (GTK_OBJECT (import_file_position_window), "hbox2",
                              hbox2, (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox2);
    gtk_box_pack_start (GTK_BOX (vbox4), hbox2, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox2), 6);

    label50 = gtk_label_new ("Program Number");
    gtk_widget_ref (label50);
    gtk_object_set_data_full (GTK_OBJECT (import_file_position_window), "label50",
                              label50, (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label50);
    gtk_box_pack_start (GTK_BOX (hbox2), label50, FALSE, TRUE, 2);

    import_file_position_spin_adj = gtk_adjustment_new (0, 0, 127, 1, 10, 0);
    position_spin = gtk_spin_button_new (GTK_ADJUSTMENT (import_file_position_spin_adj), 1, 0);
    gtk_widget_ref (position_spin);
    gtk_object_set_data_full (GTK_OBJECT (import_file_position_window),
                              "position_spin", position_spin,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (position_spin);
    gtk_box_pack_start (GTK_BOX (hbox2), position_spin, FALSE, FALSE, 2);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (position_spin), TRUE);

    import_file_position_name_label = gtk_label_new ("default voice");
    gtk_widget_ref (import_file_position_name_label);
    gtk_object_set_data_full (GTK_OBJECT (import_file_position_window),
                              "import_file_position_name_label",
                              import_file_position_name_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (import_file_position_name_label);
    gtk_box_pack_start (GTK_BOX (hbox2), import_file_position_name_label, FALSE, FALSE, 2);
    gtk_label_set_justify (GTK_LABEL (import_file_position_name_label), GTK_JUSTIFY_LEFT);

    hbox3 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox3);
    gtk_object_set_data_full (GTK_OBJECT (import_file_position_window), "hbox3", hbox3,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox3);
    gtk_box_pack_start (GTK_BOX (vbox4), hbox3, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox3), 6);

    position_cancel = gtk_button_new_with_label ("Cancel");
    gtk_widget_ref (position_cancel);
    gtk_object_set_data_full (GTK_OBJECT (import_file_position_window),
                              "position_cancel", position_cancel,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (position_cancel);
    gtk_box_pack_start (GTK_BOX (hbox3), position_cancel, TRUE, FALSE, 12);

    position_ok = gtk_button_new_with_label ("Import");
    gtk_widget_ref (position_ok);
    gtk_object_set_data_full (GTK_OBJECT (import_file_position_window),
                              "position_ok", position_ok,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (position_ok);
    gtk_box_pack_end (GTK_BOX (hbox3), position_ok, TRUE, FALSE, 12);

    gtk_signal_connect (GTK_OBJECT (import_file_position_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (import_file_position_window), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_import_file_position_cancel);
    gtk_signal_connect (GTK_OBJECT (import_file_position_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_position_change),
                        (gpointer)import_file_position_name_label);
    gtk_signal_connect (GTK_OBJECT (position_ok), "clicked",
                        (GtkSignalFunc)on_import_file_position_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (position_cancel), "clicked",
                        (GtkSignalFunc)on_import_file_position_cancel,
                        NULL);
}

void
create_notice_window (const char *tag)
{
    GtkWidget *vbox3;
    GtkWidget *hbox1;
    GtkWidget *notice_dismiss;

    notice_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (notice_window), "notice_window", notice_window);
    set_window_title(notice_window, tag, "Notice");
    gtk_window_set_position (GTK_WINDOW (notice_window), GTK_WIN_POS_MOUSE);
    gtk_window_set_modal (GTK_WINDOW (notice_window), TRUE);

    vbox3 = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox3);
    gtk_object_set_data_full (GTK_OBJECT (notice_window), "vbox3", vbox3,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox3);
    gtk_container_add (GTK_CONTAINER (notice_window), vbox3);

    notice_label_1 = gtk_label_new ("Some message\ngoes here");
    gtk_widget_ref (notice_label_1);
    gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_label_1", notice_label_1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (notice_label_1);
    gtk_box_pack_start (GTK_BOX (vbox3), notice_label_1, TRUE, TRUE, 0);
    gtk_label_set_line_wrap (GTK_LABEL (notice_label_1), TRUE);
    gtk_misc_set_padding (GTK_MISC (notice_label_1), 10, 5);

    notice_label_2 = gtk_label_new ("more text\ngoes here");
    gtk_widget_ref (notice_label_2);
    gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_label_2", notice_label_2,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (notice_label_2);
    gtk_box_pack_start (GTK_BOX (vbox3), notice_label_2, FALSE, FALSE, 0);
    gtk_label_set_line_wrap (GTK_LABEL (notice_label_2), TRUE);
    gtk_misc_set_padding (GTK_MISC (notice_label_2), 10, 5);

    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox1);
    gtk_object_set_data_full (GTK_OBJECT (notice_window), "hbox1", hbox1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox1);
    gtk_box_pack_start (GTK_BOX (vbox3), hbox1, FALSE, FALSE, 0);

    notice_dismiss = gtk_button_new_with_label ("Dismiss");
    gtk_widget_ref (notice_dismiss);
    gtk_object_set_data_full (GTK_OBJECT (notice_window), "notice_dismiss", notice_dismiss,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (notice_dismiss);
    gtk_box_pack_start (GTK_BOX (hbox1), notice_dismiss, TRUE, FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (notice_dismiss), 7);

    gtk_signal_connect (GTK_OBJECT (notice_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (notice_window), "delete_event",
                        GTK_SIGNAL_FUNC (on_delete_event_wrapper),
                        (gpointer)on_notice_dismiss);
    gtk_signal_connect (GTK_OBJECT (notice_dismiss), "clicked",
                        GTK_SIGNAL_FUNC (on_notice_dismiss),
                        NULL);
}

void
create_export_file_type_window (const char *tag)
{
    GtkWidget *vbox1;
    GtkWidget *label1;
    GtkWidget *table1;
    GtkWidget *label2;
    GtkWidget *label3;
    GSList *save_type_group = NULL;
    GtkWidget *save_type_raw;
    GtkWidget *hseparator1;
    GtkWidget *label4;
    GtkWidget *table2;
    GtkWidget *label5;
    GtkWidget *export_file_start_spin;
    GtkWidget *hseparator2;
    GtkWidget *hbox3;
    GtkWidget *sft_position_cancel;
    GtkWidget *sft_position_ok;

    export_file_type_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (export_file_type_window), "export_file_type_window",
                         export_file_type_window);
    set_window_title(export_file_type_window, tag, "Export File Type");

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox1);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "vbox1", vbox1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox1);
    gtk_container_add (GTK_CONTAINER (export_file_type_window), vbox1);
    gtk_container_set_border_width (GTK_CONTAINER (vbox1), 6);

    label1 = gtk_label_new ("Select the patch file format you wish to use for this export:");
    gtk_widget_ref (label1);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "label1", label1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label1);
    gtk_box_pack_start (GTK_BOX (vbox1), label1, TRUE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap (GTK_LABEL (label1), TRUE);
    gtk_misc_set_padding (GTK_MISC (label1), 0, 6);

    table1 = gtk_table_new (2, 2, FALSE);
    gtk_widget_ref (table1);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "table1", table1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (table1);
    gtk_box_pack_start (GTK_BOX (vbox1), table1, FALSE, FALSE, 0);

    label2 = gtk_label_new ("(32 patches, 4104 bytes, any suffix)");
    gtk_widget_ref (label2);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "label2", label2,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label2);
    gtk_table_attach (GTK_TABLE (table1), label2, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label2), 5, 0);

    label3 = gtk_label_new ("(any number of patches, 128 bytes per patch, hexter expects\".dx7\" suffix)");
    gtk_widget_ref (label3);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "label3", label3,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label3);
    gtk_table_attach (GTK_TABLE (table1), label3, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_line_wrap (GTK_LABEL (label3), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);
    gtk_misc_set_padding (GTK_MISC (label3), 5, 0);

    export_file_type_sysex = gtk_radio_button_new_with_label (save_type_group, "DX7 sys-ex file");
    save_type_group = gtk_radio_button_group (GTK_RADIO_BUTTON (export_file_type_sysex));
    gtk_widget_ref (export_file_type_sysex);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "export_file_type_sysex", export_file_type_sysex,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (export_file_type_sysex);
    gtk_table_attach (GTK_TABLE (table1), export_file_type_sysex, 0, 1, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 4, 4);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (export_file_type_sysex), TRUE);

    save_type_raw = gtk_radio_button_new_with_label (save_type_group, "raw DX7 patch file");
    save_type_group = gtk_radio_button_group (GTK_RADIO_BUTTON (save_type_raw));
    gtk_widget_ref (save_type_raw);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "save_type_raw", save_type_raw,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (save_type_raw);
    gtk_table_attach (GTK_TABLE (table1), save_type_raw, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 4, 4);

    hseparator1 = gtk_hseparator_new ();
    gtk_widget_ref (hseparator1);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "hseparator1", hseparator1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hseparator1);
    gtk_box_pack_start (GTK_BOX (vbox1), hseparator1, FALSE, FALSE, 2);

    label4 = gtk_label_new ("Select the program numbers for the range of patches you wish to export:");
    gtk_widget_ref (label4);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "label4", label4,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label4);
    gtk_box_pack_start (GTK_BOX (vbox1), label4, TRUE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (label4), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap (GTK_LABEL (label4), TRUE);
    gtk_misc_set_padding (GTK_MISC (label4), 0, 6);

    table2 = gtk_table_new (2, 3, FALSE);
    gtk_widget_ref (table2);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "table2", table2,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (table2);
    gtk_box_pack_start (GTK_BOX (vbox1), table2, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (table2), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table2), 2);

    label5 = gtk_label_new ("Start Program Number");
    gtk_widget_ref (label5);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "label5", label5,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label5);
    gtk_table_attach (GTK_TABLE (table2), label5, 0, 1, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

    export_file_end_label = gtk_label_new ("End Program (inclusive)");
    gtk_widget_ref (export_file_end_label);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "export_file_end_label", export_file_end_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (export_file_end_label);
    gtk_table_attach (GTK_TABLE (table2), export_file_end_label, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (export_file_end_label), 0, 0.5);
      gtk_widget_set_sensitive (export_file_end_label, FALSE);

    export_file_start_spin_adj = gtk_adjustment_new (0, 0, 96, 1, 10, 0);
    export_file_start_spin = gtk_spin_button_new (GTK_ADJUSTMENT (export_file_start_spin_adj), 1, 0);
    gtk_widget_ref (export_file_start_spin);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "export_file_start_spin", export_file_start_spin,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (export_file_start_spin);
    gtk_table_attach (GTK_TABLE (table2), export_file_start_spin, 1, 2, 0, 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (export_file_start_spin), TRUE);

    export_file_end_spin_adj = gtk_adjustment_new (31, 0, 127, 1, 10, 0);
    export_file_end_spin = gtk_spin_button_new (GTK_ADJUSTMENT (export_file_end_spin_adj), 1, 0);
    gtk_widget_ref (export_file_end_spin);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "export_file_end_spin", export_file_end_spin,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (export_file_end_spin);
    gtk_table_attach (GTK_TABLE (table2), export_file_end_spin, 1, 2, 1, 2,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (export_file_end_spin), TRUE);
    gtk_widget_set_sensitive (export_file_end_spin, FALSE);

    export_file_start_name = gtk_label_new ("(unset)");
    gtk_widget_ref (export_file_start_name);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "export_file_start_name", export_file_start_name,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (export_file_start_name);
    gtk_table_attach (GTK_TABLE (table2), export_file_start_name, 2, 3, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify (GTK_LABEL (export_file_start_name), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (export_file_start_name), 0, 0.5);

    export_file_end_name = gtk_label_new ("(unset)");
    gtk_widget_ref (export_file_end_name);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "export_file_end_name", export_file_end_name,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (export_file_end_name);
    gtk_table_attach (GTK_TABLE (table2), export_file_end_name, 2, 3, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (export_file_end_name), 0, 0.5);

    hseparator2 = gtk_hseparator_new ();
    gtk_widget_ref (hseparator2);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "hseparator2", hseparator2,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hseparator2);
    gtk_box_pack_start (GTK_BOX (vbox1), hseparator2, FALSE, FALSE, 2);

    hbox3 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox3);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window), "hbox3", hbox3,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox3);
    gtk_box_pack_start (GTK_BOX (vbox1), hbox3, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox3), 6);

    sft_position_cancel = gtk_button_new_with_label ("Cancel");
    gtk_widget_ref (sft_position_cancel);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window),
                              "sft_position_cancel", sft_position_cancel,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (sft_position_cancel);
    gtk_box_pack_start (GTK_BOX (hbox3), sft_position_cancel, TRUE, FALSE, 12);

    sft_position_ok = gtk_button_new_with_label ("Export");
    gtk_widget_ref (sft_position_ok);
    gtk_object_set_data_full (GTK_OBJECT (export_file_type_window),
                              "sft_position_ok", sft_position_ok,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (sft_position_ok);
    gtk_box_pack_end (GTK_BOX (hbox3), sft_position_ok, TRUE, FALSE, 12);

    gtk_signal_connect (GTK_OBJECT (export_file_type_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (export_file_type_window), "delete_event",
                        GTK_SIGNAL_FUNC (on_delete_event_wrapper),
                        (gpointer)on_export_file_type_cancel);
    gtk_signal_connect (GTK_OBJECT (export_file_type_sysex), "pressed",
                        GTK_SIGNAL_FUNC (on_export_file_type_press),
                        (gpointer)0);
    gtk_signal_connect (GTK_OBJECT (save_type_raw), "pressed",
                        GTK_SIGNAL_FUNC (on_export_file_type_press),
                        (gpointer)1);
    gtk_signal_connect (GTK_OBJECT (export_file_start_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_export_file_position_change),
                        (gpointer)0);
    gtk_signal_connect (GTK_OBJECT (export_file_end_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_export_file_position_change),
                        (gpointer)1);
    gtk_signal_connect (GTK_OBJECT (sft_position_ok), "clicked",
                        (GtkSignalFunc)on_export_file_type_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (sft_position_cancel), "clicked",
                        (GtkSignalFunc)on_export_file_type_cancel,
                        NULL);
}

void
create_export_file_selection (const char *tag)
{
    char      *title;
    GtkWidget *ok_button;
    GtkWidget *cancel_button;

    title = (char *)malloc(strlen(tag) + 21);
    sprintf(title, "%s - Export Patch Bank", tag);
    export_file_selection = gtk_file_selection_new (title);
    free(title);
    gtk_object_set_data (GTK_OBJECT (export_file_selection), "export_file_selection", export_file_selection);
    gtk_container_set_border_width (GTK_CONTAINER (export_file_selection), 10);

    ok_button = GTK_FILE_SELECTION (export_file_selection)->ok_button;
    gtk_object_set_data (GTK_OBJECT (export_file_selection), "ok_button", ok_button);
    gtk_widget_show (ok_button);
    GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);

    cancel_button = GTK_FILE_SELECTION (export_file_selection)->cancel_button;
    gtk_object_set_data (GTK_OBJECT (export_file_selection), "cancel_button", cancel_button);
    gtk_widget_show (cancel_button);
    GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);

    gtk_signal_connect (GTK_OBJECT (export_file_selection), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (export_file_selection), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_export_file_cancel);
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (export_file_selection)->ok_button),
                        "clicked", (GtkSignalFunc)on_export_file_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (export_file_selection)->cancel_button),
                        "clicked", (GtkSignalFunc)on_export_file_cancel,
                        NULL);
}

void
create_edit_save_position_window (const char *tag)
{
    GtkWidget *vbox4;
    GtkWidget *hbox2;
    GtkWidget *edit_save_position_text_label;
    GtkWidget *label50;
    GtkWidget *position_spin;
    GtkWidget *hbox3;
    GtkWidget *position_cancel;
    GtkWidget *position_ok;
 
    edit_save_position_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_object_set_data (GTK_OBJECT (edit_save_position_window),
                         "edit_save_position_window", edit_save_position_window);
    set_window_title(edit_save_position_window, tag, "Edit Save Position");

    vbox4 = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox4);
    gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "vbox4",
                              vbox4, (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (vbox4);
    gtk_container_add (GTK_CONTAINER (edit_save_position_window), vbox4);
    gtk_container_set_border_width (GTK_CONTAINER (vbox4), 6);

    edit_save_position_text_label = gtk_label_new ("Select the Program Number into which you "
                                                   "wish to save the edited patch "
                                                   "(the existing patch will be overwritten):");
    gtk_widget_ref (edit_save_position_text_label);
    gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                              "edit_save_position_text_label", edit_save_position_text_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (edit_save_position_text_label);
    gtk_box_pack_start (GTK_BOX (vbox4), edit_save_position_text_label, TRUE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (edit_save_position_text_label), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap (GTK_LABEL (edit_save_position_text_label), TRUE);
    gtk_misc_set_padding (GTK_MISC (edit_save_position_text_label), 0, 6);

    hbox2 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox2);
    gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "hbox2",
                              hbox2, (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox2);
    gtk_box_pack_start (GTK_BOX (vbox4), hbox2, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox2), 6);

    label50 = gtk_label_new ("Program Number");
    gtk_widget_ref (label50);
    gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "label50",
                              label50, (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (label50);
    gtk_box_pack_start (GTK_BOX (hbox2), label50, FALSE, TRUE, 2);

    edit_save_position_spin_adj = gtk_adjustment_new (0, 0, 127, 1, 10, 0);
    position_spin = gtk_spin_button_new (GTK_ADJUSTMENT (edit_save_position_spin_adj), 1, 0);
    gtk_widget_ref (position_spin);
    gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                              "position_spin", position_spin,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (position_spin);
    gtk_box_pack_start (GTK_BOX (hbox2), position_spin, FALSE, FALSE, 2);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (position_spin), TRUE);
 
    edit_save_position_name_label = gtk_label_new ("default voice");
    gtk_widget_ref (edit_save_position_name_label);
    gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                              "edit_save_position_name_label",
                              edit_save_position_name_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (edit_save_position_name_label);
    gtk_box_pack_start (GTK_BOX (hbox2), edit_save_position_name_label, FALSE, FALSE, 2);
    gtk_label_set_justify (GTK_LABEL (edit_save_position_name_label), GTK_JUSTIFY_LEFT);
 
    hbox3 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox3);
    gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window), "hbox3", hbox3,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (hbox3);
    gtk_box_pack_start (GTK_BOX (vbox4), hbox3, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox3), 6);
 
    position_cancel = gtk_button_new_with_label ("Cancel");
    gtk_widget_ref (position_cancel);
    gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                              "position_cancel", position_cancel,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (position_cancel);
    gtk_box_pack_start (GTK_BOX (hbox3), position_cancel, TRUE, FALSE, 12);
 
    position_ok = gtk_button_new_with_label ("Save");
    gtk_widget_ref (position_ok);
    gtk_object_set_data_full (GTK_OBJECT (edit_save_position_window),
                              "position_ok", position_ok,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show (position_ok);
    gtk_box_pack_end (GTK_BOX (hbox3), position_ok, TRUE, FALSE, 12);
 
    gtk_signal_connect (GTK_OBJECT (edit_save_position_window), "destroy",
                        GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    gtk_signal_connect (GTK_OBJECT (edit_save_position_window), "delete_event",
                        (GtkSignalFunc)on_delete_event_wrapper,
                        (gpointer)on_edit_save_position_cancel);
    gtk_signal_connect (GTK_OBJECT (edit_save_position_spin_adj),
                        "value_changed", GTK_SIGNAL_FUNC(on_position_change),
                        (gpointer)edit_save_position_name_label);
    gtk_signal_connect (GTK_OBJECT (position_ok), "clicked",
                        (GtkSignalFunc)on_edit_save_position_ok,
                        NULL);
    gtk_signal_connect (GTK_OBJECT (position_cancel), "clicked",
                        (GtkSignalFunc)on_edit_save_position_cancel,
                        NULL);
}

void
create_windows(const char *instance_tag)
{
    char tag[50];

    /* build a nice identifier string for the window titles */
    if (strlen(instance_tag) == 0) {
        strcpy(tag, "hexter v" VERSION);
    } else if (strstr(instance_tag, "hexter")) {
        if (strlen(instance_tag) > 49) {
            snprintf(tag, 50, "...%s", instance_tag + strlen(instance_tag) - 46); /* hope the unique info is at the end */
        } else {
            strcpy(tag, instance_tag);
        }
    } else {
        if (strlen(instance_tag) > 42) {
            snprintf(tag, 50, "hexter ...%s", instance_tag + strlen(instance_tag) - 39);
        } else {
            snprintf(tag, 50, "hexter %s", instance_tag);
        }
    }

    create_main_window(tag);
    create_about_window(tag);
    create_import_file_selection(tag);
    create_import_file_position_window(tag);
    create_export_file_type_window(tag);
    create_export_file_selection(tag);
    create_edit_save_position_window(tag);
    create_notice_window(tag);
}

