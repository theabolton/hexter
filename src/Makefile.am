## Process this file with automake to produce Makefile.in

if BUILD_TEXTUI
textui_PROGRAMS = hexter_text
endif
if BUILD_GTKUI
gtkui_PROGRAMS = hexter_gtk
endif

textuidir = $(libdir)/dssi/hexter
gtkuidir = $(libdir)/dssi/hexter

plugindir = $(libdir)/dssi

plugin_LTLIBRARIES = hexter.la

hexter_gtk_SOURCES = \
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
	gui_midi.h \
	gui_patch_edit.c \
	gui_patch_edit.h \
	gui_retro_console.c \
	gui_retro_console.h \
	gui_retro_editor.c \
	gui_widgy_alg.c \
	gui_widgy_editor.c

hexter_gtk_CFLAGS = @GTK_CFLAGS@ $(AM_CFLAGS) @ALSA_CFLAGS@

hexter_gtk_LDADD = @GTK_LIBS@ @ALSA_LIBS@ -lm

hexter_text_SOURCES = \
	../extra/textui_main.c \
	dx7_voice_data.c \
	dx7_voice_data.h \
	dx7_voice_patches.c \
	gui_data.c \
	gui_data.h \
	gui_main.h \
	../extra/textui_callbacks.c \
	../extra/textui_callbacks.h

hexter_text_CFLAGS = $(AM_CFLAGS) @ALSA_CFLAGS@

hexter_text_LDADD = @READLINE_LIBS@ @ALSA_LIBS@

hexter_la_SOURCES = \
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

hexter_la_LIBADD = -lm

hexter_la_LDFLAGS = -module -avoid-version

