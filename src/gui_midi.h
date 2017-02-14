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

#ifndef _GUI_MIDI_H
#define _GUI_MIDI_H

extern int sysex_enabled;
extern int sysex_seq_client_id;
extern int sysex_seq_port_id;

typedef void (sysex_callback_function)(unsigned int length, unsigned char* data);

int  sysex_start(sysex_callback_function *sysex_handler, char **error_message);
void sysex_stop(void);

#endif /* _GUI_MIDI_H */
