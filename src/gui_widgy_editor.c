/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2011, 2012 Sean Bolton.
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

// Need To Do:
// - fix envelope drawing to show L3 hold until key-off
// - fix envelope drawing to somehow be user scalable?
// - hook up mute buttons (once hexter has muting capability)

#include <math.h>

#include <gtk/gtk.h>

#include "hexter.h"
#include "dx7_voice_data.h"
#include "gui_interface.h"
#include "gui_patch_edit.h"

GtkWidget *widgy_widget;

static GtkWidget *freq_label[6];
static GtkWidget *bkpt_label[6];
#ifdef OP_MUTE_WORKS_IN_PLUGIN
static GtkObject *mute_adj[6];
#endif
static GtkWidget *env_drawing_area[6];
static GtkWidget *scaling_drawing_area[6];
static GtkWidget *pitch_drawing_area;

#define TABLE_OUTPUT     0
#define TABLE_OP1        1
#define TABLE_OP6        6
#define TABLE_GLOBAL     7
#define TABLE_COUNT      8

static GtkWidget *tables[TABLE_COUNT];

static char *tabs[TABLE_COUNT] = {
    NULL,
    "Op 1", "Op 2", "Op 3",
    "Op 4", "Op 5", "Op 6",
    "Global"
};

#define TO  TABLE_OUTPUT
#define TG  TABLE_GLOBAL

static struct _labels {
    char tab;
    char x, y;
    char width;
    const char *text;
} labels[] = {
    /* Output tab */
    { TO,  0,  0, 2, "Algorithm" },
    { TO,  0,  1, 2, "Feedback" },
    { TO,  0,  2, 1, " " }, /* spacer */
    { TO,  0,  3, 1, "Oper" },
#ifdef OP_MUTE_WORKS_IN_PLUGIN
    { TO,  1,  3, 1, "Mute" },
#endif
    { TO,  2,  3, 1, "OL" },
    { TO,  0,  4, 1, "1" },
    { TO,  0,  5, 1, "2" },
    { TO,  0,  6, 1, "3" },
    { TO,  0,  7, 1, "4" },
    { TO,  0,  8, 1, "5" },
    { TO,  0,  9, 1, "6" },
    { TO,  0, 10, 1, " " }, /* spacer */

    /* Global tab */
    { TG,  0,  1, 1, "Transpose" },
    { TG,  0,  2, 1, "Osc Key Sync" },
    { TG,  2,  0, 4, "LFO" },
    { TG,  2,  1, 1, "Speed" },
    { TG,  2,  2, 1, "Delay" },
    { TG,  2,  3, 1, "PMD" },
    { TG,  2,  4, 1, "AMD" },
    { TG,  4,  1, 1, "Wave" },
    { TG,  4,  2, 1, "Sync" },
    { TG,  4,  3, 1, "PMS" },
    { TG,  0,  5, 1, " " }, /* spacer */
    { TG,  0,  6, 1, "Pitch Envelope" },
    { TG,  1,  6, 1, "1" },
    { TG,  2,  6, 1, "2" },
    { TG,  3,  6, 1, "3" },
    { TG,  4,  6, 1, "4" },
    { TG,  0,  7, 1, "Rate" },
    { TG,  0,  8, 1, "Level" },

    { -1 } /* end */
};

static struct _op_labels {
    char x, y;
    char width;
    const char *text;
} op_labels[] = {
    { 0, 0, 1, "Frequency" },
    { 1, 0, 1, "Mode" },
    { 2, 0, 1, "Coarse" },
    { 3, 0, 1, "Fine" },
    { 4, 0, 1, "Detune" },
    { 0, 2, 1, " " }, /* spacer */
    { 0, 3, 1, "Envelope" },
    { 1, 3, 1, "1" },
    { 2, 3, 1, "2" },
    { 3, 3, 1, "3" },
    { 4, 3, 1, "4" },
    { 0, 4, 1, "Rate" },
    { 0, 5, 1, "Level" },
    { 5, 3, 2, "Sensitivity" },
    { 5, 4, 1, "Velocity" },
    { 5, 5, 1, "Amp Mod" },
    { 0, 7, 1, " " }, /* spacer */
    { 0, 8, 2, "Left" },
    { 2, 8, 2, "Level Scaling" },
    { 4, 8, 2, "Right" },
    { 0, 9, 1, "Curve" },
    { 1, 9, 1, "Depth" },
    { 2, 9, 2, "Breakpoint" },
    { 4, 9, 1, "Curve" },
    { 5, 9, 1, "Depth" },
    { 6, 8, 1, "Rate" },
    { 6, 9, 1, "Scaling" },

    { -1 } /* end */
};

