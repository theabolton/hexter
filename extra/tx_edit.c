/* Yamaha DX7 / TX7 Editor/Librarian
 *
 * Copyright (C) 1991, 1995, 1997, 1998, 2004, 2009 Sean Bolton.
 *
 * This is an ncurses-based patch editor for the Yamaha DX7 and
 * TX7.  It is provided as-is, without any documentation, and
 * is totally unsupported, but may be useful to you if you can't use
 * JSynthLib or similar.  It started out life on my Apple ][+, then
 * I ported it to my Amiga, then to my MS-DOS machine, then to
 * Linux, so the code's a mess and nothing I'm proud of.  But, it
 * does the job.
 *
 * Compile with:
 *     gcc -o tx_edit tx_edit.c -lcurses -lasound
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
 * 
 * Revision History:
 * 20040126 Sean Bolton - made help functions helpful, added ALSA support
 * 20040205 Sean Bolton - clean up display with noecho, line drawing
 * 20040207 Sean Bolton - add next/previous voice commands to edit mode
 * 20090102 Sean Bolton - incorporated Martin Tarenskeen's patch loading
 *                        enhancements
 */

/* Need to do:                              */
/*   need to provide color changing option  */
/*   bank and single receive                */
/*   auto buffer sizing?                    */
/*   a search option would be nice          */
/*   show channel/instrument on display     */

#define VERSIONSTRING "0.92s"

/* Undefine USE_ALSA_MIDI to just write to /dev/midi */
#define USE_ALSA_MIDI 1

#define DEBUG 1

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <curses.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#ifdef USE_ALSA_MIDI
#include <alsa/asoundlib.h>
#endif

#define RETURN_OK     0
#define RETURN_FAIL 255

typedef unsigned char UBYTE;

void cprintf(const char *fmt, ...);


/* ==== Files ==== */

#define MAXPATH 80

char FNameBuff[MAXPATH]="\0";

/* ==== Buffers ==== */

#define VSIZEPACKED     128
#define VSIZEUNPACKED   155
#define DEFAULTVOICES   8192
#define DUMPSIZE_SINGLE 155+8
#define DUMPSIZE_BULK   4096+8

int Voices = DEFAULTVOICES;

UBYTE *Buffer = NULL,
      *VoiceData,
      *SingleDump,
      *SingleData,
      *BulkDump;
int    BufferLength;

/* ==== Console ==== */

#define KEY_METAA       (0x200+'a')
#define KEY_METAB       (0x200+'b')
#define KEY_METAC       (0x200+'c')
#define KEY_METAD       (0x200+'d')
#define KEY_METAE       (0x200+'e')
#define KEY_METAG       (0x200+'g')
#define KEY_METAH       (0x200+'h')
#define KEY_METAL       (0x200+'l')
#define KEY_METAM       (0x200+'m')
#define KEY_METAO       (0x200+'o')
#define KEY_METAP       (0x200+'p')
#define KEY_METAQ       (0x200+'q')
#define KEY_METAR       (0x200+'r')
#define KEY_METAS       (0x200+'s')
#define KEY_METAT       (0x200+'t')
#undef KEY_ENTER
#define KEY_ENTER       (0x0a)
#define KEY_ESC         (0x1b)
#define KEY_METAEQ      (0x200+'=')  /* Alt-= */
#define KEY_METAPL      (0x200+'+')  /* Alt-+ */
#define KEY_METAEX      (0x200+'!')  /* Alt-! */
#define KEY_METANU      (0x200+'#')  /* Alt-# */

enum textcolor {COLOR_MSG = 1, COLOR_LABEL, COLOR_DATA, COLOR_BLOCK,
                COLOR_CURSOR, COLOR_BCURSOR, COLOR_OPON, COLOR_OPOFF};

#if 0
#define kc_Del       (0x5300)
#define kc_CtrlRight (0x7400)
#define kc_CtrlLeft  (0x7300)
#define kc_CtrlPgUp  (0x8400)
#define kc_CtrlPgDn  (0x7600)
#define kc_Home      (0x4700)
#define kc_End       (0x4f00)
#define kc_CtrlHome  (0x7700)
#define kc_CtrlEnd   (0x7500)
#endif

/* ==== Midi ==== */

#ifdef USE_ALSA_MIDI
snd_seq_t *MidiHandle = NULL;
int        MidiClient;
int        MidiPort = -1;
#endif

char TXChannel = 0;  /* 0-15! */

/* ==== VoiceEdit ==== */

#define VOICEPARMS      146
#define VOICEPARMMAX    145

int ve_Cursor = 0;
int ve_OpSelect = 63;

#define vptName     1
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

unsigned char vtypemax[15] = {
    0, 0, 99, 7, 1, 31, 99, 14, 3, 31, 3, 99, 48, 1, 5,
};

