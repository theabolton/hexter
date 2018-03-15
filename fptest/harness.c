/* hexter fixed-point/floating-point speed test harness
 *
 * Copyright (C) 2011 Sean Bolton.
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
#include <sys/time.h>
#include <sys/resource.h>

#include <ladspa.h>
#include <dssi.h>

#include "hexter_types.h"
#include "hexter.h"

#define VERSION  "0.1"

#define SAMPLE_RATE  44100

#define FIX   0
#define FLOAT 1

/* in hexter.c: */
const DSSI_Descriptor *dssi_descriptor_fix(unsigned long index);
const DSSI_Descriptor *dssi_descriptor_float(unsigned long index);

static const DSSI_Descriptor *descriptor[2];
static LADSPA_Handle handle[2];

static float output[HEXTER_NUGGET_SIZE];
static float port[3] = { 0.0f, 440.0f, -12.0f };

static int samples[2], note[2], delay[2];

static snd_seq_event_t event;
static snd_seq_event_t *events[1] = { &event };
static unsigned long nevents[2] = { 0, 1 };

static struct rusage before, after;
static double usage[2];

void
setup_plugin(int index, const DSSI_Descriptor *d)
{
    char *err;

    /* instantiate */
    handle[index] = d->LADSPA_Plugin->instantiate(d->LADSPA_Plugin, SAMPLE_RATE);
    if (handle[index] == NULL) {
        printf("instantiate() failed for %d!\n", index);
        exit(1);
    }

    /* connect ports */
    d->LADSPA_Plugin->connect_port(handle[index], HEXTER_PORT_OUTPUT, output);
    d->LADSPA_Plugin->connect_port(handle[index], HEXTER_PORT_TUNING, port + 1);
    d->LADSPA_Plugin->connect_port(handle[index], HEXTER_PORT_VOLUME, port + 2);

    /* activate */
    d->LADSPA_Plugin->activate(handle[index]);

    /* configure */
    err = d->configure(handle[index], "polyphony", "32");
    if (err) {
        printf("configure(..., \"polyphony\", \"32\") failed for %d\n", index);
        exit(1);
    }

    /* select program */
    d->select_program(handle[index], 0, 0);
}

void
run_plugin(int i, int count)
{
    while (samples[i] < count) {
        int start_note = 0;

        if (samples[i] >= delay[i]) {
            start_note = 1;
            event.data.note.note = note[i];
            // printf("starting note %d at %d for index %d\n", note[i], samples[i], i);
            note[i] += 2;
            if (note[i] > 100) note[i] = 36;
            delay[i] += SAMPLE_RATE / 8;
        }

        if (start_note) {
            descriptor[i]->run_multiple_synths(1, &handle[i], HEXTER_NUGGET_SIZE, events, &nevents[1]);
        } else {
            descriptor[i]->run_multiple_synths(1, &handle[i], HEXTER_NUGGET_SIZE, events, &nevents[0]);
        }
        samples[i] += HEXTER_NUGGET_SIZE;
    }
}

int
main(int argc, char **argv)
{
    int i, n;

    printf("hexter fixed-point/floating-point speed test, version " VERSION ".\n");
    printf("testing 32 voices for 20 seconds at %d samples/second.\n", SAMPLE_RATE);

    /* get descriptors */
    descriptor[FIX] = dssi_descriptor_fix(0);
    if (descriptor[FIX] == NULL) {
        printf("dssi_descriptor_fix() failed!\n");
        exit(1);
    }
    descriptor[FLOAT] = dssi_descriptor_float(0);
    if (descriptor[FLOAT] == NULL) {
        printf("dssi_descriptor_float() failed!\n");
        exit(1);
    }

    setup_plugin(FIX,   descriptor[FIX]);
    setup_plugin(FLOAT, descriptor[FLOAT]);

    event.type = SND_SEQ_EVENT_NOTEON;
    event.data.note.channel = 0;
    event.data.note.velocity = 80;
    event.time.tick = 0;

    for (i = 0; i < 2; i++) {
        samples[i] = 0;
        note[i] = 36;
        delay[i] = 0;
        usage[i] = 0.0;
    }

    printf("priming voices and cache...\n");
    for (n = 1; n <= 4; n++) {
        printf("  %d\n", n);
        for (i = 0; i < 2; i++) {
            run_plugin(i, n * SAMPLE_RATE);
        }
    }

    printf("timing run...\n");
    for (n = 6; n <= 24; n += 2) {
        double run[2];
        for (i = 0; i < 2; i++) {
            getrusage(RUSAGE_SELF, &before);
            run_plugin(i, n * SAMPLE_RATE);
            getrusage(RUSAGE_SELF, &after);
            run[i] = ((double)after.ru_utime.tv_sec + (double)after.ru_utime.tv_usec / 1000000.0 +
                      (double)after.ru_stime.tv_sec + (double)after.ru_stime.tv_usec / 1000000.0) -
                     ((double)before.ru_utime.tv_sec + (double)before.ru_utime.tv_usec / 1000000.0 +
                      (double)before.ru_stime.tv_sec + (double)before.ru_stime.tv_usec / 1000000.0);
            usage[i] += run[i];
        }
        printf("  %d: run: %f fix, %f float, %5.1f%%, total: %f fix, %f float\n",
               n, run[FIX], run[FLOAT], 100.0 * run[FIX] / run[FLOAT], usage[FIX], usage[FLOAT]);
    }

    if (usage[FIX] > usage[FLOAT]) {
        printf("On this system, with this compilation, it seems that floating point is faster:\n");
        printf("Fixed point:     %7f seconds  -> %.1f%% as fast as floating point\n", usage[FIX], 100.0 * usage[FLOAT] / usage[FIX]);
        printf("Floating point:  %7f seconds  -> faster mode\n", usage[FLOAT]);
    } else {
        printf("On this system, with this compilation, it seems that fixed point is faster:\n");
        printf("Fixed point:     %7f seconds  -> faster mode\n", usage[FIX]);
        printf("Floating point:  %7f seconds  -> %.1f%% as fast as fixed point\n", usage[FLOAT], 100.0 * usage[FIX] / usage[FLOAT]);
    }

    return 0;
}