static struct _parameters {
    char type;
    char tab;
    char x, y;
    int  offset;
} parameters[] = {
/*    type,        tab,  x, y, offset */
    { PEPT_Alg,    TO,   2, 0, 134 }, /* Alg */
    { PEPT_0_7,    TO,   2, 1, 135 }, /* Feedback */
    { PEPT_Scal99, TO,   2, 4, 121 }, /* O1 OL */
    { PEPT_Scal99, TO,   2, 5, 100 }, /* O2 OL */
    { PEPT_Scal99, TO,   2, 6,  79 }, /* O3 OL */
    { PEPT_Scal99, TO,   2, 7,  58 }, /* O4 OL */
    { PEPT_Scal99, TO,   2, 8,  37 }, /* O5 OL */
    { PEPT_Scal99, TO,   2, 9,  16 }, /* O6 OL */

    { PEPT_Name,    TG,  2,  2, 145 }, /* Name */

    { PEPT_Trans,   TG,  1,  1, 144 },
    { PEPT_OnOff,   TG,  1,  2, 136 }, /* Osc Key Sync */
    { PEPT_0_99,    TG,  3,  1, 137 }, /* LFO Speed */
    { PEPT_0_99,    TG,  3,  2, 138 }, /* Delay */
    { PEPT_0_99,    TG,  3,  3, 139 }, /* PMD */
    { PEPT_0_99,    TG,  3,  4, 140 }, /* AMD */
    { PEPT_LFOWave, TG,  5,  1, 142 }, /* Wave */
    { PEPT_OnOff,   TG,  5,  2, 141 }, /* Sync */
    { PEPT_0_7,     TG,  5,  3, 143 }, /* PMS */
    { PEPT_Env,     TG,  1,  7, 126 }, /* Pitch Envelope R1 */
    { PEPT_Env,     TG,  1,  8, 130 },
    { PEPT_Env,     TG,  2,  7, 127 },
    { PEPT_Env,     TG,  2,  8, 131 },
    { PEPT_Env,     TG,  3,  7, 128 },
    { PEPT_Env,     TG,  3,  8, 132 },
    { PEPT_Env,     TG,  4,  7, 129 },
    { PEPT_Env,     TG,  4,  8, 133 }, /* P L4 */

    { -1 } /* end */
};

#undef TO
#undef TG

static struct _op_parameters {
    char type;
    char x, y;
    int  offset;
} op_parameters[] = {
/*    type,         x,  y,  offset */
    { PEPT_Mode,    1,  1,  17 },
    { PEPT_FC,      2,  1,  18 }, /* FC */
    { PEPT_FF,      3,  1,  19 }, /* FF */
    { PEPT_Detune,  4,  1,  20 }, /* Detune */
    { PEPT_Env,     1,  4,   0 }, /* R1 */
    { PEPT_Env,     1,  5,   4 }, /* L1 */
    { PEPT_Env,     2,  4,   1 }, /* R2 */
    { PEPT_Env,     2,  5,   5 }, /* L2 */
    { PEPT_Env,     3,  4,   2 }, /* R3 */
    { PEPT_Env,     3,  5,   6 }, /* L3 */
    { PEPT_Env,     4,  4,   3 }, /* R4 */
    { PEPT_Env,     4,  5,   7 }, /* L4 */
    { PEPT_0_7,     6,  4,  15 }, /* VS */
    { PEPT_0_3,     6,  5,  14 }, /* AMS */
    { PEPT_Curve,   0, 10,  11 }, /* Left Curve */
    { PEPT_Scal99,  1, 10,   9 }, /* Left Depth */
    { PEPT_BkPt,    2, 10,   8 },
    { PEPT_Curve,   4, 10,  12 }, /* Right Curve */
    { PEPT_Scal99,  5, 10,  10 }, /* Right Depth */
    { PEPT_0_7,     6, 10,  13 }, /* RS */

    { -1 } /* end */
};

#define GEPT_Mute PEPT_COUNT   /* just for internal use */

/* ==== algorithm ==== */

static gboolean
alg_expose(GtkWidget *widget, GdkEventExpose *event)
{
    cairo_t *cr = gdk_cairo_create (widget->window);

    /* GUIDB_MESSAGE(DB_GUI, " ge alg_expose called\n"); */

    gtk_paint_box(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN,
                  &event->area, widget, NULL,
                  0, 0, widget->allocation.width, widget->allocation.height);

    gdk_cairo_rectangle (cr, &event->area);
    cairo_clip(cr);
    cairo_rectangle(cr, widget->style->xthickness, widget->style->ythickness,
                    widget->allocation.width - 2 * widget->style->xthickness,
                    widget->allocation.height - 2 * widget->style->ythickness);
    cairo_clip(cr);

    we_alg_draw(cr, patch_edit_get_edit_parameter(134));

    cairo_destroy (cr);

    return FALSE;
}

/* ==== level envelopes ==== */

static float op_color[6][3] = {
    { 1.0, 0.0, 0.0 },
    { 0.8, 0.7, 0.0 },
    { 0.0, 0.8, 0.0 },
    { 0.0, 0.8, 0.8 },
    { 0.0, 0.0, 1.0 },
    { 1.0, 0.0, 1.0 },
};

