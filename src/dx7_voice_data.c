/* hexter DSSI software synthesizer plugin and GUI
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hexter_types.h"
#include "dx7_voice.h"
#include "dx7_voice_data.h"

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
    char *p, *tmpdata;
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

    /* this needs to 1) open and parse the file, 2a) if it's good, copy up
     * to maxpatches patches beginning at firstpath, and not touch errmsg,
     * 2b) if it's not good, set errmsg to a malloc'd error message that
     * the caller must free. */

    if ((fp = fopen(filename, "rb")) == NULL) {
        // !FIX! FLUID_LOG(FLUID_ERR, "Couldn't open patch bank file '%s': %s", filename, strerror(errno));
        if (errmsg) *errmsg = strdup("could not open file for reading");
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) ||
        (filelength = ftell(fp)) == -1 ||
        fseek(fp, 0, SEEK_SET)) {
        // !FIX! FLUID_LOG(FLUID_ERR, "Couldn't get length of patch bank file: %s", strerror(errno));
        if (errmsg) *errmsg = strdup("couldn't get length of patch file");
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
    }

    if (!(raw_patch_data = (unsigned char *)malloc(filelength))) {
        if (errmsg) *errmsg = strdup("couldn't allocate memory for raw patch file");
        fclose(fp);
        return 0;
    }

    if (fread(raw_patch_data, 1, filelength, fp) != (size_t)filelength) {
        // !FIX! FLUID_LOG(FLUID_ERR, "Short read on patch bank file: %s", strerror(errno));
        if (errmsg) *errmsg = strdup("short read on patch file");
        free(raw_patch_data);
        fclose(fp);
        return 0;
    }
    fclose(fp);

    /* figure out what kind of file it is */
    filename_length = strlen(filename);
    if (filename_length > 4 &&
               !strcmp(filename + filename_length - 4, ".dx7") &&
               filelength % DX7_VOICE_SIZE_PACKED == 0) {  /* It's a raw DX7 patch bank */

        count = filelength / DX7_VOICE_SIZE_PACKED;
        if (count > maxpatches)
            count = maxpatches;
        memcpy(firstpatch, raw_patch_data, count * DX7_VOICE_SIZE_PACKED);

    } else if (filelength > 6 &&
               raw_patch_data[0] == 0xf0 &&
               raw_patch_data[1] == 0x43 &&
               (raw_patch_data[2] & 0xf0) == 0x00 &&
               raw_patch_data[3] == 0x09 &&
               (raw_patch_data[4] == 0x10 || raw_patch_data[4] == 0x20) &&  /* 0x10 is actual, 0x20 matches typo in manual */
               raw_patch_data[5] == 0x00) {  /* It's a DX7 sys-ex 32 voice dump */

        if (filelength != DX7_DUMP_SIZE_BULK ||
            raw_patch_data[DX7_DUMP_SIZE_BULK - 1] != 0xf7) {

            if (errmsg) *errmsg = strdup("badly formatted DX7 32 voice dump!");
            count = 0;

#ifdef CHECKSUM_PATCH_FILES_ON_LOAD
        } else if (dx7_bulk_dump_checksum(&raw_patch_data[6],
                                          DX7_VOICE_SIZE_PACKED * 32) !=
                   raw_patch_data[DX7_DUMP_SIZE_BULK - 2]) {

            if (errmsg) *errmsg = strdup("DX7 32 voice dump with bad checksum!");
            count = 0;

#endif
        } else {

            count = 32;
            if (count > maxpatches)
                count = maxpatches;
            memcpy(firstpatch, raw_patch_data + 6, count * DX7_VOICE_SIZE_PACKED);

        }
    } else {

        /* unsuccessful load */
        if (errmsg) *errmsg = strdup("unknown patch bank file format!");
        count = 0;

    }

    free(raw_patch_data);
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

