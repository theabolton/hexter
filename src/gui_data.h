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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _GUI_DATA_H
#define _GUI_DATA_H

#include "hexter_types.h"

char *encode_7in6(uint8_t *data, int length);
void  gui_data_patches_init(void);
void  gui_data_patches_free(void);
void  gui_data_mark_dirty_patch_sections(int start_patch, int end_patch);
void  gui_data_send_dirty_patch_sections(void);
int   gui_data_save(char *filename, int type, int start, int end,
                    char **message);
int   gui_data_load(const char *filename, int position, char **message);
void  gui_data_set_up_edit_buffer(int copy_current_program);
int   gui_data_sysex_parse(unsigned int length, unsigned char *data);
void  gui_data_send_edit_buffer(void);
void  gui_data_clear_edit_buffer(void);
void  dx7_patch_pack(uint8_t *unpacked_patch, dx7_patch_t *packed_patch, 
                     uint8_t number);

#endif /* _GUI_DATA_H */