static gboolean
env_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    int thisop = (int)data;
    int r[6][4], l[6][4];
    float d[6][4], td, da, xrange;
    int op, i, width, height;
    cairo_t *cr = gdk_cairo_create (widget->window);

    /* GUIDB_MESSAGE(DB_GUI, " ge env_expose called\n"); */

    for (op = 0; op < 6; op++) {
        for (i = 0; i < 4; i++) {
            r[op][i] = patch_edit_get_edit_parameter(0 + i + (5 - op) * 21);
            l[op][i] = patch_edit_get_edit_parameter(4 + i + (5 - op) * 21);
        }
    }
    da = 0.0f;
    for (op = 0; op < 6; op++) {
        td = 0.0f;
        for (i = 0; i < 4; i++) {
            int i0 = (i - 1) & 3;

            if (l[op][i0] < l[op][i]) {
                d[op][i] = dx7_voice_eg_rate_rise_duration[r[op][i]] *
                          (dx7_voice_eg_rate_rise_percent[l[op][i]] -
                           dx7_voice_eg_rate_rise_percent[l[op][i0]]);
            } else {
                d[op][i] = dx7_voice_eg_rate_decay_duration[r[op][i]] *
                          (dx7_voice_eg_rate_decay_percent[l[op][i0]] -
                           dx7_voice_eg_rate_decay_percent[l[op][i]]);
            }
            if (d[op][i] < 0.0f)
                d[op][i] = 0.0f;
            td += d[op][i];
        }
        if (da < td)
            da = td;
    }
    /* da is the maximum of envelope time totals */
    /* -FIX- if td is zero, then it takes a long time to render? */
    td = d[thisop][0] + d[thisop][1] + d[thisop][2] + d[thisop][3]; /* time total of this envelope */
    if (da < 0.005f) da = 0.005f; /* prevent division by zero */
    if (td < 0.005f) td = 0.005f; /* prevent division by zero */
    td = td / (0.66f + 0.34f * (1.0f - ((da - td) / da)));

    width  = widget->allocation.width;
    height = widget->allocation.height;
    gtk_paint_box(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN,
                  &event->area, widget, NULL,
                  0, 0, width, height);
    width  -= 2 * widget->style->xthickness;
    height -= 2 * widget->style->ythickness;

    gdk_cairo_rectangle (cr, &event->area);
    cairo_clip(cr);
    cairo_translate(cr, widget->style->xthickness, widget->style->ythickness);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_clip(cr);

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    xrange = (float)width  - 20.0f;

    for (op = 0; op < 6; op++) {
        if (op == thisop)
            continue;

        cairo_set_source_rgba(cr, op_color[op][0], op_color[op][1], op_color[op][2], 0.7);
        cairo_set_line_width(cr, 1);
        cairo_set_dash(cr, (const double []){ 1.5, 2.5 }, 2, 0.0);
        cairo_move_to(cr, 10.0f, 109.0f - (float)l[op][3]);
        da = 0.0f;
        for (i = 0; i < 4; i++) {
            da += d[op][i];
            cairo_line_to(cr, 10.0f + da * xrange / td,
                              109.0f - (float)l[op][i]);
        }
        cairo_stroke(cr);
    }
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);
    cairo_set_dash(cr, NULL, 0, 0.0);
    cairo_move_to(cr, 10.0f, 109.0f - (float)l[thisop][3]);
    da = 0.0f;
    for (i = 0; i < 4; i++) {
        da += d[thisop][i];
        cairo_line_to(cr, 10.0f + da * xrange / td,
                          109.0f - (float)l[thisop][i]);
    }
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);
    da = 0.0f;
    i = 0;
    while (da <= xrange) {
        cairo_move_to(cr, 10.5f + floorf(da), 110.0f);
        cairo_line_to(cr, 10.5f + floorf(da), (i % 5 == 0) ? 115.0f : 112.0f);
        cairo_stroke(cr);
        //da += floorf(xrange / td);
        da += xrange / td;
        i++;
    }

    cairo_destroy (cr);

    return FALSE;
}

/* ==== level scaling ==== */

static gboolean
scaling_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    int op = (int)data;
    int ol      = patch_edit_get_edit_parameter(16 + (5 - op) * 21);
    int l_curve = patch_edit_get_edit_parameter(11 + (5 - op) * 21);
    int l_depth = patch_edit_get_edit_parameter( 9 + (5 - op) * 21);
    int bkpt    = patch_edit_get_edit_parameter( 8 + (5 - op) * 21);
    int r_curve = patch_edit_get_edit_parameter(12 + (5 - op) * 21);
    int r_depth = patch_edit_get_edit_parameter(10 + (5 - op) * 21);
    int width, height, i, n;
    float ppn, xbase, f;
    cairo_t *cr = gdk_cairo_create (widget->window);

    /* GUIDB_MESSAGE(DB_GUI, " ge scaling_expose called\n"); */

    width  = widget->allocation.width;
    height = widget->allocation.height;
    gtk_paint_box(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN,
                  &event->area, widget, NULL,
                  0, 0, width, height);
    width  -= 2 * widget->style->xthickness;
    height -= 2 * widget->style->ythickness;

    gdk_cairo_rectangle (cr, &event->area);
    cairo_clip(cr);
    cairo_translate(cr, widget->style->xthickness, widget->style->ythickness);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_clip(cr);

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    ppn = floorf((float)(width - 20) / 128.0f);  /* horizontal pixels per note */
    xbase = (float)(width / 2) - ppn * 64.0f + 0.5f;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1.0);

    /* place ticks on axes */
    cairo_move_to(cr, xbase - 4.5f, 10.5f);
    cairo_line_to(cr, xbase - 0.5f, 10.5f);
    cairo_stroke(cr);
    cairo_move_to(cr, xbase - 4.5f, 109.5f);
    cairo_line_to(cr, xbase - 0.5f, 109.5f);
    cairo_stroke(cr);
    cairo_move_to(cr, xbase, 110.0f);
    cairo_line_to(cr, xbase, 114.0f);
    cairo_stroke(cr);
    cairo_move_to(cr, xbase + ppn * 60.0f, 110.0f); /* Middle C */
    cairo_line_to(cr, xbase + ppn * 60.0f, 114.0f);
    cairo_stroke(cr);
    f = xbase + ppn * 127.0f;
    cairo_move_to(cr, f + 0.5f, 10.5f);
    cairo_line_to(cr, f + 4.5f, 10.5f);
    cairo_stroke(cr);
    cairo_move_to(cr, f + 0.5f, 109.5f);
    cairo_line_to(cr, f + 4.5f, 109.5f);
    cairo_stroke(cr);
    cairo_move_to(cr, f, 110.0f);
    cairo_line_to(cr, f, 114.0f);
    cairo_stroke(cr);

    cairo_move_to(cr, xbase + (float)(bkpt + 21) * ppn, 104.0f - (float)ol);
    cairo_line_to(cr, xbase + (float)(bkpt + 21) * ppn, 114.0f - (float)ol);
    cairo_stroke(cr);

    for (i = 0; i < 128; i++) {
        int scaled_output_level = ol;

        if (i < bkpt + 21 && l_depth) {

            /* On the original DX7/TX7, keyboard level scaling calculations
             * group the keyboard into groups of three keys.  This can be quite
             * noticeable on patches with extreme scaling depths, so I've tried
             * to replicate it here (the steps between levels may not occur at
             * exactly the keys).  If you'd prefer smother scaling, define
             * SMOOTH_KEYBOARD_LEVEL_SCALING. */
#ifndef SMOOTH_KEYBOARD_LEVEL_SCALING
            n = bkpt - (((i + 2) / 3) * 3) + 21;
#else
            n = bkpt - i + 21;
#endif

            switch(l_curve) {
              case 0: /* -LIN */
                scaled_output_level -= (int)((float)n / 45.0f * (float)l_depth);
                break;
              case 1: /* -EXP */
                scaled_output_level -= (int)(expf((float)(n - 72) / 13.5f) * (float)l_depth);
                break;
              case 2: /* +EXP */
                scaled_output_level += (int)(expf((float)(n - 72) / 13.5f) * (float)l_depth);
                break;
              case 3: /* +LIN */
                scaled_output_level += (int)((float)n / 45.0f * (float)l_depth);
                break;
            }
            if (scaled_output_level < 0)  scaled_output_level = 0;
            if (scaled_output_level > 99) scaled_output_level = 99;

        } else if (i > bkpt + 21 && r_depth) {

#ifndef SMOOTH_KEYBOARD_LEVEL_SCALING
            n = (((i + 2) / 3) * 3) - bkpt - 21;
#else
            n = i - bkpt - 21;
#endif

            switch(r_curve) {
              case 0: /* -LIN */
                scaled_output_level -= (int)((float)n / 45.0f * (float)r_depth);
                break;
              case 1: /* -EXP */
                scaled_output_level -= (int)(exp((float)(n - 72) / 13.5f) * (float)r_depth);
                break;
              case 2: /* +EXP */
                scaled_output_level += (int)(exp((float)(n - 72) / 13.5f) * (float)r_depth);
                break;
              case 3: /* +LIN */
                scaled_output_level += (int)((float)n / 45.0f * (float)r_depth);
                break;
            }
            if (scaled_output_level < 0)  scaled_output_level = 0;
            if (scaled_output_level > 99) scaled_output_level = 99;
        }
        if (i == 0)
            cairo_move_to(cr, xbase + (float)i * ppn, 109.5f - (float)scaled_output_level);
        else
            cairo_line_to(cr, xbase + (float)i * ppn, 109.5f - (float)scaled_output_level);
    }
    cairo_stroke(cr);

    cairo_destroy (cr);

    return FALSE;
}

