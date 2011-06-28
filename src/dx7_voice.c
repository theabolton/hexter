/* hexter DSSI software synthesizer plugin
 *
 * Copyright (C) 2004, 2009, 2011 Sean Bolton and others.
 *
 * Portions of this file may have come from Juan Linietsky's
 * rx-saturno, copyright (C) 2002 by Juan Linietsky.
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
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
#include <config.h>
#endif

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "hexter_types.h"
#include "hexter_synth.h"
#include "dx7_voice.h"
#include "dx7_voice_data.h"

/*
 * dx7_voice_new
 */
dx7_voice_t *
dx7_voice_new(void)
{
    dx7_voice_t *voice;

    voice = (dx7_voice_t *)malloc(sizeof(dx7_voice_t));
    if (voice) {
        voice->status = DX7_VOICE_OFF;
    }
    return voice;
}

/*
 * dx7_voice_set_phase
 */
/* static inline */ void
dx7_voice_set_phase(hexter_instance_t *instance, dx7_voice_t *voice, int phase)
{
    int i;

    for (i = 0; i < MAX_DX7_OPERATORS; i++) {
        dx7_op_eg_set_phase(instance, &voice->op[i].eg, phase);
    }
    dx7_pitch_eg_set_phase(instance, &voice->pitch_eg, phase);
}

/*
 * dx7_voice_set_release_phase
 */
static inline void
dx7_voice_set_release_phase(hexter_instance_t *instance, dx7_voice_t *voice)
{
    dx7_voice_set_phase(instance, voice, 3);
}

/*
 * dx7_voice_note_on
 */
void
dx7_voice_note_on(hexter_instance_t *instance, dx7_voice_t *voice,
                  unsigned char key, unsigned char velocity)
{
    int i;

    voice->key      = key;
    voice->velocity = velocity;

    if (!instance->monophonic || !(_ON(voice) || _SUSTAINED(voice))) {

        /* brand-new voice, or monophonic voice in release phase; set
         * everything up */
        DEBUG_MESSAGE(DB_NOTE, " dx7_voice_note_on in polyphonic/new section: key %d, mono %d, old status %d\n", key, instance->monophonic, voice->status);

        dx7_voice_setup_note(instance, voice);

    } else {

        /* synth is monophonic, and we're modifying a playing voice */
        DEBUG_MESSAGE(DB_NOTE, " dx7_voice_note_on in monophonic section: old key %d => new key %d\n", instance->held_keys[0], key);

        /* retrigger LFO if needed */
        dx7_lfo_set(instance, voice);

        /* set new pitch */
        voice->mods_serial = instance->mods_serial - 1;
        /* -FIX- dx7_portamento_prepare(instance, voice); */
        dx7_voice_recalculate_freq_and_inc(instance, voice);

        /* if in 'on' or 'both' modes, and key has changed, then re-trigger EGs */
        if ((instance->monophonic == DSSP_MONO_MODE_ON ||
             instance->monophonic == DSSP_MONO_MODE_BOTH) &&
            (instance->held_keys[0] < 0 || instance->held_keys[0] != key)) {
            dx7_voice_set_phase(instance, voice, 0);
        }

        /* all other variables stay what they are */
    }

    instance->last_key = key;

    if (instance->monophonic) {

        /* add new key to the list of held keys */

        /* check if new key is already in the list; if so, move it to the
         * top of the list, otherwise shift the other keys down and add it
         * to the top of the list. */
        // DEBUG_MESSAGE(DB_NOTE, " note-on key list before: %d %d %d %d %d %d %d %d\n", instance->held_keys[0], instance->held_keys[1], instance->held_keys[2], instance->held_keys[3], instance->held_keys[4], instance->held_keys[5], instance->held_keys[6], instance->held_keys[7]);
        for (i = 0; i < 7; i++) {
            if (instance->held_keys[i] == key)
                break;
        }
        for (; i > 0; i--) {
            instance->held_keys[i] = instance->held_keys[i - 1];
        }
        instance->held_keys[0] = key;
        // DEBUG_MESSAGE(DB_NOTE, " note-on key list after: %d %d %d %d %d %d %d %d\n", instance->held_keys[0], instance->held_keys[1], instance->held_keys[2], instance->held_keys[3], instance->held_keys[4], instance->held_keys[5], instance->held_keys[6], instance->held_keys[7]);

    }

    if (!_PLAYING(voice)) {

        dx7_voice_start_voice(voice);

    } else if (!_ON(voice)) {  /* must be DX7_VOICE_SUSTAINED or DX7_VOICE_RELEASED */

        voice->status = DX7_VOICE_ON;

    }
}

/*
 * dx7_voice_note_off
 */
void
dx7_voice_note_off(hexter_instance_t *instance, dx7_voice_t *voice,
                   unsigned char key, unsigned char rvelocity)
{
    DEBUG_MESSAGE(DB_NOTE, " dx7_voice_note_off: called for voice %p, key %d\n", voice, key);

    /* save release velocity */
    voice->rvelocity = rvelocity;

    if (instance->monophonic) {  /* monophonic mode */

        if (instance->held_keys[0] >= 0) {  /* still some keys held */

            if (voice->key != instance->held_keys[0]) {

                /* most-recently-played key has changed */
                voice->key = instance->held_keys[0];
                DEBUG_MESSAGE(DB_NOTE, " note-off in monophonic section: changing pitch to %d\n", voice->key);
                voice->mods_serial = instance->mods_serial - 1;
                /* -FIX- dx7_portamento_prepare(instance, voice); */
                dx7_voice_recalculate_freq_and_inc(instance, voice);

                /* if mono mode is 'both', re-trigger EGs */
                if (instance->monophonic == DSSP_MONO_MODE_BOTH && !_RELEASED(voice)) {
                    dx7_voice_set_phase(instance, voice, 0);
                }
            }

        } else {  /* no keys still held */

            if (HEXTER_INSTANCE_SUSTAINED(instance)) {

                /* no more keys in list, but we're sustained */
                DEBUG_MESSAGE(DB_NOTE, " note-off in monophonic section: sustained with no held keys\n");
                if (!_RELEASED(voice))
                    voice->status = DX7_VOICE_SUSTAINED;

            } else {  /* not sustained */

                /* no more keys in list, so turn off note */
                DEBUG_MESSAGE(DB_NOTE, " note-off in monophonic section: turning off voice %p\n", voice);
                dx7_voice_set_release_phase(instance, voice);
                voice->status = DX7_VOICE_RELEASED;

            }
        }

    } else {  /* polyphonic mode */

        if (HEXTER_INSTANCE_SUSTAINED(instance)) {

            if (!_RELEASED(voice))
                voice->status = DX7_VOICE_SUSTAINED;

        } else {  /* not sustained */

            dx7_voice_set_release_phase(instance, voice);
            voice->status = DX7_VOICE_RELEASED;

        }
    }
}

