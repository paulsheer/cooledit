/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/*--------------------------------*-C-*---------------------------------*
 * File:	rxvtlib.h
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

/* #define MENUBAR */

#include "_rxvtlib.h"
#include "rxvtlibtypedef.h"
#include "rxvtexport.h"
#include "cterminal.h"

/* #if (XtSpecificationRelease >= 6) */
/* #undef USE_XIM */
#define USE_XIM
/* #endif */

#ifdef NEXT_LOOK
#define NEXT_SCROLLBAR
#endif

#define EXTSCR 
#define EXTERN

/* modes for scr_page() - scroll page. used by scrollbar window */
enum {
    UP,
    DN,
    NO_DIR
};

/* arguments for scr_change_screen() */
enum {
    PRIMARY,
    SECONDARY
};


struct _rxvtlib {
 unsigned int num_fds ;

#ifdef UTF8_FONT
 const char *fontname;
 int charset_8bit;
 char utf8buf[6];
#endif
 int utf8buflen;
 int env_fg;
 int env_bg;


/*
 * File:	feature.h
 * $Id: feature.h,v 1.20.2.4 1999/08/17 07:02:45 mason Exp $
 *
 * Compile-time configuration.
 *-----------------------------------------------------------------------
 * Copyright (C) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
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
 *
 *----------------------------------------------------------------------*/

/*
 * Define to remove handling of extra escape sequences and other rarely
 * used features.
 */
/* #define NO_FRILLS */

/*-----------------------SCREEN OPTIONS AND COLOURS---------------------*/
/*
 * Define the name of the environment variable to be used in
 * addition to the "PATH" environment and the `path' resource.
 * Usually it should point to where you keep your background pixmaps and/or
 * your menu files
 */
#define PATH_ENV	"RXVTPATH"

/*
 * Avoid enabling the colour cursor (-cr, cursorColor, cursorColor2)
 */
/* #define NO_CURSORCOLOR */

/*
 * Suppress use of BOLD and BLINK attributes for setting bright foreground
 * and background, respectively.  Simulate BOLD using colorBD, boldFont or
 * overstrike characters.
 */
/* #define NO_BRIGHTCOLOR */

/*
 * Disable separate colours for bold/underline
 */
/* #define NO_BOLDUNDERLINE */

/*
 * Disable using simulated bold using overstrike.  You can also turn off
 * overstrike just for multi char fonts
 * Note: NO_BOLDOVERSTRIKE implies NO_BOLDOVERSTRIKE_MULTI
 */
/* #define NO_BOLDOVERSTRIKE */
/* #define NO_BOLDOVERSTRIKE_MULTI */

/*
 * Also use bold font or overstrike even if we use colour for bold
 */
#define VERYBOLD

/*
 * Compile without support for real bold fonts
 */
/* #define NO_BOLDFONT */

/*
 * If the screen has 24 bit mode, use that even if the default is 8 bit.
 */
#define PREFER_24BIT

/*
 * Define default colours for certain items.  If you have a low colour
 * display, then consider using colours which are already pre-allocated:
 *   Black		(#000000)
 *   Red3		(#CD0000)	+ these
 *   Green3		(#00CD00)	+ colours
 *   Yellow3		(#CDCD00)	+ are
 *   Blue3		(#0000CD)	+ not
 *   Magenta3		(#CD00CD)	+ pre-allocated
 *   Cyan3		(#00CDCD)	+ if
 *   AntiqueWhite	(#FAEBD7)	+ NO_BRIGHTCOLOR
 *   Grey25		(#404040)	+ defined
 *   Red		(#FF0000)
 *   Green		(#00FF00)
 *   Yellow		(#FFFF00)
 *   Blue		(#0000FF)
 *   Magenta		(#FF00FF)
 *   Cyan		(#00FFFF)
 *   White		(#FFFFFF)
 */
/* These colours MUST be defined */
#define COLOR_FOREGROUND	"Black"
#define COLOR_BACKGROUND	"White"
#define COLOR_SCROLLBAR		"#B2B2B2"	/* scrollColor match Netscape */
#define COLOR_SCROLLTROUGH	"#969696"
/*
 * The cursor colours are special.  Be very careful about setting these: 
 * foreground/background colours may be modified by command line or resources
 * prior to this allocation.  Also, they are not valid if NO_CURSORCOLOR is
 * defined
 */
#define COLOR_CURSOR_FOREGROUND	NULL	/* if NULL, use background colour */
#define COLOR_CURSOR_BACKGROUND	NULL	/* if NULL, use foreground colour */

/*
 * Use alternative code for screen-refreshes when compiled with xpm-support.
 * Seems to be obsolete due to the new screen-update routines.
 */
/* #define XPM_BUFFERING */

/*
 * Printer pipe which will be used for emulation of attached vt100 printer
 */
#if 0
#define PRINTPIPE	"lpr"
#endif

/*------------------------------RESOURCES-------------------------------*/
/*
 * Define where to find installed application defaults for rxvt
 * Only if USE_XGETDEFAULT is not defined.
 */
#ifndef XAPPLOADDIR
/* #define XAPPLOADDIR	"/usr/lib/X11/app-defaults" */
#endif

/*---------------------------------KEYS---------------------------------*/

/*
 * Define defaults for backspace and delete keys - unless they have been
 * configured out with --disable-backspace-key / --disable-delete-key
 */
/* #define DEFAULT_BACKSPACE	"DEC"		*//* SPECIAL */
/* #define DEFAULT_BACKSPACE	"\177"		*/
/* #define DEFAULT_DELETE	"\033[3~"	*/

/*
 * Choose one of these values to be the `hotkey' for changing font.
 * This has been superceded and is only for you older users
 */
/* #define HOTKEY_CTRL */
/* #define HOTKEY_META */

/*
 * To use
 *	Home = "\E[1~", End = "\E[4~"
 * instead of
 *	Home = "\E[7~", End = "\E[8~"	[default]
 */
/* #define LINUX_KEYS */

/*
 * Enable the keysym resource which allows you to define strings associated
 * with various KeySyms (0xFF00 - 0xFFFF).
 * Only works with the default hand-rolled resources.
 */
#ifndef NO_RESOURCES
# define KEYSYM_RESOURCE
#endif

/*
 * Modifier/s to use to allow up/down arrows and Priot/Next keys
 * to scroll single or page-fulls
 */
#define SCROLL_ON_SHIFT
/* #define SCROLL_ON_CTRL */
/* #define SCROLL_ON_META */

/*
 * Allow unshifted Next/Prior keys to scroll forward/back
 * (in addition to shift+Next/shift+Prior)       --pjh
 */
/* #define UNSHIFTED_SCROLLKEYS */

/* (Hops) Set to choose a number of lines of context between pages 
 *      (rather than a proportion (1/5) of savedlines buffer) 
 *      when paging the savedlines with SHIFT-{Prior,Next} keys.
 */
#define PAGING_CONTEXT_LINES 1	/* */

/*--------------------------------MOUSE---------------------------------*/
/*
 * Disable sending escape sequences (up, down, page up/down)
 * from the scrollbar when XTerm mouse reporting is enabled
 */
/* #define NO_SCROLLBAR_REPORT */

/*
 * Default separating chars for multiple-click selection
 * Space and tab are separate separating characters and are not settable
 */
#define CUTCHARS	"\"&'()*,;<=>?@[\\]^`{|}~"

/*
 * Add run-time support for changing the cutchars for double click selection
 */
#define CUTCHAR_RESOURCE

/*
 * Have mouse reporting include double-click info for button1
 */
/* #define MOUSE_REPORT_DOUBLECLICK */

/*
 * Set delay between multiple click events [default: 500]
 */
/* #define MULTICLICK_TIME 500 */

/*
 * If mouse wheel is defined, then scrolling is by 5 lines (or 1 line
 * if the shift key is down).  Scrolling can be smooth or jump scrolling
 */
/* #define JUMP_MOUSE_WHEEL */

/*
 * Set delay periods for continuous scrolling with scrollbar buttons
 */
/* #define SCROLLBAR_INITIAL_DELAY 40 */
/* #define SCROLLBAR_CONTINUOUS_DELAY 2 */

/*--------------------------------BELL----------------------------------*/
/*
 * Disable automatic de-iconify when a bell is received
 */
/* #define NO_MAPALERT */

/*
 * Have mapAlert behaviour selectable with mapAlert resource
 */
#define MAPALERT_OPTION

/*-----------------------------SCROLL BAR-------------------------------*/
/*
 * Choose the scrollbar width - should be an even number [default: 10]
 * Except for XTERM_SCROLLBAR: it is *always* 15
 * 	and for NEXT_SCROLLBAR, which is *always* 19
 */
/* #define SB_WIDTH 10 */

/* 
 * When using Rxvt scrollbar, clicking above or below the slider will move
 * 1/4 of the screen height, if possible.  Setting RXVT_SCROLL_FULL will move
 * it one screen height less one line, if possible
 */
#define RXVT_SCROLL_FULL 1

/* 
 * (Hops) draw an internal border line on inside edge of the scrollbar
 */
/* #define SB_BORDER */

/*
 * (Hops)  Uncomment to revert to original funky behaviour of
 * of having scroll thumb align on thumb top rather than ptr
 * position in thumb (or center of thumb).
 * Default Behavior becomes alignment to where grab thumb.
 * Only for non XTERM scrollbar
 */
/* #define FUNKY_SCROLL_BEHAVIOUR */

/*------------------------------MENU BAR--------------------------------*/
/*
 * Choose how many of (experimental) menuBars you want to be able to stack at
 * one time.
 *  A value of 1 disables menuBar stacking.
 *  A value of 0 disables menuBar all together.
 *  Note that the amount of memory overhead is the same for any value >= 2.
 */
#define MENUBAR_MAX 8

/*
 * Change the default shadow style
 */
/* #define MENUBAR_SHADOW_IN */

/*
 * Change the default shadow style
 */
#define MENU_SHADOW_IN

/*---------------------------MULTILINGUAL-------------------------------*/
/*
 * Allow run-time selection of Meta (Alt) to set the 8th bit on
 */
#define META8_OPTION

/*---------------------------DISPLAY OPTIONS----------------------------*/
/*
 * Calls to the local X server are handled quickly
 */
/* #define INEXPENSIVE_LOCAL_X_CALLS */

/*
 * Force local connection to be socket (or other local) communication
 */
/* #define LOCAL_X_IS_UNIX */

/*
 * Have DISPLAY environment variable & "\E[7n" transmit display with IP number
 */
/* #define DISPLAY_IS_IP */

/*
 * Have "\E[7n" transmit the display name.
 * This has been cited as a potential security hole.
 */
/* #define ENABLE_DISPLAY_ANSWER */

/* 
 * Change what ESC Z transmits instead of the default "\E[?1;2c"
 */
/* #define ESCZ_ANSWER	"\033[?1;2C" */

/*
 * Check the current value of the window-time/icon-name and avoid
 * re-setting it to the same value -- avoids unnecessary window refreshes
 */
#define SMART_WINDOW_TITLE

/*
 * Allow foreground/background colour to be changed with an
 * xterm escape sequence "\E]39;colour^G" -- still experimental
 */
#define XTERM_COLOR_CHANGE

/*
 * Width of the term border
 */
#define BORDERWIDTH	1

/*
 * Default number of lines in the scrollback buffer
 */
#define SAVELINES	64

/*
 * Provide support for pathetic applications which expect specifically
 * undefined "bw" (termcap/terminfo) behaviour to be specifically defined
 * as xterm defines it.
 */
#define SUPPORT_BROKEN_APPS_WHICH_RELY_ON_UNDEFINED_BW_BEHAVIOUR_AS_XTERM

/*
 * List of default fonts available
 * NFONTS is the number of fonts in the list
 * FONT0_IDX is the default font in the list (starting at 0)
 * Sizes between multi-char fonts sets (MFONT_LIST) and single-char font
 * sets (NFONT_LIST) have been matched up
 */
#ifndef MULTICHAR_SET		/* no Kanji or Big5 or GB support */
# define NFONTS		5
# define FONT0_IDX	2
# undef  MFONT_LIST
# define NFONT_LIST	"7x14", "6x10", "6x13", "8x13", "9x15"
#endif
#ifdef KANJI
# define NFONTS		5
# define FONT0_IDX	2
# define MFONT_LIST	"k14", "jiskan16", "jiskan18", "jiskan24", "jiskan26"
# define NFONT_LIST	"7x14", "8x16", "9x18", "12x24", "13x26"
#endif
#ifdef ZH
# define NFONTS		5
# define FONT0_IDX	1
# define MFONT_LIST	"taipei16", "taipeik20", "taipeik24", "taipeik20", \
       			"taipei16"
# define NFONT_LIST	"8x16", "10x20", "12x24", "10x20", "8x16"
#endif
#ifdef ZHCN			/* Here are our default GB fonts. */
# define NFONTS		3
# define FONT0_IDX	1
# define MFONT_LIST	"hanzigb16st", "hanzigb24st", "hanzigb16fs"
# define NFONT_LIST	"8x16", "12x24", "8x16"
#endif

/*
 * $Id: rxvt.h,v 1.40.2.6 1999/07/17 09:43:31 mason Exp $
 */

/* sort out conflicts in feature.h */
#undef  MULTICHAR_SET		/* a glyph is only ever defined by 1 char */
#ifdef KANJI
# define MULTICHAR_SET		/* a glyph is defined by 1 or 2 chars     */
# undef ZH			/* remove Chinese big5 support            */
# undef ZHCN			/* remove Chinese gb support              */
# undef GREEK_SUPPORT		/* Kanji/Greek together is too weird      */
# undef DEFINE_XTERM_COLOR	/* since kterm-color doesn't exist?       */
#endif
#ifdef ZH
# define MULTICHAR_SET		/* a glyph is defined by 1 or 2 chars     */
# undef KANJI			/* can't put Chinese/Kanji together       */
# undef ZHCN
# undef GREEK_SUPPORT
# undef DEFINE_XTERM_COLOR
#endif
#ifdef ZHCN
# define MULTICHAR_SET		/* a glyph is defined by 1 or 2 chars     */
# undef KANJI
# undef ZH
# undef GREEK_SUPPORT
#endif

/*
 *****************************************************************************
 * SYSTEM HACKS
 *****************************************************************************
 */
/* Consistent defines - please report on the necessity
 * @ Unixware: defines (__svr4__)
 */
#if defined (SVR4) && !defined (__svr4__)
# define __svr4__
#endif
#if defined (sun) && !defined (__sun__)
# define __sun__
#endif

/*
 * sun <sys/ioctl.h> isn't properly protected?
 * anyway, it causes problems when <termios.h> is also included
 */
#if defined (__sun__)
# undef HAVE_SYS_IOCTL_H
#endif

/*
 * Solaris defines SRIOCSREDIR in sys/strredir.h .
 * Needed for displaying console messages under solaris
 */

/*
 *****************************************************************************
 * INCLUDES
 *****************************************************************************
 */

#if defined (HAVE_SYS_IOCTL_H) && !defined (__sun__)
/* seems to cause problems when <termios.h> is also included on some suns */
#endif

/*
 *****************************************************************************
 * STRUCTURES AND TYPEDEFS
 *****************************************************************************
 */
/* Sanitize menubar info */
#ifndef MENUBAR
# undef MENUBAR_MAX
#endif
#ifndef MENUBAR_MAX
# define MENUBAR_MAX	0
#endif

struct _menuBar_t {
    short           state;
    Window          win;
} menuBar;

/* If we're using either the fancy scrollbar or menu bars, keep the
 * scrollColor resource.
 */
#if !defined(XTERM_SCROLLBAR) || defined(MENUBAR)
# define KEEP_SCROLLCOLOR 1
#else
# undef KEEP_SCROLLCOLOR
#endif

#ifdef TRANSPARENT
# define KNOW_PARENTS		4
#else
# define KNOW_PARENTS		1
#endif

struct _TermWin_t {
    short           width,	/* window width [pixels]                    */
                    height,	/* window height [pixels]                   */
                    fwidth,	/* font width [pixels]                      */
                    fheight,	/* font height [pixels]                     */
                    fprop,	/* font is proportional                     */
		    bprop,	/* treat bold font as proportional          */
		    mprop,	/* treat multichar font as proportional     */
                    ncol, nrow,	/* window size [characters]                 */
                    focus,	/* window has focus                         */
                    mapped,	/* window state mapped?                     */
                    saveLines;	/* number of lines that fit in scrollback   */
    unsigned short  nscrolled,	/* number of line actually scrolled         */
                    view_start;	/* scrollback view starts here              */
    Window          parent[4],	/* parent[0] is our window                 */
                    vt;		/* vt100 window                             */
    GC              gc;		/* GC for drawing text                      */
#ifndef UTF8_FONT
    XFontStruct    *font;	/* main font structure                      */
#ifndef NO_BOLDFONT
    XFontStruct    *boldFont;	/* bold font                                */
#endif
#ifdef MULTICHAR_SET
    XFontStruct    *mfont;	/* Multichar font structure                 */
#endif
    XFontSet        fontset;
#endif
#ifdef XPM_BACKGROUND
    Pixmap          pixmap;
#ifdef XPM_BUFFERING
    Pixmap          buf_pixmap;
#endif
#endif
} TermWin;

struct _scrollBar_t {
    short           beg, end;	/* beg/end of slider sub-window */
    short           top, bot;	/* top/bot of slider */
    short           state;	/* scrollbar state */
    Window          win;
} scrollBar;

#ifdef RXVT_GRAPHICS
int graphics_up;
struct grwin_t {
    Window win;
    int x, y;
    unsigned int w, h;
    short screen;
    struct _grcmd_t {
	char cmd;
	short color;
	short ncoords;
	int *coords;
	unsigned char *text;
	struct grcmd_t *next;
    } *graphics;
    struct grwin_t *prev, *next;
} *gr_root;
#endif

struct _row_col_t {
    short         row, col;
} oldcursor;

#ifndef min
# define min(a,b)	(((a) < (b)) ? (a) : (b))
# define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#define MAX_IT(current, other)	if ((other) > (current)) (current) = (other)
#define MIN_IT(current, other)	if ((other) < (current)) (current) = (other)
#define SWAP_IT(one, two, tmp)				\
    do {						\
	(tmp) = (one); (one) = (two); (two) = (tmp);	\
    } while (0)

/*
 *****************************************************************************
 * NORMAL DEFINES
 *****************************************************************************
 */

#if defined (NO_OLD_SELECTION) && defined(NO_NEW_SELECTION)
# error if you disable both selection styles, how can you select, silly?
#endif

#ifndef XPM_BACKGROUND
# undef XPM_BUFFERING		/* disable what can't be used */
#endif

#define APL_CLASS	"XTerm"	/* class name */
#define APL_SUBCLASS	"Rxvt"	/* also check resources under this name */
#define APL_NAME	"rxvt"	/* normal name */

/* COLORTERM, TERM environment variables */
#define COLORTERMENV	"rxvt"
#ifdef XPM_BACKGROUND
# define COLORTERMENVFULL COLORTERMENV "-xpm"
#else
# define COLORTERMENVFULL COLORTERMENV
#endif
#ifndef TERMENV
# ifdef KANJI
#  define TERMENV	"kterm"
# else
#  define TERMENV	"rxvt"
# endif
#endif

#if defined (NO_MOUSE_REPORT) && !defined (NO_MOUSE_REPORT_SCROLLBAR)
# define NO_MOUSE_REPORT_SCROLLBAR
#endif

#ifdef NO_RESOURCES
# undef USE_XGETDEFAULT
#endif

/* now look for other badly set stuff */

#if !defined (EACCESS) && defined(EAGAIN)
# define EACCESS EAGAIN
#endif

#define DO_EXIT ((int) 1 << 30)
#ifndef EXIT_SUCCESS		/* missing from <stdlib.h> */
# define EXIT_SUCCESS		0	/* exit function success */
# define EXIT_FAILURE		1	/* exit function failure */
#endif

#define menuBar_esc		10
#define scrollBar_esc		30
#define menuBar_margin		2	/* margin below text */

/* gap between text and window edges (could be configurable) */
#define TermWin_internalBorder	2

/* width of scrollBar, menuBar shadow, must be 1 or 2 */
#ifdef HALFSHADOW
# define SHADOW 1
#else
# define SHADOW 2
#endif

#ifndef STANDALONE
# undef SHADOW
# define SHADOW 2
#endif

#ifdef NEXT_SCROLLBAR
# undef SB_WIDTH
# define SB_WIDTH		19
# define SB_PADDING		1
# define SB_BORDER_WIDTH	1
# define SB_BEVEL_WIDTH_UPPER_LEFT	1
# define SB_BEVEL_WIDTH_LOWER_RIGHT	2
# define SB_LEFT_PADDING	(SB_PADDING + SB_BORDER_WIDTH)
# define SB_MARGIN_SPACE	(SB_PADDING * 2)
# define SB_BUTTON_WIDTH	(SB_WIDTH - SB_MARGIN_SPACE - SB_BORDER_WIDTH)
# define SB_BUTTON_HEIGHT	(SB_BUTTON_WIDTH)
# define SB_BUTTON_SINGLE_HEIGHT	(SB_BUTTON_HEIGHT + SB_PADDING)
# define SB_BUTTON_BOTH_HEIGHT		(SB_BUTTON_SINGLE_HEIGHT * 2)
# define SB_BUTTON_TOTAL_HEIGHT		(SB_BUTTON_BOTH_HEIGHT + SB_PADDING)
# define SB_BUTTON_BEVEL_X	(SB_LEFT_PADDING)
# define SB_BUTTON_FACE_X	(SB_BUTTON_BEVEL_X + SB_BEVEL_WIDTH_UPPER_LEFT)
# define SB_THUMB_MIN_HEIGHT	(SB_BUTTON_WIDTH - (SB_PADDING * 2))
 /*
  *    +-------------+
  *    |             | <---< SB_PADDING
  *    | ::::::::::: |
  *    | ::::::::::: |
  *   '''''''''''''''''
  *   ,,,,,,,,,,,,,,,,,
  *    | ::::::::::: |
  *    | ::::::::::: |
  *    |  +---------------< SB_BEVEL_WIDTH_UPPER_LEFT
  *    |  | :::::::: |
  *    |  V :::: vv-------< SB_BEVEL_WIDTH_LOWER_RIGHT
  *    | +---------+ |
  *    | | ......%%| |
  *    | | ......%%| |
  *    | | ..()..%%| |
  *    | | ......%%| |
  *    | | %%%%%%%%| |
  *    | +---------+ | <.........................
  *    |             | <---< SB_PADDING         :
  *    | +---------+ | <-+..........            :---< SB_BUTTON_TOTAL_HEIGHT
  *    | | ......%%| |   |         :            :
  *    | | ../\..%%| |   |---< SB_BUTTON_HEIGHT :
  *    | | %%%%%%%%| |   |         :            :
  *    | +---------+ | <-+         :            :
  *    |             |             :            :
  *    | +---------+ | <-+         :---< SB_BUTTON_BOTH_HEIGHT
  *    | | ......%%| |   |         :            :
  *    | | ..\/..%%| |   |         :            :
  *    | | %%%%%%%%| |   |---< SB_BUTTON_SINGLE_HEIGHT
  *    | +---------+ |   |         :            :
  *    |             |   |         :            :
  *    +-------------+ <-+.........:............:
  *    ^^|_________| :
  *    ||     |      :
  *    ||     +---< SB_BUTTON_WIDTH
  *    ||            :
  *    |+------< SB_PADDING
  *    |:            :
  *    +----< SB_BORDER_WIDTH
  *     :            :
  *     :............:
  *           |
  *           +---< SB_WIDTH
  */
#else
# ifdef XTERM_SCROLLBAR
#  undef  SB_WIDTH
#  define SB_WIDTH		15
# else
#  if !defined (SB_WIDTH) || (SB_WIDTH < 8)
#   undef SB_WIDTH
#   define SB_WIDTH		11	/* scrollBar width */
#  endif
# endif				/* XTERM_SCROLLBAR */
#endif

#define NO_REFRESH		0	/* Window not visible at all!        */
#define FAST_REFRESH		(1<<1)	/* Fully exposed window              */
#define SLOW_REFRESH		(1<<2)	/* Partially exposed window          */
#define SMOOTH_REFRESH		(1<<3)	/* Do sync'ing to make it smooth     */

#ifdef NO_SECONDARY_SCREEN
# define NSCREENS		0
#else
# define NSCREENS		1
#endif

#define SAVE			's'
#define RESTORE			'r'

/* special (internal) prefix for font commands */
#define FONT_CMD		'#'
#define FONT_DN			"#-"
#define FONT_UP			"#+"

/* flags for scr_gotorc() */
#define C_RELATIVE		1	/* col movement is relative */
#define R_RELATIVE		2	/* row movement is relative */
#define RELATIVE		(R_RELATIVE|C_RELATIVE)

/* modes for scr_insdel_chars(), scr_insdel_lines() */
#define INSERT			-1	/* don't change these values */
#define DELETE			+1
#define ERASE			+2

/* all basic bit-flags in first/lower 16 bits */

#define RS_None			0	/* Normal */
#define RS_fgMask		0x0000001Fu	/* 32 colors */
#define RS_bgMask		0x000003E0u	/* 32 colors */
#define RS_Bold			0x00000400u	/* bold */
#define RS_Blink		0x00000800u	/* blink */
#define RS_RVid			0x00001000u	/* reverse video */
#define RS_Uline		0x00002000u	/* underline */
#define RS_acsFont		0x00004000u	/* ACS graphics char set */
#define RS_ukFont		0x00008000u	/* UK character set */
#define RS_fontMask		(RS_acsFont|RS_ukFont)
#define RS_baseattrMask		(RS_Bold|RS_Blink|RS_RVid|RS_Uline)

/* all other bit-flags in upper 16 bits */

#ifdef MULTICHAR_SET
# define RS_multi0		0x10000000u	/* only multibyte characters */
# define RS_multi1		0x20000000u	/* multibyte 1st byte */
# define RS_multi2		(RS_multi0|RS_multi1)	/* multibyte 2nd byte */
# define RS_multiMask		(RS_multi0|RS_multi1)	/* multibyte mask */
#else
# define RS_multiMask		0
#endif

#define RS_attrMask		(RS_baseattrMask|RS_fontMask|RS_multiMask)

#define	Opt_console		(1LU<<0)
#define Opt_loginShell		(1LU<<1)
#define Opt_iconic		(1LU<<2)
#define Opt_visualBell		(1LU<<3)
#define Opt_mapAlert		(1LU<<4)
#define Opt_reverseVideo	(1LU<<5)
#define Opt_utmpInhibit		(1LU<<6)
#define Opt_scrollBar		(1LU<<7)
#define Opt_scrollBar_right	(1LU<<8)
#define Opt_scrollBar_floating	(1LU<<9)
#define Opt_meta8		(1LU<<10)
#define Opt_scrollTtyOutput	(1LU<<11)
#define Opt_scrollKeypress	(1LU<<12)
#define Opt_transparent		(1LU<<13)
/* place holder used for parsing command-line options */
#define Opt_Reverse		(1LU<<30)
#define Opt_Boolean		(1LU<<31)

/*
 * XTerm escape sequences: ESC ] Ps;Pt BEL
 */
#define XTerm_name		0
#define XTerm_iconName		1
#define XTerm_title		2
#define XTerm_logfile		46	/* not implemented */
#define XTerm_font		50

/*
 * rxvt extensions of XTerm escape sequences: ESC ] Ps;Pt BEL
 */
#define XTerm_Menu		10	/* set menu item */
#define XTerm_Pixmap		20	/* new bg pixmap */
#define XTerm_restoreFG		39	/* change default fg color */
#define XTerm_restoreBG		49	/* change default bg color */

#define restoreFG		39	/* restore default fg color */
#define restoreBG		49	/* restore default bg color */

/* Words starting with `Color_' are colours.  Others are counts */

enum colour_list {
    Color_fg = 0,
    Color_bg,
    minCOLOR,			/* 2 */
    Color_Black = minCOLOR,
    Color_Red3,
    Color_Green3,
    Color_Yellow3,
    Color_Blue3,
    Color_Magenta3,
    Color_Cyan3,
    maxCOLOR,			/* minCOLOR + 7 */
#ifndef NO_BRIGHTCOLOR
    Color_AntiqueWhite = maxCOLOR,
    minBrightCOLOR,		/* maxCOLOR + 1 */
    Color_Grey25 = minBrightCOLOR,
    Color_Red,
    Color_Green,
    Color_Yellow,
    Color_Blue,
    Color_Magenta,
    Color_Cyan,
    maxBrightCOLOR,		/* minBrightCOLOR + 7 */
    Color_White = maxBrightCOLOR,
#else
    Color_White = maxCOLOR,
#endif
#ifndef NO_CURSORCOLOR
    Color_cursor,
    Color_cursor2,
#endif
    Color_pointer,
    Color_border,
#ifndef NO_BOLDUNDERLINE
    Color_BD,
    Color_UL,
#endif
#ifdef KEEP_SCROLLCOLOR
    Color_scroll,
    Color_trough,
#endif
    NRS_COLORS,			/* */
#ifdef KEEP_SCROLLCOLOR
    Color_topShadow = NRS_COLORS,
    Color_bottomShadow,
    TOTAL_COLORS		/* upto 28 */
#else
    TOTAL_COLORS = NRS_COLORS	/* */
#endif
} dummy_var;

#define DEFAULT_RSTYLE		(RS_None | (Color_fg) | (Color_bg<<5))

/*
 * This resource list should match xdefaults.c
 * - though not necessarily in order
 */
enum Rs_resource_list {
    Rs_display_name = 0,
    Rs_term_name,
    Rs_geometry,
    Rs_reverseVideo,
    Rs_color,
    Rs_font = Rs_color + NRS_COLORS,
    Rs_iconName = Rs_font + NFONTS,
#ifdef MULTICHAR_SET
    Rs_mfont,
    Rs_multichar_encoding = Rs_mfont + NFONTS,
#endif
    Rs_name,
    Rs_title,
#if defined (XPM_BACKGROUND) || (MENUBAR_MAX)
    Rs_path,
#endif
#ifdef XPM_BACKGROUND
    Rs_backgroundPixmap,
#endif
#if (MENUBAR_MAX)
    Rs_menu,
#endif
#ifndef NO_BOLDFONT
    Rs_boldFont,
#endif
#ifdef GREEK_SUPPORT
    Rs_greek_keyboard,
#endif
    Rs_loginShell,
    Rs_scrollBar,
    Rs_scrollBar_right,
    Rs_scrollBar_floating,
    Rs_scrollTtyOutput,
    Rs_scrollKeypress,
    Rs_saveLines,
    Rs_utmpInhibit,
    Rs_visualBell,
#if ! defined(NO_MAPALERT) && defined(MAPALERT_OPTION)
    Rs_mapAlert,
#endif
#ifdef META8_OPTION
    Rs_meta8,
#endif
#ifndef NO_BACKSPACE_KEY
    Rs_backspace_key,
#endif
#ifndef NO_DELETE_KEY
    Rs_delete_key,
#endif
    Rs_selectstyle,
#ifdef PRINTPIPE
    Rs_print_pipe,
#endif
#ifdef USE_XIM
    Rs_preeditType,
    Rs_inputMethod,
#endif
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
    Rs_bigfont_key,
    Rs_smallfont_key,
#endif
#ifdef TRANSPARENT
    Rs_transparent,
#endif
    Rs_cutchars,
    Rs_modifier,
    TOTAL_RS
} dummy_var2;

/*
 * number of graphics points
 * divisible by 2 (num lines)
 * divisible by 4 (num rect)
 */
#define	NGRX_PTS	1000

/*
 *****************************************************************************
 * MACRO DEFINES
 *****************************************************************************
 */
#define STRCPY(a, b)		strcpy ((char *)(a), (const char *) (b))
#define STRNCPY(a, b, c)	strncpy ((char *)(a), (const char *) (b), (c))
# define MEMSET(a, b, c)	memset ((a), (b), (c))

#define MALLOC(sz)		malloc (sz)
#define CALLOC(type, n)		(type *) calloc ((n), sizeof(type))
#define REALLOC(mem, sz)	((mem) ? realloc ((mem), (sz)) : malloc(sz))
#define FREE(ptr)		free (ptr)

/* convert pixel dimensions to row/column values */
#define Pixel2Col(x)		Pixel2Width((x) - TermWin_internalBorder)
#define Pixel2Row(y)		Pixel2Height((y) - TermWin_internalBorder)
#define Pixel2Width(x)		((x) / o->TermWin.fwidth)
#define Pixel2Height(y)		((y) / o->TermWin.fheight)
#define Col2Pixel(col)		(Width2Pixel(col) + TermWin_internalBorder)
#define Row2Pixel(row)		(Height2Pixel(row) + TermWin_internalBorder)
#define Width2Pixel(n)		((n) * o->TermWin.fwidth)
#define Height2Pixel(n)		((n) * o->TermWin.fheight)

#define TermWin_TotalWidth()	(o->TermWin.width  + 2 * TermWin_internalBorder)
#define TermWin_TotalHeight()	(o->TermWin.height + 2 * TermWin_internalBorder)

#define Xscreen			DefaultScreen(o->Xdisplay)
#define Xroot			DefaultRootWindow(o->Xdisplay)

/* how to build & extract colors and attributes */
#define GET_FGCOLOR(r)		(((r) & RS_fgMask))
#define GET_BGCOLOR(r)		(((r) & RS_bgMask)>>5)
#define GET_ATTR(r)		(((r) & RS_attrMask))
#define GET_BGATTR(r)							\
    (((r) & RS_RVid) ? (((r) & (RS_attrMask & ~RS_RVid))		\
			| (((r) & RS_fgMask)<<5))			\
		     : ((r) & (RS_attrMask | RS_bgMask)))
#define SET_FGCOLOR(r,fg)	(((r) & ~RS_fgMask)  | (fg))
#define SET_BGCOLOR(r,bg)	(((r) & ~RS_bgMask)  | ((bg)<<5))
#define SET_ATTR(r,a)		(((r) & ~RS_attrMask)| (a))

#define scrollbar_visible()	(o->scrollBar.state)
#define scrollbar_isMotion()	(o->scrollBar.state == 'm')
#define scrollbar_isUp()	(o->scrollBar.state == 'U')
#define scrollbar_isDn()	(o->scrollBar.state == 'D')
#define scrollbar_isUpDn()	isupper (o->scrollBar.state)
#define isScrollbarWindow(w)	(scrollbar_visible() && (w) == o->scrollBar.win)

#define scrollbar_setNone()	o->scrollBar.state = 1
#define scrollbar_setMotion()	o->scrollBar.state = 'm'
#define scrollbar_setUp()	o->scrollBar.state = 'U'
#define scrollbar_setDn()	o->scrollBar.state = 'D'

#ifdef NEXT_SCROLLBAR
# define scrollbar_dnval()	(o->scrollBar.end + (SB_WIDTH + 1))
# define scrollbar_upButton(y)	((y) > o->scrollBar.end \
				 && (y) <= scrollbar_dnval())
