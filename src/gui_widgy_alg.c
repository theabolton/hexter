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

#include <gtk/gtk.h>

GtkWidget *we_alg_drawing_area;

static PangoLayout *alg_layout;
static PangoRectangle op_char;
static int op_width, op_height,
           op_x_spacing, op_y_spacing,
           alg_x_base, alg_y_top, alg_y_base,
           alg_x_stride, alg_y_stride;

void
we_alg_drawing_area_new(void)
{

    we_alg_drawing_area = gtk_drawing_area_new();
    alg_layout = gtk_widget_create_pango_layout (we_alg_drawing_area, "3");
    pango_layout_get_pixel_extents (alg_layout, &op_char, NULL);
    /* see also gtk_calendar_size_request() for getting width and height, reusing a layout, ... */
    /* if (op_char.x != 0 || op_char.y != 0) { -- non-zero origin is not handled
     *     printf("-FIX- pango reports non-zero origin for font layout: op_char.x = %d || op_char.y = %d\n", op_char.x, op_char.y);
     * } */
    op_width  = (op_char.width + 5) & (~1);
    op_height = (op_char.height + 5) & (~1);
    op_x_spacing = op_y_spacing = (op_char.width & (~1)) + 1;
    alg_x_base = we_alg_drawing_area->style->xthickness + 2 + op_x_spacing + op_width / 2;
    alg_y_top  = we_alg_drawing_area->style->ythickness + 2 + op_y_spacing + op_height / 2;
    alg_x_stride = op_width + op_x_spacing;
    alg_y_stride = op_height + op_y_spacing;
    alg_y_base = alg_y_top + alg_y_stride * 3;

    gtk_widget_set_size_request(we_alg_drawing_area,
                                alg_x_base * 2 + alg_x_stride * 5,
                                alg_y_top  * 2 + alg_y_stride * 3);
}

static void
alg_draw_op(cairo_t *cr, int x, int y, int op)
{
    char buf[2];
    int x0 = alg_x_base + alg_x_stride * x;
    int y0 = alg_y_base - alg_y_stride * y;

    buf[0] = '0' + op;
    buf[1] = 0;
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, (float)(x0 - op_width / 2) + 0.5f,
                        (float)(y0 - op_height / 2) + 0.5f,
                        op_width, op_height);
    cairo_set_source_rgb(cr, 0.5, 1, 1);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_stroke(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to (cr, (float)(x0 - op_char.x - op_char.width / 2),
                       (float)(y0 - op_char.y - op_char.height / 2));
    pango_layout_set_text (alg_layout, buf, -1);
    pango_cairo_show_layout (cr, alg_layout);
}

static void
alg_draw_connect(cairo_t *cr, int in_x, int in_y, int out_x, int out_y)
{
    int in_x0    = alg_x_base + alg_x_stride * in_x;
    int in_y0    = alg_y_base - alg_y_stride * in_y;
    int in_yin   = in_y0 - op_height / 2;
    int ymid     = in_y0 - alg_y_stride / 2 - 1;
    int out_x0   = alg_x_base + alg_x_stride * out_x;
    int out_y0   = alg_y_base - alg_y_stride * out_y;
    int out_yout = out_y0 + op_height / 2;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, (float)in_x0 + 0.5f, (float)in_yin + 0.5f);
    if (in_x != out_x) {
        cairo_line_to(cr, (float)in_x0 + 0.5f, (float)ymid + 0.5f);
        cairo_line_to(cr, (float)out_x0 + 0.5f, (float)ymid + 0.5f);
    }
    cairo_line_to(cr, (float)out_x0 + 0.5f, (float)out_yout + 0.5f);
    cairo_stroke(cr);
}

static void
alg_draw_bus(cairo_t *cr, int a_x, int b_x, int y)
{
    int a_x0    = alg_x_base + alg_x_stride * a_x;
    int b_x0    = alg_x_base + alg_x_stride * b_x;
    int y0      = alg_y_base - alg_y_stride * y;
    int yout    = y0 + op_height / 2;
    int ybus    = y0 + alg_y_stride / 2 + 1;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, (float)a_x0 + 0.5f, (float)yout + 0.5f);
    cairo_line_to(cr, (float)a_x0 + 0.5f, (float)ybus + 0.5f);
    if (a_x != b_x) {
        cairo_line_to(cr, (float)b_x0 + 0.5f, (float)ybus + 0.5f);
        cairo_line_to(cr, (float)b_x0 + 0.5f, (float)yout + 0.5f);
    }
    cairo_stroke(cr);
}