/*
 * dx7_voice_release_note
 */
void
dx7_voice_release_note(hexter_instance_t *instance, dx7_voice_t *voice)
{
    DEBUG_MESSAGE(DB_NOTE, " dx7_voice_release_note: turning off voice %p\n", voice);
    if (_ON(voice)) {
        /* dummy up a release velocity */
        voice->rvelocity = 64;
    }
    dx7_voice_set_release_phase(instance, voice);
    voice->status = DX7_VOICE_RELEASED;
}

/* ===== operator (amplitude) envelope functions ===== */

/*
 * dx7_op_eg_set_increment
 */
void
dx7_op_eg_set_increment(hexter_instance_t *instance, dx7_op_eg_t *eg,
                        int new_rate, int new_level)
{
    int   current_level = FP_TO_INT(eg->value);
    int   need_compensation;
    float duration;

    eg->target = INT_TO_FP(new_level);

    if (eg->value <= eg->target) {  /* envelope will be rising */

        /* DX7 envelopes, when rising from levels <= 31 to levels
         * >= 32, include a compensation feature to speed the
         * attack, thereby making it sound more natural.  The
         * behavior of some of the boundary cases is bizarre, and
         * this has been exploited by some patch programmers (the
         * "Watergarden" patch found in the original ROM cartridge
         * is one example). We try to emulate it here: */

        if (eg->value <= INT_TO_FP(31)) {
            if (new_level > 31) {
                /* rise quickly to 31, then continue normally */
                need_compensation = 1;
                duration = dx7_voice_eg_rate_rise_duration[new_rate] *
                           (dx7_voice_eg_rate_rise_percent[new_level] -
                            dx7_voice_eg_rate_rise_percent[current_level]);
            } else if (new_level - current_level > 9) {
                /* these seem to take zero time */
                need_compensation = 0;
                duration = 0.0f;
            } else {
                /* these are the exploited delays */
                need_compensation = 0;
                /* -FIX- this doesn't make WATER GDN work? */
                duration = dx7_voice_eg_rate_rise_duration[new_rate] *
                           (float)(new_level - current_level) / 100.0f;
            }
        } else {
            need_compensation = 0;
            duration = dx7_voice_eg_rate_rise_duration[new_rate] *
                       (dx7_voice_eg_rate_rise_percent[new_level] -
                        dx7_voice_eg_rate_rise_percent[current_level]);
        }

    } else {

        need_compensation = 0;
        duration = dx7_voice_eg_rate_decay_duration[new_rate] *
                   (dx7_voice_eg_rate_decay_percent[current_level] -
                    dx7_voice_eg_rate_decay_percent[new_level]);

    }

    duration *= instance->sample_rate;

    eg->duration = lrintf(duration);
    if (eg->duration < 1)
        eg->duration = 1;

    if (need_compensation) {

        int32_t precomp_duration = FP_DIVIDE_CEIL(INT_TO_FP(31) - eg->value, instance->dx7_eg_max_slew);

        if (precomp_duration >= eg->duration) {

            eg->duration = precomp_duration;
            eg->increment = (eg->target - eg->value) / (dx7_sample_t)eg->duration;
            if (eg->increment > instance->dx7_eg_max_slew) {
                eg->duration = FP_DIVIDE_CEIL(eg->target - eg->value, instance->dx7_eg_max_slew);
                eg->increment = (eg->target - eg->value) / (dx7_sample_t)eg->duration;
            }
            eg->in_precomp = 0;

        } else if (precomp_duration < 1) {

            eg->increment = (eg->target - eg->value) / (dx7_sample_t)eg->duration;
            if (eg->increment > instance->dx7_eg_max_slew) {
                eg->duration = FP_DIVIDE_CEIL(eg->target - eg->value, instance->dx7_eg_max_slew);
                eg->increment = (eg->target - eg->value) / (dx7_sample_t)eg->duration;
            }
            eg->in_precomp = 0;

        } else {

            eg->postcomp_duration = eg->duration - precomp_duration;
            eg->duration = precomp_duration;
            eg->increment = (INT_TO_FP(31) - eg->value) / (dx7_sample_t)precomp_duration;
            eg->postcomp_increment = (eg->target - INT_TO_FP(31)) /
                                         (dx7_sample_t)eg->postcomp_duration;
            if (eg->postcomp_increment > instance->dx7_eg_max_slew) {
                eg->postcomp_duration = FP_DIVIDE_CEIL(eg->target - INT_TO_FP(31), instance->dx7_eg_max_slew);
                eg->postcomp_increment = (eg->target - INT_TO_FP(31)) /
                                             (dx7_sample_t)eg->postcomp_duration;
            }
            eg->in_precomp = 1;

        }

    } else {

        eg->increment = (eg->target - eg->value) / (dx7_sample_t)eg->duration;
        if (FP_ABS(eg->increment) > instance->dx7_eg_max_slew) {
            eg->duration = FP_DIVIDE_CEIL(FP_ABS(eg->target - eg->value), instance->dx7_eg_max_slew);
            eg->increment = (eg->target - eg->value) / (dx7_sample_t)eg->duration;
        }
        eg->in_precomp = 0;

    }
}

/*
 * dx7_op_eg_set_next_phase
 *
 * assumes a DX7_EG_RUNNING envelope
 */
void
dx7_op_eg_set_next_phase(hexter_instance_t *instance, dx7_op_eg_t *eg)
{
    switch (eg->phase) {

      case 0:
      case 1:
        eg->phase++;
	dx7_op_eg_set_increment(instance, eg, eg->rate[eg->phase], eg->level[eg->phase]);
#ifndef HEXTER_USE_FLOATING_POINT
        if (eg->duration == 1 && eg->increment == 0)
#else /* HEXTER_USE_FLOATING_POINT */
        if (eg->duration == 1 && fabsf(eg->increment) < 1e-10f)
#endif /* HEXTER_USE_FLOATING_POINT */
            dx7_op_eg_set_next_phase(instance, eg);
        break;

      case 2:
        eg->mode = DX7_EG_SUSTAINING;
        eg->increment = INT_TO_FP(0);
        eg->duration = -1;
        break;

      case 3:
      default: /* shouldn't be anything but 0 to 3 */
        eg->mode = DX7_EG_FINISHED;
        eg->increment = INT_TO_FP(0);
        eg->duration = -1;
        break;

    }
}