# define scrollbar_dnButton(y)	((y) > scrollbar_dnval())
# define SCROLL_MINHEIGHT	SB_THUMB_MIN_HEIGHT
#else
# define scrollbar_upButton(y)	((y) < o->scrollBar.beg)
# define scrollbar_dnButton(y)	((y) > o->scrollBar.end)
# define SCROLL_MINHEIGHT	10
#endif

#define scrollbar_above_slider(y)	((y) < o->scrollBar.top)
#define scrollbar_below_slider(y)	((y) > o->scrollBar.bot)
#define scrollbar_position(y)		((y) - o->scrollBar.beg)
#define scrollbar_size()		(o->scrollBar.end - o->scrollBar.beg - SCROLL_MINHEIGHT)

#if (MENUBAR_MAX > 1)
/* rendition style flags */
# define menubar_visible()	(o->menuBar.state)
# define menuBar_height()	(o->TermWin.fheight + SHADOW)
# define menuBar_TotalHeight()	(menuBar_height() + SHADOW + menuBar_margin)
# define isMenuBarWindow(w)	((w) == o->menuBar.win)
#else
# define isMenuBarWindow(w)	(0)
# define menuBar_height()	(0)
# define menuBar_TotalHeight()	(0)
# define menubar_visible()	(0)
#endif

