/* hexter DSSI software synthesizer text-mode UI
 *
 * Copyright (C) 2004, 2009 Sean Bolton and others.
 *
 * Portions of this file may have come from Chris Cannam and Steve
 * Harris's public domain DSSI example code.
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
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <readline/readline.h>
#include <lo/lo.h>
#include <dssi.h>

#include "hexter_types.h"
#include "hexter.h"
#include "gui_main.h"
#include "textui_callbacks.h"
/* -FIX- #include "gui_midi.h" */
#include "gui_data.h"
#include "dx7_voice_data.h"

/* ==== global variables ==== */

char *user_friendly_id;

char *     osc_host_url;
char *     osc_self_url;
lo_address osc_host_address;
char *     osc_configure_path;
char *     osc_control_path;
char *     osc_exiting_path;
char *     osc_hide_path;
char *     osc_midi_path;
char *     osc_program_path;
char *     osc_quit_path;
char *     osc_show_path;
char *     osc_update_path;

dx7_patch_t  *patches = NULL;
int           patch_section_dirty[4];

int           current_program = 0;

int           edit_buffer_active = 0;
edit_buffer_t edit_buffer;
int           edit_receive_channel = 0;

int done = 0;
int host_requested_quit = 0;

/* ==== OSC handling ==== */

static char *
osc_build_path(char *base_path, char *method)
{
    char buffer[256];
    char *full_path;

    snprintf(buffer, 256, "%s%s", base_path, method);
    if (!(full_path = strdup(buffer))) {
        TUIDB_MESSAGE(DB_OSC, ": out of memory!\n");
        exit(1);
    }
    return strdup(buffer);
}

static void
osc_error(int num, const char *msg, const char *path)
{
    TUIDB_MESSAGE(DB_OSC, " error: liblo server error %d in path \"%s\": %s\n",
            num, (path ? path : "(null)"), msg);
}

int
osc_debug_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, lo_message msg, void *user_data)
{
    int i;

    TUIDB_MESSAGE(DB_OSC, " warning: unhandled OSC message to <%s>:\n", path);

    for (i = 0; i < argc; ++i) {
        printf("arg %d: type '%c': ", i, types[i]);
        lo_arg_pp((lo_type)types[i], argv[i]);
        printf("\n");
    }

    return 1;  /* try any other handlers */
}

int
osc_action_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, lo_message msg, void *user_data)
{
    if (!strcmp(user_data, "show")) {

        /* TUIDB_MESSAGE(DB_OSC, " osc_action_handler: received 'show' message\n"); */

    } else if (!strcmp(user_data, "hide")) {

        /* TUIDB_MESSAGE(DB_OSC, " osc_action_handler: received 'hide' message\n"); */

    } else if (!strcmp(user_data, "quit")) {

        /* TUIDB_MESSAGE(DB_OSC, " osc_action_handler: received 'quit' message\n"); */
        host_requested_quit = 1;
        done = 1;

    } else {

        return osc_debug_handler(path, types, argv, argc, msg, user_data);

    }
    return 0;
}

int
osc_configure_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, lo_message msg, void *user_data)
{
    char *key, *value;

    if (argc < 2) {
        TUIDB_MESSAGE(DB_OSC, " error: too few arguments to osc_configure_handler\n");
        return 1;
    }

    key   = &argv[0]->s;
    value = &argv[1]->s;

    if (strlen(key) == 8 && !strncmp(key, "patches", 7) &&
        key[7] >= '0' && key[7] <= '3') {

        update_patches(key, value);

    } else if (!strcmp(key, "edit_buffer")) {

        /* -FIX- update_edit_buffer(value); */

    } else if (!strcmp(key, "monophonic")) {

        update_monophonic(value);

    } else if (!strcmp(key, "polyphony")) {

        update_polyphony(value);

#ifdef DSSI_GLOBAL_CONFIGURE_PREFIX
    } else if (!strcmp(key, DSSI_GLOBAL_CONFIGURE_PREFIX "polyphony")) {
#else
    } else if (!strcmp(key, "global_polyphony")) {
#endif

        TUIDB_MESSAGE(DB_OSC, " osc_configure_handler: received unsupported global polyphony limit of %s\n", value);

#ifdef DSSI_PROJECT_DIRECTORY_KEY
    } else if (!strcmp(key, DSSI_PROJECT_DIRECTORY_KEY)) {

        /* -FIX- implement project directory key */

#endif
    } else {

        return osc_debug_handler(path, types, argv, argc, msg, user_data);

    }

    return 0;
}

int
osc_control_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, lo_message msg, void *user_data)
{
    int port;
    float value;

    if (argc < 2) {
        TUIDB_MESSAGE(DB_OSC, " error: too few arguments to osc_control_handler\n");
        return 1;
    }

    port = argv[0]->i;
    value = argv[1]->f;

    TUIDB_MESSAGE(DB_OSC, " osc_control_handler: control %d now %f\n", port, value);

    /* -FIX- update_voice_widget(port, value); */

    return 0;
}

