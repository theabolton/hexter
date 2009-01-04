/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2004, 2009 Sean Bolton and others.
 *
 * Portions of this file may have come from specimen, copyright
 * (c) 2004 Pete Bessman under the GNU General Public License
 * version 2.
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

#ifdef MIDI_ALSA

#include <alsa/asoundlib.h>

#include <gtk/gtk.h>

#include "gui_main.h"
#include "gui_midi.h"

int        sysex_enabled = 0;

snd_seq_t *sysex_seq_handle;
int        sysex_seq_client_id;
int        sysex_seq_port_id;
gint       sysex_seq_gtk_fd_tag;

sysex_callback_function *sysex_handler_callback;

void
sysex_receive_callback(gpointer data, gint source,
                       GdkInputCondition condition)
{
    snd_seq_event_t *ev;

    do {
        if (snd_seq_event_input(sysex_seq_handle, &ev) <= 0)
            continue;

        if (ev->type == SND_SEQ_EVENT_SYSEX) {
            (*sysex_handler_callback)(ev->data.ext.len, ev->data.ext.ptr);
        }

        snd_seq_free_event(ev);

    } while (snd_seq_event_input_pending(sysex_seq_handle, 0) > 0);
}

int
sysex_start(sysex_callback_function *handler, char **errmsg)
{
    char buf[33];
    int npfd;
    struct pollfd pfd;

    if (snd_seq_open(&sysex_seq_handle, "default", SND_SEQ_OPEN_INPUT,
                     SND_SEQ_NONBLOCK) < 0) {
         *errmsg = "Error opening ALSA sequencer.\n";
         return 0;
    }

    snprintf(buf, 33, "hexter %s", user_friendly_id);
    snd_seq_set_client_name(sysex_seq_handle, buf);

    sysex_seq_client_id = snd_seq_client_id(sysex_seq_handle);

    if ((sysex_seq_port_id = snd_seq_create_simple_port(sysex_seq_handle,
                                 "sys-ex receive",
                                 SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                                 SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
        *errmsg = "Error creating sequencer port.\n";
        snd_seq_close(sysex_seq_handle);
        return 0;
    }

    sysex_handler_callback = handler;

    /* tell GTK+ to watch seq fd */
    npfd = snd_seq_poll_descriptors_count(sysex_seq_handle, POLLIN);
    if (npfd != 1) {
        *errmsg = "Incompatible ALSA version.\n";
        snd_seq_close(sysex_seq_handle);
        return 0;
    }
    snd_seq_poll_descriptors(sysex_seq_handle, &pfd, npfd, POLLIN);
    sysex_seq_gtk_fd_tag = gdk_input_add(pfd.fd, GDK_INPUT_READ,
                                         sysex_receive_callback, NULL);

    sysex_enabled = 1;
    return 1;
}

void
sysex_stop(void)
{
    gdk_input_remove(sysex_seq_gtk_fd_tag);
    snd_seq_delete_simple_port (sysex_seq_handle, sysex_seq_port_id);
    snd_seq_close(sysex_seq_handle);
    sysex_enabled = 0;
}

#else /* MIDI_ALSA */

#include "gui_main.h"
#include "gui_midi.h"

int sysex_enabled = 0;

int
sysex_start(sysex_callback_function *handler, char **errmsg)
{
    *errmsg = "MIDI sys-ex support not available!\n";
    sysex_enabled = 0;
    return 0;
}

void
sysex_stop(void)
{
    sysex_enabled = 0;
}

#endif /* MIDI_ALSA */
