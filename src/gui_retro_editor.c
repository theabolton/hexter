/* Yamaha DX7 / TX7 Editor/Librarian
 *
 * Copyright (C) 1991, 1995, 1997, 1998, 2004, 2009, 2011 Sean Bolton.
 *
 * This is a patch editor for the Yamaha DX7 and TX7. It started
 * out life on my Apple ][+, then I ported it to my Amiga, then to
 * my MS-DOS machine, then to Linux and ncurses, and most recently
 * to a simple GTK+ console emulation for integration into the hexter
 * DSSI soft synth.
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
#include <stdarg.h>
#include <ctype.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gui_callbacks.h"
#include "gui_interface.h"
#include "gui_patch_edit.h"
#include "gui_retro_console.h"

GtkWidget *retro_widget;

/* ==== Console ==== */

#define CONSOLE_WIDTH   74
#define CONSOLE_HEIGHT  21

enum textcolor {COLOR_MSG = 1, COLOR_LABEL, COLOR_DATA, COLOR_BLOCK,
                COLOR_CURSOR, COLOR_BCURSOR, 
                COLOR_OPON = COLOR_LABEL,
                COLOR_OPOFF = COLOR_MSG };

/* ==== VoiceEdit ==== */

static int re_Cursor = 0;
static int re_OpSelect = 63;

#define vptName     1   /* not used in hexter */
#define vpt0_99     2
#define vpt0_7      3
#define vptMode     4   /* R,F */
#define vptFC       5   /* 0-31 */
#define vptFF       6   /* 0-99 */
#define vptDetune   7   /* 0-14, as -7-+7 */
#define vpt0_3      8
#define vptAlg      9   /* 0-31, as 1-32 */
#define vptCurve   10   /* 0-3, scaling curve */
#define vptBkPt    11   /* 0-99, breakpoint */
#define vptTrans   12   /* 0-48, transpose */
#define vptOnOff   13
#define vptWave    14

static unsigned char re_parm_type_max[15] = {
    0, 0, 99, 7, 1, 31, 99, 14, 3, 31, 3, 99, 48, 1, 5,
};

