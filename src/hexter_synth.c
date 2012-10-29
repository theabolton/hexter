/* hexter DSSI software synthesizer plugin
 *
 * Copyright (C) 2004, 2009, 2011, 2012 Sean Bolton and others.
 *
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "hexter.h"
#include "hexter_types.h"
#include "hexter_synth.h"
#include "dx7_voice_data.h"
#include "dx7_voice.h"

extern hexter_synth_t hexter_synth;

/*
 * dx7_voice_off
 * 
 * turn off a voice immediately
 */
inline void
dx7_voice_off(dx7_voice_t* voice)
{
    voice->status = DX7_VOICE_OFF;
    if (voice->instance->monophonic)
        voice->instance->mono_voice = NULL;
    voice->instance->current_voices--;
}

/*
 * dx7_voice_start_voice
 */
inline void
dx7_voice_start_voice(dx7_voice_t *voice)
{
    voice->status = DX7_VOICE_ON;
    voice->instance->current_voices++;
}

/*
 * hexter_instance_clear_held_keys
 */
static inline void
hexter_instance_clear_held_keys(hexter_instance_t *instance)
{
    int i;

    for (i = 0; i < 8; i++)
        instance->held_keys[i] = -1;
}

/*
 * hexter_instance_remove_held_key
 */
static inline void
hexter_instance_remove_held_key(hexter_instance_t *instance, unsigned char key)
{
    int i;

    /* check if this key is in list of held keys; if so, remove it and
     * shift the other keys up */
    /* DEBUG_MESSAGE(DB_NOTE, " note-off key list before: %d %d %d %d %d %d %d %d\n", instance->held_keys[0], instance->held_keys[1], instance->held_keys[2], instance->held_keys[3], instance->held_keys[4], instance->held_keys[5], instance->held_keys[6], instance->held_keys[7]); */
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
    /* DEBUG_MESSAGE(DB_NOTE, " note-off key list after: %d %d %d %d %d %d %d %d\n", instance->held_keys[0], instance->held_keys[1], instance->held_keys[2], instance->held_keys[3], instance->held_keys[4], instance->held_keys[5], instance->held_keys[6], instance->held_keys[7]); */
}

/*
 * hexter_synth_all_voices_off
 *
 * stop processing all notes of all instances immediately
 */
void
hexter_synth_all_voices_off(void)
{
    int i;
    dx7_voice_t *voice;

    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (_PLAYING(voice)) {
            if (voice->instance->held_keys[0] != -1)
                hexter_instance_clear_held_keys(voice->instance);
            dx7_voice_off(voice);
        }
    }
}

/*
 * hexter_instance_all_voices_off
 *
 * stop processing all notes within instance immediately
 */
void
hexter_instance_all_voices_off(hexter_instance_t *instance)
{
    int i;
    dx7_voice_t *voice;

    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance && _PLAYING(voice)) {
            dx7_voice_off(voice);
        }
    }
    hexter_instance_clear_held_keys(instance);
}

/*
 * hexter_instance_note_off
 *
 * handle a note off message
 */
void
hexter_instance_note_off(hexter_instance_t *instance, unsigned char key,
                         unsigned char rvelocity)
{
    int i;
    dx7_voice_t *voice;

    hexter_instance_remove_held_key(instance, key);

    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance &&
            (instance->monophonic ? (_PLAYING(voice)) :
                                    (_ON(voice) && (voice->key == key)))) {
            DEBUG_MESSAGE(DB_NOTE, " hexter_instance_note_off: key %d rvel %d voice %d note id %d\n", key, rvelocity, i, voice->note_id);
            dx7_voice_note_off(instance, voice, key, rvelocity);
        } /* if voice on */
    } /* for all voices */
}

/*
 * hexter_instance_all_notes_off
 *
 * put all notes into the released state
 */
void
hexter_instance_all_notes_off(hexter_instance_t* instance)
{
    int i;
    dx7_voice_t *voice;

    /* reset the sustain controller */
    instance->cc[MIDI_CTL_SUSTAIN] = 0;
    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance &&
            (_ON(voice) || _SUSTAINED(voice))) {
            dx7_voice_release_note(instance, voice);
        }
    }
}