#ifdef XPM_BACKGROUND
# define XPMClearArea(a, b, c, d, e, f, g)	XClearArea((a), (b), (c), (d), (e), (f), (g))
#else
# define XPMClearArea(a, b, c, d, e, f, g)
#endif

#define Gr_ButtonPress(x,y)	rxvtlib_Gr_ButtonReport (o, 'P',(x),(y))
#define Gr_ButtonRelease(x,y)	rxvtlib_Gr_ButtonReport (o, 'R',(x),(y))
/*
 *****************************************************************************
 * VARIABLES
 *****************************************************************************
 */
#ifdef INTERN
# define EXTERN
#else
/* change `extern' to `' */
# define EXTERN
#endif

#ifdef PREFER_24BIT
EXTERN Colormap Xcmap;
EXTERN int      Xdepth;
EXTERN Visual  *Xvisual;
#else
# define Xcmap			DefaultColormap(Xdisplay,Xscreen)
# define Xdepth			DefaultDepth(Xdisplay,Xscreen)
# define Xvisual		DefaultVisual(Xdisplay,Xscreen)
# ifdef DEBUG_DEPTH
#  undef Xdepth
#  define Xdepth		DEBUG_DEPTH
# endif
#endif

EXTERN Display *Xdisplay;
EXTERN unsigned long Options;
EXTERN XSizeHints szHint;
EXTERN int      sb_shadow;
EXTERN unsigned long PixColors[TOTAL_COLORS];

