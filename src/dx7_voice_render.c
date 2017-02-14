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

#include <math.h>

#include "hexter_types.h"
#include "hexter.h"
#include "hexter_synth.h"
#include "dx7_voice.h"

static inline dx7_sample_t
dx7_op_calculate_operator(dx7_sample_t eg_value, dx7_sample_t phase)
{
    int32_t index;
    dx7_sample_t mod_index, out;
#ifdef HEXTER_USE_FLOATING_POINT
    float frac;
#endif /* HEXTER_USE_FLOATING_POINT */

    /* use eg_value to look up the modulation index, with interpolation */
#ifndef HEXTER_USE_FLOATING_POINT
    index = FP_TO_INT(eg_value);
    mod_index = dx7_voice_eg_ol_to_mod_index[index];
    mod_index += FP_MULTIPLY(dx7_voice_eg_ol_to_mod_index[index + 1] - mod_index,
                             eg_value & FP_MASK);
#else /* HEXTER_USE_FLOATING_POINT */
    index = lrintf(eg_value - 0.5f);
    frac = eg_value - (float)index;
    mod_index = dx7_voice_eg_ol_to_mod_index[index];
    mod_index += (dx7_voice_eg_ol_to_mod_index[index + 1] - mod_index) * frac;
#endif /* HEXTER_USE_FLOATING_POINT */

    /* use phase to look up the oscillator output, with interpolation */
#ifndef HEXTER_USE_FLOATING_POINT
    index = ((uint32_t)phase >> FP_TO_SINE_SHIFT) & SINE_MASK;
    out = dx7_voice_sin_table[index];
    out += (((int64_t)(dx7_voice_sin_table[index + 1] - out) *
             (int64_t)(phase & FP_TO_SINE_MASK)) >>
            (FP_SHIFT + FP_TO_SINE_SHIFT));
#else /* HEXTER_USE_FLOATING_POINT */
    phase *= (float)SINE_SIZE;
    index = lrintf(phase - 0.5f);
    frac = phase - (float)index;
    index &= SINE_MASK;
    out = dx7_voice_sin_table[index];
    out += (dx7_voice_sin_table[index + 1] - out) * frac;
#endif /* HEXTER_USE_FLOATING_POINT */

    /* return the product of modulation index and oscillator output */
    return FP_MULTIPLY(mod_index, out);
}

static inline dx7_sample_t
dx7_op_calculate_operator_saving_feedback(dx7_voice_t *voice, dx7_sample_t eg_value,
                                          dx7_sample_t phase)
{
    int32_t index;
    dx7_sample_t mod_index, out;
#ifndef HEXTER_USE_FLOATING_POINT
    int64_t out64;
#else /* HEXTER_USE_FLOATING_POINT */
    float frac;
#endif /* HEXTER_USE_FLOATING_POINT */

    /* use eg_value to look up the modulation index, with interpolation */
#ifndef HEXTER_USE_FLOATING_POINT
    index = FP_TO_INT(eg_value);
    mod_index = dx7_voice_eg_ol_to_mod_index[index];
    mod_index += FP_MULTIPLY(dx7_voice_eg_ol_to_mod_index[index + 1] - mod_index,
                             eg_value & FP_MASK);
#else /* HEXTER_USE_FLOATING_POINT */
    index = lrintf(eg_value - 0.5f);
    frac = eg_value - (float)index;
    mod_index = dx7_voice_eg_ol_to_mod_index[index];
    mod_index += (dx7_voice_eg_ol_to_mod_index[index + 1] - mod_index) * frac;
#endif /* HEXTER_USE_FLOATING_POINT */

    /* use phase to look up the oscillator output, with interpolation */
#ifndef HEXTER_USE_FLOATING_POINT
    index = ((uint32_t)phase >> FP_TO_SINE_SHIFT) & SINE_MASK;
    out = dx7_voice_sin_table[index];
    out64 = out +
            (((int64_t)(dx7_voice_sin_table[index + 1] - out) *
              (int64_t)(phase & FP_TO_SINE_MASK)) >>
             (FP_SHIFT + FP_TO_SINE_SHIFT));
#else /* HEXTER_USE_FLOATING_POINT */
    phase *= (float)SINE_SIZE;
    index = lrintf(phase - 0.5f);
    frac = phase - (float)index;
    index &= SINE_MASK;
    out = dx7_voice_sin_table[index];
    out += (dx7_voice_sin_table[index + 1] - out) * frac;
#endif /* HEXTER_USE_FLOATING_POINT */

    /* save that output, scaled by our eg level, feedback amount, and a
     * constant, as our feedback modulation index */
#ifndef HEXTER_USE_FLOATING_POINT
    voice->feedback = (((out64 * (int64_t)eg_value) >> FP_SHIFT) *
                       (int64_t)voice->feedback_multiplier) >> FP_SHIFT;
#else /* HEXTER_USE_FLOATING_POINT */
    voice->feedback = out * eg_value * voice->feedback_multiplier;
#endif /* HEXTER_USE_FLOATING_POINT */

    /* return the product of modulation index and oscillator output */
#ifndef HEXTER_USE_FLOATING_POINT
    return (int32_t)(((int64_t)mod_index * out64) >> FP_SHIFT);
#else /* HEXTER_USE_FLOATING_POINT */
    return mod_index * out;
#endif /* HEXTER_USE_FLOATING_POINT */
}

