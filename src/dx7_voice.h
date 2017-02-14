/* hexter DSSI software synthesizer plugin
 *
 * Copyright (C) 2004, 2009, 2011 Sean Bolton and others.
 *
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
 * Portions of this file may have come from Juan Linietsky's
 * rx-saturno, copyright (C) 2002 by Juan Linietsky.
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

#ifndef _DX7_VOICE_H
#define _DX7_VOICE_H

#include <ladspa.h>

#include "hexter_types.h"

struct _dx7_patch_t
{
    uint8_t data[128];  /* dx7_patch_t is packed patch data */
};

#ifndef HEXTER_USE_FLOATING_POINT

#define FP_SHIFT         24
#define FP_SIZE          (1<<FP_SHIFT)
#define FP_MASK          (FP_SIZE-1)
#define FP_TO_SINE_SHIFT (FP_SHIFT-SINE_SHIFT)
#define FP_TO_SINE_SIZE  (1<<FP_TO_SINE_SHIFT)
#define FP_TO_SINE_MASK  (FP_TO_SINE_SIZE-1)

#define FP_TO_INT(x)    ((x) >> FP_SHIFT)
#define FP_TO_FLOAT(x)  ((float)(x) * (1.0f / (float)FP_SIZE))
#define FP_TO_DOUBLE(x) ((double)(x) * (1.0 / (double)FP_SIZE))
#define INT_TO_FP(x)    ((x) << FP_SHIFT)
/* beware of using the next two with constants, they probably won't be optimized */
#define FLOAT_TO_FP(x)  lrintf((x) * (float)FP_SIZE)
#define DOUBLE_TO_FP(x) lrint((x) * (double)FP_SIZE)

#define FP_MULTIPLY(a, b)     ((int32_t)(((int64_t)(a) * (int64_t)(b)) >> FP_SHIFT))
#define FP_DIVIDE_CEIL(n, d)  (((n) + (d) - 1) / (d))
#define FP_ABS(x)             (abs(x))
#define FP_RAND()             (rand() & FP_MASK)

#else /* HEXTER_USE_FLOATING_POINT */

#define FP_TO_INT(x)        (lrintf(x))
#define FP_TO_FLOAT(x)      (x)
#define FP_TO_DOUBLE(x)     ((double)(x))
#define INT_TO_FP(x)        ((float)(x))
#define FLOAT_TO_FP(x)      (x)
#define DOUBLE_TO_FP(x)     ((float)(x))

#define FP_MULTIPLY(x, y)     ((x) * (y))
#define FP_DIVIDE_CEIL(n, d)  (lrintf((n) / (d) + 0.5f));
#define FP_ABS(x)             (fabsf(x))
#define FP_RAND()             ((float)rand() / (float)RAND_MAX)

#endif /* ! HEXTER_USE_FLOATING_POINT */

#define SINE_SHIFT       12
#define SINE_SIZE        (1<<SINE_SHIFT)
#define SINE_MASK        (SINE_SIZE-1)

enum dx7_eg_mode {
    DX7_EG_FINISHED,
    DX7_EG_RUNNING,
    DX7_EG_SUSTAINING,
    DX7_EG_CONSTANT
};

struct _dx7_op_eg_t   /* operator (amplitude) envelope generator */
{
    uint8_t    base_rate[4];
    uint8_t    base_level[4];
    uint8_t    rate[4];
    uint8_t    level[4];

    int          mode;        /* enum dx7_eg_mode (finished, running, sustaining, constant) */
    int          phase;       /* 0, 1, 2, or 3 */
    dx7_sample_t value;
    int32_t      duration;    /* op envelope durations are in frames */
    dx7_sample_t increment;
    dx7_sample_t target;
    int          in_precomp;
    int32_t      postcomp_duration;
    dx7_sample_t postcomp_increment;
};

struct _dx7_pitch_eg_t   /* pitch envelope generator */
{
    uint8_t    rate[4];
    uint8_t    level[4];

    int        mode;        /* enum dx7_eg_mode (finished, running, sustaining, constant) */
    int        phase;       /* 0, 1, 2, or 3 */
    double     value;       /* in semitones, zero when level is 50 */
    int32_t    duration;    /* pitch envelope durations are in bursts ('nuggets') */
    double     increment;
    double     target;
};