/* ==== pitch envelope ==== */

static gboolean
pitch_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    int i, r[4], width, height;
    float l[4], td, d[4], xrange, da;
    cairo_t *cr = gdk_cairo_create (widget->window);

    /* GUIDB_MESSAGE(DB_GUI, " ge pitch_expose called\n"); */

    for (i = 0; i < 4; i++) {
        r[i] = patch_edit_get_edit_parameter(126 + i);
        l[i] = (float)dx7_voice_pitch_level_to_shift[patch_edit_get_edit_parameter(130 + i)];
    }
    td = 0.0f;
    for (i = 0; i < 4; i++) {
        int i0 = (i - 1) & 3;

        /* -FIX- This is just a quick approximation that I derived from
         * regression of Godric Wilkie's pitch eg timings. In particular,
         * it's not accurate for very slow envelopes. */
        d[i] = expf(((float)r[i] - 70.337897f) / -25.580953f) *
               fabsf((l[i] - l[i0]) / 96.0f);
        if (d[i] < 2.083e-5f) d[i] = 2.083e-5f;
        td += d[i];
    }

    width  = widget->allocation.width;
    height = widget->allocation.height;
    gtk_paint_box(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN,
                  &event->area, widget, NULL,
                  0, 0, width, height);
    width  -= 2 * widget->style->xthickness;
    height -= 2 * widget->style->ythickness;

    gdk_cairo_rectangle (cr, &event->area);
    cairo_clip(cr);
    cairo_translate(cr, widget->style->xthickness, widget->style->ythickness);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_clip(cr);

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    xrange = (float)width  - 20.0f;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, 5.0f, 10.5f);
    cairo_line_to(cr, 9.0f, 10.5f);
    cairo_stroke(cr);
    cairo_move_to(cr, 5.0f, 58.5f);
    cairo_line_to(cr, 9.0f, 58.5f);
    cairo_stroke(cr);
    cairo_move_to(cr, 5.0f, 106.5f);
    cairo_line_to(cr, 9.0f, 106.5f);
    cairo_stroke(cr);
    cairo_move_to(cr, xrange + 11.0f, 10.5f);
    cairo_line_to(cr, xrange + 15.0f, 10.5f);
    cairo_stroke(cr);
    cairo_move_to(cr, xrange + 11.0f, 58.5f);
    cairo_line_to(cr, xrange + 15.0f, 58.5f);
    cairo_stroke(cr);
    cairo_move_to(cr, xrange + 11.0f, 106.5f);
    cairo_line_to(cr, xrange + 15.0f, 106.5f);
    cairo_stroke(cr);

    cairo_move_to(cr, 10.0f, 58.5f - l[3]);
    da = 0.0f;
    for (i = 0; i < 4; i++) {
        da += d[i];
        cairo_line_to(cr, 10.0f + da * xrange / td,
                          58.5f - l[i]);
    }
    cairo_stroke(cr);

    da = 0.0f;
    i = 0;
    while (da <= xrange) {
        cairo_move_to(cr, 10.5f + da, 107.0f);
        cairo_line_to(cr, 10.5f + da, (i % 5 == 0) ? 112.0f : 109.0f);
        cairo_stroke(cr);
        da += floorf(xrange / td);
        i++;
    }

    cairo_destroy (cr);

    return FALSE;
}