static inline void
dx7_op_eg_process(hexter_instance_t *instance, dx7_op_eg_t *eg)
{
    eg->value += eg->increment;

    if (--eg->duration == 0) {

        if (eg->mode != DX7_EG_RUNNING) {
            eg->duration = -1;
            return;
        }

        if (eg->in_precomp) {

            eg->in_precomp = 0;
            eg->duration = eg->postcomp_duration;
            eg->increment = eg->postcomp_increment;

        } else {

            dx7_op_eg_set_next_phase(instance, eg);
        }
    }
}

static inline void
dx7_op_eg_adjust(dx7_op_eg_t *eg)
{
    /* The constant in this next expression needs to be greater than
     * 0.000815 * (32/99) * sample_rate to avoid interaction with envelope
     * precompensation.  60 is safe to 192KHz. */
    if (eg->duration > 60) {

        if (eg->mode != DX7_EG_RUNNING)
            return;

        eg->increment = (eg->target - eg->value) / eg->duration;
    }
}

static inline void
dx7_pitch_eg_process(hexter_instance_t *instance, dx7_pitch_eg_t *eg)
{
    if (eg->mode != DX7_EG_RUNNING) return;

    eg->value += eg->increment;
    eg->duration--;

    if (eg->duration == 1) {

        eg->increment = eg->target - eg->value;  /* correct any rounding error */

    } else if (eg->duration == 0) {

        dx7_pitch_eg_set_next_phase(instance, eg);

    }
}

static inline void
dx7_portamento_process(hexter_instance_t *instance, dx7_portamento_t *port)
{
    if (port->segment == 0) return;

    port->value += port->increment;
    port->duration--;

    if (port->duration == 1) {

        port->increment = port->target - port->value;  /* correct any rounding error */

    } else if (port->duration == 0) {

        if (--port->segment > 0)
            dx7_portamento_set_segment(instance, port);
        else
            port->value = 0.0;

    }
}