static struct _reParm {
    guint8   Type;
    guint8   Offset;
    guint8   X;
    guint8   Y;
    guint8   Init;  /* init voice value */
} reParm[DX7_VOICE_PARAMETERS - 1 /* omitting name */] = {
/*     T,  Off,  X,  Y,  Init */
    {  2,  105,  5,  4,  0x62 }, /* O1 R1 */
    {  2,  109,  8,  4,  0x63 },
    {  2,  106, 12,  4,  0x63 },
    {  2,  110, 15,  4,  0x63 },
    {  2,  107, 19,  4,  0x63 },
    {  2,  111, 22,  4,  0x63 },
    {  2,  108, 26,  4,  0x5a },
    {  2,  112, 29,  4,  0x00 },
    {  3,  118, 33,  4,  0x00 }, /* O1 RS */
    {  4,  122, 36,  4,  0x00 },
    {  5,  123, 38,  4,  0x01 },
    {  6,  124, 41,  4,  0x00 },
    {  7,  125, 44,  4,  0x07 },
    {  2,  121, 54,  4,  0x63 },
    {  3,  120, 58,  4,  0x00 },
    {  8,  119, 60,  4,  0x00 }, /* O1 AMS */
    {  2,   84,  5,  5,  0x62 }, /* O2 R1 */
    {  2,   88,  8,  5,  0x63 },
    {  2,   85, 12,  5,  0x63 },
    {  2,   89, 15,  5,  0x63 },
    {  2,   86, 19,  5,  0x63 },
    {  2,   90, 22,  5,  0x63 },
    {  2,   87, 26,  5,  0x5a },
    {  2,   91, 29,  5,  0x00 },
    {  3,   97, 33,  5,  0x00 }, /* O2 RS */
    {  4,  101, 36,  5,  0x00 },
    {  5,  102, 38,  5,  0x01 },
    {  6,  103, 41,  5,  0x00 },
    {  7,  104, 44,  5,  0x07 },
    {  2,  100, 54,  5,  0x00 },
    {  3,   99, 58,  5,  0x00 },
    {  8,   98, 60,  5,  0x00 }, /* O2 AMS */
    {  2,   63,  5,  6,  0x62 }, /* O3 R1 */
    {  2,   67,  8,  6,  0x63 },
    {  2,   64, 12,  6,  0x63 },
    {  2,   68, 15,  6,  0x63 },
    {  2,   65, 19,  6,  0x63 },
    {  2,   69, 22,  6,  0x63 },
    {  2,   66, 26,  6,  0x5a },
    {  2,   70, 29,  6,  0x00 },
    {  3,   76, 33,  6,  0x00 }, /* O3 RS */
    {  4,   80, 36,  6,  0x00 },
    {  5,   81, 38,  6,  0x01 },
    {  6,   82, 41,  6,  0x00 },
    {  7,   83, 44,  6,  0x07 },
    {  2,   79, 54,  6,  0x00 },
    {  3,   78, 58,  6,  0x00 },
    {  8,   77, 60,  6,  0x00 }, /* O3 AMS */
    {  2,   42,  5,  7,  0x62 }, /* O4 R1 */
    {  2,   46,  8,  7,  0x63 },
    {  2,   43, 12,  7,  0x63 },
    {  2,   47, 15,  7,  0x63 },
    {  2,   44, 19,  7,  0x63 },
    {  2,   48, 22,  7,  0x63 },
    {  2,   45, 26,  7,  0x5a },
    {  2,   49, 29,  7,  0x00 },
    {  3,   55, 33,  7,  0x00 }, /* O4 RS */
    {  4,   59, 36,  7,  0x00 },
    {  5,   60, 38,  7,  0x01 },
    {  6,   61, 41,  7,  0x00 },
    {  7,   62, 44,  7,  0x07 },
    {  2,   58, 54,  7,  0x00 },
    {  3,   57, 58,  7,  0x00 },
    {  8,   56, 60,  7,  0x00 }, /* O4 AMS */
    {  2,   21,  5,  8,  0x62 }, /* O5 R1 */
    {  2,   25,  8,  8,  0x63 },
    {  2,   22, 12,  8,  0x63 },
    {  2,   26, 15,  8,  0x63 },
    {  2,   23, 19,  8,  0x63 },
    {  2,   27, 22,  8,  0x63 },
    {  2,   24, 26,  8,  0x5a },
    {  2,   28, 29,  8,  0x00 },
    {  3,   34, 33,  8,  0x00 }, /* O5 RS */
    {  4,   38, 36,  8,  0x00 },
    {  5,   39, 38,  8,  0x01 },
    {  6,   40, 41,  8,  0x00 },
    {  7,   41, 44,  8,  0x07 },
    {  2,   37, 54,  8,  0x00 },
    {  3,   36, 58,  8,  0x00 },
    {  8,   35, 60,  8,  0x00 }, /* O5 AMS */
    {  2,    0,  5,  9,  0x62 }, /* O6 R1 */
    {  2,    4,  8,  9,  0x63 },
    {  2,    1, 12,  9,  0x63 },
    {  2,    5, 15,  9,  0x63 },
    {  2,    2, 19,  9,  0x63 },
    {  2,    6, 22,  9,  0x63 },
    {  2,    3, 26,  9,  0x5a },
    {  2,    7, 29,  9,  0x00 },
    {  3,   13, 33,  9,  0x00 }, /* O6 RS */
    {  4,   17, 36,  9,  0x00 },
    {  5,   18, 38,  9,  0x01 },
    {  6,   19, 41,  9,  0x00 },
    {  7,   20, 44,  9,  0x07 },
    {  2,   16, 54,  9,  0x00 },
    {  3,   15, 58,  9,  0x00 },
    {  8,   14, 60,  9,  0x00 }, /* O6 AMS */
    {  2,  126,  5, 10,  0x63 }, /* P R1 */
    {  2,  130,  8, 10,  0x32 },
    {  2,  127, 12, 10,  0x63 },
    {  2,  131, 15, 10,  0x32 },
    {  2,  128, 19, 10,  0x63 },
    {  2,  132, 22, 10,  0x32 },
    {  2,  129, 26, 10,  0x63 },
    {  2,  133, 29, 10,  0x32 },
    {  9,  134, 48, 11,  0x00 }, /* Alg */
    {  2,  137, 35, 12,  0x23 },
    {  3,  135, 49, 12,  0x00 },
    { 10,  116,  5, 13,  0x00 }, /* O1 LC */
    {  2,  114, 10, 13,  0x00 },
    { 11,  113, 13, 13,  0x00 },
    { 10,  117, 18, 13,  0x00 },
    {  2,  115, 23, 13,  0x00 },
    {  2,  138, 35, 13,  0x00 },  /* Delay */
    { 12,  144, 46, 13,  0x18 },
    { 10,   95,  5, 14,  0x00 }, /* O2 LC */
    {  2,   93, 10, 14,  0x00 },
    { 11,   92, 13, 14,  0x00 },
    { 10,   96, 18, 14,  0x00 },
    {  2,   94, 23, 14,  0x00 },
    {  2,  139, 35, 14,  0x00 },  /* PMD */
    { 13,  136, 47, 14,  0x01 },
    { 10,   74,  5, 15,  0x00 }, /* O3 LC */
    {  2,   72, 10, 15,  0x00 },
    { 11,   71, 13, 15,  0x00 },
    { 10,   75, 18, 15,  0x00 },
    {  2,   73, 23, 15,  0x00 },
    {  2,  140, 35, 15,  0x00 },  /* AMD */
    { 10,   53,  5, 16,  0x00 }, /* O4 LC */
    {  2,   51, 10, 16,  0x00 },
    { 11,   50, 13, 16,  0x00 },
    { 10,   54, 18, 16,  0x00 },
    {  2,   52, 23, 16,  0x00 },
    { 13,  141, 34, 16,  0x01 },  /* Sync */
    { 10,   32,  5, 17,  0x00 }, /* O5 LC */
    {  2,   30, 10, 17,  0x00 },
    { 11,   29, 13, 17,  0x00 },
    { 10,   33, 18, 17,  0x00 },
    {  2,   31, 23, 17,  0x00 },
    { 14,  142, 34, 17,  0x00 },  /* Wave */
    { 10,   11,  5, 18,  0x00 }, /* O6 LC */
    {  2,    9, 10, 18,  0x00 },
    { 11,    8, 13, 18,  0x00 },
    { 10,   12, 18, 18,  0x00 },
    {  2,   10, 23, 18,  0x00 },
    {  3,  143, 36, 18,  0x03 }  /* PMS */
};