/*
 * hexter_synth_free_voice_by_kill
 *
 * selects a voice for killing. the selection algorithm is a refinement
 * of the algorithm previously in fluid_synth_alloc_voice.
 */
static dx7_voice_t*
hexter_synth_free_voice_by_kill(hexter_instance_t *instance)
{
    int i;
    int best_prio = 10001;
    int this_voice_prio;
    dx7_voice_t *voice;
    int best_voice_index = -1;

    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];

        if (instance) {
            /* only look at playing voices of this instance */
            if (_AVAILABLE(voice) || voice->instance != instance)
                continue;
        } else {
            /* safeguard against an available voice. */
            if (_AVAILABLE(voice))
                return voice;
        }

        /* Determine, how 'important' a voice is.
         * Start with an arbitrary number */
        this_voice_prio = 10000;

        if (_RELEASED(voice)) {
            /* This voice is in the release phase. Consider it much less
             * important than a voice which is still held. */
            this_voice_prio -= 2000;
        } else if (_SUSTAINED(voice)) {
            /* The sustain pedal is held down, and this voice is still "on"
             * because of this even though it has received a note off.
             * Consider it less important than voices which have not yet
             * received a note off. This decision is somewhat subjective, but
             * usually the sustain pedal is used to play 'more-voices-than-
             * fingers', and if so, it won't hurt as much to kill one of those
             * voices. */
             this_voice_prio -= 1000;
        };

        /* We are not enthusiastic about releasing voices, which have just been
         * started.  Otherwise hitting a chord may result in killing notes
         * belonging to that very same chord.  So subtract the age of the voice
         * from the priority - an older voice is just a little bit less
         * important than a younger voice. */
        this_voice_prio -= (hexter_synth.note_id - voice->note_id);
    
        /* -FIX- not yet implemented:
         * /= take a rough estimate of loudness into account. Louder voices are more important. =/
         * if (voice->volenv_section != FLUID_VOICE_ENVATTACK){
         *     this_voice_prio += voice->volenv_val*1000.;
         * };
         */

        /* check if this voice has less priority than the previous candidate. */
        if (this_voice_prio < best_prio)
            best_voice_index = i,
            best_prio = this_voice_prio;
    }

    if (best_voice_index < 0)
        return NULL;

    voice = hexter_synth.voice[best_voice_index];
    DEBUG_MESSAGE(DB_NOTE, " hexter_synth_free_voice_by_kill: no available voices, killing voice %d note id %d\n", best_voice_index, voice->note_id);
    dx7_voice_off(voice);
    return voice;
}

/*
 * hexter_synth_alloc_voice
 */
static dx7_voice_t *
hexter_synth_alloc_voice(hexter_instance_t* instance, unsigned char key)
{
    int i;
    dx7_voice_t* voice;

    /* If there is another voice on the same key, advance it
     * to the release phase. Note that a DX7 doesn't do this,
     * but we do it here to keep our CPU usage low. */
    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance && voice->key == key &&
            (_ON(voice) || _SUSTAINED(voice))) {
            dx7_voice_release_note(instance, voice);
        }
    }

    voice = NULL;

    if (instance->current_voices < instance->max_voices) {
        /* check if there's an available voice */
        for (i = 0; i < hexter_synth.global_polyphony; i++) {
            if (_AVAILABLE(hexter_synth.voice[i])) {
                voice = hexter_synth.voice[i];
                break;
            }
        }
    
        /* if not, then stop a running voice. */
        if (voice == NULL) {
            voice = hexter_synth_free_voice_by_kill(NULL);
        }
    } else {  /* at instance polyphony limit */
        voice = hexter_synth_free_voice_by_kill(instance);
    }

    if (voice == NULL) {
        DEBUG_MESSAGE(DB_NOTE, " hexter_synth_alloc_voice: failed to allocate a voice (key=%d)\n", key);
        return NULL;
    }

    DEBUG_MESSAGE(DB_NOTE, " hexter_synth_alloc_voice: key %d voice %p\n", key, voice);
    return voice;
}

/*
 * hexter_instance_note_on
 */
