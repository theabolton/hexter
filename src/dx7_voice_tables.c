/* hexter DSSI software synthesizer plugin
 *
 * Copyright (C) 2004, 2009, 2011 Sean Bolton and others.
 *
 * Portions of this file may have come from Juan Linietsky's
 * rx-saturno, copyright (C) 2002 by Juan Linietsky.
 * Portions of this file may have come from Jeff Harrington's
 * dx72csound package, for which I've found no copyright
 * attribution.
 * Significant reverse-engineering contributions were made by
 * Jamie Bullock.
 * Other parts may have been inspired by Chowning/Bristow,
 * Pinkston, Yala Abdullah's website, Godric Wilkie' website;
 * see the AUTHORS file for more detail.
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
#include "dx7_voice.h"

static int dx7_voice_tables_initialized = 0;

dx7_sample_t    dx7_voice_sin_table[SINE_SIZE + 1];

extern dx7_sample_t dx7_voice_eg_ol_to_mod_index_table[257]; /* forward */

dx7_sample_t  *dx7_voice_eg_ol_to_mod_index = &dx7_voice_eg_ol_to_mod_index_table[128];

void dx7_voice_init_tables(void) {

    int i;
    double f;

    if (!dx7_voice_tables_initialized) {

        for (i = 0; i <= SINE_SIZE; i++) {

            /* observation of my TX7's output with oscillator sync on suggests
             * it uses cosine */
            f = cos((double)(i) / SINE_SIZE * (2 * M_PI));  /* index / index max * radian cycle */
            dx7_voice_sin_table[i] = DOUBLE_TO_FP(f);
        }

#ifndef HEXTER_USE_FLOATING_POINT
#if FP_SHIFT != 24
        /* Any fixed-point tables below are in s7.24 format.  Shift
         * them to match FP_SHIFT. */
        for (i = 0; i <= 256; i++) {
            dx7_voice_eg_ol_to_mod_index_table[i] >>= (24 - FP_SHIFT);
        }
#endif
#endif /* ! HEXTER_USE_FLOATING_POINT */

        dx7_voice_tables_initialized = 1;
    }
}

/* This table lists which operators of an algorithm are carriers.  Bit 0 (LSB)
 * is set if operator 1 is a carrier, and so on through bit 5 for operator 6.
 */
uint8_t dx7_voice_carriers[32] = {
    0x05, /* algorithm 1, operators 1 and 3 */
    0x05,
    0x09, /* algorithm 3, operators 1 and 4 */
    0x09,
    0x15, /* algorithm 5, operators 1, 3, and 5 */
    0x15,
    0x05,
    0x05,
    0x05,
    0x09,
    0x09,
    0x05,
    0x05,
    0x05,
    0x05,
    0x01, /* algorithm 16, operator 1 */
    0x01,
    0x01,
    0x19, /* algorithm 19, operators 1, 4, and 5 */
    0x0b, /* algorithm 20, operators 1, 2, and 4 */
    0x1b, /* algorithm 21, operators 1, 2, 4, and 5 */
    0x1d, /* algorithm 22, operators 1, 3, 4, and 5 */
    0x1b,
    0x1f, /* algorithm 24, operators 1 through 5 */
    0x1f,
    0x0b,
    0x0b,
    0x25, /* algorithm 28, operators 1, 3, and 6 */
    0x17, /* algorithm 29, operators 1, 2, 3, and 5 */
    0x27, /* algorithm 30, operators 1, 2, 3, and 6 */
    0x1f,
    0x2f, /* algorithm 32, all operators */
};

float dx7_voice_carrier_count[32] = {
    2.0f, 2.0f, 2.0f, 2.0f, 3.0f, 3.0f, 2.0f, 2.0f,
    2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 1.0f,
    1.0f, 1.0f, 3.0f, 3.0f, 4.0f, 4.0f, 4.0f, 5.0f,
    5.0f, 3.0f, 3.0f, 3.0f, 4.0f, 4.0f, 5.0f, 6.0f
};

