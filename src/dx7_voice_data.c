/* hexter DSSI software synthesizer plugin and GUI
 *
 * Copyright (C) 2004, 2009, 2011 Sean Bolton and others.
 *
 * DX7 patchbank loading code by Martin Tarenskeen.
 *
 * Portions of this file may have come from Peter Hanappe's
 * Fluidsynth, copyright (C) 2003 Peter Hanappe and others.
 * The envelope generator rate tables are based on work by
 * Russell Pinkston and Jeff Harrington.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "hexter_types.h"
#include "dx7_voice.h"
#include "dx7_voice_data.h"
#include "gui_data.h"

/* in dx7_voice_patches.c: */
extern int         friendly_patch_count;
extern dx7_patch_t friendly_patches[];

dx7_patch_t dx7_voice_init_voice = {
  { 0x62, 0x63, 0x63, 0x5A, 0x63, 0x63, 0x63, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x02,
    0x00, 0x62, 0x63, 0x63, 0x5A, 0x63, 0x63, 0x63,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00,
    0x02, 0x00, 0x62, 0x63, 0x63, 0x5A, 0x63, 0x63,
    0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00,
    0x00, 0x02, 0x00, 0x62, 0x63, 0x63, 0x5A, 0x63,
    0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38,
    0x00, 0x00, 0x02, 0x00, 0x62, 0x63, 0x63, 0x5A,
    0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x38, 0x00, 0x00, 0x02, 0x00, 0x62, 0x63, 0x63,
    0x5A, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x38, 0x00, 0x63, 0x02, 0x00, 0x63, 0x63,
    0x63, 0x63, 0x32, 0x32, 0x32, 0x32, 0x00, 0x08,
    0x23, 0x00, 0x00, 0x00, 0x31, 0x18, 0x20, 0x20,
    0x20, 0x7F, 0x2D, 0x2D, 0x7E, 0x20, 0x20, 0x20  }
};

uint8_t dx7_init_performance[DX7_PERFORMANCE_SIZE] = {
    0,  0, 0, 2, 0,  0, 0,  0,
    0, 15, 1, 0, 4, 15, 2, 15,
    2,  0, 0, 0, 0,  0, 0,  0,
    0,  0, 0, 0, 0,  0, 0,  0,
    0,  0, 0, 0, 0,  0, 0,  0,
    0,  0, 0, 0, 0,  0, 0,  0,
    0,  0, 0, 0, 0,  0, 0,  0,
    0,  0, 0, 0, 0,  0, 0,  0
};

float dx7_voice_eg_rate_rise_duration[128] = {  /* generated from my f04new */

     39.638000,  37.013000,  34.388000,  31.763000,  27.210500,
     22.658000,  20.408000,  18.158000,  15.908000,  14.557000,
     13.206000,  12.108333,  11.010667,   9.913000,   8.921000,
      7.929000,   7.171333,   6.413667,   5.656000,   5.307000,
      4.958000,   4.405667,   3.853333,   3.301000,   2.889000,
      2.477000,   2.313000,   2.149000,   1.985000,   1.700500,
      1.416000,   1.274333,   1.132667,   0.991000,   0.909000,
      0.827000,   0.758000,   0.689000,   0.620000,   0.558000,
      0.496000,   0.448667,   0.401333,   0.354000,   0.332000,
      0.310000,   0.275667,   0.241333,   0.207000,   0.180950,
      0.154900,   0.144567,   0.134233,   0.123900,   0.106200,
      0.088500,   0.079667,   0.070833,   0.062000,   0.056800,
      0.051600,   0.047300,   0.043000,   0.038700,   0.034800,
      0.030900,   0.028000,   0.025100,   0.022200,   0.020815,
      0.019430,   0.017237,   0.015043,   0.012850,   0.011230,
      0.009610,   0.009077,   0.008543,   0.008010,   0.006960,
      0.005910,   0.005357,   0.004803,   0.004250,   0.003960,
      0.003670,   0.003310,   0.002950,   0.002590,   0.002420,
      0.002250,   0.002000,   0.001749,   0.001499,   0.001443,
      0.001387,   0.001242,   0.001096,   0.000951,   0.000815,
      0.000815,   0.000815,   0.000815,   0.000815,   0.000815,
      0.000815,   0.000815,   0.000815,   0.000815,   0.000815,
      0.000815,   0.000815,   0.000815,   0.000815,   0.000815,
      0.000815,   0.000815,   0.000815,   0.000815,   0.000815,
      0.000815,   0.000815,   0.000815,   0.000815,   0.000815,
      0.000815,   0.000815,   0.000815

};