static void
alg_draw_feedback(cairo_t *cr, int in_x, int in_y, int out_x, int out_y)
{
    int in_x0     = alg_x_base + alg_x_stride * in_x;
    int in_y0     = alg_y_base - alg_y_stride * in_y;
    int in_yin    = in_y0 - op_height / 2;
    int in_ytop   = in_y0 - alg_y_stride / 2 - 1;
    int in_xright = in_x0 + alg_x_stride / 2 + 1;
    int out_x0     = alg_x_base + alg_x_stride * out_x;
    int out_y0     = alg_y_base - alg_y_stride * out_y;
    int out_xright = in_xright;
    int out_xout   = out_x0 + op_width / 2;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, (float)in_x0 + 0.5f, (float)in_yin + 0.5f);
    cairo_line_to(cr, (float)in_x0 + 0.5f, (float)in_ytop + 0.5f);
    cairo_line_to(cr, (float)in_xright + 0.5f, (float)in_ytop + 0.5f);
    cairo_line_to(cr, (float)out_xright + 0.5f, (float)out_y0 + 0.5f);
    cairo_line_to(cr, (float)out_xout + 0.5f, (float)out_y0 + 0.5f);
    cairo_stroke(cr);
}

struct _algdef {
    char op, a, b;
};

#define OP_OP1       1   /* a is op x position, b is op y position */
#define OP_OP6       6
#define OP_CONNECT   7   /* a is 'in' or lower op, b is 'out' or upper op */
#define OP_BUS       8   /* a and b are ops to bus */
#define OP_FEEDBACK  9   /* a is 'in' or upper op, b is 'side' or lower op */

#define C   OP_CONNECT
#define B   OP_BUS
#define FB  OP_FEEDBACK

