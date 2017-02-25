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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>
#include <lo/lo.h>
#include <dssi.h>

#include "hexter_types.h"
#include "hexter.h"
#include "dx7_voice.h"
#include "dx7_voice_data.h"
#include "gui_main.h"
#include "gui_callbacks.h"
#include "gui_data.h"
#include "gui_interface.h"
#include "gui_midi.h"
#include "gui_patch_edit.h"

static int internal_gui_update_only = 0;

static unsigned char test_note_noteon_key = 60;
static int           test_note_noteoff_key = -1;
static unsigned char test_note_velocity = 84;

static int export_file_type;
static int export_file_start;
static int export_file_end;

static gchar *file_selection_last_filename = NULL;
extern char  *project_directory;

void
file_selection_set_path(GtkWidget *file_selection)
{
    if (file_selection_last_filename) {
        gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection),
                                        file_selection_last_filename);
    } else if (project_directory && strlen(project_directory)) {
        if (project_directory[strlen(project_directory) - 1] != '/') {
            char buffer[PATH_MAX];
            snprintf(buffer, PATH_MAX, "%s/", project_directory);
            gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection),
                                            buffer);
        } else {
            gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection),
                                            project_directory);
        }
    }
}

void
on_menu_import_activate               (GtkMenuItem     *menuitem,
                                       gpointer         user_data)
{
    gtk_widget_hide(import_file_position_window);
    gtk_widget_hide(export_file_type_window);
    gtk_widget_hide(export_file_selection);
    file_selection_set_path(import_file_selection);
    gtk_widget_show(import_file_selection);
}


void
on_menu_export_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_widget_hide(import_file_selection);
    gtk_widget_hide(import_file_position_window);
    gtk_widget_hide(export_file_selection);
    gtk_signal_emit_by_name (GTK_OBJECT (export_file_start_spin_adj), "value_changed");
    gtk_signal_emit_by_name (GTK_OBJECT (export_file_end_spin_adj), "value_changed");
    gtk_widget_show(export_file_type_window);
}


void
on_menu_quit_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    gtk_main_quit();
}


void
on_menu_edit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
    if (!GTK_WIDGET_MAPPED(editor_window))
        gtk_widget_show(editor_window);
    else
        gdk_window_raise(editor_window->window);
}


void
on_menu_about_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    char buf[512];
    snprintf(buf, 512, "This program licensed under the GNU General Public License v2.\n"
                       "See the enclosed file COPYING for details. NO WARRANTY.\n\n"
                       "hexter version: " VERSION "\n"
                       "hexter URL: %s\n"
                       "host URL: %s\n", osc_self_url, osc_host_url);
    /* -FIX- include credits for Jamie Bullock and Martin Tarenskeen */
    gtk_label_set_text (GTK_LABEL (about_label), buf);
    gtk_widget_show(about_window);
}

gint
on_delete_event_wrapper( GtkWidget *widget, GdkEvent *event, gpointer data )
{
    void (*handler)(GtkWidget *, gpointer) = (void (*)(GtkWidget *, gpointer))data;

    /* call our 'close', 'dismiss' or 'cancel' callback (which must not need the user data) */
    (*handler)(widget, NULL);

    /* tell GTK+ to NOT emit 'destroy' */
    return TRUE;
}

void
on_import_file_ok( GtkWidget *widget, gpointer data )
{
    gtk_widget_hide(import_file_selection);
    file_selection_last_filename = (gchar *)gtk_file_selection_get_filename(
                                                GTK_FILE_SELECTION(import_file_selection));

    GUIDB_MESSAGE(DB_GUI, " on_import_file_ok: file '%s' selected\n",
                    file_selection_last_filename);

    /* update patch name */
    gtk_signal_emit_by_name (GTK_OBJECT (import_file_position_spin_adj), "value_changed");

    gtk_widget_show(import_file_position_window);
}

void
on_import_file_cancel( GtkWidget *widget, gpointer data )
{
    GUIDB_MESSAGE(DB_GUI, ": on_import_file_cancel called\n");
    gtk_widget_hide(import_file_selection);
}

/*
 * on_position_change
 *
 * used by the both import file position and edit save position dialogs
 * data is a pointer to the dialog's patch name label
 */
