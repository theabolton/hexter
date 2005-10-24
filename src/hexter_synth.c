/* hexter DSSI software synthesizer plugin
 *
 * Copyright (C) 2004 Sean Bolton and others.
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
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

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
void
hexter_instance_key_pressure(hexter_instance_t *instance, unsigned char key,
                             unsigned char pressure)
{
    int i;
    dx7_voice_t* voice;

    /* save it for future voices */
    instance->key_pressure[key] = pressure;

    /* check if any playing voices need updating */
    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance && _PLAYING(voice) && voice->key == key) {
            dx7_voice_update_pressure_mod(instance, voice);
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
 * hexter_instance_update_wheel_mod
 */
void
hexter_instance_update_wheel_mod(hexter_instance_t* instance)
{
    /* instance->mod_wheel = (float)(instance->cc[MIDI_CTL_MSB_MODWHEEL] * 128 +
     *                               instance->cc[MIDI_CTL_LSB_MODWHEEL]) / 16256.0f;
     * if (instance->mod_wheel < 0.0f)
     *     instance->mod_wheel = 0.0f; -FIX- */
}

/*
 * hexter_instance_update_volume
 */
void
hexter_instance_update_volume(hexter_instance_t* instance)
{
    instance->cc_volume = instance->cc[MIDI_CTL_MSB_MAIN_VOLUME] * 128 +
                          instance->cc[MIDI_CTL_LSB_MAIN_VOLUME];
    if (instance->cc_volume > 16256)
        instance->cc_volume = 16256;
}

/*
 * hexter_instance_update_fc
 */
void
hexter_instance_update_fc(hexter_instance_t *instance, int opnum,
                          signed int value)
{
    int i;
    dx7_voice_t* voice;
    int fc = value / 4;  /* frequency coarse is 0 to 31 */

    /* update edit buffer */
    if (!pthread_mutex_trylock(&instance->patches_mutex)) {

        instance->current_patch_buffer[((5 - opnum) * 21) + 18] = fc;

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

            op->coarse = fc;
            dx7_op_recalculate_increment(instance, op);
        }
    }
}

/*
 * hexter_instance_control_change
 */
void
hexter_instance_control_change(hexter_instance_t *instance, unsigned int param,
                               signed int value)
{
    instance->cc[param] = value;

    switch (param) {

      case MIDI_CTL_MSB_MODWHEEL:
      case MIDI_CTL_LSB_MODWHEEL:
        hexter_instance_update_wheel_mod(instance);
        break;

      case MIDI_CTL_MSB_MAIN_VOLUME:
      case MIDI_CTL_LSB_MAIN_VOLUME:
        hexter_instance_update_volume(instance);
        break;

      case MIDI_CTL_SUSTAIN:
        DEBUG_MESSAGE(DB_NOTE, " hexter_instance_control_change: got sustain control of %d\n", value);
        if (value < 64)
            hexter_instance_damp_voices(instance);
        break;

      case MIDI_CTL_ALL_SOUNDS_OFF:
        hexter_instance_all_voices_off(instance);
        break;

      case MIDI_CTL_RESET_CONTROLLERS:
        hexter_instance_init_controls(instance);
        break;

      case MIDI_CTL_ALL_NOTES_OFF:
        hexter_instance_all_notes_off(instance);
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

      /* what others should we respond to? */
      /* -FIX- also want foot control, breath control */

      /* these we ignore (let the host handle):
       *  BANK_SELECT_MSB
       *  BANK_SELECT_LSB
       *  DATA_ENTRY_MSB
       *  NRPN_MSB
       *  NRPN_LSB
       *  RPN_MSB
       *  RPN_LSB
       * -FIX- no! we need RPN (0, 0) Pitch Bend Sensitivity!
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
    int i;
    dx7_voice_t* voice;

    /* save it for future voices */
    instance->channel_pressure = pressure;

    /* check if any playing voices need updating */
    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance && _PLAYING(voice)) {
            dx7_voice_update_pressure_mod(instance, voice);
        }
    }
}

/*
 * hexter_instance_pitch_bend
 */
void
hexter_instance_pitch_bend(hexter_instance_t *instance, signed int value)
{
    int i;
    dx7_voice_t* voice;

    instance->pitch_wheel = value; /* ALSA pitch bend is already -8192 - 8191 */
    instance->pitch_bend = (double)(value * instance->pitch_wheel_sensitivity)
                               / 8192.0;

    /* check if any playing voices need updating */
    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance && _PLAYING(voice)) {
            dx7_voice_update_pitch_bend(instance, voice);
        }
    }
}

/*
 * hexter_instance_init_controls
 */
void
hexter_instance_init_controls(hexter_instance_t *instance)
{
    int i;
    dx7_voice_t* voice;

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
    instance->pitch_wheel_sensitivity = 2;  /* two semi-tones */
    instance->pitch_wheel = 0;
    instance->pitch_bend = 0.0;
    instance->cc[MIDI_CTL_MSB_MAIN_VOLUME] = 127; /* full volume */

    hexter_instance_update_wheel_mod(instance);
    hexter_instance_update_volume(instance);

    /* check if any playing voices need updating */
    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
        if (voice->instance == instance && _PLAYING(voice)) {
            dx7_voice_update_pressure_mod(instance, voice);
            dx7_voice_update_pitch_bend(instance, voice);
        }
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

/*
 * hexter_instance_handle_monophonic
 */
char *
hexter_instance_handle_monophonic(hexter_instance_t *instance, const char *value)
{
    int mode = -1;
    int i;

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
            for (i = 0; i < 8; i++)
                instance->held_keys[i] = -1;

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
    unsigned long i;
    dx7_voice_t* voice;

    /* render each active voice */
    for (i = 0; i < hexter_synth.global_polyphony; i++) {
        voice = hexter_synth.voice[i];
    
        if (_PLAYING(voice)) {
            dx7_voice_render(voice->instance, voice,
                             voice->instance->output + samples_done,
                             sample_count, do_control_update);
        }
    }
}