struct _veParm {
    UBYTE   Type;
    UBYTE   Offset;
    UBYTE   X;
    UBYTE   Y;
    UBYTE   Up;
    UBYTE   Down;
} veParm[VOICEPARMS] = {
/*     T,  Off,  X,  Y,   U,   D */
    {  1,  145, 31,  0, 145,   8, }, /* Name */
    {  2,  105,  5,  4, 140,  17, }, /* O1 R1 */
    {  2,  109,  8,  4, 141,  18, },
    {  2,  106, 12,  4, 142,  19, },
    {  2,  110, 15,  4, 142,  20, },
    {  2,  107, 19,  4, 143,  21, },
    {  2,  111, 22,  4, 144,  22, },
    {  2,  108, 26,  4,   0,  23, },
    {  2,  112, 29,  4,   0,  24, },
    {  3,  118, 33,  4,   0,  25, }, /* O1 RS */
    {  4,  122, 36,  4,   0,  26, },
    {  5,  123, 38,  4, 145,  27, },
    {  6,  124, 41,  4, 121,  28, },
    {  7,  125, 44,  4, 121,  29, },
    {  2,  121, 54,  4,  94,  30, },
    {  3,  120, 58,  4,  95,  31, },
    {  8,  119, 60,  4,  96,  32, }, /* O1 AMS */
    {  2,   84,  5,  5,   1,  33, }, /* O2 R1 */
    {  2,   88,  8,  5,   2,  34, },
    {  2,   85, 12,  5,   3,  35, },
    {  2,   89, 15,  5,   4,  36, },
    {  2,   86, 19,  5,   5,  37, },
    {  2,   90, 22,  5,   6,  38, },
    {  2,   87, 26,  5,   7,  39, },
    {  2,   91, 29,  5,   8,  40, },
    {  3,   97, 33,  5,   9,  41, }, /* O2 RS */
    {  4,  101, 36,  5,  10,  42, },
    {  5,  102, 38,  5,  11,  43, },
    {  6,  103, 41,  5,  12,  44, },
    {  7,  104, 44,  5,  13,  45, },
    {  2,  100, 54,  5,  14,  46, },
    {  3,   99, 58,  5,  15,  47, },
    {  8,   98, 60,  5,  16,  48, }, /* O2 AMS */
    {  2,   63,  5,  6,  17,  49, }, /* O3 R1 */
    {  2,   67,  8,  6,  18,  50, },
    {  2,   64, 12,  6,  19,  51, },
    {  2,   68, 15,  6,  20,  52, },
    {  2,   65, 19,  6,  21,  53, },
    {  2,   69, 22,  6,  22,  54, },
    {  2,   66, 26,  6,  23,  55, },
    {  2,   70, 29,  6,  24,  56, },
    {  3,   76, 33,  6,  25,  57, }, /* O3 RS */
    {  4,   80, 36,  6,  26,  58, },
    {  5,   81, 38,  6,  27,  59, },
    {  6,   82, 41,  6,  28,  60, },
    {  7,   83, 44,  6,  29,  61, },
    {  2,   79, 54,  6,  30,  62, },
    {  3,   78, 58,  6,  31,  63, },
    {  8,   77, 60,  6,  32,  64, }, /* O3 AMS */
    {  2,   42,  5,  7,  33,  65, }, /* O4 R1 */
    {  2,   46,  8,  7,  34,  66, },
    {  2,   43, 12,  7,  35,  67, },
    {  2,   47, 15,  7,  36,  68, },
    {  2,   44, 19,  7,  37,  69, },
    {  2,   48, 22,  7,  38,  70, },
    {  2,   45, 26,  7,  39,  71, },
    {  2,   49, 29,  7,  40,  72, },
    {  3,   55, 33,  7,  41,  73, }, /* O4 RS */
    {  4,   59, 36,  7,  42,  74, },
    {  5,   60, 38,  7,  43,  75, },
    {  6,   61, 41,  7,  44,  76, },
    {  7,   62, 44,  7,  45,  77, },
    {  2,   58, 54,  7,  46,  78, },
    {  3,   57, 58,  7,  47,  79, },
    {  8,   56, 60,  7,  48,  80, }, /* O4 AMS */
    {  2,   21,  5,  8,  49,  81, }, /* O5 R1 */
    {  2,   25,  8,  8,  50,  82, },
    {  2,   22, 12,  8,  51,  83, },
    {  2,   26, 15,  8,  52,  84, },
    {  2,   23, 19,  8,  53,  85, },
    {  2,   27, 22,  8,  54,  86, },
    {  2,   24, 26,  8,  55,  87, },
    {  2,   28, 29,  8,  56,  88, },
    {  3,   34, 33,  8,  57,  89, }, /* O5 RS */
    {  4,   38, 36,  8,  58,  90, },
    {  5,   39, 38,  8,  59,  91, },
    {  6,   40, 41,  8,  60,  92, },
    {  7,   41, 44,  8,  61,  93, },
    {  2,   37, 54,  8,  62,  94, },
    {  3,   36, 58,  8,  63,  95, },
    {  8,   35, 60,  8,  64,  96, }, /* O5 AMS */
    {  2,    0,  5,  9,  65,  97, }, /* O6 R1 */
    {  2,    4,  8,  9,  66,  98, },
    {  2,    1, 12,  9,  67,  99, },
    {  2,    5, 15,  9,  68, 100, },
    {  2,    2, 19,  9,  69, 101, },
    {  2,    6, 22,  9,  70, 102, },
    {  2,    3, 26,  9,  71, 103, },
    {  2,    7, 29,  9,  72, 104, },
    {  3,   13, 33,  9,  73, 106, }, /* O6 RS */
    {  4,   17, 36,  9,  74, 106, },
    {  5,   18, 38,  9,  75, 106, },
    {  6,   19, 41,  9,  76, 105, },
    {  7,   20, 44,  9,  77, 105, },
    {  2,   16, 54,  9,  78, 105, },
    {  3,   15, 58,  9,  79, 105, },
    {  8,   14, 60,  9,  80, 105, }, /* O6 AMS */
    {  2,  126,  5, 10,  81, 108, }, /* P R1 */
    {  2,  130,  8, 10,  82, 109, },
    {  2,  127, 12, 10,  83, 110, },
    {  2,  131, 15, 10,  84, 110, },
    {  2,  128, 19, 10,  85, 111, },
    {  2,  132, 22, 10,  86, 112, },
    {  2,  129, 26, 10,  87, 112, },
    {  2,  133, 29, 10,  88, 106, },
    {  9,  134, 48, 11,  94, 107, }, /* Alg */
    {  2,  137, 35, 12,  90, 113, },
    {  3,  135, 49, 12, 105, 114, },
    { 10,  116,  5, 13,  97, 115, }, /* O1 LC */
    {  2,  114, 10, 13,  98, 116, },
    { 11,  113, 13, 13,  99, 117, },
    { 10,  117, 18, 13, 101, 118, },
    {  2,  115, 23, 13, 102, 119, },
    {  2,  138, 35, 13, 106, 120, },  /* Delay */
    { 12,  144, 46, 13, 107, 121, },
    { 10,   95,  5, 14, 108, 122, }, /* O2 LC */
    {  2,   93, 10, 14, 109, 123, },
    { 11,   92, 13, 14, 110, 124, },
    { 10,   96, 18, 14, 111, 125, },
    {  2,   94, 23, 14, 112, 126, },
    {  2,  139, 35, 14, 113, 127, },  /* PMD */
    { 13,  136, 47, 14, 114,  14, },
    { 10,   74,  5, 15, 115, 128, }, /* O3 LC */
    {  2,   72, 10, 15, 116, 129, },
    { 11,   71, 13, 15, 117, 130, },
    { 10,   75, 18, 15, 118, 131, },
    {  2,   73, 23, 15, 119, 132, },
    {  2,  140, 35, 15, 120, 133, },  /* AMD */
    { 10,   53,  5, 16, 122, 134, }, /* O4 LC */
    {  2,   51, 10, 16, 123, 135, },
    { 11,   50, 13, 16, 124, 136, },
    { 10,   54, 18, 16, 125, 137, },
    {  2,   52, 23, 16, 126, 138, },
    { 13,  141, 34, 16, 127, 139, },  /* Sync */
    { 10,   32,  5, 17, 128, 140, }, /* O5 LC */
    {  2,   30, 10, 17, 129, 141, },
    { 11,   29, 13, 17, 130, 142, },
    { 10,   33, 18, 17, 131, 143, },
    {  2,   31, 23, 17, 132, 144, },
    { 14,  142, 34, 17, 133, 145, },  /* Wave */
    { 10,   11,  5, 18, 134,   1, }, /* O6 LC */
    {  2,    9, 10, 18, 135,   2, },
    { 11,    8, 13, 18, 136,   3, },
    { 10,   12, 18, 18, 137,   5, },
    {  2,   10, 23, 18, 138,   6, },
    {  3,  143, 36, 18, 139,  10, }  /* PMS */
};

unsigned short veFFF[100] = {
    1000, 1023, 1047, 1072, 1096, 1122, 1148, 1175, 1202, 1230,
    1259, 1288, 1318, 1349, 1380, 1413, 1445, 1479, 1514, 1549,
    1585, 1622, 1660, 1698, 1738, 1778, 1820, 1862, 1905, 1950,
    1995, 2042, 2089, 2138, 2188, 2239, 2291, 2344, 2399, 2455,
    2512, 2570, 2630, 2692, 2754, 2818, 2884, 2951, 3020, 3090,
    3162, 3236, 3311, 3388, 3467, 3548, 3631, 3715, 3802, 3890,
    3981, 4074, 4169, 4266, 4365, 4467, 4571, 4677, 4786, 4898,
    5012, 5129, 5248, 5370, 5495, 5623, 5754, 5888, 6026, 6166,
    6310, 6457, 6607, 6761, 6918, 7079, 7244, 7413, 7586, 7762,
    7943, 8128, 8318, 8511, 8710, 8913, 9120, 9333, 9550, 9772,
};

/* ==== Algorithm ==== */

