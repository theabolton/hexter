## Process this file with automake to produce Makefile.in

if BUILD_TEXTUI
ui_PROGRAMS = hexter6_text
else
ui_PROGRAMS = hexter6_gtk
endif

uidir = $(libdir)/dssi/hexter6

plugindir = $(libdir)/dssi

plugin_LTLIBRARIES = hexter6.la

hexter6_gtk_SOURCES = \
	gui_main.c \
	bitmap_about.xpm \
	dx7_voice_data.c \
	dx7_voice_data.h \
	dx7_voice_patches.c \
	gui_callbacks.c \
	gui_callbacks.h \
	gui_data.c \
	gui_data.h \
	gui_images.c \
	gui_images.h \
	gui_interface.c \
	gui_interface.h \
	gui_main.h \
	gui_midi.c \
	gui_midi.h

hexter6_gtk_CFLAGS = @GTK_CFLAGS@ $(AM_CFLAGS) @ALSA_CFLAGS@

hexter6_gtk_LDADD = @GTK_LIBS@ @ALSA_LIBS@

hexter6_text_SOURCES = \
	../extra/textui_main.c \
	dx7_voice_data.c \
	dx7_voice_data.h \
	dx7_voice_patches.c \
	gui_data.c \
	gui_data.h \
	gui_main.h \
	../extra/textui_callbacks.c \
	../extra/textui_callbacks.h

hexter6_text_CFLAGS = $(AM_CFLAGS) @ALSA_CFLAGS@

hexter6_text_LDADD = @READLINE_LIBS@ @ALSA_LIBS@

hexter6_la_SOURCES = \
	hexter.c \
        dx7_voice.c \
        dx7_voice.h \
	dx7_voice_data.c \
	dx7_voice_data.h \
	dx7_voice_patches.c \
	dx7_voice_render.c \
	dx7_voice_tables.c \
	hexter_synth.c \
	hexter_synth.h \
	hexter_types.h \
        hexter.h

hexter6_la_LIBADD = -lm

hexter6_la_LDFLAGS = -module -avoid-version