void
hexter_instance_note_on(hexter_instance_t *instance, unsigned char key,
                        unsigned char velocity)
{
    dx7_voice_t* voice;

    if (key > 127 || velocity > 127)
        return;  /* MidiKeys 1.6b3 sends bad notes.... */

    if (instance->monophonic) {

        if (instance->mono_voice) {
            voice = instance->mono_voice;
            DEBUG_MESSAGE(DB_NOTE, " hexter_instance_note_on: retriggering mono voice on new key %d\n", key);
        } else {
            voice = hexter_synth_alloc_voice(instance, key);
            if (voice == NULL)
                return;
            instance->mono_voice = voice;
        }

    } else { /* polyphonic mode */

        voice = hexter_synth_alloc_voice(instance, key);
        if (voice == NULL)
            return;

    }

    voice->instance = instance;
    voice->note_id = hexter_synth.note_id++;

    dx7_voice_note_on(instance, voice, key, velocity);
}

/*
 * hexter_instance_key_pressure
 */
inline void
hexter_instance_key_pressure(hexter_instance_t *instance, unsigned char key,
                             unsigned char pressure)
{
    int i;
    dx7_voice_t* voice;

    if (instance->key_pressure[key] == pressure)
        return;

    /* save it for future voices */
    instance->key_pressure[key] = pressure;

    /* flag any playing voices as needing updating */
    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance && _PLAYING(voice) && voice->key == key) {
            voice->mods_serial--;
        }
    }
}

/*
 * hexter_instance_damp_voices
 *
 * advance all sustained voices to the release phase (note that this does not
 * clear the sustain controller.)
 */
void
hexter_instance_damp_voices(hexter_instance_t* instance)
{
    int i;
    dx7_voice_t* voice;

    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance && _SUSTAINED(voice)) {
            /* this assumes the caller has cleared the sustain controller */
            dx7_voice_release_note(instance, voice);
        }
    }
}

/*
 * hexter_instance_update_mod_wheel
 */
static inline void
hexter_instance_update_mod_wheel(hexter_instance_t* instance)
{
    int mod = instance->cc[MIDI_CTL_MSB_MODWHEEL] * 128 +
              instance->cc[MIDI_CTL_LSB_MODWHEEL];

    if (mod > 16256) mod = 16256;
    instance->mod_wheel = (float)mod / 16256.0f;
    instance->mods_serial++;

#ifdef HEXTER_DEBUG_CONTROL
    instance->feedback_mod = instance->cc[MIDI_CTL_MSB_MODWHEEL];
    printf("new mod wheel value %d\n", instance->feedback_mod);
#endif
}

/*
 * hexter_instance_update_breath
 */
static inline void
hexter_instance_update_breath(hexter_instance_t* instance)
{
    int mod = instance->cc[MIDI_CTL_MSB_BREATH] * 128 +
              instance->cc[MIDI_CTL_LSB_BREATH];

    if (mod > 16256) mod = 16256;
    instance->breath = (float)mod / 16256.0f;
    instance->mods_serial++;
}

/*
 * hexter_instance_update_foot
 */
static inline void
hexter_instance_update_foot(hexter_instance_t* instance)
{
    int mod = instance->cc[MIDI_CTL_MSB_FOOT] * 128 +
              instance->cc[MIDI_CTL_LSB_FOOT];

    if (mod > 16256) mod = 16256;
    instance->foot = (float)mod / 16256.0f;
    instance->mods_serial++;
}

/*
 * hexter_instance_update_volume
 */
static inline void
hexter_instance_update_volume(hexter_instance_t* instance)
{
    instance->cc_volume = instance->cc[MIDI_CTL_MSB_MAIN_VOLUME] * 128 +
                          instance->cc[MIDI_CTL_LSB_MAIN_VOLUME];
    if (instance->cc_volume > 16256)
        instance->cc_volume = 16256;
}

/*
 * hexter_instance_update_op_param
 *
 * Generic function to update operator parameters
 *
 */