struct _dx7_portamento_t  /* portamento generator */
{
    int        segment;    /* ... 3, 2, 1, or 0 */
    double     value;      /* in semitones, zero is destination pitch */
    int32_t    duration;   /* portamento segments are in bursts */
    double     increment;
    double     target;
};

enum dx7_ops {
    OP_1 = 0,
    OP_2,
    OP_3,
    OP_4,
    OP_5,
    OP_6,
    MAX_DX7_OPERATORS
};


struct _dx7_op_t   /* operator */
{
    double        frequency;
    dx7_sample_t  phase;
    dx7_sample_t  phase_increment;

    dx7_op_eg_t eg;

    uint8_t     level_scaling_bkpoint;
    uint8_t     level_scaling_l_depth;
    uint8_t     level_scaling_r_depth;
    uint8_t     level_scaling_l_curve;
    uint8_t     level_scaling_r_curve;
    uint8_t     rate_scaling;
    uint8_t     amp_mod_sens;
    uint8_t     velocity_sens;
    uint8_t     output_level;
    uint8_t     osc_mode;
    uint8_t     coarse;
    uint8_t     fine;
    uint8_t     detune;
};

enum dx7_lfo_status
{
    DX7_LFO_DELAY,
    DX7_LFO_FADEIN,
    DX7_LFO_ON
};

enum dx7_voice_status
{
    DX7_VOICE_OFF,       /* silent: is not processed by render loop */
    DX7_VOICE_ON,        /* has not received a note off event */
    DX7_VOICE_SUSTAINED, /* has received note off, but sustain controller is on */
    DX7_VOICE_RELEASED   /* had note off, not sustained, in final decay phase of envelopes */
};

/*
 * dx7_voice_t
 */
struct _dx7_voice_t
{
    hexter_instance_t *instance;

    unsigned int     note_id;

    unsigned char    status;
    unsigned char    key;
    unsigned char    velocity;
    unsigned char    rvelocity;   /* the note-off velocity */

    /* persistent voice state */
    dx7_op_t         op[MAX_DX7_OPERATORS];

    double           last_pitch;
    dx7_pitch_eg_t   pitch_eg;
    dx7_portamento_t portamento;
    float            last_port_tuning;
    double           pitch_mod_depth_pmd;
    double           pitch_mod_depth_mods;

    uint8_t          algorithm;
    dx7_sample_t     feedback;
    dx7_sample_t     feedback_multiplier;
    uint8_t          osc_key_sync;

    uint8_t          lfo_speed;
    uint8_t          lfo_delay;
    uint8_t          lfo_pmd;
    uint8_t          lfo_amd;
    uint8_t          lfo_key_sync;
    uint8_t          lfo_wave;
    uint8_t          lfo_pms;

    int              transpose;

    /* modulation */
    int              mods_serial;
    dx7_sample_t     amp_mod_env_value;
    int32_t          amp_mod_env_duration;
    dx7_sample_t     amp_mod_env_increment;
    dx7_sample_t     amp_mod_env_target;
    dx7_sample_t     amp_mod_lfo_mods_value;
    int32_t          amp_mod_lfo_mods_duration;
    dx7_sample_t     amp_mod_lfo_mods_increment;
    dx7_sample_t     amp_mod_lfo_mods_target;
    dx7_sample_t     amp_mod_lfo_amd_value;
    int32_t          amp_mod_lfo_amd_duration;
    dx7_sample_t     amp_mod_lfo_amd_increment;
    dx7_sample_t     amp_mod_lfo_amd_target;
    int              lfo_delay_segment;
    dx7_sample_t     lfo_delay_value;
    int32_t          lfo_delay_duration;
    dx7_sample_t     lfo_delay_increment;

    /* volume */
    float            last_port_volume;
    unsigned long    last_cc_volume;
    float            volume_value;
    int32_t          volume_duration;
    float            volume_increment;
    float            volume_target;
};