float dx7_voice_eg_rate_decay_duration[128] = {  /* generated from my f06new */

    317.487000, 285.764500, 254.042000, 229.857000, 205.672000,
    181.487000, 170.154000, 158.821000, 141.150667, 123.480333,
    105.810000,  98.382500,  90.955000,  81.804667,  72.654333,
     63.504000,  58.217000,  52.930000,  48.512333,  44.094667,
     39.677000,  33.089000,  26.501000,  24.283333,  22.065667,
     19.848000,  17.881500,  15.915000,  14.389667,  12.864333,
     11.339000,  10.641000,   9.943000,   8.833333,   7.723667,
      6.614000,   6.149500,   5.685000,   5.112667,   4.540333,
      3.968000,   3.639000,   3.310000,   3.033667,   2.757333,
      2.481000,   2.069500,   1.658000,   1.518667,   1.379333,
      1.240000,   1.116500,   0.993000,   0.898333,   0.803667,
      0.709000,   0.665500,   0.622000,   0.552667,   0.483333,
      0.414000,   0.384500,   0.355000,   0.319333,   0.283667,
      0.248000,   0.228000,   0.208000,   0.190600,   0.173200,
      0.155800,   0.129900,   0.104000,   0.095400,   0.086800,
      0.078200,   0.070350,   0.062500,   0.056600,   0.050700,
      0.044800,   0.042000,   0.039200,   0.034833,   0.030467,
      0.026100,   0.024250,   0.022400,   0.020147,   0.017893,
      0.015640,   0.014305,   0.012970,   0.011973,   0.010977,
      0.009980,   0.008310,   0.006640,   0.006190,   0.005740,
      0.005740,   0.005740,   0.005740,   0.005740,   0.005740,
      0.005740,   0.005740,   0.005740,   0.005740,   0.005740,
      0.005740,   0.005740,   0.005740,   0.005740,   0.005740,
      0.005740,   0.005740,   0.005740,   0.005740,   0.005740,
      0.005740,   0.005740,   0.005740,   0.005740,   0.005740,
      0.005740,   0.005740,   0.005740

};

float dx7_voice_eg_rate_decay_percent[128] = {  /* generated from P/H/Op f07 */

    0.000010, 0.025009, 0.050008, 0.075007, 0.100006,
    0.125005, 0.150004, 0.175003, 0.200002, 0.225001,
    0.250000, 0.260000, 0.270000, 0.280000, 0.290000,
    0.300000, 0.310000, 0.320000, 0.330000, 0.340000,
    0.350000, 0.358000, 0.366000, 0.374000, 0.382000,
    0.390000, 0.398000, 0.406000, 0.414000, 0.422000,
    0.430000, 0.439000, 0.448000, 0.457000, 0.466000,
    0.475000, 0.484000, 0.493000, 0.502000, 0.511000,
    0.520000, 0.527000, 0.534000, 0.541000, 0.548000,
    0.555000, 0.562000, 0.569000, 0.576000, 0.583000,
    0.590000, 0.601000, 0.612000, 0.623000, 0.634000,
    0.645000, 0.656000, 0.667000, 0.678000, 0.689000,
    0.700000, 0.707000, 0.714000, 0.721000, 0.728000,
    0.735000, 0.742000, 0.749000, 0.756000, 0.763000,
    0.770000, 0.777000, 0.784000, 0.791000, 0.798000,
    0.805000, 0.812000, 0.819000, 0.826000, 0.833000,
    0.840000, 0.848000, 0.856000, 0.864000, 0.872000,
    0.880000, 0.888000, 0.896000, 0.904000, 0.912000,
    0.920000, 0.928889, 0.937778, 0.946667, 0.955556,
    0.964444, 0.973333, 0.982222, 0.991111, 1.000000,
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
    1.000000, 1.000000, 1.000000

};