static void
hexter_instance_update_op_param(hexter_instance_t *instance, int opnum,
                                int param, signed int value)
{
    int i;
    dx7_voice_t* voice;

    /* scale the value */
    switch(param){
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 16:
        case 19:
            value = value * 100 / 16384;  /* 0 to 99 */
            break;
        case 11:
        case 12:
        case 14:
            value = value * 4 / 16384;  /* 0 to 3 */
            break;
        case 13:
        case 15:
            value = value * 8 / 16384;  /* 0 to 7 */
            break;
        case 17:
            value = value * 2 / 16384;  /* 0 or 1 */
            break;
        case 18:
            value = value * 32 / 16384;  /* 0 to 31 */
            break;
        case 20:
            value = value * 15 / 16384;  /* 0 to 14 */
            break;
    }

    /* update edit buffer */
    if (!pthread_mutex_trylock(&instance->patches_mutex)) {

        instance->current_patch_buffer[((5 - opnum) * 21) + param] =
            value;

        pthread_mutex_unlock(&instance->patches_mutex);
    } else {
        /* In the unlikely event that we get here, it means another thread is
         * currently updating the current patch buffer. We could do something
         * like the 'pending_program_change' mechanism to cache this change
         * until we can lock the mutex, if it's really important. */
    }

    /* check if any playing voices need updating */
    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance && _PLAYING(voice)) {
            dx7_op_t *op = &voice->op[opnum];

            /* set values */
            switch (param) {
                case 0:
                    op->eg.base_rate[0] = value;
                    break;
                case 1:
                    op->eg.base_rate[1] = value;
                    break;
                case 2:
                    op->eg.base_rate[2] = value;
                    break;
                case 3:
                    op->eg.base_rate[3] = value;
                    break;
                case 4:
                    op->eg.base_level[0] = value;
                    break;
                case 5:
                    op->eg.base_level[1] = value;
                    break;
                case 6:
                    op->eg.base_level[2] = value;
                    break;
                case 7:
                    op->eg.base_level[3] = value;
                    break;
                case 8:
                    op->level_scaling_bkpoint = value;
                    break;
                case 9:
                    op->level_scaling_l_depth = value;
                    break;
                case 10:
                    op->level_scaling_r_depth = value;
                    break;
                case 11:
                    op->level_scaling_l_curve = value;
                    break;
                case 12:
                    op->level_scaling_r_curve = value;
                    break;
                case 13:
                    op->rate_scaling = value;
                    break;
                case 14:
                    op->amp_mod_sens = value;
                    break;
                case 15:
                    op->velocity_sens = value;
                    break;
                case 16:
                    op->output_level = value;
                    break;
                case 17:
                    op->osc_mode = value;
                    break;
                case 18:
                    op->coarse = value;
                    break;
                case 19:
                    op->fine = value;
                    break;
                case 20:
                    op->detune = value;
                    break;
            }

            /* do recalculations */
            switch (param) {
                case 17:    /* osc mode */
                case 18:    /* coarse */
                case 19:    /* fine */
                case 20:    /* detune */
                    dx7_op_recalculate_increment(instance, op);
                    break;
                /* which other operator params need a recalc ?? */
            }
        }
    }
}

/*
 * hexter_instance_update_fc
 */
static inline void
hexter_instance_update_fc(hexter_instance_t *instance, int opnum,
                          signed int value)
{
    hexter_instance_update_op_param(instance, opnum, 18, value * 128);
}

/*
 * hexter_instance_handle_nrpn
 *
 * Update operator parameters via NRPN 0-125.
 */
static void
hexter_instance_handle_nrpn(hexter_instance_t *instance)
{
    int nrpn = instance->cc[MIDI_CTL_NONREG_PARM_NUM_MSB] * 128 +
               instance->cc[MIDI_CTL_NONREG_PARM_NUM_LSB];
    int value = instance->cc[MIDI_CTL_MSB_DATA_ENTRY] * 128 +
                instance->cc[MIDI_CTL_LSB_DATA_ENTRY];

    int opnum;
    int op_param;

    if (nrpn >= 126) return; /* for now we only support operator params */

    opnum    = nrpn / 21;   /* 0 = OP6, 5 = OP1 */
    op_param = nrpn - (21 * opnum);

    hexter_instance_update_op_param(instance, 5 - opnum, op_param, value);
}

#ifdef HEXTER_DEBUG_CONTROL
void dx7_lfo_set_speed_x(hexter_instance_t *instance);  /* prototype for test code below */
#endif

/*
 * hexter_instance_control_change
 */