void
dx7_op_eg_set_phase(hexter_instance_t *instance, dx7_op_eg_t *eg, int phase)
{
    eg->phase = phase;

    if (phase == 0) {

        if (eg->level[0] == eg->level[1] &&
            eg->level[1] == eg->level[2] &&
            eg->level[2] == eg->level[3]) {

            eg->mode = DX7_EG_CONSTANT;
            eg->value = INT_TO_FP(eg->level[3]);
            eg->increment = INT_TO_FP(0);
            eg->duration = -1;

        } else {

            eg->mode = DX7_EG_RUNNING;
            dx7_op_eg_set_increment(instance, eg, eg->rate[phase], eg->level[phase]);
#ifndef HEXTER_USE_FLOATING_POINT
            if (eg->duration == 1 && eg->increment == 0)
#else /* HEXTER_USE_FLOATING_POINT */
            if (eg->duration == 1 && fabsf(eg->increment) < 1e-10f)
#endif /* HEXTER_USE_FLOATING_POINT */
                dx7_op_eg_set_next_phase(instance, eg);

        }
    } else {

        if (eg->mode != DX7_EG_CONSTANT) {

            eg->mode = DX7_EG_RUNNING;
            dx7_op_eg_set_increment(instance, eg, eg->rate[phase], eg->level[phase]);
#ifndef HEXTER_USE_FLOATING_POINT
            if (eg->duration == 1 && eg->increment == 0)
#else /* HEXTER_USE_FLOATING_POINT */
            if (eg->duration == 1 && fabsf(eg->increment) < 1e-10f)
#endif /* HEXTER_USE_FLOATING_POINT */
                dx7_op_eg_set_next_phase(instance, eg);

        }
    }
}

void
dx7_op_envelope_prepare(hexter_instance_t *instance, dx7_op_t *op,
                        int transposed_note, int velocity)
{
    int scaled_output_level, i, rate_bump;
    float vel_adj;

    scaled_output_level = op->output_level;

    /* things that affect breakpoint calculations: transpose, ? */
    /* things that don't affect breakpoint calculations: pitch envelope, ? */

    if (transposed_note < op->level_scaling_bkpoint + 21 && op->level_scaling_l_depth) {

        /* On the original DX7/TX7, keyboard level scaling calculations
         * group the keyboard into groups of three keys.  This can be quite
         * noticeable on patches with extreme scaling depths, so I've tried
         * to replicate it here (the steps between levels may not occur at
         * exactly the keys).  If you'd prefer smother scaling, define
         * SMOOTH_KEYBOARD_LEVEL_SCALING. */
#ifndef SMOOTH_KEYBOARD_LEVEL_SCALING
        i = op->level_scaling_bkpoint - (((transposed_note + 2) / 3) * 3) + 21;
#else
        i = op->level_scaling_bkpoint - transposed_note + 21;
#endif

        switch(op->level_scaling_l_curve) {
          case 0: /* -LIN */
            scaled_output_level -= (int)((float)i / 45.0f * (float)op->level_scaling_l_depth);
            break;
          case 1: /* -EXP */
            scaled_output_level -= (int)(exp((float)(i - 72) / 13.5f) * (float)op->level_scaling_l_depth);
            break;
          case 2: /* +EXP */
            scaled_output_level += (int)(exp((float)(i - 72) / 13.5f) * (float)op->level_scaling_l_depth);
            break;
          case 3: /* +LIN */
            scaled_output_level += (int)((float)i / 45.0f * (float)op->level_scaling_l_depth);
            break;
        }
        if (scaled_output_level < 0)  scaled_output_level = 0;
        if (scaled_output_level > 99) scaled_output_level = 99;

    } else if (transposed_note > op->level_scaling_bkpoint + 21 && op->level_scaling_r_depth) {

#ifndef SMOOTH_KEYBOARD_LEVEL_SCALING
        i = (((transposed_note + 2) / 3) * 3) - op->level_scaling_bkpoint - 21;
#else
        i = transposed_note - op->level_scaling_bkpoint - 21;
#endif

        switch(op->level_scaling_r_curve) {
          case 0: /* -LIN */
            scaled_output_level -= (int)((float)i / 45.0f * (float)op->level_scaling_r_depth);
            break;
          case 1: /* -EXP */
            scaled_output_level -= (int)(exp((float)(i - 72) / 13.5f) * (float)op->level_scaling_r_depth);
            break;
          case 2: /* +EXP */
            scaled_output_level += (int)(exp((float)(i - 72) / 13.5f) * (float)op->level_scaling_r_depth);
            break;
          case 3: /* +LIN */
            scaled_output_level += (int)((float)i / 45.0f * (float)op->level_scaling_r_depth);
            break;
        }
        if (scaled_output_level < 0)  scaled_output_level = 0;
        if (scaled_output_level > 99) scaled_output_level = 99;
    }

    vel_adj = dx7_voice_velocity_ol_adjustment[velocity] * (float)op->velocity_sens;

    /* DEBUG_MESSAGE(DB_NOTE, " dx7_op_envelope_prepare: s_o_l=%d, vel_adj=%f\n", scaled_output_level, vel_adj); */

    /* -FIX- This calculation comes from Pinkston/Harrington; the original "* 6.0" scaling factor
     * was close to what my TX7 does, but tended to not bump the rate as much, so I changed it
     * to "* 6.5" which seems a little closer, but it's still not spot-on. */
    /* Things which affect this calculation: transpose, ? */
    /* rate_bump = lrintf((float)op->rate_scaling * (float)(transposed_note - 21) / (126.0f - 21.0f) * 127.0f / 128.0f * 6.0f - 0.5f); */
    rate_bump = lrintf((float)op->rate_scaling * (float)(transposed_note - 21) / (126.0f - 21.0f) * 127.0f / 128.0f * 6.5f - 0.5f);
    /* -FIX- just a hunch: try it again with "* 6.0f" but also "(120.0f - 21.0f)" instead of "(126.0f - 21.0f)": */
    /* rate_bump = lrintf((float)op->rate_scaling * (float)(transposed_note - 21) / (120.0f - 21.0f) * 127.0f / 128.0f * 6.0f - 0.5f); */

    for (i=0;i<4;i++) {

        float level = (float)op->eg.base_level[i];

        /* -FIX- is this scaling of eg.base_level values to og.level values correct, i.e. does a softer
         * velocity shorten the time, since the rate stays the same? */
        level = level * (float)scaled_output_level / 99.0f + vel_adj;
        if (level < 0.0f)
            level = 0.0f;
        else if (level > 99.0f)
            level = 99.0f;

        op->eg.level[i] = lrintf(level);

        op->eg.rate[i] = op->eg.base_rate[i] + rate_bump;
        if (op->eg.rate[i] > 99) op->eg.rate[i] = 99;

    }

    op->eg.value = INT_TO_FP(op->eg.level[3]);

    dx7_op_eg_set_phase(instance, &op->eg, 0);
}