#define _PLAYING(voice)    ((voice)->status != DX7_VOICE_OFF)
#define _ON(voice)         ((voice)->status == DX7_VOICE_ON)
#define _SUSTAINED(voice)  ((voice)->status == DX7_VOICE_SUSTAINED)
#define _RELEASED(voice)   ((voice)->status == DX7_VOICE_RELEASED)
#define _AVAILABLE(voice)  ((voice)->status == DX7_VOICE_OFF)

extern dx7_sample_t  dx7_voice_sin_table[SINE_SIZE + 1];

extern uint8_t       dx7_voice_carriers[32];
extern float         dx7_voice_carrier_count[32];

extern dx7_sample_t *dx7_voice_eg_ol_to_mod_index;
extern float         dx7_voice_velocity_ol_adjustment[128];
extern float         dx7_voice_lfo_frequency[128];
extern float         dx7_voice_pms_to_semitones[8];
extern float         dx7_voice_amd_to_ol_adjustment[100];
extern float         dx7_voice_mss_to_ol_adjustment[16];

/* dx7_voice.c */
dx7_voice_t *dx7_voice_new(void);
void    dx7_voice_note_on(hexter_instance_t *instance, dx7_voice_t *voice,
                          unsigned char key, unsigned char velocity);
void    dx7_voice_note_off(hexter_instance_t *instance, dx7_voice_t *voice,
                           unsigned char key, unsigned char rvelocity);
void    dx7_voice_release_note(hexter_instance_t *instance, dx7_voice_t *voice);
void    dx7_op_eg_set_increment(hexter_instance_t *instance, dx7_op_eg_t *eg,
                                int new_rate, int new_level);
void    dx7_op_eg_set_next_phase(hexter_instance_t *instance, dx7_op_eg_t *eg);
void    dx7_op_eg_set_phase(hexter_instance_t *instance, dx7_op_eg_t *eg,
                            int phase);
void    dx7_op_envelope_prepare(hexter_instance_t *instance, dx7_op_t *op,
                                int transposed_note, int velocity);
void    dx7_eg_init_constants(hexter_instance_t *instance);
void    dx7_pitch_eg_set_increment(hexter_instance_t *instance,
                                   dx7_pitch_eg_t *eg, int new_rate,
                                   int new_level);
void    dx7_pitch_eg_set_next_phase(hexter_instance_t *instance,
                                    dx7_pitch_eg_t *eg);
void    dx7_pitch_eg_set_phase(hexter_instance_t *instance, dx7_pitch_eg_t *eg,
                               int phase);
void    dx7_pitch_envelope_prepare(hexter_instance_t *instance,
                                   dx7_voice_t *voice);
void    dx7_portamento_set_segment(hexter_instance_t *instance,
                                   dx7_portamento_t *port);
void    dx7_portamento_prepare(hexter_instance_t *instance,
                               dx7_voice_t *voice);
void    dx7_op_recalculate_increment(hexter_instance_t *instance, dx7_op_t *op);
double  dx7_voice_recalculate_frequency(hexter_instance_t *instance,
                                        dx7_voice_t *voice);
void    dx7_voice_recalculate_freq_and_inc(hexter_instance_t *instance,
                                           dx7_voice_t *voice);
void    dx7_voice_recalculate_volume(hexter_instance_t *instance,
                                     dx7_voice_t *voice);
void    dx7_lfo_reset(hexter_instance_t *instance);
void    dx7_lfo_set(hexter_instance_t *instance, dx7_voice_t *voice);
void    dx7_lfo_update(hexter_instance_t *instance,
                       unsigned long sample_count);
void    dx7_voice_update_mod_depths(hexter_instance_t *instance,
                                    dx7_voice_t* voice);
void    dx7_voice_calculate_runtime_parameters(hexter_instance_t *instance,
                                               dx7_voice_t *voice);
void    dx7_voice_setup_note(hexter_instance_t *instance, dx7_voice_t *voice);
void    dx7_voice_set_data(hexter_instance_t *instance, dx7_voice_t *voice);

/* dx7_voice_render.c */
void    dx7_voice_render(hexter_instance_t *instance, dx7_voice_t *voice,
                         LADSPA_Data *out, unsigned long sample_count,
                         int do_control_update);

/* dx7_voice_tables.c */
void    dx7_voice_init_tables(void);

#endif /* _DX7_VOICE_H */