/* ==== shadow adjustments ==== */

/* The adjustments in edit_adj[] represent the actual value in the edit buffer, yet
 * some parameters need to be displayed as a different range.  For example:
 *   Algorithm, which is 0 to 31 in edit_adj[], but the spinbutton needs to show 1 to 32
 *   Detune,    which is 0 to 14 in edit_adj[], but the spinbutton needs to show -7 to +7
 *   Transpose, which is 0 to 48 in edit_adj[], but the spinbutton needs to show -24 to +24
 * So we use a 'shadow' adjustment with the translated range (e.g. -7 to 7), that is
 * connected directly to the spinbutton, and link it to the 'real' adjustment having the
 * zero-based (e.g. 0 to 14) range.
 *
 * The shadow adjustment and its bias are stored as key-value pairs attached to the real
 * adjustment using g_object_set_data().
 */

static void on_shadow_adj_changed(GtkAdjustment *shadow_adj, gpointer data); /* forward */

static void
on_real_adj_changed(GtkAdjustment *real_adj, gpointer data)
{
    int offset = (int)data;
    GtkAdjustment *shadow_adj = g_object_get_data(G_OBJECT(real_adj), "shadow_adjustment");
    int bias = (int)g_object_get_data(G_OBJECT(real_adj), "shadow_bias");

    /* GUIDB_MESSAGE(DB_GUI, " ge on_real_adj_changed: offset %d now %d\n", offset, (int)real_adj->value); */

    /* temporarily block reverse signal, then update shadow adjustment */
    g_signal_handlers_block_by_func(G_OBJECT(shadow_adj), on_shadow_adj_changed, (gpointer)offset);
    gtk_adjustment_set_value(shadow_adj, real_adj->value + (float)bias);
    g_signal_handlers_unblock_by_func(G_OBJECT(shadow_adj), on_shadow_adj_changed, (gpointer)offset);
}

static void
on_shadow_adj_changed(GtkAdjustment *shadow_adj, gpointer data)
{
    int offset = (int)data;
    int bias = (int)g_object_get_data(G_OBJECT(edit_adj[offset]), "shadow_bias");

    /* GUIDB_MESSAGE(DB_GUI, " ge on_shadow_adj_changed: offset %d now %d\n", offset, (int)shadow_adj->value); */

    /* temporarily block reverse signal, then update the real adjustment */
    g_signal_handlers_block_by_func(G_OBJECT(edit_adj[offset]), on_real_adj_changed, (gpointer)offset);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(edit_adj[offset]), shadow_adj->value - (float)bias);
    g_signal_handlers_unblock_by_func(G_OBJECT(edit_adj[offset]), on_real_adj_changed, (gpointer)offset);
}

/* ==== spin buttons ==== */

static void
on_alg_changed(GtkAdjustment *adj, gpointer data)
{
    /* int alg = (int)adj->value; */

    /* GUIDB_MESSAGE(DB_GUI, " ge on_alg_changed: now %d\n", alg); */

    gtk_widget_queue_draw (we_alg_drawing_area);
}

static void
on_env_changed(GtkAdjustment *adj, gpointer data)
{
    int offset = (int)data;
    int op = 5 - (offset / 21);  /* 0 to 5, or -1 for pitch envelope */

    if (op >= 0) {
        gtk_widget_queue_draw (env_drawing_area[op]);
    } else {
        gtk_widget_queue_draw (pitch_drawing_area);
    }
}

static void
we_update_freq(int op)
{
    unsigned long fc, ff, f;
    char *units;
    char ca[10], cb[10];

    fc = patch_edit_get_edit_parameter(123 - op * 21);
    ff = patch_edit_get_edit_parameter(124 - op * 21);
    if (patch_edit_get_edit_parameter(122 - op * 21)) { /* fixed */
        f = peFFF[ff];
        switch (fc & 3) {
            case 1: f *=   10;    break;
            case 2: f *=  100;    break;
            case 3: f *= 1000;    break;
        }
        units = " Hz";
    } else {
        f = fc * 10;
        if (!f) f = 5;
        f = (100 + ff) * f;
        units = "";
    }
    f = snprintf(ca, 10, "%ld", f);
    switch (f) {
        case 3: snprintf(cb, 10, "0.%.3s%s", ca, units);              break;
        case 4: snprintf(cb, 10, "%.1s.%.3s%s", ca, ca + 1, units);   break;
        case 5: snprintf(cb, 10, "%.2s.%.2s%s", ca, ca + 2, units);   break;
        case 6: snprintf(cb, 10, "%.3s.%.1s%s", ca, ca + 3, units);   break;
        case 7: snprintf(cb, 10, "%.4s.%s", ca, units);               break;
    }
    gtk_label_set_text(GTK_LABEL(freq_label[op]), cb);
}

static void
on_freq_changed(GtkAdjustment *adj, gpointer data)
{
    /* float value = adj->value;   -- for FC or FF */
    /* int mode = (int)adj->value; -- for mode */
    int offset = (int)data;
    int op = 5 - ((offset - 17) / 21);  /* 0 to 5 */

    /* GUIDB_MESSAGE(DB_GUI, " ge on_freq_changed: offset %d, operator %d\n", offset, op); */

    we_update_freq(op);
}

