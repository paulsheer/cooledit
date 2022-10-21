/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/*--------------------------------*-C-*---------------------------------*
 * File:	rxvtlibtypedef.h
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

#ifndef rxvtlib_DEFINED
#define rxvtlib_DEFINED
typedef struct _rxvtlib rxvtlib;
#endif

#define UTF8_FONT
// #undef UTF8_FONT

#ifdef UTF8_FONT
typedef int rxvt_buf_char_t;
#else
typedef char rxvt_buf_char_t;
#endif

#ifndef _RXVT_H			/* include once only */
#ifndef HAVE_XPOINTER
typedef char   *XPointer;
#endif
typedef struct _menuBar_t menuBar_t;
typedef struct _TermWin_t TermWin_t;
typedef struct _scrollBar_t scrollBar_t;
typedef struct _grcmd_t grcmd_t;
typedef struct grwin_t grwin_t;
typedef struct _row_col_t row_col_t;
#if defined (NO_OLD_SELECTION) && defined(NO_NEW_SELECTION)
# error if you disable both selection styles, how can you select, silly?
#endif
#endif				/* _RXVT_H */
#if defined(UTF8_FONT)
#ifdef THREE_BYTE_CELLS
struct text_t_s_ {
    unsigned char s[3]; /* large enough for any unicode character, saves a bit of memory */
};
typedef struct text_t_s_ text_t;
#else
union text_t_s_ {
    uint32_t c32bit;
    unsigned char s[4]; /* large enough for any unicode character */
};
typedef union text_t_s_ text_t;
#endif
#else
typedef unsigned char text_t;
#endif
typedef struct _screen_t screen_t;
typedef struct save_t save_t;
#ifdef HAVE_TERMIOS_H
typedef struct termios ttymode_t;
#else
typedef struct _ttymode_t ttymode_t;
#endif				/* HAVE_TERMIOS_H */
typedef struct XCNQueue_t XCNQueue_t;
#ifdef UTMP_SUPPORT
# if ! defined(HAVE_STRUCT_UTMPX) && ! defined(HAVE_STRUCT_UTMP)
#  error cannot build with utmp support - no utmp or utmpx struct found
# endif
# ifdef WTMP_SUPPORT
#  ifdef RXVT_UTMP_AS_UTMPX
#   ifdef RXVT_WTMPX_FILE
#   else
#    error cannot build with wtmp support - no wtmpx file found
#   endif
#  else
#   ifdef RXVT_WTMP_FILE
#   else
#    error cannot build with wtmp support - no wtmp file found
#   endif
#  endif
# endif
#endif
typedef struct _action_t action_t;
typedef struct _submenu_t submenu_t;
typedef struct menuitem_t menuitem_t;
typedef struct menu_t menu_t;
typedef struct bar_t bar_t;
typedef CARD32  Atom32;
#ifndef NEXT_SCROLLBAR
#ifdef XTERM_SCROLLBAR		/* bitmap scrollbar */
#if (SB_WIDTH != 15)
#error Error, check scrollbar width (SB_WIDTH).It must be 15 for XTERM_SCROLLBAR
#endif
#else				/* XTERM_SCROLLBAR */
#endif				/* ! XTERM_SCROLLBAR */
#else				/* ! NEXT_SCROLLBAR */
#endif				/* ! NEXT_SCROLLBAR */
typedef struct _bgPixmap_t bgPixmap_t;
