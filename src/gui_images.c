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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "gui_images.h"

#include "bitmap_about.xbm"

// #include "bitmap_logo.xbm"

/* This is a dummy pixmap we use when a pixmap can't be found. */
static char *dummy_pixmap_xpm[] = {
/* columns rows colors chars-per-pixel */
"1 1 1 1",
"  c None",
/* pixels */
" "
};

/* This is an internally used function to create pixmaps. */
static GtkWidget*
create_dummy_pixmap                    (GtkWidget       *widget)
{
  GdkColormap *colormap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;

  colormap = gtk_widget_get_colormap (widget);
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, &mask,
                                                     NULL, dummy_pixmap_xpm);
  if (gdkpixmap == NULL)
    g_error ("Couldn't create replacement pixmap.");
  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gdk_pixmap_unref (gdkpixmap);
  gdk_bitmap_unref (mask);
  return pixmap;
}

/* window must be realized before you call this */
static GtkWidget *
create_colored_pixmap_from_data(GtkWidget   *window,
                                const gchar *bitmap_data,
                                gint         bitmap_width,
                                gint         bitmap_height)
{
    GdkColormap *colormap;
    GdkColor     fg_color;
    GdkColor     bg_color;
    GdkPixmap   *gdkpixmap;
    GtkWidget   *pixmap;

    colormap = gtk_widget_get_colormap (window);
    fg_color.red = 0;
    fg_color.green = 0;
    fg_color.blue = 0;
    if (!gdk_color_alloc(colormap, &fg_color)) {
        g_warning("couldn't allocate fg color");
    }
    bg_color.red = 0x4848;
    bg_color.green = 0xf6f6;
    bg_color.blue = 0xfefe;
    if (!gdk_color_alloc(colormap, &bg_color)) {
        g_warning("couldn't allocate bg color");
    }
    gdkpixmap = gdk_pixmap_create_from_data ((GdkWindow *)(window->window),
                                             bitmap_data,
                                             bitmap_width,
                                             bitmap_height,
                                             -1,
                                             &fg_color,
                                             &bg_color);
    if (gdkpixmap == NULL) {
        g_warning("error creating pixmap");
        return create_dummy_pixmap (window);
    }
    pixmap = gtk_pixmap_new (gdkpixmap, NULL);
    gdk_pixmap_unref (gdkpixmap);
    return pixmap;
}

GtkWidget *
create_about_pixmap(GtkWidget *window)
{
    return create_colored_pixmap_from_data(window,
                                           bitmap_about_bits,
                                           bitmap_about_width,
                                           bitmap_about_height);
}