float dx7_voice_eg_rate_rise_percent[128] = {  /* checked, matches P/H/Op f05 */

    0.000010, 0.000010, 0.000010, 0.000010, 0.000010,
    0.000010, 0.000010, 0.000010, 0.000010, 0.000010,
    0.000010, 0.000010, 0.000010, 0.000010, 0.000010,
    0.000010, 0.000010, 0.000010, 0.000010, 0.000010,
    0.000010, 0.000010, 0.000010, 0.000010, 0.000010,
    0.000010, 0.000010, 0.000010, 0.000010, 0.000010,
    0.000010, 0.000010, 0.005007, 0.010005, 0.015003,
    0.020000, 0.028000, 0.036000, 0.044000, 0.052000,
    0.060000, 0.068000, 0.076000, 0.084000, 0.092000,
    0.100000, 0.108000, 0.116000, 0.124000, 0.132000,
    0.140000, 0.150000, 0.160000, 0.170000, 0.180000,
    0.190000, 0.200000, 0.210000, 0.220000, 0.230000,
    0.240000, 0.251000, 0.262000, 0.273000, 0.284000,
    0.295000, 0.306000, 0.317000, 0.328000, 0.339000,
    0.350000, 0.365000, 0.380000, 0.395000, 0.410000,
    0.425000, 0.440000, 0.455000, 0.470000, 0.485000,
    0.500000, 0.520000, 0.540000, 0.560000, 0.580000,
    0.600000, 0.620000, 0.640000, 0.660000, 0.680000,
    0.700000, 0.732000, 0.764000, 0.796000, 0.828000,
    0.860000, 0.895000, 0.930000, 0.965000, 1.000000,
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
    1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
    1.000000, 1.000000, 1.000000

};

/* This table converts pitch envelope level parameters into the
 * actual pitch shift in semitones.  For levels [17,85], this is
 * just ((level - 50) / 32 * 12), but at the outer edges the shift
 * is exagerated to 0 = -48 and 99 => 47.624.  This is based on
 * measurements I took from my TX7. */
double dx7_voice_pitch_level_to_shift[128] = {

    -48.000000, -43.497081, -38.995993, -35.626132, -31.873615,
    -28.495880, -25.500672, -22.872620, -20.998167, -19.496961,
    -18.373238, -17.251065, -16.122139, -15.375956, -14.624487,
    -13.876516, -13.126351, -12.375000, -12.000000, -11.625000,
    -11.250000, -10.875000, -10.500000, -10.125000, -9.750000,
    -9.375000, -9.000000, -8.625000, -8.250000, -7.875000,
    -7.500000, -7.125000, -6.750000, -6.375000, -6.000000,
    -5.625000, -5.250000, -4.875000, -4.500000, -4.125000,
    -3.750000, -3.375000, -3.000000, -2.625000, -2.250000,
    -1.875000, -1.500000, -1.125000, -0.750000, -0.375000, 0.000000,
    0.375000, 0.750000, 1.125000, 1.500000, 1.875000, 2.250000,
    2.625000, 3.000000, 3.375000, 3.750000, 4.125000, 4.500000,
    4.875000, 5.250000, 5.625000, 6.000000, 6.375000, 6.750000,
    7.125000, 7.500000, 7.875000, 8.250000, 8.625000, 9.000000,
    9.375000, 9.750000, 10.125000, 10.500000, 10.875000, 11.250000,
    11.625000, 12.000000, 12.375000, 12.750000, 13.125000,
    14.251187, 15.001922, 16.126327, 17.250917, 18.375718,
    19.877643, 21.753528, 24.373913, 27.378021, 30.748956,
    34.499234, 38.627888, 43.122335, 47.624065, 48.0, 48.0, 48.0,
    48.0, 48.0, 48.0, 48.0, 48.0, 48.0, 48.0, 48.0, 48.0, 48.0,
    48.0, 48.0, 48.0, 48.0, 48.0, 48.0, 48.0, 48.0, 48.0, 48.0,
    48.0, 48.0, 48.0, 48.0, 48.0

};

