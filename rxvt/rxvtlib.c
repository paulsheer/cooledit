/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/*--------------------------------*-C-*---------------------------------*
 * File:	rxvtlib.c
 *----------------------------------------------------------------------*
 * $Id: command.c,v 1.85.2.23 1999/08/12 16:32:39 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1992      John Bovey, University of Kent at Canterbury <jdb@ukc.ac.uk>
 *				- original version
 * Copyright (C) 1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 * 				- extensive modifications
 * Copyright (C) 1995      Garrett D'Amore <garrett@netcom.com>
 *				- vt100 printing
 * Copyright (C) 1995      Steven Hirsch <hirsch@emba.uvm.edu>
 *				- X11 mouse report mode and support for
 *				  DEC "private mode" save/restore functions.
 * Copyright (C) 1995      Jakub Jelinek <jj@gnu.ai.mit.edu>
 *				- key-related changes to handle Shift+function
 *				  keys properly.
 * Copyright (C) 1997      MJ Olesen <olesen@me.queensu.ca>
 *				- extensive modifications
 * Copyright (C) 1997      Raul Garcia Garcia <rgg@tid.es>
 *				- modification and cleanups for Solaris 2.x
 *				  and Linux 1.2.x
 * Copyright (C) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
 * Copyright (C) 1998      Geoff Wing <gcw@pobox.com>
 * Copyright (C) 1998      Alfredo K. Kojima <kojima@windowmaker.org>
 * Copyright (C) 1996-2022 Paul Sheer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*/

/*************************************************************************/
/* This code has only the vaguest resemblance to rxvt-2.6.1 - Paul Sheer */
/*************************************************************************/


#include "inspect.h"
#include "rxvtlib.h"
#include <stringtools.h>
#include <remotefs.h>