void
on_position_change(GtkWidget *widget, gpointer data)
{
    char name[21];

    int position = lrintf(GTK_ADJUSTMENT(widget)->value);
    GtkWidget *label = (GtkWidget *)data;

    patch_edit_copy_name_to_utf8(name, patches[position].data, TRUE);
    gtk_label_set_text (GTK_LABEL (label), name);
}

void
on_import_file_position_ok( GtkWidget *widget, gpointer data )
{
    int position = lrintf(GTK_ADJUSTMENT(import_file_position_spin_adj)->value);
    char *message;

    gtk_widget_hide(import_file_position_window);

    GUIDB_MESSAGE(DB_GUI, " on_import_file_position_ok: position %d\n", position);

    if (gui_data_load(file_selection_last_filename, position, &message)) {

        /* successfully loaded at least one patch */
        rebuild_patches_clist();
        if (!edit_buffer_active) {
            dx7_patch_unpack(patches, edit_buffer.program, edit_buffer.voice);
            /* set all the patch edit widgets to match */
            patch_edit_update_editors();
        }
        display_notice("Load Patch File succeeded:", message);
        gui_data_send_dirty_patch_sections();

    } else {  /* didn't load anything successfully */

        display_notice("Load Patch File failed:", message);

    }
    free(message);
}

void
on_import_file_position_cancel( GtkWidget *widget, gpointer data )
{
    GUIDB_MESSAGE(DB_GUI, ": on_import_file_position_cancel called\n");
    gtk_widget_hide(import_file_position_window);
}

void
on_export_file_type_press( GtkWidget *widget, gpointer data )
{
    int type = (int)data;
    int start, end;

    GUIDB_MESSAGE(DB_GUI, ": on_export_file_type_press called with %d\n", type);

    if (type == 0) { /* sys-ex */

        start = lrintf(GTK_ADJUSTMENT(export_file_start_spin_adj)->value);
        end   = lrintf(GTK_ADJUSTMENT(export_file_end_spin_adj)->value);
        (GTK_ADJUSTMENT(export_file_start_spin_adj))->upper = 96.0f;
        gtk_signal_emit_by_name (GTK_OBJECT (export_file_start_spin_adj), "changed");
        if (start > 96) {
            (GTK_ADJUSTMENT(export_file_start_spin_adj))->value = 96.0f;
            (GTK_ADJUSTMENT(export_file_end_spin_adj))->value = 127.0f;
            gtk_signal_emit_by_name (GTK_OBJECT (export_file_start_spin_adj), "value_changed");
            gtk_signal_emit_by_name (GTK_OBJECT (export_file_end_spin_adj), "value_changed");
        } else if (end != start + 31) {
            (GTK_ADJUSTMENT(export_file_end_spin_adj))->value = (float)(start + 31);
            gtk_signal_emit_by_name (GTK_OBJECT (export_file_end_spin_adj), "value_changed");
        }

        gtk_widget_set_sensitive (export_file_end_label, FALSE);
        gtk_widget_set_sensitive (export_file_end_spin, FALSE);

    } else { /* raw */
        (GTK_ADJUSTMENT(export_file_start_spin_adj))->upper = 127.0f;
        gtk_signal_emit_by_name (GTK_OBJECT (export_file_start_spin_adj), "changed");
        gtk_widget_set_sensitive (export_file_end_label, TRUE);
        gtk_widget_set_sensitive (export_file_end_spin, TRUE);
    }
}

/*
 * on_export_file_position_change
 */
void
on_export_file_position_change(GtkWidget *widget, gpointer data)
{
    int which = (int)data;
    int type = GTK_TOGGLE_BUTTON (export_file_type_sysex)->active ? 0 : 1;
    int start = lrintf(GTK_ADJUSTMENT(export_file_start_spin_adj)->value);
    int end   = lrintf(GTK_ADJUSTMENT(export_file_end_spin_adj)->value);
    char name[21];

    if (which == 0) {  /* start */
        if (type == 0) {  /* sys-ex */
            if (end != start + 31) {
                (GTK_ADJUSTMENT(export_file_end_spin_adj))->value = (float)(start + 31);
                gtk_signal_emit_by_name (GTK_OBJECT (export_file_end_spin_adj), "value_changed");
            }
        } else {  /* raw */
            if (end < start) {
                (GTK_ADJUSTMENT(export_file_end_spin_adj))->value = (float)start;
                gtk_signal_emit_by_name (GTK_OBJECT (export_file_end_spin_adj), "value_changed");
            }
        }
        patch_edit_copy_name_to_utf8(name, patches[start].data, TRUE);
        gtk_label_set_text (GTK_LABEL (export_file_start_name), name);
    } else { /* end */
        if (type == 1) {  /* raw */
            if (end < start) {
                (GTK_ADJUSTMENT(export_file_start_spin_adj))->value = (float)end;
                gtk_signal_emit_by_name (GTK_OBJECT (export_file_start_spin_adj), "value_changed");
            }
        }
        patch_edit_copy_name_to_utf8(name, patches[end].data, TRUE);
        gtk_label_set_text (GTK_LABEL (export_file_end_name), name);
    }
}

