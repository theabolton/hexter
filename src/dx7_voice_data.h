/* hexter DSSI software synthesizer plugin and GUI
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

#ifndef _DX7_VOICE_DATA_H
#define _DX7_VOICE_DATA_H

#include "hexter_types.h"

/* dx7_voice_data.c */
extern dx7_patch_t dx7_voice_init_voice;
extern char        base64[];

int  decode_7in6(const char *string, int expected_length, uint8_t *data);
void dx7_voice_copy_name(char *name, dx7_patch_t *patch);
void dx7_patch_unpack(dx7_patch_t *packed_patch, uint8_t number,
                      uint8_t *unpacked_patch);
int  dx7_bulk_dump_checksum(uint8_t *data, int length);
int  dx7_patchbank_load(const char *filename, dx7_patch_t *firstpatch,
                        int maxpatches, char **errmsg);
void hexter_data_patches_init(dx7_patch_t *patches);

/* dx7_voice_patches.c */
extern int         friendly_patch_count;
extern dx7_patch_t friendly_patches[];

#endif /* _DX7_VOICE_DATA_H */

