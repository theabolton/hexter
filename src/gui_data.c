/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2004, 2009, 2011, 2012 Sean Bolton and others.
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
#  include <config.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "hexter_types.h"
#include "hexter.h"
#include "dx7_voice.h"
#include "gui_main.h"
#include "dx7_voice_data.h"

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
    int patchstart;
    int midshift;
    int datastart;
    int i;
    int op;

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
    } else if (filelength > 2097152) {
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
    filename_length = strlen (filename);

    /* check if the file is a standard MIDI file */
    if (raw_patch_data[0] == 0x4d &&	/* "M" */
        raw_patch_data[1] == 0x54 &&	/* "T" */
        raw_patch_data[2] == 0x68 &&	/* "h" */
        raw_patch_data[3] == 0x64)	/* "d" */
        midshift = 2;
    else
        midshift = 0;

    /* scan SysEx or MIDI file for SysEx header(s) */
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

            memmove(raw_patch_data + count * DX7_VOICE_SIZE_PACKED,
                    raw_patch_data + patchstart + 6 + midshift, 4096);
            count += 32;
            patchstart += (DX7_DUMP_SIZE_VOICE_BULK - 1);

        } else if (raw_patch_data[patchstart] == 0xf0 && 
                   raw_patch_data[patchstart + midshift + 1] == 0x43 && 
                   raw_patch_data[patchstart + midshift + 2] <= 0x0f && 
                   raw_patch_data[patchstart + midshift + 4] == 0x01 && 
                   raw_patch_data[patchstart + midshift + 5] == 0x1b &&
                   patchstart + midshift + 162 < filelength &&
                   raw_patch_data[patchstart + midshift + 162] == 0xf7) {  /* DX7 single voice (edit buffer) dump */

            unsigned char buf[DX7_VOICE_SIZE_PACKED]; /* to avoid overlap in dx7_patch_pack() */

            dx7_patch_pack(raw_patch_data + patchstart + midshift + 6,
                           (dx7_patch_t *)buf, 0);
            memcpy(raw_patch_data + count * DX7_VOICE_SIZE_PACKED,
                   buf, DX7_VOICE_SIZE_PACKED);

            count += 1;
            patchstart += (DX7_DUMP_SIZE_VOICE_SINGLE - 1);
        }
    }
            
    /* assume raw DX7/TX7 data if no SysEx header was found. */
    /* assume the user knows what he is doing ;-) */

    if (count == 0)
        count = filelength / DX7_VOICE_SIZE_PACKED;

    /* Dr.T and Steinberg TX7 file needs special treatment */
    if ((!strcmp(filename + filename_length - 4, ".TX7") ||
         !strcmp(filename + filename_length -4, ".SND") ||
         !strcmp(filename + filename_length -4, ".tx7") ||
         !strcmp(filename + filename_length - 4, ".snd")) && filelength == 8192) {

        count = 32;
        filelength = 4096;
    }

    /* Transform XSyn file also needs special treatment */
    if ((!strcmp(filename + filename_length - 4, ".BNK") ||
         !strcmp(filename + filename_length - 4, ".bnk")) && filelength == 8192) {

        for (i=0; i<32; i++)
        {
            memmove(raw_patch_data + 128*i, raw_patch_data + 256*i, 128);
        }
        count = 32;
        filelength = 4096;
    }

    /* Steinberg Synthworks DX7 SND */
    if ((!strcmp (filename + filename_length - 4, ".SND") ||
         !strcmp (filename + filename_length - 4, ".snd")) && filelength == 5216) {

        count = 32;
        filelength = 4096;
    }

    /* Voyetra SIDEMAN DX/TX
     * Voyetra Patchmaster DX7/TX7 */
    if ((filelength == 9816 || filelength == 5663) &&
        raw_patch_data[0] == 0xdf &&
        raw_patch_data[1] == 0x05 &&
        raw_patch_data[2] == 0x01 && raw_patch_data[3] == 0x00) {

        count = 32;
        datastart = 0x60f;
    }

    /* Yamaha DX200 editor .DX2 */
    if ((!strcmp (filename + filename_length - 4, ".DX2") ||
         !strcmp (filename + filename_length - 4, ".dx2"))
        && filelength == 326454)
      {
          memmove (raw_patch_data + 16384, raw_patch_data + 34, 128 * 381);
          for (count = 0; count < 128; count++)
            {
                for (op = 0; op < 6; op++)
                  {
                      for (i = 0; i < 8; i++)
                        {
                            raw_patch_data[17 * (5 - op) + i + 128 * count] =
                                raw_patch_data[16384 + 35 * op + 76 + i + 381 * count];
                        }
                      raw_patch_data[17 * (5 - op) + 8 + 128 * count] =
                          raw_patch_data[16384 + 35 * op + 84 + 381 * count] - 21;
                      raw_patch_data[17 * (5 - op) + 9 + 128 * count] =
                          raw_patch_data[16384 + 35 * op + 87 + 381 * count];
                      raw_patch_data[17 * (5 - op) + 10 + 128 * count] =
                          raw_patch_data[16384 + 35 * op + 88 + 381 * count];
                      raw_patch_data[17 * (5 - op) + 11 + 128 * count] =
                          raw_patch_data[16384 + 35 * op + 85 + 381 * count] +
                          raw_patch_data[16384 + 35 * op + 86 + 381 * count] * 4;
                      raw_patch_data[17 * (5 - op) + 12 + 128 * count] =
                          raw_patch_data[16384 + 35 * op + 89 + 381 * count] +
                          raw_patch_data[16384 + 35 * op + 75 + 381 * count] * 8;
                      if (raw_patch_data[16384 + 35 * op + 71 + 381 * count] > 3)
                          raw_patch_data[16384 + 35 * op + 71 + 381 * count] = 3;
                      raw_patch_data[17 * (5 - op) + 13 + 128 * count] =
                          raw_patch_data[16384 + 35 * op + 71 + 381 * count] / 2 +
                          raw_patch_data[16384 + 35 * op + 91 + 381 * count] * 4;
                      raw_patch_data[17 * (5 - op) + 14 + 128 * count] =
                          raw_patch_data[16384 + 35 * op + 90 + 381 * count];
                      raw_patch_data[17 * (5 - op) + 15 + 128 * count] =
                          raw_patch_data[16384 + 35 * op + 72 + 381 * count] +
                          raw_patch_data[16384 + 35 * op + 73 + 381 * count] * 2;
                      raw_patch_data[17 * (5 - op) + 16 + 128 * count] =
                          raw_patch_data[16384 + 35 * op + 74 + 381 * count];
                  }
                for (i = 0; i < 4; i++)
                  {
                      raw_patch_data[102 + i + 128 * count] =
                          raw_patch_data[16384 + 26 + i + 381 * count];
                  }
                for (i = 0; i < 4; i++)
                  {
                      raw_patch_data[106 + i + 128 * count] =
                          raw_patch_data[16384 + 32 + i + 381 * count];
                  }
                raw_patch_data[110 + 128 * count] =
                    raw_patch_data[16384 + 17 + 381 * count];
                raw_patch_data[111 + 128 * count] =
                    raw_patch_data[16384 + 18 + 381 * count] +
                    raw_patch_data[16384 + 38 + 381 * count] * 8;
                for (i = 0; i < 4; i++)
                  {
                      raw_patch_data[112 + i + 128 * count] =
                          raw_patch_data[16384 + 20 + i + 381 * count];
                  }
                raw_patch_data[116 + 128 * count] =
                    raw_patch_data[16384 + 24 + 381 * count] +
                    raw_patch_data[16384 + 19 + 381 * count] * 2 +
                    raw_patch_data[16384 + 25 + 381 * count] * 16;
                raw_patch_data[117 + 128 * count] =
                    raw_patch_data[16384 + 37 + 381 * count] - 36;
                for (i = 0; i < 10; i++)
                  {
                      raw_patch_data[118 + i + 128 * count] =
                          raw_patch_data[16384 + i + 381 * count];
                  }
            }

          count = 128;
          filelength = 16384;
          datastart = 0;

      }

    /* finally, copy patchdata to the right location */
    if (count > maxpatches)
        count = maxpatches;

    memcpy(firstpatch, raw_patch_data + datastart, 128 * count);
    free (raw_patch_data);
    return count;
}