static void
on_bkpt_changed(GtkAdjustment *adj, gpointer data)
{
    int bkpt = (int)adj->value;
    int offset = (int)data;
    int op = 5 - ((offset - 8) / 21);  /* 0 to 5 */

    /* GUIDB_MESSAGE(DB_GUI, " ge on_bkpt_changed: op %d breakpoint now %d\n", op, bkpt); */

    gtk_label_set_text(GTK_LABEL(bkpt_label[op]), patch_edit_NoteText(bkpt + 21));

    gtk_widget_queue_draw (scaling_drawing_area[op]);
}

static void
on_scaling_changed(GtkAdjustment *adj, gpointer data)
{
    int offset = (int)data;
    int op = 5 - (offset / 21);  /* 0 to 5 */

    gtk_widget_queue_draw (scaling_drawing_area[op]);
}

static void
place_spin(GtkObject *adj, GtkWidget *table, int x, int y)
{
    GtkWidget *spin = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 2, 0);

    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spin), TRUE);
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spin), GTK_UPDATE_IF_VALID);
    gtk_table_attach (GTK_TABLE (table), spin, x, x + 1, y, y + 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);
}


/* ==== switches ==== */

static void
update_switch_label_text(GtkToggleButton *t)
{
    int type = (int)g_object_get_data(G_OBJECT(t), "parameter_type");
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(t));
    gboolean state = gtk_toggle_button_get_active(t);

    switch (type) {
      case PEPT_Mode:
        gtk_label_set_text(GTK_LABEL(label), state ? "Fixed" : "Ratio");
        break;
      case PEPT_OnOff:
        gtk_label_set_text(GTK_LABEL(label), state ? "On" : "Off");
        break;
      case GEPT_Mute:
        gtk_label_set_text(GTK_LABEL(label),
                           state ? "<span foreground=\"black\" background=\"red\">M</span>" : "M");
        if (state) gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
        break;
    }
}

static void on_switch_changed(GtkToggleButton *togglebutton, gpointer user_data); /* forward */

static void
on_switch_adjustment_changed(GtkAdjustment *adj, gpointer data)
{
    GtkToggleButton *togglebutton = GTK_TOGGLE_BUTTON(data);

    /* GUIDB_MESSAGE(DB_GUI, " ge on_switch_adjustment_changed %p %p %d\n", adj, data, (int)adj->value); */

    /* temporarily block reverse signal, then set the togglebutton state */
    g_signal_handlers_block_by_func(G_OBJECT(togglebutton), on_switch_changed, (gpointer)adj);
    gtk_toggle_button_set_active(togglebutton, (adj->value > 1e-20f));
    g_signal_handlers_unblock_by_func(G_OBJECT(togglebutton), on_switch_changed, (gpointer)adj);

    update_switch_label_text(togglebutton);
}

static void
on_switch_changed(GtkToggleButton *togglebutton,
                  gpointer         user_data)
{
    GtkAdjustment *adj = GTK_ADJUSTMENT(user_data);
    gboolean state = gtk_toggle_button_get_active(togglebutton);

    /* GUIDB_MESSAGE(DB_GUI, " ge on_switch_changed %p %p %d\n", togglebutton, user_data, state); */

    update_switch_label_text(togglebutton);

    /* temporarily block reverse signal, then update the adjustment */
    g_signal_handlers_block_by_func(G_OBJECT(adj), on_switch_adjustment_changed, (gpointer)togglebutton);
    gtk_adjustment_set_value(adj, state ? 1.0 : 0.0);
    g_signal_handlers_unblock_by_func(G_OBJECT(adj), on_switch_adjustment_changed, (gpointer)togglebutton);
}

static void
place_switch(GtkObject *adj, GtkWidget *table, int x, int y, int type)
{
    GtkWidget *w = gtk_toggle_button_new_with_label ("-");

    g_object_set_data(G_OBJECT(w), "parameter_type", (gpointer)type);
    gtk_table_attach (GTK_TABLE (table), w, x, x + 1, y, y + 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);

    if (type == GEPT_Mute) {  /* -FIX- set mute buttons insensitive until hexter supports muting */
        gtk_widget_set_sensitive(w, FALSE);
        update_switch_label_text((GtkToggleButton *)w);
    }

    g_signal_connect(G_OBJECT(adj), "value-changed", G_CALLBACK(on_switch_adjustment_changed), w);
    g_signal_connect(G_OBJECT(w), "toggled", G_CALLBACK(on_switch_changed), (gpointer)adj);
}

/* ==== combo boxes ==== */

static void on_combo_changed(GtkComboBox *widget, gpointer user_data); /* forward */

static void
on_combo_adjustment_changed(GtkAdjustment *adjustment,
                            gpointer       user_data)
{
    GtkComboBox *w = GTK_COMBO_BOX(user_data);

    /* GUIDB_MESSAGE(DB_GUI, " ge on_combo_adjustment_changed %p %p\n", adjustment, user_data); */

    /* temporarily block reverse signal, then set combobox active item */
    g_signal_handlers_block_by_func(G_OBJECT(w), on_combo_changed, (gpointer)adjustment);
    gtk_combo_box_set_active(w, gtk_adjustment_get_value(adjustment));
    g_signal_handlers_unblock_by_func(G_OBJECT(w), on_combo_changed, (gpointer)adjustment);
}

static void
on_combo_changed(GtkComboBox *widget,
                 gpointer     user_data)
{
    GtkAdjustment *adj = GTK_ADJUSTMENT(user_data);

    /* GUIDB_MESSAGE(DB_GUI, " ge on_combo_changed %p %p\n", widget, user_data); */

    /* temporarily block reverse signal, then update adjustment */
    g_signal_handlers_block_by_func(G_OBJECT(adj), on_combo_adjustment_changed, (gpointer)widget);
    gtk_adjustment_set_value(adj, gtk_combo_box_get_active(widget));
    g_signal_handlers_unblock_by_func(G_OBJECT(adj), on_combo_adjustment_changed, (gpointer)widget);
}