#ifdef INEXPENSIVE_LOCAL_X_CALLS
EXTERN int      display_is_local;
#endif
EXTERN short    want_refresh;

EXTERN char *rs[TOTAL_RS];
EXTERN char *rs_free[TOTAL_RS];

#ifndef NO_BACKSPACE_KEY
EXTERN const char *key_backspace;
#endif
#ifndef NO_DELETE_KEY
EXTERN const char *key_delete;
#endif
#ifndef NO_BRIGHTCOLOR
EXTERN unsigned int colorfgbg;
#endif
#ifdef KEYSYM_RESOURCE
EXTERN const unsigned char *KeySym_map[256];
#endif
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
EXTERN KeySym   ks_bigfont;
EXTERN KeySym   ks_smallfont;
#endif

/*
 *****************************************************************************
 * PROTOTYPES
 *****************************************************************************
 */

#ifdef PROTOTYPES
# define __PROTO(p)	p
#else
# define __PROTO(p)	()
#endif

/*
 * If we haven't pulled in typedef's like short , then do them ourself
 */

/* type of (normal and unsigned) basic sizes */
/* e.g. typedef short short */

/* e.g. typedef unsigned short u_short */

/* e.g. typedef int int32_t */

/* e.g. typedef unsigned int u_int32_t */

