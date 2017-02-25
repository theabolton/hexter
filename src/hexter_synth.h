/* hexter DSSI software synthesizer plugin
 *
 * Copyright (C) 2004, 2009, 2011, 2012 Sean Bolton and others.
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
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
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
    hexter_instance_t *next;

    /* output */
    LADSPA_Data    *output;
    /* input */
    LADSPA_Data    *tuning;
    LADSPA_Data    *volume;

    float           sample_rate;
    float           nugget_rate;       /* nuggets per second */
    unsigned long   nugget_remains;
    int32_t         ramp_duration;     /* frames per ramp for mods and volume */
    dx7_sample_t    dx7_eg_max_slew;   /* max op eg increment, in units per frame */

    /* voice tracking */
    unsigned int    note_id;           /* incremented for every new note, used for voice-stealing prioritization */
    int             polyphony;         /* requested polyphony, must be <= HEXTER_MAX_POLYPHONY */
    int             monophonic;        /* true if operating in monophonic mode */
    int             max_voices;        /* current max polyphony, either requested polyphony above or 1 while in monophonic mode */
    int             current_voices;    /* count of currently playing voices */
    dx7_voice_t    *mono_voice;
    unsigned char   last_key;          /* portamento starting key */
    signed char     held_keys[8];      /* for monophonic key tracking, an array of note-ons, most recently received first */

    pthread_mutex_t voicelist_mutex;
    int             voicelist_mutex_grab_failed;

    dx7_voice_t    *voice[HEXTER_MAX_POLYPHONY];

    /* patches and edit buffer */
    pthread_mutex_t patches_mutex;
    int             pending_program_change;

    dx7_patch_t    *patches;

    int             current_program;
    uint8_t         current_patch_buffer[DX7_VOICE_SIZE_UNPACKED];  /* current unpacked patch in use */

    int             overlay_program;   /* program to which 'configure edit_buffer' patch applies, or -1 */
    uint8_t         overlay_patch_buffer[DX7_VOICE_SIZE_UNPACKED];  /* 'configure edit_buffer' patch */

    /* global performance parameter buffer */
    uint8_t         performance_buffer[DX7_PERFORMANCE_SIZE];

    /* current performance perameters (from global buffer or current patch) */
    uint8_t         pitch_bend_range;         /* in semitones */
    uint8_t         portamento_time;
    uint8_t         mod_wheel_sensitivity;
    uint8_t         mod_wheel_assign;
    uint8_t         foot_sensitivity;
    uint8_t         foot_assign;
    uint8_t         pressure_sensitivity;
    uint8_t         pressure_assign;
    uint8_t         breath_sensitivity;
    uint8_t         breath_assign;

    /* current non-LADSPA-port-mapped controller values */
    unsigned char   key_pressure[128];
    unsigned char   cc[128];                  /* controller values */
    unsigned char   channel_pressure;
    int             pitch_wheel;              /* range is -8192 - 8191 */

    /* translated port and controller values */
    double          fixed_freq_multiplier;
    unsigned long   cc_volume;                /* volume msb*128 + lsb, max 16256 */
    double          pitch_bend;               /* frequency shift, in semitones */
    int             mods_serial;
    float           mod_wheel;
    float           foot;
    float           breath;

    uint8_t         lfo_speed;
    uint8_t         lfo_wave;
    uint8_t         lfo_delay;
    dx7_sample_t    lfo_delay_value[3];
    int32_t         lfo_delay_duration[3];
    dx7_sample_t    lfo_delay_increment[3];
    int32_t         lfo_phase;
    dx7_sample_t    lfo_value;
    double          lfo_value_for_pitch;      /* no delay, unramped */
    int32_t         lfo_duration;
    dx7_sample_t    lfo_increment;
    dx7_sample_t    lfo_target;
    dx7_sample_t    lfo_increment0;
    dx7_sample_t    lfo_increment1;
    int32_t         lfo_duration0;
    int32_t         lfo_duration1;
    dx7_sample_t    lfo_buffer[HEXTER_NUGGET_SIZE];
#ifdef HEXTER_DEBUG_CONTROL
    dx7_sample_t    feedback_mod;
#endif
};