/* This table converts an output level of 0 to 99 into a phase
 * modulation index of 0 to ~2.089 periods.  It actually extends
 * below 0 and beyond 99, since amplitude modulation can produce
 * 'negative' output levels, and velocities above 100 can produce
 * output levels above 99, plus it includes a 257th 'guard' point.
 * Table index 128 corresponds to output level 0, and index 227 to OL 99.
 * I believe this is based on information from the Chowning/Bristow
 * book (see the CREDITS file), filtered down to me through the work of
 * Pinkston, Harrington, and Abdullah as I found it on the Internet.  The
 * code used to calculate it looks something like this:
 *
 *    // DX7 output level to TL translation table
 *    int tl_table[128] = {
 *        127, 122, 118, 114, 110, 107, 104, 102, 100, 98, 96, 94, 92, 90,
 *        88, 86, 85, 84, 82, 81, 79, 78, 77, 76, 75, 74, 73, 72, 71,
 *        70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55,
 *        54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39,
 *        38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23,
 *        22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6,
 *        5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11,
 *        -12, -13, -14, -15, -16, -17, -18, -19, -20, -21, -22, -23, -24,
 *        -25, -26, -27, -28
 *    };
 *
 *    int ol;
 *    double mi;
 *
 *    for (ol = 0; ol < 128; ol++) {
 *        if (ol < 5) {    // smoothly ramp from 0.0 at 0 to the proper value at 5
 *            mi = pow(2.0, ( (33.0/16.0) - ((double)tl_table[5]/8.0) - 1.0));
 *            mi = mi * ((double)ol / 5.0);
 *        } else {
 *            mi = pow(2.0, ( (33.0/16.0) - ((double)tl_table[ol]/8.0) - 1.0));
 *        }
 *    #ifndef HEXTER_USE_FLOATING_POINT
 *        printf(" %6d,", DOUBLE_TO_FP(mi));
 *    #else
 *        printf(" %g,", mi);
 *    #endif
 *    }
 */
dx7_sample_t dx7_voice_eg_ol_to_mod_index_table[257] = {
#ifndef HEXTER_USE_FLOATING_POINT
    /* phase modulation index, expressed in s7.24 fixed point */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 659, 1319, 1978, 2638, 3298, 4277, 5086, 6049, 7193, 8554,
    10173, 12098, 14387, 17109, 20346, 22188, 24196, 28774, 31378,
    37315, 40693, 44376, 48392, 52772, 57548, 62757, 68437, 74631,
    81386, 88752, 96785, 105545, 115097, 125514, 136875, 149263,
    162772, 177504, 193570, 211090, 230195, 251029, 273750, 298526,
    325545, 355009, 387141, 422180, 460390, 502059, 547500, 597053,
    651091, 710019, 774282, 844360, 920781, 1004119, 1095000,
    1194106, 1302182, 1420039, 1548564, 1688721, 1841563, 2008239,
    2190000, 2388212, 2604364, 2840079, 3097128, 3377443, 3683127,
    4016479, 4380001, 4776425, 5208729, 5680159, 6194257, 6754886,
    7366255, 8032958, 8760003, 9552851, 10417458, 11360318,
    12388515, 13509772, 14732510, 16065917, 17520006, 19105702,
    20834916, 22720637, 24777031, 27019544, 29465021, 32131834,
    35040013, 38211405, 41669833, 45441275, 49554062, 54039088,
    58930043, 64263668, 70080027, 76422811, 83339667, 90882551,
    99108124, 108078176, 117860087, 128527336, 140160054, 152845623,
    166679334, 181765102, 198216249, 216156353, 235720174,
    257054673, 280320108, 305691246, 333358668, 363530205,
    396432499, 396432499
#else /* HEXTER_USE_FLOATING_POINT */
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 3.93186e-05f, 7.86372e-05f, 0.000117956f,
    0.000157274f, 0.000196593f, 0.00025495f, 0.000303188f,
    0.000360553f, 0.000428773f, 0.000509899f, 0.000606376f,
    0.000721107f, 0.000857545f, 0.0010198f, 0.00121275f,
    0.00132252f, 0.00144221f, 0.00171509f, 0.00187032f,
    0.0022242f, 0.0024255f, 0.00264503f, 0.00288443f,
    0.00314549f, 0.00343018f, 0.00374064f, 0.00407919f,
    0.00444839f, 0.00485101f, 0.00529006f, 0.00576885f,
    0.00629098f, 0.00686036f, 0.00748128f, 0.00815839f,
    0.00889679f, 0.00970201f, 0.0105801f, 0.0115377f,
    0.012582f, 0.0137207f, 0.0149626f, 0.0163168f,
    0.0177936f, 0.019404f, 0.0211602f, 0.0230754f,
    0.0251639f, 0.0274414f, 0.0299251f, 0.0326336f,
    0.0355871f, 0.0388081f, 0.0423205f, 0.0461508f,
    0.0503278f, 0.0548829f, 0.0598502f, 0.0652671f,
    0.0711743f, 0.0776161f, 0.084641f, 0.0923016f,
    0.100656f, 0.109766f, 0.1197f, 0.130534f,
    0.142349f, 0.155232f, 0.169282f, 0.184603f,
    0.201311f, 0.219532f, 0.239401f, 0.261068f,
    0.284697f, 0.310464f, 0.338564f, 0.369207f,
    0.402623f, 0.439063f, 0.478802f, 0.522137f,
    0.569394f, 0.620929f, 0.677128f, 0.738413f,
    0.805245f, 0.878126f, 0.957603f, 1.04427f,
    1.13879f, 1.24186f, 1.35426f, 1.47683f,
    1.61049f, 1.75625f, 1.91521f, 2.08855f,
    2.27758f, 2.48372f, 2.70851f, 2.95365f,
    3.22098f, 3.5125f, 3.83041f, 4.1771f,
    4.55515f, 4.96743f, 5.41702f, 5.9073f,
    6.44196f, 7.02501f, 7.66083f, 8.35419f,
    9.11031f, 9.93486f, 10.834f, 11.8146f,
    12.8839f, 14.05f, 15.3217f, 16.7084f,
    18.2206f, 19.8697f, 21.6681f, 23.6292f,
    23.6292f
#endif /* HEXTER_USE_FLOATING_POINT */
};

