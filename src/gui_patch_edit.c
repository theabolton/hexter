/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2011, 2012, 2013, 2018 Sean Bolton.
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

#include <string.h>

#include <gtk/gtk.h>

#include "hexter_types.h"
#include "dx7_voice_data.h"
#include "gui_main.h"
#include "gui_data.h"
#include "gui_callbacks.h"
#include "gui_interface.h"
#include "gui_patch_edit.h"

GtkWidget *editor_window;
GtkWidget *editor_status_label;
GtkWidget *editor_name_entry;
GtkWidget *editor_discard_button;
GtkWidget *editor_save_button;
GtkWidget *editor_test_note_button;

GtkObject *edit_adj[DX7_VOICE_PARAMETERS - 1];  /* omitting name */

static unsigned char pept_maximum[PEPT_COUNT] = {
    0, 0, 99, 7, 1, 31, 99, 14, 3, 31, 3, 99, 48, 1, 5, 99, 99
};

typedef struct _pe_voice_parameter_t {
    char type;
} pe_voice_parameter_t;

static pe_voice_parameter_t pe_voice_parameter[DX7_VOICE_PARAMETERS - 1 /* omitting name */] = {
    { PEPT_Env        }, /*   0 R1 */
    { PEPT_Env        }, /*   1 R2 */
    { PEPT_Env        }, /*   2 R3 */
    { PEPT_Env        }, /*   3 R4 */
    { PEPT_Env        }, /*   4 L1 */
    { PEPT_Env        }, /*   5 L2 */
    { PEPT_Env        }, /*   6 L3 */
    { PEPT_Env        }, /*   7 L4 */
    { PEPT_BkPt       }, /*   8    */
    { PEPT_Scal99     }, /*   9 Left Depth */
    { PEPT_Scal99     }, /*  10 Right Depth */
    { PEPT_Curve      }, /*  11 Left Curve */
    { PEPT_Curve      }, /*  12 Right Curve */
    { PEPT_0_7        }, /*  13 RS */
    { PEPT_0_3        }, /*  14 AMS */
    { PEPT_0_7        }, /*  15 VS */
    { PEPT_Scal99     }, /*  16 OL */
    { PEPT_Mode       }, /*  17    */
    { PEPT_FC         }, /*  18 FC */
    { PEPT_FF         }, /*  19 FF */
    { PEPT_Detune     }, /*  20 Detune */

    { PEPT_Env        }, /*  21 R1 */
    { PEPT_Env        }, /*  22 R2 */
    { PEPT_Env        }, /*  23 R3 */
    { PEPT_Env        }, /*  24 R4 */
    { PEPT_Env        }, /*  25 L1 */
    { PEPT_Env        }, /*  26 L2 */
    { PEPT_Env        }, /*  27 L3 */
    { PEPT_Env        }, /*  28 L4 */
    { PEPT_BkPt       }, /*  29    */
    { PEPT_Scal99     }, /*  30 Left Depth */
    { PEPT_Scal99     }, /*  31 Right Depth */
    { PEPT_Curve      }, /*  32 Left Curve */
    { PEPT_Curve      }, /*  33 Right Curve */
    { PEPT_0_7        }, /*  34 RS */
    { PEPT_0_3        }, /*  35 AMS */
    { PEPT_0_7        }, /*  36 VS */
    { PEPT_Scal99     }, /*  37 OL */
    { PEPT_Mode       }, /*  38    */
    { PEPT_FC         }, /*  39 FC */
    { PEPT_FF         }, /*  40 FF */
    { PEPT_Detune     }, /*  41 Detune */

    { PEPT_Env        }, /*  42 R1 */
    { PEPT_Env        }, /*  43 R2 */
    { PEPT_Env        }, /*  44 R3 */
    { PEPT_Env        }, /*  45 R4 */
    { PEPT_Env        }, /*  46 L1 */
    { PEPT_Env        }, /*  47 L2 */
    { PEPT_Env        }, /*  48 L3 */
    { PEPT_Env        }, /*  49 L4 */
    { PEPT_BkPt       }, /*  50    */
    { PEPT_Scal99     }, /*  51 Left Depth */
    { PEPT_Scal99     }, /*  52 Right Depth */
    { PEPT_Curve      }, /*  53 Left Curve */
    { PEPT_Curve      }, /*  54 Right Curve */
    { PEPT_0_7        }, /*  55 RS */
    { PEPT_0_3        }, /*  56 AMS */
    { PEPT_0_7        }, /*  57 VS */
    { PEPT_Scal99     }, /*  58 OL */
    { PEPT_Mode       }, /*  59    */
    { PEPT_FC         }, /*  60 FC */
    { PEPT_FF         }, /*  61 FF */
    { PEPT_Detune     }, /*  62 Detune */

    { PEPT_Env        }, /*  63 R1 */
    { PEPT_Env        }, /*  64 R2 */
    { PEPT_Env        }, /*  65 R3 */
    { PEPT_Env        }, /*  66 R4 */
    { PEPT_Env        }, /*  67 L1 */
    { PEPT_Env        }, /*  68 L2 */
    { PEPT_Env        }, /*  69 L3 */
    { PEPT_Env        }, /*  70 L4 */
    { PEPT_BkPt       }, /*  71    */
    { PEPT_Scal99     }, /*  72 Left Depth */
    { PEPT_Scal99     }, /*  73 Right Depth */
    { PEPT_Curve      }, /*  74 Left Curve */
    { PEPT_Curve      }, /*  75 Right Curve */
    { PEPT_0_7        }, /*  76 RS */
    { PEPT_0_3        }, /*  77 AMS */
    { PEPT_0_7        }, /*  78 VS */
    { PEPT_Scal99     }, /*  79 OL */
    { PEPT_Mode       }, /*  80    */
    { PEPT_FC         }, /*  81 FC */
    { PEPT_FF         }, /*  82 FF */
    { PEPT_Detune     }, /*  83 Detune */

    { PEPT_Env        }, /*  84 R1 */
    { PEPT_Env        }, /*  85 R2 */
    { PEPT_Env        }, /*  86 R3 */
    { PEPT_Env        }, /*  87 R4 */
    { PEPT_Env        }, /*  88 L1 */
    { PEPT_Env        }, /*  89 L2 */
    { PEPT_Env        }, /*  90 L3 */
    { PEPT_Env        }, /*  91 L4 */
    { PEPT_BkPt       }, /*  92    */
    { PEPT_Scal99     }, /*  93 Left Depth */
    { PEPT_Scal99     }, /*  94 Right Depth */
    { PEPT_Curve      }, /*  95 Left Curve */
    { PEPT_Curve      }, /*  96 Right Curve */
    { PEPT_0_7        }, /*  97 RS */
    { PEPT_0_3        }, /*  98 AMS */
    { PEPT_0_7        }, /*  99 VS */
    { PEPT_Scal99     }, /* 100 OL */
    { PEPT_Mode       }, /* 101    */
    { PEPT_FC         }, /* 102 FC */
    { PEPT_FF         }, /* 103 FF */
    { PEPT_Detune     }, /* 104 Detune */

    { PEPT_Env        }, /* 105 R1 */
    { PEPT_Env        }, /* 106 R2 */
    { PEPT_Env        }, /* 107 R3 */
    { PEPT_Env        }, /* 108 R4 */
    { PEPT_Env        }, /* 109 L1 */
    { PEPT_Env        }, /* 110 L2 */
    { PEPT_Env        }, /* 111 L3 */
    { PEPT_Env        }, /* 112 L4 */
    { PEPT_BkPt       }, /* 113    */
    { PEPT_Scal99     }, /* 114 Left Depth */
    { PEPT_Scal99     }, /* 115 Right Depth */
    { PEPT_Curve      }, /* 116 Left Curve */
    { PEPT_Curve      }, /* 117 Right Curve */
    { PEPT_0_7        }, /* 118 RS */
    { PEPT_0_3        }, /* 119 AMS */
    { PEPT_0_7        }, /* 120 VS */
    { PEPT_Scal99     }, /* 121 OL */
    { PEPT_Mode       }, /* 122    */
    { PEPT_FC         }, /* 123 FC */
    { PEPT_FF         }, /* 124 FF */
    { PEPT_Detune     }, /* 125 Detune */

    { PEPT_Env        }, /* 126 Pitch Envelope R1 */
    { PEPT_Env        }, /* 127 */
    { PEPT_Env        }, /* 128 */
    { PEPT_Env        }, /* 129 */
    { PEPT_Env        }, /* 130 */
    { PEPT_Env        }, /* 131 */
    { PEPT_Env        }, /* 132 */
    { PEPT_Env        }, /* 133 P L4 */
    { PEPT_Alg        }, /* 134 Alg */
    { PEPT_0_7        }, /* 135 Feedback */
    { PEPT_OnOff      }, /* 136 Osc Key Sync */
    { PEPT_0_99       }, /* 137 LFO Speed */
    { PEPT_0_99       }, /* 138 Delay */
    { PEPT_0_99       }, /* 139 PMD */
    { PEPT_0_99       }, /* 140 AMD */
    { PEPT_OnOff      }, /* 141 Sync */
    { PEPT_LFOWave    }, /* 142 Wave */
    { PEPT_0_7        }, /* 143 PMS */
    { PEPT_Trans      }  /* 144 */

    // { PEPT_Name       }, /* 145 Name */
};