char *veAlg[32] = {  /* 32 algs, max 9 high and 17 wide */
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

/* ==== Librarian ==== */

void noop(void){}

void  (*LibBlockAction)(void) = noop;

int   Voice_Cursor = 0, /* voice number under curser  */
      Voice_TOS = 0,    /* " at top of screen         */
      Voice_BMark,      /* " where block first marked */
      Voice_BStart,     /* " of block start           */
      Voice_BEnd;       /* " of block end             */
bool  blocking = FALSE,
      blocked = FALSE;
/* -FIX- This should be dynamic! */
int   LibCols=5,        /* columns of names that fit on screen (15 char each) */
      LibRows=16,       /* rows    of names that fit on screen */
      LibNames=80;      /*            names that fit on screen */

/* ==== Control ==== */

int txmode=0,
    newmode=0;

#define MODELIB     1
#define MODEEDIT    2
#define MODEQUIT    3

/* ==== Utility Stuff ==== */

inline int min(int a, int b) { return (a < b ? a : b); }
inline int max(int a, int b) { return (a > b ? a : b); }

char _NoteText[5]={'x','x','x','x','\0'};

char *
NoteText(int note)
{
    _NoteText[0]=(" C D  F G A ")[note%12];
    _NoteText[1]=("C#D#EF#G#A#B")[note%12];
    _NoteText[2]=("-0123456789")[note/12];
    _NoteText[3]=("1          ")[note/12];
    return _NoteText;
}

UBYTE *
voiceAddr(int voice)
{
 /* return voice*VSIZEPACKED+VoiceData; */
    return (voice << 7)     +VoiceData;
}

void
PrintVName(int voice)  /* print name of voice - packed only! */
{
    UBYTE ca[11];
    int i;

    memcpy(ca, voiceAddr(voice)+118, 10);
    for (i=0; i<10; i++) {
        if (ca[i] < 32)
            ca[i]=183;  /* centered dot */
        else if (ca[i] >= 128)
            ca[i]=174;  /* (R) symbol = out-of-range */
        else {
            switch (ca[i]) {
                case  92:  ca[i]=165;  break;  /* yen */
                case 126:  ca[i]=187;  break;  /* >> */
                case 127:  ca[i]=171;  break;  /* << */
            }
        }
    }
    ca[10]=0;
    cprintf("%s", ca);
}

/* ==== Init/Term Routines ==== */

UBYTE InitVoice[] = {
    0x62, 0x63, 0x63, 0x5A, 0x63, 0x63, 0x63, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x02,
    0x00, 0x62, 0x63, 0x63, 0x5A, 0x63, 0x63, 0x63,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00,
    0x02, 0x00, 0x62, 0x63, 0x63, 0x5A, 0x63, 0x63,
    0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00,
    0x00, 0x02, 0x00, 0x62, 0x63, 0x63, 0x5A, 0x63,
    0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38,
    0x00, 0x00, 0x02, 0x00, 0x62, 0x63, 0x63, 0x5A,
    0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x38, 0x00, 0x00, 0x02, 0x00, 0x62, 0x63, 0x63,
    0x5A, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x38, 0x00, 0x63, 0x02, 0x00, 0x63, 0x63,
    0x63, 0x63, 0x32, 0x32, 0x32, 0x32, 0x00, 0x08,
    0x23, 0x00, 0x00, 0x00, 0x31, 0x18, 0x20, 0x20,
    0x20, 0x7F, 0x2D, 0x2D, 0x7E, 0x20, 0x20, 0x20 };

UBYTE InitVoiceUnpacked[155] = {
    0x62, 0x63, 0x63, 0x5a, 0x63, 0x63, 0x63, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x07, 0x62, 0x63, 0x63,
    0x5a, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x07, 0x62, 0x63, 0x63, 0x5a, 0x63, 0x63,
    0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 0x62,
    0x63, 0x63, 0x5a, 0x63, 0x63, 0x63, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x07, 0x62, 0x63, 0x63, 0x5a,
    0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x07, 0x62, 0x63, 0x63, 0x5a, 0x63, 0x63, 0x63,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x63, 0x00, 0x01, 0x00, 0x07, 0x63, 0x63,
    0x63, 0x63, 0x32, 0x32, 0x32, 0x32, 0x00, 0x00,
    0x01, 0x23, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03,
    0x18, 0x20, 0x20, 0x20, 0x7f, 0x2d, 0x2d, 0x7e,
    0x20, 0x20, 0x20
};

void
EraseVoice(int voice)
{
    memcpy(voiceAddr(voice), InitVoice, VSIZEPACKED);
}

void
ConsoleTerm(void)
{
    endwin();
}

bool
ConsoleInit(void)
{
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
#if 1
    init_pair(COLOR_MSG,     COLOR_WHITE, COLOR_YELLOW);  /* 0x1c */
    init_pair(COLOR_LABEL,   COLOR_GREEN, COLOR_BLUE);    /* 0x1a */
    init_pair(COLOR_DATA,    COLOR_WHITE, COLOR_BLUE);    /* 0x1f */
    init_pair(COLOR_BLOCK,   COLOR_WHITE, COLOR_CYAN);    /* 0x3f */
    init_pair(COLOR_CURSOR,  COLOR_BLACK, COLOR_GREEN);   /* 0x20 */
    init_pair(COLOR_BCURSOR, COLOR_WHITE, COLOR_GREEN);   /* 0x2f */
    init_pair(COLOR_OPON,    COLOR_GREEN, COLOR_BLUE);    /* 0x1a */
    init_pair(COLOR_OPOFF,   COLOR_WHITE, COLOR_YELLOW);  /* 0x14 */
#else
    init_pair(COLOR_MSG,     COLOR_WHITE, COLOR_YELLOW);  /* 0x1c */
    init_pair(COLOR_LABEL,   COLOR_WHITE, COLOR_YELLOW);  /* 0x1a */
    init_pair(COLOR_DATA,    COLOR_WHITE, COLOR_BLUE);    /* 0x1f */
    init_pair(COLOR_BLOCK,   COLOR_BLACK, COLOR_YELLOW);  /* 0x3f */
    init_pair(COLOR_CURSOR,  COLOR_WHITE, COLOR_YELLOW);  /* 0x20 */
    init_pair(COLOR_BCURSOR, COLOR_BLACK, COLOR_YELLOW);  /* 0x2f */
    init_pair(COLOR_OPON,    COLOR_WHITE, COLOR_YELLOW);  /* 0x1a */
    init_pair(COLOR_OPOFF,   COLOR_WHITE, COLOR_YELLOW);  /* 0x14 */
#endif
    bkgdset(COLOR_PAIR(COLOR_DATA));
    attrset(COLOR_PAIR(COLOR_DATA)|A_BOLD);
    noecho();
    return TRUE;
}

void
BuffTerm(void)
{
    if (Buffer) {
        free(Buffer);
        Buffer = NULL;
    }
}

bool
BuffInit(void)  /* returns TRUE if successful */
{
    int i;

    BuffTerm();
    /* calculate buffer size from number of voices * VSIZEPACKED plus a bulk
       dump buffer and a single dump buffer */
    BufferLength = Voices * VSIZEPACKED + DUMPSIZE_BULK + DUMPSIZE_SINGLE;
    if (!(Buffer = malloc(BufferLength))) {
        return FALSE;
    }
    VoiceData = Buffer;
    BulkDump = VoiceData + Voices * VSIZEPACKED;
    SingleDump = BulkDump + DUMPSIZE_BULK;  /* must be last for alignment */
    SingleData = SingleDump + 6;
    /* Init the buffers */
    for (i = 0; i < Voices; EraseVoice(i++));
    return TRUE;
}

bool
MidiInit(void)  /* returns TRUE if successful */
{
#ifdef USE_ALSA_MIDI

    const char *device = "hw";   /* could also be "default" */
    
 /* if (snd_seq_open(&MidiHandle, device, SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK) < 0) { */
    if (snd_seq_open(&MidiHandle, device, SND_SEQ_OPEN_OUTPUT, 0) < 0) {
        fprintf(stderr, "MidiInit: could not open sequencer: %s\n", snd_strerror(errno));
        return FALSE;
    }

    snd_seq_set_client_name (MidiHandle, "TX/Edit");

    MidiClient = snd_seq_client_id(MidiHandle);

    if ((MidiPort = snd_seq_create_simple_port(MidiHandle, "TX/Edit",
                                               SND_SEQ_PORT_CAP_READ |
                                               SND_SEQ_PORT_CAP_SUBS_READ,
                                               SND_SEQ_PORT_TYPE_MIDI_GENERIC)) < 0) {
        fprintf(stderr, "MidiInit: error creating port: %s\n", snd_strerror(errno));
        return FALSE;
    }

    return TRUE;

#else /* not USE_ALSA_MIDI */
    return TRUE;
#endif /* USE_ALSA_MIDI */
}

void
MidiTerm(void)
{
#ifdef USE_ALSA_MIDI
    if (MidiHandle) {
        snd_seq_drain_output(MidiHandle);  /* do we need this? could use snd_seq_drop_output(handle) */
    }
    if (MidiPort >= 0) {
        snd_seq_delete_simple_port (MidiHandle, MidiPort);
        MidiPort = -1;
    }
    if (MidiHandle) {
        snd_seq_close(MidiHandle);
        MidiHandle = NULL;
    }
#endif /* USE_ALSA_MIDI */
}

void
TXTerm(int retcode)
{
    ConsoleTerm();
    BuffTerm();
    MidiTerm();
    exit(retcode);
}

void
TXInit(void)  /* exit()s on failure */
{
    if (!MidiInit()) {
        fprintf(stderr, "MidiInit error!\n");
        TXTerm(RETURN_FAIL);
    }
    if (!BuffInit()) {
        fprintf(stderr, "Can't allocate memory for Voice Buffer!\n");
        TXTerm(RETURN_FAIL);
    }
    if (!ConsoleInit()) {
        fprintf(stderr, "ConsoleInit error!\n");
        TXTerm(RETURN_FAIL);
    }
}

/* ==== Console Routines ==== */

void
cprintf(const char *fmt, ...) {
    va_list ap;
    char tbuf[512];

    va_start(ap, fmt);
    (void) vsnprintf(tbuf, 512, fmt, ap);
    va_end(ap);
    addstr(tbuf);
} 

int
cputch(int c) {
    return addch(c);
}

int
cgetch(void)
{
    int c = getch();

    if(c == 27) {
	c = getch();
	if (isalpha(c)) c = tolower(c);
        return 0x200 + c;
    }
    return c;
}

void
textattr(enum textcolor c) {
    bkgdset(COLOR_PAIR(c));
    attrset(COLOR_PAIR(c)|A_BOLD);
}

void
paktc(void)
{
    textattr(COLOR_MSG);
    cprintf(" Press Any Key to Continue... ");
    textattr(COLOR_DATA);
    cgetch();
    cprintf("\r");
    clrtoeol();
}

void
gotomsg(void)
{
    move(LibRows + 4, 2);
}

/* ==== Midi Routines ==== */

void
Unpack(int voice)
{
    UBYTE *a2, *a3;
    UBYTE  d1,  d2;

        a2=SingleData;       /* a2 = &unpacked data */
        a3=voiceAddr(voice); /* a3 = &packed data  */
        for (d2=6; d2>0; d2--) {
            for (d1=11; d1>0; d1--) { *a2++=*a3++; }  /* through rd */
            *a2++=(*a3)&0x03;   /* lc */
            *a2++=(*a3++)>>2;   /* rc */
            *a2++=(*a3)&0x07;   /* rs */
            *(a2+6)=(*a3++)>>3; /* pd */
            *a2++=(*a3)&0x03;   /* ams */
            *a2++=(*a3++)>>2;   /* kvs */
            *a2++=*a3++;        /* ol */
            *a2++=(*a3)&0x01;   /* m */
            *a2++=(*a3++)>>1;   /* fc */
            *a2=*a3++;          /* ff */
            a2+=2;
        }                       /* operator done */
        for (d2=9; d2>0; d2--) {
            *a2++=*a3++;
        }                       /* through algorithm */
        *a2++=(*a3)&0x07;       /* feedback */
        *a2++=(*a3++)>>3;       /* oks */
        for (d2=4; d2>0; d2--) {
            *a2++=*a3++;
        }                       /* through lamd */
        *a2++=(*a3)&0x01;       /* lfo ks */
        *a2++=((*a3)>>1)&0x07;  /* lfo wave */
        *a2++=(*a3++)>>4;       /* lfo pms */
        for (d2=11; d2>0; d2--) {
            *a2++=*a3++;
        }                       /* through name */
}

void
_Pack(UBYTE *packed, UBYTE *unpacked)
{
    UBYTE *a2, *a3;
    UBYTE  d1,  d2;

        a3=packed;    /* a3 = &packed data   */
        a2=unpacked;  /* a2 = &unpacked data */
        for (d2=6; d2>0; d2--) {
            for (d1=11; d1>0; d1--) { *a3++=*a2++; }  /* through rd */
            *a3++=((*a2)&0x03)|(((*(a2+1))&0x03)<<2);
            a2+=2;              /* rc+lc */
            *a3++=((*a2)&0x07)|(((*(a2+7))&0x0f)<<3);
            a2++;               /* pd+rs */
            *a3++=((*a2)&0x03)|(((*(a2+1))&0x07)<<2);
            a2+=2;              /* kvs+ams */
            *a3++=*a2++;        /* ol */
            *a3++=((*a2)&0x01)|(((*(a2+1))&0x1f)<<1);
            a2+=2;              /* fc+m */
            *a3++=*a2;
            a2+=2;              /* ff */
        }                       /* operator done */
        for (d2=9; d2>0; d2--) { *a3++=*a2++; }  /* through algorithm */
        *a3++=((*a2)&0x07)|(((*(a2+1))&0x01)<<3);
        a2+=2;                  /* oks+fb */
        for (d2=4; d2>0; d2--) { *a3++=*a2++; }  /* through lamd */
        *a3++=((*a2)&0x01)|(((*(a2+1))&0x07)<<1)|
                           (((*(a2+2))&0x07)<<4);
        a2+=3;                  /* lpms+lfw+lks */
        for (d2=11; d2>0; d2--) { *a3++=*a2++; }  /* through name */
#ifdef GAK
d2=a3-voiceAddr(voice);
cprintf("128 %d\r\n",d2);
d2=a2-SingleData;
cprintf("155 %d\r\n",d2);
getch();
#endif
}

void
Pack(int voice)
{
    _Pack(voiceAddr(voice), SingleData);
}

void
PutMidiMsg(UBYTE *buf, unsigned int size)
{
#ifdef USE_ALSA_MIDI
    snd_seq_event_t event;

    snd_seq_ev_clear(&event);
    snd_seq_ev_set_source(&event, MidiPort);
    snd_seq_ev_set_subs(&event);
    snd_seq_ev_set_direct(&event);

    // set event type, data, so on:
    event.type = SND_SEQ_EVENT_SYSEX;
    snd_seq_ev_set_variable(&event, size, buf);

    if (snd_seq_event_output(MidiHandle, &event) < 0) {
        gotomsg();
        cprintf("PutMidiMsg: could not write output: %s\n", snd_strerror(errno));
        return;
    }
    if (snd_seq_drain_output(MidiHandle) < 0) {
        gotomsg();
        cprintf("PutMidiMsg: error draining output: %s\n", snd_strerror(errno));
    }

#else /* not USE_ALSA_MIDI: */
    FILE *fh;
    
    if ((fh = fopen("/dev/midi00", "wb"))) {  /* should fix this.... */
	fwrite(buf, 1, size, fh);
	fclose(fh);
    }
#endif /* USE_ALSA_MIDI */
}

void
ChecksumSingle(void)
{
    int sum = 0;
    int i;

    for (i = 0; i < VSIZEUNPACKED; sum -= SingleData[i++]);
    SingleData[VSIZEUNPACKED] = sum & 0x7F;
}

void
PutUnpacked(void)
{
    UBYTE *sd;

    ChecksumSingle();
    sd = SingleDump;
    sd[0] = 0xF0;
    sd[1] = 0x43;
    sd[2] = TXChannel;
    sd[3] = 0;
    sd[4] = 0x01;
    sd[5] = 0x1B;
    sd[DUMPSIZE_SINGLE - 1] = 0xF7;
    PutMidiMsg(sd, DUMPSIZE_SINGLE);
}

void
PutPacked(int voice)
{
    Unpack(voice);
    PutUnpacked();
}

void
PutBank(int voice)
{
    UBYTE *bd = BulkDump,
          *p1 = voiceAddr(voice),
          *p2,
          *pe,
          chksum = 0;

    if(voice <= Voices - 32) {
        bd[0] = 0xF0;
        bd[1] = 0x43;
        bd[2] = TXChannel;
        bd[3] = 9;
        bd[4] = 0x10;
        bd[5] = 0x00;
        for(p2 = bd + 6, pe = p2 + 4096; p2 < pe; chksum -= *p2++ = *p1++);
        bd[DUMPSIZE_BULK - 2] = chksum & 0x7f;
        bd[DUMPSIZE_BULK - 1] = 0xF7;
        PutMidiMsg(bd, DUMPSIZE_BULK);
    } else {
        beep();
    }
}

void
PutParm(int parm)
{
    UBYTE offset = veParm[parm].Offset;
    UBYTE bd[7];
    bd[0] = 0xF0;
    bd[1] = 0x43;
    bd[2] = 0x10 | TXChannel;
    bd[3] = offset >> 7;
    bd[4] = offset & 0x7F;
    bd[5] = SingleData[offset];
    bd[6] = 0xF7;
    PutMidiMsg(bd, 7);
}

/* ==== Load/Save Routines ==== */

bool
FileRequest(char *namebuf, char *title)
{
    echo(); curs_set(1);
    cprintf(" %s\n\n Enter filename:\n > ",title);
    refresh();
    if (wgetnstr(stdscr, namebuf, 76) == ERR) {
	namebuf[0] = 0;
    }
    noecho(); curs_set(0);
    cprintf("\n");
    return (strlen(namebuf)!=0);
}

void
LoadError(const char *filename, const char *error)
{
    cprintf(" Load - Error reading file '%s':\n", filename);
    cprintf(" '%s'!\n", error);
    paktc();
}

int
Load(char *filename)
{
    FILE *fp;
    char errbuf[256];
    long filelength;
    unsigned char *raw_patch_data = NULL;
    size_t filename_length;
    int count;
    int patchstart;
    int midshift;
    int datastart;

    /* this needs to 1) open and parse the file, 2a) if it's good, copy as
     * many patches as will fit into VoiceData beginning at Voice_Cursor,
     * 2b) if it's not good, print an appropriate error message and return. */

    if ((fp = fopen(filename, "rb")) == NULL) {
        snprintf(errbuf, 256, "could not open file for reading: %s", strerror(errno));
        LoadError(filename, errbuf);
        return 0;
    }
    if (fseek(fp, 0, SEEK_END) ||
        (filelength = ftell(fp)) == -1 ||
        fseek(fp, 0, SEEK_SET)) {
        snprintf(errbuf, 256, "couldn't get length of patch file: %s", strerror(errno));
        fclose(fp);
        LoadError(filename, errbuf);
        return 0;
    }
    if (filelength == 0) {
        fclose(fp);
        LoadError(filename, "patch file has zero length");
        return 0;
    } else if (filelength > 2097152) {
        fclose(fp);
        LoadError(filename, "patch file is too large");
        return 0;
    } else if (filelength < 128) {
        fclose (fp);
        LoadError(filename, "patch file is too small");
        return 0;
    }

    if (!(raw_patch_data = (unsigned char *)malloc(filelength))) {
        fclose(fp);
        LoadError(filename, "couldn't allocate memory for raw patch file");
        return 0;
    }

    if (fread(raw_patch_data, 1, filelength, fp) != (size_t)filelength) {
        snprintf(errbuf, 256, "short read on patch file: %s", strerror(errno));
        free(raw_patch_data);
        fclose(fp);
        LoadError(filename, errbuf);
        return 0;
    }
    fclose(fp);

    /* check if the file is a standard MIDI file */
    if (raw_patch_data[0] == 0x4d &&	/* "M" */
        raw_patch_data[1] == 0x54 &&	/* "T" */
        raw_patch_data[2] == 0x68 &&	/* "h" */
        raw_patch_data[3] == 0x64)	/* "d" */
        midshift = 2;
    else
        midshift = 0;

    /* scan SysEx or MIDI file for SysEx header */
    count = 0;
    datastart = 0;
    for (patchstart = 0; patchstart + midshift + 5 < filelength; patchstart++) {
        
        if (raw_patch_data[patchstart] == 0xf0 &&
            raw_patch_data[patchstart + 1 + midshift] == 0x43 &&
            raw_patch_data[patchstart + 2 + midshift] <= 0x0f &&
            raw_patch_data[patchstart + 3 + midshift] == 0x09 &&
            raw_patch_data[patchstart + 5 + midshift] == 0x00 &&
            patchstart + 4103 + midshift < filelength &&
            raw_patch_data[patchstart + 4103 + midshift] == 0xf7) {  /* DX7 32 voice dump */

            count = 32;
            datastart = patchstart + 6 + midshift;
            break;

        } else if (raw_patch_data[patchstart] == 0xf0 && 
                   raw_patch_data[patchstart + midshift + 1] == 0x43 && 
                   raw_patch_data[patchstart + midshift + 2] <= 0x0f && 
                   raw_patch_data[patchstart + midshift + 4] == 0x01 && 
                   raw_patch_data[patchstart + midshift + 5] == 0x1b &&
                   patchstart + midshift + 162 < filelength &&
                   raw_patch_data[patchstart + midshift + 162] == 0xf7) {  /* DX7 single voice (edit buffer) dump */

            _Pack(raw_patch_data,
                  raw_patch_data + patchstart + midshift + 6);

            datastart = 0;
            count = 1;
            break;
        }
    }
            
    /* assume raw DX7/TX7 data if no SysEx header was found. */
    /* assume the user knows what she is doing ;-) */

    if (count == 0)
        count = filelength / 128;

    /* Dr.T TX7 file needs special treatment */
    filename_length = strlen (filename);
    if ((!strcmp(filename + filename_length - 4, ".TX7") ||
         !strcmp(filename + filename_length - 4, ".tx7")) && filelength == 8192) {

        count = 32;
        filelength = 4096;
    }

    /* Voyetra SIDEMAN DX/TX */
    if (filelength == 9816 &&
        raw_patch_data[0] == 0xdf &&
        raw_patch_data[1] == 0x05 &&
        raw_patch_data[2] == 0x01 && raw_patch_data[3] == 0x00) {

        count = 32;
        datastart = 0x60f;
    }

    /* Double SySex bank */
    if (filelength == 8208 &&
        raw_patch_data[4104] == 0xf0 && raw_patch_data[4104 + 4103] == 0xf7) {

        memcpy(raw_patch_data + 4102, raw_patch_data + 4110, 4096);
        count = 64;
        datastart = 6;
    }

    /* finally, copy patchdata to the right location */
    if (count > Voices - Voice_Cursor) {  /* if ( voices-in-file > voices-to-end-of-buffer ) */
        cprintf(" Load - Patch file '%s' too big for remaining space:\n", filename);
        cprintf(" %d patches in file, only %d loaded!\n", count, Voices - Voice_Cursor);
        paktc();
        count = Voices - Voice_Cursor;
    }

    memcpy(voiceAddr(Voice_Cursor), raw_patch_data + datastart, 128 * count);
    free (raw_patch_data);
    cprintf(" File '%s' loaded.\n", filename);
    return count;
}

void
Save(int vstart, int vend)  /* range is inclusive, filename in FNameBuff */
{
    FILE *fh;
    long flength;

    if ((fh = fopen(FNameBuff, "wb")) != 0) {
        flength = (vend - vstart + 1) * VSIZEPACKED;
	if (flength != fwrite(voiceAddr(vstart), 1, flength, fh)) {
            cprintf("Save - Error writing file '%s':\n", FNameBuff);
            cprintf(" '%s'!\n", strerror(errno));
            paktc();
        }
	fclose(fh);
    } else {
        cprintf("Save - Error opening file '%s':\n", FNameBuff);
        cprintf(" '%s'!\n", strerror(errno));
        paktc();
    }
}

/* ==== VoiceEdit Stuff ==== */

void
ve_ShowAlg(void)
{
    int alg = SingleData[134],
        x,
        y;
    char *s = veAlg[alg];
    bool t;

    textattr(COLOR_LABEL);
    for (y = 12; y <= 20; y++) {
        x = 56;
	move(y, x);
        for (; x <= 72; x++) {
            switch (*s) {
                case '1':   t = (!!(ve_OpSelect&32));  goto printop;
                case '2':   t = (!!(ve_OpSelect&16));  goto printop;
                case '3':   t = (!!(ve_OpSelect& 8));  goto printop;
                case '4':   t = (!!(ve_OpSelect& 4));  goto printop;
                case '5':   t = (!!(ve_OpSelect& 2));  goto printop;
                case '6':   t = (!!(ve_OpSelect& 1));
                  printop:
                    textattr(t ? COLOR_OPON : COLOR_OPOFF);
                    cputch(*s++);
                    textattr(COLOR_LABEL);
                    break;
                case 0:
                    cputch(' ');
                    break;
                case '\n':
                    cputch(' ');
                    if (x == 72) s++;
                    break;
                case '^':
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
#if 0
                case '?':  /* -FIX- */
                    cputch(ACS_RTEE); s++;
                    break;
                case '?':  /* -FIX- */
                    cputch(ACS_TTEE); s++;
                    break;
#endif
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
    textattr(COLOR_DATA);
}

void
ve_FExtend(UBYTE fc, UBYTE ff, UBYTE mode, UBYTE y)
{
    unsigned long f;
    char ca[10];

    textattr(COLOR_LABEL);
    move(y, 47);
    if (mode) { /* fixed */
        f = veFFF[ff];
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
    textattr(COLOR_DATA);
}

void
ve_printParm(int parm)
{
    unsigned char val = SingleData[veParm[parm].Offset],
                  c;
    char *t;

    move(veParm[parm].Y, veParm[parm].X);
    if (parm == ve_Cursor) textattr(COLOR_CURSOR);
    switch (veParm[parm].Type) {
        case vptName:
            {
                UBYTE ca[11];

                memcpy(ca, SingleData + 145, 10);
                for (c = 0; c < 10; c++) {
                    if (ca[c] < 32)
                        ca[c]=183;  /* centered dot */
                    else if (ca[c] >= 128)
                        ca[c]=174;  /* (R) symbol = out-of-range */
                    else {
                        switch (ca[c]) {
                            case  92:  ca[c]=165;  break;  /* yen */
                            case 126:  ca[c]=187;  break;  /* >> */
                            case 127:  ca[c]=171;  break;  /* << */
                        }
                    }
                }
                ca[10] = 0;
                cprintf("%s", ca);
            }
            break;
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
            ve_FExtend(SingleData[veParm[parm+1].Offset],
                       SingleData[veParm[parm+2].Offset],
                       val,
                       veParm[parm].Y);
            break;
        case vptFC:
            if (val > 31)
                cprintf("??");
            else
                cprintf("%2d", val);
            ve_FExtend(val,
                       SingleData[veParm[parm+1].Offset],
                       SingleData[veParm[parm-1].Offset],
                       veParm[parm].Y);
            break;
        case vptFF:
            if (val > 99)
                cprintf("??");
            else
                cprintf("%2d", val);
            ve_FExtend(SingleData[veParm[parm-1].Offset],
                       val,
                       SingleData[veParm[parm-2].Offset],
                       veParm[parm].Y);
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
            cprintf("%s", NoteText(val));
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
    if (parm == ve_Cursor) textattr(COLOR_DATA);
}

void
ve_SetScreen(void)
{
    textattr(COLOR_DATA);
    cprintf(" TX/Edit Voice Edit");
    textattr(COLOR_LABEL);
    cprintf("     Name\n"
            "                                                            A\n"
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
    textattr(COLOR_DATA);
}

void
ve_DoScreen(void)
{
    int i;

    for (i = 0; i < VOICEPARMS; ve_printParm(i++));
}

void
ve_Name(int c)
{
    UBYTE i;

    for(i = 145; i < 154; i++) SingleData[i] = SingleData[i + 1];
    SingleData[154] = c;
    PutUnpacked();
    ve_printParm(0);
    ve_OpSelect = 63;
    ve_ShowAlg();
}

void
ve_Inc(int parm)
{
    unsigned char val = SingleData[veParm[parm].Offset],
                  max;
    
    max = vtypemax[veParm[parm].Type];
    if (!max) return;
    if (++val > max) val = 0;
    SingleData[veParm[parm].Offset] = val;
    PutParm(parm);
    ve_printParm(parm);
}

void
ve_Dec(int parm)
{
    unsigned char val = SingleData[veParm[parm].Offset],
                  max;
    
    max = vtypemax[veParm[parm].Type];
    if (!max) return;
    if (val)
        val--;
    else
        val = max;
    SingleData[veParm[parm].Offset] = val;
    PutParm(parm);
    ve_printParm(parm);
}

void
ve_Zero(void)
{
    UBYTE offset = veParm[ve_Cursor].Offset;

    if(ve_Cursor == 0) { /* do name */
        for(offset = 154; offset > 145; offset--)
        SingleData[offset] = SingleData[offset - 1];
        SingleData[145] = ' ';
        PutUnpacked();
        ve_OpSelect = 63;
        ve_ShowAlg();
    } else {
        SingleData[offset] = InitVoiceUnpacked[offset];
        PutParm(ve_Cursor);
    }
    ve_printParm(ve_Cursor);
}

void
ve_CurUp(void)
{
    int oldcursor = ve_Cursor;
    ve_Cursor = veParm[ve_Cursor].Up;
    ve_printParm(oldcursor);
    ve_printParm(ve_Cursor);
}

void
ve_CurDown(void)
{
    int oldcursor = ve_Cursor;
    ve_Cursor = veParm[ve_Cursor].Down;
    ve_printParm(oldcursor);
    ve_printParm(ve_Cursor);
}

void
ve_CurRight(void)
{
    int oldcursor = ve_Cursor;
    if(++ve_Cursor > VOICEPARMMAX) ve_Cursor = 0;
    ve_printParm(oldcursor);
    ve_printParm(ve_Cursor);
}

void
ve_CurLeft(void)
{
    int oldcursor = ve_Cursor;
    if(ve_Cursor)
        ve_Cursor--;
    else
        ve_Cursor = VOICEPARMMAX;
    ve_printParm(oldcursor);
    ve_printParm(ve_Cursor);
}

void
ve_Erase(void)
{
    memcpy(SingleData, InitVoiceUnpacked, VSIZEUNPACKED);
    // erase();
    // ve_SetScreen();
    ve_DoScreen();
    refresh();
    PutUnpacked();
}

void
ve_PgUp(void)
{
    if (Voice_Cursor) {
        Pack(Voice_Cursor);
        Unpack(--Voice_Cursor);
        ve_DoScreen();
        refresh();
        PutUnpacked();
    }
}

void
ve_PgDn(void)
{
    if (Voice_Cursor < Voices - 1) {
        Pack(Voice_Cursor);
        Unpack(++Voice_Cursor);
        ve_DoScreen();
        refresh();
        PutUnpacked();
    }
}

void
_ve_SetOps(void)
{
    UBYTE bd[7];

    bd[0] = 0xF0;
    bd[1] = 0x43;
    bd[2] = 0x10 | TXChannel;
    bd[3] = 1;
    bd[4] = 27;
    bd[5] = ve_OpSelect;
    bd[6] = 0xF7;
    PutMidiMsg(bd, 7);
}

void
ve_SetOps(int o)
{
    UBYTE i;

    ve_OpSelect ^= o;
    for (i = 1; i < 7; i++) {
        if (ve_OpSelect & (1 << (6 - i)))
            textattr(COLOR_OPON);
        else
            textattr(COLOR_OPOFF);
	move(3 + i, 2);
        cprintf("%c", i + '0');
	move(12 + i, 2);
        cprintf("%c", i + '0');
    }
    /* textattr(COLOR_DATA);  not needed with ve_ShowAlg() */
    ve_ShowAlg();
    _ve_SetOps();
}

void
ve_Number(int c)
{
    unsigned char val = SingleData[veParm[ve_Cursor].Offset],
                  typ = veParm[ve_Cursor].Type,
                  max = vtypemax[typ];
    
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
    SingleData[veParm[ve_Cursor].Offset] = val;
    PutParm(ve_Cursor);
    ve_printParm(ve_Cursor);
}

void
ve_Help(void)
{
    erase();
    cprintf(" TX/Edit v" VERSIONSTRING " Voice Edit Help\n\n");

    cprintf(" Up / Down /\n"
            " Right / Left  - Move cursor\n\n");

    cprintf(" +             - Increment field\n"
            " -             - Decrement field\n"
            " 0 through 9   - Enter data into field\n"
            " Backspace     - Zero/reset field\n\n");

    cprintf(" A through Z   - Rotate letters into name (name field only)\n\n"

            " F1 through F6 - Enable/disable operators 1 through 6\n\n");

    cprintf(" Meta-P        - Put edit buffer as single voice\n"
            " Meta-!        - Erase edit buffer to init voice\n"
            " PgUp / PgDn   - Move to previous / next voice\n"

            " Esc / Meta-L  - Return to librarian mode\n\n"

            " h / Meta-H    - Display this help screen\n\n");

    paktc();
    erase();
    ve_SetScreen();
    ve_DoScreen();
}

void
ve_CharIn(int c)
{
    if(!ve_Cursor && c>=32 && c<=127) {
        ve_Name(c);
    } else {
        if(c >= 'A' && c <= 'Z')
            c = tolower(c);
        switch(c) {
            case '+':
            case '=':           ve_Inc(ve_Cursor);  break;
            case '-':           ve_Dec(ve_Cursor);  break;
#if 0
            case kc_Del:
#endif
            case KEY_BACKSPACE: ve_Zero();          break;
            case KEY_UP:        ve_CurUp();         break;
            case KEY_DOWN:      ve_CurDown();       break;
            case KEY_RIGHT:     ve_CurRight();      break;
            case KEY_LEFT:      ve_CurLeft();       break;
            case KEY_METAL:
            case KEY_ESC:       newmode=MODELIB;    break;
            case KEY_F(1):      ve_SetOps(32);      break;
            case KEY_F(2):      ve_SetOps(16);      break;
            case KEY_F(3):      ve_SetOps( 8);      break;
            case KEY_F(4):      ve_SetOps( 4);      break;
            case KEY_F(5):      ve_SetOps( 2);      break;
            case KEY_F(6):      ve_SetOps( 1);      break;
            case KEY_PPAGE:     ve_PgUp();          break;
            case KEY_NPAGE:     ve_PgDn();          break;
            case KEY_METAP:     PutUnpacked();      break;
            case KEY_METAEX:    ve_Erase();         break;
            case KEY_METAH:
            case 'h':           ve_Help();          break;
            default:
                if(c>='0' && c<='9') {
                    ve_Number(c);
                } else {
                    gotomsg();
                    cprintf(" Type <Alt-H> for help...");
                    beep();
#ifdef DEBUG
                    cprintf("keycode=0x%04x", c);
#endif
                }
                break;
        }
    }
}

/* ==== Librarian Stuff ==== */

void
LibPrintVName(int voice)  /* print name at correct cursor position */
{
    if (Voice_TOS <= voice && voice < Voice_TOS + LibNames) {
        move((voice-Voice_TOS)%LibRows + 2,
             (voice-Voice_TOS)/LibRows * 15 + 1);
        if (voice<Voices) {
	    textattr(COLOR_LABEL);
	    if (voice < 10000) {
                cprintf("%4d ", voice);
            } else {
		cprintf("?%03d ", voice % 1000);
            }
            if (voice==Voice_Cursor) {
                if(blocking)
                    textattr(COLOR_BCURSOR);
                else
                    textattr(COLOR_CURSOR);
            } else {
                if (blocking||blocked) {
                    if (Voice_BStart<=voice&&voice<=Voice_BEnd) {
                        textattr(COLOR_BLOCK);
                    } else
                        textattr(COLOR_DATA);
                } else
                    textattr(COLOR_DATA);
            }
            PrintVName(voice);
            textattr(COLOR_DATA);
        } else {
            cprintf("               ");
	}
    }
}

void
LibSetScreen(void)  /* setup librarian screen */
{
#if USE_ALSA_MIDI
    char temp[80];

    snprintf(temp, 80, "TX/Edit v%s (%d:%d)", VERSIONSTRING, MidiClient, MidiPort);
    mvaddstr(0, 1, temp);
#else
    mvaddstr(0, 1, "TX/Edit v" VERSIONSTRING);
#endif
}

void
LibDoScreen(void)  /* print voice names */
{
    int i;

    for (i=Voice_TOS; i<Voice_TOS+LibNames; LibPrintVName(i++));
}

void
SetBlock(void)  /* fake a block under cursor if no block marked */
{
    if (!(blocking || blocked))
        Voice_BMark = Voice_BStart = Voice_BEnd = Voice_Cursor;
}

void
CancelBlock(void)
{
    blocking = FALSE;
    blocked = FALSE;
    LibBlockAction = noop;
    LibDoScreen();
    gotomsg();
    clrtoeol();
}

void
StartBlock(void)  /* or cancel block if one is marked */
{
    if (!(blocking || blocked)) {
        blocking = TRUE;
        Voice_BMark = Voice_BStart = Voice_BEnd = Voice_Cursor;
        LibPrintVName(Voice_Cursor);
    } else
        CancelBlock();
}

void
EndBlock(void)
{
    blocking = FALSE;
    blocked = TRUE;
    LibPrintVName(Voice_Cursor);
}

void
EscBlock(void)  /* cancel block if one is marked */
{
    if(blocking || blocked)
        CancelBlock();
}

void
CheckBlock(void)
{
    if (blocking) {
        if (Voice_Cursor < Voice_BMark) {
            Voice_BStart = Voice_Cursor;
            Voice_BEnd   = Voice_BMark;
        } else {
            Voice_BStart = Voice_BMark;
            Voice_BEnd   = Voice_Cursor;
        }
    }
}

void
LibCurUp(void)
{
    if (Voice_Cursor) {
        int vc;

        vc = --Voice_Cursor;
        CheckBlock();
        if (vc >= Voice_TOS) {
            LibPrintVName(vc);
            LibPrintVName(++vc);
        } else {
            Voice_TOS = max(Voice_TOS - LibRows, 0);
            LibDoScreen();
        }
    }
}

void
LibCurDown(void)
{
    if (Voice_Cursor < Voices - 1) {
        int vc;

        vc = ++Voice_Cursor;
        CheckBlock();
        if (vc < Voice_TOS + LibNames) {
            LibPrintVName(vc);
            LibPrintVName(--vc);
        } else {
            Voice_TOS += LibRows;
            LibDoScreen();
        }
    }
}

void
LibCurRight(void)
{
    if (Voice_Cursor < Voices - 1) {
        int oldvc, newvc;

        oldvc = Voice_Cursor;
        Voice_Cursor = newvc = min(oldvc + LibRows, Voices - 1);
        CheckBlock();
        if (newvc < Voice_TOS + LibNames) {
            if (blocking) {
                for (; oldvc <= newvc; LibPrintVName(oldvc++));
            } else {
                LibPrintVName(oldvc);
                LibPrintVName(newvc);
            }
        } else {
            Voice_TOS += LibRows;
            LibDoScreen();
        }
    }
}

void
LibCurLeft(void)
{
    if (Voice_Cursor) {
        int oldvc, newvc;

        oldvc = Voice_Cursor;
        Voice_Cursor = newvc = max(oldvc - LibRows, 0);
        CheckBlock();
        if (newvc >= Voice_TOS) {
            if (blocking) {
                for (; oldvc >= newvc; LibPrintVName(oldvc--));
            } else {
                LibPrintVName(oldvc);
                LibPrintVName(newvc);
            }
        } else {
            Voice_TOS = max(Voice_TOS - LibRows, 0);
            LibDoScreen();
        }
    }
}

void
LibCtrlPgUp(void)
{
    if (Voice_Cursor) {
        int oldvc, newvc;

        oldvc = Voice_Cursor;
        Voice_Cursor = newvc = max(Voice_Cursor - 1, 0) & 0xffe0;
        CheckBlock();
        if (newvc >= Voice_TOS) {
            if (blocking) {
                for (; oldvc >= newvc; LibPrintVName(oldvc--));
            } else {
                LibPrintVName(oldvc);
                LibPrintVName(newvc);
            }
        } else {
            Voice_TOS=newvc;
            LibDoScreen();
        }
    }
}

void
LibCtrlPgDn(void)
{
    if (Voice_Cursor < Voices-1) {
        int oldvc, newvc;
        
        oldvc = Voice_Cursor;
        Voice_Cursor = newvc = min(((Voice_Cursor + 33) & 0xffe0) - 1, Voices - 1);
        CheckBlock();
        if (newvc < Voice_TOS + LibNames) {
            if (blocking) {
                for (; oldvc <= newvc; LibPrintVName(oldvc++));
            } else {
                LibPrintVName(oldvc);
                LibPrintVName(newvc);
            }
        } else {
            Voice_TOS=max(newvc - LibNames + 1, 0);
            LibDoScreen();
        }
    }
}

void
LibCurEnd(void)
{
    Voice_Cursor = Voices - 1;
    Voice_TOS = max(Voices - LibNames, 0);
    CheckBlock();
    LibDoScreen();
}

void
LibCurHome(void)
{
    Voice_Cursor = 0;
    Voice_TOS = 0;
    CheckBlock();
    LibDoScreen();
}

void
LibPgUp(void)
{
    if (Voice_Cursor) {
        int oldvc, newvc;

        oldvc = Voice_Cursor;
        Voice_Cursor = newvc = max(Voice_Cursor - 1, 0) & 0xfff0;
        CheckBlock();
        if (newvc >= Voice_TOS) {
            if (blocking) {
                for (; oldvc >= newvc; LibPrintVName(oldvc--));
            } else {
                LibPrintVName(oldvc);
                LibPrintVName(newvc);
            }
        } else {
            Voice_TOS = newvc;
            LibDoScreen();
        }
    }
}

void
LibPgDn(void)
{
    if (Voice_Cursor < Voices-1) {
        int oldvc, newvc;
        
        oldvc = Voice_Cursor;
        Voice_Cursor = newvc = min(((Voice_Cursor + 17) & 0xfff0) - 1, Voices - 1);
        CheckBlock();
        if (newvc < Voice_TOS + LibNames) {
            if (blocking) {
                for (; oldvc <= newvc; LibPrintVName(oldvc++));
            } else {
                LibPrintVName(oldvc);
                LibPrintVName(newvc);
            }
        } else {
            Voice_TOS = max(newvc - LibNames + 1, 0);
            LibDoScreen();
        }
    }
}

void
LibCtrlRight(void)
{
    
    Voice_Cursor = min(Voice_Cursor + LibNames, Voices - 1);
    Voice_TOS    = min(Voice_TOS    + LibNames, max(Voices - LibNames, 0));
    CheckBlock();
    LibDoScreen();
}

void
LibCtrlLeft(void)
{
    if (Voice_Cursor) {
        Voice_Cursor = max(Voice_Cursor - LibNames, 0);
        Voice_TOS    = max(Voice_TOS    - LibNames, 0);
        CheckBlock();
        LibDoScreen();
    }
}

void
LBA_Copy(void)
{
#ifdef DEBUG
    if (!blocked) { cprintf("LBA_Copy - not Blocked?!"); return; }
#endif
    memmove(voiceAddr(Voice_Cursor),
	    voiceAddr(Voice_BStart),
           min(Voice_BEnd - Voice_BStart + 1, Voices - Voice_Cursor) * VSIZEPACKED);
    CancelBlock();
}

void
LibCopy(void)
{
    SetBlock();
    EndBlock();
    LibBlockAction = LBA_Copy;
    gotomsg();
    cprintf("Move Cursor to Copy destination and press return...");
}

void
LibDelete(void)
{
    int i;

    SetBlock();
    if (Voices > Voice_BEnd + 1)
        memmove(voiceAddr(Voice_BStart),
		voiceAddr(Voice_BEnd + 1),
               (Voices - Voice_BEnd - 1) * VSIZEPACKED);
    for (i = Voices - Voice_BEnd + Voice_BStart - 1; i < Voices; EraseVoice(i++));
    CancelBlock();
}

void
LibErase(void)
{
    int i;

    SetBlock();
    for (i = Voice_BStart; i <= Voice_BEnd; EraseVoice(i++));
    CancelBlock();
}

void
LibLoad(void)
{
    erase();
    LibSetScreen();
    cprintf("\n\n");
    if (FileRequest(FNameBuff, "TX/Edit Load Voices")) {
        Load(FNameBuff);
    }
    erase();
    LibSetScreen();
    LibDoScreen();
}

void
LBA_Move(void)  /* swap patches, overlapping ranges CF! */
{
    int i;
    UBYTE *v1, *v2;

#ifdef DEBUG
    if (!blocked) { cprintf("LBA_Move - not Blocked?!"); return; }
#endif
    for (i = Voice_BStart;
            (i <= Voice_BEnd) && (Voice_Cursor + i - Voice_BStart < Voices); i++) {
        v1=voiceAddr(i);
        v2=voiceAddr(Voice_Cursor + i - Voice_BStart);
        memmove(SingleData, v1, VSIZEPACKED);
        memmove(v1, v2,         VSIZEPACKED);
        memmove(v2, SingleData, VSIZEPACKED);
    }
    CancelBlock();
}

void
LibMove(void)
{
    SetBlock();
    EndBlock();
    LibBlockAction = LBA_Move;
    gotomsg();
    cprintf("Move Cursor to Move destination and press return...");
}

void
LibSave(void)
{
    if (blocking) {
        erase();
        LibSetScreen();
        cprintf("\n\n");
        Save(Voice_BStart, Voice_BEnd);
        erase();
        LibSetScreen();
        CancelBlock();
    } else {
        gotomsg();
        textattr(COLOR_MSG);
        cprintf(" Save - Must mark block first! - ");
        paktc();
    }
}

void
LibSaveAs(void)
{
    if (blocking) {
        erase();
        LibSetScreen();
        cprintf("\n\n");
        if (FileRequest(FNameBuff, "TX/Edit Save Voices")) {
            Save(Voice_BStart, Voice_BEnd);
        }
        erase();
        LibSetScreen();
        CancelBlock();
    } else {
        gotomsg();
        textattr(COLOR_MSG);
        cprintf("Save As - Must mark block first! - ");
        paktc();
    }
}

UBYTE
lstol(UBYTE c)
{
    if (c >= 'A' && c <= 'Z')
        return c + 32;
    else
        return c;
}

int
_LibSortCmp(const UBYTE *v1, const UBYTE *v2)
{
    int i;
    for (i = 118; i < 128; i++) {
        if (lstol(v1[i]) != lstol(v2[i]))
            return lstol(v1[i]) - lstol(v2[i]);
    }
    return 0;
}

void
LibSort(void)
{
    SetBlock();
    if (Voice_BStart != Voice_BEnd) {
        qsort(voiceAddr(Voice_BStart), Voice_BEnd - Voice_BStart + 1,
              VSIZEPACKED, (int (*)())_LibSortCmp);
    }
    CancelBlock();
}

int
_LibSortCmpPatch(const UBYTE *v1, const UBYTE *v2)
{
    int i;
#define SORT_BY_ALGORITHM_FIRST
#ifdef  SORT_BY_ALGORITHM_FIRST
    if (v1[110] != v2[110]) return v1[110] - v2[110];
#endif
    for (i = 0; i < 118; i++) {
        if (v1[i] != v2[i])
            return v1[i] - v2[i];
    }
    return 0;
}

void
LibSortPatch(void)
{
    SetBlock();
    if (Voice_BStart != Voice_BEnd) {
        qsort(voiceAddr(Voice_BStart), Voice_BEnd - Voice_BStart + 1,
              VSIZEPACKED, (int (*)())_LibSortCmpPatch);
    }
    CancelBlock();
}

void
LibHelp(void)
{
    erase();
    cprintf(" TX/Edit v" VERSIONSTRING " Help\n\n");

    cprintf(" a / Meta-A  - Save block As\n"
            " b / Meta-B  - Start block\n"
            " c / Meta-C  - Copy\n"
            "     Meta-D  - Delete\n"
            " e / Meta-E  - Enter Edit mode\n");

#if 0
            " g / Meta-G  - Get (voice or 32 voices?)\n"
#endif

    cprintf(" h / Meta-H  - Display this help screen\n\n"
            " m / Meta-M  - Move\n"
            " o / Meta-O  - Load\n"
            " p / Meta-P  - Put single voice (use space to step-send)\n");

#if 0
            " r / Meta-R  - Receive (voice or 32 voices?)\n"
#endif

    cprintf("     Meta-S  - Save block\n"
            "     Meta-T  - Put bank (32 voices starting at cursor)\n"
            "     Meta-=  - Sort block by patch names\n"
            "     Meta-+  - Sort block by patch data\n"
            "     Meta-!  - Erase voice\n");

#if 0
            "     Meta-#  - Change MIDI channel\n"
#endif

    cprintf(" Escape      - Cancel block\n"
            " Enter       - Perform pending block action\n"
            " Up / Down /\n"
            " Right / Left /\n"
            " PgUp / PgDn - Move cursor\n");

#if 0
    /* Ctrl-Right, Ctrl-Left, Ctrl-PgUp, Ctrl-PgDn, End, Ctrl-End, Home, Ctrl-Home could have functions */
#endif

    paktc();
    erase();
    LibSetScreen();
    LibDoScreen();
}

void
vl_CharIn(int c)
{
    if(c >= 'A' && c <= 'Z')
        c = tolower(c);
    switch (c) {
        case KEY_METAA:
        case 'a':       LibSaveAs();            break;
        case KEY_METAB:
        case 'b':       StartBlock();           break;
        case KEY_METAC:
        case 'c':       LibCopy();              break;
        case KEY_METAD: LibDelete();            break;
        case KEY_METAE:
        case 'e':       newmode=MODEEDIT;       break;
#if 0
        case KEY_METAG:
        case 'g':       cprintf("Get!!!!");     break;
#endif
        case KEY_METAH:
        case 'h':       LibHelp();              break;
        case KEY_METAM:
        case 'm':       LibMove();              break;
        case KEY_METAO:
        case 'o':       LibLoad();              break;
        /* Space for Step-Send */
        case ' ':       LibCurDown();  /* and fall through to: */
        case KEY_METAP:
        case 'p':       PutPacked(Voice_Cursor); break;
#if 0
        case KEY_METAR:
        case 'r':       cprintf("Receive!!!!"); break;
#endif
        case KEY_METAS:   LibSave();              break;
        case KEY_METAT:   PutBank(Voice_Cursor);  break;
        case KEY_METAEQ:  LibSort();              break;
        case KEY_METAPL:  LibSortPatch();         break;
        case KEY_METAEX:  LibErase();             break;
#if 0
        case KEY_METANU:
        case '#':       cprintf("Channel!!!!"); break;
#endif
        case KEY_ESC:       EscBlock();         break;
        case KEY_ENTER:     LibBlockAction();   break;
        case KEY_UP:        LibCurUp();         break;
        case KEY_DOWN:      LibCurDown();       break;
        case KEY_RIGHT:     LibCurRight();      break;
        case KEY_LEFT:      LibCurLeft();       break;
#if 0
        case kc_CtrlRight:  LibCtrlRight();     break;
        case kc_CtrlLeft:   LibCtrlLeft();      break;
#endif
        case KEY_PPAGE:     LibPgUp();          break;
        case KEY_NPAGE:     LibPgDn();          break;
#if 0
        case kc_CtrlPgUp:   LibCtrlPgUp();      break;
        case kc_CtrlPgDn:   LibCtrlPgDn();      break;
        case kc_End:
        case kc_CtrlEnd:    LibCurEnd();        break;
        case kc_Home:
        case kc_CtrlHome:   LibCurHome();       break;
#endif
        default:
            gotomsg();
            cprintf(" Type <Alt-H> for help...");
            beep();
#ifdef DEBUG
            cprintf("keycode=0x%04x, 0%o", c, c);
#endif
    }
}

/* ==== Control ==== */

void
TXsetmode(void)
{
    /* cleanup from old mode */
    switch(txmode) {
        case MODELIB:
            blocking = FALSE;
            blocked = FALSE;
            LibBlockAction = noop;
            break;
        case MODEEDIT:
            if(ve_OpSelect != 63) {
                ve_OpSelect = 63;
                _ve_SetOps();
            }
            Pack(Voice_Cursor);
            break;
    }
    txmode=newmode;
    /* set up for new mode */
    switch(txmode) {
        case MODELIB:
            if (Voice_Cursor < Voice_TOS || Voice_Cursor >= Voice_TOS + LibNames) {
                Voice_TOS = min(Voice_Cursor & 0xfff0, max(Voices - LibNames, 0));
            }
            erase();
            LibSetScreen();
            LibDoScreen();
            refresh();
            break;
        case MODEEDIT:
            erase();
            Unpack(Voice_Cursor);
            ve_SetScreen();
            ve_DoScreen();
            refresh();
            PutUnpacked();
            break;
    }
}

int
main(int argc, char ** argv)
{
    int c;

    static struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"voices", 1, 0, 'v'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "hv:", long_options, NULL)) != EOF) {
        switch (c) {
          case 'v':
            Voices = atoi(optarg);
            if (Voices < 32) Voices = 32;
            break;
          case 'h':
          default:
            fprintf(stderr, "TX/Edit v" VERSIONSTRING " (c)2004 Sean Bolton\n"
                            "options:\n"
                            "-vnnn --voices=nnn   allocate nnn voice slots\n"
                            "-h    --help         print this message and exit\n");
            exit(c == 'h' ? 0 : 1);
        }
    }

    TXInit();
    if (optind < argc) { /* load command line arguments */
        scrollok(stdscr, TRUE);
        erase();
        LibSetScreen();
        cprintf("\n\n");
        for (c = optind; c < argc; c++) {
            Voice_Cursor += Load(argv[c]);
        }
        paktc();
        Voice_Cursor = 0;
        scrollok(stdscr, FALSE);
    }
    newmode=MODELIB;
    TXsetmode();
    do {
refresh(); /* -FIX- */
        c = cgetch();
        switch (c) {
            case KEY_METAQ:     newmode=MODEQUIT;  break;
            default:
                switch(txmode) {
                    case MODELIB:   vl_CharIn(c);  break;
                    case MODEEDIT:  ve_CharIn(c);  break;
                }
                break;
        }
        if (txmode != newmode) TXsetmode();
    } while (txmode != MODEQUIT);
    TXTerm(RETURN_OK);  /* never returns */
    return 0;
}
