/* hexter DSSI software synthesizer text-mode UI
 *
 * Copyright (C) 2004, 2009, 2012 Sean Bolton and others.
 *
 * Portions of this file came from readline 4.3, copyright
 * (c) 1987-2002 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <dssi.h>

#include "hexter.h"
#include "gui_main.h"
#include "gui_data.h"
#include "dx7_voice.h"
#include "dx7_voice_data.h"

extern int done;

static unsigned char test_note_state = 0;
static unsigned char test_note_noteon_key = 60;
static unsigned char test_note_noteoff_key;
static unsigned char test_note_velocity = 84;
static int           monophonic_mode = 0;
static int           polyphony_instance = HEXTER_DEFAULT_POLYPHONY;
static int           polyphony_global = HEXTER_DEFAULT_POLYPHONY;

static char *monophonic_modes[] = {
    "off", "on", "once", "both", NULL
};

/* ==== commands ==== */

void
command_status(char *args)
{
    printf("hexter instance id: %s\n", user_friendly_id);
    printf("UI URL: %s\n", osc_self_url);
    printf("host URL: %s\n", osc_host_url);
    printf("test note: key %d, velocity %d, currently %s\n", test_note_noteon_key,
           test_note_velocity, test_note_state ? "on" : "off");
    printf("monophonic mode: %s\n", monophonic_modes[monophonic_mode]);
    printf("polyphony: instance %d, global %d\n", polyphony_instance, polyphony_global);
    printf("current program: %d\n", current_program);
}

void
command_patches(char *args)
{
    char name[11];
    int i, j, n;

    for (i = 0; i < 32; i++) {
        for (j = 0; j < 128; j += 32) {
            n = i + j;
            dx7_voice_copy_name(name, &patches[n]);
            printf("%c%3d %s", (n == current_program ? '*': ' '), n, name);
            if (j < 96)
                printf("  ");
            else
                printf("\n");
        }
    }
}

void
command_program(char *args)
{
    int prog;
    char *tail;

    if (!args || !*args) {
        printf("usage: 'p <patch number>'\n");
        return;
    }
    prog = strtol(args, &tail, 10);
    if (prog == 0 && args == tail) {
        printf("invalid patch number\n");
        return;
    }
 
    current_program = prog;

    /* set all the patch edit widgets to match */
    /* update_voice_widgets_from_patch(&patches[current_program]); */

    lo_send(osc_host_address, osc_program_path, "ii", 0, current_program);

    if (edit_buffer_active && current_program != edit_buffer.program) {

        gui_data_send_edit_buffer_off();

        /* -FIX- update UI display to say "no active edits" */

        edit_buffer_active = 0;
    }
}

void
command_load(char *args)
{
    int position = 0; /* -FIX- need load position support */
    char *message;

    if (gui_data_load(args, position, &message)) {

        /* successfully loaded at least one patch */
        printf("Load Patch File succeeded:\n%s\n", message);
        gui_data_send_dirty_patch_sections();

    } else {  /* didn't load anything successfully */

        printf("Load Patch File failed:\n%s\n", message);

    }
    free(message);
}

void
command_mono(char *args)
{
    if (!args) {
  usage:
        printf("bad monophonic mode, try 'on', 'off', 'once', or 'both'\n");
        return;
    } else if (!strcmp(args, "off")) {
        monophonic_mode = 0;
    } else if (!strcmp(args, "on")) {
        monophonic_mode = 1;
    } else if (!strcmp(args, "once")) {
        monophonic_mode = 2;
    } else if (!strcmp(args, "both")) {
        monophonic_mode = 3;
    } else {
        goto usage;
    }

    lo_send(osc_host_address, osc_configure_path, "ss", "monophonic",
            monophonic_modes[monophonic_mode]);
}

void
command_poly(char *args)
{
    int poly;
    char buffer[4];

    if (!args || !*args) {
        printf("usage: 'poly <voice count>'\n");
        return;
    }
    poly = atoi(args);

    if (poly > 0 && poly < HEXTER_MAX_POLYPHONY) {
        polyphony_instance = poly;

        snprintf(buffer, 4, "%d", poly);
        lo_send(osc_host_address, osc_configure_path, "ss", "polyphony", buffer);
    }
}

void
command_gpoly(char *args)
{
    int poly;
    char buffer[4];

    if (!args || !*args) {
        printf("usage: 'gpoly <voice count>'\n");
        return;
    }
    poly = atoi(args);

    if (poly > 0 && poly < HEXTER_MAX_POLYPHONY) {
        polyphony_global = poly;

        snprintf(buffer, 4, "%d", poly);
#ifdef DSSI_GLOBAL_CONFIGURE_PREFIX
        lo_send(osc_host_address, osc_configure_path, "ss",
                DSSI_GLOBAL_CONFIGURE_PREFIX "polyphony", buffer);
#else
        lo_send(osc_host_address, osc_configure_path, "ss", "global_polyphony",
                buffer);
#endif
    }
}

void
command_quit(char *args)
{
    done = 1;
}

/* ==== command line parsing ==== */

/* A structure which contains information on the commands this program
   can understand. */

typedef struct {
  char *name;			/* User printable name of the function. */
  rl_vcpfunc_t *func;		/* Function to call to do the job. */
  char *doc;			/* Documentation for this function.  */
} COMMAND;

void command_help(char *);

