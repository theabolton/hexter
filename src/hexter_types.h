/* hexter DSSI software synthesizer plugin
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

#ifndef _HEXTER_TYPES_H
#define _HEXTER_TYPES_H

#include <stdint.h>

#define DX7_VOICE_SIZE_PACKED     128
#define DX7_VOICE_SIZE_UNPACKED   155
#define DX7_PERFORMANCE_SIZE       64
#define DX7_DUMP_SIZE_SINGLE      155+8
#define DX7_DUMP_SIZE_BULK        4096+8

typedef struct _hexter_synth_t    hexter_synth_t;
typedef struct _hexter_instance_t hexter_instance_t;

typedef struct _dx7_patch_t       dx7_patch_t;
typedef struct _dx7_voice_t       dx7_voice_t;
typedef struct _dx7_op_eg_t       dx7_op_eg_t;
typedef struct _dx7_pitch_eg_t    dx7_pitch_eg_t;
typedef struct _dx7_portamento_t  dx7_portamento_t;
typedef struct _dx7_op_t          dx7_op_t;

/* in hexter.c: */
int   dssp_voicelist_mutex_lock(void);
int   dssp_voicelist_mutex_unlock(void);

#endif /* _HEXTER_TYPES_H */