static inline int
dx7_voice_check_for_dead(dx7_voice_t *voice)
{
    int i, b;

    for (i = 0, b = 1; i < 6; i++, b <<= 1) {

        if (!(dx7_voice_carriers[voice->algorithm] & b))
            continue;  /* not a carrier, so still a candidate for killing; continue to check next op */

        if (voice->op[i].eg.mode == DX7_EG_FINISHED)
            continue;  /* carrier, eg finished, so still a candidate */

        if ((voice->op[i].eg.mode == DX7_EG_CONSTANT ||
             voice->op[i].eg.mode == DX7_EG_SUSTAINING ||
             (voice->op[i].eg.mode == DX7_EG_RUNNING && voice->op[i].eg.phase == 3)) &&
            (FP_TO_INT(voice->op[i].eg.value) == 0))
            continue;  /* eg constant at 0 or decayed to effectively 0, still a candidate */

        return 0; /* if we got this far, this carrier still has output, so return without killing voice */
    }

    DEBUG_MESSAGE(DB_NOTE, " dx7_voice_check_for_dead: killing voice %p:%d\n", voice, voice->note_id);
    dx7_voice_off(voice);
    return 1;
}

static inline int
float_equality(float a, float b)
{
    union { float f; unsigned long l; } ua, ub;
    ua.f = a;
    ub.f = b;
    return ua.l == ub.l;
}

static inline int
double_equality(double a, double b)
{
    union { double d; unsigned long l[2]; } ua, ub;
    ua.d = a;
    ub.d = b;
    return ua.l[0] == ub.l[0] && ua.l[1] == ub.l[1];
}

/*
 * dx7_voice_render
 *
 * generate the actual sound data for this voice
 */