void
dx7_eg_init_constants(hexter_instance_t *instance)
{
    float duration = dx7_voice_eg_rate_rise_duration[99] *
                     (dx7_voice_eg_rate_rise_percent[99] -
                      dx7_voice_eg_rate_rise_percent[0]);

    instance->dx7_eg_max_slew = FLOAT_TO_FP(99.0f / (duration * instance->sample_rate));

    instance->nugget_rate = instance->sample_rate / (float)HEXTER_NUGGET_SIZE;

    instance->ramp_duration = lrintf(instance->sample_rate * 0.006f);  /* 6ms ramp */
}

/* ===== pitch envelope functions ===== */

/*
 * dx7_pitch_eg_set_increment
 */
void
dx7_pitch_eg_set_increment(hexter_instance_t *instance, dx7_pitch_eg_t *eg,
                           int new_rate, int new_level)
{
    double duration;

    /* translate 0-99 level to shift in semitones */
    eg->target = dx7_voice_pitch_level_to_shift[new_level];

    /* -FIX- This is just a quick approximation that I derived from
     * regression of Godric Wilkie's pitch eg timings. In particular,
     * it's not accurate for very slow envelopes. */
    duration = exp(((double)new_rate - 70.337897) / -25.580953) *
               fabs((eg->target - eg->value) / 96.0);

    duration *= (double)instance->nugget_rate;

    eg->duration = lrint(duration);

    if (eg->duration > 1) {

        eg->increment = (eg->target - eg->value) / (dx7_sample_t)eg->duration;

    } else {

        eg->duration = 1;
        eg->increment = eg->target - eg->value;

    }
}

/*
 * dx7_pitch_eg_set_next_phase
 *
 * assumes a DX7_EG_RUNNING envelope
 */
void
dx7_pitch_eg_set_next_phase(hexter_instance_t *instance, dx7_pitch_eg_t *eg)
{
    switch (eg->phase) {

      case 0:
      case 1:
        eg->phase++;
	dx7_pitch_eg_set_increment(instance, eg, eg->rate[eg->phase],
                                   eg->level[eg->phase]);
        break;

      case 2:
        eg->mode = DX7_EG_SUSTAINING;
        break;

      case 3:
      default: /* shouldn't be anything but 0 to 3 */
        eg->mode = DX7_EG_FINISHED;
        break;

    }
}

void
dx7_pitch_eg_set_phase(hexter_instance_t *instance, dx7_pitch_eg_t *eg, int phase)
{
    eg->phase = phase;

    if (phase == 0) {

        if (eg->level[0] == eg->level[1] &&
            eg->level[1] == eg->level[2] &&
            eg->level[2] == eg->level[3]) {

            eg->mode = DX7_EG_CONSTANT;
            eg->value = dx7_voice_pitch_level_to_shift[eg->level[3]];

        } else {

            eg->mode = DX7_EG_RUNNING;
            dx7_pitch_eg_set_increment(instance, eg, eg->rate[phase], eg->level[phase]);

        }
    } else {

        if (eg->mode != DX7_EG_CONSTANT) {

            eg->mode = DX7_EG_RUNNING;
            dx7_pitch_eg_set_increment(instance, eg, eg->rate[phase], eg->level[phase]);

        }
    }
}

void
dx7_pitch_envelope_prepare(hexter_instance_t *instance, dx7_voice_t *voice)
{
    voice->pitch_eg.value = dx7_voice_pitch_level_to_shift[voice->pitch_eg.level[3]];
    dx7_pitch_eg_set_phase(instance, &voice->pitch_eg, 0);
}

/* ===== portamento functions ===== */

void
dx7_portamento_set_segment(hexter_instance_t *instance, dx7_portamento_t *port)
{
    /* -FIX- implement portamento multi-segment curve */
    port->increment = (port->target - port->value) / (double)port->duration;
}

void
dx7_portamento_prepare(hexter_instance_t *instance, dx7_voice_t *voice)
{
    dx7_portamento_t *port = &voice->portamento;

    if (instance->portamento_time == 0 ||
        instance->last_key == voice->key) {

        port->segment = 0;
        port->value = 0.0;

    } else {

        /* -FIX- implement portamento time and multi-segment curve */
        float t = expf((float)(instance->portamento_time - 99) / 15.0f) * 18.0f; /* not at all related to what a real DX7 does */
        port->segment = 1;
        port->value = (double)(instance->last_key - voice->key);
        port->duration = lrintf(instance->nugget_rate * t);
        port->target = 0.0;

        dx7_portamento_set_segment(instance, port);
    }
}

/* ===== frequency related functions ===== */

static inline int
limit_note(int note) {
    while (note < 0)   note += 12;
    while (note > 127) note -= 12;
    return note;
}

void
dx7_op_recalculate_increment(hexter_instance_t *instance, dx7_op_t *op)
{
    double freq;

    if (op->osc_mode) { /* fixed frequency */
        /* pitch envelope does not affect this */

        /* -FIX- convert this to a table lookup for speed? */
        freq = instance->fixed_freq_multiplier *
                   exp(M_LN10 * ((double)(op->coarse & 3) + (double)op->fine / 100.0));
        /* -FIX- figure out what to do with detune */

    } else {
    
        freq = op->frequency;
        freq += ((double)op->detune - 7.0) / 32.0; /* -FIX- is this correct? */
        if (op->coarse) {
            freq = freq * (double)op->coarse;
        } else {
            freq = freq / 2.0;
        }
        freq *= (1.0 + ((double)op->fine / 100.0));

    }
    op->phase_increment = DOUBLE_TO_FP(freq / (double)instance->sample_rate);
}

double
dx7_voice_recalculate_frequency(hexter_instance_t *instance, dx7_voice_t *voice)
{
    double freq;

    voice->last_port_tuning = *instance->tuning;

    instance->fixed_freq_multiplier = *instance->tuning / 440.0;

    freq = voice->pitch_eg.value + voice->portamento.value +
           instance->pitch_bend -
           instance->lfo_value_for_pitch *
               (voice->pitch_mod_depth_pmd * FP_TO_DOUBLE(voice->lfo_delay_value) +
                voice->pitch_mod_depth_mods);

    voice->last_pitch = freq;

    freq += (double)(limit_note(voice->key + voice->transpose - 24));

    /* -FIX- this maybe could be optimized */
    /*       a lookup table of 8k values would give ~1.5 cent accuracy,
     *       but then would interpolating that be faster than exp()? */
    freq = *instance->tuning * exp((freq - 69.0) * M_LN2 / 12.0);

    return freq;
}