static struct _algdef *_algdef[32] = {
    (struct _algdef []){ { 1, 2, 0 }, { 2, 2, 1 }, { 3, 3, 0 }, { 4, 3, 1 }, { 5, 3, 2 }, { 6, 3, 3 },
    /*  1 */             { C, 1, 2 }, { C, 3, 4 }, { C, 4, 5 }, { C, 5, 6 }, { B, 1, 3 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 2, 0 }, { 2, 2, 1 }, { 3, 3, 0 }, { 4, 3, 1 }, { 5, 3, 2 }, { 6, 3, 3 },
    /*  2 */             { C, 1, 2 }, { C, 3, 4 }, { C, 4, 5 }, { C, 5, 6 }, { B, 1, 3 }, { FB, 2, 2 }, { 0 } },
    (struct _algdef []){ { 1, 2, 0 }, { 2, 2, 1 }, { 3, 2, 2 }, { 4, 3, 0 }, { 5, 3, 1 }, { 6, 3, 2 },
    /*  3 */             { C, 1, 2 }, { C, 2, 3 }, { C, 4, 5 }, { C, 5, 6 }, { B, 1, 4 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 2, 0 }, { 2, 2, 1 }, { 3, 2, 2 }, { 4, 3, 0 }, { 5, 3, 1 }, { 6, 3, 2 },
    /*  4 */             { C, 1, 2 }, { C, 2, 3 }, { C, 4, 5 }, { C, 5, 6 }, { B, 1, 4 }, { FB, 6, 4 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 0 }, { 4, 2, 1 }, { 5, 3, 0 }, { 6, 3, 1 },
    /*  5 */             { C, 1, 2 }, { C, 3, 4 }, { C, 5, 6 }, { B, 1, 3 }, { B, 3, 5 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 0 }, { 4, 2, 1 }, { 5, 3, 0 }, { 6, 3, 1 },
    /*  6 */             { C, 1, 2 }, { C, 3, 4 }, { C, 5, 6 }, { B, 1, 3 }, { B, 3, 5 }, { FB, 6, 5 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 0 }, { 4, 2, 1 }, { 5, 3, 1 }, { 6, 3, 2 },
    /*  7 */             { C, 1, 2 }, { C, 3, 4 }, { C, 3, 5 }, { C, 5, 6 }, { B, 1, 3 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 0 }, { 4, 2, 1 }, { 5, 3, 1 }, { 6, 3, 2 },
    /*  8 */             { C, 1, 2 }, { C, 3, 4 }, { C, 3, 5 }, { C, 5, 6 }, { B, 1, 3 }, { FB, 4, 4 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 0 }, { 4, 2, 1 }, { 5, 3, 1 }, { 6, 3, 2 },
    /*  9 */             { C, 1, 2 }, { C, 3, 4 }, { C, 3, 5 }, { C, 5, 6 }, { B, 1, 3 }, { FB, 2, 2 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 1, 2 }, { 4, 2, 0 }, { 5, 2, 1 }, { 6, 3, 1 },
    /* 10 */             { C, 1, 2 }, { C, 2, 3 }, { C, 4, 5 }, { C, 4, 6 }, { B, 1, 4 }, { FB, 3, 3 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 1, 2 }, { 4, 2, 0 }, { 5, 2, 1 }, { 6, 3, 1 },
    /* 11 */             { C, 1, 2 }, { C, 2, 3 }, { C, 4, 5 }, { C, 4, 6 }, { B, 1, 4 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 3, 0 }, { 4, 2, 1 }, { 5, 3, 1 }, { 6, 4, 1 },
    /* 12 */             { C, 1, 2 }, { C, 3, 4 }, { C, 3, 5 }, { C, 3, 6 }, { B, 1, 3 }, { FB, 2, 2 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 3, 0 }, { 4, 2, 1 }, { 5, 3, 1 }, { 6, 4, 1 },
    /* 13 */             { C, 1, 2 }, { C, 3, 4 }, { C, 3, 5 }, { C, 3, 6 }, { B, 1, 3 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 0 }, { 4, 2, 1 }, { 5, 2, 2 }, { 6, 3, 2 },
    /* 14 */             { C, 1, 2 }, { C, 3, 4 }, { C, 4, 5 }, { C, 4, 6 }, { B, 1, 3 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 0 }, { 4, 2, 1 }, { 5, 2, 2 }, { 6, 3, 2 },
    /* 15 */             { C, 1, 2 }, { C, 3, 4 }, { C, 4, 5 }, { C, 4, 6 }, { B, 1, 3 }, { FB, 2, 2 }, { 0 } },
    (struct _algdef []){ { 1, 2, 0 }, { 2, 1, 1 }, { 3, 2, 1 }, { 4, 2, 2 }, { 5, 3, 1 }, { 6, 3, 2 },
    /* 16 */             { C, 1, 2 }, { C, 1, 3 }, { C, 3, 4 }, { C, 1, 5 }, { C, 5, 6 }, { B, 1, 1 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 2, 0 }, { 2, 1, 1 }, { 3, 2, 1 }, { 4, 2, 2 }, { 5, 3, 1 }, { 6, 3, 2 },
    /* 17 */             { C, 1, 2 }, { C, 1, 3 }, { C, 3, 4 }, { C, 1, 5 }, { C, 5, 6 }, { B, 1, 1 }, { FB, 2, 2 }, { 0 } },
    (struct _algdef []){ { 1, 2, 0 }, { 2, 1, 1 }, { 3, 2, 1 }, { 4, 3, 1 }, { 5, 3, 2 }, { 6, 3, 3 },
    /* 18 */             { C, 1, 2 }, { C, 1, 3 }, { C, 1, 4 }, { C, 4, 5 }, { C, 5, 6 }, { B, 1, 1 }, { FB, 3, 3 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 1, 2 }, { 4, 2, 0 }, { 5, 3, 0 }, { 6, 2, 1 },
    /* 19 */             { C, 1, 2 }, { C, 2, 3 }, { C, 4, 6 }, { C, 5, 6 }, { B, 1, 4 }, { B, 4, 5 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 2, 0 }, { 3, 1, 1 }, { 4, 3, 0 }, { 5, 3, 1 }, { 6, 4, 1 },
    /* 20 */             { C, 1, 3 }, { C, 2, 3 }, { C, 4, 5 }, { C, 4, 6 }, { B, 1, 2 }, { B, 2, 4 }, { FB, 3, 3 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 2, 0 }, { 3, 1, 1 }, { 4, 3, 0 }, { 5, 4, 0 }, { 6, 3, 1 },
    /* 21 */             { C, 1, 3 }, { C, 2, 3 }, { C, 4, 6 }, { C, 5, 6 }, { B, 1, 2 }, { B, 2, 4 }, { B, 4, 5 }, { FB, 3, 3 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 0 }, { 4, 3, 0 }, { 5, 4, 0 }, { 6, 3, 1 },
    /* 22 */             { C, 1, 2 }, { C, 3, 6 }, { C, 4, 6 }, { C, 5, 6 }, { B, 1, 3 }, { B, 3, 4 }, { B, 4, 5 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 2, 0 }, { 3, 2, 1 }, { 4, 3, 0 }, { 5, 4, 0 }, { 6, 3, 1 },
    /* 23 */             { C, 2, 3 }, { C, 4, 6 }, { C, 5, 6 }, { B, 1, 2 }, { B, 2, 4 }, { B, 4, 5 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 2, 0 }, { 3, 3, 0 }, { 4, 4, 0 }, { 5, 5, 0 }, { 6, 4, 1 },
    /* 24 */             { C, 3, 6 }, { C, 4, 6 }, { C, 5, 6 }, { B, 1, 2 }, { B, 2, 3 }, { B, 3, 4 }, { B, 4, 5 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 2, 0 }, { 3, 3, 0 }, { 4, 4, 0 }, { 5, 5, 0 }, { 6, 4, 1 },
    /* 25 */             { C, 4, 6 }, { C, 5, 6 }, { B, 1, 2 }, { B, 2, 3 }, { B, 3, 4 }, { B, 4, 5 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 2, 0 }, { 3, 2, 1 }, { 4, 3, 0 }, { 5, 3, 1 }, { 6, 4, 1 },
    /* 26 */             { C, 2, 3 }, { C, 4, 5 }, { C, 4, 6 }, { B, 1, 2 }, { B, 2, 4 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 2, 0 }, { 3, 2, 1 }, { 4, 3, 0 }, { 5, 3, 1 }, { 6, 4, 1 },
    /* 27 */             { C, 2, 3 }, { C, 4, 5 }, { C, 4, 6 }, { B, 1, 2 }, { B, 2, 4 }, { FB, 3, 3 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 1, 1 }, { 3, 2, 0 }, { 4, 2, 1 }, { 5, 2, 2 }, { 6, 3, 0 },
    /* 28 */             { C, 1, 2 }, { C, 3, 4 }, { C, 4, 5 }, { B, 1, 3 }, { B, 3, 6 }, { FB, 5, 5 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 2, 0 }, { 3, 3, 0 }, { 4, 3, 1 }, { 5, 4, 0 }, { 6, 4, 1 },
    /* 29 */             { C, 3, 4 }, { C, 5, 6 }, { B, 1, 2 }, { B, 2, 3 }, { B, 3, 5 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 2, 0 }, { 3, 3, 0 }, { 4, 3, 1 }, { 5, 3, 2 }, { 6, 4, 0 },
    /* 30 */             { C, 3, 4 }, { C, 4, 5 }, { B, 1, 2 }, { B, 2, 3 }, { B, 3, 6 }, { FB, 5, 5 }, { 0 } },
    (struct _algdef []){ { 1, 1, 0 }, { 2, 2, 0 }, { 3, 3, 0 }, { 4, 4, 0 }, { 5, 5, 0 }, { 6, 5, 1 },
    /* 31 */             { C, 5, 6 }, { B, 1, 2 }, { B, 2, 3 }, { B, 3, 4 }, { B, 4, 5 }, { FB, 6, 6 }, { 0 } },
    (struct _algdef []){ { 1, 0, 0 }, { 2, 1, 0 }, { 3, 2, 0 }, { 4, 3, 0 }, { 5, 4, 0 }, { 6, 5, 0 },
    /* 32 */             { B, 1, 2 }, { B, 2, 3 }, { B, 3, 4 }, { B, 4, 5 }, { B, 5, 6 }, { FB, 6, 6 }, { 0 } },
};