int
osc_program_handler(const char *path, const char *types, lo_arg **argv,
                  int argc, lo_message msg, void *user_data)
{
    int bank, program;

    if (argc < 2) {
        TUIDB_MESSAGE(DB_OSC, " error: too few arguments to osc_program_handler\n");
        return 1;
    }

    bank = argv[0]->i;
    program = argv[1]->i;

    if (bank || program < 0 || program > 127) {
        TUIDB_MESSAGE(DB_OSC, ": out-of-range program select (bank %d, program %d)\n", bank, program);
        return 0;
    }

    TUIDB_MESSAGE(DB_OSC, " osc_program_handler: received program change, bank %d, program %d\n", bank, program);

    update_from_program_select(bank, program);

    return 0;
}

/* ==== main ==== */

int
main(int argc, char *argv[])
{
    char *host, *port, *path, *tmp_url, *rl_prompt;
    lo_server osc_server;
    int lo_fd, ret;
    fd_set fds;

    DSSP_DEBUG_INIT("hexter_text");

#ifdef DSSP_DEBUG
    TUIDB_MESSAGE(DB_MAIN, " starting (pid %d)...\n", getpid());
#else
    printf("hexter_text starting (pid %d)...\n", getpid());
#endif
    /* { int i; fprintf(stderr, "args:\n"); for(i=0; i<argc; i++) printf("%d: %s\n", i, argv[i]); } // debug */

    if (argc != 5) {
        printf("usage: %s <osc url> <plugin dllname> <plugin label> <user-friendly id>\n", argv[0]);
        exit(1);
    }
    user_friendly_id = argv[4];

    /* set up OSC support */
    osc_host_url = argv[1];
    host = lo_url_get_hostname(osc_host_url);
    port = lo_url_get_port(osc_host_url);
    path = lo_url_get_path(osc_host_url);
    osc_host_address = lo_address_new(host, port);
    osc_configure_path = osc_build_path(path, "/configure");
    osc_control_path   = osc_build_path(path, "/control");
    osc_exiting_path   = osc_build_path(path, "/exiting");
    osc_hide_path      = osc_build_path(path, "/hide");
    osc_midi_path      = osc_build_path(path, "/midi");
    osc_program_path   = osc_build_path(path, "/program");
    osc_quit_path      = osc_build_path(path, "/quit");
    osc_show_path      = osc_build_path(path, "/show");
    osc_update_path    = osc_build_path(path, "/update");

    osc_server = lo_server_new(NULL, osc_error);
    lo_server_add_method(osc_server, osc_configure_path, "ss", osc_configure_handler, NULL);
    lo_server_add_method(osc_server, osc_control_path, "if", osc_control_handler, NULL);
    lo_server_add_method(osc_server, osc_hide_path, "", osc_action_handler, "hide");
    lo_server_add_method(osc_server, osc_program_path, "ii", osc_program_handler, NULL);
    lo_server_add_method(osc_server, osc_quit_path, "", osc_action_handler, "quit");
    lo_server_add_method(osc_server, osc_show_path, "", osc_action_handler, "show");
    lo_server_add_method(osc_server, NULL, NULL, osc_debug_handler, NULL);

    tmp_url = lo_server_get_url(osc_server);
    osc_self_url = osc_build_path(tmp_url, (strlen(path) > 1 ? path + 1 : path));
    free(tmp_url);

    /* get OSC server socket fd for main loop select() */
    lo_fd = lo_server_get_socket_fd(osc_server);
    if (lo_fd < 0) {
        printf("hexter_text fatal: OSC transport does not support exposing socket fd\n");
        exit(1);
    }

    /* set up patches */
    gui_data_patches_init();

    /* set up readline */
    rl_prompt = (char *)malloc(strlen(user_friendly_id) + 12);  // error handling....
    sprintf(rl_prompt, "hexter %s> ", user_friendly_id);
    rl_callback_handler_install (rl_prompt, readline_callback);
    rl_add_defun("test-note", test_note_callback, CTRL('t'));

    /* send our update request */
    lo_send(osc_host_address, osc_update_path, "s", osc_self_url);

    /* main loop */
    do {

        /* set up list of file descriptors to watch */
        FD_ZERO(&fds);
        FD_SET(0, &fds);  /* stdin */
        FD_SET(lo_fd, &fds);

        ret = select(lo_fd + 1, &fds, NULL, NULL, NULL);

        if (ret == -1) {

            printf("hexter_text fatal: main loop select() error!\n");
            done = 1;

        } else if (ret > 0) {

            if (FD_ISSET(0, &fds)) {

                rl_callback_read_char();

            }
            if (FD_ISSET(lo_fd, &fds)) {

                lo_server_recv_noblock(osc_server, 0);

            }

        }
    } while (!done);

    /* clean up and exit */
    TUIDB_MESSAGE(DB_MAIN, ": yep, we got to the cleanup!\n");

    /* shut down sys-ex receive, if enabled */
    /* -FIX- if (sysex_enabled) {
     *           sysex_stop();
     *       } */

    /* readline cleanup */
    rl_callback_handler_remove();

    /* say bye-bye */
    if (!host_requested_quit) {
        lo_send(osc_host_address, osc_exiting_path, "");
    }

    /* clean up patches */
    gui_data_patches_free();

    /* clean up OSC support */
    lo_server_free(osc_server);
    free(host);
    free(port);
    free(path);
    free(osc_configure_path);
    free(osc_control_path);
    free(osc_exiting_path);
    free(osc_hide_path);
    free(osc_midi_path);
    free(osc_program_path);
    free(osc_quit_path);
    free(osc_show_path);
    free(osc_update_path);
    free(osc_self_url);

    return 0;
}