void
dx7_voice_recalculate_freq_and_inc(hexter_instance_t *instance,
                                   dx7_voice_t *voice)
{
    double freq = dx7_voice_recalculate_frequency(instance, voice);
    int i;

    for (i = 0; i < 6; i++) {
        voice->op[i].frequency = freq;
        dx7_op_recalculate_increment(instance, &voice->op[i]);
    }
}

/* ===== output volume ===== */

void
dx7_voice_recalculate_volume(hexter_instance_t *instance, dx7_voice_t *voice)
{
    float f;
    int i;

    voice->last_port_volume = *instance->volume;
    voice->last_cc_volume = instance->cc_volume;

    /* This 41 OL volume cc mapping matches my TX7 fairly well, to within
     * +/-0.8dB for most of the scale. (It even duplicates the "feature"
     * of not going completely silent at zero....) */
    f = (*instance->volume - 20.0f) * 1.328771f + 86.0f;
    f += (float)instance->cc_volume * 41.0f / 16256.0f;
    i = lrintf(f - 0.5f);
    f -= (float)i;
    voice->volume_target = (FP_TO_FLOAT(dx7_voice_eg_ol_to_mod_index[i]) +
                            f * FP_TO_FLOAT(dx7_voice_eg_ol_to_mod_index[i + 1] -
                                            dx7_voice_eg_ol_to_mod_index[i]))
                           / 2.08855f  /* scale modulation index to output amplitude */
                           / dx7_voice_carrier_count[voice->algorithm]  /* scale for number of carriers */
                           * 0.110384f;  /* Where did this value come from? It approximates the
                                          * -18.1dBFS nominal per-voice output level hexter should
                                          * have, but then why didn't I just use 0.125f like in
                                          * hexter 0.5.7? */

    if (voice->volume_value < 0.0f) { /* initial setup */
        voice->volume_value = voice->volume_target;
        voice->volume_duration = 0;
    } else {
        voice->volume_duration = instance->ramp_duration;
        voice->volume_increment = (voice->volume_target - voice->volume_value) /
                                      (float)voice->volume_duration;
    }
}

/* ===== LFO functions ===== */

/* dx7_lfo_set_speed
 *
 * called by dx7_lfo_reset() and dx7_lfo_set() to set LFO speed and phase
 */
static inline void
dx7_lfo_set_speed(hexter_instance_t *instance)
{
    int32_t period = lrintf(instance->sample_rate /
                            dx7_voice_lfo_frequency[instance->lfo_speed]);

    switch (instance->lfo_wave) {
      default:
      case 0:  /* triangle */
        instance->lfo_phase = 0;
        instance->lfo_value = INT_TO_FP(0);
        instance->lfo_duration0 = period / 2;
        instance->lfo_duration1 = period - instance->lfo_duration0;
        instance->lfo_increment0 = INT_TO_FP(1) / (dx7_sample_t)instance->lfo_duration0;
        instance->lfo_increment1 = -instance->lfo_increment0;
        instance->lfo_duration = instance->lfo_duration0;
        instance->lfo_increment = instance->lfo_increment0;
        break;
      case 1:  /* saw down */
        instance->lfo_phase = 0;
        instance->lfo_value = INT_TO_FP(0);
        if (period >= (instance->ramp_duration * 4)) {
            instance->lfo_duration0 = period - instance->ramp_duration;
            instance->lfo_duration1 = instance->ramp_duration;
        } else {
            instance->lfo_duration0 = period * 3 / 4;
            instance->lfo_duration1 = period - instance->lfo_duration0;
        }
        instance->lfo_increment0 = INT_TO_FP(1)  / (dx7_sample_t)instance->lfo_duration0;
        instance->lfo_increment1 = INT_TO_FP(-1) / (dx7_sample_t)instance->lfo_duration1;
        instance->lfo_duration = instance->lfo_duration0;
        instance->lfo_increment = instance->lfo_increment0;
        break;
      case 2:  /* saw up */
        instance->lfo_phase = 1;
        instance->lfo_value = INT_TO_FP(1);
        if (period >= (instance->ramp_duration * 4)) {
            instance->lfo_duration0 = instance->ramp_duration;
            instance->lfo_duration1 = period - instance->ramp_duration;
        } else {
            instance->lfo_duration1 = period * 3 / 4;
            instance->lfo_duration0 = period - instance->lfo_duration1;
        }
        instance->lfo_increment0 = INT_TO_FP(1)  / (dx7_sample_t)instance->lfo_duration0;
        instance->lfo_increment1 = INT_TO_FP(-1) / (dx7_sample_t)instance->lfo_duration1;
        instance->lfo_duration = instance->lfo_duration1;
        instance->lfo_increment = instance->lfo_increment1;
        break;
      case 3:  /* square */
        instance->lfo_phase = 0;
        instance->lfo_value = INT_TO_FP(1);
        if (period >= (instance->ramp_duration * 6)) {
            instance->lfo_duration0 = (period / 2) - instance->ramp_duration;
            instance->lfo_duration1 = instance->ramp_duration;
        } else {
            instance->lfo_duration0 = period / 3;
            instance->lfo_duration1 = (period / 2) - instance->lfo_duration0;
        }
        instance->lfo_increment1 = INT_TO_FP(1) / (dx7_sample_t)instance->lfo_duration1;
        instance->lfo_increment0 = -instance->lfo_increment1;
        instance->lfo_duration = instance->lfo_duration0;
        instance->lfo_increment = INT_TO_FP(0);
        break;
      case 4:  /* sine */
#ifndef HEXTER_USE_FLOATING_POINT
        instance->lfo_value = FP_SIZE / 4; /* phase of pi/2 in cosine table */
#else /* HEXTER_USE_FLOATING_POINT */
        instance->lfo_value = 0.25f;       /* phase of pi/2 in cosine table */
#endif /* HEXTER_USE_FLOATING_POINT */
        instance->lfo_increment = INT_TO_FP(1) / (dx7_sample_t)period;
        break;
      case 5:  /* sample/hold */
        instance->lfo_phase = 0;
        instance->lfo_value = FP_RAND();
        if (period >= (instance->ramp_duration * 4)) {
            instance->lfo_duration0 = period - instance->ramp_duration;
            instance->lfo_duration1 = instance->ramp_duration;
        } else {
            instance->lfo_duration0 = period * 3 / 4;
            instance->lfo_duration1 = period - instance->lfo_duration0;
        }
        instance->lfo_duration = instance->lfo_duration0;
        instance->lfo_increment = INT_TO_FP(0);
        break;
    }
}