void
on_export_file_type_ok( GtkWidget *widget, gpointer data )
{
    export_file_type  = GTK_TOGGLE_BUTTON (export_file_type_sysex)->active ? 0 : 1;
    export_file_start = lrintf(GTK_ADJUSTMENT(export_file_start_spin_adj)->value);
    export_file_end   = lrintf(GTK_ADJUSTMENT(export_file_end_spin_adj)->value);

    GUIDB_MESSAGE(DB_GUI, " on_export_file_type_ok: type %s, start %d, end %d\n",
                  export_file_type == 0 ? "sys-ex" : "raw", export_file_start, export_file_end);

    gtk_widget_hide(export_file_type_window);
    file_selection_set_path(export_file_selection);
    gtk_widget_show(export_file_selection);
}

void
on_export_file_type_cancel( GtkWidget *widget, gpointer data )
{
    GUIDB_MESSAGE(DB_GUI, ": on_export_file_type_cancel called\n");
    gtk_widget_hide(export_file_type_window);
}

void
on_export_file_ok( GtkWidget *widget, gpointer data )
{
    char *message;

    gtk_widget_hide(export_file_selection);
    file_selection_last_filename = (gchar *)gtk_file_selection_get_filename(
                                                GTK_FILE_SELECTION(export_file_selection));

    GUIDB_MESSAGE(DB_GUI, " on_export_file_ok: file '%s' selected\n",
                    file_selection_last_filename);

    if (gui_data_save(file_selection_last_filename, export_file_type,
                      export_file_start, export_file_end, &message)) {

        display_notice("Save Patch File succeeded:", message);

    } else {  /* problem with save */

        display_notice("Save Patch File failed:", message);

    }
    free(message);
}

void
on_export_file_cancel( GtkWidget *widget, gpointer data )
{
    GUIDB_MESSAGE(DB_GUI, ": on_export_file_cancel called\n");
    gtk_widget_hide(export_file_selection);
}

void
on_about_dismiss( GtkWidget *widget, gpointer data )
{
    gtk_widget_hide(about_window);
}

void
on_patches_selection(GtkWidget      *clist,
                     gint            row,
                     gint            column,
                     GdkEventButton *event,
                     gpointer        data )
{
    if (internal_gui_update_only) {
        /* GUIDB_MESSAGE(DB_GUI, " on_patches_selection: skipping further action\n"); */
        return;
    }

    current_program = row;

    GUIDB_MESSAGE(DB_GUI, " on_patches_selection: patch %d selected\n", current_program);

    if (current_program != edit_buffer.program) {
        if (edit_buffer_active) {
            gui_data_send_edit_buffer_off();
            edit_buffer_active = FALSE;
        }
        edit_buffer.program = current_program;
        dx7_patch_unpack(patches, edit_buffer.program, edit_buffer.voice);
        /* set all the patch edit widgets to match */
        patch_edit_update_editors(); /* also updates status */
    }
    lo_send(osc_host_address, osc_program_path, "ii", 0, current_program);
}

void
on_tuning_change( GtkWidget *widget, gpointer data )
{
    float value = GTK_ADJUSTMENT(widget)->value;

    if (internal_gui_update_only) {
        /* GUIDB_MESSAGE(DB_GUI, " on_tuning_change: skipping further action\n"); */
        return;
    }

    GUIDB_MESSAGE(DB_GUI, " on_tuning_change: tuning set to %10.6f\n", value);

    lo_send(osc_host_address, osc_control_path, "if", HEXTER_PORT_TUNING, value);
}