static COMMAND commands[] = {
    { "status",  command_status, "show hexter status" },
    { "patches", command_patches, "show patch names" },
    { "pc",      command_program, "select patch (e.g. 'pc 3')" },
    { "load",    command_load,   "load patch bank (e.g. 'load <filename>')" },
    { "mono",    command_mono, "set monophonic mode (e.g. 'mono on')" },
    { "poly",    command_poly, "set instance polyphony ('poly 8')" },
    { "gpoly",   command_gpoly, "set global polyphony ('gpoly 16')" },
    { "quit",    command_quit, "exit hexter_text" },
    { "help",    command_help, "print this help message" },
    { (char *)NULL, (rl_vcpfunc_t *)NULL, (char *)NULL }
};

void
command_help(char *args)
{
    int i, len, maxlen = 0;

    printf("hexter_text help - commands available are:\n");

    for (i = 0; commands[i].name; i++) {
        len = strlen(commands[i].name);
        if (len > maxlen) maxlen = len;
    }

    for (i = 0; commands[i].name; i++) {
        printf("  %s", commands[i].name);
        for (len = strlen(commands[i].name); len <= maxlen; len++)
            printf(" ");
        printf("- %s\n", commands[i].doc);
    }

    printf("ctrl-t starts or stops a test note\n");
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char *
stripwhite (string)
     char *string;
{
  register char *s, *t;

  for (s = string; whitespace (*s); s++)
    ;
    
  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */
COMMAND *
find_command (name)
     char *name;
{
  register int i;

  for (i = 0; commands[i].name; i++)
    if (strcmp (name, commands[i].name) == 0)
      return (&commands[i]);

  return ((COMMAND *)NULL);
}

void
readline_callback(char *line)
{
    char *s, *args;
    int i;
    COMMAND *command;

    if (!line || !*line) return;

    s = stripwhite (line);
    if (*s) {
        add_history (s);

        for (i = 0; s[i] && !whitespace(s[i]); i++);

        if (line[i])
            line[i++] = '\0';

        command = find_command(s);

        if (command) {
            if (line[i]) {
               args = stripwhite(s + i);
            } else {
                args = NULL;
            }
            ((*(command->func)) (args));
        } else {
            printf("unrecognized command '%s'\n", s);
            command_help(NULL);
        }
    }

    free(line);
}

/* ==== test note ==== */

int
test_note_callback(int a, int b)
{
    unsigned char midi[4];

    if (!test_note_state) {  /* send note on */

        midi[0] = 0;
        midi[1] = 0x90;
        midi[2] = test_note_noteon_key;
        midi[3] = test_note_velocity;
        lo_send(osc_host_address, osc_midi_path, "m", midi);
        test_note_noteoff_key = test_note_noteon_key;
        test_note_state = 1;

    } else { /* send note off */

        midi[0] = 0;
        midi[1] = 0x80;
        midi[2] = test_note_noteoff_key;
        midi[3] = 0x40;
        lo_send(osc_host_address, osc_midi_path, "m", midi);
        test_note_state = 0;

    }

    return 0;
}

/* ==== OSC update handlers ==== */

void
update_from_program_select(unsigned long bank, unsigned long program)
{
    if (!bank && program < 128) {

        current_program = program;

        printf("\ncurrent program changed to %d\n", current_program);

        if (edit_buffer_active && program != edit_buffer.program) {

            gui_data_send_edit_buffer_off();

            /* -FIX- update UI display to say "no active edits" */

            edit_buffer_active = 0;
        }
    }
}

void
update_patches(const char *key, const char *value)
{
    int section = key[7] - '0';

    TUIDB_MESSAGE(DB_OSC, ": update_patches: received new '%s'\n", key);

    if (section < 0 || section > 3)
        return;

    if (!decode_7in6(value, 32 * sizeof(dx7_patch_t),
                     (uint8_t *)&patches[section * 32])) {
        TUIDB_MESSAGE(DB_OSC, " update_patches: corrupt data!\n");
        return;
    }

    patch_section_dirty[section] = 0;

    /* patch names may have changed, so if the UI is displaying a patch list,
     * it should be updated now. */
}

void
update_monophonic(const char *value)
{
    TUIDB_MESSAGE(DB_OSC, ": update_monophonic called with '%s'\n", value);

    if (!strcmp(value, "off")) {
        monophonic_mode = 0;
    } else if (!strcmp(value, "on")) {
        monophonic_mode = 1;
    } else if (!strcmp(value, "once")) {
        monophonic_mode = 2;
    } else if (!strcmp(value, "both")) {
        monophonic_mode = 3;
    } else {
        return;
    }

    /* if the UI is displaying the mono mode, it should be updated now. */
}

void
update_polyphony(const char *value)
{
    int poly = atoi(value);

    TUIDB_MESSAGE(DB_OSC, ": update_polyphony called with '%s'\n", value);

    if (poly > 0 && poly < HEXTER_MAX_POLYPHONY) {

        polyphony_instance = poly;

        /* if the UI is displaying the instance polyphony, it should be updated now. */
    }
}

void
update_global_polyphony(const char *value)
{
    int poly = atoi(value);

    TUIDB_MESSAGE(DB_OSC, ": update_global_polyphony called with '%s'\n", value);

    if (poly > 0 && poly < HEXTER_MAX_POLYPHONY) {

        polyphony_global = poly;

        /* if the UI is displaying the global polyphony, it should be updated now. */
    }
}