/*
 * dx7_lfo_reset
 *
 * called from hexter_activate() to give instance LFO parameters sane values
 * until they're set by a playing voice
 */
void
dx7_lfo_reset(hexter_instance_t *instance)
{
    instance->lfo_speed = 20;
    instance->lfo_wave = 1;
    instance->lfo_delay = 255;  /* force setup at first note on */
    instance->lfo_value_for_pitch = 0.0;
    dx7_lfo_set_speed(instance);
}

void
dx7_lfo_set(hexter_instance_t *instance, dx7_voice_t *voice)
{
    int set_speed = 0;

    instance->lfo_wave = voice->lfo_wave;
    if (instance->lfo_speed != voice->lfo_speed) {
        instance->lfo_speed = voice->lfo_speed;
        set_speed = 1;
    }
    if (voice->lfo_key_sync) {
        set_speed = 1; /* because we need to reset the LFO phase */
    }
    if (set_speed)
        dx7_lfo_set_speed(instance);
    if (instance->lfo_delay != voice->lfo_delay) {
        instance->lfo_delay = voice->lfo_delay;
        if (voice->lfo_delay > 0) {
            instance->lfo_delay_value[0] = INT_TO_FP(0);
            /* -FIX- Jamie's early approximation, replace when he has more data */
            instance->lfo_delay_duration[0] =
                lrintf(instance->sample_rate *
                       (0.00175338f * pow((float)voice->lfo_delay, 3.10454f) + 169.344f - 168.0f) /
                       1000.0f);
            instance->lfo_delay_increment[0] = INT_TO_FP(0);
            instance->lfo_delay_value[1] = INT_TO_FP(0);
            /* -FIX- Jamie's early approximation, replace when he has more data */
            instance->lfo_delay_duration[1] =
                lrintf(instance->sample_rate *
                       (0.321877f * pow((float)voice->lfo_delay, 2.01163) + 494.201f - 168.0f) /
                       1000.0f);                                                 /* time from note-on until full on */
            instance->lfo_delay_duration[1] -= instance->lfo_delay_duration[0];  /* now time from end-of-delay until full */
            instance->lfo_delay_increment[1] = INT_TO_FP(1) / (dx7_sample_t)instance->lfo_delay_duration[1];
            instance->lfo_delay_value[2] = INT_TO_FP(1);
            instance->lfo_delay_duration[2] = 0;
            instance->lfo_delay_increment[2] = INT_TO_FP(0);
        } else {
            instance->lfo_delay_value[0] = INT_TO_FP(1);
            instance->lfo_delay_duration[0] = 0;
            instance->lfo_delay_increment[0] = INT_TO_FP(0);
        }
        /* -FIX- The TX7 resets the lfo delay for all playing notes at each
         * new note on. We're not doing that yet, and I don't really wanna,
         * 'cause it's stupid.... */
    }
}

void
dx7_lfo_update(hexter_instance_t *instance, unsigned long sample_count)
{
    unsigned long sample;

    switch (instance->lfo_wave) {
      default:
      case 0:  /* triangle */
      case 1:  /* saw down */
      case 2:  /* saw up */
        for (sample = 0; sample < sample_count; sample++) {
            instance->lfo_buffer[sample] = instance->lfo_value;
            instance->lfo_value += instance->lfo_increment;
            if (!(--instance->lfo_duration)) {
                if (instance->lfo_phase) {
                    instance->lfo_phase = 0;
                    instance->lfo_value = INT_TO_FP(0);
                    instance->lfo_duration = instance->lfo_duration0;
                    instance->lfo_increment = instance->lfo_increment0;
                } else {
                    instance->lfo_phase = 1;
                    instance->lfo_value = INT_TO_FP(1);
                    instance->lfo_duration = instance->lfo_duration1;
                    instance->lfo_increment = instance->lfo_increment1;
                }
            }
        }
        instance->lfo_value_for_pitch = FP_TO_DOUBLE(instance->lfo_value) * 2.0 - 1.0;  /* -FIX- this is still ramped for saw! */
        break;
      case 3:  /* square */
        for (sample = 0; sample < sample_count; sample++) {
            instance->lfo_buffer[sample] = instance->lfo_value;
            instance->lfo_value += instance->lfo_increment;
            if (!(--instance->lfo_duration)) {
                switch (instance->lfo_phase) {
                  default:
                  case 0:
                    instance->lfo_phase = 1;
                    instance->lfo_duration = instance->lfo_duration1;
                    instance->lfo_increment = instance->lfo_increment0;
                    break;
                  case 1:
                    instance->lfo_phase = 2;
                    instance->lfo_value = INT_TO_FP(0);
                    instance->lfo_duration = instance->lfo_duration0;
                    instance->lfo_increment = INT_TO_FP(0);
                    break;
                  case 2:
                    instance->lfo_phase = 3;
                    instance->lfo_duration = instance->lfo_duration1;
                    instance->lfo_increment = instance->lfo_increment1;
                    break;
                  case 3:
                    instance->lfo_phase = 0;
                    instance->lfo_value = INT_TO_FP(1);
                    instance->lfo_duration = instance->lfo_duration0;
                    instance->lfo_increment = INT_TO_FP(0);
                    break;
                }
            }
        }
        if (instance->lfo_phase == 0 || instance->lfo_phase == 3)
            instance->lfo_value_for_pitch = 1.0;
        else
            instance->lfo_value_for_pitch = -1.0;
        break;
      case 4:  /* sine */
        for (sample = 0; sample < sample_count; sample++) {
#ifndef HEXTER_USE_FLOATING_POINT
            int32_t phase, index, out;
#else /* HEXTER_USE_FLOATING_POINT */
            int32_t index;
            float phase, frac, out;
#endif /* HEXTER_USE_FLOATING_POINT */

#ifndef HEXTER_USE_FLOATING_POINT
            phase = instance->lfo_value;
            index = (phase >> FP_TO_SINE_SHIFT) & SINE_MASK;
            out = dx7_voice_sin_table[index];
            out += (((int64_t)(dx7_voice_sin_table[index + 1] - out) *
                     (int64_t)(phase & FP_TO_SINE_MASK)) >>
                    (FP_SHIFT + FP_TO_SINE_SHIFT));
            out = (out + FP_SIZE) >> 1;  /* shift to unipolar */
#else /* HEXTER_USE_FLOATING_POINT */
            phase = instance->lfo_value * (float)SINE_SIZE;
            index = lrintf(phase - 0.5f);
            frac = phase - (float)index;
            out = dx7_voice_sin_table[index];
            out += (dx7_voice_sin_table[index + 1] - out) * frac;
            out = (out + 1.0f) / 2.0f;  /* shift to unipolar */
#endif /* HEXTER_USE_FLOATING_POINT */
            instance->lfo_buffer[sample] = out;
            instance->lfo_value += instance->lfo_increment;
#ifdef HEXTER_USE_FLOATING_POINT
            if (instance->lfo_value > 1.0f) instance->lfo_value -= 1.0f;
#endif /* HEXTER_USE_FLOATING_POINT */
        }
        instance->lfo_value_for_pitch = FP_TO_DOUBLE(instance->lfo_buffer[sample - 1]) * 2.0 - 1.0;
        break;
      case 5:  /* sample/hold */
        for (sample = 0; sample < sample_count; sample++) {
            instance->lfo_buffer[sample] = instance->lfo_value;
            instance->lfo_value += instance->lfo_increment;
            if (!(--instance->lfo_duration)) {
                if (instance->lfo_phase) {
                    instance->lfo_phase = 0;
                    instance->lfo_value = instance->lfo_target;
                    instance->lfo_duration = instance->lfo_duration0;
                    instance->lfo_increment = INT_TO_FP(0);
                } else {
                    instance->lfo_phase = 1;
                    instance->lfo_duration = instance->lfo_duration1;
                    instance->lfo_target = FP_RAND();
                    instance->lfo_increment = (instance->lfo_target - instance->lfo_value) /
                                                  (dx7_sample_t)instance->lfo_duration;
                }
            }
        }
        instance->lfo_value_for_pitch = FP_TO_DOUBLE(instance->lfo_target) * 2.0 - 1.0;
        break;
    }
}