static void
place_combo(GtkObject *adj, GtkWidget *table, int x, int y, int type)
{
    const char **labels;
    int i;
    /* -FIX- this is kinda ugly */
    switch (type) {
      default:
      case PEPT_LFOWave:
        labels = (const char *[]){ "Tri", "Saw+", "Saw-", "Square", "Sine", "S/H", NULL };
        break;
      case PEPT_Curve:
        labels = (const char *[]){ "-Lin", "-Exp", "+Exp", "+Lin", NULL };
        break;
    }

    GtkWidget *w = gtk_combo_box_new_text();
    for (i = 0; labels[i]; i++)
        gtk_combo_box_append_text(GTK_COMBO_BOX(w), labels[i]);
    gtk_combo_box_set_active((GtkComboBox *)w, 0);
    gtk_table_attach (GTK_TABLE (table), w, x, x + 1, y, y + 1,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 0, 0);

    g_signal_connect(G_OBJECT(adj), "value-changed", G_CALLBACK(on_combo_adjustment_changed), w);
    g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(on_combo_changed), (gpointer)adj);
}

/* ==== widgy editor instantiation ==== */

static void
place_widget(int type, int tab, int x, int y, int offset)
{
    switch (type) {
      case PEPT_0_99:
      case PEPT_0_7:
      case PEPT_0_3:
        place_spin(edit_adj[offset], tables[tab], x, y);
        break;
      case PEPT_Env:
        place_spin(edit_adj[offset], tables[tab], x, y);
        g_signal_connect (G_OBJECT(edit_adj[offset]), "value-changed",
                          G_CALLBACK (on_env_changed), (gpointer)offset);
        break;
      case PEPT_FC:
      case PEPT_FF:
        place_spin(edit_adj[offset], tables[tab], x, y);
        g_signal_connect (G_OBJECT(edit_adj[offset]), "value-changed",
                          G_CALLBACK (on_freq_changed), (gpointer)offset);
        break;
      case PEPT_Mode:
        place_switch(edit_adj[offset], tables[tab], x, y, PEPT_Mode);
        g_signal_connect (G_OBJECT(edit_adj[offset]), "value-changed",
                          G_CALLBACK (on_freq_changed), (gpointer)offset);
        break;
      case PEPT_Alg:     /* 0 to 31, algorithm, as 1 to 32 */
      case PEPT_Detune:  /* 0 to 14, detune,    as -7 to +7 */
      case PEPT_Trans:   /* 0 to 48, transpose, as -24 to +24 */
        {
            int range, bias;
            GtkObject *shadow_adj;  /* the 'shadow' adjustment, directly connected to spinbutton */
            void (*callback)(GtkAdjustment *, gpointer);

            switch (type) {
              default:
              case PEPT_Alg:    range = 31; bias = 1;   callback = on_alg_changed;  break;
              case PEPT_Detune: range = 14; bias = -7;  callback = NULL;            break;
              case PEPT_Trans:  range = 48; bias = -24; callback = NULL;            break;
            }

            shadow_adj = gtk_adjustment_new(bias, bias, range + bias, 1, (range > 10) ? 10 : 1, 0);
            place_spin(shadow_adj, tables[tab], x, y);
            g_signal_connect (G_OBJECT(shadow_adj), "value-changed",
                              G_CALLBACK (on_shadow_adj_changed), (gpointer)offset);
            /* edit_adj[offset] is the 'real' adjustment */
            g_object_set_data(G_OBJECT(edit_adj[offset]), "shadow_adjustment", (gpointer)shadow_adj);
            g_object_set_data(G_OBJECT(edit_adj[offset]), "shadow_bias", (gpointer)bias);
            if (callback != NULL)
                g_signal_connect (G_OBJECT(edit_adj[offset]), "value-changed",
                                  G_CALLBACK(callback), (gpointer)offset);
            g_signal_connect (G_OBJECT(edit_adj[offset]), "value-changed",
                              G_CALLBACK (on_real_adj_changed), (gpointer)offset);
        }
        break;
      case PEPT_BkPt:
        place_spin(edit_adj[offset], tables[tab], x, y);
        g_signal_connect (G_OBJECT(edit_adj[offset]), "value-changed",
                          G_CALLBACK (on_bkpt_changed), (gpointer)offset);
        break;
      case PEPT_Scal99:
        place_spin(edit_adj[offset], tables[tab], x, y);
        g_signal_connect (G_OBJECT(edit_adj[offset]), "value-changed",
                          G_CALLBACK (on_scaling_changed), (gpointer)offset);
        break;
      case PEPT_OnOff:
        place_switch(edit_adj[offset], tables[tab], x, y, PEPT_OnOff);
        break;
      case PEPT_LFOWave:
        place_combo(edit_adj[offset], tables[tab], x, y, PEPT_LFOWave);
        break;
      case PEPT_Curve:
        place_combo(edit_adj[offset], tables[tab], x, y, PEPT_Curve);
        g_signal_connect (G_OBJECT(edit_adj[offset]), "value-changed",
                          G_CALLBACK (on_scaling_changed), (gpointer)offset);
        break;
    }
}