void
hexter_instance_control_change(hexter_instance_t *instance, unsigned int param,
                               signed int value)
{
    switch (param) {  /* these controls we act on always */

      case MIDI_CTL_SUSTAIN:
        DEBUG_MESSAGE(DB_NOTE, " hexter_instance_control_change: got sustain control of %d\n", value);
        instance->cc[param] = value;
        if (value < 64)
            hexter_instance_damp_voices(instance);
        return;

      case MIDI_CTL_ALL_SOUNDS_OFF:
        instance->cc[param] = value;
        hexter_instance_all_voices_off(instance);
        return;

      case MIDI_CTL_RESET_CONTROLLERS:
        instance->cc[param] = value;
        hexter_instance_init_controls(instance);
        return;

      case MIDI_CTL_ALL_NOTES_OFF:
        instance->cc[param] = value;
        hexter_instance_all_notes_off(instance);
        return;
    }

    if (param == MIDI_CTL_REGIST_PARM_NUM_LSB ||
        param == MIDI_CTL_REGIST_PARM_NUM_MSB) {

        /* reset NRPN numbers on receipt of RPN */
        instance->cc[MIDI_CTL_NONREG_PARM_NUM_LSB] = 127;
        instance->cc[MIDI_CTL_NONREG_PARM_NUM_MSB] = 127;
    }

    if (instance->cc[param] == value)  /* do nothing if control value has not changed */
        return;

    instance->cc[param] = value;

    switch (param) {

#ifdef HEXTER_DEBUG_CONTROL
      case MIDI_CTL_MSB_PAN: /* panning */
        // hexter_instance_channel_pressure(instance, value);
        // { float f;
        //     f = 52.75f / (instance->sample_rate * 0.001f * (float)value);
        //     instance->amp_mod_max_slew = FLOAT_TO_FP(f);
        //     printf("new amp_mod_max_slew, %dms => %f = %d\n", value, f, instance->amp_mod_max_slew);
        // }
        {
            if (value == 0)
                instance->ramp_duration = 1;
            else
                instance->ramp_duration = (int)(instance->sample_rate * 0.001f * (float)value);  /* value ms ramp */
            printf("new ramp_duration, %dms => %d frames\n", value, instance->ramp_duration);
            dx7_lfo_set_speed_x(instance);
        }
        break;

      case MIDI_CTL_MSB_EXPRESSION: /* 'expression' */
        hexter_instance_key_pressure(instance, 60, value);
        break;
#endif /* HEXTER_DEBUG_CONTROL */

      case MIDI_CTL_MSB_MODWHEEL:
      case MIDI_CTL_LSB_MODWHEEL:
        hexter_instance_update_mod_wheel(instance);
        break;

      case MIDI_CTL_MSB_BREATH:
      case MIDI_CTL_LSB_BREATH:
        hexter_instance_update_breath(instance);
        break;

      case MIDI_CTL_MSB_FOOT:
      case MIDI_CTL_LSB_FOOT:
        hexter_instance_update_foot(instance);
        break;

      case MIDI_CTL_MSB_MAIN_VOLUME:
      case MIDI_CTL_LSB_MAIN_VOLUME:
        hexter_instance_update_volume(instance);
        break;

      case MIDI_CTL_MSB_GENERAL_PURPOSE1:
      case MIDI_CTL_MSB_GENERAL_PURPOSE2:
      case MIDI_CTL_MSB_GENERAL_PURPOSE3:
      case MIDI_CTL_MSB_GENERAL_PURPOSE4:
        hexter_instance_update_fc(instance, param - MIDI_CTL_MSB_GENERAL_PURPOSE1,
                                  value);
        break;

      case MIDI_CTL_GENERAL_PURPOSE5:
      case MIDI_CTL_GENERAL_PURPOSE6:
        hexter_instance_update_fc(instance, param - MIDI_CTL_GENERAL_PURPOSE5 + 4,
                                  value);
        break;

      /* handle NRPN as real-time parameter change */
      case MIDI_CTL_MSB_DATA_ENTRY:
      case MIDI_CTL_LSB_DATA_ENTRY:
        if (instance->cc[MIDI_CTL_NONREG_PARM_NUM_MSB] != 127 &&
            instance->cc[MIDI_CTL_NONREG_PARM_NUM_LSB] != 127) {
            hexter_instance_handle_nrpn(instance);
        }
        break;

      /* what others should we respond to? */

      /* these we ignore (let the host handle):
       *  BANK_SELECT_MSB
       *  BANK_SELECT_LSB
       *  RPN_MSB
       *  RPN_LSB
       * (may want to eventually implement RPN (0, 0) Pitch Bend Sensitivity)
       */
    }
}