/* ===== modulation functions ===== */

/* there used to be a dx7_voice_update_pitch_bend() here, but it didn't
 * have anything to do.... */

void
dx7_voice_update_mod_depths(hexter_instance_t *instance, dx7_voice_t* voice)
{
    unsigned char kp = instance->key_pressure[voice->key];
    unsigned char cp = instance->channel_pressure;
    float pressure;
    float pdepth, adepth, mdepth, edepth;

    /* add the channel and key pressures together in a way that 'feels' good */
    if (kp > cp) {
        pressure = (float)kp / 127.0f;
        pressure += (1.0f - pressure) * ((float)cp / 127.0f);
    } else {
        pressure = (float)cp / 127.0f;
        pressure += (1.0f - pressure) * ((float)kp / 127.0f);
    }

    /* calculate modulation depths */
    pdepth = (float)voice->lfo_pmd / 99.0f;
    voice->pitch_mod_depth_pmd = (double)dx7_voice_pms_to_semitones[voice->lfo_pms] *
                                 (double)pdepth;
    // -FIX- this could be optimized:
    // -FIX- this just adds everything together -- maybe it should limit the result, or
    // combine the various mods like update_pressure() does
    pdepth = (instance->mod_wheel_assign & 0x01 ?
                 // -FIX- this assumes that mod_wheel_sensitivity, etc. scale linearly => verify
                 (float)instance->mod_wheel_sensitivity / 15.0f * instance->mod_wheel :
                 0.0f) +
             (instance->foot_assign & 0x01 ?
                 (float)instance->foot_sensitivity / 15.0f * instance->foot :
                 0.0f) +
             (instance->pressure_assign & 0x01 ?
                 (float)instance->pressure_sensitivity / 15.0f * pressure :
                 0.0f) +
             (instance->breath_assign & 0x01 ?
                 (float)instance->breath_sensitivity / 15.0f * instance->breath :
                 0.0f);
    voice->pitch_mod_depth_mods = (double)dx7_voice_pms_to_semitones[voice->lfo_pms] *
                                  (double)pdepth;

    // -FIX- these are total guesses at how to combine/limit the amp mods:
    adepth = dx7_voice_amd_to_ol_adjustment[voice->lfo_amd];
    // -FIX- this could be optimized:
    mdepth = (instance->mod_wheel_assign & 0x02 ?
                 dx7_voice_mss_to_ol_adjustment[instance->mod_wheel_sensitivity] *
                     instance->mod_wheel :
                 0.0f) +
             (instance->foot_assign & 0x02 ?
                 dx7_voice_mss_to_ol_adjustment[instance->foot_sensitivity] *
                     instance->foot :
                 0.0f) +
             (instance->pressure_assign & 0x02 ?
                 dx7_voice_mss_to_ol_adjustment[instance->pressure_sensitivity] *
                     pressure :
                 0.0f) +
             (instance->breath_assign & 0x02 ?
                 dx7_voice_mss_to_ol_adjustment[instance->breath_sensitivity] *
                     instance->breath :
                 0.0f);
    edepth = // -FIX- this could be optimized:
             (instance->mod_wheel_assign & 0x04 ?
                 dx7_voice_mss_to_ol_adjustment[instance->mod_wheel_sensitivity] *
                     (1.0f - instance->mod_wheel) :
                 0.0f) +
             (instance->foot_assign & 0x04 ?
                 dx7_voice_mss_to_ol_adjustment[instance->foot_sensitivity] *
                     (1.0f - instance->foot) :
                 0.0f) +
             (instance->pressure_assign & 0x04 ?
                 dx7_voice_mss_to_ol_adjustment[instance->pressure_sensitivity] *
                     (1.0f - pressure) :
                 0.0f) +
             (instance->breath_assign & 0x04 ?
                 dx7_voice_mss_to_ol_adjustment[instance->breath_sensitivity] *
                     (1.0f - instance->breath) :
                 0.0f);

    /* full-scale amp mod for adepth and edepth should be 52.75 and
     * their sum _must_ be limited to less than 128, or bad things will happen! */
    if (adepth > 127.5f) adepth = 127.5f;
    if (adepth + mdepth > 127.5f)
        mdepth = 127.5f - adepth;
    if (adepth + mdepth + edepth > 127.5f)
        edepth = 127.5f - (adepth + mdepth);

    voice->amp_mod_lfo_amd_target = FLOAT_TO_FP(adepth);
    if (voice->amp_mod_lfo_amd_value <= INT_TO_FP(-64)) {
        voice->amp_mod_lfo_amd_value = voice->amp_mod_lfo_amd_target;
        voice->amp_mod_lfo_amd_increment = INT_TO_FP(0);
        voice->amp_mod_lfo_amd_duration = 0;
    } else {
        voice->amp_mod_lfo_amd_duration = instance->ramp_duration;
        voice->amp_mod_lfo_amd_increment = (voice->amp_mod_lfo_amd_target - voice->amp_mod_lfo_amd_value) /
                                               (dx7_sample_t)voice->amp_mod_lfo_amd_duration;
    }
    voice->amp_mod_lfo_mods_target = FLOAT_TO_FP(mdepth);
    if (voice->amp_mod_lfo_mods_value <= INT_TO_FP(-64)) {
        voice->amp_mod_lfo_mods_value = voice->amp_mod_lfo_mods_target;
        voice->amp_mod_lfo_mods_increment = INT_TO_FP(0);
        voice->amp_mod_lfo_mods_duration = 0;
    } else {
        voice->amp_mod_lfo_mods_duration = instance->ramp_duration;
        voice->amp_mod_lfo_mods_increment = (voice->amp_mod_lfo_mods_target - voice->amp_mod_lfo_mods_value) /
                                                (dx7_sample_t)voice->amp_mod_lfo_mods_duration;
    }
    voice->amp_mod_env_target = FLOAT_TO_FP(edepth);
    if (voice->amp_mod_env_value <= INT_TO_FP(-64)) {
        voice->amp_mod_env_value = voice->amp_mod_env_target;
        voice->amp_mod_env_increment = INT_TO_FP(0);
        voice->amp_mod_env_duration = 0;
    } else {
        voice->amp_mod_env_duration = instance->ramp_duration;
        voice->amp_mod_env_increment = (voice->amp_mod_env_target - voice->amp_mod_env_value) /
                                           (dx7_sample_t)voice->amp_mod_env_duration;
    }
}