GtkWidget *
create_widgy_editor(const char *tag)
{
    GtkWidget *hbox;
    GtkWidget *notebook;
    GtkWidget *table;
    GtkWidget *label;
    int i, x, y;

    /* create notebook and tabs */
    hbox = gtk_hbox_new(FALSE, 0);

    table = gtk_table_new (1, 1, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 4);
    tables[TABLE_OUTPUT] = table;
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);

    notebook = gtk_notebook_new ();
    gtk_box_pack_start(GTK_BOX(hbox), notebook, TRUE, TRUE, 0);

    for (i = TABLE_OUTPUT + 1; i < TABLE_COUNT; i++) {
        table = gtk_table_new (1, 1, FALSE);
        gtk_container_set_border_width(GTK_CONTAINER(table), 4);
        gtk_container_add (GTK_CONTAINER (notebook), table);
        tables[i] = table;
        label = gtk_label_new (tabs[i]);
        gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook),
                                    gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), i - 1),
                                    label);
    }

    /* place labels */
    for (i = 0; labels[i].tab >= 0; i++) {
        label = gtk_label_new(labels[i].text);
        /* if (strlen(labels[i].text) > 1)
         *    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);  -FIX- */
        x = labels[i].x;
        y = labels[i].y;
        gtk_table_attach (GTK_TABLE (tables[(int)labels[i].tab]), label,
                          x, x + labels[i].width, y, y + 1,
                          (GtkAttachOptions) (0),
                          (GtkAttachOptions) (0), 0, 0);
    }
    for (i = 0; op_labels[i].x >= 0; i++) {
        int op;

        for (op = 0; op < 6; op++) {
            label = gtk_label_new(op_labels[i].text);
            x = op_labels[i].x;
            y = op_labels[i].y;
            gtk_table_attach (GTK_TABLE (tables[TABLE_OP1 + op]), label,
                              x, x + op_labels[i].width, y, y + 1,
                              (GtkAttachOptions) (0),
                              (GtkAttachOptions) (0), 0, 0);
        }
    }

    /* place widgets */
    for (i = 0; parameters[i].type >= 0; i++) {
        place_widget(parameters[i].type, parameters[i].tab,
                     parameters[i].x, parameters[i].y,
                     parameters[i].offset);
    }
    for (i = 0; op_parameters[i].type >= 0; i++) {
        int op;

        for (op = 0; op < 6; op++) {
            place_widget(op_parameters[i].type, TABLE_OP1 + op,
                         op_parameters[i].x, op_parameters[i].y,
                         op_parameters[i].offset + (5 - op) * 21);
        }
    }

#ifdef OP_MUTE_WORKS_IN_PLUGIN
    /* create mute buttons */
    for (i = 0; i < 6; i++) {
        mute_adj[i] = gtk_adjustment_new(0, 0, 1, 1, 1, 0);
        place_switch(mute_adj[i], tables[TABLE_OUTPUT], 1, i + 4, GEPT_Mute);
    }
#endif /* OP_MUTE_WORKS_IN_PLUGIN */

    /* create breakpoint and frequency labels */
    for (i = 0; i < 6; i++) {
        bkpt_label[i] = gtk_label_new("bkpt");
        gtk_table_attach (GTK_TABLE (tables[TABLE_OP1 + i]), bkpt_label[i], 3, 4, 10, 11,
                          (GtkAttachOptions) (0),
                          (GtkAttachOptions) (0), 0, 0);
        freq_label[i] = gtk_label_new(NULL);
        gtk_table_attach (GTK_TABLE (tables[TABLE_OP1 + i]), freq_label[i], 0, 1, 1, 2,
                          (GtkAttachOptions) (0),
                          (GtkAttachOptions) (0), 0, 0);
    }

    /* create algorithm drawing area */
    we_alg_drawing_area_new();
    gtk_table_attach (GTK_TABLE (tables[TABLE_OUTPUT]), we_alg_drawing_area, 0, 3, 11, 12,
                      (GtkAttachOptions) (0),
                      (GtkAttachOptions) (0), 5, 5);
    g_signal_connect(G_OBJECT(we_alg_drawing_area), "expose-event", G_CALLBACK(alg_expose), NULL);

    /* create envelope and scaling drawing areas */
    for (i = 0; i < 6; i++) {
        env_drawing_area[i] = gtk_drawing_area_new();
        gtk_widget_set_size_request(env_drawing_area[i],
                                    220 + env_drawing_area[i]->style->xthickness * 2,
                                    120 + env_drawing_area[i]->style->ythickness * 2);
        gtk_table_attach (GTK_TABLE (tables[TABLE_OP1 + i]), env_drawing_area[i],
                              0, 7, 6, 7,
                              (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                              (GtkAttachOptions) (0), 0, 5);
        g_signal_connect(G_OBJECT(env_drawing_area[i]), "expose-event", G_CALLBACK(env_expose),
                         (gpointer)i);

        scaling_drawing_area[i] = gtk_drawing_area_new();
        gtk_widget_set_size_request(scaling_drawing_area[i],
                                    276 + scaling_drawing_area[i]->style->xthickness * 2,
                                    120 + scaling_drawing_area[i]->style->ythickness * 2);
        gtk_table_attach (GTK_TABLE (tables[TABLE_OP1 + i]), scaling_drawing_area[i],
                              0, 7, 11, 12,
                              (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                              (GtkAttachOptions) (0), 0, 5);
        g_signal_connect(G_OBJECT(scaling_drawing_area[i]), "expose-event", G_CALLBACK(scaling_expose),
                         (gpointer)i);
    }
    pitch_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(pitch_drawing_area,
                                220 + pitch_drawing_area->style->xthickness * 2,
                                116 + pitch_drawing_area->style->ythickness * 2);
    gtk_table_attach (GTK_TABLE (tables[TABLE_GLOBAL]), pitch_drawing_area,
                          0, 6, 12, 13,
                          (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                          (GtkAttachOptions) (0), 0, 5);
    g_signal_connect(G_OBJECT(pitch_drawing_area), "expose-event", G_CALLBACK(pitch_expose),
                     NULL);

    gtk_widget_show_all(hbox);

    widgy_widget = hbox;

    return widgy_widget;
}