/* e.g. typedef long int64_t */

/* e.g. typedef unsigned long u_int64_t */

/* whatever normal size corresponds to a integer pointer */
#define intp_t unsigned int
/* whatever normal size corresponds to a unsigned integer pointer */
#define u_intp_t unsigned int
#ifdef STANDALONE
#undef VERSION
#define VERSION "2.6.2"
#endif
#define DATE	"17 AUGUST 1999"
#define LSMDATE	"17AUG99"
/* Include prototypes for all files */
/*
 * $Id: protos.h,v 1.7 1998/10/24 10:22:45 mason Exp $
 */
/*--------------------------------*-C-*---------------------------------*
 * File:	rxvtgrx.h
 * $Id: rxvtgrx.h,v 1.2 1998/04/20 07:27:05 mason Exp $
 *
 * Stuff for text alignment for rxvt special graphics mode
 *
 * alignment
 * Top:
 *	text is placed so that the specified point is at the top of the
 *	capital letters
 * Center:
 *	text is placed so that the specified point is equidistant from the
 *	bottom of descenders and the top of the capital letters
 * Bottom:
 *	text is placed so that the bottom of descenders is on the specified
 *	point
 * Base:
 *	text is placed so that the bottom of the characters with no descenders
 *	is on the specified point
 * Caps_Center:
 *	text is placed so that the specified point is equidistant from the
 *	bottom and tops of capital letters
 *----------------------------------------------------------------------*/
#ifndef _RXVTGRX_H
#define _RXVTGRX_H

#define GRX_SCALE		10000

#define RIGHT_TEXT		0x10
#define HCENTER_TEXT		0x20
#define LEFT_TEXT		0x30
#define HORIZONTAL_ALIGNMENT	0x70

#define TOP_TEXT		0x01
#define VCENTER_TEXT		0x02
#define BOTTOM_TEXT		0x03
#define BASE_TEXT		0x04
#define VCAPS_CENTER_TEXT	0x05
#define VERTICAL_ALIGNMENT	0x0F

#if 0				/* this would be nicer */
# define TXT_RIGHT		'r'
# define TXT_CENTER		'c'
# define TXT_LEFT		'l'

# define TXT_TOP		't'
# define TXT_VCENTER		'v'
# define TXT_BOTTOM		'b'
# define TXT_BASE		'_'
# define TXT_VCAPS_CENTER	'C'
#endif

#endif				/* whole file */
/*----------------------- end-of-file (C header) -----------------------*/
/*
 * $Id: screen.h,v 1.8 1998/11/25 16:34:13 mason Exp $
 */

#ifndef _SCREEN_H		/* include once only */
#define _SCREEN_H

#ifdef UTF8_FONT
#define rend_t		unsigned int
#else
#if defined(MULTICHAR_SET)
#define rend_t		unsigned int
#else
#define rend_t		unsigned short
#endif
#endif

/*
 * screen accounting:
 * screen_t elements
 *   text:      Contains all text information including the scrollback buffer.
 *              Each line is length TermWin.ncol
 *   tlen:      The length of the line or -1 for wrapped lines.
 *   rend:      Contains rendition information: font, bold, colour, etc.
 * * Note: Each line for both text and rend are only allocated on demand, and
 *         text[x] is allocated <=> rend[x] is allocated  for all x.
 *   row:       Cursor row position                   : 0 <= row < TermWin.nrow
 *   col:       Cursor column position                : 0 <= col < TermWin.ncol
 *   tscroll:   Scrolling region top row inclusive    : 0 <= row < TermWin.nrow
 *   bscroll:   Scrolling region bottom row inclusive : 0 <= row < TermWin.nrow
 *
 * selection_t elements
 *   clicks:    1, 2 or 3 clicks - 4 indicates a special condition of 1 where
 *              nothing is selected
 *   beg:       row/column of beginning of selection  : never past mark
 *   mark:      row/column of initial click           : never past end
 *   end:       row/column of one character past end of selection
 * * Note: -TermWin.nscrolled <= beg.row <= mark.row <= end.row < TermWin.nrow
 * * Note: col == -1 ==> we're left of screen
 *
 * TermWin.saveLines:
 *              Maximum number of lines in the scrollback buffer.
 *              This is fixed for each rxvt instance.
 * TermWin.nscrolled:
 *              Actual number of lines we've used of the scrollback buffer
 *              0 <= TermWin.nscrolled <= TermWin.saveLines
 * TermWin.view_start:  
 *              Offset back into the scrollback buffer for out current view
 *              0 <= TermWin.view_start <= TermWin.nscrolled
 *
 * Layout of text/rend information in the screen_t text/rend structures:
 *   Rows [0] ... [TermWin.saveLines - 1]
 *     scrollback region : we're only here if TermWin.view_start != 0
 *   Rows [TermWin.saveLines] ... [TermWin.saveLines + TermWin.nrow - 1]
 *     normal `unscrolled' screen region
 */

struct _screen_t {
    text_t        **text;	/* _all_ the text                            */
    short        *tlen;	/* length of each text line                  */
    rend_t        **rend;	/* rendition, uses RS_ flags                 */
    row_col_t       cur;	/* cursor position on the screen             */
    short         tscroll,	/* top of settable scroll region             */
                    bscroll,	/* bottom of settable scroll region          */
                    charset;	/* character set number [0..3]               */
    unsigned int    flags;	/* see below                                 */
} screen;

struct save_t {
    row_col_t       cur;	/* cursor position                           */
    short         charset;	/* character set number [0..3]               */
    char            charset_char;
    rend_t          rstyle;	/* rendition style                           */
} save;

/* this must be the same as struct selection from edit.h */
struct edit_selection {
   unsigned char * text;
   int len;
} edit_selection;

struct _selection_t {
    unsigned char  *text;	/* selected text                             */
    int             len;	/* length of selected text                   */
    enum {
	SELECTION_CLEAR = 0,	/* nothing selected                          */
	SELECTION_INIT,		/* marked a point                            */
	SELECTION_BEGIN,	/* started a selection                       */
	SELECTION_CONT,		/* continued selection                       */
	SELECTION_DONE		/* selection put in CUT_BUFFER0              */
    } op;			/* current operation                         */
    short           screen;	/* screen being used                         */
    short           clicks;	/* number of clicks                          */
    row_col_t       beg, mark, end;
} selection;