/*
 * hexter_instance_channel_pressure
 */
void
hexter_instance_channel_pressure(hexter_instance_t *instance,
                                 signed int pressure)
{
    if (instance->channel_pressure == pressure)
        return;

    instance->channel_pressure = pressure;
    instance->mods_serial++;
}

/*
 * hexter_instance_pitch_bend
 */
void
hexter_instance_pitch_bend(hexter_instance_t *instance, signed int value)
{
    instance->pitch_wheel = value; /* ALSA pitch bend is already -8192 - 8191 */
    instance->pitch_bend = (double)(value * instance->pitch_bend_range)
                               / 8192.0;
}

/*
 * hexter_instance_init_controls
 */
void
hexter_instance_init_controls(hexter_instance_t *instance)
{
    int i;

    /* if sustain was on, we need to damp any sustained voices */
    if (HEXTER_INSTANCE_SUSTAINED(instance)) {
        instance->cc[MIDI_CTL_SUSTAIN] = 0;
        hexter_instance_damp_voices(instance);
    }

    for (i = 0; i < 128; i++) {
        instance->key_pressure[i] = 0;
        instance->cc[i] = 0;
    }
    instance->channel_pressure = 0;
    instance->pitch_wheel = 0;
    instance->pitch_bend = 0.0;
    instance->cc[MIDI_CTL_MSB_MAIN_VOLUME] = 127; /* full volume */
    instance->cc[MIDI_CTL_NONREG_PARM_NUM_LSB] = 127; /* 'null' */
    instance->cc[MIDI_CTL_NONREG_PARM_NUM_MSB] = 127; /* 'null' */

    hexter_instance_update_mod_wheel(instance);
    hexter_instance_update_breath(instance);
    hexter_instance_update_foot(instance);
    hexter_instance_update_volume(instance);

    instance->mods_serial++;
}

static inline int
limit(int x, int min, int max)
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

/*
 * hexter_instance_set_performance_data
 */
void
hexter_instance_set_performance_data(hexter_instance_t *instance)
{
    uint8_t *perf_buffer = instance->performance_buffer;

    /* set instance performance parameters */
    /* -FIX- later these will optionally come from patch */
    instance->pitch_bend_range      = limit(perf_buffer[3], 0, 12);
    instance->portamento_time       = limit(perf_buffer[5], 0, 99);
    instance->mod_wheel_sensitivity = limit(perf_buffer[9], 0, 15);
    instance->mod_wheel_assign      = limit(perf_buffer[10], 0, 7);
    instance->foot_sensitivity      = limit(perf_buffer[11], 0, 15);
    instance->foot_assign           = limit(perf_buffer[12], 0, 7);
    instance->pressure_sensitivity  = limit(perf_buffer[13], 0, 15);
    instance->pressure_assign       = limit(perf_buffer[14], 0, 7);
    instance->breath_sensitivity    = limit(perf_buffer[15], 0, 15);
    instance->breath_assign         = limit(perf_buffer[16], 0, 7);
    if (perf_buffer[0] & 0x01) { /* 0.5.9 compatibility */
        instance->pitch_bend_range      = 2;
        instance->portamento_time       = 0;
        instance->mod_wheel_sensitivity = 0;
        instance->foot_sensitivity      = 0;
        instance->pressure_sensitivity  = 0;
        instance->breath_sensitivity    = 0;
    }
}

/*
 * hexter_instance_select_program
 */
void
hexter_instance_select_program(hexter_instance_t *instance, unsigned long bank,
                               unsigned long program)
{
    /* no support for banks, so we just ignore the bank number */
    if (program >= 128) return;
    instance->current_program = program;
    if (instance->overlay_program == program) { /* edit buffer applies */
        memcpy(instance->current_patch_buffer, instance->overlay_patch_buffer, DX7_VOICE_SIZE_UNPACKED);
    } else {
        dx7_patch_unpack(instance->patches, program, instance->current_patch_buffer);
    }
}