char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * decode_7in6
 *
 * decode a block of base64-ish 7-bit encoded data
 */
int
decode_7in6(const char *string, int expected_length, uint8_t *data)
{
    int in, stated_length, reg, above, below, shift, out;
    char *p;
    uint8_t *tmpdata;
    int string_length = strlen(string);
    unsigned int sum = 0, stated_sum;

    if (string_length < 6)
        return 0;  /* too short */

    stated_length = strtol(string, &p, 10);
    in = p - string;
    if (in == 0 || string[in] != ' ')
        return 0;  /* stated length is bad */
    in++;
    if (stated_length != expected_length)
        return 0;

    if (string_length - in < ((expected_length * 7 + 5) / 6))
        return 0;  /* encoded data too short */

    if (!(tmpdata = (uint8_t *)malloc(expected_length)))
        return 0;  /* out of memory */

    reg = above = below = out = 0;
    while (1) {
        if (above == 7) {
            tmpdata[out] = reg >> 6;
            sum += tmpdata[out];
            reg &= 0x3f;
            above = 0;
            if (++out == expected_length)
                break;
        }
        if (below == 0) {
            if (!(p = strchr(base64, string[in]))) {
                return 0;  /* illegal character */
            }
            reg |= p - base64;
            below = 6;
            in++;
        }
        shift = 7 - above;
        if (below < shift) shift = below;
        reg <<= shift;
        above += shift;
        below -= shift;
    }

    if (string[in++] != ' ') {  /* encoded data wrong length */
        free(tmpdata);
        return 0;
    }

    stated_sum = strtol(string + in, &p, 10);
    if (sum != stated_sum) {
        free(tmpdata);
        return 0;
    }

    memcpy(data, tmpdata, expected_length);
    free(tmpdata);

    return 1;
}

/*
 * dx7_voice_copy_name
 */
void
dx7_voice_copy_name(char *name, dx7_patch_t *patch)
{
    int i;
    unsigned char c;

    for (i = 0; i < 10; i++) {
        c = (unsigned char)patch->data[i + 118];
        switch (c) {
            case  92:  c = 'Y';  break;  /* yen */
            case 126:  c = '>';  break;  /* >> */
            case 127:  c = '<';  break;  /* << */
            default:
                if (c < 32 || c > 127) c = 32;
                break;
        }
        name[i] = c;
    }
    name[10] = 0;
}

/*
 * dx7_patch_unpack
 */
void
dx7_patch_unpack(dx7_patch_t *packed_patch, uint8_t number, uint8_t *unpacked_patch)
{
    uint8_t *up = unpacked_patch,
            *pp = (uint8_t *)(&packed_patch[number]);
    int     i, j;

    /* ugly because it used to be 68000 assembly... */
    for(i = 6; i > 0; i--) {
        for(j = 11; j > 0; j--) {
            *up++ = *pp++;
        }                           /* through rd */
        *up++     = (*pp) & 0x03;   /* lc */
        *up++     = (*pp++) >> 2;   /* rc */
        *up++     = (*pp) & 0x07;   /* rs */
        *(up + 6) = (*pp++) >> 3;   /* pd */
        *up++     = (*pp) & 0x03;   /* ams */
        *up++     = (*pp++) >> 2;   /* kvs */
        *up++     = *pp++;          /* ol */
        *up++     = (*pp) & 0x01;   /* m */
        *up++     = (*pp++) >> 1;   /* fc */
        *up       = *pp++;          /* ff */
        up += 2;
    }                               /* operator done */
    for(i = 9; i > 0; i--) {
        *up++ = *pp++;
    }                               /* through algorithm */
    *up++ = (*pp) & 0x07;           /* feedback */
    *up++ = (*pp++) >> 3;           /* oks */
    for(i = 4; i > 0; i--) {
        *up++ = *pp++;
    }                               /* through lamd */
    *up++ = (*pp) & 0x01;           /* lfo ks */
    *up++ = ((*pp) >> 1) & 0x07;    /* lfo wave */
    *up++ = (*pp++) >> 4;           /* lfo pms */
    for(i = 11; i > 0; i--) {
        *up++ = *pp++;
    }
}