void
on_volume_change( GtkWidget *widget, gpointer data )
{
    float value = GTK_ADJUSTMENT(widget)->value;

    if (internal_gui_update_only) {
        /* GUIDB_MESSAGE(DB_GUI, " on_volume_change: skipping further action\n"); */
        return;
    }

    GUIDB_MESSAGE(DB_GUI, " on_volume_change: volume set to %10.6f\n", value);

    lo_send(osc_host_address, osc_control_path, "if", HEXTER_PORT_VOLUME, value);
}

void
on_polyphony_change(GtkWidget *widget, gpointer data)
{
    int polyphony = lrintf(GTK_ADJUSTMENT(widget)->value);
    char buffer[4];

    if (internal_gui_update_only) {
        /* GUIDB_MESSAGE(DB_GUI, " on_polyphony_change: skipping further action\n"); */
        return;
    }

    GUIDB_MESSAGE(DB_GUI, " on_polyphony_change: polyphony set to %d\n", polyphony);

    snprintf(buffer, 4, "%d", polyphony);
    lo_send(osc_host_address, osc_configure_path, "ss", "polyphony", buffer);
}

void
on_mono_mode_activate(GtkWidget *widget, gpointer data)
{
    char *mode = data;

    GUIDB_MESSAGE(DB_GUI, " on_mono_mode_activate: monophonic mode '%s' selected\n", mode);

    lo_send(osc_host_address, osc_configure_path, "ss", "monophonic", mode);
}

void
on_compat059_toggled(GtkWidget *widget, gpointer data)
{
    int state = GTK_TOGGLE_BUTTON (widget)->active;

    gtk_widget_set_sensitive (performance_frame, !state);

    if (internal_gui_update_only) {
        /* GUIDB_MESSAGE(DB_GUI, " on_compat059_toggled: skipping further action\n"); */
        return;
    }

    GUIDB_MESSAGE(DB_GUI, " on_compat059_toggled: button now %s\n",
                  (state ? "on" : "off"));

    send_performance();
}

#ifdef HEXTER_DEBUG_CONTROL
void
on_test_changed(GtkObject *adj, gpointer data)
{
    int value = (int)GTK_ADJUSTMENT(adj)->value;
    unsigned char midi[4];

#if 1
    midi[0] = 0;
    midi[1] = 0xB0; /* control change */
    midi[2] = 0x63; /* NRPN MSB */
    midi[3] = 0;
    lo_send(osc_host_address, osc_midi_path, "m", midi);
    midi[2] = 0x62; /* NRPN LSB */
    midi[3] = 40;
    lo_send(osc_host_address, osc_midi_path, "m", midi);
    midi[2] = 0x06; /* Data Entry MSB */
    midi[3] = (value / 128) & 127;
    lo_send(osc_host_address, osc_midi_path, "m", midi);
    midi[2] = 0x26; /* Data Entry LSB */
    midi[3] = value & 127;
    lo_send(osc_host_address, osc_midi_path, "m", midi);
#else
    midi[0] = 0;
    midi[1] = 0xB0; /* control change */
    midi[2] = 0x50; /* GP5 */
    midi[3] = (value / 128) & 127;
    lo_send(osc_host_address, osc_midi_path, "m", midi);
#endif
}
#endif /* HEXTER_DEBUG_CONTROL */

#ifdef MIDI_ALSA
void
on_sysex_enable_toggled( GtkWidget *widget, gpointer data )
{
    int state = GTK_TOGGLE_BUTTON (widget)->active;

    if (internal_gui_update_only) {
        /* GUIDB_MESSAGE(DB_GUI, " on_sysex_enable_toggled: skipping further action\n"); */
        return;
    }

    GUIDB_MESSAGE(DB_GUI, " on_sysex_enable_toggled: button now %s\n",
                  (state ? "on" : "off"));

    if (state) {  /* enable */

        char *message;
        char buf[32];

        if (sysex_enabled)
            return;

        if (sysex_start(on_sysex_receipt, &message)) {

            gtk_widget_set_sensitive (sysex_channel_label, TRUE);
            gtk_widget_set_sensitive (sysex_channel_spin, TRUE);
            snprintf(buf, 32, " Listening on port %d:%d", sysex_seq_client_id, sysex_seq_port_id);
            gtk_label_set_text (GTK_LABEL (sysex_status_label), buf);

        } else {

            display_notice("Error: could not start MIDI client for sys-ex reception.", message);

            internal_gui_update_only = 1;
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), 0); /* causes call to on_sysex_enable_toggled callback */
            internal_gui_update_only = 0;

        }

    } else {  /* disable */

        if (!sysex_enabled)
            return;

        sysex_stop();

        gtk_widget_set_sensitive (sysex_channel_label, FALSE);
        gtk_widget_set_sensitive (sysex_channel_spin, FALSE);
        gtk_label_set_text (GTK_LABEL (sysex_status_label), "");
    }
}