/*
 * hexter_instance_set_program_descriptor
 */
int
hexter_instance_set_program_descriptor(hexter_instance_t *instance,
                                       DSSI_Program_Descriptor *pd,
                                       unsigned long bank,
                                       unsigned long program)
{
    static char name[11];

    /* no support for banks, so we just ignore the bank number */
    if (program >= 128) {
        return 0;
    }
    pd->Bank = bank;
    pd->Program = program;
    /* -FIX- some character set conversion would be appropriate here, but to what? */
    dx7_voice_copy_name(name, &instance->patches[program]);
    pd->Name = name;
    return 1;
}

/*
 * hexter_instance_handle_patches
 */
char *
hexter_instance_handle_patches(hexter_instance_t *instance, const char *key,
                               const char *value)
{
    int section;

    DEBUG_MESSAGE(DB_DATA, " hexter_instance_handle_patches: received new '%s'\n", key);

    section = key[7] - '0';
    if (section < 0 || section > 3)
        return dssp_error_message("patch configuration failed: invalid section '%c'", key[7]);

    pthread_mutex_lock(&instance->patches_mutex);

    if (!decode_7in6(value, 32 * sizeof(dx7_patch_t),
                     (uint8_t *)&instance->patches[section * 32])) {
        pthread_mutex_unlock(&instance->patches_mutex);
        return dssp_error_message("patch configuration failed: corrupt data");
    }

    if ((instance->current_program / 32) == section &&
        instance->current_program != instance->overlay_program) 
        dx7_patch_unpack(instance->patches, instance->current_program,
                         instance->current_patch_buffer);

    pthread_mutex_unlock(&instance->patches_mutex);

    return NULL; /* success */
}

/*
 * hexter_instance_handle_edit_buffer
 */
char *
hexter_instance_handle_edit_buffer(hexter_instance_t *instance,
                                   const char *value)
{
    struct {
        int     program;
        uint8_t buffer[DX7_VOICE_SIZE_UNPACKED];
    } edit_buffer;

    pthread_mutex_lock(&instance->patches_mutex);

    if (!strcmp(value, "off")) {

        DEBUG_MESSAGE(DB_DATA, " hexter_instance_handle_edit_buffer: cancelled\n");
        if (instance->current_program == instance->overlay_program) {
            dx7_patch_unpack(instance->patches, instance->current_program, instance->current_patch_buffer);
        }
        instance->overlay_program = -1;

    } else {

        DEBUG_MESSAGE(DB_DATA, " hexter_instance_handle_edit_buffer: received new overlay\n");

        if (!decode_7in6(value, sizeof(edit_buffer), (uint8_t *)&edit_buffer)) {
            pthread_mutex_unlock(&instance->patches_mutex);
            return dssp_error_message("patch edit failed: corrupt data");
        }

        instance->overlay_program = edit_buffer.program;
        memcpy(instance->overlay_patch_buffer, edit_buffer.buffer, DX7_VOICE_SIZE_UNPACKED);
        if (instance->current_program == instance->overlay_program) { /* applies to current patch also */
            memcpy(instance->current_patch_buffer, instance->overlay_patch_buffer, DX7_VOICE_SIZE_UNPACKED);
        }
    }

    pthread_mutex_unlock(&instance->patches_mutex);

    return NULL; /* success */
}

char *
hexter_instance_handle_performance(hexter_instance_t *instance,
                                   const char *value)
{
    pthread_mutex_lock(&instance->patches_mutex);

    DEBUG_MESSAGE(DB_DATA, " hexter_instance_handle_performance: received new global performance parameters\n");

    if (!decode_7in6(value, DX7_PERFORMANCE_SIZE, instance->performance_buffer)) {
        pthread_mutex_unlock(&instance->patches_mutex);
        return dssp_error_message("performance edit failed: corrupt data");
    }

    hexter_instance_set_performance_data(instance);

    pthread_mutex_unlock(&instance->patches_mutex);

    /* we eventually may want to update playing voices here */

    return NULL; /* success */
}

