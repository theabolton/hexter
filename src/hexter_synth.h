/* hexter DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
 *
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
 * Portions of this file may have come from alsa-lib, copyright
 * and licensed under the LGPL v2.1.
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

#ifndef _HEXTER_SYNTH_H
#define _HEXTER_SYNTH_H

#include <pthread.h>

#include <ladspa.h>
#include <dssi.h>

#include "hexter_types.h"
#include "hexter.h"

#define DSSP_MONO_MODE_OFF  0
#define DSSP_MONO_MODE_ON   1
#define DSSP_MONO_MODE_ONCE 2
#define DSSP_MONO_MODE_BOTH 3

/*
 * hexter_instance_t
 */
struct _hexter_instance_t
{
    /* output */
    LADSPA_Data    *output;
    /* input */
    LADSPA_Data    *tuning;

    float           sample_rate;  
    float           nugget_rate;       /* nuggets per second */

    /* voice tracking */
    int             polyphony;         /* requested polyphony, must be <= HEXTER_MAX_POLYPHONY */
    int             monophonic;        /* true if operating in monophonic mode */
    int             max_voices;        /* current max polyphony, either requested polyphony above or 1 while in monophonic mode */
    int             current_voices;    /* count of currently playing voices */
    dx7_voice_t    *mono_voice;
    signed char     held_keys[8];      /* for monophonic key tracking, an array of note-ons, most recently received first */

    /* patches and edit buffer */
    pthread_mutex_t patches_mutex;
    int             pending_program_change;

    dx7_patch_t    *patches;

    int             current_program;
    uint8_t         current_patch_buffer[DX7_VOICE_SIZE_UNPACKED];  /* current unpacked patch in use */

    int             overlay_program;   /* program to which 'configure edit_buffer' patch applies, or -1 */
    uint8_t         overlay_patch_buffer[DX7_VOICE_SIZE_UNPACKED];  /* 'configure edit_buffer' patch */

    /* tuning */
    float           last_tuning;
    double          fixed_freq_multiplier;

    /* current non-LADSPA-port-mapped controller values */
    unsigned char   key_pressure[128];
    unsigned char   cc[128];                  /* controller values */
    unsigned char   channel_pressure;
    unsigned char   pitch_wheel_sensitivity;  /* in semitones */
    int             pitch_wheel;              /* range is -8192 - 8191 */

    /* translated controller values */
    /* float        mod_wheel; */
    double          pitch_bend;               /* frequency shift, in semitones */

    int32_t         dx7_eg_max_slew;   /* max op eg increment, in s15.16 units per frame */
};

/*
 * hexter_synth_t
 */
struct _hexter_synth_t {
    int             initialized;
    int             instance_count;

    pthread_mutex_t mutex;
    int             mutex_grab_failed;

    unsigned long   nugget_remains;

    unsigned int    note_id;           /* incremented for every new note, used for voice-stealing prioritization */
    int             global_polyphony;  /* must be <= HEXTER_MAX_POLYPHONY */

    dx7_voice_t    *voice[HEXTER_MAX_POLYPHONY];
};

/* hexter_synth.c */
void  dx7_voice_off(dx7_voice_t* voice);
void  dx7_voice_start_voice(dx7_voice_t *voice);
void  hexter_synth_all_voices_off(void);
void  hexter_instance_all_voices_off(hexter_instance_t *instance);
void  hexter_instance_note_off(hexter_instance_t *instance, unsigned char key,
                               unsigned char rvelocity);
void  hexter_instance_all_notes_off(hexter_instance_t *instance);
void  hexter_instance_note_on(hexter_instance_t *instance, unsigned char key,
                              unsigned char velocity);
void  hexter_instance_key_pressure(hexter_instance_t *instance,
                                   unsigned char key, unsigned char pressure);
void  hexter_instance_damp_voices(hexter_instance_t *instance);
void  hexter_instance_update_wheel_mod(hexter_instance_t *instance);
void  hexter_instance_control_change(hexter_instance_t *instance,
                                     unsigned int param, signed int value);
void  hexter_instance_channel_pressure(hexter_instance_t *instance,
                                       signed int pressure);
void  hexter_instance_pitch_bend(hexter_instance_t *instance, signed int value);
void  hexter_instance_init_controls(hexter_instance_t *instance);
void  hexter_instance_select_program(hexter_instance_t *instance,
                                     unsigned long bank, unsigned long program);
int   hexter_instance_set_program_descriptor(hexter_instance_t *instance,
                                             DSSI_Program_Descriptor *pd,
                                             unsigned long bank,
                                             unsigned long program);
char *hexter_instance_handle_patches(hexter_instance_t *instance,
                                     const char *key, const char *value);
char *hexter_instance_handle_edit_buffer(hexter_instance_t *instance,
                                         const char *value);
char *hexter_instance_handle_monophonic(hexter_instance_t *instance,
                                        const char *value);
char *hexter_instance_handle_polyphony(hexter_instance_t *instance,
                                       const char *value);
char *hexter_synth_handle_global_polyphony(const char *value);
void  hexter_synth_render_voices(unsigned long samples_done,
                                 unsigned long sample_count, 
                                 int do_control_update);

/* these come right out of alsa/asoundef.h */
#define MIDI_CTL_MSB_MODWHEEL           0x01    /**< Modulation */
#define MIDI_CTL_SUSTAIN                0x40    /**< Sustain pedal */
#define MIDI_CTL_ALL_SOUNDS_OFF         0x78    /**< All sounds off */
#define MIDI_CTL_RESET_CONTROLLERS      0x79    /**< Reset Controllers */
#define MIDI_CTL_ALL_NOTES_OFF          0x7b    /**< All notes off */

#define HEXTER_INSTANCE_SUSTAINED(_s)  ((_s)->cc[MIDI_CTL_SUSTAIN] >= 64)

#endif /* _HEXTER_SYNTH_H */