/* These should be the largest possible horizontal and vertical span of
 * parameter cursor positions, respectively.  If cursor motion wrap around
 * is not working like it should, check these. */
#define VOICEPARMS_X_SPAN  55
#define VOICEPARMS_Y_SPAN  14

/* ==== Algorithm ==== */

static char *veAlg[32] = {  /* 32 algs, max 9 high and 17 wide */
    "   '`\n"           /* 1 */
    "   6/\n"
    "   |\n"
    "   5\n"
    "   |\n"
    "2  4\n"
    "|  |\n"
    "1  3\n"
    "L--/",
    "\n"                /* 2 */
    "   6\n"
    "   |\n"
    "   5\n"
    "'` |\n"
    "2/ 4\n"
    "|  |\n"
    "1  3\n"
    "L--/",
    "   '`\n"           /* 3 */
    "3  6/\n"
    "|  |\n"
    "2  5\n"
    "|  |\n"
    "1  4\n"
    "L--/",
    "   '`\n"           /* 4 */
    "3  6|\n"
    "|  ||\n"
    "2  5|\n"
    "|  ||\n"
    "1  4/\n"
    "L--/",
    "      '`\n"        /* 5 */
    "2  4  6/\n"
    "|  |  |\n"
    "1  3  5\n"
    "L--^--/",
    "      '`\n"        /* 6 */
    "2  4  6|\n"
    "|  |  ||\n"
    "1  3  5/\n"
    "L--^--/",
    "      '`\n"        /* 7 */
    "      6/\n"
    "      |\n"
    "2  4  5\n"
    "|  }--/\n"
    "1  3\n"
    "L--/",
    "\n"                /* 8 */
    "      6\n"
    "   '` |\n"
    "2  4/ 5\n"
    "|  }--/\n"
    "1  3\n"
    "L--/",
    "\n"                /* 9 */
    "      6\n"
    "'`    |\n"
    "2/ 4  5\n"
    "|  }--/\n"
    "1  3\n"
    "L--/",
    "'`\n"              /* 10 */
    "3/\n"
    "|\n"
    "2  5  6\n"
    "|  }--/\n"
    "1  4\n"
    "L--/",
    "\n"                /* 11 */
    "3\n"
    "|     '`\n"
    "2  5  6/\n"
    "|  }--/\n"
    "1  4\n"
    "L--/",
    "'`\n"              /* 12 */
    "2/ 4  5  6\n"
    "|  L--+--/\n"
    "1     3\n"
    "L-----/",
    "         '`\n"     /* 13 */
    "2  4  5  6/\n" /* 13 */
    "|  L--+--/\n"
    "1     3\n"
    "L-----/",
    "      '`\n"        /* 14 */
    "   5  6/\n"
    "   }--/\n"
    "2  4\n"
    "|  |\n"
    "1  3\n"
    "L--/",
    "\n"                /* 15 */
    "   5  6\n"
    "'` }--/\n"
    "2/ 4\n"
    "|  |\n"
    "1  3\n"
    "L--/",
    "      '`\n"        /* 16 */
    "   4  6/\n"
    "   |  |\n"
    "2  3  5\n"
    "L--+--/\n"
    "   1",
    "\n"                /* 17 */
    "   4  6\n"
    "'` |  |\n"
    "2/ 3  5\n"
    "L--+--/\n"
    "   1",
    "      6\n"         /* 18 */
    "      |\n"
    "      5\n"
    "   '` |\n"
    "2  3/ 4\n"
    "L--+--/\n"
    "   1",
    "3\n"               /* 19 */
    "|  '`\n"
    "2  6/\n"
    "|  }--`\n"
    "1  4  5\n"
    "L--^--/",
    "'`\n"              /* 20 */
    "3/    5  6\n"
    "}--`  }--/\n"
    "1  2  4\n"
    "L--^--/",
    "'`\n"              /* 21 */
    "3/    6\n"
    "}--`  }--`\n"
    "1  2  4  5\n"
    "L--^--^--/",
    "      '`\n"        /* 22 */
    "2     6/\n"
    "|  '--+--`\n"
    "1  3  4  5\n"
    "L--^--^--/",
    "      '`\n"        /* 23 */
    "   3  6/\n"
    "   |  }--`\n"
    "1  2  4  5\n"
    "L--^--^--/",
    "         '`\n"     /* 24 */
    "         6/\n"
    "      '--+--`\n"
    "1  2  3  4  5\n"
    "L--^--^--^--/",
    "         '`\n"     /* 25 */
    "         6/\n"
    "         }--`\n"
    "1  2  3  4  5\n"
    "L--^--^--^--/",
    "         '`\n"     /* 26 */
    "   3  5  6/\n"
    "   |  }--/\n"
    "1  2  4\n"
    "L--^--/",
    "   '`\n"           /* 27 */
    "   3/ 5  6\n"
    "   |  }--/\n"
    "1  2  4\n"
    "L--^--/",
    "   '`\n"           /* 28 */
    "   5/\n"
    "   |\n"
    "2  4\n"
    "|  |\n"
    "1  3  6\n"
    "L--^--/",
    "         '`\n"     /* 29 */
    "      4  6/\n"
    "      |  |\n"
    "1  2  3  5\n"
    "L--^--^--/",
    "      '`\n"        /* 30 */
    "      5/\n"
    "      |\n"
    "      4\n"
    "      |\n"
    "1  2  3  6\n"
    "L--^--^--/",
    "            '`\n"
    "            6/\n"  /* 31 */
    "            |\n"
    "1  2  3  4  5\n"
    "L--^--^--^--/",
    "               '`" /* 32 */
    "1  2  3  4  5  6/"
    "L--^--^--^--^--/",
};

