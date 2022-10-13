/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"
#include "stringtools.h"
#include "edit.h"


#ifdef UTF8_FONT
#define XSetFont(d, gc, fid)    do { } while (0)
#endif


/*--------------------------------*-C-*--------------------------------------*
 * File:	screen.c
 *---------------------------------------------------------------------------*
 * $Id: screen.c,v 1.76.2.9 1999/07/07 13:22:22 mason Exp $
 *
 * Copyright (C) 1997,1998 Geoff Wing <gcw@pobox.com>
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
 *--------------------------------------------------------------------------*/
/*
 * We handle _all_ screen updates and selections
 */

/* ------------------------------------------------------------------------- */

#ifdef MULTICHAR_SET
	/* set ==> currently using 2 bytes per glyph */
	/* set ==> we only got half a glyph */

#else
#endif

/* KANJI methods *//* BIG5 methods: CNS not implemented *//* GB method */

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 *             GENERAL SCREEN AND SELECTION UPDATE ROUTINES                  *
 * ------------------------------------------------------------------------- */

/* these must be row_col_t */

/*
 * CLEAR_ROWS : clear <num> rows starting from row <row>
 * CLEAR_CHARS: clear <num> chars starting from pixel position <x,y>
 * ERASE_ROWS : set <num> rows starting from row <row> to the foreground colour
 */

#ifdef UTF8_FONT

static inline text_t char_to_text_t (rxvt_buf_char_t c)
{
    text_t r;
    r.s[0] = c >> 16;
    r.s[1] = c >> 8;
    r.s[2] = c >> 0;
    return r;
}

static inline rxvt_buf_char_t text_t_to_char (text_t c)
{
    rxvt_buf_char_t r;
    r = c.s[0];
    r <<= 8;
    r |= c.s[1];
    r <<= 8;
    r |= c.s[2];
    return r;
}

#else

#define char_to_text_t(c)       (c)
#define text_t_to_char(c)       (c)

#endif

/* ------------------------------------------------------------------------- *
 *                        SCREEN `COMMON' ROUTINES                           *
 * ------------------------------------------------------------------------- */
/* Fill part/all of a line with blanks. */
/* INTPROTO */
void            blank_line (text_t * et, rend_t * er, int width, rend_t efs)
{E_
    for (; width--;) {
	*er++ = efs;
	*et++ = char_to_text_t (' ');
    }
}

/* ------------------------------------------------------------------------- */
/* Fill a full line with blanks - make sure it is allocated first */
/* INTPROTO */
void            rxvtlib_blank_screen_mem (rxvtlib *o, text_t ** tp, rend_t ** rp, int row,
				  rend_t efs)
{E_
    int             width = o->TermWin.ncol;
    rend_t         *er;
    text_t         *et;

    if (tp[row] == NULL) {
	tp[row] = (text_t *) MALLOC (sizeof (text_t) * o->TermWin.ncol);
	rp[row] = (rend_t *) MALLOC (sizeof (rend_t) * o->TermWin.ncol);
    }
    for (er = rp[row], et = tp[row]; width--;) {
	*er++ = efs;
	*et++ = char_to_text_t (' ');
    }
}

/* ------------------------------------------------------------------------- *
 *                          SCREEN INITIALISATION                            *
 * ------------------------------------------------------------------------- */

/* EXTPROTO */
void            rxvtlib_scr_reset (rxvtlib *o)
{E_
    int             i, j, k, total_rows, prev_total_rows;
    rend_t          setrstyle;

    D_SCREEN ((stderr, "scr_reset()"));

    o->TermWin.view_start = 0;
    RESET_CHSTAT;

    if (o->TermWin.ncol == o->prev_ncol && o->TermWin.nrow == o->prev_nrow)
	return;

    o->want_refresh = 1;

    if (o->TermWin.ncol <= 0)
	o->TermWin.ncol = 80;
    if (o->TermWin.nrow <= 0)
	o->TermWin.nrow = 24;
#ifdef DEBUG_STRICT
    assert (o->TermWin.saveLines >= 0);
#else				/* drive with your eyes closed */
    MAX_IT (o->TermWin.saveLines, 0);
#endif

    total_rows = o->TermWin.nrow + o->TermWin.saveLines;
    prev_total_rows = o->prev_nrow + o->TermWin.saveLines;

    o->screen.tscroll = 0;
    o->screen.bscroll = (o->TermWin.nrow - 1);

    if (o->prev_nrow == -1) {
/*
 * A: first time called so just malloc everything : don't rely on realloc
 *    Note: this is still needed so that all the scrollback lines are NULL
 */
	o->screen.text = CALLOC (text_t *, total_rows);
	o->buf_text = CALLOC (text_t *, total_rows);
	o->drawn_text = CALLOC (text_t *, o->TermWin.nrow);
	o->swap.text = CALLOC (text_t *, o->TermWin.nrow);

	o->screen.tlen = CALLOC (short, total_rows);
	o->buf_tlen = CALLOC (short, total_rows);
	o->swap.tlen = CALLOC (short, o->TermWin.nrow);

	o->screen.rend = CALLOC (rend_t *, total_rows);
	o->buf_rend = CALLOC (rend_t *, total_rows);
	o->drawn_rend = CALLOC (rend_t *, o->TermWin.nrow);
	o->swap.rend = CALLOC (rend_t *, o->TermWin.nrow);

	for (i = 0; i < o->TermWin.nrow; i++) {
	    j = i + o->TermWin.saveLines;
	    rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, j, DEFAULT_RSTYLE);
	    rxvtlib_blank_screen_mem (o, o->swap.text, o->swap.rend, i, DEFAULT_RSTYLE);
	    o->screen.tlen[j] = o->swap.tlen[i] = 0;
	    rxvtlib_blank_screen_mem (o, o->drawn_text, o->drawn_rend, i, DEFAULT_RSTYLE);
	}
	o->TermWin.nscrolled = 0;	/* no saved lines */
	o->screen.flags = o->swap.flags = Screen_DefaultFlags;
	o->save.cur.row = o->save.cur.col = 0;
	o->save.charset = 0;
	o->save.charset_char = 'B';
	o->rstyle = o->save.rstyle = DEFAULT_RSTYLE;
	o->selection.text = NULL;
	o->selection.len = 0;
	o->selection.op = SELECTION_CLEAR;
	o->selection.screen = PRIMARY;
	o->selection.clicks = 0;
	CLEAR_ALL_SELECTION;
	MEMSET (o->charsets, 'B', sizeof (o->charsets));
	o->current_screen = PRIMARY;
	o->rvideo = 0;
#ifdef MULTICHAR_SET
	o->multi_byte = 0;
	o->lost_multi = 0;
	o->chstat = SBYTE;
# ifdef ZH
	o->encoding_method = BIG5;
# else
#  ifdef ZHCN
	o->encoding_method = GB;
#  endif
# endif
#endif

    } else {
/*
 * B1: add or delete rows as appropriate
 */
	setrstyle = DEFAULT_RSTYLE | (o->rvideo ? RS_RVid : 0);

	if (o->TermWin.nrow < o->prev_nrow) {
	    /* delete rows */
	    k = min (o->TermWin.nscrolled, o->prev_nrow - o->TermWin.nrow);
	    rxvtlib_scroll_text (o, 0, o->prev_nrow - 1, k, 1);
	    for (i = o->TermWin.nrow; i < o->prev_nrow; i++) {
		j = i + o->TermWin.saveLines;
		if (o->screen.text[j])
		    FREE (o->screen.text[j]);
		if (o->screen.rend[j])
		    FREE (o->screen.rend[j]);
		if (o->swap.text[i])
		    FREE (o->swap.text[i]);
		if (o->swap.rend[i])
		    FREE (o->swap.rend[i]);
		FREE (o->drawn_text[i]);
		FREE (o->drawn_rend[i]);
	    }
	    /* we have fewer rows so fix up cursor position */
	    MIN_IT (o->screen.cur.row, o->TermWin.nrow - 1);
	    MIN_IT (o->swap.cur.row, o->TermWin.nrow - 1);

	    rxvtlib_scr_reset_realloc (o);	/* REALLOC _last_ */

	} else if (o->TermWin.nrow > o->prev_nrow) {
	    /* add rows */
	    rxvtlib_scr_reset_realloc (o);	/* REALLOC _first_ */

	    k = min (o->TermWin.nscrolled, o->TermWin.nrow - o->prev_nrow);
	    for (i = prev_total_rows; i < total_rows - k; i++) {
		o->screen.tlen[i] = 0;
		o->screen.text[i] = NULL;
		rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, i, setrstyle);
	    }
	    for (i = total_rows - k; i < total_rows; i++) {
		o->screen.tlen[i] = 0;
		o->screen.text[i] = NULL;
		o->screen.rend[i] = NULL;
	    }
	    for (i = o->prev_nrow; i < o->TermWin.nrow; i++) {
		o->swap.tlen[i] = 0;
		o->swap.text[i] = NULL;
		o->drawn_text[i] = NULL;
		rxvtlib_blank_screen_mem (o, o->swap.text, o->swap.rend, i, setrstyle);
		rxvtlib_blank_screen_mem (o, o->drawn_text, o->drawn_rend, i, setrstyle);
	    }
	    if (k > 0) {
		rxvtlib_scroll_text (o, 0, o->TermWin.nrow - 1, -k, 1);
		o->screen.cur.row += k;
		o->TermWin.nscrolled -= k;
		for (i = o->TermWin.saveLines - o->TermWin.nscrolled; k--; i--)
		    if (o->screen.text[i] == NULL) {
			rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, i,
					  setrstyle);
			o->screen.tlen[i] = 0;
		    }
	    }
#ifdef DEBUG_STRICT
	    assert (o->screen.cur.row < o->TermWin.nrow);
	    assert (o->swap.cur.row < o->TermWin.nrow);
#else				/* drive with your eyes closed */
	    MIN_IT (o->screen.cur.row, o->TermWin.nrow - 1);
	    MIN_IT (o->swap.cur.row, o->TermWin.nrow - 1);
#endif
	}
/* B2: resize columns */
	if (o->TermWin.ncol != o->prev_ncol) {
	    for (i = 0; i < total_rows; i++) {
		if (o->screen.text[i]) {
		    o->screen.text[i] = (text_t *) REALLOC (o->screen.text[i],
					      o->TermWin.ncol * sizeof (text_t));
		    o->screen.rend[i] = (rend_t *) REALLOC (o->screen.rend[i],
					      o->TermWin.ncol * sizeof (rend_t));
		    MIN_IT (o->screen.tlen[i], o->TermWin.ncol);
		    if (o->TermWin.ncol > o->prev_ncol)
			blank_line (&(o->screen.text[i][o->prev_ncol]),
				    &(o->screen.rend[i][o->prev_ncol]),
				    o->TermWin.ncol - o->prev_ncol, setrstyle);
		}
	    }
	    for (i = 0; i < o->TermWin.nrow; i++) {
		o->drawn_text[i] = (text_t *) REALLOC (o->drawn_text[i],
					 o->TermWin.ncol * sizeof (text_t));
		o->drawn_rend[i] = (rend_t *) REALLOC (o->drawn_rend[i],
					 o->TermWin.ncol * sizeof (rend_t));
		if (o->swap.text[i]) {
		    o->swap.text[i] = (text_t *) REALLOC (o->swap.text[i],
					    o->TermWin.ncol * sizeof (text_t));
		    o->swap.rend[i] = (rend_t *) REALLOC (o->swap.rend[i],
					    o->TermWin.ncol * sizeof (rend_t));
		    MIN_IT (o->swap.tlen[i], o->TermWin.ncol);
		    if (o->TermWin.ncol > o->prev_ncol)
			blank_line (&(o->swap.text[i][o->prev_ncol]),
				    &(o->swap.rend[i][o->prev_ncol]),
				    o->TermWin.ncol - o->prev_ncol, setrstyle);
		}
		if (o->TermWin.ncol > o->prev_ncol)
		    blank_line (&(o->drawn_text[i][o->prev_ncol]),
				&(o->drawn_rend[i][o->prev_ncol]),
				o->TermWin.ncol - o->prev_ncol, setrstyle);
	    }
	    MIN_IT (o->screen.cur.col, o->TermWin.ncol - 1);
	    MIN_IT (o->swap.cur.col, o->TermWin.ncol - 1);
	}
	if (o->tabs)
	    FREE (o->tabs);
    }

    o->tabs = MALLOC (o->TermWin.ncol * sizeof (char));

    for (i = 0; i < o->TermWin.ncol; i++)
	o->tabs[i] = (i % TABSIZE == 0) ? 1 : 0;

    o->prev_nrow = o->TermWin.nrow;
    o->prev_ncol = o->TermWin.ncol;

    rxvtlib_tt_resize (o);
}

/* INTPROTO */
void            rxvtlib_scr_reset_realloc (rxvtlib *o)
{E_
    int             total_rows;

    total_rows = o->TermWin.nrow + o->TermWin.saveLines;
/* *INDENT-OFF* */
    o->screen.text = (text_t **) REALLOC(o->screen.text, total_rows   * sizeof(text_t *));
    o->buf_text    = (text_t **) REALLOC(o->buf_text   , total_rows   * sizeof(text_t *));
    o->drawn_text  = (text_t **) REALLOC(o->drawn_text , o->TermWin.nrow * sizeof(text_t *));
    o->swap.text   = (text_t **) REALLOC(o->swap.text  , o->TermWin.nrow * sizeof(text_t *));

    o->screen.tlen = REALLOC(o->screen.tlen, total_rows   * sizeof(short));
    o->buf_tlen    = REALLOC(o->buf_tlen   , total_rows   * sizeof(short));
    o->swap.tlen   = REALLOC(o->swap.tlen  , total_rows   * sizeof(short));

    o->screen.rend = (rend_t **) REALLOC(o->screen.rend, total_rows   * sizeof(rend_t *));
    o->buf_rend    = (rend_t **) REALLOC(o->buf_rend   , total_rows   * sizeof(rend_t *));
    o->drawn_rend  = (rend_t **) REALLOC(o->drawn_rend , o->TermWin.nrow * sizeof(rend_t *));
    o->swap.rend   = (rend_t **) REALLOC(o->swap.rend  , o->TermWin.nrow * sizeof(rend_t *));
/* *INDENT-ON* */
}

/* ------------------------------------------------------------------------- */
/*
 * Free everything.  That way malloc debugging can find leakage.
 */
/* EXTPROTO */
void            rxvtlib_scr_release (rxvtlib *o)
{E_
    int             i, total_rows;

    total_rows = o->TermWin.nrow + o->TermWin.saveLines;
    for (i = 0; i < total_rows; i++) {
	if (o->screen.text[i]) {	/* then so is screen.rend[i] */
	    FREE (o->screen.text[i]);
	    FREE (o->screen.rend[i]);
	}
    }
    for (i = 0; i < o->TermWin.nrow; i++) {
	FREE (o->drawn_text[i]);
	FREE (o->drawn_rend[i]);
	FREE (o->swap.text[i]);
	FREE (o->swap.rend[i]);
    }
    FREE (o->screen.text);
    FREE (o->screen.tlen);
    FREE (o->screen.rend);
    FREE (o->drawn_text);
    FREE (o->drawn_rend);
    FREE (o->swap.text);
    FREE (o->swap.tlen);
    FREE (o->swap.rend);
    FREE (o->buf_text);
    FREE (o->buf_tlen);
    FREE (o->buf_rend);
    FREE (o->tabs);

/* NULL these so if anything tries to use them, we'll know about it */
    o->screen.text = o->drawn_text = o->swap.text = NULL;
    o->screen.rend = o->drawn_rend = o->swap.rend = NULL;
    o->screen.tlen = o->swap.tlen = o->buf_tlen = NULL;
    o->buf_text = NULL;
    o->buf_rend = NULL;
    o->tabs = NULL;
}

/* ------------------------------------------------------------------------- */
/* EXTPROTO */
void            rxvtlib_scr_poweron (rxvtlib *o)
{E_
    D_SCREEN ((stderr, "scr_poweron()"));

    MEMSET (o->charsets, 'B', sizeof (o->charsets));
    o->rvideo = 0;
    o->swap.tscroll = 0;
    o->swap.bscroll = o->TermWin.nrow - 1;
    o->screen.cur.row = o->screen.cur.col = o->swap.cur.row = o->swap.cur.col = 0;
    o->screen.charset = o->swap.charset = 0;
    o->screen.flags = o->swap.flags = Screen_DefaultFlags;

    rxvtlib_scr_cursor (o, SAVE);

    rxvtlib_scr_release (o);
    o->prev_nrow = -1;
    o->prev_ncol = -1;
    rxvtlib_scr_reset (o);

    rxvtlib_scr_clear (o);
    rxvtlib_scr_refresh (o, SLOW_REFRESH);
    rxvtlib_Gr_reset (o);
}

/* ------------------------------------------------------------------------- *
 *                         PROCESS SCREEN COMMANDS                           *
 * ------------------------------------------------------------------------- */
/*
 * Save and Restore cursor
 * XTERM_SEQ: Save cursor   : ESC 7     
 * XTERM_SEQ: Restore cursor: ESC 8
 */