void
patch_edit_copy_name_to_utf8(char *buffer, uint8_t *patch, int packed)
{
    int i, j;
    int offset = packed ? DX7_VOICE_SIZE_PACKED - 10 : DX7_VOICE_PARAMETERS - 1;

    for (i = 0, j = 0; i < 10; i++, j++) {
        /* translate DX7-ASCII to UTF-8 */
        buffer[j] = patch[offset + i];
        switch (buffer[j]) {
          case  92:  buffer[j++] = 0xC2; buffer[j] = 0xA5;  break;  /* yen C2 A5 */
          case 126:  buffer[j++] = 0xC2; buffer[j] = 0xBB;  break;  /* >>  C2 BB */
          case 127:  buffer[j++] = 0xC2; buffer[j] = 0xAB;  break;  /* <<  C2 AB */
          default:
            if (buffer[j] < 32 || buffer[j] > 127) {
                buffer[j++] = 0xC2; buffer[j] = 0xB7;  /* bullet C2 B7 */
            }
            break;
        }
    }
    buffer[j] = 0;
}

void
patch_edit_on_name_entry_changed(GtkEditable *editable, gpointer user_data)
{
    const char *p0, *p;
    int i;

    /* GUIDB_MESSAGE(DB_GUI, ": patch_edit_on_name_entry_changed called\n"); */

    /* copy name entry to patch */
    p0 = gtk_entry_get_text(GTK_ENTRY(editor_name_entry));
    for (i = 0; i < 10; i++) {
        /* translate UTF-8 to DX7-ASCII */
        char c = ' ';

        p = *p0 ? g_utf8_next_char(p0) : p0;
        if (p - p0 == 3) {
            if (!strncmp(p0, "•", 3)) c =  31;       /* 0 to 31 block graphics */
        } else if (p - p0 == 2) {
            if      (!strncmp(p0, "·", 2)) c =  31;  /* 0 to 31 block graphics */
            else if (!strncmp(p0, "¥", 2)) c =  92;  /* yen */
            else if (!strncmp(p0, "«", 2)) c = 127;  /* << */
            else if (!strncmp(p0, "»", 2)) c = 126;  /* >> */
        } else if (p - p0 == 1) {
            c = *p0;
        }
        edit_buffer.voice[i + DX7_VOICE_PARAMETERS - 1] = c;
        p0 = p;
    }
    edit_buffer_active = TRUE;
    gui_data_send_edit_buffer();
    patch_edit_update_status();
}