#undef C
#undef B
#undef FB

void
we_alg_draw(cairo_t *cr, int alg)
{
    struct _algdef *algdef;
    int i, op_x[7], op_y[7];

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    if (alg < 0 || alg >= 32)
        return;

    algdef = _algdef[alg];
    for (i = 0; algdef[i].op; i++) {
        int op = algdef[i].op;

        if (op >= OP_OP1 && op <= OP_OP6) {
            op_x[op] = algdef[i].a;
            op_y[op] = algdef[i].b;
            alg_draw_op(cr, op_x[op], op_y[op], op);
        } else if (op == OP_CONNECT) {
            alg_draw_connect(cr, op_x[(int)algdef[i].a], op_y[(int)algdef[i].a],
                                 op_x[(int)algdef[i].b], op_y[(int)algdef[i].b]);
        } else if (op == OP_BUS) {
            alg_draw_bus(cr, op_x[(int)algdef[i].a], op_x[(int)algdef[i].b],
                             op_y[(int)algdef[i].a]);
        } else if (op == OP_FEEDBACK) {
            alg_draw_feedback(cr, op_x[(int)algdef[i].a], op_y[(int)algdef[i].a],
                                  op_x[(int)algdef[i].b], op_y[(int)algdef[i].b]);
        }
    }
}