/* EXTPROTO */
void            rxvtlib_scr_cursor (rxvtlib *o, int mode)
{E_
    D_SCREEN ((stderr, "scr_cursor(%c)", mode));

    switch (mode) {
    case SAVE:
	o->save.cur.row = o->screen.cur.row;
	o->save.cur.col = o->screen.cur.col;
	o->save.rstyle = o->rstyle;
	o->save.charset = o->screen.charset;
	o->save.charset_char = o->charsets[o->screen.charset];
	break;
    case RESTORE:
	o->want_refresh = 1;
	o->screen.cur.row = o->save.cur.row;
	o->screen.cur.col = o->save.cur.col;
	o->rstyle = o->save.rstyle;
	o->screen.charset = o->save.charset;
	o->charsets[o->screen.charset] = o->save.charset_char;
	rxvtlib_set_font_style (o);
	break;
    }
/* boundary check in case screen size changed between SAVE and RESTORE */
    MIN_IT (o->screen.cur.row, o->TermWin.nrow - 1);
    MIN_IT (o->screen.cur.col, o->TermWin.ncol - 1);
#ifdef DEBUG_STRICT
    assert (o->screen.cur.row >= 0);
    assert (o->screen.cur.col >= 0);
#else				/* drive with your eyes closed */
    MAX_IT (o->screen.cur.row, 0);
    MAX_IT (o->screen.cur.col, 0);
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Swap between primary and secondary screens
 * XTERM_SEQ: Primary screen  : ESC [ ? 4 7 h
 * XTERM_SEQ: Secondary screen: ESC [ ? 4 7 l
 */
/* EXTPROTO */
int             rxvtlib_scr_change_screen (rxvtlib *o, int scrn)
{E_
    int             i, tmp;

#if NSCREENS
    int             offset;
    text_t         *t0;
    rend_t         *r0;
    short         l0;
#endif

    o->want_refresh = 1;

    D_SCREEN ((stderr, "scr_change_screen(%d)", scrn));

    o->TermWin.view_start = 0;
    RESET_CHSTAT;

    if (o->current_screen == scrn)
	return o->current_screen;

    CHECK_SELECTION (2);	/* check for boundary cross */

    SWAP_IT (o->current_screen, scrn, tmp);
#if NSCREENS
    offset = o->TermWin.saveLines;
    for (i = o->TermWin.nrow; i--;) {
	SWAP_IT (o->screen.text[i + offset], o->swap.text[i], t0);
	SWAP_IT (o->screen.tlen[i + offset], o->swap.tlen[i], l0);
	SWAP_IT (o->screen.rend[i + offset], o->swap.rend[i], r0);
    }
    SWAP_IT (o->screen.cur.row, o->swap.cur.row, l0);
    SWAP_IT (o->screen.cur.col, o->swap.cur.col, l0);
# ifdef DEBUG_STRICT
    assert (o->screen.cur.row >= 0);
    assert (o->screen.cur.col >= 0);
    assert (o->screen.cur.row < o->TermWin.nrow);
    assert (o->screen.cur.col < o->TermWin.ncol);
# else				/* drive with your eyes closed */
    MAX_IT (o->screen.cur.row, 0);
    MAX_IT (o->screen.cur.col, 0);
    MIN_IT (o->screen.cur.row, o->TermWin.nrow - 1);
    MIN_IT (o->screen.cur.col, o->TermWin.ncol - 1);
# endif
    SWAP_IT (o->screen.charset, o->swap.charset, l0);
    SWAP_IT (o->screen.flags, o->swap.flags, tmp);
    o->screen.flags |= Screen_VisibleCursor;
    o->swap.flags |= Screen_VisibleCursor;

    if (rxvtlib_Gr_Displayed (o)) {
	rxvtlib_Gr_scroll (o, 0);
	rxvtlib_Gr_ChangeScreen (o);
    }
#else
# ifdef SCROLL_ON_NO_SECONDARY
    if (rxvtlib_Gr_Displayed (o))
	rxvtlib_Gr_ClearScreen (o);
    if (o->current_screen == PRIMARY) {
	if (!rxvtlib_Gr_Displayed (o))
	    rxvtlib_scroll_text (o, 0, (o->TermWin.nrow - 1), o->TermWin.nrow, 0);
	for (i = o->TermWin.saveLines; i < o->TermWin.nrow + o->TermWin.saveLines; i++)
	    if (o->screen.text[i] == NULL) {
		rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, i, DEFAULT_RSTYLE);
		o->screen.tlen[i] = 0;
	    }
    }
# endif
#endif
    return scrn;
}

/* ------------------------------------------------------------------------- */
/*
 * Change the colour for following text
 */
/* EXTPROTO */
void            rxvtlib_scr_color (rxvtlib *o, unsigned int color, unsigned int Intensity)
{E_
    if (color == restoreFG)
	color = Color_fg;
    else if (color == restoreBG)
	color = Color_bg;
    else {
	if (o->Xdepth <= 2) {	/* Monochrome - ignore colour changes */
	    switch (Intensity) {
	    case RS_Bold:
		color = Color_fg;
		break;
	    case RS_Blink:
		color = Color_bg;
		break;
	    }
	} else {
#ifndef NO_BRIGHTCOLOR
	    if ((o->rstyle & Intensity) && color >= minCOLOR && color <= maxCOLOR)
		color += (minBrightCOLOR - minCOLOR);
	    else if (color >= minBrightCOLOR && color <= maxBrightCOLOR) {
		if (o->rstyle & Intensity)
		    return;
		color -= (minBrightCOLOR - minCOLOR);
	    }
#endif
	}
    }
    switch (Intensity) {
    case RS_Bold:
	o->rstyle = SET_FGCOLOR (o->rstyle, color);
	break;
    case RS_Blink:
	o->rstyle = SET_BGCOLOR (o->rstyle, color);
	break;
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Change the rendition style for following text
 */
/* EXTPROTO */
void            rxvtlib_scr_rendition (rxvtlib *o, int set, int style)
{E_
    unsigned int    color;
    rend_t          font_attr;

    if (set) {
/* A: Set style */
	o->rstyle |= style;
	switch (style) {
	case RS_RVid:
	    if (o->rvideo)
		o->rstyle &= ~RS_RVid;
	    break;
#ifndef NO_BRIGHTCOLOR
	case RS_Bold:
	    color = GET_FGCOLOR (o->rstyle);
	    rxvtlib_scr_color (o, (color == Color_fg ? GET_FGCOLOR (o->colorfgbg) : color),
		       RS_Bold);
	    break;
	case RS_Blink:
	    color = GET_BGCOLOR (o->rstyle);
	    rxvtlib_scr_color (o, (color == Color_bg ? GET_BGCOLOR (o->colorfgbg) : color),
		       RS_Blink);
	    break;
#endif
	}
    } else {
/* B: Unset style */
	font_attr = o->rstyle & RS_fontMask;
	o->rstyle &= ~style;

	switch (style) {
	case ~RS_None:		/* default fg/bg colours */
	    o->rstyle = DEFAULT_RSTYLE | font_attr;
	    /* FALLTHROUGH */
	case RS_RVid:
	    if (o->rvideo)
		o->rstyle |= RS_RVid;
	    break;
#ifndef NO_BRIGHTCOLOR
	case RS_Bold:
	    color = GET_FGCOLOR (o->rstyle);
	    if (color >= minBrightCOLOR && color <= maxBrightCOLOR) {
		rxvtlib_scr_color (o, color, RS_Bold);
		if ((o->rstyle & RS_fgMask) == (o->colorfgbg & RS_fgMask))
		    rxvtlib_scr_color (o, restoreFG, RS_Bold);
	    }
	    break;
	case RS_Blink:
	    color = GET_BGCOLOR (o->rstyle);
	    if (color >= minBrightCOLOR && color <= maxBrightCOLOR) {
		rxvtlib_scr_color (o, color, RS_Blink);
		if ((o->rstyle & RS_bgMask) == (o->colorfgbg & RS_bgMask))
		    rxvtlib_scr_color (o, restoreBG, RS_Blink);
	    }
	    break;
#endif
	}
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Scroll text between <row1> and <row2> inclusive, by <count> lines
 * count positive ==> scroll up
 * count negative ==> scroll down
 * spec == 0 for normal routines
 */
/* INTPROTO */
int             rxvtlib_scroll_text (rxvtlib *o, int row1, int row2, int count, int spec)
{E_
    int             i, j;

    o->want_refresh = 1;
    D_SCREEN (
	      (stderr, "scroll_text(%d,%d,%d,%d): %s", row1, row2, count, spec,
	       (o->current_screen == PRIMARY) ? "Primary" : "Secondary"));

    if (count == 0 || (row1 > row2))
	return 0;

    if ((count > 0) && (row1 == 0) && (o->current_screen == PRIMARY)) {
	o->TermWin.nscrolled += count;
	MIN_IT (o->TermWin.nscrolled, o->TermWin.saveLines);
    } else if (!spec)
	row1 += o->TermWin.saveLines;
    row2 += o->TermWin.saveLines;

    if (o->selection.op && o->current_screen == o->selection.screen) {
	i = o->selection.beg.row + o->TermWin.saveLines;
	j = o->selection.end.row + o->TermWin.saveLines;
	if ((i < row1 && j > row1)
	    || (i < row2 && j > row2)
	    || (i - count < row1 && i >= row1)
	    || (i - count > row2 && i <= row2)
	    || (j - count < row1 && j >= row1)
	    || (j - count > row2 && j <= row2)) {
	    CLEAR_ALL_SELECTION;
	    o->selection.op = SELECTION_CLEAR;	/* XXX: too aggressive? */
	} else if (j >= row1 && j <= row2) {
	    /* move selected region too */
	    o->selection.beg.row -= count;
	    o->selection.end.row -= count;
	    o->selection.mark.row -= count;
	}
    }
    CHECK_SELECTION (0);	/* _after_ TermWin.nscrolled update */

    if (count > 0) {
/* A: scroll up */

	MIN_IT (count, row2 - row1 + 1);
/* A1: Copy lines that will get clobbered by the rotation */
	for (i = 0, j = row1; i < count; i++, j++) {
	    o->buf_text[i] = o->screen.text[j];
	    o->buf_tlen[i] = o->screen.tlen[j];
	    o->buf_rend[i] = o->screen.rend[j];
	}
/* A2: Rotate lines */
	for (j = row1; (j + count) <= row2; j++) {
	    o->screen.text[j] = o->screen.text[j + count];
	    o->screen.tlen[j] = o->screen.tlen[j + count];
	    o->screen.rend[j] = o->screen.rend[j + count];
	}
/* A3: Resurrect lines */
	for (i = 0; i < count; i++, j++) {
	    o->screen.text[j] = o->buf_text[i];
	    o->screen.tlen[j] = o->buf_tlen[i];
	    o->screen.rend[j] = o->buf_rend[i];
	}
    } else if (count < 0) {
/* B: scroll down */

	count = min (-count, row2 - row1 + 1);
/* B1: Copy lines that will get clobbered by the rotation */
	for (i = 0, j = row2; i < count; i++, j--) {
	    o->buf_text[i] = o->screen.text[j];
	    o->buf_tlen[i] = o->screen.tlen[j];
	    o->buf_rend[i] = o->screen.rend[j];
	}
/* B2: Rotate lines */
	for (j = row2; (j - count) >= row1; j--) {
	    o->screen.text[j] = o->screen.text[j - count];
	    o->screen.tlen[j] = o->screen.tlen[j - count];
	    o->screen.rend[j] = o->screen.rend[j - count];
	}
/* B3: Resurrect lines */
	for (i = 0, j = row1; i < count; i++, j++) {
	    o->screen.text[j] = o->buf_text[i];
	    o->screen.tlen[j] = o->buf_tlen[i];
	    o->screen.rend[j] = o->buf_rend[i];
	}
	count = -count;
    }
    if (rxvtlib_Gr_Displayed (o))
	rxvtlib_Gr_scroll (o, count);
    return count;
}

/* ------------------------------------------------------------------------- */
/*
 * A safe scroll text routine
 */
/* EXTPROTO */
void            rxvtlib_scr_scroll_text (rxvtlib *o, int count)
{E_
    int             row, erow;

    if (count == 0)
	return;
    count = rxvtlib_scroll_text (o, o->screen.tscroll, o->screen.bscroll, count, 0);
/* XXX: Ummm, no?  care needed with [bt]scroll, yes? */
    if (count > 0) {
	row = o->TermWin.nrow - count + o->TermWin.saveLines;
	erow = o->TermWin.nrow + o->TermWin.saveLines;
    } else {
	row = o->TermWin.saveLines;
	erow = o->TermWin.saveLines - count;
    }
    for (; row < erow; row++)
	if (o->screen.text[row] == NULL)
	    rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, row, o->rstyle);
}

/* ------------------------------------------------------------------------- */
/*
 * Add text given in <str> of length <len> to screen struct
 */
/* EXTPROTO */
void            rxvtlib_scr_add_lines (rxvtlib *o, const unsigned char *str, int nlines, int len)
{E_
    rxvt_buf_char_t c;
    int             i, j, row, last_col, checksel, clearsel;
    text_t         *stp;
    rend_t         *srp;

    if (len <= 0)		/* sanity */
	return;

    o->want_refresh = 1;
    last_col = o->TermWin.ncol;

    D_SCREEN ((stderr, "scr_add_lines(*,%d,%d)", nlines, len));
    ZERO_SCROLLBACK;
    if (nlines > 0) {
	nlines += (o->screen.cur.row - o->screen.bscroll);
	if ((nlines > 0)
	    && (o->screen.tscroll == 0)
	    && (o->screen.bscroll == (o->TermWin.nrow - 1))) {
	    /* _at least_ this many lines need to be scrolled */
	    rxvtlib_scroll_text (o, o->screen.tscroll, o->screen.bscroll, nlines, 0);
	    for (i = nlines, j = o->screen.bscroll + o->TermWin.saveLines; i--; j--) {
		rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, j, o->rstyle);
		o->screen.tlen[j] = 0;
	    }
	    o->screen.cur.row -= nlines;
	}
    }
#ifdef DEBUG_STRICT
    assert (o->screen.cur.col < last_col);
    assert (o->screen.cur.row < o->TermWin.nrow);
    assert (o->screen.cur.row >= -o->TermWin.nscrolled);
#else				/* drive with your eyes closed */
    MIN_IT (o->screen.cur.col, last_col - 1);
    MIN_IT (o->screen.cur.row, o->TermWin.nrow - 1);
    MAX_IT (o->screen.cur.row, -o->TermWin.nscrolled);
#endif
    row = o->screen.cur.row + o->TermWin.saveLines;

    checksel = (o->selection.op && o->current_screen == o->selection.screen) ? 1 : 0;
    clearsel = 0;

    stp = o->screen.text[row];
    srp = o->screen.rend[row];

#ifdef MULTICHAR_SET
    if (o->lost_multi && o->screen.cur.col > 0
	&& ((srp[o->screen.cur.col - 1] & RS_multiMask) == RS_multi1)
	&& *str != '\n' && *str != '\r' && *str != '\t')
	o->chstat = WBYTE;
#endif

    for (i = 0; i < len || o->utf8buflen;) {
#ifdef UTF8_FONT
        if (!o->utf8buflen && str[i] < 0xC0) {
	    c = str[i++];       /* inline the common case */
        } else {
            int e, l;
            l = len - i;
            if (l > 6 - o->utf8buflen)
                l = 6 - o->utf8buflen;
            memcpy (o->utf8buf + o->utf8buflen, str + i, l);
            o->utf8buflen += l;
            i += l;
            e = mbrtowc_utf8_to_wchar (&c, o->utf8buf, o->utf8buflen, NULL);
            if (e == -2) {
                break;      /* waiting for more chars to complete the encoding */
            } else if (e <= 0 || c > FONT_LAST_UNICHAR) {
                c = o->utf8buf[0];  /* write through bogus char whatever it is */
                memmove (o->utf8buf, o->utf8buf + 1, o->utf8buflen - 1);
                o->utf8buflen -= 1;
            } else {

/* handle a double-wide char trying to write across the right border: */
                if (is_unicode_doublewidth_char (c) && o->screen.cur.col == (last_col - 1)) {
                    o->screen.tlen[row] = (last_col - 1);
                    if (o->screen.flags & Screen_Autowrap)
                        o->screen.flags |= Screen_WrapNext;
                    else
                        o->screen.flags &= ~Screen_WrapNext;

                    /* this block of code is the same as below */
                    if (o->screen.flags & Screen_WrapNext) {
                        o->screen.tlen[row] = -1;
                        if (o->screen.cur.row == o->screen.bscroll) {
                            rxvtlib_scroll_text (o, o->screen.tscroll, o->screen.bscroll, 1, 0);
                            j = o->screen.bscroll + o->TermWin.saveLines;
                            rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, j, o->rstyle);
                            o->screen.tlen[j] = 0;
                        } else if (o->screen.cur.row < (o->TermWin.nrow - 1))
                            row = (++o->screen.cur.row) + o->TermWin.saveLines;
                        stp = o->screen.text[row];	/* _must_ refresh */
                        srp = o->screen.rend[row];	/* _must_ refresh */
                        o->screen.cur.col = 0;
                        o->screen.flags &= ~Screen_WrapNext;
                        continue;
                    }
                }

                memmove (o->utf8buf, o->utf8buf + e, o->utf8buflen - e);
                o->utf8buflen -= e;
            }
        }
#else
	c = str[i++];
#endif
	switch (c) {
	case '\t':
	    rxvtlib_scr_tab (o, 1);
	    continue;
	case '\n':
	    if (o->screen.tlen[row] != -1)	/* XXX: think about this */
		MAX_IT (o->screen.tlen[row], o->screen.cur.col);
	    o->screen.flags &= ~Screen_WrapNext;
	    if (o->screen.cur.row == o->screen.bscroll) {
		rxvtlib_scroll_text (o, o->screen.tscroll, o->screen.bscroll, 1, 0);
		j = o->screen.bscroll + o->TermWin.saveLines;
		rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, j, o->rstyle);
		o->screen.tlen[j] = 0;
	    } else if (o->screen.cur.row < (o->TermWin.nrow - 1))
		row = (++o->screen.cur.row) + o->TermWin.saveLines;
	    stp = o->screen.text[row];	/* _must_ refresh */
	    srp = o->screen.rend[row];	/* _must_ refresh */
	    RESET_CHSTAT;
	    continue;
	case '\r':
	    if (o->screen.tlen[row] != -1)	/* XXX: think about this */
		MAX_IT (o->screen.tlen[row], o->screen.cur.col);
	    o->screen.flags &= ~Screen_WrapNext;
	    o->screen.cur.col = 0;
	    RESET_CHSTAT;
	    continue;
	default:
#ifdef MULTICHAR_SET
	    o->rstyle &= ~RS_multiMask;
	    if (o->chstat == WBYTE) {
		o->rstyle |= RS_multi2;	/* multibyte 2nd byte */
		o->chstat = SBYTE;
		if ((o->encoding_method == EUCJ) || (o->encoding_method == GB))
		    c |= 0x80;	/* maybe overkill, but makes it selectable */
	    } else if (o->chstat == SBYTE) {
		if (o->multi_byte || (c & 0x80)) {	/* multibyte 1st byte */
		    o->rstyle |= RS_multi1;
		    o->chstat = WBYTE;
		    if ((o->encoding_method == EUCJ) || (o->encoding_method == GB))
			c |= 0x80;	/* maybe overkill, but makes selectable */
		}
	    } else
#endif
	    if (c == 127)
		continue;	/* yummmm..... */
	    break;
	}

	if (checksel && !ROWCOL_IS_BEFORE (o->screen.cur, o->selection.beg)
	    && ROWCOL_IS_BEFORE (o->screen.cur, o->selection.end)) {
	    checksel = 0;
	    clearsel = 1;
	}
	if (o->screen.flags & Screen_WrapNext) {
	    o->screen.tlen[row] = -1;
	    if (o->screen.cur.row == o->screen.bscroll) {
		rxvtlib_scroll_text (o, o->screen.tscroll, o->screen.bscroll, 1, 0);
		j = o->screen.bscroll + o->TermWin.saveLines;
		rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, j, o->rstyle);
		o->screen.tlen[j] = 0;
	    } else if (o->screen.cur.row < (o->TermWin.nrow - 1))
		row = (++o->screen.cur.row) + o->TermWin.saveLines;
	    stp = o->screen.text[row];	/* _must_ refresh */
	    srp = o->screen.rend[row];	/* _must_ refresh */
	    o->screen.cur.col = 0;
	    o->screen.flags &= ~Screen_WrapNext;
	}
	if (o->screen.flags & Screen_Insert)
	    rxvtlib_scr_insdel_chars (o, 1, INSERT);
#ifdef MULTICHAR_SET
	if ((o->rstyle & RS_multiMask) == RS_multi1
	    && o->screen.cur.col > 0
	    && (srp[o->screen.cur.col - 1] & RS_multiMask) == RS_multi1) {
	    stp[o->screen.cur.col - 1] = ' ';
	    srp[o->screen.cur.col - 1] &= ~RS_multiMask;
	} else if ((o->rstyle & RS_multiMask) == RS_multi2
		   && o->screen.cur.col < (last_col - 1)
		   && (srp[o->screen.cur.col + 1] & RS_multiMask) == RS_multi2) {
	    stp[o->screen.cur.col + 1] = ' ';
	    srp[o->screen.cur.col + 1] &= ~RS_multiMask;
	}
#endif

#ifdef UTF8_FONT
        if (is_unicode_doublewidth_char (c)) {
	    stp[o->screen.cur.col] = char_to_text_t (c);
	    srp[o->screen.cur.col] = o->rstyle;
	    if (o->screen.cur.col < (last_col - 1)) {
                o->screen.cur.col++;
	        stp[o->screen.cur.col] = char_to_text_t (ZERO_WIDTH_EMPTY_CHAR);
	        srp[o->screen.cur.col] = o->rstyle;
            }
        } else
#endif
        {
	    stp[o->screen.cur.col] = char_to_text_t (c);
	    srp[o->screen.cur.col] = o->rstyle;
        }
	if (o->screen.cur.col < (last_col - 1))
	    o->screen.cur.col++;
	else {
	    o->screen.tlen[row] = last_col;
	    if (o->screen.flags & Screen_Autowrap)
		o->screen.flags |= Screen_WrapNext;
	    else
		o->screen.flags &= ~Screen_WrapNext;
	}
    }
    if (o->screen.tlen[row] != -1)	/* XXX: think about this */
	MAX_IT (o->screen.tlen[row], o->screen.cur.col);

/*
 * If we wrote anywhere in the selected area, kill the selection
 * XXX: should we kill the mark too?  Possibly, but maybe that 
 *      should be a similar check.
 */
    if (clearsel)
	CLEAR_SELECTION;

#ifdef DEBUG_STRICT
    assert (o->screen.cur.row >= 0);
#else				/* drive with your eyes closed */
    MAX_IT (o->screen.cur.row, 0);
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Process Backspace.  Move back the cursor back a position, wrap if have to
 * XTERM_SEQ: CTRL-H
 */
/* EXTPROTO */
void            rxvtlib_scr_backspace (rxvtlib *o)
{E_
    RESET_CHSTAT;
    o->want_refresh = 1;
    if (o->screen.cur.col == 0) {
	if (o->screen.cur.row > 0) {
#ifdef SUPPORT_BROKEN_APPS_WHICH_RELY_ON_UNDEFINED_BW_BEHAVIOUR_AS_XTERM
	    o->screen.flags &= ~Screen_WrapNext;
#else
	    o->screen.cur.col = o->TermWin.ncol - 1;
	    o->screen.cur.row--;
#endif
	} else
	    o->screen.flags &= ~Screen_WrapNext;
    } else if (o->screen.flags & Screen_WrapNext) {
	o->screen.flags &= ~Screen_WrapNext;
    } else
	rxvtlib_scr_gotorc (o, 0, -1, RELATIVE);
}

/* ------------------------------------------------------------------------- */
/*
 * Process Horizontal Tab
 * count: +ve = forward; -ve = backwards
 * XTERM_SEQ: CTRL-I
 */
/* EXTPROTO */
void            rxvtlib_scr_tab (rxvtlib *o, int count)
{E_
    int             i, x;

    o->want_refresh = 1;
    RESET_CHSTAT;
    x = o->screen.cur.col;
    if (count == 0)
	return;
    else if (count > 0) {
	for (i = x + 1; i < o->TermWin.ncol; i++) {
	    if (o->tabs[i]) {
		x = i;
		if (!--count)
		    break;
	    }
	}
	if (count)
	    x = o->TermWin.ncol - 1;
    } else if (count < 0) {
	for (i = x - 1; i >= 0; i--) {
	    if (o->tabs[i]) {
		x = i;
		if (!++count)
		    break;
	    }
	}
	if (count)
	    x = 0;
    }
    if (x != o->screen.cur.col)
	rxvtlib_scr_gotorc (o, 0, x, R_RELATIVE);
}
/* ------------------------------------------------------------------------- */
/*
 * Process DEC Back Index
 * XTERM_SEQ: ESC 6
 * Move cursor left in row.  If we're at the left boundary, shift everything
 * in that row right.  Clear left column.
 */
#ifndef NO_FRILLS
/* EXTPROTO */
void            rxvtlib_scr_backindex (rxvtlib *o)
{E_
    int             i, row;
    text_t         *t0;
    rend_t         *r0;

    o->want_refresh = 1;
    if (o->screen.cur.col > 0)
	rxvtlib_scr_gotorc (o, 0, o->screen.cur.col - 1, R_RELATIVE);
    else {
	row = o->screen.cur.row + o->TermWin.saveLines;
	if (o->screen.tlen[row] == 0)
	    return;		/* um, yeah? */
	else if (o->screen.tlen[row] < o->TermWin.ncol - 1)
	    o->screen.tlen[row]++;
	t0 = o->screen.text[row];
	r0 = o->screen.rend[row];
	for (i = o->TermWin.ncol; i-- > 1;) {
	    t0[i] = t0[i - 1];
	    r0[i] = r0[i - 1];
	}
	t0[0] = char_to_text_t (' ');
	r0[0] = DEFAULT_RSTYLE;
/* TODO: Multi check on last character */
    }
}
#endif
/* ------------------------------------------------------------------------- */
/*
 * Process DEC Forward Index
 * XTERM_SEQ: ESC 9
 * Move cursor right in row.  If we're at the right boundary, shift everything
 * in that row left.  Clear right column.
 */
#ifndef NO_FRILLS
/* EXTPROTO */
void            rxvtlib_scr_forwardindex (rxvtlib *o)
{E_
    int             i, row;
    text_t         *t0;
    rend_t         *r0;

    o->want_refresh = 1;
    if (o->screen.cur.col < o->TermWin.ncol - 1)
	rxvtlib_scr_gotorc (o, 0, o->screen.cur.col + 1, R_RELATIVE);
    else {
	row = o->screen.cur.row + o->TermWin.saveLines;
	if (o->screen.tlen[row] == 0)
	    return;		/* um, yeah? */
	else if (o->screen.tlen[row] > 0)
	    o->screen.tlen[row]--;
	else
	    o->screen.tlen[row] = o->TermWin.ncol - 1;
	t0 = o->screen.text[row];
	r0 = o->screen.rend[row];
	for (i = 0; i < o->TermWin.ncol - 2; i++) {
	    t0[i] = t0[i + 1];
	    r0[i] = r0[i + 1];
	}
	t0[i] = char_to_text_t (' '); 
	r0[i] = DEFAULT_RSTYLE;
/* TODO: Multi check on first character */
    }
}
#endif

/* ------------------------------------------------------------------------- */
/*
 * Goto Row/Column
 */
/* EXTPROTO */
void            rxvtlib_scr_gotorc (rxvtlib *o, int row, int col, int relative)
{E_
    o->want_refresh = 1;
    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    if (rxvtlib_Gr_Displayed (o))
	rxvtlib_Gr_scroll (o, 0);

    D_SCREEN (
	      (stderr, "scr_gotorc(r:%d,c:%d,%d): from (r:%d,c:%d)", row, col,
	       relative, o->screen.cur.row, o->screen.cur.col));

    o->screen.cur.col = ((relative & C_RELATIVE) ? (o->screen.cur.col + col) : col);
    MAX_IT (o->screen.cur.col, 0);
    MIN_IT (o->screen.cur.col, o->TermWin.ncol - 1);

    if (o->screen.flags & Screen_WrapNext)
	o->screen.flags &= ~Screen_WrapNext;
    if (relative & R_RELATIVE) {
	if (row > 0) {
	    if (o->screen.cur.row <= o->screen.bscroll
		&& (o->screen.cur.row + row) > o->screen.bscroll)
		    o->screen.cur.row = o->screen.bscroll;
	    else
		o->screen.cur.row += row;
	} else if (row < 0) {
	    if (o->screen.cur.row >= o->screen.tscroll
		&& (o->screen.cur.row + row) < o->screen.tscroll)
		    o->screen.cur.row = o->screen.tscroll;
	    else
		o->screen.cur.row += row;
	}
    } else {
	if (o->screen.flags & Screen_Relative) {	/* relative origin mode */
	    o->screen.cur.row = row + o->screen.tscroll;
	    MIN_IT (o->screen.cur.row, o->screen.bscroll);
	} else
	    o->screen.cur.row = row;
    }
    MAX_IT (o->screen.cur.row, 0);
    MIN_IT (o->screen.cur.row, o->TermWin.nrow - 1);
}

/* ------------------------------------------------------------------------- */
/*
 * direction  should be UP or DN
 */
/* EXTPROTO */
void            rxvtlib_scr_index (rxvtlib *o, int direction)
{E_
    int             dirn;

    o->want_refresh = 1;
    dirn = ((direction == UP) ? 1 : -1);
    D_SCREEN ((stderr, "scr_index(%d)", dirn));

    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    if (rxvtlib_Gr_Displayed (o))
	rxvtlib_Gr_scroll (o, 0);

    if (o->screen.flags & Screen_WrapNext) {
	o->screen.flags &= ~Screen_WrapNext;
    }
    if ((o->screen.cur.row == o->screen.bscroll && direction == UP)
	|| (o->screen.cur.row == o->screen.tscroll && direction == DN)) {
	rxvtlib_scroll_text (o, o->screen.tscroll, o->screen.bscroll, dirn, 0);
	if (direction == UP)
	    dirn = o->screen.bscroll + o->TermWin.saveLines;
	else
	    dirn = o->screen.tscroll + o->TermWin.saveLines;
	rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, dirn, o->rstyle);
	o->screen.tlen[dirn] = 0;
    } else
	o->screen.cur.row += dirn;
    MAX_IT (o->screen.cur.row, 0);
    MIN_IT (o->screen.cur.row, o->TermWin.nrow - 1);
    CHECK_SELECTION (0);
}

/* ------------------------------------------------------------------------- */
/*
 * Erase part or whole of a line
 * XTERM_SEQ: Clear line to right: ESC [ 0 K
 * XTERM_SEQ: Clear line to left : ESC [ 1 K
 * XTERM_SEQ: Clear whole line   : ESC [ 2 K
 */
/* EXTPROTO */
void            rxvtlib_scr_erase_line (rxvtlib *o, int mode)
{E_
    int             row, col, num;

    o->want_refresh = 1;
    D_SCREEN (
	      (stderr, "scr_erase_line(%d) at screen row: %d", mode,
	       o->screen.cur.row));
    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    if (rxvtlib_Gr_Displayed (o))
	rxvtlib_Gr_scroll (o, 0);
    CHECK_SELECTION (1);

    if (o->screen.flags & Screen_WrapNext)
	o->screen.flags &= ~Screen_WrapNext;

    row = o->TermWin.saveLines + o->screen.cur.row;
    switch (mode) {
    case 0:			/* erase to end of line */
	col = o->screen.cur.col;
	num = o->TermWin.ncol - col;
	MIN_IT (o->screen.tlen[row], col);
	if (ROWCOL_IN_ROW_AT_OR_AFTER (o->selection.beg, o->screen.cur)
	    || ROWCOL_IN_ROW_AT_OR_AFTER (o->selection.end, o->screen.cur))
	    CLEAR_SELECTION;
	break;
    case 1:			/* erase to beginning of line */
	col = 0;
	num = o->screen.cur.col + 1;
	if (ROWCOL_IN_ROW_AT_OR_BEFORE (o->selection.beg, o->screen.cur)
	    || ROWCOL_IN_ROW_AT_OR_BEFORE (o->selection.end, o->screen.cur))
	    CLEAR_SELECTION;
	break;
    case 2:			/* erase whole line */
	col = 0;
	num = o->TermWin.ncol;
	o->screen.tlen[row] = 0;
	if (o->selection.beg.row <= o->screen.cur.row
	    && o->selection.end.row >= o->screen.cur.row) CLEAR_SELECTION;
	break;
    default:
	return;
    }
    if (o->screen.text[row])
	blank_line (&(o->screen.text[row][col]), &(o->screen.rend[row][col]), num,
		    o->rstyle & ~RS_Uline);
    else
	rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, row, o->rstyle & ~RS_Uline);
}