/* ------------------------------------------------------------------------- */

/* screen_t flags */
#define Screen_Relative		(1<<0)	/* relative origin mode flag         */
#define Screen_VisibleCursor	(1<<1)	/* cursor visible?                   */
#define Screen_Autowrap		(1<<2)	/* auto-wrap flag                    */
#define Screen_Insert		(1<<3)	/* insert mode (vs. overstrike)      */
#define Screen_WrapNext		(1<<4)	/* need to wrap for next char?       */
#define Screen_DefaultFlags	(Screen_VisibleCursor|Screen_Autowrap)

/* ------------------------------------------------------------------------- *
 *                             MODULE VARIABLES                              *
 * ------------------------------------------------------------------------- */

#ifdef INTERN_SCREEN
# define EXTSCR
#else
/* changed `extern' to `' */
# define EXTSCR
#endif

/* This tells what's actually on the screen */
EXTSCR text_t **drawn_text;
EXTSCR rend_t **drawn_rend;
EXTSCR text_t **buf_text;
EXTSCR rend_t **buf_rend;
EXTSCR short *buf_tlen;
EXTSCR char    *tabs;		/* a 1 for a location with a tab-stop */
EXTSCR screen_t swap;
EXTSCR int selection_style;

#endif				/* repeat inclusion protection */

/*----------------------------------------------------------------------*
 * system default characters if defined and reasonable
 */
#ifndef CINTR
# define CINTR		'\003'	/* ^C */
#endif
#ifndef CQUIT
# define CQUIT		'\034'	/* ^\ */
#endif
#ifndef CERASE
# ifdef linux
#  define CERASE	'\177'	/* ^? */
# else
#  define CERASE	'\010'	/* ^H */
# endif
#endif
#ifndef CKILL
# define CKILL		'\025'	/* ^U */
#endif
#ifndef CEOF
# define CEOF		'\004'	/* ^D */
#endif
#ifndef CSTART
# define CSTART		'\021'	/* ^Q */
#endif
#ifndef CSTOP
# define CSTOP		'\023'	/* ^S */
#endif
#ifndef CSUSP
# define CSUSP		'\032'	/* ^Z */
#endif
#ifndef CDSUSP
# define CDSUSP		'\031'	/* ^Y */
#endif
#ifndef CRPRNT
# define CRPRNT		'\022'	/* ^R */
#endif
#ifndef CFLUSH
# define CFLUSH		'\017'	/* ^O */
#endif
#ifndef CWERASE
# define CWERASE	'\027'	/* ^W */
#endif
#ifndef CLNEXT
# define CLNEXT		'\026'	/* ^V */
#endif

#ifndef VDISCRD
# ifdef VDISCARD
#  define VDISCRD	VDISCARD
# endif
#endif

#ifndef VWERSE
# ifdef VWERASE
#  define VWERSE	VWERASE
# endif
#endif

#define KBUFSZ		512	/* size of keyboard mapping buffer */
#define STRING_MAX	512	/* max string size for process_xterm_seq() */
#define ESC_ARGS	32	/* max # of args for esc sequences */

/* a large REFRESH_PERIOD causes problems with `cat' */
#define REFRESH_PERIOD		1

#ifndef MULTICLICK_TIME
# define MULTICLICK_TIME	500
#endif
#ifndef SCROLLBAR_INITIAL_DELAY
# ifdef NEXT_SCROLLER
#  define SCROLLBAR_INITIAL_DELAY	20
# else
#  define SCROLLBAR_INITIAL_DELAY	40
# endif
#endif
#ifndef SCROLLBAR_CONTINUOUS_DELAY
# define SCROLLBAR_CONTINUOUS_DELAY	2
#endif

/* time factor to slow down a `jumpy' mouse */
#define MOUSE_THRESHOLD		50
#define CONSOLE		"/dev/console"	/* console device */

/*
 * key-strings: if only these keys were standardized <sigh>
 */
#ifdef LINUX_KEYS
# define KS_HOME	"\033[1~"	/* Home == Find */
# define KS_END		"\033[4~"	/* End == Select */
#else
# define KS_HOME	"\033[7~"	/* Home */
# define KS_END		"\033[8~"	/* End */
#endif

#ifdef SCROLL_ON_SHIFT
# define SCROLL_SHIFTKEY (shft)
#else
# define SCROLL_SHIFTKEY 0
#endif
#ifdef SCROLL_ON_CTRL
# define SCROLL_CTRLKEY  (ctrl)
#else
# define SCROLL_CTRLKEY 0
#endif
#ifdef SCROLL_ON_META
# define SCROLL_METAKEY  (meta)
#else
# define SCROLL_METAKEY 0
#endif
#define IS_SCROLL_MOD  (SCROLL_SHIFTKEY || SCROLL_CTRLKEY || SCROLL_METAKEY)

struct XCNQueue_t {
    struct XCNQueue_t *next;
    short         width, height;
} *XCNQueue;

/*
 * ESC-Z processing:
 *
 * By stealing a sequence to which other xterms respond, and sending the
 * same number of characters, but having a distinguishable sequence,
 * we can avoid having a timeout (when not under an rxvt) for every login
 * shell to auto-set its DISPLAY.
 *
 * This particular sequence is even explicitly stated as obsolete since
 * about 1985, so only very old software is likely to be confused, a
 * confusion which can likely be remedied through termcap or TERM. Frankly,
 * I doubt anyone will even notice.  We provide a #ifdef just in case they
 * don't care about auto-display setting.  Just in case the ancient
 * software in question is broken enough to be case insensitive to the 'c'
 * character in the answerback string, we make the distinguishing
 * characteristic be capitalization of that character. The length of the
 * two strings should be the same so that identical read(2) calls may be
 * used.
 */
#define VT100_ANS	"\033[?1;2c"	/* vt100 answerback */
#ifndef ESCZ_ANSWER
# define ESCZ_ANSWER	VT100_ANS	/* obsolete ANSI ESC[c */
#endif

/* DEC private modes */
#define PrivMode_132		(1LU<<0)
#define PrivMode_132OK		(1LU<<1)
#define PrivMode_rVideo		(1LU<<2)
#define PrivMode_relOrigin	(1LU<<3)
#define PrivMode_Screen		(1LU<<4)
#define PrivMode_Autowrap	(1LU<<5)
#define PrivMode_aplCUR		(1LU<<6)
#define PrivMode_aplKP		(1LU<<7)
#define PrivMode_HaveBackSpace	(1LU<<8)
#define PrivMode_BackSpace	(1LU<<9)
#define PrivMode_ShiftKeys	(1LU<<10)
#define PrivMode_VisibleCursor	(1LU<<11)
#define PrivMode_MouseX10	(1LU<<12)
#define PrivMode_MouseX11	(1LU<<13)
#define PrivMode_scrollBar	(1LU<<14)
#define PrivMode_menuBar	(1LU<<15)
#define PrivMode_TtyOutputInh	(1LU<<16)
#define PrivMode_Keypress	(1LU<<17)
/* too annoying to implement X11 highlight tracking */
/* #define PrivMode_MouseX11Track       (1LU<<18) */

#define PrivMode_mouse_report	(PrivMode_MouseX10|PrivMode_MouseX11)
#define PrivMode(test,bit)		\
    if (test)				\
	o->PrivateModes |= (bit);		\
    else				\
	o->PrivateModes &= ~(bit)

#define PrivMode_Default						 \
(PrivMode_Autowrap|PrivMode_aplKP|PrivMode_ShiftKeys|PrivMode_VisibleCursor)

/* command input buffering */
#ifndef BUFSIZ
# define BUFSIZ		4096
#endif
 unsigned char cmdbuf_base[BUFSIZ], *cmdbuf_ptr, *cmdbuf_endp;

#ifdef UTMP_SUPPORT
# if ! defined(HAVE_STRUCT_UTMPX) && ! defined(HAVE_STRUCT_UTMP)
#  error cannot build with utmp support - no utmp or utmpx struct found
# endif

# if defined(RXVT_UTMPX_FILE) && defined(HAVE_STRUCT_UTMPX)
#   define RXVT_UTMP_AS_UTMPX
# else
#  if defined(RXVT_UTMP_FILE) && defined(HAVE_STRUCT_UTMP)
#   undef RXVT_UTMP_AS_UTMPX
#  endif
# endif
/* if you have both utmp and utmpx files lying around and are really
 * using utmp not utmpx, then uncomment the following line */
/* #undef RXVT_UTMP_AS_UTMPX */

# ifdef RXVT_UTMP_AS_UTMPX
#  define RXVT_REAL_UTMP_FILE	RXVT_UTMPX_FILE
# else
#  define RXVT_REAL_UTMP_FILE	RXVT_UTMP_FILE
# endif

# ifdef RXVT_UTMP_AS_UTMPX
#  define USE_SYSV_UTMP
# else
#  ifdef HAVE_SETUTENT
#   define USE_SYSV_UTMP
#  else
#   undef USE_SYSV_UTMP
#  endif
# endif