/* ==== Console Routines ==== */

static void
cerase(void)
{
    rc_console_erase();
}

static void
cmove(int y, int x) {
    rc_console_move(y, x);
}

static void
cprintf(const char *fmt, ...) {
    va_list ap;
    char tbuf[512];

    va_start(ap, fmt);
    (void) vsnprintf(tbuf, 512, fmt, ap);
    va_end(ap);

    rc_console_addstr(tbuf);
} 

static void
cputch(int c) {
    rc_console_addch(c);
}

static void
csetcolor(enum textcolor c)
{
    rc_console_set_color_pair(c);
}

//- void clrtoeol(void) { ; } // -FIX-
//- 
//- void
//- paktc(void)
//- {
//-     csetcolor(COLOR_MSG);
//-     cprintf(" Press Any Key to Continue... ");
//-     csetcolor(COLOR_DATA);
//- //x     cgetch();
//-     cprintf("\r");
//-     clrtoeol();
//- }
//- 
//- static void
//- gotomsg(void)
//- {
//-     cmove(LibRows + 4, 2);
//- }

/* ==== VoiceEdit Stuff ==== */

void
ve_ShowAlg(void)
{
    int alg = patch_edit_get_edit_parameter(134),
        x,
        y;
    char *s = veAlg[alg];
    int t;

    csetcolor(COLOR_LABEL);
    for (y = 11; y <= 19; y++) {
        x = 56;
	cmove(y, x);
        for (; x <= 72; x++) {
            switch (*s) {
                case '1':   t = (!!(re_OpSelect&32));  goto printop;
                case '2':   t = (!!(re_OpSelect&16));  goto printop;
                case '3':   t = (!!(re_OpSelect& 8));  goto printop;
                case '4':   t = (!!(re_OpSelect& 4));  goto printop;
                case '5':   t = (!!(re_OpSelect& 2));  goto printop;
                case '6':   t = (!!(re_OpSelect& 1));
                  printop:
                    csetcolor(t ? COLOR_OPON : COLOR_OPOFF);
                    cputch(*s++);
                    csetcolor(COLOR_LABEL);
                    break;
                case 0:
                    cputch(' ');
                    break;
                case '\n':
                    cputch(' ');
                    if (x == 72) s++;
                    break;
                case '^':
#define ACS_BTEE      151  /* '^' */
#define ACS_HLINE     146  /* '-' */
#define ACS_LLCORNER  142  /* 'L' */
#define ACS_LRCORNER  139  /* '/' */
#define ACS_LTEE      149  /* '}' */
#define ACS_PLUS      143  /* '+' */
#define ACS_ULCORNER  141  /* '/' */
#define ACS_URCORNER  140  /* '\\' */
#define ACS_VLINE     153  /* '|' */
                    cputch(ACS_BTEE); s++;
                    break;
                case '-':
                    cputch(ACS_HLINE); s++;
                    break;
                case 'L':
                    cputch(ACS_LLCORNER); s++;
                    break;
                case '/':
                    cputch(ACS_LRCORNER); s++;
                    break;
                case '}':
                    cputch(ACS_LTEE); s++;
                    break;
                case '+':
                    cputch(ACS_PLUS); s++;
                    break;
                /* case '?':
                 *     cputch(ACS_RTEE); s++;
                 *     break;
                 * case '?':
                 *     cputch(ACS_TTEE); s++;
                 *     break;
                 */
                case '\'':
                    cputch(ACS_ULCORNER); s++;
                    break;
                case '`':
                    cputch(ACS_URCORNER); s++;
                    break;
                case '|':
                    cputch(ACS_VLINE); s++;
                    break;
                default:
                    cputch(*s++);
                    break;
            }
        }
    }
    csetcolor(COLOR_DATA);
}