/*
 * dx7_patch_pack
 */
void
dx7_patch_pack(uint8_t *unpacked_patch, dx7_patch_t *packed_patch, uint8_t number)
{
    uint8_t *up = unpacked_patch,
            *pp = (uint8_t *)(&packed_patch[number]);
    int     i, j;

    /* ugly because it used to be 68000 assembly... */
    for (i = 6; i > 0; i--) {
        for (j = 11; j > 0; j--) {
            *pp++ = *up++;
        }                           /* through rd */
        *pp++ = ((*up) & 0x03) | (((*(up + 1)) & 0x03) << 2);
        up += 2;                    /* rc+lc */
        *pp++ = ((*up) & 0x07) | (((*(up + 7)) & 0x0f) << 3);
        up++;                       /* pd+rs */
        *pp++ = ((*up) & 0x03) | (((*(up + 1)) & 0x07) << 2);
        up += 2;                    /* kvs+ams */
        *pp++ = *up++;              /* ol */
        *pp++ = ((*up) & 0x01) | (((*(up + 1)) & 0x1f) << 1);
        up += 2;                    /* fc+m */
        *pp++ = *up;
        up += 2;                    /* ff */
    }                               /* operator done */
    for (i = 9; i > 0; i--) {
        *pp++ = *up++;
    }                               /* through algorithm */
    *pp++ = ((*up) & 0x07) | (((*(up + 1)) & 0x01) << 3);
    up += 2;                        /* oks+fb */
    for (i = 4; i > 0; i--) {
        *pp++ = *up++;
    }                               /* through lamd */
    *pp++ = ((*up) & 0x01) |
            (((*(up + 1)) & 0x07) << 1) |
            (((*(up + 2)) & 0x07) << 4);
    up += 3;                        /* lpms+lfw+lks */
    for (i = 11; i > 0; i--) {
        *pp++ = *up++;
    }                               /* through name */
}

/*
 * dssp_error_message
 */
char *
dssp_error_message(const char *fmt, ...)
{
    va_list args;
    char buffer[256];

    va_start(args, fmt);
    vsnprintf(buffer, 256, fmt, args);
    va_end(args);
    return strdup(buffer);
}

/*
 * hexter_data_patches_init
 *
 * initialize the patch bank, including a default set of good patches to get
 * the new user started.
 */
void
hexter_data_patches_init(dx7_patch_t *patches)
{
    /* Try loading from file first */
    if ( hexter_file_patches_init(patches) == 1 ) {
        return;
    }

    int i;

    memcpy(patches, friendly_patches, friendly_patch_count * sizeof(dx7_patch_t));

    for (i = friendly_patch_count; i < 128; i++) {
        memcpy(&patches[i], &dx7_voice_init_voice, sizeof(dx7_patch_t));
    }
}

/*
 * hexter_file_patches_init
 *
 * initialize the patch bank if HEXTER_DEFAULT_PATCH envar is set to a
 * valid patch file.
 */
int
hexter_file_patches_init(dx7_patch_t *patches)
{
    char *env = "HEXTER_DEFAULT_PATCH";
    char *val = NULL;
    int ret;
    char *message;

    val = getenv(env);

    if (!val) {
        fprintf(stderr, "%s is not defined, will load embedded friendly patches.\n",
                        env);
        return 0;
    }

    fprintf(stderr, "%s is defined, loading default patches from %s\n",
                    env, val);

    if (gui_data_load(val, 0, &message)) {

        /* successfully loaded at least one patch */
        fprintf(stderr, "Load Patch File succeeded: %s\n", message);
        gui_data_send_dirty_patch_sections();
        ret = 1;

    } else {  /* didn't load anything successfully */

        fprintf(stderr, "Load Patch File failed: %s\n", message);
        ret = 0;

    }
    free(message);

    return ret;

}


/*
 * hexter_data_performance_init
 *
 * initialize the global performance parameters.
 */
void
hexter_data_performance_init(uint8_t *performance)
{
    memcpy(performance, &dx7_init_performance, DX7_PERFORMANCE_SIZE);
}