# undef UTMP
# ifdef USE_SYSV_UTMP
#  ifndef USER_PROCESS
#   define USER_PROCESS		7
#  endif
#  ifndef DEAD_PROCESS
#   define DEAD_PROCESS		8
#  endif
#  ifdef RXVT_UTMP_AS_UTMPX
#   define UTMP			struct utmpx
#   define setutent		setutxent
#   define getutent		getutxent
#   define getutid		getutxid
#   define endutent		endutxent
#   define pututline		pututxline
#  endif
# endif
# ifndef UTMP
#  define UTMP			struct utmp
# endif

# ifdef WTMP_SUPPORT
#  ifdef RXVT_UTMP_AS_UTMPX
#   define update_wtmp		updwtmpx
#   ifdef RXVT_WTMPX_FILE
#    define RXVT_REAL_WTMP_FILE	RXVT_WTMPX_FILE
#   else
#    error cannot build with wtmp support - no wtmpx file found
#   endif
#  else
#   define update_wtmp		rxvt_update_wtmp
#   ifdef RXVT_WTMP_FILE
#    define RXVT_REAL_WTMP_FILE	RXVT_WTMP_FILE
#   else
#    error cannot build with wtmp support - no wtmp file found
#   endif
#  endif
# endif

#endif

struct menuitem_t {
    struct menuitem_t *prev;	/* prev menu-item */
    struct menuitem_t *next;	/* next menu-item */
    char *name;			/* character string displayed */
    char *name2;		/* character string displayed (right) */
    short len;			/* strlen (name) */
    short len2;			/* strlen (name) */
    union {
	short type;		/* must not be changed; first element */
	struct _action_t {
	    short type;		/* must not be changed; first element */
	    short len;		/* strlen (str) */
	    unsigned char *str;	/* action to take */
	} action;
	struct _submenu_t {
	    short type;		/* must not be changed; first element */
	    struct menu_t *menu;	/* sub-menu */
	} submenu;
    } entry;
} *dummy_var3;

enum menuitem_t_action {
    MenuLabel,
    MenuAction,
    MenuTerminalAction,
    MenuSubMenu
} dummy_var4;

struct bar_t {
    struct menu_t {
	struct menu_t *parent;	/* parent menu */
	struct menu_t *prev;	/* prev menu */
	struct menu_t *next;	/* next menu */
	menuitem_t *head;	/* double-linked list */
	menuitem_t *tail;	/* double-linked list */
	menuitem_t *item;	/* current item */
	char *name;		/* menu name */
	short len;		/* strlen (name) */
	short width;		/* maximum menu width [chars] */
	Window win;		/* window of the menu */
	short x;		/* x location [pixels] (chars if parent == NULL) */
	short y;		/* y location [pixels] */
	short w, h;		/* window width, height [pixels] */
    } *head, *tail;	/* double-linked list of menus */
    char *title;		/* title to put in the empty menuBar */
#if (MENUBAR_MAX > 1)
# define MAXNAME 16
    char name[MAXNAME];		/* name to use to refer to menubar */
    struct bar_t *next, *prev;	/* circular linked-list */
#endif				/* (MENUBAR_MAX > 1) */
#define NARROWS	4
    action_t arrows[NARROWS];
} *CurrentBar;

/* #define DEBUG_MENU */
/* #define DEBUG_MENU_LAYOUT */
/* #define DEBUG_MENUBAR_STACKING */

#define HSPACE		1	/* one space */
#define isSeparator(name)	((name)[0] == '\0')
#define HEIGHT_SEPARATOR	(SHADOW + 1)
#define HEIGHT_TEXT		(Height2Pixel(1) + 2)

#define MENU_DELAY_USEC	250000	/* 1/4 sec */

#define SEPARATOR_NAME		"-"
#define MENUITEM_BEG		'{'
#define MENUITEM_END		'}'
#define COMMENT_CHAR		'#'

#define DOT	"."
#define DOTS	".."

/* On Solaris link with -lsocket and -lnsl */

/* these next two are probably only on Sun (not Solaris) */

 struct remotefs *remotefs;
 char     host[256] ;   /* host of cmd_pid */
 char     ttydev[64] ;  /* tty on host */
 pid_t    cmd_pid ;     /* pid on host */
 int      cmd_fd ;
 int      Xfd ;

#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
 int      scroll_arrow_delay;
#endif
#ifdef META8_OPTION
 unsigned char meta_char;
#endif
 unsigned int ModMetaMask, ModNumLockMask;

 unsigned long PrivateModes ;
 unsigned long SavedModes ;

 int      refresh_count , refresh_limit , refresh_type ;
 Atom     wmDeleteWindow;

#ifdef USE_XIM
 XIC      Input_Context;
#endif

 char    *v_buffer;
 char    *v_bufstr ;
 char    *v_bufptr;
 char    *v_bufend;

#define FKEY(n, fkey)							\
    len = 5;								\
    sprintf((char *) kbuf,"\033[%02d~", (int)((n) + (keysym - fkey)))

#define TIMEOUT_USEC	5000

struct _MEvent {
    int             clicks;
    Time            time;	
    unsigned int    state;	
    unsigned int    button;	
} MEvent;

#define PrivCases(bit)							\
    if (mode == 's') {							\
	o->SavedModes |= (o->PrivateModes & bit);			\
	break;								\
    } else {								\
	if (mode == 'r')						\
	    state = (o->SavedModes & bit) ? 1 : 0;			\
	else								\
	    state = (mode == 't') ? !(o->PrivateModes & bit) : mode;	\
	PrivMode(state, bit);						\
    }

#define MAX_PTY_WRITE 128	/* 1/2 POSIX minimum MAX_INPUT */

#ifndef GRX_SCALE
# define GRX_SCALE	10000
#endif

#ifdef UTMP_SUPPORT
 char     ut_id[5];

# ifndef USE_SYSV_UTMP
 int      utmp_pos;

# endif
#endif

#define INTERN			/* assign all global vars to me */

 char *def_colorName[32] 
;

#ifdef MULTICHAR_SET

 char *def_mfontName[32] ;
#endif				/* MULTICHAR_SET */

 char *def_fontName[32] ;

#ifdef XTERM_COLOR_CHANGE

#else
# define set_window_color(idx,color)	((void)0)
#endif				/* XTERM_COLOR_CHANGE */

#if (FONT0_IDX == 0)
# define IDX2FNUM(i)	(i)
# define FNUM2IDX(f)	(f)
#else
# define IDX2FNUM(i)	(i == 0 ? FONT0_IDX : (i <= FONT0_IDX ? (i-1) : i))
# define FNUM2IDX(f)	(f == FONT0_IDX ? 0 : (f < FONT0_IDX  ? (f+1) : f))
#endif
#define FNUM_RANGE(i)	(i <= 0 ? 0 : (i >= NFONTS ? (NFONTS-1) : i))

#ifdef MENUBAR
#define Menu_PixelWidth(menu)					\
    (2 * SHADOW + Width2Pixel ((menu)->width + 3 * HSPACE))

 GC       topShadowGC, botShadowGC, neutralGC, menubarGC;

 int      menu_readonly ;
 int      Arrows_x ;
struct _Arrows {
    char            name;	
    unsigned char   str[4];	
};
 struct _Arrows Arrows[NARROWS] ;

#if (MENUBAR_MAX > 1)
 int      Nbars ;
#else				/* (MENUBAR_MAX > 1) */
 bar_t    BarList;
#endif				/* (MENUBAR_MAX > 1) */

 menu_t  *ActiveMenu ;
#endif

#define INTERN_SCREEN

 char     charsets[4];
 short    current_screen;
 rend_t   rstyle;
 short    rvideo;

#ifdef MULTICHAR_SET
short           multi_byte;
short           lost_multi;
enum _chstat {
    SBYTE, WBYTE
};

enum _chstat    chstat;

#define RESET_CHSTAT			\
    if (chstat == WBYTE)		\
	chstat = SBYTE, lost_multi = 1
#else
# define RESET_CHSTAT
#endif

#ifdef MULTICHAR_SET
int      encoding_method;
#endif

#define PROP_SIZE		4096
#define TABSIZE			8	/* default tab size */

#ifdef DEBUG_SCREEN
# define D_SCREEN(x)		fprintf x ; fputc('\n', stderr)
#else
# define D_SCREEN(x)
#endif
#ifdef DEBUG_SELECT
# define D_SELECT(x)		fprintf x ; fputc('\n', stderr)
#else
# define D_SELECT(x)
#endif

#define ZERO_SCROLLBACK						\
    if ((o->Options & Opt_scrollTtyOutput) == Opt_scrollTtyOutput)	\
	o->TermWin.view_start = 0
#define CHECK_SELECTION(x)					\
    if (o->selection.op)					\
	rxvtlib_selection_check(o, x)
#define CLEAR_SELECTION						\
    o->selection.beg.row = o->selection.beg.col			\
	= o->selection.end.row = o->selection.end.col = 0
#define CLEAR_ALL_SELECTION					\
    o->selection.beg.row = o->selection.beg.col			\
	= o->selection.mark.row = o->selection.mark.col		\
	= o->selection.end.row = o->selection.end.col = 0