void
ve_FExtend(guint8 fc, guint8 ff, guint8 mode, guint8 y)
{
    unsigned long f;
    char ca[10];

    csetcolor(COLOR_LABEL);
    cmove(y, 47);
    if (mode) { /* fixed */
        f = peFFF[ff];
        switch (fc & 3) {
            case 1: f *=   10;    break;
            case 2: f *=  100;    break;
            case 3: f *= 1000;    break;
        }
    } else {
        f = fc * 10;
        if (!f) f = 5;
        f = (100 + ff) * f;
    }
    f = sprintf(ca, "%ld", f);
    switch (f) {
        case 3: cprintf("0.%.3s", ca);              break;
        case 4: cprintf("%.1s.%.3s", ca, ca + 1);   break;
        case 5: cprintf("%.2s.%.2s", ca, ca + 2);   break;
        case 6: cprintf("%.3s.%.1s", ca, ca + 3);   break;
        case 7: cprintf("%.4s.", ca);               break;
    }
    csetcolor(COLOR_DATA);
}

static void
re_printParm(int parm)
{
    unsigned char val = patch_edit_get_edit_parameter(reParm[parm].Offset),
                  c;
    char *t;

    cmove(reParm[parm].Y, reParm[parm].X);
    if (parm == re_Cursor) csetcolor(COLOR_CURSOR);
    switch (reParm[parm].Type) {
        case vpt0_99:
            if (val > 99)
                cprintf("??");
            else
                cprintf("%2.2d", val);
            break;
        case vpt0_7:
            if (val > 7)
                cputch('?');
            else
                cputch(val+'0');
            break;
        case vptMode:
            switch (val) {
                case 0:  c = 'R'; break;
                case 1:  c = 'F'; break;
                default: c = '?'; break;
            }
            cputch(c);
            ve_FExtend(patch_edit_get_edit_parameter(reParm[parm+1].Offset),
                       patch_edit_get_edit_parameter(reParm[parm+2].Offset),
                       val,
                       reParm[parm].Y);
            break;
        case vptFC:
            if (val > 31)
                cprintf("??");
            else
                cprintf("%2d", val);
            ve_FExtend(val,
                       patch_edit_get_edit_parameter(reParm[parm+1].Offset),
                       patch_edit_get_edit_parameter(reParm[parm-1].Offset),
                       reParm[parm].Y);
            break;
        case vptFF:
            if (val > 99)
                cprintf("??");
            else
                cprintf("%2d", val);
            ve_FExtend(patch_edit_get_edit_parameter(reParm[parm-1].Offset),
                       val,
                       patch_edit_get_edit_parameter(reParm[parm-2].Offset),
                       reParm[parm].Y);
            break;
        case vptDetune:    /* 0-14, as -7-+7 */
            if (val > 14)
                cprintf("??");
            else
                cprintf("%+2d", val-7);
            break;
        case vpt0_3:
            if (val > 3)
                cputch('?');
            else
                cputch(val+'0');
            break;
        case vptAlg:   /* 0-31, as 1-32 */
            if (val > 31) {
                cprintf("??");
                break;
            }
            cprintf("%2d", val+1);
            ve_ShowAlg();
            return;
        case vptCurve:
            switch (val) {
                case 0:   t = "-Lin"; break;
                case 1:   t = "-Exp"; break;
                case 2:   t = "+Exp"; break;
                case 3:   t = "+Lin"; break;
                default:  t = "????";
            }
            cprintf("%s", t);
            break;
        case vptBkPt:
            if (val > 99) {
                cprintf("????");
                break;
            }
            val += 21;
            goto printnote;
        case vptTrans:
            if (val > 48) {
                cprintf("????");
                break;
            }
            val += 36;
          printnote:
            cprintf("%s", patch_edit_NoteText(val));
            break;
        case vptOnOff:
            switch (val) {
                case 0:  t = "Off";   break;
                case 1:  t = " On";   break;
                default: t = "???";
            }
            cprintf("%s", t);
            break;
        case vptWave:
            switch (val) {
                case 0:   t = "TRI"; break;
                case 1:   t = "SW-"; break;
                case 2:   t = "SW+"; break;
                case 3:   t = "SQU"; break;
                case 4:   t = "SIN"; break;
                case 5:   t = "S/H"; break;
                default:  t = "???"; break;
            }
            cprintf("%s", t);
            break;
    }
    if (parm == re_Cursor) csetcolor(COLOR_DATA);
}

static void
re_SetScreen(void)
{
    csetcolor(COLOR_DATA);
    cerase();
    cprintf(" TX/Edit Voice Edit\n");
    csetcolor(COLOR_LABEL);
    cprintf("                                                            A\n"
            "  O                              R                        V M\n"
            "  P  R1 L1  R2 L2  R3 L3  R4 L4  S  M FC FF  D Freq.  OL  S S\n");
    cprintf("  1\n"
            "  2\n"
            "  3\n"
            "  4\n"
            "  5\n"
            "  6\n"
            "  P\n");
    cprintf("                                          Alg.\n"
            "      Left   BkPt  Right     Speed        Fdbk\n");
    cprintf("  1                          Delay        C4=\n");
    cprintf("  2                          PMD          OKS\n");
    cprintf("  3                          AMD\n");
    cprintf("  4                          Sync\n");
    cprintf("  5                          Wave\n");
    cprintf("  6                          PMS\n");
    csetcolor(COLOR_DATA);
}