static  int      Xfd = -1;
static  unsigned long PrivateModes = PrivMode_Default;
static  unsigned long SavedModes = PrivMode_Default;
static  int      refresh_count = 0, refresh_limit = 1, refresh_type = SLOW_REFRESH;
static  struct _MEvent MEvent = {
    0, CurrentTime, 0, AnyButton
};
#ifdef UTMP_SUPPORT
static  char     ut_id[5];
# ifndef USE_SYSV_UTMP
static  int      utmp_pos;
# endif
#endif
static  const char *const def_colorName[] = {
    COLOR_FOREGROUND,
    COLOR_BACKGROUND,

    "Black",			
#ifndef NO_BRIGHTCOLOR
    "Red",			
    "Green3",			
    "Yellow3",			
    "Blue",
    "Magenta3",			
    "Cyan3",			
    "AntiqueWhite",		

    "Grey25",			
#endif				/* NO_BRIGHTCOLOR */
    "Orange red",
    "Green",			
    "Yellow",			
    "Sky Blue",
    "Magenta",			
    "Cyan",			
    "White",			
#ifndef NO_CURSORCOLOR
    COLOR_CURSOR_BACKGROUND,
    COLOR_CURSOR_FOREGROUND,
#endif				/* ! NO_CURSORCOLOR */
    NULL,			
    NULL			
#ifndef NO_BOLDUNDERLINE
	, NULL,			
    NULL			
#endif				/* ! NO_BOLDUNDERLINE */
#ifdef KEEP_SCROLLCOLOR
	, COLOR_SCROLLBAR,
    COLOR_SCROLLTROUGH
#endif				/* KEEP_SCROLLCOLOR */
};
#ifdef MULTICHAR_SET
static  const char *const def_mfontName[] = {
    MFONT_LIST
};
#endif				/* MULTICHAR_SET */
static  const char *const def_fontName[] = {
    NFONT_LIST
};
#ifdef MENUBAR
static  int      menu_readonly = 1;
static  int      Arrows_x = 0;
static  struct _Arrows Arrows[NARROWS] = {
    {
     'l', "\003\033[D"}
    , {
       'u', "\003\033[A"}
    , {
       'd', "\003\033[B"}
    , {
       'r', "\003\033[C"}
};
#if (MENUBAR_MAX > 1)
static  int      Nbars = 0;
static  bar_t   *CurrentBar = NULL;
#else				/* (MENUBAR_MAX > 1) */
static  bar_t    BarList;
static  bar_t   *CurrentBar = &BarList;
#endif				/* (MENUBAR_MAX > 1) */
static  menu_t  *ActiveMenu = NULL;
#endif
#ifdef MULTICHAR_SET
static short           multi_byte;
static short           lost_multi;
static enum _chstat    chstat;
#else
#endif
#ifdef MULTICHAR_SET
static ENC_METHOD      encoding_method;
#endif
static  int      prev_nrow = -1, prev_ncol = -1;
#ifdef MULTICHAR_SET
#ifdef KANJI
static  void     (*multichar_decode) (unsigned char *str, int len) = eucj2jis;
#else				/* then we must be BIG5 to get in here */
# ifdef ZH
static  void     (*multichar_decode) (unsigned char *str, int len) = big5dummy;
# else
#  ifdef ZHCN			/* The GB fonts are in iso-2022 encoding (JIS). */
static  void     (*multichar_decode) (unsigned char *str, int len) = gb2jis;
#  endif
# endif
#endif
#endif				/* MULTICHAR_SET */
#ifndef NEXT_SCROLLBAR
#ifdef XTERM_SCROLLBAR		/* bitmap scrollbar */
static  char     sb_bits[] = { 0xaa, 0x0a, 0x55, 0x05 };
#endif				/* ! XTERM_SCROLLBAR */
#else				/* ! NEXT_SCROLLBAR */
static  GC       blackGC, whiteGC, grayGC, darkGC, stippleGC;
static  Pixmap   dimple, upArrow, downArrow, upArrowHi, downArrowHi;
static const char     *const SCROLLER_DIMPLE[] = {
    ".%###.",
    "%#%%%%",
    "#%%...",
    "#%..  ",
    "#%.   ",
    ".%.  ."
};
static const char     *const SCROLLER_ARROW_UP[] = {
    ".............",
    ".............",
    "......%......",
    "......#......",
    ".....%#%.....",
    ".....###.....",
    "....%###%....",
    "....#####....",
    "...%#####%...",
    "...#######...",
    "..%#######%..",
    ".............",
    "............."
};
static const char     *const SCROLLER_ARROW_DOWN[] = {
    ".............",
    ".............",
    "..%#######%..",
    "...#######...",
    "...%#####%...",
    "....#####....",
    "....%###%....",
    ".....###.....",
    ".....%#%.....",
    "......#......",
    "......%......",
    ".............",
    "............."
};
static const char     *const HI_SCROLLER_ARROW_UP[] = {
    "             ",
    "             ",
    "      %      ",
    "      %      ",
    "     %%%     ",
    "     %%%     ",
    "    %%%%%    ",
    "    %%%%%    ",
    "   %%%%%%%   ",
    "   %%%%%%%   ",
    "  %%%%%%%%%  ",
    "             ",
    "             "
};
static const char     *const HI_SCROLLER_ARROW_DOWN[] = {
    "             ",
    "             ",
    "  %%%%%%%%%  ",
    "   %%%%%%%   ",
    "   %%%%%%%   ",
    "    %%%%%    ",
    "    %%%%%    ",
    "     %%%     ",
    "     %%%     ",
    "      %      ",
    "      %      ",
    "             ",
    "             "
};
static const unsigned char stp_bits[] =
    { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
#endif				/* ! NEXT_SCROLLBAR */

static  bgPixmap_t bgPixmap = { 0, 0, 50, 50, None };
#ifdef XPM_BACKGROUND
static  XpmAttributes xpmAttr;
#endif



void rxvtlib_init (rxvtlib *o, int charset_8bit)
{E_
    memset (o, 0, sizeof (rxvtlib));
    o->charset_8bit = charset_8bit;
    if (charset_8bit)
        o->fontname = "rxvt8bit";
    else
        o->fontname = "rxvt";
    o->Xfd = Xfd;
    o->PrivateModes = PrivateModes;
    o->SavedModes = SavedModes;
    o->refresh_count = refresh_count;
    o->refresh_limit = refresh_limit;
    o->refresh_type = refresh_type;
    o->MEvent = MEvent;
    memcpy (o->def_colorName, def_colorName, sizeof (def_colorName));
#ifdef MULTICHAR_SET
    memcpy (o->def_mfontName, def_mfontName, sizeof (def_mfontName));
#endif				/* MULTICHAR_SET */
    memcpy (o->def_fontName, def_fontName, sizeof (def_fontName));
#ifdef MENUBAR
    o->menu_readonly = menu_readonly;
    o->Arrows_x = Arrows_x;
    memcpy (o->Arrows, Arrows, sizeof (Arrows));
#if (MENUBAR_MAX > 1)
    o->Nbars = Nbars;
    o->CurrentBar = CurrentBar;
#else				/* (MENUBAR_MAX > 1) */
    o->CurrentBar = CurrentBar;
#endif				/* (MENUBAR_MAX > 1) */
    o->ActiveMenu = ActiveMenu;
#endif
    o->prev_nrow = prev_nrow;
    o->prev_ncol = prev_ncol;
#ifdef MULTICHAR_SET
#ifdef KANJI
    o->multichar_decode = multichar_decode;
#else				/* then we must be BIG5 to get in here */
# ifdef ZH
    o->multichar_decode = multichar_decode;
# else
#  ifdef ZHCN			/* The GB fonts are in iso-2022 encoding (JIS). */
    o->multichar_decode = multichar_decode;
#  endif
# endif
#endif
#endif				/* MULTICHAR_SET */
#ifndef NEXT_SCROLLBAR
#ifdef XTERM_SCROLLBAR		/* bitmap scrollbar */
    memcpy (o->sb_bits, sb_bits, sizeof (sb_bits));
#if (SB_WIDTH != 15)
#error Error, check scrollbar width (SB_WIDTH).It must be 15 for XTERM_SCROLLBAR
#endif
#else				/* XTERM_SCROLLBAR */
#endif				/* ! XTERM_SCROLLBAR */
#else				/* ! NEXT_SCROLLBAR */
    memcpy (o->SCROLLER_DIMPLE, SCROLLER_DIMPLE, sizeof (SCROLLER_DIMPLE));
    memcpy (o->SCROLLER_ARROW_UP, SCROLLER_ARROW_UP, sizeof (SCROLLER_ARROW_UP));
    memcpy (o->SCROLLER_ARROW_DOWN, SCROLLER_ARROW_DOWN, sizeof (SCROLLER_ARROW_DOWN));
    memcpy (o->HI_SCROLLER_ARROW_UP, HI_SCROLLER_ARROW_UP, sizeof (HI_SCROLLER_ARROW_UP));
    memcpy (o->HI_SCROLLER_ARROW_DOWN, HI_SCROLLER_ARROW_DOWN, sizeof (HI_SCROLLER_ARROW_DOWN));
    memcpy (o->stp_bits, stp_bits, sizeof (stp_bits));
#endif				/* ! NEXT_SCROLLBAR */
    o->old_height = -1;
    o->buffer = NULL;
    o->currmaxcol = 0;
#ifdef MULTICHAR_SET
    o->oldcursormulti = 0;
#endif
    o->oldcursor.row = -1;
    o->oldcursor.col = -1;
    o->bgPixmap = bgPixmap;
}

static void myfree (void *x)
{E_
    if (x)
	free (x);
}

void rxvtlib_shut (rxvtlib * o)
{E_
    int i;

    if (o->cmd_fd >= 0) {
        CRemoveWatch (o->cmd_fd, NULL, 3);
        close (o->cmd_fd);
        o->cmd_fd = -1;
    }

    for (i = 0; i < o->TermWin.nrow; i++) {
	if (o->swap.text)
	    myfree (o->swap.text[i]);
	if (o->swap.rend)
	    myfree (o->swap.rend[i]);
	if (o->drawn_text)
	    myfree (o->drawn_text[i]);
	if (o->drawn_rend)
	    myfree (o->drawn_rend[i]);
    }

    for (i = 0; i < o->TermWin.nrow + o->TermWin.saveLines; i++) {
	if (o->screen.text)
	    myfree (o->screen.text[i]);
	if (o->screen.rend)
	    myfree (o->screen.rend[i]);
    }

    myfree (o->screen.text);
    myfree (o->buf_text);
    myfree (o->drawn_text);
    myfree (o->swap.text);

    myfree (o->screen.tlen);
    myfree (o->buf_tlen);
    myfree (o->swap.tlen);

    myfree (o->screen.rend);
    myfree (o->buf_rend);
    myfree (o->drawn_rend);
    myfree (o->swap.rend);

    myfree (o->buffer);
    myfree (o->v_buffer);

    myfree (o->tabs);

#ifndef UTF8_FONT
    if (o->TermWin.fontset)
	XFreeFontSet (o->Xdisplay, o->TermWin.fontset);
#endif
#ifdef USE_XIM
    if (o->Input_Context)
	XDestroyIC (o->Input_Context);
#endif		/* USE_XIM */
    if (o->TermWin.gc)
	XFreeGC (o->Xdisplay, o->TermWin.gc);
    if (o->scrollbarGC)
	XFreeGC (o->Xdisplay, o->scrollbarGC);
    if (o->topShadowGC)
	XFreeGC (o->Xdisplay, o->topShadowGC);
    if (o->botShadowGC)
	XFreeGC (o->Xdisplay, o->botShadowGC);
#ifndef UTF8_FONT
#ifdef MULTICHAR_SET
    if (o->TermWin.mfont)
	XFreeFont (o->Xdisplay, o->TermWin.mfont);
#endif
    if (o->TermWin.font)
	XFreeFont (o->Xdisplay, o->TermWin.font);
#ifndef NO_BOLDFONT
    if (o->TermWin.boldFont)
	XFreeFont (o->Xdisplay, o->TermWin.boldFont);
#endif
#endif

    for (i = 0; i < TOTAL_RS; i++)
	myfree (o->rs_free[i]);

    myfree (o->cmdbuf_base);

    remotefs_free (o->remotefs);
    remotefs_free_terminalio (o->cterminal_io);
    myfree (o->cterminal_io);

    memset (o, 0, sizeof (*o));
}