/* hexter_synth.c */
void  dx7_voice_off(dx7_voice_t* voice);
void  dx7_voice_start_voice(dx7_voice_t *voice);
void  hexter_instance_all_voices_off(hexter_instance_t *instance);
void  hexter_instance_note_off(hexter_instance_t *instance, unsigned char key,
                               unsigned char rvelocity);
void  hexter_instance_all_notes_off(hexter_instance_t *instance);
void  hexter_instance_note_on(hexter_instance_t *instance, unsigned char key,
                              unsigned char velocity);
void  hexter_instance_key_pressure(hexter_instance_t *instance,
                                   unsigned char key, unsigned char pressure);
void  hexter_instance_damp_voices(hexter_instance_t *instance);
void  hexter_instance_control_change(hexter_instance_t *instance,
                                     unsigned int param, signed int value);
void  hexter_instance_channel_pressure(hexter_instance_t *instance,
                                       signed int pressure);
void  hexter_instance_pitch_bend(hexter_instance_t *instance, signed int value);
void  hexter_instance_init_controls(hexter_instance_t *instance);
void  hexter_instance_set_performance_data(hexter_instance_t *instance);
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
char *hexter_instance_handle_performance(hexter_instance_t *instance,
                                         const char *value);
void  hexter_instance_render_voices(hexter_instance_t *instance,
                                    unsigned long samples_done,
                                    unsigned long sample_count,
                                    int do_control_update);

/* these come right out of alsa/asoundef.h */
#define MIDI_CTL_MSB_MODWHEEL           0x01    /**< Modulation */
#define MIDI_CTL_MSB_BREATH             0x02    /**< Breath */
#define MIDI_CTL_MSB_FOOT               0x04    /**< Foot */
/* -FIX- support 5 portamento time */
#define MIDI_CTL_MSB_DATA_ENTRY         0x06    /**< Data entry */
#define MIDI_CTL_MSB_MAIN_VOLUME        0x07    /**< Main volume */
#define MIDI_CTL_MSB_PAN                0x0a    /**< Panpot */
#define MIDI_CTL_MSB_EXPRESSION         0x0b    /**< Expression */
#define MIDI_CTL_MSB_GENERAL_PURPOSE1   0x10    /**< General purpose 1 */
#define MIDI_CTL_MSB_GENERAL_PURPOSE2   0x11    /**< General purpose 2 */
#define MIDI_CTL_MSB_GENERAL_PURPOSE3   0x12    /**< General purpose 3 */
#define MIDI_CTL_MSB_GENERAL_PURPOSE4   0x13    /**< General purpose 4 */
#define MIDI_CTL_LSB_MODWHEEL           0x21    /**< Modulation */
#define MIDI_CTL_LSB_BREATH             0x22    /**< Breath */
#define MIDI_CTL_LSB_FOOT               0x24    /**< Foot */
#define MIDI_CTL_LSB_DATA_ENTRY         0x26    /**< Data entry */
#define MIDI_CTL_LSB_MAIN_VOLUME        0x27    /**< Main volume */
#define MIDI_CTL_SUSTAIN                0x40    /**< Sustain pedal */
/* -FIX- support 65(?) portamento switch */
#define MIDI_CTL_GENERAL_PURPOSE5       0x50    /**< General purpose 5 */
#define MIDI_CTL_GENERAL_PURPOSE6       0x51    /**< General purpose 6 */
#define MIDI_CTL_NONREG_PARM_NUM_LSB    0x62    /**< Non-registered parameter number */
#define MIDI_CTL_NONREG_PARM_NUM_MSB    0x63    /**< Non-registered parameter number */
#define MIDI_CTL_REGIST_PARM_NUM_LSB    0x64    /**< Registered parameter number */
#define MIDI_CTL_REGIST_PARM_NUM_MSB    0x65    /**< Registered parameter number */
#define MIDI_CTL_ALL_SOUNDS_OFF         0x78    /**< All sounds off */
#define MIDI_CTL_RESET_CONTROLLERS      0x79    /**< Reset Controllers */
#define MIDI_CTL_ALL_NOTES_OFF          0x7b    /**< All notes off */

#define HEXTER_INSTANCE_SUSTAINED(_s)  ((_s)->cc[MIDI_CTL_SUSTAIN] >= 64)

#endif /* _HEXTER_SYNTH_H */