void
on_sysex_channel_change(GtkWidget *widget, gpointer data)
{
    sysex_receive_channel = lrintf(GTK_ADJUSTMENT(widget)->value) - 1;

    GUIDB_MESSAGE(DB_GUI, " on_sysex_channel_change: channel now %d\n",
                  sysex_receive_channel + 1);
}
#endif /* MIDI_ALSA */

void
on_edit_save_position_ok(GtkWidget *widget, gpointer data)
{
    int position = lrintf(GTK_ADJUSTMENT(edit_save_position_spin_adj)->value);

    gtk_widget_hide(edit_save_position_window);

    GUIDB_MESSAGE(DB_GUI, " on_edit_save_position_ok: position %d\n", position);

    /* Hmm, while the edit save position dialog is shown, there's nothing
     * to prevent program changes or patch configures from causing the edit
     * buffer to be overwritten.... */

    edit_buffer.program = position;
    dx7_patch_pack(edit_buffer.voice, patches, position);
    gui_data_mark_dirty_patch_sections(position, position);
    gui_data_send_dirty_patch_sections();

    /* We make the saved-to program the new current one, and clear the
     * edit_buffer overlay if active, because this makes the system's
     * state more intuitively understandable. */
    if (position != current_program) {
        current_program = position;
        lo_send(osc_host_address, osc_program_path, "ii", 0, current_program);
    }
    if (edit_buffer_active) {
        edit_buffer_active = FALSE;
        gui_data_send_edit_buffer_off();
    }

    rebuild_patches_clist();
    patch_edit_update_status();
}

void
on_edit_save_position_cancel(GtkWidget *widget, gpointer data)
{
    GUIDB_MESSAGE(DB_GUI, ": on_edit_save_position_cancel called\n");
    gtk_widget_hide(edit_save_position_window);
}

void
send_performance(void)
{
    uint8_t perf_buffer[DX7_PERFORMANCE_SIZE];
    uint8_t p;

    hexter_data_performance_init(perf_buffer);

    perf_buffer[0]  = (GTK_TOGGLE_BUTTON (compat059_button)->active) ? 1 : 0;  /* 0.5.9 compatibility */

    perf_buffer[3]  = lrintf(GTK_ADJUSTMENT(performance_spin_adjustments[PP_PITCH_BEND_RANGE])->value);
    perf_buffer[5]  = lrintf(GTK_ADJUSTMENT(performance_spin_adjustments[PP_PORTAMENTO_TIME])->value);
    perf_buffer[9]  = lrintf(GTK_ADJUSTMENT(performance_spin_adjustments[PP_MOD_WHEEL_SENSITIVITY])->value);
    perf_buffer[11] = lrintf(GTK_ADJUSTMENT(performance_spin_adjustments[PP_FOOT_SENSITIVITY])->value);
    perf_buffer[13] = lrintf(GTK_ADJUSTMENT(performance_spin_adjustments[PP_PRESSURE_SENSITIVITY])->value);
    perf_buffer[15] = lrintf(GTK_ADJUSTMENT(performance_spin_adjustments[PP_BREATH_SENSITIVITY])->value);

    p = (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_MOD_WHEEL_ASSIGN][0])->active ? 1 : 0) +
        (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_MOD_WHEEL_ASSIGN][1])->active ? 2 : 0) +
        (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_MOD_WHEEL_ASSIGN][2])->active ? 4 : 0);
    perf_buffer[10] = p;
    p = (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_FOOT_ASSIGN][0])->active ? 1 : 0) +
        (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_FOOT_ASSIGN][1])->active ? 2 : 0) +
        (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_FOOT_ASSIGN][2])->active ? 4 : 0);
    perf_buffer[12] = p;
    p = (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_PRESSURE_ASSIGN][0])->active ? 1 : 0) +
        (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_PRESSURE_ASSIGN][1])->active ? 2 : 0) +
        (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_PRESSURE_ASSIGN][2])->active ? 4 : 0);
    perf_buffer[14] = p;
    p = (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_BREATH_ASSIGN][0])->active ? 1 : 0) +
        (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_BREATH_ASSIGN][1])->active ? 2 : 0) +
        (GTK_TOGGLE_BUTTON(performance_assign_widgets[PP_BREATH_ASSIGN][2])->active ? 4 : 0);
    perf_buffer[16] = p;

    gui_data_send_performance_buffer(perf_buffer);
}

