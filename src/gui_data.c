/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2004 Sean Bolton and others.
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

/* ==== DX7 MIDI system exclusive message handling ==== */

void
gui_data_set_up_edit_buffer(int copy_current_program)
{
    if (!edit_buffer_active) {
        edit_buffer.program = current_program;
        if (copy_current_program) {
            dx7_patch_unpack(patches, current_program, edit_buffer.buffer);
        }
        edit_buffer_active = 1;
    }
}

int
gui_data_sysex_parse(unsigned int length, unsigned char* data)
{
    if (length < 6 || data[1] != 0x43)  /* too short, or not Yamaha */
        return 0;

    if ((data[2] & 0x0f) != edit_receive_channel)  /* wrong MIDI channel */
        return 0;

    if ((data[2] & 0xf0) == 0x00 && data[3] == 0x00 &&  /* DX7 single voice dump */
        data[4] == 0x01 && data[5] == 0x1b) {

        if (length != DX7_DUMP_SIZE_SINGLE ||
            data[DX7_DUMP_SIZE_SINGLE - 1] != 0xf7) {
            GUIDB_MESSAGE(DB_IO, " gui_data_sysex_parse: badly formatted DX7 single voice dump!\n");
            return 0;
        }

        if (dx7_bulk_dump_checksum(&data[6], DX7_VOICE_SIZE_UNPACKED) !=
            data[DX7_DUMP_SIZE_SINGLE - 2]) {
            GUIDB_MESSAGE(DB_IO, " gui_data_sysex_parse: DX7 single voice dump with bad checksum!\n");
            return 0;
        }

        GUIDB_MESSAGE(DB_IO, " gui_data_sysex_parse: DX7 single voice dump received\n");

        gui_data_set_up_edit_buffer(0);
        memcpy(&edit_buffer.buffer, data + 6, DX7_VOICE_SIZE_UNPACKED);

        return 1;

    } else if ((data[2] & 0xf0) == 0x10 &&    /* DX7 voice parameter change (g = 0)*/
               (data[3] & 0xfc) == 0x00) {

        int i;

        if (length != 7 || data[6] != 0xf7) {
            GUIDB_MESSAGE(DB_IO, " gui_data_sysex_parse: badly formatted DX7 voice parameter change!\n");
            return 0;
        }

        i = ((data[3] & 0x03) << 7) + (data[4] & 0x7f);

        if (i >= DX7_VOICE_SIZE_UNPACKED) {
            GUIDB_MESSAGE(DB_IO, " gui_data_sysex_parse: out-of-range DX7 voice parameter change!\n");
            return 0;
        }

        GUIDB_MESSAGE(DB_IO, " gui_data_sysex_parse: DX7 voice parameter change #%d => %d\n", i, data[5] & 0x7f);

        gui_data_set_up_edit_buffer(1);
        edit_buffer.buffer[i] = data[5] & 0x7f;

        return 1;
    }

    /* nope, don't know what this is */
    return 0;
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
gui_data_clear_edit_buffer(void)
{
    lo_send(osc_host_address, osc_configure_path, "ss", "edit_buffer", "off");
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

