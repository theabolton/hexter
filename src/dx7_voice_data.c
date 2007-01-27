/* hexter DSSI software synthesizer plugin and GUI
 *
 * Copyright (C) 2004-2006 Sean Bolton and others.
 *
 * DX7 patchbank loading code by Martin Tarenskeen.
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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "hexter_types.h"
#include "dx7_voice.h"
#include "dx7_voice_data.h"

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

/* ==== patch load and save ==== */

/*
 * dx7_bulk_dump_checksum
 */
int
dx7_bulk_dump_checksum(uint8_t *data, int length)
{
    int sum = 0;
    int i;

    for (i = 0; i < length; sum -= data[i++]);
    return sum & 0x7F;
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
 * dx7_patchbank_load
 */
int
dx7_patchbank_load(const char *filename, dx7_patch_t *firstpatch,
                   int maxpatches, char **errmsg)
{
    FILE *fp;
    long filelength;
    unsigned char *raw_patch_data = NULL;
    size_t filename_length;
    int count;
    int patchstart;
    int midshift;
    int datastart;

    /* this needs to 1) open and parse the file, 2a) if it's good, copy up
     * to maxpatches patches beginning at firstpath, and not touch errmsg,
     * 2b) if it's not good, set errmsg to a malloc'd error message that
     * the caller must free. */

    if ((fp = fopen(filename, "rb")) == NULL) {
        if (errmsg) *errmsg = dssp_error_message("could not open file '%s' for reading: %s", filename, strerror(errno));
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) ||
        (filelength = ftell(fp)) == -1 ||
        fseek(fp, 0, SEEK_SET)) {
        if (errmsg) *errmsg = dssp_error_message("couldn't get length of patch file: %s", strerror(errno));
        fclose(fp);
        return 0;
    }
    if (filelength == 0) {
        if (errmsg) *errmsg = strdup("patch file has zero length");
        fclose(fp);
        return 0;
    } else if (filelength > 16384) {
        if (errmsg) *errmsg = strdup("patch file is too large");
        fclose(fp);
        return 0;
    } else if (filelength < 128) {
        if (errmsg) *errmsg = strdup ("patch file is too small");
        fclose (fp);
        return 0;
    }

    if (!(raw_patch_data = (unsigned char *)malloc(filelength))) {
        if (errmsg) *errmsg = strdup("couldn't allocate memory for raw patch file");
        fclose(fp);
        return 0;
    }

    if (fread(raw_patch_data, 1, filelength, fp) != (size_t)filelength) {
        if (errmsg) *errmsg = dssp_error_message("short read on patch file: %s", strerror(errno));
        free(raw_patch_data);
        fclose(fp);
        return 0;
    }
    fclose(fp);

    /* check if the file is a standard MIDI file */
    if (raw_patch_data[0] == 0x4d &&	/* "M" */
        raw_patch_data[1] == 0x54 &&	/* "T" */
        raw_patch_data[2] == 0x68 &&	/* "h" */
        raw_patch_data[3] == 0x64)	/* "d" */
        midshift = 2;
    else
        midshift = 0;

    /* scan SysEx or MIDI file for SysEx header */
    count = 0;
    datastart = 0;
    for (patchstart = 0; patchstart + midshift + 5 < filelength; patchstart++) {
        
        if (raw_patch_data[patchstart] == 0xf0 &&
            raw_patch_data[patchstart + 1 + midshift] == 0x43 &&
            raw_patch_data[patchstart + 2 + midshift] <= 0x0f &&
            raw_patch_data[patchstart + 3 + midshift] == 0x09 &&
            raw_patch_data[patchstart + 5 + midshift] == 0x00 &&
            patchstart + 4103 + midshift < filelength &&
            raw_patch_data[patchstart + 4103 + midshift] == 0xf7) {  /* DX7 32 voice dump */

            count = 32;
            datastart = patchstart + 6 + midshift;
            break;

        } else if (raw_patch_data[patchstart] == 0xf0 && 
                   raw_patch_data[patchstart + midshift + 1] == 0x43 && 
                   raw_patch_data[patchstart + midshift + 2] <= 0x0f && 
                   raw_patch_data[patchstart + midshift + 4] == 0x01 && 
                   raw_patch_data[patchstart + midshift + 5] == 0x1b &&
                   patchstart + midshift + 162 < filelength &&
                   raw_patch_data[patchstart + midshift + 162] == 0xf7) {  /* DX7 single voice (edit buffer) dump */

            dx7_patch_pack(raw_patch_data + patchstart + midshift + 6,
                           (dx7_patch_t *)raw_patch_data, 0);
            datastart = 0;
            count = 1;
            break;
        }
    }
            
    /* assume raw DX7/TX7 data if no SysEx header was found. */
    /* assume the user knows what he is doing ;-) */

    if (count == 0)
        count = filelength / 128;

    /* Dr.T TX7 file needs special treatment */
    filename_length = strlen (filename);
    if ((!strcmp(filename + filename_length - 4, ".TX7") ||
         !strcmp(filename + filename_length - 4, ".tx7")) && filelength == 8192) {

        count = 32;
        filelength = 4096;
    }

    /* Voyetra SIDEMAN DX/TX */
    if (filelength == 9816 &&
        raw_patch_data[0] == 0xdf &&
        raw_patch_data[1] == 0x05 &&
        raw_patch_data[2] == 0x01 && raw_patch_data[3] == 0x00) {

        count = 32;
        datastart = 0x60f;
    }

    /* Double SySex bank */
    if (filelength == 8208 &&
        raw_patch_data[4104] == 0xf0 && raw_patch_data[4104 + 4103] == 0xf7) {

        memcpy(raw_patch_data + 4102, raw_patch_data + 4110, 4096);
        count = 64;
        datastart = 6;
    }

    /* finally, copy patchdata to the right location */
    if (count > maxpatches)
        count = maxpatches;

    memcpy(firstpatch, raw_patch_data + datastart, 128 * count);
    free (raw_patch_data);
    return count;
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
    int i;

    memcpy(patches, friendly_patches, friendly_patch_count * sizeof(dx7_patch_t));

    for (i = friendly_patch_count; i < 128; i++) {
        memcpy(&patches[i], &dx7_voice_init_voice, sizeof(dx7_patch_t));
    }
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