void
on_performance_spin_change(GtkWidget *widget, gpointer data)
{
    if (internal_gui_update_only) {
        GUIDB_MESSAGE(DB_GUI, " on_performance_spin_change: skipping further action\n");
        return;
    }

    GUIDB_MESSAGE(DB_GUI, " on_performance_spin_change: '%s' set to %ld\n",
                  performance_spin_names[(int)data],
                  lrintf(GTK_ADJUSTMENT(widget)->value));

    send_performance();
}

void
on_performance_assign_toggled(GtkWidget *widget, gpointer data)
{
    if (internal_gui_update_only) {
        GUIDB_MESSAGE(DB_GUI, " on_performance_assign_toggled: skipping further action\n");
        return;
    }

    GUIDB_MESSAGE(DB_GUI, " on_performance_assign_toggled: '%s' now P%d A%d E%d\n",
                  performance_assign_names[(int)data],
                  GTK_TOGGLE_BUTTON (performance_assign_widgets[(int)data][0])->active ? 1 : 0,
                  GTK_TOGGLE_BUTTON (performance_assign_widgets[(int)data][1])->active ? 1 : 0,
                  GTK_TOGGLE_BUTTON (performance_assign_widgets[(int)data][2])->active ? 1 : 0);

    send_performance();
}

void
on_test_note_slider_change(GtkWidget *widget, gpointer data)
{
    unsigned char value = lrintf(GTK_ADJUSTMENT(widget)->value);

    if ((int)data == 0) {  /* key */

        test_note_noteon_key = value;
        GUIDB_MESSAGE(DB_GUI, " on_test_note_slider_change: new test note key %d\n", test_note_noteon_key);

    } else {  /* velocity */

        test_note_velocity = value;
        GUIDB_MESSAGE(DB_GUI, " on_test_note_slider_change: new test note velocity %d\n", test_note_velocity);

    }
}

static void
send_midi(unsigned char b0, unsigned char b1, unsigned char b2)
{
    unsigned char midi[4];

    midi[0] = 0;
    midi[1] = b0;
    midi[2] = b1;
    midi[3] = b2;
    lo_send(osc_host_address, osc_midi_path, "m", midi);
}

void
release_test_note(void)
{
    if (test_note_noteoff_key >= 0) {
        send_midi(0x80, test_note_noteoff_key, 0x40);
        test_note_noteoff_key = -1;
    }
}

void
on_test_note_button_press(GtkWidget *widget, gpointer data)
{
    int state = (int)data;

    GUIDB_MESSAGE(DB_GUI, " on_test_note_button_press: button %s\n",
                  state ? "pressed" : "released")

    if (state) {  /* button pressed */

        if (test_note_noteoff_key < 0) {
            send_midi(0x90, test_note_noteon_key, test_note_velocity);
            test_note_noteoff_key = test_note_noteon_key;
        }
    } else { /* button released */

        release_test_note();

    }
}

void
display_notice(char *message1, char *message2)
{
    gtk_label_set_text (GTK_LABEL (notice_label_1), message1);
    gtk_label_set_text (GTK_LABEL (notice_label_2), message2);
    gtk_widget_show(notice_window);
}

void
on_notice_dismiss( GtkWidget *widget, gpointer data )
{
    gtk_widget_hide(notice_window);
}