void
patch_edit_on_edit_adj_changed(GtkAdjustment *adj, gpointer data)
{
    float value = adj->value;
    int offset = GPOINTER_TO_INT(data);

    /* GUIDB_MESSAGE(DB_GUI, ": patch_edit_on_edit_adj_changed: offset %d, value %f\n", offset, value); */
    edit_buffer.voice[offset] = value;
    edit_buffer_active = TRUE;
    gui_data_send_edit_buffer();
    patch_edit_update_status();
}

void
patch_edit_create_edit_adjs(void)
{
    int i;

    /* create the adjustments */
    for (i = 0; i < DX7_VOICE_PARAMETERS - 1 /* omitting name */; i++) {
        int max = pept_maximum[(int)pe_voice_parameter[i].type];
        edit_adj[i] = gtk_adjustment_new(0, 0, max, 1, (max > 10) ? 10 : 1, 0);
    }
}

/* gtk_spin_button_new() (wrongly) forces the emission of "value-changed" by the
 * adjustment it attaches to, so we don't connect the edit_buffer to the
 * edit_adj[] adjustments at least until the widgy editor is built.
 */
void
patch_edit_connect_to_model(void)
{
    int i;

    for (i = 0; i < DX7_VOICE_PARAMETERS - 1 /* omitting name */; i++) {
        g_signal_connect (G_OBJECT(edit_adj[i]), "value-changed",
                          G_CALLBACK (patch_edit_on_edit_adj_changed), GINT_TO_POINTER(i));
    }
}