/*
 * hexter_instance_handle_monophonic
 */
char *
hexter_instance_handle_monophonic(hexter_instance_t *instance, const char *value)
{
    int mode = -1;

    if (!strcmp(value, "on")) mode = DSSP_MONO_MODE_ON;
    else if (!strcmp(value, "once")) mode = DSSP_MONO_MODE_ONCE;
    else if (!strcmp(value, "both")) mode = DSSP_MONO_MODE_BOTH;
    else if (!strcmp(value, "off"))  mode = DSSP_MONO_MODE_OFF;

    if (mode == -1) {
        return dssp_error_message("error: monophonic value not recognized");
    }

    if (mode == DSSP_MONO_MODE_OFF) {  /* polyphonic mode */

        instance->monophonic = 0;
        instance->max_voices = instance->polyphony;

    } else {  /* one of the monophonic modes */

        if (!instance->monophonic) {

            dssp_voicelist_mutex_lock();

            hexter_instance_all_voices_off(instance);
            instance->max_voices = 1;
            instance->mono_voice = NULL;
            hexter_instance_clear_held_keys(instance);
            dssp_voicelist_mutex_unlock();
        }
        instance->monophonic = mode;
    }

    return NULL; /* success */
}

/*
 * hexter_instance_handle_polyphony
 */
char *
hexter_instance_handle_polyphony(hexter_instance_t *instance, const char *value)
{
    int polyphony = atoi(value);
    int i;
    dx7_voice_t *voice;

    if (polyphony < 1 || polyphony > HEXTER_MAX_POLYPHONY) {
        return dssp_error_message("error: polyphony value out of range");
    }
    /* set the new limit */
    instance->polyphony = polyphony;

    if (!instance->monophonic) {

        dssp_voicelist_mutex_lock();

        instance->max_voices = polyphony;

        /* turn off any voices above the new limit */
        for (i = 0; instance->current_voices > instance->max_voices &&
                    i < hexter_synth.global_polyphony; i++) {
            voice = hexter_synth.voice[i];
            if (voice->instance == instance && _PLAYING(voice)) {
                if (voice->instance->held_keys[0] != -1)
                    hexter_instance_clear_held_keys(voice->instance);
                dx7_voice_off(voice);
            }
        }

        dssp_voicelist_mutex_unlock();
    }

    return NULL; /* success */
}

/*
 * hexter_synth_handle_global_polyphony
 */
char *
hexter_synth_handle_global_polyphony(const char *value)
{
    int polyphony = atoi(value);
    int i;
    dx7_voice_t *voice;

    if (polyphony < 1 || polyphony > HEXTER_MAX_POLYPHONY) {
        return dssp_error_message("error: polyphony value out of range");
    }

    dssp_voicelist_mutex_lock();

    /* set the new limit */
    hexter_synth.global_polyphony = polyphony;

    /* turn off any voices above the new limit */
    for (i = polyphony; i < HEXTER_MAX_POLYPHONY; i++) {
        voice = hexter_synth.voice[i];
        if (_PLAYING(voice)) {
            if (voice->instance->held_keys[0] != -1)
                hexter_instance_clear_held_keys(voice->instance);
            dx7_voice_off(voice);
        }
    }

    dssp_voicelist_mutex_unlock();

    return NULL; /* success */
}

/*
 * hexter_synth_render_voices
 */
void
hexter_synth_render_voices(unsigned long samples_done,
                           unsigned long sample_count, int do_control_update)
{
    hexter_instance_t *instance;
    unsigned long i;
    dx7_voice_t* voice;

    /* update each LFO */
    for (instance = hexter_synth.instances; instance;
             instance = instance->next) {
        dx7_lfo_update(instance, sample_count);
    }

    /* render each active voice */
    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
    
        if (_PLAYING(voice)) {
            if (voice->mods_serial != voice->instance->mods_serial) {
                dx7_voice_update_mod_depths(voice->instance, voice);
                voice->mods_serial = voice->instance->mods_serial;
            }
            dx7_voice_render(voice->instance, voice,
                             voice->instance->output + samples_done,
                             sample_count, do_control_update);
        }
    }
}