void
update_voice_widget(int port, float value)
{
    if (port == HEXTER_PORT_TUNING) {

        if (value < 415.3f) {
            value = 415.3f;
        } else if (value > 466.2f) {
            value = 466.2f;
        }

        /* GUIDB_MESSAGE(DB_OSC, " update_voice_widget: change of 'Tuning' to %f\n", value); */

        internal_gui_update_only = 1;

        GTK_ADJUSTMENT(tuning_adj)->value = value;
        gtk_signal_emit_by_name (GTK_OBJECT (tuning_adj), "value_changed");  /* causes call to on_voice_slider_change callback */

        internal_gui_update_only = 0;

    } else if (port == HEXTER_PORT_VOLUME) {

        if (value < -70.0f) {
            value = -70.0f;
        } else if (value > 20.0f) {
            value = 20.0f;
        }

        /* GUIDB_MESSAGE(DB_OSC, " update_voice_widget: change of 'Volume' to %f\n", value); */

        internal_gui_update_only = 1;

        GTK_ADJUSTMENT(volume_adj)->value = value;
        gtk_signal_emit_by_name (GTK_OBJECT (volume_adj), "value_changed");  /* causes call to on_voice_slider_change callback */

        internal_gui_update_only = 0;
    }
}

void
update_from_program_select(unsigned long bank, unsigned long program)
{
    if (!bank && program < 128) {

        current_program = program;

        patches_clist_set_program();

        /* If we receive a program change that doesn't match what's in the
         * edit buffer, then we clear the edit buffer. Note that the converse
         * is not done: if we receive an edit_buffer configure whose program
         * number does not match current_program, we don't change the program.
         */
        if (program != edit_buffer.program) {
            if (edit_buffer_active) {
                gui_data_send_edit_buffer_off();
                edit_buffer_active = FALSE;
            }
            edit_buffer.program = program;
            dx7_patch_unpack(patches, edit_buffer.program, edit_buffer.voice);
            /* set all the patch edit widgets to match */
            patch_edit_update_editors(); /* also updates status */
        } else
            patch_edit_update_status();

    } else {  /* out of range */

        /* gtk_clist_unselect_all (GTK_CLIST(patches_clist)); */

    }
}

void
update_patches(const char *key, const char *value)
{
    int section = key[7] - '0';

    GUIDB_MESSAGE(DB_OSC, ": update_patches: received new '%s'\n", key);

    if (section < 0 || section > 3)
        return;

    if (!decode_7in6(value, 32 * sizeof(dx7_patch_t),
                     (uint8_t *)&patches[section * 32])) {
        GUIDB_MESSAGE(DB_OSC, " update_patches: corrupt data!\n");
        return;
    }

    patch_section_dirty[section] = 0;

    rebuild_patches_clist();

    if (!edit_buffer_active && (edit_buffer.program / 32) == section) {
        dx7_patch_unpack(patches, edit_buffer.program, edit_buffer.voice);
        /* set all the patch edit widgets to match */
        patch_edit_update_editors();
    }
}

void
update_edit_buffer(const char *value)
{
    GUIDB_MESSAGE(DB_OSC, ": update_edit_buffer called\n");

    if (!strcmp(value, "off")) {

        edit_buffer.program = current_program;
        dx7_patch_unpack(patches, current_program, edit_buffer.voice);
        edit_buffer_active = FALSE;
        patch_edit_update_editors();

    } else {

        if (!decode_7in6(value, sizeof(edit_buffer), (uint8_t *)&edit_buffer)) {
            GUIDB_MESSAGE(DB_OSC, " update_edit_buffer: corrupt data!\n");
            return;
        }
        edit_buffer_active = TRUE;
        patch_edit_update_editors();
    }
}

void
update_performance_spin(int parameter, uint8_t value, uint8_t max)
{
    GtkObject *adjustment = performance_spin_adjustments[parameter];

    GUIDB_MESSAGE(DB_OSC, ": update_performance_spin called for '%s' with %d\n",
                  performance_spin_names[parameter], value);

    if (value <= max) {

        internal_gui_update_only = 1;

        GTK_ADJUSTMENT(adjustment)->value = (float)value;
        gtk_signal_emit_by_name (adjustment, "value_changed");  /* causes call to on_performance_spin_change callback */

        internal_gui_update_only = 0;
    }
}

void
update_performance_assign(int parameter, uint8_t bits)
{
    GUIDB_MESSAGE(DB_OSC, ": update_performance_assign called for '%s' with %d\n",
                  performance_assign_names[parameter], bits);

    if (bits <= 7) {

        internal_gui_update_only = 1;

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(performance_assign_widgets[parameter][0]),
                                     bits & 1 ? 1 : 0); /* causes call to on_performance_assign_toggled callback */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(performance_assign_widgets[parameter][1]),
                                     bits & 2 ? 1 : 0); /* causes call to on_performance_assign_toggled callback */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(performance_assign_widgets[parameter][2]),
                                     bits & 4 ? 1 : 0); /* causes call to on_performance_assign_toggled callback */

        internal_gui_update_only = 0;
    }
}