void
patch_edit_update_status(void)
{
    char buf[60];
    int editable;

    if (!edit_buffer_active) {
       snprintf(buf, 60, "Editing patch %d (currently unchanged)", edit_buffer.program);
       gtk_label_set_text (GTK_LABEL (editor_status_label), buf);
        editable = TRUE;
    } else {
        if (current_program != edit_buffer.program) {
            /* this should happen only transiently at startup, or as the result of a buggy host */
            /* GUIDB_MESSAGE(DB_GUI, ": edit buffer sync error, current_program %d, edit_buffer.program %d\n",
             *                current_program, edit_buffer.program); */
            gtk_label_set_text (GTK_LABEL (editor_status_label), "(edit buffer sync error, select program to clear)");
            editable = FALSE;
        } else {
            snprintf(buf, 60, "Editing patch %d (changes have not been saved)", edit_buffer.program);
            gtk_label_set_text (GTK_LABEL (editor_status_label), buf);
            editable = TRUE;
        }
    }
    gtk_widget_set_sensitive (editor_name_entry, editable);
    gtk_widget_set_sensitive (widgy_widget, editable);
    gtk_widget_set_sensitive (retro_widget, editable);

    gtk_widget_set_sensitive (editor_discard_button, edit_buffer_active);
}

void
patch_edit_update_editors(void)
{
    int i;
    char name[21];

    /* update all the adjustments */
    for (i = 0; i < DX7_VOICE_PARAMETERS - 1 /* omitting name */; i++) {
        GTK_ADJUSTMENT(edit_adj[i])->value = (float)edit_buffer.voice[i];
        /* temporarily block signal to model, then update adjustment */
        g_signal_handlers_block_by_func(G_OBJECT(edit_adj[i]), patch_edit_on_edit_adj_changed,
                                        GINT_TO_POINTER(i));
        gtk_adjustment_value_changed(GTK_ADJUSTMENT(edit_adj[i]));
        g_signal_handlers_unblock_by_func(G_OBJECT(edit_adj[i]), patch_edit_on_edit_adj_changed,
                                          GINT_TO_POINTER(i));
    }
    /* update name entry */
    patch_edit_copy_name_to_utf8(name, edit_buffer.voice, FALSE);
    /* temporarily block signal to model, then update entry text */
    g_signal_handlers_block_by_func(G_OBJECT(editor_name_entry), patch_edit_on_name_entry_changed,
                                    NULL);
    gtk_entry_set_text(GTK_ENTRY(editor_name_entry), name);
    g_signal_handlers_unblock_by_func(G_OBJECT(editor_name_entry), patch_edit_on_name_entry_changed,
                                      NULL);

    patch_edit_update_status();
}

int
patch_edit_get_edit_parameter(int offset)
{
    if (offset < 0 || offset >= DX7_VOICE_PARAMETERS - 1)
        return 0; /* hmmm. */
    else
        return (int)GTK_ADJUSTMENT(edit_adj[offset])->value;
}

char *
patch_edit_NoteText(int note)
{
    static char _NoteText[5]={'x','x','x','x','\0'};

    _NoteText[0]=(" C D  F G A ")[note%12];
    _NoteText[1]=("C#D#EF#G#A#B")[note%12];
    _NoteText[2]=("--012345678")[note/12];
    _NoteText[3]=("21         ")[note/12];

    return _NoteText;
}

unsigned short peFFF[100] = {
    1000, 1023, 1047, 1072, 1096, 1122, 1148, 1175, 1202, 1230,
    1259, 1288, 1318, 1349, 1380, 1413, 1445, 1479, 1514, 1549,
    1585, 1622, 1660, 1698, 1738, 1778, 1820, 1862, 1905, 1950,
    1995, 2042, 2089, 2138, 2188, 2239, 2291, 2344, 2399, 2455,
    2512, 2570, 2630, 2692, 2754, 2818, 2884, 2951, 3020, 3090,
    3162, 3236, 3311, 3388, 3467, 3548, 3631, 3715, 3802, 3890,
    3981, 4074, 4169, 4266, 4365, 4467, 4571, 4677, 4786, 4898,
    5012, 5129, 5248, 5370, 5495, 5623, 5754, 5888, 6026, 6166,
    6310, 6457, 6607, 6761, 6918, 7079, 7244, 7413, 7586, 7762,
    7943, 8128, 8318, 8511, 8710, 8913, 9120, 9333, 9550, 9772,
};