/* ------------------------------------------------------------------------- */
/*
 * Erase part of whole of the screen
 * XTERM_SEQ: Clear screen after cursor : ESC [ 0 J
 * XTERM_SEQ: Clear screen before cursor: ESC [ 1 J
 * XTERM_SEQ: Clear whole screen        : ESC [ 2 J
 */
/* EXTPROTO */
void            rxvtlib_scr_erase_screen (rxvtlib *o, int mode)
{E_
    int             row, num, row_offset;
    rend_t          ren;
    long            gcmask;
    XGCValues       gcvalue;

    o->want_refresh = 1;
    D_SCREEN (
	      (stderr, "scr_erase_screen(%d) at screen row: %d", mode,
	       o->screen.cur.row));
    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    row_offset = o->TermWin.saveLines;

    switch (mode) {
    case 0:			/* erase to end of screen */
	CHECK_SELECTION (1);
	rxvtlib_scr_erase_line (o, 0);
	row = o->screen.cur.row + 1;	/* possible OOB */
	num = o->TermWin.nrow - row;
	break;
    case 1:			/* erase to beginning of screen */
	CHECK_SELECTION (3);
	rxvtlib_scr_erase_line (o, 1);
	row = 0;		/* possible OOB */
	num = o->screen.cur.row;
	break;
    case 2:			/* erase whole screen */
	CHECK_SELECTION (3);
	rxvtlib_Gr_ClearScreen (o);
	row = 0;
	num = o->TermWin.nrow;
	break;
    default:
	return;
    }
    if (o->selection.op && o->current_screen == o->selection.screen
	&& ((o->selection.beg.row >= row && o->selection.beg.row <= row + num)
	    || (o->selection.end.row >= row && o->selection.end.row <= row + num)))
	CLEAR_SELECTION;
    if (row >= 0 && row < o->TermWin.nrow) {	/* check OOB */
	MIN_IT (num, (o->TermWin.nrow - row));
	if (o->rstyle & (RS_RVid | RS_Uline))
	    ren = (rend_t) ~ RS_None;
	else if (GET_BGCOLOR (o->rstyle) == Color_bg) {
	    ren = DEFAULT_RSTYLE;
	    CLEAR_ROWS (row, num);
	} else {
	    ren = (o->rstyle & (RS_fgMask | RS_bgMask));
	    gcvalue.foreground = o->PixColors[GET_BGCOLOR (ren)];
	    gcmask = GCForeground;
	    XChangeGC (o->Xdisplay, o->TermWin.gc, gcmask, &gcvalue);
	    ERASE_ROWS (row, num);
	    gcvalue.foreground = o->PixColors[Color_fg];
	    XChangeGC (o->Xdisplay, o->TermWin.gc, gcmask, &gcvalue);
	}
	for (; num--; row++) {
	    rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, row + row_offset,
			      o->rstyle & ~RS_Uline);
	    o->screen.tlen[row + row_offset] = 0;
	    blank_line (o->drawn_text[row], o->drawn_rend[row], o->TermWin.ncol, ren);
	}
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Fill the screen with `E's
 * XTERM_SEQ: Screen Alignment Test: ESC # 8
 */
/* EXTPROTO */
void            rxvtlib_scr_E (rxvtlib *o)
{E_
    int             i, j;
    text_t         *t;
    rend_t         *r, fs;

    o->want_refresh = 1;
    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    CHECK_SELECTION (3);

    fs = o->rstyle;
    for (i = o->TermWin.saveLines; i < o->TermWin.nrow + o->TermWin.saveLines; i++) {
	t = o->screen.text[i];
	r = o->screen.rend[i];
	for (j = 0; j < o->TermWin.ncol; j++) {
	    *t++ = char_to_text_t ('E');
	    *r++ = fs;
	}
	o->screen.tlen[i] = o->TermWin.ncol;	/* make the `E's selectable */
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Insert/Delete <count> lines
 */
/* EXTPROTO */
void            rxvtlib_scr_insdel_lines (rxvtlib *o, int count, int insdel)
{E_
    int             end;

    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    if (rxvtlib_Gr_Displayed (o))
	rxvtlib_Gr_scroll (o, 0);
    CHECK_SELECTION (1);

    if (o->screen.cur.row > o->screen.bscroll)
	return;

    end = o->screen.bscroll - o->screen.cur.row + 1;
    if (count > end) {
	if (insdel == DELETE)
	    return;
	else if (insdel == INSERT)
	    count = end;
    }
    if (o->screen.flags & Screen_WrapNext)
	o->screen.flags &= ~Screen_WrapNext;

    rxvtlib_scroll_text (o, o->screen.cur.row, o->screen.bscroll, insdel * count, 0);

/* fill the inserted or new lines with rstyle. TODO: correct for delete? */
    if (insdel == DELETE)
	end = o->screen.bscroll + o->TermWin.saveLines;
    else if (insdel == INSERT)
	end = o->screen.cur.row + count - 1 + o->TermWin.saveLines;
    for (; count--;) {
	rxvtlib_blank_screen_mem (o, o->screen.text, o->screen.rend, end, o->rstyle);
	o->screen.tlen[end--] = 0;
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Insert/Delete <count> characters from the current position
 */
/* EXTPROTO */
void            rxvtlib_scr_insdel_chars (rxvtlib *o, int count, int insdel)
{E_
    int             col, row;
    rend_t          tr;

    o->want_refresh = 1;
    ZERO_SCROLLBACK;
#if 0
    RESET_CHSTAT;
#endif
    if (rxvtlib_Gr_Displayed (o))
	rxvtlib_Gr_scroll (o, 0);

    if (count <= 0)
	return;

    CHECK_SELECTION (1);
    MIN_IT (count, (o->TermWin.ncol - o->screen.cur.col));

    row = o->screen.cur.row + o->TermWin.saveLines;
    o->screen.flags &= ~Screen_WrapNext;

    switch (insdel) {
    case INSERT:
	for (col = o->TermWin.ncol - 1; (col - count) >= o->screen.cur.col; col--) {
	    o->screen.text[row][col] = o->screen.text[row][col - count];
	    o->screen.rend[row][col] = o->screen.rend[row][col - count];
	}
	if (o->screen.tlen[row] != -1) {
	    o->screen.tlen[row] += count;
	    MIN_IT (o->screen.tlen[row], o->TermWin.ncol);
	}
	if (o->selection.op && o->current_screen == o->selection.screen
	    && ROWCOL_IN_ROW_AT_OR_AFTER (o->selection.beg, o->screen.cur)) {
	    if (o->selection.end.row != o->screen.cur.row
		|| (o->selection.end.col + count >= o->TermWin.ncol))
		CLEAR_SELECTION;
	    else {		/* shift selection */
		o->selection.beg.col += count;
		o->selection.mark.col += count;	/* XXX: yes? */
		o->selection.end.col += count;
	    }
	}
	blank_line (&(o->screen.text[row][o->screen.cur.col]),
		    &(o->screen.rend[row][o->screen.cur.col]), count, o->rstyle);
	break;
    case ERASE:
	o->screen.cur.col += count;	/* don't worry if > TermWin.ncol */
	CHECK_SELECTION (1);
	o->screen.cur.col -= count;
	blank_line (&(o->screen.text[row][o->screen.cur.col]),
		    &(o->screen.rend[row][o->screen.cur.col]), count, o->rstyle);
	break;
    case DELETE:
	tr = o->screen.rend[row][o->TermWin.ncol - 1]
	    & (RS_fgMask | RS_bgMask | RS_baseattrMask);
	for (col = o->screen.cur.col; (col + count) < o->TermWin.ncol; col++) {
	    o->screen.text[row][col] = o->screen.text[row][col + count];
	    o->screen.rend[row][col] = o->screen.rend[row][col + count];
	}
	blank_line (&(o->screen.text[row][o->TermWin.ncol - count]),
		    &(o->screen.rend[row][o->TermWin.ncol - count]), count, tr);
	if (o->screen.tlen[row] == -1)	/* break line continuation */
	    o->screen.tlen[row] = o->TermWin.ncol;
	o->screen.tlen[row] -= count;
	MAX_IT (o->screen.tlen[row], 0);
	if (o->selection.op && o->current_screen == o->selection.screen
	    && ROWCOL_IN_ROW_AT_OR_AFTER (o->selection.beg, o->screen.cur)) {
	    if (o->selection.end.row != o->screen.cur.row
		|| (o->screen.cur.col >= o->selection.beg.col - count)
		|| o->selection.end.col >= o->TermWin.ncol)
		CLEAR_SELECTION;
	    else {
		/* shift selection */
		o->selection.beg.col -= count;
		o->selection.mark.col -= count;	/* XXX: yes? */
		o->selection.end.col -= count;
	    }
	}
	break;
    }
#if 0
    if ((o->screen.rend[row][0] & RS_multiMask) == RS_multi2) {
	o->screen.rend[row][0] &= ~RS_multiMask;
	o->screen.text[row][0] = ' ';
    }
    if ((o->screen.rend[row][o->TermWin.ncol - 1] & RS_multiMask) == RS_multi1) {
	o->screen.rend[row][o->TermWin.ncol - 1] &= ~RS_multiMask;
	o->screen.text[row][o->TermWin.ncol - 1] = ' ';
    }
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Set the scrolling region
 * XTERM_SEQ: Set region <top> - <bot> inclusive: ESC [ <top> ; <bot> r
 */
/* EXTPROTO */
void            rxvtlib_scr_scroll_region (rxvtlib *o, int top, int bot)
{E_
    MAX_IT (top, 0);
    MIN_IT (bot, o->TermWin.nrow - 1);
    if (top > bot)
	return;
    o->screen.tscroll = top;
    o->screen.bscroll = bot;
    rxvtlib_scr_gotorc (o, 0, 0, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Make the cursor visible/invisible
 * XTERM_SEQ: Make cursor visible  : ESC [ ? 25 h
 * XTERM_SEQ: Make cursor invisible: ESC [ ? 25 l
 */
/* EXTPROTO */
void            rxvtlib_scr_cursor_visible (rxvtlib *o, int mode)
{E_
    o->want_refresh = 1;
    if (mode)
	o->screen.flags |= Screen_VisibleCursor;
    else
	o->screen.flags &= ~Screen_VisibleCursor;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/unset automatic wrapping
 * XTERM_SEQ: Set Wraparound  : ESC [ ? 7 h
 * XTERM_SEQ: Unset Wraparound: ESC [ ? 7 l
 */
/* EXTPROTO */
void            rxvtlib_scr_autowrap (rxvtlib *o, int mode)
{E_
    if (mode)
	o->screen.flags |= Screen_Autowrap;
    else
	o->screen.flags &= ~Screen_Autowrap;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/unset margin origin mode
 * Absolute mode: line numbers are counted relative to top margin of screen
 *      and the cursor can be moved outside the scrolling region.
 * Relative mode: line numbers are relative to top margin of scrolling region
 *      and the cursor cannot be moved outside.
 * XTERM_SEQ: Set Absolute: ESC [ ? 6 h
 * XTERM_SEQ: Set Relative: ESC [ ? 6 l
 */
/* EXTPROTO */
void            rxvtlib_scr_relative_origin (rxvtlib *o, int mode)
{E_
    if (mode)
	o->screen.flags |= Screen_Relative;
    else
	o->screen.flags &= ~Screen_Relative;
    rxvtlib_scr_gotorc (o, 0, 0, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Set insert/replace mode
 * XTERM_SEQ: Set Insert mode : ESC [ ? 4 h
 * XTERM_SEQ: Set Replace mode: ESC [ ? 4 l
 */
/* EXTPROTO */
void            rxvtlib_scr_insert_mode (rxvtlib *o, int mode)
{E_
    if (mode)
	o->screen.flags |= Screen_Insert;
    else
	o->screen.flags &= ~Screen_Insert;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/Unset tabs
 * XTERM_SEQ: Set tab at current column  : ESC H
 * XTERM_SEQ: Clear tab at current column: ESC [ 0 g
 * XTERM_SEQ: Clear all tabs             : ESC [ 3 g
 */
/* EXTPROTO */
void            rxvtlib_scr_set_tab (rxvtlib *o, int mode)
{E_
    if (mode < 0)
	MEMSET (o->tabs, 0, o->TermWin.ncol * sizeof (char));

    else if (o->screen.cur.col < o->TermWin.ncol)
	o->tabs[o->screen.cur.col] = (mode ? 1 : 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Set reverse/normal video
 * XTERM_SEQ: Reverse video: ESC [ ? 5 h
 * XTERM_SEQ: Normal video : ESC [ ? 5 l
 */
/* EXTPROTO */
void            rxvtlib_scr_rvideo_mode (rxvtlib *o, int mode)
{E_
    int             i, j;
    rend_t         *r;

    if (o->rvideo != mode) {
	o->rvideo = mode;
	o->rstyle ^= RS_RVid;

	for (i = 0; i < o->TermWin.nrow; i++) {
	    r = o->screen.rend[o->TermWin.saveLines + i];
	    for (j = 0; j < o->TermWin.ncol; j++)
		*r++ ^= RS_RVid;
	}
	rxvtlib_scr_refresh (o, SLOW_REFRESH);
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Report current cursor position
 * XTERM_SEQ: Report position: ESC [ 6 n
 */
/* EXTPROTO */
void            rxvtlib_scr_report_position (rxvtlib *o)
{E_
    rxvtlib_tt_printf (o, "\033[%d;%dR", o->screen.cur.row + 1, o->screen.cur.col + 1);
}

/* ------------------------------------------------------------------------- *
 *                                  FONTS                                    * 
 * ------------------------------------------------------------------------- */

/*
 * Set font style
 */
/* INTPROTO */
void            rxvtlib_set_font_style (rxvtlib *o)
{E_
    o->rstyle &= ~RS_fontMask;
    switch (o->charsets[o->screen.charset]) {
    case '0':			/* DEC Special Character & Line Drawing Set */
	o->rstyle |= RS_acsFont;
	break;
    case 'A':			/* United Kingdom (UK) */
	o->rstyle |= RS_ukFont;
	break;
    case 'B':			/* United States (USASCII) */
	break;
    case '<':			/* Multinational character set */
	break;
    case '5':			/* Finnish character set */
	break;
    case 'C':			/* Finnish character set */
	break;
    case 'K':			/* German character set */
	break;
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Choose a font
 * XTERM_SEQ: Invoke G0 character set: CTRL-O
 * XTERM_SEQ: Invoke G1 character set: CTRL-N
 * XTERM_SEQ: Invoke G2 character set: ESC N
 * XTERM_SEQ: Invoke G3 character set: ESC O
 */
/* EXTPROTO */
void            rxvtlib_scr_charset_choose (rxvtlib *o, int set)
{E_
    o->screen.charset = set;
    rxvtlib_set_font_style (o);
}

/* ------------------------------------------------------------------------- */
/*
 * Set a font
 * XTERM_SEQ: Set G0 character set: ESC ( <C>
 * XTERM_SEQ: Set G1 character set: ESC ) <C>
 * XTERM_SEQ: Set G2 character set: ESC * <C>
 * XTERM_SEQ: Set G3 character set: ESC + <C>
 * See set_font_style for possible values for <C>
 */
/* EXTPROTO */
void            rxvtlib_scr_charset_set (rxvtlib *o, int set, unsigned int ch)
{E_
#ifdef MULTICHAR_SET
    o->multi_byte = (set < 0);
    set = abs (set);
#endif
    o->charsets[set] = (unsigned char)ch;
    rxvtlib_set_font_style (o);
}

/* ------------------------------------------------------------------------- *
 *          MULTIPLE-CHARACTER FONT SET MANIPULATION FUNCTIONS               * 
 * ------------------------------------------------------------------------- */
#ifdef MULTICHAR_SET

/* INTPROTO */
void            eucj2jis (unsigned char *str, int len)
{E_
    register int    i;

    for (i = 0; i < len; i++)
	str[i] &= 0x7F;
}

/* ------------------------------------------------------------------------- */
/* INTPROTO */
void            sjis2jis (unsigned char *str, int len)
{E_
    register int    i;
    unsigned char  *high, *low;

    for (i = 0; i < len; i += 2, str += 2) {
	high = str;
	low = str + 1;
	(*high) -= (*high > 0x9F ? 0xB1 : 0x71);
	*high = (*high) * 2 + 1;
	if (*low > 0x9E) {
	    *low -= 0x7E;
	    (*high)++;
	} else {
	    if (*low > 0x7E)
		(*low)--;
	    *low -= 0x1F;
	}
    }
}

/* INTPROTO */
void            big5dummy (unsigned char *str, int len)
{E_
}

/* INTPROTO */
void            gb2jis (unsigned char *str, int len)
{E_
    register int    i;

    for (i = 0; i < len; i++)
	str[i] &= 0x7F;
}

/* EXTPROTO */
void            rxvtlib_set_multichar_encoding (rxvtlib *o, const char *str)
{E_
    if (str && *str) {
	if (!Cstrcasecmp (str, "sjis")) {
	    o->encoding_method = SJIS;	/* Kanji SJIS */
	    o->multichar_decode = sjis2jis;
	} else if (!Cstrcasecmp (str, "eucj")) {
	    o->encoding_method = EUCJ;	/* Kanji EUCJ */
	    o->multichar_decode = eucj2jis;
	}
    }
}
#endif				/* MULTICHAR_SET */

/* ------------------------------------------------------------------------- *
 *                           GRAPHICS COLOURS                                * 
 * ------------------------------------------------------------------------- */

#ifdef RXVT_GRAPHICS
/* EXTPROTO */
int             rxvtlib_scr_get_fgcolor (rxvtlib *o)
{E_
    return GET_FGCOLOR (o->rstyle);
}

/* ------------------------------------------------------------------------- */
/* EXTPROTO */
int             rxvtlib_scr_get_bgcolor (rxvtlib *o)
{E_
    return GET_BGCOLOR (o->rstyle);
}
#endif

/* ------------------------------------------------------------------------- *
 *                        MAJOR SCREEN MANIPULATION                          * 
 * ------------------------------------------------------------------------- */

/*
 * Refresh an area
 */
enum {
    PART_BEG = 0,
    PART_END,
#if defined(XPM_BACKGROUND) && defined(XPM_BUFFERING)
    FULL_BEG,
    FULL_END,
#endif
    RC_COUNT
};
/* EXTPROTO */
void            rxvtlib_scr_expose (rxvtlib *o, int x, int y, int width, int height)
{E_
    int             i;

    row_col_t       rc[RC_COUNT];

    if (o->drawn_text == NULL)	/* sanity check */
	return;

/* round down */
    rc[PART_BEG].col = Pixel2Col(x);
    rc[PART_BEG].row = Pixel2Row(y);

/* round up */
    rc[PART_END].col = Pixel2Width(x + width + o->TermWin.fwidth - 1);
    rc[PART_END].row = Pixel2Row(y + height + o->TermWin.fheight - 1);
 
#if defined(XPM_BACKGROUND) && defined(XPM_BUFFERING)
/* round down */
    rc[FULL_END].col = Pixel2Width(x + width);
    rc[FULL_END].row = Pixel2Row(y + height);
/* round up */
    rc[FULL_BEG].col = Pixel2Col(x + o->TermWin.fwidth - 1);
    rc[FULL_BEG].row = Pixel2Row(y + o->TermWin.fheight - 1);
#endif

/* sanity checks */
    for (i = PART_BEG; i < RC_COUNT; i++) {
	MAX_IT(rc[i].col, 0);
	MAX_IT(rc[i].row, 0);
	MIN_IT(rc[i].col, o->TermWin.ncol - 1);
	MIN_IT(rc[i].row, o->TermWin.nrow - 1);
    }

    D_SCREEN((stderr, "scr_expose(x:%d, y:%d, w:%d, h:%d) area (c:%d,r:%d)-(c:%d,r:%d)", x, y, width, height, rc[PART_BEG}.col, rc[PART_BEG].row, rc[PART_END].col, rc[PART_END].row));

#if defined(XPM_BACKGROUND) && defined(XPM_BUFFERING)
/* supposedly we're exposed - so `clear' the fully exposed clear areas */
    x = Col2Pixel(rc[FULL_BEG].col);
    y = Row2Pixel(rc[FULL_BEG].row);
    width = Width2Pixel(rc[FULL_END].col - rc[FULL_BEG].col + 1);
    height = Height2Pixel(rc[FULL_END].row - rc[FULL_BEG].row + 1);
    XCopyArea (o->Xdisplay, o->TermWin.pixmap, drawBuffer, o->TermWin.gc,
	       x, y, width, height, x, y);
#endif

    for (i = rc[PART_BEG].row; i <= rc[PART_END].row; i++)
	MEMSET(&(o->drawn_text[i][rc[PART_BEG].col]), 0,
	       (rc[PART_END].col - rc[PART_BEG].col + 1) * sizeof (text_t));

    rxvtlib_scr_refresh (o, SLOW_REFRESH);
}

/* ------------------------------------------------------------------------- */
/*
 * Refresh the entire screen
 */
/* EXTPROTO */
void            rxvtlib_scr_touch (rxvtlib *o)
{E_
    rxvtlib_scr_expose (o, 0, 0, o->TermWin.width, o->TermWin.height);
}

/* ------------------------------------------------------------------------- */
/*
 * Move the display so that the line represented by scrollbar value Y is at
 * the top of the screen
 */
/* EXTPROTO */
int             rxvtlib_scr_move_to (rxvtlib *o, int y, int len)
{E_
    int             start;

    o->want_refresh = 1;
    start = o->TermWin.view_start;
    if (y >= len)
	o->TermWin.view_start = 0;
    else {
	o->TermWin.view_start = ((len - y)
			      * (o->TermWin.nrow - 1 + o->TermWin.nscrolled) / len);
	if (o->TermWin.view_start < o->TermWin.nrow)
	    o->TermWin.view_start = 0;
	else
	    o->TermWin.view_start -= (o->TermWin.nrow - 1);
    }
    D_SCREEN (
	      (stderr, "scr_move_to(%d, %d) view_start:%d", y, len,
	       o->TermWin.view_start));

    MIN_IT (o->TermWin.view_start, o->TermWin.nscrolled);

    if (rxvtlib_Gr_Displayed (o))
	rxvtlib_Gr_scroll (o, 0);
    return (o->TermWin.view_start - start);
}
/* ------------------------------------------------------------------------- */
/*
 * Page the screen up/down nlines
 * direction  should be UP or DN
 */
/* EXTPROTO */
int             rxvtlib_scr_page (rxvtlib *o, int direction, int nlines)
{E_
    int             start;

    D_SCREEN (
	      (stderr, "scr_page(%s, %d) view_start:%d",
	       ((direction == UP) ? "UP" : "DN"), nlines, o->TermWin.view_start));

    start = o->TermWin.view_start;
    MAX_IT (nlines, 1);
    MIN_IT (nlines, o->TermWin.nrow);
    if (direction == UP)
	o->TermWin.view_start = o->TermWin.view_start < o->TermWin.nscrolled - nlines
	    ? o->TermWin.view_start + nlines : o->TermWin.nscrolled;
    else
	o->TermWin.view_start = o->TermWin.view_start > nlines
	    ? o->TermWin.view_start - nlines : 0;

    if (rxvtlib_Gr_Displayed (o))
	rxvtlib_Gr_scroll (o, 0);
    if (o->TermWin.view_start != start)
	o->want_refresh = 1;
    return (int)(o->TermWin.view_start - start);
}

/* ------------------------------------------------------------------------- */
/* EXTPROTO */
void            rxvtlib_scr_bell (rxvtlib *o)
{E_
#ifndef NO_MAPALERT
# ifdef MAPALERT_OPTION
    if (o->Options & Opt_mapAlert)
# endif
	XMapWindow (o->Xdisplay, o->TermWin.parent[0]);
#endif
    if (o->Options & Opt_visualBell) {
	rxvtlib_scr_rvideo_mode (o, !o->rvideo);	/* scr_refresh() also done */
	rxvtlib_scr_rvideo_mode (o, !o->rvideo);	/* scr_refresh() also done */
    } else
	XBell (o->Xdisplay, 0);
}

/* ------------------------------------------------------------------------- */
/* ARGSUSED */
/* EXTPROTO */
void            rxvtlib_scr_printscreen (rxvtlib *o, int fullhist)
{E_
#ifdef PRINTPIPE
    int             i, r, nrows, row_offset;
    text_t         *t;
    FILE           *fd;

    if ((fd = rxvtlib_popen_printer (o)) == NULL)
	return;
    nrows = o->TermWin.nrow;
    row_offset = o->TermWin.saveLines;
    if (!fullhist)
	row_offset -= o->TermWin.view_start;
    else {
	nrows += o->TermWin.nscrolled;
	row_offset -= o->TermWin.nscrolled;
    }

    for (r = 0; r < nrows; r++) {
	t = o->screen.text[r + row_offset];
	for (i = o->TermWin.ncol - 1; i >= 0; i--)
	    if (!isspace (t[i]))
		break;
	fprintf (fd, "%.*s\n", (i + 1), t);
    }
    pclose_printer (fd);
#endif
}

#ifdef UTF8_FONT
static int bfont;

unsigned long scale_brightness (unsigned long c, unsigned long denominator, unsigned long numerator)
{
    unsigned long ret = 0L;
    Visual *v;
    unsigned long cr, cg, cb;
    unsigned long r, g, b;
    int br, bg, bb;
    v = CVisual;
    if (v->class != TrueColor)
        return c;

#define shift(cm, m, bm, q_mask) \
    m = v->q_mask; \
    cm = m & c; \
    bm = 0; \
    while (!(m & 1) && bm < 64) { \
        m >>= 1; \
        cm >>= 1; \
        bm++; \
    } \
    cm = cm * denominator / numerator; \
    ret |= (cm << bm); \

    shift (cr, r, br, red_mask);
    shift (cg, g, bg, green_mask);
    shift (cb, b, bb, blue_mask);

    return ret;
}

static int draw_image_string_ (Display * display, Drawable d, GC gc, int x, int y, rxvt_buf_char_t *string, int length)
{
    XGCValues values_return;
    XGetGCValues (display, gc, GCForeground | GCBackground, &values_return);
    if (bfont)
        CSetColor (scale_brightness (values_return.foreground, 3, 5));
    else
        CSetColor (values_return.foreground);
    CSetBackgroundColor (values_return.background);
    return CImageTextWC (d, x, y, NULL, string, length);
}
#endif

/* ------------------------------------------------------------------------- */
/*
 * Refresh the screen
 * drawn_text/drawn_rend contain the screen information before the update.
 * screen.text/screen.rend contain what the screen will change to.
 */

/* EXTPROTO */
void            rxvtlib_scr_refresh (rxvtlib *o, int type)
{E_
    int             i, j,	/* tmp                                       */
                    col, row,	/* column/row we're processing               */
                    scrrow,	/* screen row offset                         */
                    row_offset,	/* basic offset in screen structure          */
                    currow,	/* cursor row at appropriate offset          */
                    boldlast,	/* last character in some row was bold       */
                    len, wlen,	/* text length screen/buffer                 */
                    fprop,	/* proportional font used                    */
                    rvid,	/* reverse video this position               */
                    rend,	/* rendition                                 */
                    fore, back,	/* desired foreground/background             */
                    wbyte,	/* we're in multibyte                        */
		    fontdiff,	/* current font size different to base font  */
                    morecur = 0, xpixel,	/* x offset for start of drawing (font)      */
                    ypixel,	/* y offset for start of drawing (font)      */
		    ypixelc;	/* y offset for top of drawing               */
    long            gcmask,	/* Graphics Context mask                     */
                    gcmaskf;
    static int      focus = -1;	/* screen in focus?                          */
    unsigned long   ltmp;
    rend_t          rt1, rt2;	/* tmp rend values                           */

#ifndef NO_CURSORCOLOR
    rend_t          ccol1,	/* Cursor colour                             */
                    ccol2,	/* Cursor colour2                            */
                    cc1 = 0;	/* store colours at cursor position(s)       */

# ifdef MULTICHAR_SET
    rend_t          cc2;	/* store colours at cursor position(s)       */

# endif
#endif
    rend_t         *drp, *srp;	/* drawn-rend-pointer, screen-rend-pointer   */
    text_t         *dtp, *stp;	/* drawn-text-pointer, screen-text-pointer   */
    XGCValues       gcvalue;	/* Graphics Context values                   */
#ifndef UTF8_FONT
    XFontStruct    *wf;		/* which font are we in                      */
#endif

    /* is there an old outline cursor on screen? */
#ifndef UTF8_FONT
#ifndef NO_BOLDFONT
    int             bfont;	/* we've changed font to bold font           */
#endif
#endif
    int             (*draw_string) (), (*draw_image_string) ();

    bfont = 0;

    if (type == NO_REFRESH)
	return;
    if (!o->TermWin.mapped)
	return;

/*
 * A: set up vars
 */
    if (o->currmaxcol < o->TermWin.ncol) {
	o->currmaxcol = o->TermWin.ncol;
	if (o->buffer)
	    o->buffer = (rxvt_buf_char_t *) REALLOC (o->buffer, (sizeof (rxvt_buf_char_t) * (o->currmaxcol + 1)));

	else
	    o->buffer = (rxvt_buf_char_t *) MALLOC ((sizeof (rxvt_buf_char_t) * (o->currmaxcol + 1)));
    }
    row_offset = o->TermWin.saveLines - o->TermWin.view_start;
    fprop = o->TermWin.fprop;
    gcvalue.foreground = o->PixColors[Color_fg];
    gcvalue.background = o->PixColors[Color_bg];
/*
 * always go back to the base font - it's much safer
 */
    wbyte = 0;
#ifdef UTF8_FONT
    CPushFont ("rxvt");
    draw_string = draw_image_string_;
    draw_image_string = draw_image_string_;
#else
    XSetFont (o->Xdisplay, o->TermWin.gc, o->TermWin.font->fid);
    draw_string = XDrawString;
    draw_image_string = XDrawImageString;
#endif
    boldlast = 0;

/*
 * B: reverse any characters which are selected
 */
    rxvtlib_scr_reverse_selection (o);

#ifndef NO_BOLDOVERSTRIKE
/*
 * C: Bold Overstrike pixel dropping avoidance.  Do this before the main
 *    refresh.  Do a pass across each line at the start, require a refresh of
 *    anything that will need to be refreshed due to pixels being dropped
 *    into our area by a previous character which has now been changed.
 */
    for (row = 0; row < o->TermWin.nrow; row++) {
	scrrow = row + row_offset;
	stp = o->screen.text[scrrow];
	srp = o->screen.rend[scrrow];
	dtp = o->drawn_text[row];
	drp = o->drawn_rend[row];
#ifndef UTF8_FONT
# ifndef NO_BOLDFONT
	if (o->TermWin.boldFont == NULL) {
# endif
	    wf = o->TermWin.font;
	    j = wbyte;
	    for (col = o->TermWin.ncol - 2; col >= 0; col--) {
# if ! defined (NO_BRIGHTCOLOR) && ! defined (VERYBOLD)
		fore = GET_FGCOLOR (drp[col]);
# endif
		if (!MONO_BOLD (drp[col]))
		    continue;
		if (dtp[col] == stp[col]
		    && drp[col] == srp[col])
		    continue;
		if (wbyte) {
		    ;		/* TODO: handle multibyte */
		    continue;	/* don't go past here */
		}
		if (dtp[col] == ' ') {	/* TODO: check character set? */
		    continue;
		}
#ifdef UTF8_FONT
                if (font_per_char (dtp[col]) <= 0)
#else
		if (wf->per_char == NULL
		    || dtp[col] < wf->min_char_or_byte2
		    || dtp[col] > wf->max_char_or_byte2
		    || FONT_WIDTH (wf, dtp[col]) == FONT_RBEAR (wf, dtp[col]))
#endif
                {
		    dtp[col + 1] = 0;
# if defined(MULTICHAR_SET) && ! defined(NO_BOLDOVERSTRIKE_MULTI)
		    if ((srp[col] & RS_multiMask) == RS_multi2) {
			col--;
			wbyte = 1;
			continue;
		    }
# endif
		}
	    }
# if ! defined (NO_BRIGHTCOLOR) && ! defined (VERYBOLD)
	    fore = GET_FGCOLOR (srp[o->TermWin.ncol - 1]);
# endif
	    if (MONO_BOLD (srp[o->TermWin.ncol - 1]))
		boldlast = 1;
	    wbyte = j;
# ifndef NO_BOLDFONT
	}
# endif
#endif
    }
#endif				/* ! NO_BOLDOVERSTRIKE */

/*
 * D: set the cursor character(s)
 */
    currow = o->screen.cur.row + o->TermWin.saveLines;
    if (focus != o->TermWin.focus)
	focus = o->TermWin.focus;
    if (o->screen.flags & Screen_VisibleCursor && focus) {
	srp = &(o->screen.rend[currow][o->screen.cur.col]);
	*srp ^= RS_RVid;
#ifndef NO_CURSORCOLOR
	cc1 = *srp & (RS_fgMask | RS_bgMask);
	if (o->Xdepth <= 2 || !o->rs[Rs_color + Color_cursor])
	    ccol1 = Color_fg;
	else
	    ccol1 = Color_cursor;
	if (o->Xdepth <= 2 || !o->rs[Rs_color + Color_cursor2])
	    ccol2 = Color_bg;
	else
	    ccol2 = Color_cursor2;
	*srp = SET_FGCOLOR (*srp, ccol1);
	*srp = SET_BGCOLOR (*srp, ccol2);
#endif
#ifdef MULTICHAR_SET
	rt1 = *srp & RS_multiMask;
	if (rt1 == RS_multi1) {
	    if (o->screen.cur.col < o->TermWin.ncol - 2
		&& ((srp[1] & RS_multiMask) == RS_multi2))
		morecur = 1;
	} else if (rt1 == RS_multi2) {
	    if (o->screen.cur.col > 0 && ((srp[-1] & RS_multiMask) == RS_multi1))
		morecur = -1;
	}
	if (morecur) {
	    srp += morecur;
	    *srp ^= RS_RVid;
	}
# ifndef NO_CURSORCOLOR
	if (morecur) {
	    cc2 = *srp & (RS_fgMask | RS_bgMask);
	    *srp = SET_FGCOLOR (*srp, ccol1);
	    *srp = SET_BGCOLOR (*srp, ccol2);
	}
# endif
#endif
    }
    i = 0;
    if (o->oldcursor.row != -1) {
	/* make sure no outline cursor is left around */
	if (o->screen.cur.row + o->TermWin.view_start != o->oldcursor.row
	    || o->screen.cur.col != o->oldcursor.col) {
	    if (o->oldcursor.row < o->TermWin.nrow && o->oldcursor.col < o->TermWin.ncol) {
		o->drawn_text[o->oldcursor.row][o->oldcursor.col] = char_to_text_t (0);
#ifdef MULTICHAR_SET
		if (o->oldcursormulti) {
		    col = o->oldcursor.col + o->oldcursormulti;
		    if (col < o->TermWin.ncol)
			o->drawn_text[o->oldcursor.row][col] = 0;
		}
#endif
	    }
	    if (focus || !(o->screen.flags & Screen_VisibleCursor))
		o->oldcursor.row = -1;
	    else
		i = 1;
	}
    } else if (!focus)
	i = 1;
    if (i) {
	if (o->screen.cur.row + o->TermWin.view_start >= o->TermWin.nrow)
	    o->oldcursor.row = -1;
	else {
	    o->oldcursor.row = o->screen.cur.row + o->TermWin.view_start;
	    o->oldcursor.col = o->screen.cur.col;
#ifdef MULTICHAR_SET
	    o->oldcursormulti = morecur;
#endif
	}
    }
/*
 * E: OK, now the real pass
 */
    for (row = 0; row < o->TermWin.nrow; row++) {
	scrrow = row + row_offset;
	stp = o->screen.text[scrrow];
	srp = o->screen.rend[scrrow];
	dtp = o->drawn_text[row];
	drp = o->drawn_rend[row];
	for (col = 0; col < o->TermWin.ncol; col++) {
            rxvt_buf_char_t ec;
	    /* compare new text with old - if exactly the same then continue */
	    rt1 = srp[col];	/* screen rendition */
	    rt2 = drp[col];	/* drawn rendition  */
	    if ((text_t_to_char (stp[col]) == text_t_to_char (dtp[col]))	/* must match characters to skip */
		&&((rt1 == rt2)	/* either rendition the same or  */
		   ||((text_t_to_char (stp[col]) == ' ')	/* space w/ no bg change */
		      &&(GET_BGATTR (rt1) == GET_BGATTR (rt2))))) {
#ifdef MULTICHAR_SET
		/* if first byte is Kanji then compare second bytes */
		if ((rt1 & RS_multiMask) != RS_multi1)
		    continue;
		else if (stp[col + 1] == dtp[col + 1]) {
		    /* assume no corrupt characters on the screen */
		    col++;
		    continue;
		}
#else
		continue;
#endif
	    }
	    /* redraw one or more characters */
	    dtp[col] = stp[col];
	    rend = drp[col] = srp[col];

	    len = 0;
	    o->buffer[len++] = ec = text_t_to_char (stp[col]);
	    ypixelc = Row2Pixel(row);
#ifdef UTF8_FONT
	    ypixel = ypixelc + FONT_ASCENT;
#else
	    ypixel = ypixelc + o->TermWin.font->ascent;
#endif
	    xpixel = Col2Pixel (col + (ec == ZERO_WIDTH_EMPTY_CHAR));
	    fontdiff = 0;
	    wlen = 1;

/*
 * Find out the longest string we can write out at once
 */
	    if (fprop == 0) {	/* Fixed width font */
#ifdef MULTICHAR_SET
#ifdef UTF8_FONT
#error
#endif
		if (((rend & RS_multiMask) == RS_multi1)
		    && col < o->TermWin.ncol - 1
		    && ((srp[col + 1]) & RS_multiMask) == RS_multi2) {
		    if (!wbyte) {
			wbyte = 1;
			XSetFont (o->Xdisplay, o->TermWin.gc, o->TermWin.mfont->fid);
			fontdiff = o->TermWin.mprop;
			draw_string = XDrawString16;
			draw_image_string = XDrawImageString16;
		    }
		    /* double stepping - we're in Kanji mode */
		    for (; ++col < o->TermWin.ncol;) {
			/* XXX: could check sanity on 2nd byte */
			dtp[col] = stp[col];
			drp[col] = srp[col];
			o->buffer[len++] = stp[col];
			col++;
			if ((col == o->TermWin.ncol) || (srp[col] != rend))
			    break;
			if ((stp[col] == dtp[col])
			    && (srp[col] == drp[col])
			    && (stp[col + 1] == dtp[col + 1]))
			    break;
			if (len == o->currmaxcol)
			    break;
			dtp[col] = stp[col];
			drp[col] = srp[col];
			o->buffer[len++] = stp[col];
		    }
		    col--;
		    if (o->buffer[0] & 0x80)
			o->multichar_decode (o->buffer, len);
		    wlen = len / 2;
		} else {
		    if ((rend & RS_multiMask) == RS_multi1) {
			/* XXX : maybe do the same thing for RS_multi2 */
			/* corrupt character - you're outta there */
			rend &= ~RS_multiMask;
			drp[col] = rend;	/* TODO check: may also want */
			dtp[col] = ' ';	/* to poke into stp/srp      */
			o->buffer[0] = ' ';
		    }
		    if (wbyte) {
			wbyte = 0;
			XSetFont (o->Xdisplay, o->TermWin.gc, o->TermWin.font->fid);
			fontdiff = o->TermWin.mprop;
			draw_string = XDrawString;
			draw_image_string = XDrawImageString;
		    }
#endif
		    /* single stepping - `normal' mode */
		    for (j = 0; ++col < o->TermWin.ncol - 1;) {
			if (rend != srp[col])
			    break;
			if (len == o->currmaxcol)
			    break;
			if ((text_t_to_char (stp[col]) == text_t_to_char (dtp[col])) && (srp[col] == drp[col])) {
#ifdef INEXPENSIVE_LOCAL_X_CALLS
			    if (o->display_is_local)
				break;
#endif
			    j++;
			} else {
			    j = 0;
			    dtp[col] = stp[col];
			    drp[col] = srp[col];
			}
			o->buffer[len++] = text_t_to_char (stp[col]);
		    }
		    col--;	/* went one too far.  move back */
		    len -= j;	/* dump any matching trailing chars */
		    wlen = len;
#ifdef MULTICHAR_SET
		}
#endif
	    }
	    o->buffer[len] = '\0';

/*
 * Determine the attributes for the string
 */
	    fore = GET_FGCOLOR (rend);
	    back = GET_BGCOLOR (rend);
	    rend = GET_ATTR (rend);
	    gcmask = 0;
	    rvid = (rend & RS_RVid) ? 1 : 0;

	    switch (rend & RS_fontMask) {
	    case RS_acsFont:
		for (i = 0; i < len; i++)
		    if (o->buffer[i] == 0x5f)
			o->buffer[i] = 0x7f;
		    else if (o->buffer[i] > 0x5f && o->buffer[i] < 0x7f)
			o->buffer[i] -= 0x5f;
		break;
	    case RS_ukFont:
		for (i = 0; i < len; i++)
		    if (o->buffer[i] == '#')
			o->buffer[i] = 0x1e;
		break;
	    }
	    if (rvid)
		SWAP_IT (fore, back, i);
	    if (back != Color_bg) {
		gcvalue.background = o->PixColors[back];
		gcmask |= GCBackground;
	    }
	    if (fore != Color_fg) {
		gcvalue.foreground = o->PixColors[fore];
		gcmask |= GCForeground;
	    }
#ifndef NO_BOLDUNDERLINE
	    else if (rend & RS_Bold) {
		if (o->Xdepth > 2 && o->rs[Rs_color + Color_BD]
		    && o->PixColors[fore] != o->PixColors[Color_BD]
		    && o->PixColors[back] != o->PixColors[Color_BD]) {
		    gcvalue.foreground = o->PixColors[Color_BD];
		    gcmask |= GCForeground;
# ifndef VERYBOLD
		    rend &= ~RS_Bold;	/* we've taken care of it */
# endif
		}
	    } else if (rend & RS_Uline) {
		if (o->Xdepth > 2 && o->rs[Rs_color + Color_UL]
		    && o->PixColors[fore] != o->PixColors[Color_UL]
		    && o->PixColors[back] != o->PixColors[Color_UL]) {
		    gcvalue.foreground = o->PixColors[Color_UL];
		    gcmask |= GCForeground;
		    rend &= ~RS_Uline;	/* we've taken care of it */
		}
	    }
#endif
	    if (gcmask)
		XChangeGC (o->Xdisplay, o->TermWin.gc, gcmask, &gcvalue);
#ifndef NO_BOLDFONT
#ifdef UTF8_FONT
	    if (!wbyte && MONO_BOLD (rend))
#else
	    if (!wbyte && MONO_BOLD (rend) && o->TermWin.boldFont != NULL)
#endif
            {
		bfont = 1;
		XSetFont (o->Xdisplay, o->TermWin.gc, o->TermWin.boldFont->fid);
		rend &= ~RS_Bold;	/* we've taken care of it */
	    } else if (bfont) {
		bfont = 0;
		XSetFont (o->Xdisplay, o->TermWin.gc, o->TermWin.font->fid);
	    }
#endif
/*
 * Actually do the drawing of the string here
 */
	    if (back == Color_bg) {
		CLEAR_CHARS(xpixel, ypixelc, len);
		DRAW_STRING(draw_string, xpixel, ypixel, o->buffer, wlen);
	    } else {
		if (fprop || fontdiff) {
		    gcmaskf = GCForeground;
		    ltmp = gcvalue.foreground;
		    gcvalue.foreground = gcvalue.background;
		    XChangeGC(o->Xdisplay, o->TermWin.gc, gcmaskf, &gcvalue);
		    XFillRectangle (o->Xdisplay, drawBuffer, o->TermWin.gc,
				   xpixel, ypixelc,
				   Width2Pixel(len), Height2Pixel(1));
		    gcvalue.foreground = ltmp;
		    XChangeGC(o->Xdisplay, o->TermWin.gc, gcmaskf, &gcvalue);
		    DRAW_STRING(draw_string, xpixel, ypixel, o->buffer, wlen);
		} else
		    DRAW_STRING(draw_image_string, xpixel, ypixel, o->buffer, wlen);
	    }

#ifndef NO_BOLDOVERSTRIKE
# ifdef NO_BOLDOVERSTRIKE_MULTI
	    if (!wbyte)
# endif
		if (MONO_BOLD (rend))
		    DRAW_STRING (draw_string, xpixel + 1, ypixel, o->buffer,
				 wlen);
#endif
#ifdef UTF8_FONT
#warning finish
	    if ((rend & RS_Uline) && 1)
#else
	    if ((rend & RS_Uline) && (o->TermWin.font->descent > 1))
#endif
		XDrawLine (o->Xdisplay, drawBuffer, o->TermWin.gc,
			   xpixel, ypixel + 1,
			   xpixel + Width2Pixel (len) - 1, ypixel + 1);
	    if (gcmask) {	/* restore normal colours */
		gcvalue.foreground = o->PixColors[Color_fg];
		gcvalue.background = o->PixColors[Color_bg];
		XChangeGC (o->Xdisplay, o->TermWin.gc, gcmask, &gcvalue);
	    }
	}
    }

/*
 * F: cleanup cursor and display outline cursor in necessary
 */
    if (o->screen.flags & Screen_VisibleCursor) {
	if (focus) {
	    srp = &(o->screen.rend[currow][o->screen.cur.col]);
	    *srp ^= RS_RVid;
#ifndef NO_CURSORCOLOR
	    *srp = (*srp & ~(RS_fgMask | RS_bgMask)) | cc1;
#endif
	    if (morecur) {
		srp += morecur;
		*srp ^= RS_RVid;
#if defined(MULTICHAR_SET) && ! defined(NO_CURSORCOLOR)
		*srp = (*srp & ~(RS_fgMask | RS_bgMask)) | cc2;
#endif
	    }
	} else if (o->oldcursor.row >= 0) {
	    col = o->oldcursor.col + morecur;
	    wbyte = morecur ? 1 : 0;
#ifndef NO_CURSORCOLOR
	    gcmask = 0;
	    if (o->Xdepth > 2 && o->rs[Rs_color + Color_cursor]) {
		gcvalue.foreground = o->PixColors[Color_cursor];
		gcmask = GCForeground;
		XChangeGC (o->Xdisplay, o->TermWin.gc, gcmask, &gcvalue);
		gcvalue.foreground = o->PixColors[Color_fg];
	    }
#endif
	    XDrawRectangle (o->Xdisplay, drawBuffer, o->TermWin.gc,
			    Col2Pixel (col), Row2Pixel (o->oldcursor.row),
			    Width2Pixel (1 + wbyte) - 1, Height2Pixel (1) - 1);
#ifndef NO_CURSORCOLOR
	    if (gcmask)		/* restore normal colours */
		XChangeGC (o->Xdisplay, o->TermWin.gc, gcmask, &gcvalue);
#endif
	}
    }
/*
 * G: cleanup selection
 */
    rxvtlib_scr_reverse_selection (o);

/*
 * H: other general cleanup
 */
#if defined(XPM_BACKGROUND) && defined(XPM_BUFFERING)
    XClearWindow (o->Xdisplay, o->TermWin.vt);
#else
    if (boldlast)		/* clear the whole screen height */
	XClearArea (o->Xdisplay, o->TermWin.vt, TermWin_TotalWidth () - 2, 0,
		    1, TermWin_TotalHeight () - 1, 0);
#endif
    if (type & SMOOTH_REFRESH)
	XSync (o->Xdisplay, False);

    o->want_refresh = 0;		/* screen is current */
#ifdef UTF8_FONT
    CPopFont ();
#endif
}

/* EXTPROTO */
void            rxvtlib_scr_clear (rxvtlib *o)
{E_
    if (!o->TermWin.mapped)
	return;
#ifdef TRANSPARENT
    if (o->Options & Opt_transparent) {
	int             i;

	for (i = KNOW_PARENTS; i--;)
	    if (o->TermWin.parent[i] != None)
		XClearWindow (o->Xdisplay, o->TermWin.parent[i]);
    }
#endif
    XClearWindow (o->Xdisplay, o->TermWin.vt);
}

/* ------------------------------------------------------------------------- */
/* INTPROTO */
void            rxvtlib_scr_reverse_selection (rxvtlib *o)
{E_
    int             i, col, row, end_row;
    rend_t         *srp;

    end_row = o->TermWin.saveLines - o->TermWin.view_start;
    if (o->selection.op && o->current_screen == o->selection.screen) {
	i = o->selection.beg.row + o->TermWin.saveLines;
	row = o->selection.end.row + o->TermWin.saveLines;
	if (i >= end_row)
	    col = o->selection.beg.col;
	else {
	    col = 0;
	    i = end_row;
	}
	end_row += o->TermWin.nrow;
	for (; i < row && i < end_row; i++, col = 0)
	    for (srp = o->screen.rend[i]; col < o->TermWin.ncol; col++)
		srp[col] ^= RS_RVid;
	if (i == row && i < end_row)
	    for (srp = o->screen.rend[i]; col < o->selection.end.col; col++)
		srp[col] ^= RS_RVid;
    }
}

/* ------------------------------------------------------------------------- *
 *                           CHARACTER SELECTION                             * 
 * ------------------------------------------------------------------------- */

/*
 * -TermWin.nscrolled <= (selection row) <= TermWin.nrow - 1
 */
/* INTPROTO */
void            rxvtlib_selection_check (rxvtlib *o, int check_more)
{E_
    row_col_t       pos;

    if ((o->selection.beg.row < -o->TermWin.nscrolled)
	|| (o->selection.beg.row >= o->TermWin.nrow)
	|| (o->selection.mark.row < -o->TermWin.nscrolled)
	|| (o->selection.mark.row >= o->TermWin.nrow)
	|| (o->selection.end.row < -o->TermWin.nscrolled)
	|| (o->selection.end.row >= o->TermWin.nrow))
	CLEAR_ALL_SELECTION;

    if (check_more == 1 && o->current_screen == o->selection.screen) {
	/* check for cursor position */
	pos.row = o->screen.cur.row;
	pos.col = o->screen.cur.col;
	if (!ROWCOL_IS_BEFORE (pos, o->selection.beg)
	    && ROWCOL_IS_BEFORE (pos, o->selection.end))
	    CLEAR_SELECTION;
    } else if (check_more == 2) {
	pos.row = 0;
	pos.col = 0;
	if (ROWCOL_IS_BEFORE (o->selection.beg, pos)
	    && ROWCOL_IS_AFTER (o->selection.end, pos))
	    CLEAR_SELECTION;
    } else if (check_more == 3) {
	pos.row = 0;
	pos.col = 0;
	if (ROWCOL_IS_AFTER (o->selection.end, pos))
	    CLEAR_SELECTION;
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Paste a selection direct to the command
 */
/* INTPROTO */
void            rxvtlib_PasteIt (rxvtlib *o, const unsigned char *data, unsigned int nitems)
{E_
    int             num;
    const unsigned char *p;
    const unsigned char cr = '\r';

    for (p = data, num = 0; nitems--; p++)
	if (*p != '\n')
	    num++;
	else {
	    rxvtlib_tt_write (o, data, num);
	    rxvtlib_tt_write (o, &cr, 1);
	    data += (num + 1);
	    num = 0;
	}
    if (num)
	rxvtlib_tt_write (o, data, num);
}

/* ------------------------------------------------------------------------- */
/*
 * Respond to a notification that a primary selection has been sent
 * EXT: SelectionNotify
 */
/* EXTPROTO */
int _rxvtlib_selection_paste (rxvtlib * o, Window win, unsigned int prop, int Delete)
{E_
    long nread;
    unsigned long bytes_after;
    XTextProperty ct;

#ifdef MULTICHAR_SET
    int dummy_count;
    char **cl;
#endif

    D_SELECT ((stderr, "selection_paste(%08x, %u, %d)", win, prop, Delete));
    if (prop == None)
	return 1;
    for (nread = 0, bytes_after = 1; bytes_after > 0; nread += ct.nitems) {
	if ((XGetWindowProperty (o->Xdisplay, win, prop, (nread / 4), PROP_SIZE,
				 Delete, AnyPropertyType, &ct.encoding,
				 &ct.format, &ct.nitems, &bytes_after, &ct.value) != Success)) {
	    XFree (ct.value);
	    return 1;
	}
	if (ct.value == NULL)
	    continue;
#ifdef MULTICHAR_SET
	if (XmbTextPropertyToTextList (o->Xdisplay, &ct, &cl, &dummy_count) == Success && cl) {
	    rxvtlib_PasteIt (o, cl[0], STRLEN (cl[0]));
	    XFreeStringList (cl);
	} else
#endif
	    rxvtlib_PasteIt (o, ct.value, ct.nitems);
	XFree (ct.value);
    }
    if (!nread)
	return 1;
    return 0;
}

#define RXVTLIB_SELECTION_PASTE__RESET          NULL, 0, 0, 0
/*
 * Respond to a notification that a primary selection has been sent
 * repeated in editwidget.c
 */
void rxvtlib_selection_paste (rxvtlib * o, Window win, unsigned int prop, int delete_prop)
{E_
#ifdef UTF8_FONT
    static int attempt_next_conversion = 0;
#endif
    struct timeval tv, tv_start;
    unsigned long bytes_after;
    Atom actual_type;
    int actual_fmt;
    unsigned long nitems;
    unsigned char *s = 0;

#ifdef UTF8_FONT
    if (!o /* RXVTLIB_SELECTION_PASTE__RESET */) {
        attempt_next_conversion = 0;
        return;
    }
#endif

    if (prop == None)
    {
#ifdef UTF8_FONT
        if (!attempt_next_conversion) {
            attempt_next_conversion = 1;
            XConvertSelection (o->Xdisplay, XA_PRIMARY, XA_STRING, XInternAtom (o->Xdisplay, "VT_SELECTION", False), o->TermWin.vt, 0);
        }
#endif
	return;
    }
    if (XGetWindowProperty
	(o->Xdisplay, win, prop, 0, 8, False, AnyPropertyType, &actual_type, &actual_fmt, &nitems,
	 &bytes_after, &s) != Success) {
	XFree (s);
	return;
    }
    XFree (s);
    if (actual_type != XInternAtom (o->Xdisplay, "INCR", False)) {
	_rxvtlib_selection_paste (o, win, prop, delete_prop);
	return;
    }
    XDeleteProperty (o->Xdisplay, win, prop);
    gettimeofday (&tv_start, 0);
    for (;;) {
	long t;
	fd_set r;
	XEvent xe;
	if (XCheckMaskEvent (o->Xdisplay, PropertyChangeMask, &xe)) {
	    if (xe.type == PropertyNotify && xe.xproperty.state == PropertyNewValue) {
/* time between arrivals of data */
		gettimeofday (&tv_start, 0);
		if (_rxvtlib_selection_paste (o, win, prop, True))
		    break;
	    }
	} else {
	    tv.tv_sec = 0;
	    tv.tv_usec = 10000;
	    FD_ZERO (&r);
	    FD_SET (ConnectionNumber (o->Xdisplay), &r);
	    select (ConnectionNumber (o->Xdisplay) + 1, &r, 0, 0, &tv);
	    if (FD_ISSET (ConnectionNumber (o->Xdisplay), &r))
		continue;
	}
	gettimeofday (&tv, 0);
	t = (tv.tv_sec - tv_start.tv_sec) * 1000000L + (tv.tv_usec - tv_start.tv_usec);
/* no data for five seconds, so quit */
	if (t > 5000000L)
	    break;
    }
}

#ifndef STANDALONE
extern struct edit_selection selection;
#endif

/* ------------------------------------------------------------------------- */
/*
 * Request the current primary selection
 * EXT: button 2 release
 */
/* EXTPROTO */
void            rxvtlib_selection_request (rxvtlib *o, Time tm, int x, int y)
{E_
    Atom            prop;

#ifdef MULTICHAR_SET
    Atom            ct;
#endif

    D_SELECT ((stderr, "selection_request(%ld, %d, %d)", tm, x, y));
    if (x < 0 || x >= o->TermWin.width || y < 0 || y >= o->TermWin.height)
	return;			/* outside window */

    if (o->selection.text != NULL)
	rxvtlib_PasteIt (o, o->selection.text, o->selection.len);	/* internal selection */
#if 0
#ifndef STANDALONE
    else if (selection.text != NULL)
	rxvtlib_PasteIt (o, selection.text, selection.len);	/* cooledit selection */
#endif
#endif
    else if (XGetSelectionOwner (o->Xdisplay, XA_PRIMARY) == None)
	rxvtlib_selection_paste (o, Xroot, XA_CUT_BUFFER0, False);
    else {
#ifdef UTF8_FONT
        rxvtlib_selection_paste (RXVTLIB_SELECTION_PASTE__RESET);
#endif
	prop = XInternAtom (o->Xdisplay, "VT_SELECTION", False);
#ifdef MULTICHAR_SET
	ct = XInternAtom (o->Xdisplay, "COMPOUND_TEXT", False);
	XConvertSelection (o->Xdisplay, XA_PRIMARY, ct, prop, o->TermWin.vt, tm);
#else
#ifdef UTF8_FONT
	XConvertSelection (o->Xdisplay, XA_PRIMARY, ATOM_UTF8_STRING, prop, o->TermWin.vt, tm);
#else
	XConvertSelection (o->Xdisplay, XA_PRIMARY, XA_STRING, prop, o->TermWin.vt,
			   tm);
#endif
#endif
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Clear all selected text
 * EXT: SelectionClear
 */
/* EXTPROTO */
void            rxvtlib_selection_clear (rxvtlib *o)
{E_
#ifdef STANDALONE
    D_SELECT ((stderr, "selection_clear()"));

    o->want_refresh = 1;
    if (o->selection.text)
	FREE (o->selection.text);
    o->selection.text = NULL;
    o->selection.len = 0;
    CLEAR_SELECTION;
#else
    o->want_refresh = 1;
    selection_clear ();
    CLEAR_SELECTION;
#endif
}

extern Atom ATOM_ICCCM_P2P_CLIPBOARD;

/* ------------------------------------------------------------------------- */
/* 
 * Copy a selection into the cut buffer
 * EXT: button 1 or 3 release
 */
/* EXTPROTO */
void            rxvtlib_selection_make (rxvtlib *o, Time tm)
{E_
    int             i, col, end_col, row, end_row;
    unsigned char  *new_selection_text;
    char           *str;
    text_t         *t;

    D_SELECT (
	      (stderr,
	       "selection_make(): selection.op=%d, selection.clicks=%d",
	       o->selection.op, o->selection.clicks));
    switch (o->selection.op) {
    case SELECTION_CONT:
	break;
    case SELECTION_INIT:
	CLEAR_SELECTION;
	/* FALLTHROUGH */
    case SELECTION_BEGIN:
	o->selection.op = SELECTION_DONE;
	/* FALLTHROUGH */
    default:
	return;
    }
    o->selection.op = SELECTION_DONE;

    if (o->selection.clicks == 4)
	return;			/* nothing selected, go away */

    i = (o->selection.end.row - o->selection.beg.row + 1) * (o->TermWin.ncol + 1) + 1;

#ifdef UTF8_FONT
    /* conservative allocation assumng every char is 4-bytes */
    str = MALLOC (i * strlen ((char *) wcrtomb_wchar_to_utf8 (FONT_LAST_UNICHAR)));
#else
    str = MALLOC (i * sizeof (char));
#endif

#warning selection
    new_selection_text = (unsigned char *)str;

    col = max (o->selection.beg.col, 0);
    row = o->selection.beg.row + o->TermWin.saveLines;
    end_row = o->selection.end.row + o->TermWin.saveLines;
/*
 * A: rows before end row
 */
    for (; row < end_row; row++) {
	t = &(o->screen.text[row][col]);
	if ((end_col = o->screen.tlen[row]) == -1)
	    end_col = o->TermWin.ncol;

#ifdef UTF8_FONT

#define DO_ONE_CHAR \
            rxvt_buf_char_t u; \
            u = text_t_to_char (*t++); \
            if (u < 128) { \
	        *str++ = u; /* optimise the common case */ \
            } else if (u == ZERO_WIDTH_EMPTY_CHAR) { \
                /* pass */ \
            } else { \
                char *q; \
                q = (char *) wcrtomb_wchar_to_utf8 (u); \
                *str++ = *q++; \
                if (*q) { \
                    *str++ = *q++; \
                    if (*q) { \
                        *str++ = *q++; \
                        if (*q) \
                            *str++ = *q++; \
                    } \
                } \
            }

#else

#define DO_ONE_CHAR     *str++ = text_t_to_char (*t++)

#endif

	for (; col < end_col; col++) {
            DO_ONE_CHAR;
        }
	col = 0;
	if (o->screen.tlen[row] != -1)
	    *str++ = '\n';
    }
/*
 * B: end row
 */
    t = &(o->screen.text[row][col]);
    end_col = o->screen.tlen[row];
    if (end_col == -1 || o->selection.end.col <= end_col)
	end_col = o->selection.end.col;
    MIN_IT (end_col, o->TermWin.ncol);	/* CHANGE */
    for (; col < end_col; col++) {
        DO_ONE_CHAR;
    }
#ifndef NO_OLD_SELECTION
    if (o->selection_style == OLD_SELECT)
	if (end_col == o->TermWin.ncol)
	    *str++ = '\n';
#endif
#ifndef NO_NEW_SELECTION
    if (o->selection_style != OLD_SELECT)
	if (end_col != o->selection.end.col)
	    *str++ = '\n';
#endif
    *str = '\0';
    if ((i = strlen ((char *)new_selection_text)) == 0) {
	FREE (new_selection_text);
	return;
    }
#ifndef STANDALONE
    selection_clear ();
#endif
    o->selection.len = i;
    if (o->selection.text)
	FREE (o->selection.text);
    o->selection.text = new_selection_text;

    XSetSelectionOwner (o->Xdisplay, XA_PRIMARY, o->TermWin.vt, tm);
    if (XGetSelectionOwner (o->Xdisplay, XA_PRIMARY) != o->TermWin.vt)
	print_error ("can't get primary selection");
    XSetSelectionOwner (o->Xdisplay, ATOM_ICCCM_P2P_CLIPBOARD, o->TermWin.vt, tm);
    XChangeProperty (o->Xdisplay, Xroot, XA_CUT_BUFFER0, XA_STRING, 8,
		     PropModeReplace, o->selection.text, o->selection.len);
    D_SELECT ((stderr, "selection_make(): selection.len=%d", o->selection.len));
}

/* ------------------------------------------------------------------------- */
/*
 * Mark or select text based upon number of clicks: 1, 2, or 3
 * EXT: button 1 press
 */
/* EXTPROTO */
void            rxvtlib_selection_click (rxvtlib *o, int clicks, int x, int y)
{E_
/*    int             r, c;
 *   row_col_t             ext_beg, ext_end;
 */

    D_SELECT ((stderr, "selection_click(%d, %d, %d)", clicks, x, y));

    clicks = ((clicks - 1) % 3) + 1;
    o->selection.clicks = clicks;	/* save clicks so extend will work */

    rxvtlib_selection_start_colrow (o, Pixel2Col (x), Pixel2Row (y));
    if (clicks == 2 || clicks == 3)
	rxvtlib_selection_extend_colrow (o, o->selection.mark.col,
				 o->selection.mark.row + o->TermWin.view_start, 0,	/* button 3     */
				 1,	/* button press */
				 0);	/* click change */
}

/* ------------------------------------------------------------------------- */
/*
 * Mark a selection at the specified col/row
 */
/* INTPROTO */
void            rxvtlib_selection_start_colrow (rxvtlib *o, int col, int row)
{E_
    o->want_refresh = 1;
    o->selection.mark.col = col;
    o->selection.mark.row = row - o->TermWin.view_start;
    MAX_IT (o->selection.mark.row, -o->TermWin.nscrolled);
    MIN_IT (o->selection.mark.row, o->TermWin.nrow - 1);
    MAX_IT (o->selection.mark.col, 0);
    MIN_IT (o->selection.mark.col, o->TermWin.ncol - 1);

    if (o->selection.op) {		/* clear the old selection */
	o->selection.beg.row = o->selection.end.row = o->selection.mark.row;
	o->selection.beg.col = o->selection.end.col = o->selection.mark.col;
    }
    o->selection.op = SELECTION_INIT;
    o->selection.screen = o->current_screen;
}

/* ------------------------------------------------------------------------- */
/*
 * Word select: select text for 2 clicks
 * We now only find out the boundary in one direction
 */

/* what do we want: spaces/tabs are delimiters or cutchars or non-cutchars */

/* INTPROTO */
void            rxvtlib_selection_delimit_word (rxvtlib *o, int dirn, const row_col_t * mark,
					row_col_t * ret)
{E_
    int             col, row, dirnadd, tcol, trow, w1, w2;
    row_col_t       bound;
    text_t         *stp;
    rend_t         *srp;

    if (o->selection.clicks != 2)
	return;			/* Go away: we only handle double clicks */

    if (dirn == UP) {
	bound.row = o->TermWin.saveLines - o->TermWin.nscrolled - 1;
	bound.col = 0;
	dirnadd = -1;
    } else {
	bound.row = o->TermWin.saveLines + o->TermWin.nrow;
	bound.col = o->TermWin.ncol - 1;
	dirnadd = 1;
    }
    row = mark->row + o->TermWin.saveLines;
    col = mark->col;
    MAX_IT (col, 0);
/* find the edge of a word */
    stp = &(o->screen.text[row][col]);
    w1 = DELIMIT_TEXT (*stp);

    if (o->selection_style != NEW_SELECT) {
	if (w1 == 1) {
	    stp += dirnadd;
	    if (DELIMIT_TEXT (*stp) == 1)
		goto Old_Word_Selection_You_Die;
	    col += dirnadd;
	}
	w1 = 0;
    }
    srp = (&o->screen.rend[row][col]);
    w2 = DELIMIT_REND (*srp);

    for (;;) {
	for (; col != bound.col; col += dirnadd) {
	    stp += dirnadd;
	    if (DELIMIT_TEXT (*stp) != w1)
		break;
	    srp += dirnadd;
	    if (DELIMIT_REND (*srp) != w2)
		break;
	}
	if ((col == bound.col) && (row != bound.row)) {
	    if (o->screen.tlen[(row - (dirn == UP))] == -1) {
		trow = row + dirnadd;
		tcol = (dirn == UP) ? (o->TermWin.ncol - 1) : 0;
		if (o->screen.text[trow] == NULL)
		    break;
		stp = &(o->screen.text[trow][tcol]);
		srp = &(o->screen.rend[trow][tcol]);
		if (DELIMIT_TEXT (*stp) != w1 || DELIMIT_REND (*srp) != w2)
		    break;
		row = trow;
		col = tcol;
		continue;
	    }
	}
	break;
    }
  Old_Word_Selection_You_Die:
    D_SELECT (
	      (stderr,
	       "selection_delimit_word(%s,...) @ (r:%3d, c:%3d) has boundary (r:%3d, c:%3d)",
	       (dirn == UP ? "up	" : "down"), mark->row, mark->col,
	       row - o->TermWin.saveLines, col));

    if (dirn == DN)
	col++;			/* put us on one past the end */

/* Poke the values back in */
    ret->row = row - o->TermWin.saveLines;
    ret->col = col;
}

/* ------------------------------------------------------------------------- */
/*
 * Extend the selection to the specified x/y pixel location
 * EXT: button 3 press; button 1 or 3 drag
 * flag == 0 ==> button 1
 * flag == 1 ==> button 3 press
 * flag == 2 ==> button 3 motion
 */
/* EXTPROTO */
void            rxvtlib_selection_extend (rxvtlib *o, int x, int y, int flag)
{E_
    int             col, row;

    col = Pixel2Col (x);
    row = Pixel2Row (y);
    MAX_IT (row, 0);
    MIN_IT (row, o->TermWin.nrow - 1);
    MAX_IT (col, 0);
    MIN_IT (col, o->TermWin.ncol);
#ifndef NO_NEW_SELECTION
/*
 * If we're selecting characters (single click) then we must check first
 * if we are at the same place as the original mark.  If we are then
 * select nothing.  Otherwise, if we're to the right of the mark, you have to
 * be _past_ a character for it to be selected.
 */
    if (o->selection_style != OLD_SELECT) {
	if (((o->selection.clicks % 3) == 1) && !flag
	    && (col == o->selection.mark.col
		&& (row == o->selection.mark.row + o->TermWin.view_start))) {
	    /* select nothing */
	    o->selection.beg.row = o->selection.end.row = 0;
	    o->selection.beg.col = o->selection.end.col = 0;
	    o->selection.clicks = 4;
	    D_SELECT ((stderr, "selection_extend() selection.clicks = 4"));
	    return;
	}
    }
#endif
    if (o->selection.clicks == 4)
	o->selection.clicks = 1;
    rxvtlib_selection_extend_colrow (o, col, row, !!flag,	/* ? button 3      */
			     flag == 1 ? 1 : 0,	/* ? button press  */
			     0);	/* no click change */
}

#ifdef MULTICHAR_SET
/* INTPROTO */
void            rxvtlib_selection_adjust_kanji (rxvtlib *o)
{E_
    int             c, r;

    if (o->selection.beg.col > 0) {
	r = o->selection.beg.row + o->TermWin.saveLines;
	c = o->selection.beg.col;
	if (((o->screen.rend[r][c] & RS_multiMask) == RS_multi2)
	    && ((o->screen.rend[r][c - 1] & RS_multiMask) == RS_multi1))
	    o->selection.beg.col--;
    }
    if (o->selection.end.col < o->TermWin.ncol) {
	r = o->selection.end.row + o->TermWin.saveLines;
	c = o->selection.end.col;
	if (((o->screen.rend[r][c - 1] & RS_multiMask) == RS_multi1)
	    && ((o->screen.rend[r][c] & RS_multiMask) == RS_multi2))
	    o->selection.end.col++;
    }
}
#endif				/* MULTICHAR_SET */

/* ------------------------------------------------------------------------- */
/*
 * Extend the selection to the specified col/row
 */
/* INTPROTO */
void            rxvtlib_selection_extend_colrow (rxvtlib *o, int col, int row, int button3,
					 int buttonpress, int clickchange)
{E_
    int             end_col;
    row_col_t       pos;
    enum {
	LEFT, RIGHT
    } closeto = RIGHT;

    D_SELECT (
	      (stderr, "selection_extend_colrow(c:%d, r:%d, %d, %d) clicks:%d",
	       col, row, button3, buttonpress, o->selection.clicks));
    D_SELECT (
	      (stderr,
	       "selection_extend_colrow() ENT  b:(r:%d,c:%d) m:(r:%d,c:%d), e:(r:%d,c:%d)",
	       o->selection.beg.row, o->selection.beg.col, o->selection.mark.row,
	       o->selection.mark.col, o->selection.end.row, o->selection.end.col));

    o->want_refresh = 1;
    switch (o->selection.op) {
    case SELECTION_INIT:
	CLEAR_SELECTION;
	o->selection.op = SELECTION_BEGIN;
	/* FALLTHROUGH */
    case SELECTION_BEGIN:
	if (row != o->selection.mark.row || col != o->selection.mark.col
	    || (!button3 && buttonpress))
	    o->selection.op = SELECTION_CONT;
	break;
    case SELECTION_DONE:
	o->selection.op = SELECTION_CONT;
	/* FALLTHROUGH */
    case SELECTION_CONT:
	break;
    case SELECTION_CLEAR:
	rxvtlib_selection_start_colrow (o, col, row);
	/* FALLTHROUGH */
    default:
	return;
    }

    pos.col = col;
    pos.row = row;

    pos.row -= o->TermWin.view_start;	/* adjust for scroll */

#ifndef NO_OLD_SELECTION
/*
 * This mimics some of the selection behaviour of version 2.20 and before.
 * There are no ``selection modes'', button3 is always character extension.
 * Note: button3 drag is always available, c.f. v2.20
 * Selection always terminates (left or right as appropriate) at the mark.
 */
    if (o->selection_style == OLD_SELECT) {
	static int      hate_those_clicks = 0;	/* a.k.a. keep mark position */

	if (o->selection.clicks == 1 || button3) {
	    if (hate_those_clicks) {
		hate_those_clicks = 0;
		if (o->selection.clicks == 1) {
		    o->selection.beg.row = o->selection.mark.row;
		    o->selection.beg.col = o->selection.mark.col;
		} else {
		    o->selection.mark.row = o->selection.beg.row;
		    o->selection.mark.col = o->selection.beg.col;
		}
	    }
	    if (ROWCOL_IS_BEFORE (pos, o->selection.mark)) {
		o->selection.end.row = o->selection.mark.row;
		o->selection.end.col = o->selection.mark.col + 1;
		o->selection.beg.row = pos.row;
		o->selection.beg.col = pos.col;
	    } else {
		o->selection.beg.row = o->selection.mark.row;
		o->selection.beg.col = o->selection.mark.col;
		o->selection.end.row = pos.row;
		o->selection.end.col = pos.col + 1;
	    }
# ifdef MULTICHAR_SET
	    rxvtlib_selection_adjust_kanji (o);
# endif				/* MULTICHAR_SET */
	} else if (o->selection.clicks == 2) {
	    rxvtlib_selection_delimit_word (o, UP, &(o->selection.mark), &(o->selection.beg));
	    rxvtlib_selection_delimit_word (o, DN, &(o->selection.mark), &(o->selection.end));
	    hate_those_clicks = 1;
	} else if (o->selection.clicks == 3) {
	    o->selection.beg.row = o->selection.end.row = o->selection.mark.row;
	    o->selection.beg.col = 0;
	    o->selection.end.col = o->TermWin.ncol;
	    hate_those_clicks = 1;
	}
	D_SELECT (
		  (stderr,
		   "selection_extend_colrow() EXIT b:(r:%d,c:%d) m:(r:%d,c:%d), e:(r:%d,c:%d)",
		   o->selection.beg.row, o->selection.beg.col, o->selection.mark.row,
		   o->selection.mark.col, o->selection.end.row, o->selection.end.col));
	return;
    }
#endif				/* ! NO_OLD_SELECTION */
#ifndef NO_NEW_SELECTION
/* selection_style must not be OLD_SELECT to get here */
/*
 * This is mainly xterm style selection with a couple of differences, mainly
 * in the way button3 drag extension works.
 * We're either doing: button1 drag; button3 press; or button3 drag
 *  a) button1 drag : select around a midpoint/word/line - that point/word/line
 *     is always at the left/right edge of the selection.
 *  b) button3 press: extend/contract character/word/line at whichever edge of
 *     the selection we are closest to.
 *  c) button3 drag : extend/contract character/word/line - we select around
 *     a point/word/line which is either the start or end of the selection
 *     and it was decided by whichever point/word/line was `fixed' at the 
 *     time of the most recent button3 press
 */
    if (button3 && buttonpress) {	/* button3 press */
	/*
	 * first determine which edge of the selection we are closest to
	 */
	if (ROWCOL_IS_BEFORE (pos, o->selection.beg)
	    || (!ROWCOL_IS_AFTER (pos, o->selection.end)
		&& (((pos.col - o->selection.beg.col)
		     + ((pos.row - o->selection.beg.row) * o->TermWin.ncol))
		    < ((o->selection.end.col - pos.col)
		       + ((o->selection.end.row - pos.row) * o->TermWin.ncol)))))
	    closeto = LEFT;
	if (closeto == LEFT) {
	    o->selection.beg.row = pos.row;
	    o->selection.beg.col = pos.col;
	    o->selection.mark.row = o->selection.end.row;
	    o->selection.mark.col = o->selection.end.col - (o->selection.clicks == 2);
	} else {
	    o->selection.end.row = pos.row;
	    o->selection.end.col = pos.col;
	    o->selection.mark.row = o->selection.beg.row;
	    o->selection.mark.col = o->selection.beg.col;
	}
    } else {			/* button1 drag or button3 drag */
	if (ROWCOL_IS_AFTER (o->selection.mark, pos)) {
	    if ((o->selection.mark.row == o->selection.end.row)
		&& (o->selection.mark.col == o->selection.end.col)
		&& clickchange && o->selection.clicks == 2)
		o->selection.mark.col--;
	    o->selection.beg.row = pos.row;
	    o->selection.beg.col = pos.col;
	    o->selection.end.row = o->selection.mark.row;
	    o->selection.end.col = o->selection.mark.col + (o->selection.clicks == 2);
	} else {
	    o->selection.beg.row = o->selection.mark.row;
	    o->selection.beg.col = o->selection.mark.col;
	    o->selection.end.row = pos.row;
	    o->selection.end.col = pos.col;
	}
    }

    if (o->selection.clicks == 1) {
	end_col = o->screen.tlen[o->selection.beg.row + o->TermWin.saveLines];
	if (end_col != -1 && o->selection.beg.col > end_col) {
#if 1
	    o->selection.beg.col = o->TermWin.ncol;
#else
	    if (o->selection.beg.row != o->selection.end.row)
		o->selection.beg.col = o->TermWin.ncol;
	    else
		o->selection.beg.col = o->selection.mark.col;
#endif
	}
	end_col = o->screen.tlen[o->selection.end.row + o->TermWin.saveLines];
	if (end_col != -1 && o->selection.end.col > end_col)
	    o->selection.end.col = o->TermWin.ncol;

# ifdef MULTICHAR_SET
	rxvtlib_selection_adjust_kanji (o);
# endif				/* MULTICHAR_SET */
    } else if (o->selection.clicks == 2) {
	if (ROWCOL_IS_AFTER (o->selection.end, o->selection.beg))
	    o->selection.end.col--;
	rxvtlib_selection_delimit_word (o, UP, &(o->selection.beg), &(o->selection.beg));
	rxvtlib_selection_delimit_word (o, DN, &(o->selection.end), &(o->selection.end));
    } else if (o->selection.clicks == 3) {
	if (ROWCOL_IS_AFTER (o->selection.mark, o->selection.beg))
	    o->selection.mark.col++;
	o->selection.beg.col = 0;
	o->selection.end.col = o->TermWin.ncol;
    }
    if (button3 && buttonpress) {	/* mark may need to be changed */
	if (closeto == LEFT) {
	    o->selection.mark.row = o->selection.end.row;
	    o->selection.mark.col = o->selection.end.col - (o->selection.clicks == 2);
	} else {
	    o->selection.mark.row = o->selection.beg.row;
	    o->selection.mark.col = o->selection.beg.col;
	}
    }
    D_SELECT (
	      (stderr,
	       "selection_extend_colrow() EXIT b:(r:%d,c:%d) m:(r:%d,c:%d), e:(r:%d,c:%d)",
	       o->selection.beg.row, o->selection.beg.col, o->selection.mark.row,
	       o->selection.mark.col, o->selection.end.row, o->selection.end.col));
#endif				/* ! NO_NEW_SELECTION */
}

/* ------------------------------------------------------------------------- */
/*
 * Double click on button 3 when already selected
 * EXT: button 3 double click
 */
/* EXTPROTO */
void            rxvtlib_selection_rotate (rxvtlib *o, int x, int y)
{E_
    o->selection.clicks = o->selection.clicks % 3 + 1;
    rxvtlib_selection_extend_colrow (o, Pixel2Col (x), Pixel2Row (y), 1, 0, 1);
}

/* ------------------------------------------------------------------------- */
/*
 * On some systems, the Atom typedef is 64 bits wide.  We need to have a type
 * that is exactly 32 bits wide, because a format of 64 is not allowed by
 * the X11 protocol.
 */

/* ------------------------------------------------------------------------- */
/*
 * Respond to a request for our current selection
 * EXT: SelectionRequest
 */
/* EXTPROTO */
void            rxvtlib_selection_send (rxvtlib *o, const XSelectionRequestEvent * rq)
{E_
    XEvent          ev;
#ifdef UTF8_FONT
    Atom            target_list[5];
#else
    Atom            target_list[4];
#endif
    Atom            target;
    static Atom     xa_targets = None;
    static Atom     xa_compound_text = None;
    static Atom     xa_text = None;
    XTextProperty   ct;
    XICCEncodingStyle style;
    char           *cl[4];

    memset(&ev, 0, sizeof(ev));
    memset(&ct, 0, sizeof(ct));
    memset(&style, 0, sizeof(style));

    if (xa_text == None)
	xa_text = XInternAtom (o->Xdisplay, "TEXT", False);
    if (xa_compound_text == None)
	xa_compound_text = XInternAtom (o->Xdisplay, "COMPOUND_TEXT", False);
    if (xa_targets == None)
	xa_targets = XInternAtom (o->Xdisplay, "TARGETS", False);

    ev.xselection.type = SelectionNotify;
    ev.xselection.property = None;
    ev.xselection.display = rq->display;
    ev.xselection.requestor = rq->requestor;
    ev.xselection.selection = rq->selection;
    ev.xselection.target = rq->target;
    ev.xselection.time = rq->time;

    if (rq->target == xa_targets) {
	target_list[0] = (Atom32) xa_targets;
	target_list[1] = (Atom32) XA_STRING;
#ifdef UTF8_FONT
	target_list[2] = (Atom32) ATOM_UTF8_STRING;
	target_list[3] = (Atom32) xa_text;
	target_list[4] = (Atom32) xa_compound_text;
#else
	target_list[2] = (Atom32) xa_text;
	target_list[3] = (Atom32) xa_compound_text;
#endif

	XChangeProperty (o->Xdisplay, rq->requestor, rq->property, rq->target,
			 32, PropModeReplace,
			 (unsigned char *)target_list,
			 (sizeof (target_list) / sizeof (target_list[0])));
	ev.xselection.property = rq->property;
    } else if (rq->target == XA_STRING
	       || rq->target == xa_compound_text || rq->target == xa_text) {
	if (rq->target == XA_STRING) {
	    style = XStringStyle;
	    target = XA_STRING;
	} else {
	    target = xa_compound_text;
	    style = (rq->target == xa_compound_text) ? XCompoundTextStyle
		: XStdICCTextStyle;
	}
	cl[0] = (char *) o->selection.text;
	XmbTextListToTextProperty (o->Xdisplay, cl, 1, style, &ct);
	XChangeProperty (o->Xdisplay, rq->requestor, rq->property,
			 target, 8, PropModeReplace, ct.value, ct.nitems);
	ev.xselection.property = rq->property;
#ifdef UTF8_FONT
    } else if (rq->target == ATOM_UTF8_STRING) {
	XChangeProperty (o->Xdisplay, rq->requestor, rq->property,
			 ATOM_UTF8_STRING, 8, PropModeReplace,
			 (unsigned char *) o->selection.text, o->selection.len);
	ev.xselection.property = rq->property;
#endif
    }
    XSendEvent (o->Xdisplay, rq->requestor, False, 0, &ev);
}

/* ------------------------------------------------------------------------- *
 *                              MOUSE ROUTINES                               * 
 * ------------------------------------------------------------------------- */

/*
 * return col/row values corresponding to x/y pixel values
 */
/* EXTPROTO */
void            pixel_position (rxvtlib *o, int *x, int *y)
{E_
    *x = Pixel2Col (*x);
/* MAX_IT(*x, 0); MIN_IT(*x, TermWin.ncol - 1); */
    *y = Pixel2Row (*y);
/* MAX_IT(*y, 0); MIN_IT(*y, TermWin.nrow - 1); */
}

/* ------------------------------------------------------------------------- */
/* ARGSUSED */
/* INTPROTO */
void            mouse_tracking (int report, int x, int y, int firstrow,
				int lastrow)
{E_
/* TODO */
}

/* ------------------------------------------------------------------------- *
 *                              DEBUG ROUTINES                               * 
 * ------------------------------------------------------------------------- */
/* ARGSUSED */
/* INTPROTO */
void            debug_PasteIt (unsigned char *data, int nitems)
{E_
/* TODO */
}

/* ------------------------------------------------------------------------- */
#if 0
/* INTPROTO */
void            debug_colors (void)
{E_
    int             color;
    const char     *name[] = { "fg", "bg",
	"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"
    };

    fprintf (stderr, "Color ( ");
    if (rstyle & RS_RVid)
	fprintf (stderr, "rvid ");
    if (rstyle & RS_Bold)
	fprintf (stderr, "bold ");
    if (rstyle & RS_Blink)
	fprintf (stderr, "blink ");
    if (rstyle & RS_Uline)
	fprintf (stderr, "uline ");
    fprintf (stderr, "): ");

    color = GET_FGCOLOR (rstyle);
#ifndef NO_BRIGHTCOLOR
    if (color >= minBrightCOLOR && color <= maxBrightCOLOR) {
	color -= (minBrightCOLOR - minCOLOR);
	fprintf (stderr, "bright ");
    }
#endif
    fprintf (stderr, "%s on ", name[color]);

    color = GET_BGCOLOR (rstyle);
#ifndef NO_BRIGHTCOLOR
    if (color >= minBrightCOLOR && color <= maxBrightCOLOR) {
	color -= (minBrightCOLOR - minCOLOR);
	fprintf (stderr, "bright ");
    }
#endif
    fprintf (stderr, "%s\n", name[color]);
}
#endif

#ifdef USE_XIM
/* EXTPROTO */
void            rxvtlib_setPosition (rxvtlib *o, XPoint * pos)
{E_
    XWindowAttributes xwa;

    XGetWindowAttributes (o->Xdisplay, o->TermWin.vt, &xwa);
    pos->x = Col2Pixel (o->screen.cur.col) + xwa.x;
    pos->y = Height2Pixel ((o->screen.cur.row + 1)) + xwa.y;
}

#endif