/* ===== whole patch related functions ===== */

void
dx7_voice_calculate_runtime_parameters(hexter_instance_t *instance, dx7_voice_t* voice)
{
    int i;
    double freq;

    dx7_pitch_envelope_prepare(instance, voice);
    voice->amp_mod_lfo_amd_value = INT_TO_FP(-65);   /* force initial setup */
    voice->amp_mod_lfo_mods_value = INT_TO_FP(-65);
    voice->amp_mod_env_value = INT_TO_FP(-65);
    voice->lfo_delay_segment = 0;
    voice->lfo_delay_value     = instance->lfo_delay_value[0];
    voice->lfo_delay_duration  = instance->lfo_delay_duration[0];
    voice->lfo_delay_increment = instance->lfo_delay_increment[0];
    voice->mods_serial = instance->mods_serial - 1;  /* force mod depths update */
    dx7_portamento_prepare(instance, voice);
    freq = dx7_voice_recalculate_frequency(instance, voice);

    voice->volume_value = -1.0f;                     /* force initial setup */
    dx7_voice_recalculate_volume(instance, voice);

    for (i = 0; i < MAX_DX7_OPERATORS; i++) {
        voice->op[i].frequency = freq;
        if (voice->osc_key_sync) {
            voice->op[i].phase = INT_TO_FP(0);
        }
        dx7_op_recalculate_increment(instance, &voice->op[i]);
        dx7_op_envelope_prepare(instance, &voice->op[i],
                                limit_note(voice->key + voice->transpose - 24),
                                voice->velocity);
    }
}

/*
 * dx7_voice_setup_note
 */
void
dx7_voice_setup_note(hexter_instance_t *instance, dx7_voice_t *voice)
{
    dx7_voice_set_data(instance, voice);
    hexter_instance_set_performance_data(instance);
    dx7_lfo_set(instance, voice);
    dx7_voice_calculate_runtime_parameters(instance, voice);
}

static inline int
limit(int x, int min, int max)
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

/*
 * dx7_voice_set_data
 */
void
dx7_voice_set_data(hexter_instance_t *instance, dx7_voice_t *voice)
{
    uint8_t *edit_buffer = instance->current_patch_buffer;
    int compat059 = (instance->performance_buffer[0] & 0x01);  /* 0.5.9 compatibility */
    int i, j;
    double aux_feedbk;

    for (i = 0; i < MAX_DX7_OPERATORS; i++) {
	uint8_t *eb_op = edit_buffer + ((5 - i) * 21);

        voice->op[i].output_level  = limit(eb_op[16], 0, 99);

	voice->op[i].osc_mode      = eb_op[17] & 0x01;
        voice->op[i].coarse        = eb_op[18] & 0x1f;
        voice->op[i].fine          = limit(eb_op[19], 0, 99);
	voice->op[i].detune        = limit(eb_op[20], 0, 14);

        voice->op[i].level_scaling_bkpoint = limit(eb_op[ 8], 0, 99);
        voice->op[i].level_scaling_l_depth = limit(eb_op[ 9], 0, 99);
        voice->op[i].level_scaling_r_depth = limit(eb_op[10], 0, 99);
        voice->op[i].level_scaling_l_curve = eb_op[11] & 0x03;
        voice->op[i].level_scaling_r_curve = eb_op[12] & 0x03;
        voice->op[i].rate_scaling          = eb_op[13] & 0x07;
        voice->op[i].amp_mod_sens          = (compat059 ? 0 : eb_op[14] & 0x03);
        voice->op[i].velocity_sens         = eb_op[15] & 0x07;

        for (j = 0; j < 4; j++) {
            voice->op[i].eg.base_rate[j]  = limit(eb_op[j], 0, 99);
            voice->op[i].eg.base_level[j] = limit(eb_op[4 + j], 0, 99);
        }
    }

    for (i = 0; i < 4; i++) {
        voice->pitch_eg.rate[i]  = limit(edit_buffer[126 + i], 0, 99);
        voice->pitch_eg.level[i] = limit(edit_buffer[130 + i], 0, 99);
    }

    voice->algorithm = edit_buffer[134] & 0x1f;

    aux_feedbk = (double)(edit_buffer[135] & 0x07) / (2.0 * M_PI) * 0.18 /* -FIX- feedback_scaling[voice->algorithm] */;

    /* the "99.0" here is because we're also using this multiplier to scale the
     * eg level from 0-99 to 0-1 */
    voice->feedback_multiplier = DOUBLE_TO_FP(aux_feedbk / 99.0);

    voice->osc_key_sync = edit_buffer[136] & 0x01;

    voice->lfo_speed    = limit(edit_buffer[137], 0, 99);
    voice->lfo_delay    = limit(edit_buffer[138], 0, 99);
    voice->lfo_pmd      = limit(edit_buffer[139], 0, 99);
    voice->lfo_amd      = limit(edit_buffer[140], 0, 99);
    voice->lfo_key_sync = edit_buffer[141] & 0x01;
    voice->lfo_wave     = limit(edit_buffer[142], 0, 5);
    voice->lfo_pms      = (compat059 ? 0 : edit_buffer[143] & 0x07);

    voice->transpose = limit(edit_buffer[144], 0, 48);
}