static void re_on_adj_changed(GtkAdjustment *shadow_adj, gpointer data); /* forward */

static void
re_set_edit_parameter(int offset, int val)
{
    if (offset < 0 || offset >= DX7_VOICE_PARAMETERS - 1 /* omitting name */)
        return;

    /* temporarily block reverse signal, then update the adjustment */
    g_signal_handlers_block_by_func(G_OBJECT(edit_adj[offset]), re_on_adj_changed, (gpointer)offset);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(edit_adj[offset]), (float)val);
    g_signal_handlers_unblock_by_func(G_OBJECT(edit_adj[offset]), re_on_adj_changed, (gpointer)offset);
}

static void
re_Inc(int parm)
{
    unsigned char val = patch_edit_get_edit_parameter(reParm[parm].Offset),
                  max;

    max = re_parm_type_max[reParm[parm].Type];
    if (!max) return;
    if (++val > max) val = 0;

    re_set_edit_parameter(reParm[parm].Offset, val);
    re_printParm(parm);
}

static void
re_Dec(int parm)
{
    unsigned char val = patch_edit_get_edit_parameter(reParm[parm].Offset),
                  max;
    
    max = re_parm_type_max[reParm[parm].Type];
    if (!max) return;
    if (val)
        val--;
    else
        val = max;

    re_set_edit_parameter(reParm[parm].Offset, val);
    re_printParm(parm);
}

static void
re_Zero(void)
{
    guint8 offset = reParm[re_Cursor].Offset;

    re_set_edit_parameter(offset, reParm[re_Cursor].Init);
    re_printParm(re_Cursor);
}

static void
re_CurUp(void)
{
    int oldcursor = re_Cursor;
    int i, dx, dy, distance,
        best = -1,
        bestdistance = 1000;

    /* try to find the closest parm in the 'up' direction */
    for (i = 0; i < DX7_VOICE_PARAMETERS - 1 /* omitting name */; i++) {
        dx = abs(reParm[re_Cursor].X - reParm[i].X);
        dy = (reParm[re_Cursor].Y - 1) - reParm[i].Y;
        if (dy < 0)
            dy += VOICEPARMS_Y_SPAN + 2; /* wrap */
        dy = dy * 5 / 2; /* prefer something <2.5 spaces away horizontally to 1 space vertically */
        distance = dx + dy;
        if (bestdistance > distance) {
            bestdistance = distance;
            best = i;
        }
    }
    if (best >= 0) {
        re_Cursor = best;
        re_printParm(oldcursor);
        re_printParm(re_Cursor);
    }
}

static void
re_CurDown(void)
{
    int oldcursor = re_Cursor;
    int i, dx, dy, distance,
        best = -1,
        bestdistance = 1000;

    /* try to find the closest parm in the 'down' direction */
    for (i = 0; i < DX7_VOICE_PARAMETERS - 1 /* omitting name */; i++) {
        dx = abs(reParm[i].X - reParm[re_Cursor].X);
        dy = reParm[i].Y - (reParm[re_Cursor].Y + 1);
        if (dy < 0)
            dy += VOICEPARMS_Y_SPAN + 2; /* wrap */
        dy = dy * 5 / 2; /* prefer something <2.5 spaces away horizontally to 1 space vertically */
        distance = dx + dy;
        if (bestdistance > distance) {
            bestdistance = distance;
            best = i;
        }
    }
    if (best >= 0) {
        re_Cursor = best;
        re_printParm(oldcursor);
        re_printParm(re_Cursor);
    }
}

static void
re_CurRight(void)
{
    int oldcursor = re_Cursor;
    int i, dx, dy, distance,
        best = -1,
        bestdistance = 1000;

    /* try to find the closest parm in the 'right' direction */
    for (i = 0; i < DX7_VOICE_PARAMETERS - 1 /* omitting name */; i++) {
        dx = reParm[i].X - (reParm[re_Cursor].X + 2);
        if (dx < 0)
            dx += VOICEPARMS_X_SPAN + 4; /* wrap */
        dy = abs(reParm[i].Y - reParm[re_Cursor].Y);
        dy = dy * 12; /* prefer something <12 spaces away horizontally to 1 space vertically */
        distance = dx + dy;
        if (bestdistance > distance) {
            bestdistance = distance;
            best = i;
        }
    }
    if (best >= 0) {
        re_Cursor = best;
        re_printParm(oldcursor);
        re_printParm(re_Cursor);
    }
}

static void
re_CurLeft(void)
{
    int oldcursor = re_Cursor;
    int i, dx, dy, distance,
        best = -1,
        bestdistance = 1000;

    /* try to find the closest parm in the 'left' direction */
    for (i = 0; i < DX7_VOICE_PARAMETERS - 1 /* omitting name */; i++) {
        dx = (reParm[re_Cursor].X - 2) - reParm[i].X;
        if (dx < 0)
            dx += VOICEPARMS_X_SPAN + 4; /* wrap */
        dy = abs(reParm[i].Y - reParm[re_Cursor].Y);
        dy = dy * 12; /* prefer something <12 spaces away horizontally to 1 space vertically */
        distance = dx + dy;
        if (bestdistance > distance) {
            bestdistance = distance;
            best = i;
        }
    }
    if (best >= 0) {
        re_Cursor = best;
        re_printParm(oldcursor);
        re_printParm(re_Cursor);
    }
}