void
patch_edit_on_mode_change(GtkComboBox *combo, gpointer user_data)
{
    if (gtk_combo_box_get_active(combo) == 1) {
        gtk_widget_hide(widgy_widget);
        gtk_widget_show(retro_widget);
        gtk_widget_grab_focus(retro_widget);
        gtk_window_resize(GTK_WINDOW(editor_window), 1, 1); /* let window get smaller */
    } else {
        gtk_widget_hide(retro_widget);
        gtk_widget_show(widgy_widget);
    }
}

void
on_editor_save_button_press(GtkWidget *widget, gpointer data)
{
    GUIDB_MESSAGE(DB_GUI, ": on_editor_save_button_press called\n");

    (GTK_ADJUSTMENT(edit_save_position_spin_adj))->value = (double)edit_buffer.program;
    /* update patch name */
    gtk_signal_emit_by_name (GTK_OBJECT (edit_save_position_spin_adj), "value_changed");

    gtk_widget_show(edit_save_position_window);
}

void
on_editor_discard_button_press(GtkWidget *widget, gpointer data)
{
    GUIDB_MESSAGE(DB_GUI, ": on_editor_discard_button_press called\n");

    gtk_widget_hide(edit_save_position_window);

    edit_buffer_active = FALSE;
    gui_data_send_edit_buffer_off();
    edit_buffer.program = current_program;
    dx7_patch_unpack(patches, current_program, edit_buffer.voice);

    patch_edit_update_editors();
}

/* ==== DX7 MIDI system exclusive message handling ==== */

int sysex_receive_channel = 0;

static int
patch_edit_sysex_parse(unsigned int length, unsigned char* data)
{
    if (length < 6 || data[1] != 0x43)  /* too short, or not Yamaha */
        return 0;

    if ((data[2] & 0x0f) != sysex_receive_channel)  /* wrong MIDI channel */
        return 0;

    if ((data[2] & 0xf0) == 0x00 && data[3] == 0x00 &&  /* DX7 single voice dump */
        data[4] == 0x01 && data[5] == 0x1b) {

        if (length != DX7_DUMP_SIZE_VOICE_SINGLE ||
            data[DX7_DUMP_SIZE_VOICE_SINGLE - 1] != 0xf7) {
            GUIDB_MESSAGE(DB_IO, " gui_data_sysex_parse: badly formatted DX7 single voice dump!\n");
            return 0;
        }

        if (dx7_bulk_dump_checksum(&data[6], DX7_VOICE_SIZE_UNPACKED) !=
            data[DX7_DUMP_SIZE_VOICE_SINGLE - 2]) {
            GUIDB_MESSAGE(DB_IO, " gui_data_sysex_parse: DX7 single voice dump with bad checksum!\n");
            return 0;
        }

        GUIDB_MESSAGE(DB_IO, " gui_data_sysex_parse: DX7 single voice dump received\n");

        memcpy(&edit_buffer.voice, data + 6, DX7_VOICE_SIZE_UNPACKED);

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

        edit_buffer.voice[i] = data[5] & 0x7f;

        return 1;
    }

    /* nope, don't know what this is */
    return 0;
}

void
on_sysex_receipt(unsigned int length, unsigned char* data)
{
    GUIDB_MESSAGE(DB_IO, " on_sysex_receipt: message length %d, mfg id %02x\n", length, data[1]);

    if (patch_edit_sysex_parse(length, data)) {

        edit_buffer_active = TRUE;
        gui_data_send_edit_buffer();
        patch_edit_update_editors();
    }
}

/* ==== main patch edit window ==== */