/* This table lists the output level adjustment needed for a certain
 * velocity, expressed in output level units per unit of velocity
 * sensitivity. It is based on measurements I took from my TX7. */
float dx7_voice_velocity_ol_adjustment[128] = {

    -99.0,    -10.295511, -9.709229, -9.372207,
    -9.121093, -8.629703, -8.441805, -8.205647,
    -7.810857, -7.653259, -7.299901, -7.242308,
    -6.934396, -6.727051, -6.594723, -6.427755,
    -6.275133, -6.015212, -5.843023, -5.828787,
    -5.725659, -5.443202, -5.421110, -5.222133,
    -5.160615, -5.038265, -4.948225, -4.812105,
    -4.632120, -4.511531, -4.488645, -4.370043,
    -4.370610, -4.058591, -4.066902, -3.952988,
    -3.909686, -3.810096, -3.691883, -3.621306,
    -3.527286, -3.437519, -3.373512, -3.339195,
    -3.195983, -3.167622, -3.094788, -2.984045,
    -2.937463, -2.890713, -2.890660, -2.691874,
    -2.649229, -2.544696, -2.498147, -2.462573,
    -2.396637, -2.399795, -2.236338, -2.217625,
    -2.158336, -2.135569, -1.978521, -1.913965,
    -1.937082, -1.752275, -1.704013, -1.640514,
    -1.598791, -1.553859, -1.512187, -1.448088,
    -1.450443, -1.220567, -1.182340, -1.123139,
    -1.098469, -1.020642, -0.973039, -0.933279,
    -0.938035, -0.757380, -0.740860, -0.669721,
    -0.681526, -0.555390, -0.519321, -0.509318,
    -0.456936, -0.460622, -0.290578, -0.264393,
    -0.252716, -0.194141, -0.153566, -0.067842,
    -0.033402, -0.054947,  0.012860,  0.000000,
    -0.009715,  0.236054,  0.273956,  0.271968,
     0.330177,  0.345427,  0.352333,  0.433861,
     0.442952,  0.476411,  0.539632,  0.525355,
     0.526115,  0.707022,  0.701551,  0.734875,
     0.739149,  0.794320,  0.801578,  0.814225,
     0.818939,  0.897102,  0.895082,  0.927998,
     0.929797,  0.956112,  0.956789,  0.958121

};

/* This table converts LFO speed to frequency in Hz. It is based on
 * interpolation of Jamie Bullock's measurements. */
