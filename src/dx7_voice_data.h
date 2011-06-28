/* hexter DSSI software synthesizer plugin and GUI
 *
 * Copyright (C) 2004, 2009, 2011 Sean Bolton and others.
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

#ifndef _DX7_VOICE_DATA_H
#define _DX7_VOICE_DATA_H

#include "hexter_types.h"

/* dx7_voice_data.c */
extern dx7_patch_t dx7_voice_init_voice;
extern uint8_t     dx7_init_performance[DX7_PERFORMANCE_SIZE];
extern float       dx7_voice_eg_rate_rise_duration[128];
extern float       dx7_voice_eg_rate_decay_duration[128];
extern float       dx7_voice_eg_rate_rise_percent[128];
extern float       dx7_voice_eg_rate_decay_percent[128];
extern double      dx7_voice_pitch_level_to_shift[128];
extern char        base64[];

char *dssp_error_message(const char *fmt, ...);
int  decode_7in6(const char *string, int expected_length, uint8_t *data);
void dx7_voice_copy_name(char *name, dx7_patch_t *patch);
void dx7_patch_unpack(dx7_patch_t *packed_patch, uint8_t number,
                      uint8_t *unpacked_patch);
void dx7_patch_pack(uint8_t *unpacked_patch, dx7_patch_t *packed_patch, 
                    uint8_t number);
void hexter_data_patches_init(dx7_patch_t *patches);
void hexter_data_performance_init(uint8_t *performance);

#endif /* _DX7_VOICE_DATA_H */

