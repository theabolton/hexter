/* hexter DSSI software synthesizer GUI
 *
 * Copyright (C) 2004, 2009 Sean Bolton and others.
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

#include <gtk/gtk.h>

#include "gui_images.h"

#include "bitmap_about.xpm"

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
GtkWidget *
create_pixmap_from_xpm(GtkWidget *window, char *xpm_data[])
{
    GdkColormap *colormap;
    GdkBitmap   *mask;
    GdkPixmap   *gdkpixmap;
    GtkWidget   *pixmap;

    colormap = gtk_widget_get_colormap(window);
    gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask,
                    NULL, xpm_data);
    if (gdkpixmap == NULL) {
        g_warning("error creating pixmap");
        return create_dummy_pixmap (window);
    }
    pixmap = gtk_pixmap_new(gdkpixmap, mask);
    gdk_pixmap_unref(gdkpixmap);
    gdk_bitmap_unref(mask);
    return pixmap;
}

GtkWidget *
create_about_pixmap(GtkWidget *window)
{
    return create_pixmap_from_xpm(window, bitmap_about_xpm);
}