#define ROW_AND_COL_IS_AFTER(A, B, C, D)				\
    (((A) > (C)) || (((A) == (C)) && ((B) > (D))))
#define ROW_AND_COL_IS_BEFORE(A, B, C, D)				\
    (((A) < (C)) || (((A) == (C)) && ((B) < (D))))
#define ROW_AND_COL_IN_ROW_AFTER(A, B, C, D)				\
    (((A) == (C)) && ((B) > (D)))
#define ROW_AND_COL_IN_ROW_AT_OR_AFTER(A, B, C, D)			\
    (((A) == (C)) && ((B) >= (D)))
#define ROW_AND_COL_IN_ROW_BEFORE(A, B, C, D)				\
    (((A) == (C)) && ((B) < (D)))
#define ROW_AND_COL_IN_ROW_AT_OR_BEFORE(A, B, C, D)			\
    (((A) == (C)) && ((B) <= (D)))

#define ROWCOL_IS_AFTER(X, Y)						\
    ROW_AND_COL_IS_AFTER((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IS_BEFORE(X, Y)						\
    ROW_AND_COL_IS_BEFORE((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_AFTER(X, Y)					\
    ROW_AND_COL_IN_ROW_AFTER((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_BEFORE(X, Y)					\
    ROW_AND_COL_IN_ROW_BEFORE((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_AT_OR_AFTER(X, Y)					\
    ROW_AND_COL_IN_ROW_AT_OR_AFTER((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_AT_OR_BEFORE(X, Y)				\
    ROW_AND_COL_IN_ROW_AT_OR_BEFORE((X).row, (X).col, (Y).row, (Y).col)

#if defined(XPM_BACKGROUND) && defined(XPM_BUFFERING)

#define drawBuffer	(o->TermWin.buf_pixmap)

#define CLEAR_ROWS(row, num)						\
    if (o->TermWin.mapped)						\
	XCopyArea(o->Xdisplay, o->TermWin.pixmap, drawBuffer, o->TermWin.gc,	\
		  Col2Pixel(0), Row2Pixel(row),				\
		  o->TermWin.width, Height2Pixel(num),			\
		  Col2Pixel(0), Row2Pixel(row))

#define CLEAR_CHARS(x, y, num)						\
    if (o->TermWin.mapped)							\
	XCopyArea(Xdisplay, TermWin.pixmap, drawBuffer, TermWin.gc,	\
		  x, y, Width2Pixel(num), Height2Pixel(1), x, y)

#else				/* XPM_BUFFERING && XPM_BACKGROUND */

#define drawBuffer	(o->TermWin.vt)

#define CLEAR_ROWS(row, num)						\
    if (o->TermWin.mapped)						\
	XClearArea(o->Xdisplay, drawBuffer, Col2Pixel(0), Row2Pixel(row),	\
		   o->TermWin.width, Height2Pixel(num), 0)

#define CLEAR_CHARS(x, y, num)						\
    if (o->TermWin.mapped)						\
	XClearArea(o->Xdisplay, drawBuffer, x, y,			\
		   Width2Pixel(num), Height2Pixel(1), 0)

#endif				/* XPM_BUFFERING && XPM_BACKGROUND */

#define ERASE_ROWS(row, num)						\
    XFillRectangle(o->Xdisplay, drawBuffer, o->TermWin.gc,		\
		   Col2Pixel(0), Row2Pixel(row),			\
		   o->TermWin.width, Height2Pixel(num))

 int      prev_nrow , prev_ncol ;

#ifdef MULTICHAR_SET
#ifdef KANJI
 void     (*multichar_decode) (unsigned char *str, int len) ;
#else				/* then we must be BIG5 to get in here */
# ifdef ZH
 void     (*multichar_decode) (unsigned char *str, int len) ;

# else
#  ifdef ZHCN			/* The GB fonts are in iso-2022 encoding (JIS). */
 void     (*multichar_decode) (unsigned char *str, int len) ;

#  endif
# endif
#endif

#endif				/* MULTICHAR_SET */

#define DRAW_STRING(Func, x, y, str, len)				\
    Func(o->Xdisplay, drawBuffer, o->TermWin.gc, (x), (y), (str), (len))

#if defined (NO_BRIGHTCOLOR) || defined (VERYBOLD)
# define MONO_BOLD(x)	((x) & (RS_Bold|RS_Blink))
#else
# define MONO_BOLD(x)	(((x) & RS_Bold) && fore == Color_fg)
#endif

#define FONT_WIDTH(X, Y)						\
    (X)->per_char[(Y) - (X)->min_char_or_byte2].width
#define FONT_RBEAR(X, Y)						\
    (X)->per_char[(Y) - (X)->min_char_or_byte2].rbearing

#define DELIMIT_TEXT(x) \
    ((text_t_to_char(x) == ' ' || text_t_to_char(x) == '\t') ? 2 : ((text_t_to_char(x) <= 255 && strchr(o->rs[Rs_cutchars], text_t_to_char(x)) != NULL)))
#ifdef MULTICHAR_SET
# define DELIMIT_REND(x)	(((x) & RS_multiMask) ? 1 : 0)
#else
# define DELIMIT_REND(x)	1
#endif

#ifndef NEXT_SCROLLBAR
 GC       scrollbarGC;

#ifdef XTERM_SCROLLBAR		/* bitmap scrollbar */
 GC       ShadowGC;

 char     sb_bits[] ;

#if (SB_WIDTH != 15)
#error Error, check scrollbar width (SB_WIDTH).It must be 15 for XTERM_SCROLLBAR
#endif

#else				/* XTERM_SCROLLBAR */
#ifndef MENUBAR
 GC       topShadowGC, botShadowGC;
#endif

#endif				/* ! XTERM_SCROLLBAR */

#else				/* ! NEXT_SCROLLBAR */

 GC       blackGC, whiteGC, grayGC, darkGC, stippleGC;
 Pixmap   dimple, upArrow, downArrow, upArrowHi, downArrowHi;

char     *SCROLLER_DIMPLE[32];

#define SCROLLER_DIMPLE_WIDTH   6
#define SCROLLER_DIMPLE_HEIGHT  6

char     *SCROLLER_ARROW_UP[32];

char     *SCROLLER_ARROW_DOWN[32];

char     *HI_SCROLLER_ARROW_UP[32];

char     *HI_SCROLLER_ARROW_DOWN[32];

#define ARROW_WIDTH   13
#define ARROW_HEIGHT  13

#define stp_width 8
#define stp_height 8
unsigned char stp_bits[256] ;

#endif				/* ! NEXT_SCROLLBAR */

#if 0
#define INFO(opt, arg, desc)				\
    {0, NULL, NULL, opt, arg, desc}

#define STRG(p, kw, opt, arg, desc)			\
    {0, &(o->rs[p]), kw, opt, arg, desc}

#define RSTRG(p, kw, arg)				\
    {0, &(o->rs[p]), kw, NULL, arg, NULL}

#define BOOL(p, kw, opt, flag, desc)			\
    {(Opt_Boolean|flag), &(o->rs[p]), kw, opt, NULL, desc}

#define SWCH(opt, flag, desc)				\
    {(flag), NULL, NULL, opt, NULL, desc}
#else

#define INFO(opt, arg, desc)				\
    {0, -1, NULL, opt, arg, desc}

#define STRG(p, kw, opt, arg, desc)			\
    {0, p, kw, opt, arg, desc}

#define RSTRG(p, kw, arg)				\
    {0, p, kw, NULL, arg, NULL}

#define BOOL(p, kw, opt, flag, desc)			\
    {(Opt_Boolean|flag), p, kw, opt, NULL, desc}

#define SWCH(opt, flag, desc)				\
    {(flag), -1, NULL, opt, NULL, desc}

#endif
#define INDENT 30

#ifndef NO_RESOURCES

# ifdef KEYSYM_RESOURCE

#define NEWARGLIM	500	/* `reasonable' size */

# endif				/* KEYSYM_RESOURCE */

#endif				/* NO_RESOURCES */

struct _bgPixmap_t {
    short           w, h, x, y;
    Pixmap          pixmap;
} bgPixmap;

#ifdef XPM_BACKGROUND
 XpmAttributes xpmAttr;
#endif

#ifndef STANDALONE
    XEvent xevent;
    int x_events_pending;
    int cmd_fd_available;
    int fds_available;
#endif
    int old_width;
    int old_height;
    rxvt_buf_char_t *buffer;
    int currmaxcol;
#ifdef MULTICHAR_SET
    int oldcursormulti;
#endif
#ifdef MENUBAR
    struct menu_t *BuildMenu;
#endif
    Window parent_window;
    int killed;
};

enum _sstyle_t {
    OLD_SELECT, OLD_WORD_SELECT, NEW_SELECT
};
typedef enum _sstyle_t sstyle_t;

#ifdef MULTICHAR_SET
enum _ENC_METHOD {
    EUCJ, SJIS,			
    BIG5, CNS,			
    GB				
};
typedef enum _ENC_METHOD ENC_METHOD;
#endif

#include "rxvtlibproto.h"