/*
 * encode_7in6
 *
 * encode a block of 7-bit data, in base64-ish style
 */
char *
encode_7in6(uint8_t *data, int length)
{
    char *buffer;
    int in, reg, above, below, shift, out;
    int outchars = (length * 7 + 5) / 6;
    unsigned int sum = 0;

    if (!(buffer = (char *)malloc(25 + outchars)))
        return NULL;

    out = snprintf(buffer, 12, "%d ", length);

    in = reg = above = below = 0;
    while (outchars) {
        if (above == 6) {
            buffer[out] = base64[reg >> 7];
            reg &= 0x7f;
            above = 0;
            out++;
            outchars--;
        }
        if (below == 0) {
            if (in < length) {
                reg |= data[in] & 0x7f;
                sum += data[in];
            }
            below = 7;
            in++;
        }
        shift = 6 - above;
        if (below < shift) shift = below;
        reg <<= shift;
        above += shift;
        below -= shift;
    }

    snprintf(buffer + out, 12, " %d", sum);

    return buffer;
}

void
gui_data_patches_init(void)
{
    if (!patches)
        patches = (dx7_patch_t *)malloc(128 * sizeof(dx7_patch_t));
    hexter_data_patches_init(patches);
    patch_section_dirty[0] = 0;
    patch_section_dirty[1] = 0;
    patch_section_dirty[2] = 0;
    patch_section_dirty[3] = 0;
}