void
dx7_voice_render(hexter_instance_t *instance, dx7_voice_t *voice,
                 LADSPA_Data *out, unsigned long sample_count,
                 int do_control_update)
{
    unsigned long       sample;
    static dx7_sample_t ampmod[4] = { 0 };
    dx7_sample_t        i;
    dx7_sample_t        output;

    if (!float_equality(voice->last_port_volume, *instance->volume) ||
        voice->last_cc_volume != instance->cc_volume)
        dx7_voice_recalculate_volume(instance, voice);

    switch (voice->algorithm) {

      case 0: /* algorithm 1 */

        /* This first algorithm is all written out, so you can see how it looks */

        for (sample = 0; sample < sample_count; sample++) {

            /* calculate amplitude modulation amounts */
            i = FP_MULTIPLY(voice->amp_mod_lfo_amd_value, voice->lfo_delay_value);
            i = voice->amp_mod_env_value +
                    FP_MULTIPLY(i + voice->amp_mod_lfo_mods_value, instance->lfo_buffer[sample]);

#ifndef HEXTER_USE_FLOATING_POINT
#define AMPMOD2_CONSTANT  (7726076 >> (24 - FP_SHIFT))  /* 0.460510 */
#define AMPMOD1_CONSTANT  (3993950 >> (24 - FP_SHIFT))  /* 0.238058 */
#else /* HEXTER_USE_FLOATING_POINT */
#define AMPMOD2_CONSTANT  (0.460510f)
#define AMPMOD1_CONSTANT  (0.238058f)
#endif /* HEXTER_USE_FLOATING_POINT */
            ampmod[3] = i;
            ampmod[2] = FP_MULTIPLY(i, AMPMOD2_CONSTANT);
            ampmod[1] = FP_MULTIPLY(i, AMPMOD1_CONSTANT);

            output = (
                      dx7_op_calculate_operator(voice->op[OP_3].eg.value - ampmod[voice->op[OP_3].amp_mod_sens],
                                                voice->op[OP_3].phase +
                                                dx7_op_calculate_operator(voice->op[OP_4].eg.value - ampmod[voice->op[OP_4].amp_mod_sens],
                                                                          voice->op[OP_4].phase +
                                                                          dx7_op_calculate_operator(voice->op[OP_5].eg.value - ampmod[voice->op[OP_5].amp_mod_sens],
                                                                                                    voice->op[OP_5].phase +
                                                                                                    /* -FIX- need to determine if amp mod is included in feedback, or after */
                                                                                                    dx7_op_calculate_operator_saving_feedback(voice,
                                                                                                                                              voice->op[OP_6].eg.value - ampmod[voice->op[OP_6].amp_mod_sens],
                                                                                                                                              voice->op[OP_6].phase +
                                                                                                                                              voice->feedback)))) +
                      dx7_op_calculate_operator(voice->op[OP_1].eg.value - ampmod[voice->op[OP_1].amp_mod_sens],
                                                voice->op[OP_1].phase +
                                                dx7_op_calculate_operator(voice->op[OP_2].eg.value - ampmod[voice->op[OP_2].amp_mod_sens],
                                                                          voice->op[OP_2].phase))
                     );
            /* voice->volume_value contains a scaling factor for the number of carriers */

            /* mix voice output into output buffer */
            out[sample] += FP_TO_FLOAT(output) * voice->volume_value;

            /* update runtime parameters for next sample */
            voice->op[OP_6].phase += voice->op[OP_6].phase_increment;
            voice->op[OP_5].phase += voice->op[OP_5].phase_increment;
            voice->op[OP_4].phase += voice->op[OP_4].phase_increment;
            voice->op[OP_3].phase += voice->op[OP_3].phase_increment;
            voice->op[OP_2].phase += voice->op[OP_2].phase_increment;
            voice->op[OP_1].phase += voice->op[OP_1].phase_increment;

            dx7_op_eg_process(instance, &voice->op[OP_6].eg);
            dx7_op_eg_process(instance, &voice->op[OP_5].eg);
            dx7_op_eg_process(instance, &voice->op[OP_4].eg);
            dx7_op_eg_process(instance, &voice->op[OP_3].eg);
            dx7_op_eg_process(instance, &voice->op[OP_2].eg);
            dx7_op_eg_process(instance, &voice->op[OP_1].eg);

            if (voice->amp_mod_env_duration) {
                voice->amp_mod_env_value += voice->amp_mod_env_increment;
                voice->amp_mod_env_duration--;
            }
            if (voice->amp_mod_lfo_mods_duration) {
                voice->amp_mod_lfo_mods_value += voice->amp_mod_lfo_mods_increment;
                voice->amp_mod_lfo_mods_duration--;
            }
            if (voice->amp_mod_lfo_amd_duration) {
                voice->amp_mod_lfo_amd_value += voice->amp_mod_lfo_amd_increment;
                voice->amp_mod_lfo_amd_duration--;
            }
            if (voice->lfo_delay_duration) {
                voice->lfo_delay_value += voice->lfo_delay_increment;
                if (--voice->lfo_delay_duration == 0) {
                    int seg = ++voice->lfo_delay_segment;
                    voice->lfo_delay_duration  = instance->lfo_delay_duration[seg];
                    voice->lfo_delay_value     = instance->lfo_delay_value[seg];
                    voice->lfo_delay_increment = instance->lfo_delay_increment[seg];
                }
            }
            if (voice->volume_duration) {
                voice->volume_value += voice->volume_increment;
                voice->volume_duration--;
            }
        }
        break;

      /* Now we'll use some macros to make it easier to read */
#define op(_i, _p)     dx7_op_calculate_operator(voice->op[_i].eg.value - ampmod[voice->op[_i].amp_mod_sens], voice->op[_i].phase + _p)
#define op_sfb(_i, _p) dx7_op_calculate_operator_saving_feedback(voice, voice->op[_i].eg.value - ampmod[voice->op[_i].amp_mod_sens], voice->op[_i].phase + _p)

#define RENDER \
        for (sample = 0; sample < sample_count; sample++) { \
            /* calculate amplitude modulation amounts */ \
            i = FP_MULTIPLY(voice->amp_mod_lfo_amd_value, voice->lfo_delay_value); \
            i = voice->amp_mod_env_value + \
                    FP_MULTIPLY(i + voice->amp_mod_lfo_mods_value, instance->lfo_buffer[sample]); \
            ampmod[3] = i; \
            ampmod[2] = FP_MULTIPLY(i, AMPMOD2_CONSTANT); \
            ampmod[1] = FP_MULTIPLY(i, AMPMOD1_CONSTANT); \
            ALGORITHM; \
            /* voice->volume_value contains a scaling factor for the number of carriers */ \
            /* mix voice output into output buffer */ \
            out[sample] += FP_TO_FLOAT(output) * voice->volume_value; \
            /* update runtime parameters for next sample */ \
            voice->op[OP_6].phase += voice->op[OP_6].phase_increment; \
            voice->op[OP_5].phase += voice->op[OP_5].phase_increment; \
            voice->op[OP_4].phase += voice->op[OP_4].phase_increment; \
            voice->op[OP_3].phase += voice->op[OP_3].phase_increment; \
            voice->op[OP_2].phase += voice->op[OP_2].phase_increment; \
            voice->op[OP_1].phase += voice->op[OP_1].phase_increment; \
            dx7_op_eg_process(instance, &voice->op[OP_6].eg); \
            dx7_op_eg_process(instance, &voice->op[OP_5].eg); \
            dx7_op_eg_process(instance, &voice->op[OP_4].eg); \
            dx7_op_eg_process(instance, &voice->op[OP_3].eg); \
            dx7_op_eg_process(instance, &voice->op[OP_2].eg); \
            dx7_op_eg_process(instance, &voice->op[OP_1].eg); \
            if (voice->amp_mod_env_duration) { \
                voice->amp_mod_env_value += voice->amp_mod_env_increment; \
                voice->amp_mod_env_duration--; \
            } \
            if (voice->amp_mod_lfo_mods_duration) { \
                voice->amp_mod_lfo_mods_value += voice->amp_mod_lfo_mods_increment; \
                voice->amp_mod_lfo_mods_duration--; \
            } \
            if (voice->amp_mod_lfo_amd_duration) { \
                voice->amp_mod_lfo_amd_value += voice->amp_mod_lfo_amd_increment; \
                voice->amp_mod_lfo_amd_duration--; \
            } \
            if (voice->lfo_delay_duration) { \
                voice->lfo_delay_value += voice->lfo_delay_increment; \
                if (--voice->lfo_delay_duration == 0) { \
                    int seg = ++voice->lfo_delay_segment; \
                    voice->lfo_delay_duration  = instance->lfo_delay_duration[seg]; \
                    voice->lfo_delay_value     = instance->lfo_delay_value[seg]; \
                    voice->lfo_delay_increment = instance->lfo_delay_increment[seg]; \
                } \
            } \
            if (voice->volume_duration) { \
                voice->volume_value += voice->volume_increment; \
                voice->volume_duration--; \
            } \
        }

      case 1: /* algorithm 2 */

#define ALGORITHM { \
            output = (                                             \
                      op(OP_3, op(OP_4, op(OP_5, op(OP_6, 0)))) +  \
                      op(OP_1, op_sfb(OP_2, voice->feedback))      \
                     );                                            \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 2: /* algorithm 3 */

#define ALGORITHM { \
            output = (                                                     \
                      op(OP_4, op(OP_5, op_sfb(OP_6, voice->feedback))) +  \
                      op(OP_1, op(OP_2, op(OP_3, 0)))                      \
                     );                                                    \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 3: /* algorithm 4 */

#define ALGORITHM { \
            output = (                                                     \
                      op_sfb(OP_4, op(OP_5, op(OP_6, voice->feedback))) +  \
                      op(OP_1, op(OP_2, op(OP_3, 0)))                      \
                     );                                                    \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 4: /* algorithm 5 */

#define ALGORITHM { \
            output = (                                           \
                      op(OP_5, op_sfb(OP_6, voice->feedback)) +  \
                      op(OP_3, op(OP_4, 0)) +                    \
                      op(OP_1, op(OP_2, 0))                      \
                     );                                          \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 5: /* algorithm 6 */

#define ALGORITHM { \
            output = (                                           \
                      op_sfb(OP_5, op(OP_6, voice->feedback)) +  \
                      op(OP_3, op(OP_4, 0)) +                    \
                      op(OP_1, op(OP_2, 0))                      \
                     );                                          \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 6: /* algorithm 7 */

#define ALGORITHM { \
            output = (                                                    \
                      op(OP_3, op(OP_5, op_sfb(OP_6, voice->feedback)) +  \
                               op(OP_4, 0)) +                             \
                      op(OP_1, op(OP_2, 0))                               \
                     );                                                   \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 7: /* algorithm 8 */

#define ALGORITHM { \
            output = (                                           \
                      op(OP_3, op(OP_5, op(OP_6, 0)) +           \
                               op_sfb(OP_4, voice->feedback)) +  \
                      op(OP_1, op(OP_2, 0))                      \
                     );                                          \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 8: /* algorithm 9 */

#define ALGORITHM { \
            output = (                                         \
                      op(OP_3, op(OP_5, op(OP_6, 0)) +         \
                               op(OP_4, 0)) +                  \
                      op(OP_1, op_sfb(OP_2, voice->feedback))  \
                     );                                        \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 9: /* algorithm 10 */

#define ALGORITHM { \
            output = (                                                   \
                      op(OP_4, op(OP_6, 0) +                             \
                               op(OP_5, 0)) +                            \
                      op(OP_1, op(OP_2, op_sfb(OP_3, voice->feedback)))  \
                     );                                                  \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 10: /* algorithm 11 */

#define ALGORITHM { \
            output = (                                          \
                      op(OP_4, op_sfb(OP_6, voice->feedback) +  \
                               op(OP_5, 0)) +                   \
                      op(OP_1, op(OP_2, op(OP_3, 0)))           \
                     );                                         \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 11: /* algorithm 12 */

#define ALGORITHM { \
            output = (                                         \
                      op(OP_3, op(OP_6, 0) +                   \
                               op(OP_5, 0) +                   \
                               op(OP_4, 0)) +                  \
                      op(OP_1, op_sfb(OP_2, voice->feedback))  \
                     );                                        \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 12: /* algorithm 13 */

#define ALGORITHM { \
            output = (                                          \
                      op(OP_3, op_sfb(OP_6, voice->feedback) +  \
                               op(OP_5, 0) +                    \
                               op(OP_4, 0)) +                   \
                      op(OP_1, op(OP_2, 0))                     \
                     );                                         \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 13: /* algorithm 14 */

#define ALGORITHM { \
            output = (                                                   \
                      op(OP_3, op(OP_4, op_sfb(OP_6, voice->feedback) +  \
                                        op(OP_5, 0))) +                  \
                      op(OP_1, op(OP_2, 0))                              \
                     );                                                  \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 14: /* algorithm 15 */

#define ALGORITHM { \
            output = (                                         \
                      op(OP_3, op(OP_4, op(OP_6, 0) +          \
                                        op(OP_5, 0))) +        \
                      op(OP_1, op_sfb(OP_2, voice->feedback))  \
                     );                                        \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 15: /* algorithm 16 */

#define ALGORITHM { \
            output = op(OP_1, op(OP_5, op_sfb(OP_6, voice->feedback)) +  \
                              op(OP_3, op(OP_4, 0)) +                    \
                              op(OP_2, 0));                              \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 16: /* algorithm 17 */

#define ALGORITHM { \
            output = op(OP_1, op(OP_5, op(OP_6, 0)) +          \
                              op(OP_3, op(OP_4, 0)) +          \
                              op_sfb(OP_2, voice->feedback));  \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 17: /* algorithm 18 */

#define ALGORITHM { \
            output = op(OP_1, op(OP_4, op(OP_5, op(OP_6, 0))) +  \
                              op_sfb(OP_3, voice->feedback) +    \
                              op(OP_2, 0));                      \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 18: /* algorithm 19 */

#define ALGORITHM { \
            i = op_sfb(OP_6, voice->feedback);         \
            output = (                                 \
                      op(OP_5, i) +                    \
                      op(OP_4, i) +                    \
                      op(OP_1, op(OP_2, op(OP_3, 0)))  \
                     );                                \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 19: /* algorithm 20 */

#define ALGORITHM { \
            i = op_sfb(OP_3, voice->feedback);  \
            output = (                          \
                      op(OP_4, op(OP_6, 0) +    \
                               op(OP_5, 0)) +   \
                      op(OP_2, i) +             \
                      op(OP_1, i)               \
                     );                         \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 20: /* algorithm 21 */

#define ALGORITHM { \
            i = op(OP_6, 0);                    \
            output = op(OP_5, i) +              \
                     op(OP_4, i);               \
            i = op_sfb(OP_3, voice->feedback);  \
            output += op(OP_2, i) +             \
                      op(OP_1, i);              \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 21: /* algorithm 22 */

#define ALGORITHM { \
            i = op_sfb(OP_6, voice->feedback);  \
            output = (                          \
                      op(OP_5, i) +             \
                      op(OP_4, i) +             \
                      op(OP_3, i) +             \
                      op(OP_1, op(OP_2, 0))     \
                     );                         \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 22: /* algorithm 23 */

#define ALGORITHM { \
            i = op_sfb(OP_6, voice->feedback);  \
            output = (                          \
                      op(OP_5, i) +             \
                      op(OP_4, i) +             \
                      op(OP_2, op(OP_3, 0)) +   \
                      op(OP_1, 0)               \
                     );                         \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 23: /* algorithm 24 */

#define ALGORITHM { \
            i = op_sfb(OP_6, voice->feedback);  \
            output = (                          \
                      op(OP_5, i) +             \
                      op(OP_4, i) +             \
                      op(OP_3, i) +             \
                      op(OP_2, 0) +             \
                      op(OP_1, 0)               \
                     );                         \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 24: /* algorithm 25 */

#define ALGORITHM { \
            i = op_sfb(OP_6, voice->feedback);  \
            output = (                          \
                      op(OP_5, i) +             \
                      op(OP_4, i) +             \
                      op(OP_3, 0) +             \
                      op(OP_2, 0) +             \
                      op(OP_1, 0)               \
                     );                         \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 25: /* algorithm 26 */

#define ALGORITHM { \
            output = (                                          \
                      op(OP_4, op_sfb(OP_6, voice->feedback) +  \
                               op(OP_5, 0)) +                   \
                      op(OP_2, op(OP_3, 0)) +                   \
                      op(OP_1, 0)                               \
                     );                                         \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 26: /* algorithm 27 */

#define ALGORITHM { \
            output = (                                           \
                      op(OP_4, op(OP_6, 0) +                     \
                               op(OP_5, 0)) +                    \
                      op(OP_2, op_sfb(OP_3, voice->feedback)) +  \
                      op(OP_1, 0)                                \
                     );                                          \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 27: /* algorithm 28 */

#define ALGORITHM { \
            output = (                                                     \
                      op(OP_6, 0) +                                        \
                      op(OP_3, op(OP_4, op_sfb(OP_5, voice->feedback))) +  \
                      op(OP_1, op(OP_2, 0))                                \
                     );                                                    \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 28: /* algorithm 29 */

#define ALGORITHM { \
            output = (                                           \
                      op(OP_5, op_sfb(OP_6, voice->feedback)) +  \
                      op(OP_3, op(OP_4, 0)) +                    \
                      op(OP_2, 0) +                              \
                      op(OP_1, 0)                                \
                     );                                          \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 29: /* algorithm 30 */

#define ALGORITHM { \
            output = (                                                     \
                      op(OP_6, 0) +                                        \
                      op(OP_3, op(OP_4, op_sfb(OP_5, voice->feedback))) +  \
                      op(OP_2, 0) +                                        \
                      op(OP_1, 0)                                          \
                     );                                                    \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 30: /* algorithm 31 */

#define ALGORITHM { \
            output = (                                           \
                      op(OP_5, op_sfb(OP_6, voice->feedback)) +  \
                      op(OP_4, 0) +                              \
                      op(OP_3, 0) +                              \
                      op(OP_2, 0) +                              \
                      op(OP_1, 0)                                \
                     );                                          \
        }

        RENDER;
        break;
#undef ALGORITHM

      case 31: /* algorithm 32 */
      default: /* just in case */

#define ALGORITHM { \
            output = (                                 \
                      op_sfb(OP_6, voice->feedback) +  \
                      op(OP_5, 0) +                    \
                      op(OP_4, 0) +                    \
                      op(OP_3, 0) +                    \
                      op(OP_2, 0) +                    \
                      op(OP_1, 0)                      \
                     );                                \
        }

        RENDER;
        break;