GtkWidget *
create_editor_window(const char *tag)
{
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *combo;
    GtkWidget *separator;
    GtkWidget *frame;
    GtkWidget *table;
    GtkWidget *scale;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    set_window_title(window, tag, "Patch Edit");
    g_signal_connect (G_OBJECT(window), "destroy",
                      G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (G_OBJECT (window), "delete-event",
                      G_CALLBACK (on_delete_event_wrapper),
                      (gpointer)gtk_widget_hide);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

    editor_status_label = gtk_label_new (" ");
    gtk_container_add (GTK_CONTAINER (vbox), editor_status_label);
    gtk_misc_set_alignment (GTK_MISC (editor_status_label), 0, 0.5);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

    label = gtk_label_new ("Patch Name");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    editor_name_entry = gtk_entry_new_with_max_length(10);
    gtk_box_pack_start (GTK_BOX (hbox), editor_name_entry, FALSE, FALSE, 2);
    g_signal_connect(G_OBJECT(editor_name_entry), "changed",
                     G_CALLBACK(patch_edit_on_name_entry_changed), NULL);

    /* -FIX- comment (reuse Performance Name?)
     * GtkWidget *comment_label = gtk_label_new ("Comment");
     * gtk_box_pack_start (GTK_BOX (hbox), comment_label, FALSE, FALSE, 2);
     * gtk_misc_set_alignment (GTK_MISC (comment_label), 0, 0.5);

     * GtkWidget *comment_entry = gtk_entry_new_with_max_length(60);
     * gtk_box_pack_start (GTK_BOX (hbox), comment_entry, TRUE, TRUE, 2);
     */

    /* separator */
    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 2);

    gtk_widget_show_all(vbox);

    /* editor widgets */
    create_widgy_editor(tag);
    gtk_box_pack_start (GTK_BOX (vbox), widgy_widget, TRUE, FALSE, 2);
    gtk_widget_show(widgy_widget);

    create_retro_editor(tag);
    gtk_box_pack_start (GTK_BOX (vbox), retro_widget, TRUE, FALSE, 2);

    /* separator */
    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 2);
    gtk_widget_show(separator);

    /* edit action widgets */
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

    label = gtk_label_new ("Editor Mode");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);

    combo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Widgy");
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Retro");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 2);
    g_signal_connect(G_OBJECT(combo), "changed",
                     G_CALLBACK(patch_edit_on_mode_change), NULL);

    /* -FIX- add: [compare to original?] [swap A/B?] [close?] */

    label = gtk_label_new (NULL);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);

    editor_discard_button = gtk_button_new_with_label ("Discard Changes");
    gtk_box_pack_start (GTK_BOX (hbox), editor_discard_button, FALSE, FALSE, 2);
    gtk_signal_connect (GTK_OBJECT (editor_discard_button), "clicked",
                        GTK_SIGNAL_FUNC (on_editor_discard_button_press),
                        NULL);

    editor_save_button = gtk_button_new_with_label ("Save Changes into Patch Bank");
    gtk_box_pack_start (GTK_BOX (hbox), editor_save_button, FALSE, FALSE, 2);
    gtk_signal_connect (GTK_OBJECT (editor_save_button), "clicked",
                        GTK_SIGNAL_FUNC (on_editor_save_button_press),
                        NULL);

    gtk_widget_show_all(hbox);

    /* test note widgets */
    frame = gtk_frame_new ("Test Note");
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

    table = gtk_table_new (3, 3, FALSE);
    gtk_container_add (GTK_CONTAINER (frame), table);
    gtk_container_set_border_width (GTK_CONTAINER (table), 2);
    gtk_table_set_row_spacings (GTK_TABLE (table), 1);
    gtk_table_set_col_spacings (GTK_TABLE (table), 5);

    label = gtk_label_new ("key");
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    scale = gtk_hscale_new (GTK_ADJUSTMENT (test_note_key_adj));
    gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
    gtk_scale_set_digits (GTK_SCALE (scale), 0);
    gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);

    label = gtk_label_new ("velocity");
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    scale = gtk_hscale_new (GTK_ADJUSTMENT (test_note_velocity_adj));
    gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
    gtk_scale_set_digits (GTK_SCALE (scale), 0);
    gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);

    editor_test_note_button = gtk_button_new_with_label (" Send Test Note");
    gtk_table_attach (GTK_TABLE (table), editor_test_note_button, 2, 3, 0, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 4, 0);
    gtk_button_set_focus_on_click(GTK_BUTTON(editor_test_note_button), FALSE);
    g_signal_connect (G_OBJECT (editor_test_note_button), "pressed",
                      G_CALLBACK (on_test_note_button_press), (gpointer)1);
    g_signal_connect (G_OBJECT (editor_test_note_button), "released",
                      G_CALLBACK (on_test_note_button_press), (gpointer)0);

    gtk_widget_show_all (frame);

    editor_window = window;

    return editor_window;
}