void
gui_data_patches_free(void)
{
    if (patches) free(patches);
}

void
gui_data_mark_dirty_patch_sections(int start_patch, int end_patch)
{
    int i, block;
    for (i = start_patch; i <= end_patch; ) {
        block = i >> 5;
        patch_section_dirty[block] = 1;
        i = (block + 1) << 5;
    }
}

void
gui_data_send_dirty_patch_sections(void)
{
    int block;
    char *p;
    char key[9];
    for (block = 0; block < 4; block++) {
        if (patch_section_dirty[block]) {
            if ((p = encode_7in6((uint8_t *)&patches[block << 5],
                                 32 * DX7_VOICE_SIZE_PACKED))) {
                snprintf(key, 9, "patches%d", block);
                lo_send(osc_host_address, osc_configure_path, "ss", key, p);
                free(p);
                patch_section_dirty[block] = 0;
            }
        }
    }
}

int
gui_data_save(char *filename, int type, int start, int end, char **message)
{
    FILE *fh;
    char buffer[20];
    int i, j;
    uint8_t *patch;
    int checksum = 0;

    GUIDB_MESSAGE(DB_IO, " gui_data_save: attempting to save '%s'\n", filename);

    if ((fh = fopen(filename, "wb")) == NULL) {
        if (message) *message = dssp_error_message("could not open file '%s'for writing", filename);
        return 0;
    }

    if (type == 0) { /* sys-ex */
        buffer[0] = 0xf0;
        buffer[1] = 0x43;
        buffer[2] = 0x00;
        buffer[3] = 0x09;
        buffer[4] = 0x10;
        buffer[5] = 0x00;
        if (fwrite(buffer, 1, 6, fh) != 6) {
            fclose(fh);
            if (message) *message = dssp_error_message("error while writing sys-ex header: %s", strerror(errno));
            return 0;
        }
    }

    for (i = start; i <= end; i++) {
        patch = (uint8_t *)&patches[i];

        for (j = 0; j < DX7_VOICE_SIZE_PACKED; checksum -= patch[j++]);

        if (fwrite(patch, 1, DX7_VOICE_SIZE_PACKED, fh) != DX7_VOICE_SIZE_PACKED) {
            fclose(fh);
            if (message) *message = dssp_error_message("error while writing file: %s", strerror(errno));
            return 0;
        }
    }

    if (type == 0) { /* sys-ex */
        buffer[0] = checksum & 0x7f;
        buffer[1] = 0xf7;
        if (fwrite(buffer, 1, 2, fh) != 2) {
            fclose(fh);
            if (message) *message = dssp_error_message("error while writing sys-ex footer: %s", strerror(errno));
            return 0;
        }
    }

    fclose(fh);

    if (message) {
        *message = dssp_error_message("wrote %d patches", end - start + 1);
    }
    return 1;
}

/*
 * gui_data_load
 */
int
gui_data_load(const char *filename, int position, char **message)
{
    int tmpcount = 0;

    GUIDB_MESSAGE(DB_IO, " gui_data_load: attempting to load '%s'\n", filename);

    tmpcount = dx7_patchbank_load(filename, &patches[position], 128 - position,
                                  message);

    if (!tmpcount) {
        /* no patches loaded (message was set by dx7_patchbank_load()) */
        return 0;
    }
    gui_data_mark_dirty_patch_sections(position, position + tmpcount - 1);

    if (message) {
        *message = dssp_error_message("loaded %d patches", tmpcount);
    }

    return tmpcount;
}

void
gui_data_send_edit_buffer(void)
{
    char *p;

    if ((p = encode_7in6((uint8_t *)&edit_buffer, sizeof(edit_buffer)))) {
        lo_send(osc_host_address, osc_configure_path, "ss", "edit_buffer", p);
        free(p);
    }
}

void
gui_data_send_edit_buffer_off(void)
{
    lo_send(osc_host_address, osc_configure_path, "ss", "edit_buffer", "off");
}

void
gui_data_send_performance_buffer(uint8_t *performance_buffer)
{
    char *p;

    if ((p = encode_7in6(performance_buffer, DX7_PERFORMANCE_SIZE))) {
        lo_send(osc_host_address, osc_configure_path, "ss", "performance", p);
        free(p);
    }
}