//- void
//- ve_Erase(void)
//- {
//-     memcpy(SingleData, InitVoiceUnpacked, VSIZEUNPACKED);
//-     // ve_SetScreen();
//-     ve_DoScreen();
//- //+     refresh();
//- //+     PutUnpacked();
//- }

//+ void
//+ ve_PgUp(void)
//+ {
//+     if (Voice_Cursor) {
//+         Pack(Voice_Cursor);
//+         Unpack(--Voice_Cursor);
//+         ve_DoScreen();
//+         refresh();
//+         PutUnpacked();
//+     }
//+ }

//+ void
//+ ve_PgDn(void)
//+ {
//+     if (Voice_Cursor < Voices - 1) {
//+         Pack(Voice_Cursor);
//+         Unpack(++Voice_Cursor);
//+         ve_DoScreen();
//+         refresh();
//+         PutUnpacked();
//+     }
//+ }

static void
_re_SetOps(void)
{
//+     guint8 bd[7];
//+ 
//+     bd[0] = 0xF0;
//+     bd[1] = 0x43;
//+     bd[2] = 0x10 | TXChannel;
//+     bd[3] = 1;
//+     bd[4] = 27;
//+     bd[5] = re_OpSelect;
//+     bd[6] = 0xF7;
//+     PutMidiMsg(bd, 7);
}

static void
re_SetOps(int o)
{
//-     guint8 i;
//- 
//-     re_OpSelect ^= o;
//-     for (i = 1; i < 7; i++) {
//-         if (re_OpSelect & (1 << (6 - i)))
//-             csetcolor(COLOR_OPON);
//-         else
//-             csetcolor(COLOR_OPOFF);
//- 	cmove(3 + i, 2);
//-         cprintf("%c", i + '0');
//- 	cmove(12 + i, 2);
//-         cprintf("%c", i + '0');
//-     }
//-     /* csetcolor(COLOR_DATA);  not needed with ve_ShowAlg() */
//-     ve_ShowAlg();
    _re_SetOps();
}

static void
re_Number(int c)
{
    unsigned char val = patch_edit_get_edit_parameter(reParm[re_Cursor].Offset),
                  typ = reParm[re_Cursor].Type,
                  max = re_parm_type_max[typ];

    switch(typ) {
        case vpt0_99:
        case vptFF:
            val=(val * 10 + c - '0') % 100;
            break;
        case vptFC:
            val=(val * 10 + c - '0') % 100;
            for(; val > 31; val -= 10);
            break;
        case vpt0_7:
        case vpt0_3:
            val=c - '0';
            if(val > max) val = max;
            break;
        case vptAlg:
            val=((val + 1) * 10 + c - '0') % 100;
            if (!val) val = 1;
            for(; val > 32; val -= 10);
            val--;
            break;
        default:
            return;
    }

    re_set_edit_parameter(reParm[re_Cursor].Offset, val);
    re_printParm(re_Cursor);
}

//+ void
//+ ve_Help(void)
//+ {
//+     erase();
//+     cprintf(" TX/Edit v" VERSIONSTRING " Voice Edit Help\n\n");
//+ 
//+     cprintf(" Up / Down /\n"
//+             " Right / Left  - Move cursor\n\n");
//+ 
//+     cprintf(" +             - Increment field\n"
//+             " -             - Decrement field\n"
//+             " 0 through 9   - Enter data into field\n"
//+             " Backspace     - Zero/reset field\n\n");
//+ 
//+     cprintf(" A through Z   - Rotate letters into name (name field only)\n\n"
//+ 
//+             " F1 through F6 - Enable/disable operators 1 through 6\n\n");
//+ 
//+     cprintf(" Meta-P        - Put edit buffer as single voice\n"
//+             " Meta-!        - Erase edit buffer to init voice\n"
//+             " PgUp / PgDn   - Move to previous / next voice\n"
//+ 
//+             " Esc / Meta-L  - Return to librarian mode\n\n"
//+ 
//+             " h / Meta-H    - Display this help screen\n\n");
//+ 
//+     paktc();
//+     ve_SetScreen();
//+     ve_DoScreen();
//+ }

void
re_ButtonPress(guint button, guint x, guint y)
{
    int oldcursor = re_Cursor;
    int i, dx, dy, distance, best, bestdistance;

    /* GUIDB_MESSAGE(DB_GUI, " re_ButtonPress: button %u @ %u, %u\n", button, x, y); */

//-     /* check for operator (un)mute */  // -FIX- hard-coded screen locations!
//-     if (x == 2 && (y >= 4 && y <= 9)) {
//-         ve_SetOps(32 >> (y - 4));
//-         return;
//-     } else if (x == 2 && (y >= 13 && y <= 18)) {
//-         ve_SetOps(32 >> (y - 13));
//-         return;
//-     }
//-     // -FIX- algorithm ops!

    /* try to find the closest parm */
    best = -1;
    bestdistance = 1000;
    for (i = 0; i < DX7_VOICE_PARAMETERS - 1 /* omitting name */; i++) {
        dx = abs(x - reParm[i].X);
        dy = abs(y - reParm[i].Y);
        dy = dy * 5 / 2; /* prefer something <2.5 spaces away horizontally to 1 space vertically */
        distance = dx + dy;
        if (bestdistance > distance) {
            bestdistance = distance;
            best = i;
        }
    }
    if (best >= 0) {
        re_Cursor = best;
        re_printParm(oldcursor);
        re_printParm(re_Cursor);
    }
}