#undef ALGORITHM

#undef op
#undef op_sfb
    }

    if (do_control_update) {
        double new_pitch;

        /* do those things which should be done only once per control-
         * calculation interval ("nugget"), such as voice check-for-dead,
         * pitch envelope calculations, etc. */

        /* check if we've decayed to nothing, turn off voice if so */
        if (dx7_voice_check_for_dead(voice))
            return; /* we're dead now, so return */

#ifdef HEXTER_USE_FLOATING_POINT
        /* wrap oscillator phases */
        voice->op[OP_6].phase -= floorf(voice->op[OP_6].phase);
        voice->op[OP_5].phase -= floorf(voice->op[OP_5].phase);
        voice->op[OP_4].phase -= floorf(voice->op[OP_4].phase);
        voice->op[OP_3].phase -= floorf(voice->op[OP_3].phase);
        voice->op[OP_2].phase -= floorf(voice->op[OP_2].phase);
        voice->op[OP_1].phase -= floorf(voice->op[OP_1].phase);
#endif /* HEXTER_USE_FLOATING_POINT */

        /* update pitch envelope and portamento */
        dx7_pitch_eg_process(instance, &voice->pitch_eg);
        dx7_portamento_process(instance, &voice->portamento);

        /* update phase increments if pitch or tuning changed */
        new_pitch = voice->pitch_eg.value + voice->portamento.value +
                    instance->pitch_bend -
                    instance->lfo_value_for_pitch *
                        (voice->pitch_mod_depth_pmd * FP_TO_DOUBLE(voice->lfo_delay_value) +
                         voice->pitch_mod_depth_mods);
        if (!double_equality(voice->last_pitch, new_pitch) ||
            !float_equality(voice->last_port_tuning, *instance->tuning)) {

            dx7_voice_recalculate_freq_and_inc(instance, voice);
        }

        /* op envelope rounding correction */
        dx7_op_eg_adjust(&voice->op[OP_6].eg);
        dx7_op_eg_adjust(&voice->op[OP_5].eg);
        dx7_op_eg_adjust(&voice->op[OP_4].eg);
        dx7_op_eg_adjust(&voice->op[OP_3].eg);
        dx7_op_eg_adjust(&voice->op[OP_2].eg);
        dx7_op_eg_adjust(&voice->op[OP_1].eg);

        /* mods and output volume */
        if (!voice->amp_mod_env_duration)
            voice->amp_mod_env_value = voice->amp_mod_env_target;
        if (!voice->amp_mod_lfo_mods_duration)
            voice->amp_mod_lfo_mods_value = voice->amp_mod_lfo_mods_target;
        if (!voice->amp_mod_lfo_amd_duration)
            voice->amp_mod_lfo_amd_value = voice->amp_mod_lfo_amd_target;
        if (!voice->volume_duration)
            voice->volume_value = voice->volume_target;
    }
}