float dx7_voice_lfo_frequency[128] = {
     0.062506,  0.124815,  0.311474,  0.435381,  0.619784,
     0.744396,  0.930495,  1.116390,  1.284220,  1.496880,
     1.567830,  1.738994,  1.910158,  2.081322,  2.252486,
     2.423650,  2.580668,  2.737686,  2.894704,  3.051722,
     3.208740,  3.366820,  3.524900,  3.682980,  3.841060,
     3.999140,  4.159420,  4.319700,  4.479980,  4.640260,
     4.800540,  4.953584,  5.106628,  5.259672,  5.412716,
     5.565760,  5.724918,  5.884076,  6.043234,  6.202392,
     6.361550,  6.520044,  6.678538,  6.837032,  6.995526,
     7.154020,  7.300500,  7.446980,  7.593460,  7.739940,
     7.886420,  8.020588,  8.154756,  8.288924,  8.423092,
     8.557260,  8.712624,  8.867988,  9.023352,  9.178716,
     9.334080,  9.669644, 10.005208, 10.340772, 10.676336,
    11.011900, 11.963680, 12.915460, 13.867240, 14.819020,
    15.770800, 16.640240, 17.509680, 18.379120, 19.248560,
    20.118000, 21.040700, 21.963400, 22.886100, 23.808800,
    24.731500, 25.759740, 26.787980, 27.816220, 28.844460,
    29.872700, 31.228200, 32.583700, 33.939200, 35.294700,
    36.650200, 37.812480, 38.974760, 40.137040, 41.299320,
    42.461600, 43.639800, 44.818000, 45.996200, 47.174400,
    47.174400, 47.174400, 47.174400, 47.174400, 47.174400,
    47.174400, 47.174400, 47.174400, 47.174400, 47.174400,
    47.174400, 47.174400, 47.174400, 47.174400, 47.174400,
    47.174400, 47.174400, 47.174400, 47.174400, 47.174400,
    47.174400, 47.174400, 47.174400, 47.174400, 47.174400,
    47.174400, 47.174400, 47.174400
};

/* This table converts pitch modulation sensitivity to semitones at full
 * modulation (assuming a perfectly linear pitch mod depth to pitch
 * relationship).  It is from a simple averaging of Jamie Bullock's
 * TX-data-1/PMD and TX-data-2/ENV data, and ignores the apparent ~0.1
 * semitone positive bias that Jamie observed. [-FIX- smbolton: my
 * inclination would be to call this bias, if it's reproducible, a
 * non-desirable 'bug', and _not_ implement it in hexter. And, at
 * least for my own personal build, I'd change that PMS=7 value to a
 * full octave, since that's one thing that's always bugged me about
 * my TX7.  Thoughts? ] */
float dx7_voice_pms_to_semitones[8] = {
    0.0, 0.450584, 0.900392, 1.474744,
    2.587385, 4.232292, 6.982097, /* 11.722111 */ 12.0
};

/* This table converts amplitude modulation depth to output level
 * reduction at full modulation with an amplitude modulation sensitivity
 * of 3.  It was constructed from regression of a very few data points,
 * using this code:
 *   perl -e 'for ($i = 0; $i <= 99; $i++) { printf " %f,\n", exp($i * 0.0428993 - 0.285189); }' >x.c
 * and is probably rather rough in its accuracy. -FIX- */
float dx7_voice_amd_to_ol_adjustment[100] = {
    0.0, 0.784829, 0.819230, 0.855139, 0.892622, 0.931748,
    0.972589, 1.015221, 1.059721, 1.106171, 1.154658, 1.205270,
    1.258100, 1.313246, 1.370809, 1.430896, 1.493616, 1.559085,
    1.627424, 1.698759, 1.773220, 1.850945, 1.932077, 2.016765,
    2.105166, 2.197441, 2.293761, 2.394303, 2.499252, 2.608801,
    2.723152, 2.842515, 2.967111, 3.097167, 3.232925, 3.374633,
    3.522552, 3.676956, 3.838127, 4.006362, 4.181972, 4.365280,
    4.556622, 4.756352, 4.964836, 5.182458, 5.409620, 5.646738,
    5.894251, 6.152612, 6.422298, 6.703805, 6.997652, 7.304378,
    7.624549, 7.958754, 8.307609, 8.671754, 9.051861, 9.448629,
    9.862789, 10.295103, 10.746365, 11.217408, 11.709099,
    12.222341, 12.758080, 13.317302, 13.901036, 14.510357,
    15.146387, 15.810295, 16.503304, 17.226690, 17.981783,
    18.769975, 19.592715, 20.451518, 21.347965, 22.283705,
    23.260462, 24.280032, 25.344294, 26.455204, 27.614809,
    28.825243, 30.088734, 31.407606, 32.784289, 34.221315,
    35.721330, 37.287095, 38.921492, 40.627529, 42.408347,
    44.267222, 46.207578, 48.232984, 50.347169, 52.75
};

/* This table converts modulation source sensitivity (e.g. 'foot
 * controller sensitivity') into output level reduction at full modulation
 * with amplitude modulation sensitivity 3.  It's basically just the above
 * table scaled for 0 to 15 instead of 0 to 99. */
float dx7_voice_mss_to_ol_adjustment[16] = {
    0.0, 0.997948, 1.324562, 1.758071, 2.333461, 3.097167, 4.110823,
    5.456233, 7.241976, 9.612164, 12.758080, 16.933606, 22.475719,
    29.831681, 39.595137, 52.75
};