void
update_performance_widgets(uint8_t *performance)
{
    update_performance_spin(PP_PITCH_BEND_RANGE,      performance[3],  12);
    update_performance_spin(PP_PORTAMENTO_TIME,       performance[5],  99);
    update_performance_spin(PP_MOD_WHEEL_SENSITIVITY, performance[9],  15);
    update_performance_spin(PP_FOOT_SENSITIVITY,      performance[11], 15);
    update_performance_spin(PP_PRESSURE_SENSITIVITY,  performance[13], 15);
    update_performance_spin(PP_BREATH_SENSITIVITY,    performance[15], 15);
    update_performance_assign(PP_MOD_WHEEL_ASSIGN, performance[10]);
    update_performance_assign(PP_FOOT_ASSIGN,      performance[12]);
    update_performance_assign(PP_PRESSURE_ASSIGN,  performance[14]);
    update_performance_assign(PP_BREATH_ASSIGN,    performance[16]);

    /* 0.5.9 compatibility */
    internal_gui_update_only = 1;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compat059_button),
                                 performance[0] & 0x01);  /* causes call to on_compat059_toggled callback */
    internal_gui_update_only = 0;
}

void
update_performance(const char *value)
{
    uint8_t perf_buffer[DX7_PERFORMANCE_SIZE];

    if (!decode_7in6(value, DX7_PERFORMANCE_SIZE, perf_buffer)) {
        GUIDB_MESSAGE(DB_OSC, " update_performance: corrupt data!\n");
        return;
    }

    update_performance_widgets(perf_buffer);
}

void
update_monophonic(const char *value)
{
    int index;

    GUIDB_MESSAGE(DB_OSC, ": update_monophonic called with '%s'\n", value);

    if (!strcmp(value, "off")) {
        index = 0;
    } else if (!strcmp(value, "on")) {
        index = 1;
    } else if (!strcmp(value, "once")) {
        index = 2;
    } else if (!strcmp(value, "both")) {
        index = 3;
    } else {
        return;
    }

    gtk_option_menu_set_history(GTK_OPTION_MENU (monophonic_option_menu),
                                index);  /* updates optionmenu current selection,
                                          * without needing to send it a signal */
}

void
update_polyphony(const char *value)
{
    int poly = atoi(value);

    GUIDB_MESSAGE(DB_OSC, ": update_polyphony called with '%s'\n", value);

    if (poly > 0 && poly < HEXTER_MAX_POLYPHONY) {

        internal_gui_update_only = 1;

        GTK_ADJUSTMENT(polyphony_adj)->value = (float)poly;
        gtk_signal_emit_by_name (GTK_OBJECT (polyphony_adj), "value_changed");  /* causes call to on_voice_slider_change callback */

        internal_gui_update_only = 0;
    }
}

void
patches_clist_set_program(void)
{
    /* set active row */
    internal_gui_update_only = 1;
    gtk_clist_select_row (GTK_CLIST(patches_clist), current_program, 0);
    internal_gui_update_only = 0;
    /* scroll window to show active row */
    gtk_clist_moveto(GTK_CLIST(patches_clist), current_program, 0, 0, 0);
}

void
rebuild_patches_clist(void)
{
    char number[4], name[21];
    char *data[2] = { number, name };
    int i;

    GUIDB_MESSAGE(DB_GUI, ": rebuild_patches_clist called\n");

    gtk_clist_freeze(GTK_CLIST(patches_clist));
    gtk_clist_clear(GTK_CLIST(patches_clist));
    for (i = 0; i < 128; i++) {
        snprintf(number, 4, "%d", i);
        patch_edit_copy_name_to_utf8(name, patches[i].data, TRUE);
        gtk_clist_append(GTK_CLIST(patches_clist), data);
    }

    /* kick GTK+ 2.4.x in the pants.... */
    gtk_signal_emit_by_name (GTK_OBJECT (patches_clist), "check-resize");

    gtk_clist_thaw(GTK_CLIST(patches_clist));

    patches_clist_set_program();
}