void
re_CharIn(gboolean pressed, guint c, guint state)
{
    /* GUIDB_MESSAGE(DB_GUI, " re_ButtonPress: button %u @ %u, %u\n", button, x, y); */

    /* if (c < 127) {
     *     GUIDB_MESSAGE(DB_GUI, " re_CharIn: read char '%c'\n", (char)c);
     * } else {
     *     GUIDB_MESSAGE(DB_GUI, " re_CharIn: read char %08x\n", c);
     * } */
    if (!pressed) {  /* key release */
        if (c == ' ') {  /* turn off test note */
            on_test_note_button_press(NULL, (gpointer)0);
        }
        return;
    }

        if(c >= 'A' && c <= 'Z')
            c = tolower(c);
        switch(c) {
            case '+':
            case '=':           re_Inc(re_Cursor);  break;
            case '-':           re_Dec(re_Cursor);  break;
            case GDK_BackSpace:
            case GDK_Delete:    re_Zero();          break;
            case GDK_Up:        re_CurUp();         break;
            case GDK_Down:      re_CurDown();       break;
            case GDK_Tab:
            case GDK_Right:     re_CurRight();      break;
            case GDK_ISO_Left_Tab:
            case GDK_Left:      re_CurLeft();       break;
            case GDK_F1:        re_SetOps(32);      break;
            case GDK_F2:        re_SetOps(16);      break;
            case GDK_F3:        re_SetOps( 8);      break;
            case GDK_F4:        re_SetOps( 4);      break;
            case GDK_F5:        re_SetOps( 2);      break;
            case GDK_F6:        re_SetOps( 1);      break;
//+             case KEY_PPAGE:     re_PgUp();          break;
//+             case KEY_NPAGE:     re_PgDn();          break;
//+             case KEY_METAP:     PutUnpacked();      break;
//k             case KEY_METAEX:    re_Erase();         break;
//k             case KEY_METAH:
//+             case 'h':           re_Help();          break;
            case ' ':  /* turn on test note */
                on_test_note_button_press(NULL, (gpointer)1);
                break;
            default:
                if(c>='0' && c<='9') {
                    re_Number(c);
//-                 } else {
//-                     gotomsg();
//-                     cprintf(" Type <Alt-H> for help...");
//- #ifdef DEBUG
//-                     cprintf("keycode=0x%04x", c);
//- #endif
                }
                break;
        }
}

static void
re_on_adj_changed(GtkAdjustment *shadow_adj, gpointer data)
{
    int offset = (int)data;
    int i;

    /* GUIDB_MESSAGE(DB_GUI, " re_on_adj_changed: offset %d\n", offset); */

    for (i = 0; i < DX7_VOICE_PARAMETERS - 1 /* omitting name */; i++) {
        if (reParm[i].Offset == offset) {
            re_printParm(i);
            return;
        }
    }

}

/* ==== Instantiation ==== */

GtkWidget *
create_retro_editor(const char *tag)
{
    int i;

    retro_console_init(CONSOLE_WIDTH, CONSOLE_HEIGHT);

    // -FIX- white on yellow!
    rc_console_init_color_pair(COLOR_MSG,     RC_COLOR_WHITE, RC_COLOR_YELLOW);  /* also COLOR_OPOFF */
    rc_console_init_color_pair(COLOR_LABEL,   RC_COLOR_GREEN, RC_COLOR_BLUE);    /* also COLOR_OPON */
    rc_console_init_color_pair(COLOR_DATA,    RC_COLOR_WHITE, RC_COLOR_BLUE);
    rc_console_init_color_pair(COLOR_BLOCK,   RC_COLOR_WHITE, RC_COLOR_CYAN);
    rc_console_init_color_pair(COLOR_CURSOR,  RC_COLOR_BLACK, RC_COLOR_YELLOW);
    rc_console_init_color_pair(COLOR_BCURSOR, RC_COLOR_WHITE, RC_COLOR_GREEN);

    re_SetScreen();
    /* re_DoScreen(); */
    rc_console_set_button_press_callback(re_ButtonPress);
    rc_console_set_key_press_callback(re_CharIn);

    for (i = 0; i < DX7_VOICE_PARAMETERS - 1 /* omitting name */; i++) {
        g_signal_connect(G_OBJECT(edit_adj[i]), "value-changed",
                         G_CALLBACK(re_on_adj_changed), (gpointer)i);
    }

    retro_widget = rc_drawing_area;

    return retro_widget;
}

