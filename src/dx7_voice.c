/* hexter DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
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
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

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
static inline void
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
        dx7_voice_update_pressure_mod(instance, voice);

    } else {

        /* synth is monophonic, and we're modifying a playing voice */
        DEBUG_MESSAGE(DB_NOTE, " dx7_voice_note_on in monophonic section: old key %d => new key %d\n", instance->held_keys[0], key);

        /* set new pitch */
        dx7_voice_recalculate_freq_and_inc(instance, voice);
        dx7_voice_update_pressure_mod(instance, voice);

        /* if in 'on' or 'both' modes, and key has changed, then re-trigger EGs */
        if ((instance->monophonic == DSSP_MONO_MODE_ON ||
             instance->monophonic == DSSP_MONO_MODE_BOTH) &&
            (instance->held_keys[0] < 0 || instance->held_keys[0] != key)) {
            dx7_voice_set_phase(instance, voice, 0);
        }

        /* all other variables stay what they are */
    }

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
    unsigned char previous_top_key;

    DEBUG_MESSAGE(DB_NOTE, " dx7_voice_note_off: called for voice %p, key %d\n", voice, key);

    /* save release velocity */
    voice->rvelocity = rvelocity;

    if (instance->monophonic) {

        /* remove this key from list of held keys */

        int i;

        /* check if this key is in list of held keys; if so, remove it and
         * shift the other keys up */
        // DEBUG_MESSAGE(DB_NOTE, " note-off key list before: %d %d %d %d %d %d %d %d\n", instance->held_keys[0], instance->held_keys[1], instance->held_keys[2], instance->held_keys[3], instance->held_keys[4], instance->held_keys[5], instance->held_keys[6], instance->held_keys[7]);
        previous_top_key = instance->held_keys[0];
        for (i = 7; i >= 0; i--) {
            if (instance->held_keys[i] == key)
                break;
        }
        if (i >= 0) {
            for (; i < 7; i++) {
                instance->held_keys[i] = instance->held_keys[i + 1];
            }
            instance->held_keys[7] = -1;
        }
        // DEBUG_MESSAGE(DB_NOTE, " note-off key list after: %d %d %d %d %d %d %d %d\n", instance->held_keys[0], instance->held_keys[1], instance->held_keys[2], instance->held_keys[3], instance->held_keys[4], instance->held_keys[5], instance->held_keys[6], instance->held_keys[7]);
    }

    if (instance->monophonic) {  /* monophonic mode */

        if (instance->held_keys[0] >= 0) {

            /* still some keys held */

            if (instance->held_keys[0] != previous_top_key) {

                /* most-recently-played key has changed */
                voice->key = instance->held_keys[0];
                DEBUG_MESSAGE(DB_NOTE, " note-off in monophonic section: changing pitch to %d\n", voice->key);
                dx7_voice_recalculate_freq_and_inc(instance, voice);
                dx7_voice_update_pressure_mod(instance, voice);

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
                /* !FIX! this doesn't make WATER GDN work? */
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

        int32_t precomp_duration = (INT_TO_FP(31) - eg->value + instance->dx7_eg_max_slew - 1) /
                                   instance->dx7_eg_max_slew;

        if (precomp_duration >= eg->duration) {

            eg->duration = precomp_duration;
            eg->increment = (eg->target - eg->value) / eg->duration;
            if (eg->increment > instance->dx7_eg_max_slew) {
                eg->duration = (eg->target - eg->value + instance->dx7_eg_max_slew - 1) /
                               instance->dx7_eg_max_slew;
                eg->increment = (eg->target - eg->value) / eg->duration;
            }
            eg->in_precomp = 0;

        } else if (precomp_duration < 1) {

            eg->increment = (eg->target - eg->value) / eg->duration;
            if (eg->increment > instance->dx7_eg_max_slew) {
                eg->duration = (eg->target - eg->value + instance->dx7_eg_max_slew - 1) /
                               instance->dx7_eg_max_slew;
                eg->increment = (eg->target - eg->value) / eg->duration;
            }
            eg->in_precomp = 0;

        } else {

            eg->postcomp_duration = eg->duration - precomp_duration;
            eg->duration = precomp_duration;
            eg->increment = (INT_TO_FP(31) - eg->value) / precomp_duration;
            eg->postcomp_increment = (eg->target - INT_TO_FP(31)) /
                                     eg->postcomp_duration;
            if (eg->postcomp_increment > instance->dx7_eg_max_slew) {
                eg->postcomp_duration = (eg->target - INT_TO_FP(31) + instance->dx7_eg_max_slew - 1) /
                                        instance->dx7_eg_max_slew;
                eg->postcomp_increment = (eg->target - INT_TO_FP(31)) /
                                         eg->postcomp_duration;
            }
            eg->in_precomp = 1;

        }

    } else {

        eg->increment = (eg->target - eg->value) / eg->duration;
        if (abs(eg->increment) > instance->dx7_eg_max_slew) {
            eg->duration = (abs(eg->target - eg->value) + instance->dx7_eg_max_slew - 1) /
                           instance->dx7_eg_max_slew;
            eg->increment = (eg->target - eg->value) / eg->duration;
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
        if (eg->duration == 1 && eg->increment == 0)
            dx7_op_eg_set_next_phase(instance, eg);
        break;

      case 2:
        eg->mode = DX7_EG_SUSTAINING;
        eg->increment = 0;
        eg->duration = -1;
        break;

      case 3:
      default: /* shouldn't be anything but 0 to 3 */
        eg->mode = DX7_EG_FINISHED;
        eg->increment = 0;
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
            eg->increment = 0;
            eg->duration = -1;

        } else {

            eg->mode = DX7_EG_RUNNING;
            dx7_op_eg_set_increment(instance, eg, eg->rate[phase], eg->level[phase]);
            if (eg->duration == 1 && eg->increment == 0)
                dx7_op_eg_set_next_phase(instance, eg);
        }
    } else {

        if (eg->mode != DX7_EG_CONSTANT) {

            eg->mode = DX7_EG_RUNNING;
            dx7_op_eg_set_increment(instance, eg, eg->rate[phase], eg->level[phase]);
            if (eg->duration == 1 && eg->increment == 0)
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
    /* !FIX! just a hunch: try it again with "* 6.0f" but also "(120.0f - 21.0f)" instead of "(126.0f - 21.0f)": */
    /* rate_bump = lrintf((float)op->rate_scaling * (float)(transposed_note - 21) / (120.0f - 21.0f) * 127.0f / 128.0f * 6.0f - 0.5f); */

    for (i=0;i<4;i++) {

        float level = (float)op->eg.base_level[i];

        /* !FIX! is this scaling of eg.base_level values to og.level values correct, i.e. does a softer
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

    /* !FIX! This is just a quick approximation that I derived from
     * regression of Godric Wilkie's pitch eg timings. In particular,
     * it's not accurate for very slow envelopes. */
    duration = exp(((double)new_rate - 70.337897) / -25.580953) *
               fabs((eg->target - eg->value) / 96.0);

    duration *= (double)instance->nugget_rate;

    eg->duration = lrint(duration);

    if (eg->duration > 1) {

        eg->increment = (eg->target - eg->value) / eg->duration;

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

void
dx7_eg_init_constants(hexter_instance_t *instance)
{
    float duration = dx7_voice_eg_rate_rise_duration[99] *
                     (dx7_voice_eg_rate_rise_percent[99] -
                      dx7_voice_eg_rate_rise_percent[0]);

    instance->dx7_eg_max_slew = lrintf((float)INT_TO_FP(99) /
                                       (duration * instance->sample_rate));

    instance->nugget_rate = instance->sample_rate / (float)HEXTER_NUGGET_SIZE;
}

/* ===== frequency related functions ===== */

static int
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

        /* !FIX! convert this to a table lookup for speed? */
        freq = instance->fixed_freq_multiplier *
                   exp(M_LN10 * ((double)(op->coarse & 3) + (double)op->fine / 100.0));
        /* !FIX! figure out what to do with detune */

    } else {
    
        freq = op->frequency;
        freq += ((double)op->detune - 7.0) / 32.0; /* !FIX! is this correct? */
        if (op->coarse) {
            freq = freq * (double)op->coarse;
        } else {
            freq = freq / 2.0;
        }
        freq *= (1.0 + ((double)op->fine / 100.0));

    }
    op->phase_increment = lrint(freq * (double)FP_SIZE / (double)instance->sample_rate);
}

double
dx7_voice_recalculate_frequency(hexter_instance_t *instance, dx7_voice_t *voice)
{
    double freq;

    voice->last_port_tuning = *instance->tuning;

    instance->fixed_freq_multiplier = *instance->tuning / 440.0;

    freq = voice->pitch_eg.value + instance->pitch_bend;

    voice->last_pitch = freq;

    freq += (double)(limit_note(voice->key + voice->transpose - 24));

    /* !FIX! this maybe could be optimized */
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
    voice->volume_target = FP_TO_FLOAT(dx7_voice_eg_ol_to_amp[i] +
                                       f * (dx7_voice_eg_ol_to_amp[i + 1] -
                                            dx7_voice_eg_ol_to_amp[i])) *
                           0.110384f / dx7_voice_carrier_count[voice->algorithm];

    if (voice->volume_value < 0.0f) { /* initial setup */
        voice->volume_value = voice->volume_target;
        voice->volume_count = 0;
    } else {
        voice->volume_count = (unsigned long)lrintf(instance->sample_rate / 20.0f);  /* 50ms ramp */
        voice->volume_delta = (voice->volume_target - voice->volume_value) /
                                  (float)voice->volume_count;
    }
}

/* ===== whole patch related functions ===== */

void
dx7_voice_calculate_runtime_parameters(hexter_instance_t *instance, dx7_voice_t* voice)
{
    int i;
    double freq;

    dx7_pitch_envelope_prepare(instance, voice);

    freq = dx7_voice_recalculate_frequency(instance, voice);

    voice->volume_value = -1.0f;  /* force initial setup */
    dx7_voice_recalculate_volume(instance, voice);

    for (i = 0; i < MAX_DX7_OPERATORS; i++) {
        voice->op[i].frequency = freq;
        if (voice->osc_key_sync) {
            voice->op[i].phase = 0;
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
    dx7_voice_calculate_runtime_parameters(instance, voice);
}

static int
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

    aux_feedbk = (double)(edit_buffer[135] & 0x07) / (2.0 * M_PI) * 0.18 /* !FIX! feedback_scaling[voice->algorithm] */;

    /* the "99.0" here is because we're also using this multiplier to scale the
     * eg level from 0-99 to 0-1 */
    voice->feedback_multiplier = lrintf(aux_feedbk / 99.0 * FP_SIZE);

    voice->osc_key_sync = edit_buffer[136] & 0x01;

    voice->transpose = limit(edit_buffer[144], 0, 48);

    /* !FIX! this doesn't get:
     * per operator: 14 AMS
     * 137-143 LFO parameters
     */
}

/*
 * dx7_voice_update_pressure_mod
 */
void
dx7_voice_update_pressure_mod(hexter_instance_t *instance, dx7_voice_t *voice)
{
    unsigned char kp = instance->key_pressure[voice->key];
    unsigned char cp = instance->channel_pressure;
    float p;

    /* add the channel and key pressures together in a way that 'feels' good */
    if (kp > cp) {
        p = (float)kp / 127.0f;
        p += (1.0f - p) * ((float)cp / 127.0f);
    } else {
        p = (float)cp / 127.0f;
        p += (1.0f - p) * ((float)kp / 127.0f);
    }
    /* !FIX! apply pressure modulation: */
    /* voice->pressure = f(p); */
}

/*
 * dx7_voice_update_pitch_bend
 */
void
dx7_voice_update_pitch_bend(hexter_instance_t *instance, dx7_voice_t *voice)
{
    /* nothing to do here, it's automatic */
}

