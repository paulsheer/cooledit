/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* propfont.c - editor text drawing for proportional fonts.
   Copyright (C) 1996-2022 Paul Sheer
 */



#include "inspect.h"
#include <config.h>
#include "edit.h"
#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif

/* this file definatively relies on int being 32 bits or more */

int option_long_whitespace = 0;
int cache_width = 0;
int cache_height = 0;

/* must be a multiple of 8 */
#define MAX_LINE_LEN 8192


/* background colors: marked is refers to mouse highlighting, highlighted refers to a found string. */
extern unsigned long edit_abnormal_color, edit_marked_abnormal_color;
extern unsigned long edit_highlighted_color, edit_marked_color;
extern unsigned long edit_normal_background_color;

/* foreground colors */
extern unsigned long edit_normal_foreground_color, edit_bold_color;
extern unsigned long edit_italic_color;

/* cursor color */
extern unsigned long edit_cursor_color;

extern int EditExposeRedraw;
extern int EditClear;

int utf8_to_wchar_t_one_char_safe (C_wchar_t * c, const char *t, int n);


int set_style_color (cache_type s, unsigned long *fg, unsigned long *bg)
{E_
    int fgp, bgp, underlined = 0;
    fgp = s.c.fg;
/* NO_COLOR would give fgp == 255 */
    if (fgp < 0xFF)
	*fg = color_palette (fgp);
    else
	*fg = edit_normal_foreground_color;
    bgp = s.c.bg;
    if (bgp == 0xFE)
	underlined = 1;
    if (bgp < 0xFD)
	*bg = color_palette (bgp);
    else
	*bg = edit_normal_background_color;
    if (!(s.c.style | s.c.fg | s.c.fg))	/* check this first as an optimization */
	return underlined;
    if (s.c.style & MOD_HIGHLIGHTED) {
	*bg = edit_highlighted_color;
    } else if (s.c.style & MOD_ABNORMAL) {
	*bg = edit_abnormal_color;
	if (s.c.style & MOD_MARKED)
	    *bg = edit_marked_abnormal_color;
    } else if (s.c.style & MOD_MARKED) {
	*bg = edit_marked_color;
    }
    if (s.c.style & MOD_BOLD)
	*fg = edit_bold_color;
    if (s.c.style & MOD_ITALIC)
	*fg = edit_italic_color;
    if (s.c.style & MOD_INVERSE) {
	unsigned long t;
	t = *fg;
	*fg = *bg;
	*bg = t;
	if (*bg == COLOR_BLACK)
	    *bg = color_palette (1);
    }
    return underlined;
}


int tab_width = 1;

static inline int next_tab_pos (int x)
{E_
    return x += tab_width - x % tab_width;
}

/* this now properly uses ctypes */
static inline int convert_to_long_printable (C_wchar_t c, C_wchar_t * t, int *n)
{E_
    c &= 0x7FFFFFFFUL;
    if (c == ' ') {
	if (option_long_whitespace) {
	    t[0] = ' ';
	    t[1] = ' ';
            *n = 2;
	    return FONT_PER_CHAR (' ') + FONT_PER_CHAR (' ');
	} else {
	    t[0] = ' ';
            *n = 1;
	    return FONT_PER_CHAR (' ');
	}
    }
    if (c == '\t' || c == '\n') {
        *n = 0;
	return 0;
    }
    if (FONT_PER_CHAR (c)) {
	t[0] = c;
        *n = 1;
	return FONT_PER_CHAR (c);
    }
    if (c > 0xFFFFFF) {
	t[0] = ("0123456789ABCDEF")[(c >> 28) & 0xF];
	t[1] = ("0123456789ABCDEF")[(c >> 24) & 0xF];
	t[2] = ("0123456789ABCDEF")[(c >> 20) & 0xF];
	t[3] = ("0123456789ABCDEF")[(c >> 16) & 0xF];
	t[4] = ("0123456789ABCDEF")[(c >> 12) & 0xF];
	t[5] = ("0123456789ABCDEF")[(c >> 8) & 0xF];
	t[6] = ("0123456789ABCDEF")[(c >> 4) & 0xF];
	t[7] = ("0123456789ABCDEF")[c & 0xF];
	t[8] = 'h';
	*n = 9;
	return FONT_PER_CHAR (t[0]) + FONT_PER_CHAR (t[1]) + FONT_PER_CHAR (t[2]) + FONT_PER_CHAR (t[3]) +
	    FONT_PER_CHAR (t[4]) + FONT_PER_CHAR (t[5]) + FONT_PER_CHAR (t[6]) + FONT_PER_CHAR (t[7]) + FONT_PER_CHAR (t[8]);
    }
    if (c > 0xFFFF) {
	t[0] = ("0123456789ABCDEF")[(c >> 20) & 0xF];
	t[1] = ("0123456789ABCDEF")[(c >> 16) & 0xF];
	t[2] = ("0123456789ABCDEF")[(c >> 12) & 0xF];
	t[3] = ("0123456789ABCDEF")[(c >> 8) & 0xF];
	t[4] = ("0123456789ABCDEF")[(c >> 4) & 0xF];
	t[5] = ("0123456789ABCDEF")[c & 0xF];
	t[6] = 'h';
	*n = 7;
	return FONT_PER_CHAR (t[0]) + FONT_PER_CHAR (t[1]) + FONT_PER_CHAR (t[2]) + FONT_PER_CHAR (t[3]) +
	    FONT_PER_CHAR (t[4]) + FONT_PER_CHAR (t[5]) + FONT_PER_CHAR (t[6]);
    }
    if (c > 0xFF) {
	t[0] = ("0123456789ABCDEF")[(c >> 12) & 0xF];
	t[1] = ("0123456789ABCDEF")[(c >> 8) & 0xF];
	t[2] = ("0123456789ABCDEF")[(c >> 4) & 0xF];
	t[3] = ("0123456789ABCDEF")[c & 0xF];
	t[4] = 'h';
	*n = 5;
	return FONT_PER_CHAR (t[0]) + FONT_PER_CHAR (t[1]) + FONT_PER_CHAR (t[2]) + FONT_PER_CHAR (t[3]) +
	    FONT_PER_CHAR (t[4]);
    }
    if (c > '~') {
	t[0] = ("0123456789ABCDEF")[c >> 4];
	t[1] = ("0123456789ABCDEF")[c & 0xF];
	t[2] = 'h';
	*n = 3;
	return FONT_PER_CHAR (t[0]) + FONT_PER_CHAR (t[1]) + FONT_PER_CHAR (t[2]);
    }
    t[0] = '^';
    t[1] = c + '@';
    *n = 2;
    return FONT_PER_CHAR (t[0]) + FONT_PER_CHAR (t[1]);
}


int propfont_convert_to_long_printable (C_wchar_t c, C_wchar_t * t)
{E_
    int n = 0, r;
    r = convert_to_long_printable (c, t, &n);
    t[n] = 0;
    return r;
}


/* same as above but just gets the length */
static inline int width_of_long_printable (C_wchar_t c)
{E_
    c &= 0x7FFFFFFFUL;
    if (c == ' ') {
	if (option_long_whitespace)
	    return FONT_PER_CHAR (' ') + FONT_PER_CHAR (' ');
	else
	    return FONT_PER_CHAR (' ');
    }
    if (c == '\t' || c == '\n')
	return 0;
    if (FONT_PER_CHAR (c))
	return FONT_PER_CHAR (c);
    if (c > 0xFFFFFF)
	return FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 28) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 24) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 20) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 16) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 12) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 8) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 4) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[c & 0xF]) + FONT_PER_CHAR ((unsigned char) 'h');
    if (c > 0xFFFF)
	return FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 20) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 16) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 12) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 8) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 4) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[c & 0xF]) + FONT_PER_CHAR ((unsigned char) 'h');
    if (c > 0xFF)
	return FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 12) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 8) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[(c >> 4) & 0xF]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[c & 0xF]) + FONT_PER_CHAR ((unsigned char) 'h');
    if (c > '~')
	return FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[c >> 4]) +
	    FONT_PER_CHAR ((unsigned char) ("0123456789ABCDEF")[c & 0xF]) + FONT_PER_CHAR ((unsigned char) 'h');
    return FONT_PER_CHAR ('^') + FONT_PER_CHAR (c + '@');
}


int propfont_width_of_long_printable (C_wchar_t c)
{E_
    return width_of_long_printable (c);
}


int edit_width_of_long_printable (C_wchar_t c)
{E_
    return width_of_long_printable (c);
}

/* returns x pixel pos of char at offset *q with x not more than l */
static int calc_text_pos (WEdit * edit, long b, long *q, int l)
{E_
    int x = 0, xn = 0;
    C_wchar_t c;
    for (;;) {
	c = edit_get_wide_byte (edit, b);
	switch (c) {
	case -1:
/* no character since used up by a multi-byte sequence */
	    break;
	case '\n':
	    *q = b;
	    if (x > edit->max_column)
		edit->max_column = x;
	    return x;
	case '\t':
	    xn = next_tab_pos (x);
	    break;
	default:
	    xn = x + width_of_long_printable (c);
	    break;
	}
	if (xn > l)
	    break;
	x = xn;
	b++;
    }
    *q = b;
    if (x > edit->max_column)
	edit->max_column = x;
    return x;
}


/* calcs pixel length of the line beginning at b up to upto */
static int calc_text_len (WEdit * edit, long b, long upto)
{E_
    int x = 0;
    C_wchar_t c;
    for (;;) {
	if (b == upto) {
	    if (x > edit->max_column)
		edit->max_column = x;
	    return x;
	}
	c = edit_get_wide_byte (edit, b);
	switch (c) {
	case -1:
/* no character since used up by a multi-byte sequence */
	    break;
	case '\n':{
		if (x > edit->max_column)
		    edit->max_column = x;
		return x;
	    }
	case '\t':
	    x = next_tab_pos (x);
	    break;
	default:
	    x += width_of_long_printable (c);
	    break;
	}
	b++;
    }
}

/* If pixels is zero this returns the count of pixels from current to upto. */
/* If upto is zero returns index of pixels across from current. */
long edit_move_forward3 (WEdit * edit, long current, int pixels, long upto)
{E_
    CPushFont ("editor", 0);
    if (upto) {
	current = calc_text_len (edit, current, upto);
    } else if (pixels) {
	long q;
	calc_text_pos (edit, current, &q, pixels);
	current = q;
    }
    CPopFont ();
    return current;
}

extern int column_highlighting;

/* gets the characters style (eg marked, highlighted) from its position in the edit buffer */
static inline cache_type get_style_fast (WEdit * edit, long q, C_wchar_t c)
{E_
    cache_type s;
    unsigned int fg, bg;
    s.c.ch = s._style = 0;
    if (c != '\n' && c != '\t')
	if (!FONT_PER_CHAR(c))
	    s.c.style = MOD_ABNORMAL;
    edit_get_syntax_color (edit, q, (int *) &fg, (int *) &bg);
    s.c.fg = fg;
    s.c.bg = bg;
    return s;
}

/* gets the characters style (eg marked, highlighted) from its position in the edit buffer */
static inline cache_type get_style (WEdit * edit, long q, C_wchar_t c, long m1, long m2, int x)
{E_
    cache_type s;
    unsigned int fg, bg;
    s.c.ch = s._style = 0;
    if (q == edit->curs1)
	s.c.style |= MOD_CURSOR;
    if (q >= m1 && q < m2) {
	if (column_highlighting) {
	    if ((x >= edit->column1 && x < edit->column2)
		|| (x >= edit->column2 && x < edit->column1))
		s.c.style |= MOD_INVERSE;
	} else {
	    s.c.style |= MOD_MARKED;
	}
    }
    if (q == edit->bracket)
	s.c.style |= MOD_BOLD;
    if (q >= edit->found_start && q < edit->found_start + edit->found_len)
	s.c.style |= MOD_HIGHLIGHTED;
    if (c != '\n' && c != '\t')
        if (!FONT_PER_CHAR(c))
	    s.c.style |= MOD_ABNORMAL;
    edit_get_syntax_color (edit, q, (int *) &fg, (int *) &bg);
    s.c.fg = fg;
    s.c.bg = bg;
    return s;
}

static void convert_text (WEdit * edit, long bol, long q, cache_type * p, cache_type * eol, int x, int x_max, int row, struct _book_mark **book_mark_all, int all_bookmarks)
{E_
    C_wchar_t c;
    cache_type s;
    long m1, m2, last;
    int i, n;
    C_wchar_t text[12];
    int n_bookmarks = 0;
    int column_marker = -1;
    struct _book_mark *book_mark_colors[10];
    eval_marks (edit, &m1, &m2);

    for (i = 0; i < all_bookmarks; i++) {
        if (!book_mark_all[i]->height)
            book_mark_colors[n_bookmarks++] = book_mark_all[i];
        if (book_mark_all[i]->column_marker)
            column_marker = book_mark_all[i]->column_marker - 1;        /* column 1 is index 0 */
    }

#if 0   /* this 'last' calculation is bogus for characters like u115BD which is very narrow yet consumes 4 bytes */
    last = q + (x_max - x) / 2 + 2;	/* for optimization, we say that the last character 
					   of this line cannot have an offset greater than this.
					   This can be used to rule out uncommon text styles,
					   like a character with a cursor, or selected text */
#else
    (void) last;
#endif

    memset(text, '\0', sizeof(text));

    if (n_bookmarks) {
	int the_end = 0, book_mark_cycle = 0;
	for (;;) {
	    c = edit_get_wide_byte (edit, q);
	    if (!the_end) {
		*p = get_style (edit, q, c, m1, m2, x);
                if (column_marker != -1 && bol + column_marker == q)
                    p->c.style |= MOD_UNDERCARET;
            }
	    if (the_end)
		p->c.ch = p->_style = 0;
	    book_mark_cycle = (book_mark_cycle + 1) % n_bookmarks;
	    p->c.fg = book_mark_colors[book_mark_cycle]->c << 8;
	    p->c.bg = book_mark_colors[book_mark_cycle]->c;
	    switch (c) {
	    case -1:
/* no character since used up by a multi-byte sequence */
		break;
	    case '\n':
		the_end = 1;
		c = ' ';
		q--;
		goto the_default;
	    case '\t':
		if (FIXED_FONT) {
		    int t;
		    t = next_tab_pos (x);
		    t = min (t, x_max);
		    s = *p;
		    s.c.ch = ' ';
		    while (x < t) {
			x += FONT_PER_CHAR(' ');
			*p++ = s;
			if (p >= eol)
			    goto end_loop;
		    }
		} else {
		    (p++)->c.ch = '\t';
		    if (p >= eol)
			goto end_loop;
		    x = next_tab_pos (x);
		}
		break;
	    default:
	      the_default:
		x += convert_to_long_printable (c, text, &n);
		s = *p;
                for (i = 0; i < n; i++) {
                    s.c.ch = text[i];
                    *p++ = s;
                }
		if (p >= eol)
		    goto end_loop;
		break;
	    }
	    if (x >= x_max)
		break;
	    q++;
	}
#if 0
    } else if ((m2 < q || m1 > last) && (edit->curs1 < q || edit->curs1 > last) && \
	       (edit->found_start + edit->found_len < q || edit->found_start > last) &&
	       (edit->bracket < q || edit->bracket > last)) {
#else
    } else if ((m2 < q) && (edit->curs1 < q) && (edit->found_start + edit->found_len < q) && (edit->bracket < q)) {
#endif
	for (;;) {
	    c = edit_get_wide_byte (edit, q);
	    *p = get_style_fast (edit, q, c);
            if (column_marker != -1 && bol + column_marker == q)
                p->c.style |= MOD_UNDERCARET;
	    switch (c) {
	    case -1:
/* no character since used up by a multi-byte sequence */
		break;
	    case '\n':
		(p++)->c.ch = ' ';
		if (p >= eol)
		    goto end_loop;
		p->c.ch = p->_style = 0;
		if (x > edit->max_column)
		    edit->max_column = x;
		return;
	    case '\t':
		if (FIXED_FONT) {
		    int t;
		    t = next_tab_pos (x);
		    t = min (t, x_max);
		    s = *p;
		    s.c.ch = ' ';
		    while (x < t) {
			x += FONT_PER_CHAR(' ');
			*p++ = s;
			if (p >= eol)
			    goto end_loop;
		    }
		} else {
		    (p++)->c.ch = '\t';
		    if (p >= eol)
			goto end_loop;
		    x = next_tab_pos (x);
		}
		break;
	    default:
		x += convert_to_long_printable (c, text, &n);
		s = *p;
                for (i = 0; i < n; i++) {
                    s.c.ch = text[i];
                    *p++ = s;
                }
		if (p >= eol)
		    goto end_loop;
		break;
	    }
	    if (x >= x_max)
		break;
	    q++;
	}
    } else {
	for (;;) {
	    c = edit_get_wide_byte (edit, q);
	    *p = get_style (edit, q, c, m1, m2, x);
            if (column_marker != -1 && bol + column_marker == q)
                p->c.style |= MOD_UNDERCARET;
	    switch (c) {
	    case -1:
/* no character since used up by a multi-byte sequence */
		break;
	    case '\n':
		(p++)->c.ch = ' ';
		if (p >= eol)
		    goto end_loop;
		p->c.ch = p->_style = 0;
		if (x > edit->max_column)
		    edit->max_column = x;
		return;
	    case '\t':
		if (FIXED_FONT) {
		    int t;
		    t = next_tab_pos (x);
		    t = min (t, x_max);
		    s = *p;
		    s.c.ch = ' ';
		    while (x < t) {
			x += FONT_PER_CHAR(' ');
			*p++ = s;
			if (p >= eol)
			    goto end_loop;
		    }
		} else {
		    (p++)->c.ch = '\t';
		    if (p >= eol)
			goto end_loop;
		    x = next_tab_pos (x);
		}
		break;
	    default:
		x += convert_to_long_printable (c, text, &n);
		s = *p;
                for (i = 0; i < n; i++) {
                    s.c.ch = text[i];
                    *p++ = s;
                }
		if (p >= eol)
		    goto end_loop;
		break;
	    }
	    if (x >= x_max)
		break;
	    q++;
	}
    }
  end_loop:
    if (x > edit->max_column)
	edit->max_column = x;
    p->c.ch = p->_style = 0;
}

static void edit_set_cursor (Window win, int x, int y, int bg, int fg, int width, C_wchar_t t, int style)
{E_
    CSetColor (edit_cursor_color);
    if (style & MOD_REVERSE)
	CLine (win, x + width - 1, y + FONT_OVERHEAD, x + width - 1, y + FONT_HEIGHT - 1);	/* non focussed cursor form */
    else
	CLine (win, x, y + FONT_OVERHEAD, x, y + FONT_HEIGHT - 1);	/* non focussed cursor form */
    CLine (win, x, y + FONT_OVERHEAD, x + width - 1, y + FONT_OVERHEAD);
    set_cursor_position (win, x, y, width, FONT_HEIGHT, CURSOR_TYPE_EDITOR, t, bg, fg, style);	/* widget library's flashing cursor */
}

static inline int next_tab (int x, int scroll_right)
{E_
    return next_tab_pos (x - scroll_right - EDIT_TEXT_HORIZONTAL_OFFSET) - x + scroll_right + EDIT_TEXT_HORIZONTAL_OFFSET;
}

static int draw_tab (Window win, int x, int y, cache_type s, int scroll_right)
{E_
    int l;
    unsigned long fg, bg;
    l = next_tab (x, scroll_right);
    set_style_color (s, &fg, &bg);
    CSetColor (bg);
    CRectangle (win, x, y + FONT_OVERHEAD, l, FONT_HEIGHT);
/* if we printed a cursor: */
    if (s.c.style & MOD_CURSOR)
	edit_set_cursor (win, x, y, bg, fg, FONT_PER_CHAR(' '), ' ', s.c.style);
    return x + l;
}


static inline void draw_space (Window win, int x, int y, cache_type s, int l)
{E_
    unsigned long fg, bg;
    set_style_color (s, &fg, &bg);
    CSetColor (bg);
    CRectangle (win, x, y + FONT_OVERHEAD, l, FONT_HEIGHT);
/* if we printed a cursor: */
    if (s.c.style & MOD_CURSOR)
	edit_set_cursor (win, x, y, bg, fg, FONT_PER_CHAR(' '), ' ', s.c.style);
}


static int draw_string (Window win, int x, int y, cache_type s, XChar2b *text, C_wchar_t *textwc, int length)
{E_
    unsigned long fg, bg;
    int underlined, l;
    underlined = set_style_color (s, &fg, &bg);
    CSetBackgroundColor (bg);
    CSetColor (fg);
    CImageTextWC (win, x + FONT_OFFSET_X, y + FONT_OFFSET_Y, text, textwc, length);
    l = CImageTextWidthWC (text, textwc, length);
    if (underlined) {
	int i, h, inc;
	inc = FONT_MEAN_WIDTH * 2 / 3;
	CSetColor (color_palette (18));
	h = (x / inc) & 1;
	CLine (win, x, y + FONT_HEIGHT + FONT_OVERHEAD - 1 - h, x + min (l, inc - (x % inc) - 1), y + FONT_HEIGHT + FONT_OVERHEAD - 1 - h);
	h = h ^ 1;
	for (i = inc - min (l, (x % inc)); i < l; i += inc) {
	    CLine (win, x + i, y + FONT_HEIGHT + FONT_OVERHEAD - 1 - h, x + min (l, i + inc - 1), y + FONT_HEIGHT + FONT_OVERHEAD - 1 - h);
	    h = h ^ 1;
	}
    }
    if (s.c.style & MOD_CURSOR)
	edit_set_cursor (win, x, y, bg, fg, FONT_PER_CHAR(*textwc), *textwc, s.c.style);
    return x + l;
}

#define STYLE_DIFF (cache->_style != line->_style || cache->c.ch != line->c.ch \
	    || ((cache->c.style | line->c.style) & MOD_CURSOR) \
	    || !(cache->c.ch | cache->_style) || !(line->c.ch | line->_style))

int get_ignore_length (cache_type *cache, cache_type *line)
{E_
    int i;
    for (i = 0; i < cache_width; i++, line++, cache++) {
	if (STYLE_DIFF)
	    return i;
    }
    return cache_width;
}

static inline size_t lwstrnlen (const cache_type * s, size_t count)
{E_
    const cache_type *sc;
    for (sc = s; count-- && (sc->c.ch | sc->_style) != 0; ++sc);
    return sc - s;
}

static inline size_t lwstrlen (const cache_type * s)
{E_
    const cache_type *sc;
    for (sc = s; (sc->c.ch | sc->_style) != 0; ++sc);
    return sc - s;
}

int get_ignore_trailer (cache_type *cache, cache_type *line, int length)
{E_
    int i;
    int cache_len, line_len;
    cache_len = lwstrnlen (cache, cache_width);
    line_len = lwstrlen (line);

    if (line_len > cache_len)
	for (i = line_len - 1; i >= cache_len && i >= length; i--) {
	    if (line[i].c.ch == ' ' && !(line[i].c.style | line[i].c.fg | line[i].c.bg))
		continue;
	    return i + 1;
	}
    for (i = cache_len - 1, line = line + i, cache = cache + i; i > length; i--, line--, cache--)
	if (STYLE_DIFF)
	    return i + 1;

    return length + 1;
}

/* erases trailing bit of old line if a new line is printed over a longer old line */
static void cover_trail (Window win, int x_start, int x_new, int x_old, int y, int cover_up)
{E_
    if (x_new < EDIT_TEXT_HORIZONTAL_OFFSET)
	x_new = EDIT_TEXT_HORIZONTAL_OFFSET;
    if (x_new < x_old || cover_up) {	/* no need to print */
	CSetColor (edit_normal_background_color);
	CRectangle (win, x_new, y + FONT_OVERHEAD, x_old - x_new, FONT_HEIGHT + (FONT_OVERHEAD != 0 && !FIXED_FONT));
    } else {
	CSetColor (edit_normal_background_color);
    }
/* true type fonts print stuff out of the bounding box (aaaaaaaaarrrgh!!) */
    if (cover_up) {
	if (FONT_OVERHEAD && x_new > EDIT_TEXT_HORIZONTAL_OFFSET)
	    CLine (win, max (x_start, EDIT_TEXT_HORIZONTAL_OFFSET), y + FONT_HEIGHT + FONT_OVERHEAD, x_old - 1, y + FONT_HEIGHT + FONT_OVERHEAD);
    } else if (!FIXED_FONT) {
	if (FONT_OVERHEAD && x_new > EDIT_TEXT_HORIZONTAL_OFFSET)
	    CLine (win, max (x_start, EDIT_TEXT_HORIZONTAL_OFFSET), y + FONT_HEIGHT + FONT_OVERHEAD, x_new - 1, y + FONT_HEIGHT + FONT_OVERHEAD);
    }
}

#define NOT_VALID (-2000000000)

struct cache_line {
    int x0, x1, y;
    unsigned int bookmarkhash;
    cache_type *data;
};

struct cache_line *cache_lines = 0;

void edit_free_cache_lines (void)
{E_
    int i;
    if (cache_lines) {
	for (i = 0; i < cache_height; i++)
	    free (cache_lines[i].data);
	free (cache_lines);
	cache_lines = 0;
    }
}

static void edit_realloc_cache_lines (int width, int height)
{E_
    if (width > cache_width || height > cache_height) {
	int i;
	edit_free_cache_lines ();
	cache_width = max (width + 10, cache_width);
	cache_height = max (height + 10, cache_height);
	cache_lines = malloc (sizeof (struct cache_line) * cache_height);
	memset (cache_lines, 0, sizeof (struct cache_line) * cache_height);
	for (i = 0; i < cache_height; i++) {
	    cache_lines[i].data = malloc (sizeof (cache_type) * (cache_width + 1));
	    memset (cache_lines[i].data, 0, sizeof (cache_type) * (cache_width + 1));
	    cache_lines[i].x0 = NOT_VALID;
	    cache_lines[i].x1 = 10000;
	    cache_lines[i].y = -1;
	    cache_lines[i].bookmarkhash = 0;
	}
    }
}

/*

One would like to think one could use "UnicodeData....txt" to work out what is
left-to-right or right-to-left text. Unfortunately not, because this list goes on a per-character
style, ignoring the concept of the "block". In other words, consider an Arabic or Hebrew block: not
all code-points are defined, but OBVIOUSLY if a code point is added in the Hebrew block is is NOT
going to be the single one in the block that is Left-to-Right -- that would be extremely unlikely.
Close inspection of all the right-to-left characters while cross-referencing the block names reveals
there are only five ranges of characters that are right to left. These five ranges cover all
right-to-left character blocks, and all ranges of characters where a newly-defined right-to-left
code point is likely to be inserted.

Right-to-left blocks:

0590..05FF; Hebrew
0600..06FF; Arabic
0700..074F; Syriac
0750..077F; Arabic Supplement
0780..07BF; Thaana
07C0..07FF; NKo
0800..083F; Samaritan
0840..085F; Mandaic
0860..086F; Syriac Supplement
08A0..08FF; Arabic Extended-A

FB1D..FB4F; Alphabetic Presentation Forms (hebrew part)
FB50..FDFF; Arabic Presentation Forms-A
FE70..FEFF; Arabic Presentation Forms-B

10800..1083F; Cypriot Syllabary
10840..1085F; Imperial Aramaic
10860..1087F; Palmyrene
10880..108AF; Nabataean
108E0..108FF; Hatran
10900..1091F; Phoenician
10920..1093F; Lydian
10980..1099F; Meroitic Hieroglyphs
109A0..109FF; Meroitic Cursive
10A00..10A5F; Kharoshthi
10A60..10A7F; Old South Arabian
10A80..10A9F; Old North Arabian
10AC0..10AFF; Manichaean
10B00..10B3F; Avestan
10B40..10B5F; Inscriptional Parthian
10B60..10B7F; Inscriptional Pahlavi
10B80..10BAF; Psalter Pahlavi
10C00..10C4F; Old Turkic
10C80..10CFF; Old Hungarian

1E800..1E8DF; Mende Kikakui
1E900..1E95F; Adlam
1EE00..1EEFF; Arabic Mathematical Alphabetic Symbols



optimized comparator macro: most common text will stop at the first >=.

This macro is correct for every code point in unicode 10 except for:

    200F;RIGHT-TO-LEFT MARK;Cf;0;R;;;;;N;;;;;

*/

#define IS_LEVANT(c)  ((c) >= 0x00590 && ((c) <= 0x008FF || \
                      ((c) >= 0x0FB1D && ((c) <= 0x0FDFF || \
                      ((c) >= 0x0FE70 && ((c) <= 0x0FEFF || \
                      ((c) >= 0x10800 && ((c) <= 0x10CFF || \
                      ((c) >= 0x1E800 && ((c) <= 0x1EEFF))))))))))

int option_reverse_levant = 1;

static void reverse_text (cache_type * line)
{E_
    int i, n;
    if (option_reverse_levant) {
	while (line->c.ch | line->_style) {
	    while (!IS_LEVANT (line->c.ch) && (line->c.ch | line->_style))
		line++;
	    for (n = 0; (IS_LEVANT (line[n].c.ch) || (line[n].c.ch == ' ')) && (line[n].c.ch | line[n]._style); n++);
	    while (n && !IS_LEVANT (line[n - 1].c.ch))
		n--;
	    n--;
	    for (i = 0; i < (n + 1) / 2; i++) {
		cache_type t;
		t = line[i];
		line[i] = line[n - i];
		line[i].c.style |= MOD_REVERSE;
		line[n - i] = t;
		line[n - i].c.style |= MOD_REVERSE;
	    }
	    line += n + 1;
	}
    }
}


#define HASH401(c)      ((((c) + 9) * ((c) + 2) * 401) >> 1)

static unsigned int book_marks_calc_hash (struct _book_mark **book_marks, int n_book_marks, int undercaret_offset)
{E_
    unsigned int v, r = 3182;
    int i;
    if (!n_book_marks)
        return 0;
    for (i = 0; i < n_book_marks; i++) {
        struct _book_mark *b;
        const char *p;
        b = book_marks[i];

#define ADD_HASH(c) \
        v = (c); \
	r ^= v; \
	r ^= HASH401 (r % 3259);

        ADD_HASH(b->c);
        ADD_HASH(b->height);
        ADD_HASH(b->column_marker);
        ADD_HASH(undercaret_offset);
        if ((p = b->text)) {
            for (;;) {
		if (!p[0])
                    break;
                ADD_HASH (((int) p[0]) | ((int) p[1] << 8));
		if (!p[1])
                    break;
                p += 2;
            }
        }
    }
    return r;
}

void edit_draw_proportional_invalidate (int row_start, int row_end, int x_max)
{E_
    int i;
    for (i = row_start; i <= row_end && i < cache_height; i++) {
	cache_lines[i].x0 = NOT_VALID;
	cache_lines[i].x1 = x_max;
	cache_lines[i].y = -1;
	cache_lines[i].bookmarkhash = 0;
    }
}

int edit_draw_proportional (void *data,
	                        converttext_cb_t converttext,
                                calctextpos_cb_t calctextpos,
				int scroll_right,
				Window win,
				int x_max,
				long b,
				int row,
				int y,
				int x_offset,
				int tabwidth,
                                struct _book_mark **book_marks,
                                int n_book_marks)
{E_
    static Window last = 0;
    cache_type style, line[MAX_LINE_LEN], *p, *eol;
    XChar2b text[128];
    C_wchar_t textwc[128];
    int x0, x, ignore_text = 0, ignore_trailer = 2000000000, j, i, undercaret_offset = -1, drawn_extents_y = 0;
    long q;
    unsigned int bookmarkhash = 0;

    tab_width = tabwidth;
    if (option_long_whitespace)
	tab_width = tabwidth *= 2;

    x_max -= 3;

    edit_realloc_cache_lines (x_max / 3, row + 1);

/* if its not the same window, reset the screen rememberer */
    if (last != win || !win) {
	last = win;
        for (i = 0; i < cache_height; i++) {
	    cache_lines[i].x0 = NOT_VALID;
	    cache_lines[i].x1 = x_max;
	    cache_lines[i].y = -1;
	    cache_lines[i].bookmarkhash = 0;
        }
	if (!win)
	    return 0;
    }

    drawn_extents_y = y + FONT_PIX_PER_LINE;

/* get point to start drawing */
    x0 = (*calctextpos) (data, b, &q, -scroll_right + x_offset);
/* q contains the offset in the edit buffer */

/* translate this line into printable characters with a style (=color) high byte */
    for (i = 0; i < MAX_LINE_LEN; i += 8) {
	line[i]._style = line[i].c.ch = 0;
	line[i + 1]._style = line[i + 1].c.ch = 0;
	line[i + 2]._style = line[i + 2].c.ch = 0;
	line[i + 3]._style = line[i + 3].c.ch = 0;
	line[i + 4]._style = line[i + 4].c.ch = 0;
	line[i + 5]._style = line[i + 5].c.ch = 0;
	line[i + 6]._style = line[i + 6].c.ch = 0;
	line[i + 7]._style = line[i + 7].c.ch = 0;
    }
    (*converttext) (data, b, q, line, &line[MAX_LINE_LEN - 16], x0, x_max - scroll_right - EDIT_TEXT_HORIZONTAL_OFFSET, row, book_marks, n_book_marks);
    reverse_text (line);

/* adjust for the horizontal scroll and border */
    x0 += scroll_right + EDIT_TEXT_HORIZONTAL_OFFSET;
    x = x0;

/* is some of the line identical to that already printed so that we can ignore it? */
    if (!EditExposeRedraw) {
	if (cache_lines[row].x0 == x0 && cache_lines[row].y == y && row < cache_height) {	/* i.e. also  && cache_lines[row].x0 != NOT_VALID */
	    ignore_text = get_ignore_length (cache_lines[row].data, line);
	    if (FIXED_FONT)
		ignore_trailer = get_ignore_trailer (cache_lines[row].data, line, ignore_text);
	}
    }
    p = line;
    eol = &line[MAX_LINE_LEN - 1];
    j = 0;
    while (p->c.ch | p->_style) {
	if (p->c.style & MOD_UNDERCARET)
            undercaret_offset = x;
	if (p->c.style & MOD_TAB) {
	    draw_space (win, x, y, *p, p->c.ch);
	    x += p->c.ch;
	    j++;
	    if (p++ == eol)
		goto done_loop;
	} else if (p->c.ch == '\t') {
	    j++;
	    if (j > ignore_text && j < ignore_trailer + 1)
		x = draw_tab (win, x, y, *p, scroll_right);
	    else
		x += next_tab (x, scroll_right);
	    if (p++ == eol)
		goto done_loop;
	} else {
	    style = *p;
	    style.c.ch = 0;
	    i = 0;
	    do {
		text[i].byte2 = (unsigned char) p->c.ch;
		text[i].byte1 = (unsigned char) (p->c.ch >> 8);
		textwc[i++] = (p++)->c.ch;
		if (p == eol)
		    goto done_loop;
		j++;
		if (j == ignore_text || j == ignore_trailer)
		    break;
	    } while (i < 128 && p->c.ch && style._style == p->_style && p->c.ch != '\t');
	    if (j > ignore_text && j < ignore_trailer + 1) {
		x = draw_string (win, x, y, style, text, textwc, i);
	    } else {
		x += CImageTextWidthWC (text, textwc, i);
	    }
	}
    }
  done_loop:

    x = min (x, x_max);

    if (!EditExposeRedraw || EditClear) {
        if (cache_lines[row].y == y)
	    cover_trail (win, x0, x, cache_lines[row].x1, y, 0);
        else if (cache_lines[row].y != y)
            cover_trail (win, x_offset, x, x_max, y, 1);
    }

    bookmarkhash = book_marks_calc_hash (book_marks, n_book_marks, undercaret_offset);

    if (EditExposeRedraw || cache_lines[row].bookmarkhash != bookmarkhash || cache_lines[row].y != y) {
        int i, h;
        for (h = 0, i = 0; i < n_book_marks; i++) {
            int H, Y;
            H = book_marks[i]->height;
            drawn_extents_y += H;
            Y = y + FONT_PIX_PER_LINE + h;
            if (!H) {
                /* overline bookmark */
            } else if (BLANK_UNDERLINING_BOOKMARK (book_marks[i])) {
                if (!EditExposeRedraw || EditClear) {
	            CSetColor (edit_normal_background_color);
                    CLine (win, EDIT_TEXT_HORIZONTAL_OFFSET, Y, x_max - 1, Y);
                    CLine (win, EDIT_TEXT_HORIZONTAL_OFFSET, Y + H, x_max - 1, Y + H);
                }
                CSetColor (color_palette ((book_marks[i]->c >> 0) & 0xFF));
                CRectangle (win, EDIT_TEXT_HORIZONTAL_OFFSET, Y + 1, x_max - EDIT_TEXT_HORIZONTAL_OFFSET, H - 1);
            } else if (TEXT_UNDERLINING_BOOKMARK (book_marks[i])) {
                if (!EditExposeRedraw || EditClear) {
	            CSetColor (edit_normal_background_color);

/*CSetColor (color_palette(9+2));
*/

                    CLine (win, EDIT_TEXT_HORIZONTAL_OFFSET, Y, x_max - 1, Y);
                    CLine (win, EDIT_TEXT_HORIZONTAL_OFFSET, Y + H, x_max - 1, Y + H);
                    CLine (win, EDIT_TEXT_HORIZONTAL_OFFSET, Y + 1, EDIT_TEXT_HORIZONTAL_OFFSET + 1, Y + 1);
                    CLine (win, EDIT_TEXT_HORIZONTAL_OFFSET, Y + H - 1, EDIT_TEXT_HORIZONTAL_OFFSET + 1, Y + H - 1);
                    CLine (win, x_max - 3, Y + 1, x_max - 1, Y + 1);
                    CLine (win, x_max - 3, Y + H - 1, x_max - 1, Y + H - 1);
                    CLine (win, EDIT_TEXT_HORIZONTAL_OFFSET, Y + 2, EDIT_TEXT_HORIZONTAL_OFFSET, Y + H - 2);
                    CRectangle (win, x_max - 2, Y + 2, 2, H - 3);
                }
                CSetColor (color_palette ((book_marks[i]->c >> 0) & 0xFF));

/*CSetColor (color_palette(18));
*/

                CRectangle (win, EDIT_TEXT_HORIZONTAL_OFFSET + 1, Y + 2, x_max - EDIT_TEXT_HORIZONTAL_OFFSET - 2, H - 3);
                CLine (win, EDIT_TEXT_HORIZONTAL_OFFSET + 2, Y + 1, x_max - 3, Y + 1);
                CLine (win, EDIT_TEXT_HORIZONTAL_OFFSET + 2, Y + H - 1, x_max - 3, Y + H - 1);

                CSetColor (edit_normal_background_color);
                if (undercaret_offset == -1)
                    undercaret_offset = book_marks[i]->undercaret_offset;
                else
                    book_marks[i]->undercaret_offset = undercaret_offset;
                CLine (win, undercaret_offset - 3, Y + 4, undercaret_offset - 3, Y + 4);
                CLine (win, undercaret_offset - 2, Y + 3, undercaret_offset - 2, Y + 4);
                CLine (win, undercaret_offset - 1, Y + 2, undercaret_offset - 1, Y + 4);
                CLine (win, undercaret_offset + 0, Y + 1, undercaret_offset + 0, Y + 4);
                CLine (win, undercaret_offset + 1, Y + 2, undercaret_offset + 1, Y + 4);
                CLine (win, undercaret_offset + 2, Y + 3, undercaret_offset + 2, Y + 4);
                CLine (win, undercaret_offset + 3, Y + 4, undercaret_offset + 3, Y + 4);

                CPushFont("bookmark");
                CSetColor (edit_normal_background_color);
                CSetBackgroundColor (color_palette ((book_marks[i]->c >> 0) & 0xFF));
                CImageString (win, EDIT_TEXT_HORIZONTAL_OFFSET + 1 + FONT_OFFSET_X + 3, Y + FONT_OFFSET_Y + 5, book_marks[i]->text);
                CPopFont();
            } else {     /* don't know what this is, so just clear the area */
                if (!EditExposeRedraw || EditClear) {
                    CRectangle (win, EDIT_TEXT_HORIZONTAL_OFFSET, Y, x_max - EDIT_TEXT_HORIZONTAL_OFFSET, h + H + FONT_OVERHEAD);
                }
            }
            h += H;
        }
    }

    memcpy (&(cache_lines[row].data[ignore_text]),
	    &(line[ignore_text]),
	 (min (j, cache_width) - ignore_text) * sizeof (cache_type));

    cache_lines[row].data[min (j, cache_width)].c.ch = 0;
    cache_lines[row].data[min (j, cache_width)]._style = 0;

    cache_lines[row].x0 = x0;
    cache_lines[row].x1 = x;
    cache_lines[row].y = y;
    cache_lines[row].bookmarkhash = bookmarkhash;
    if (EditExposeRedraw)
	last = 0;
    else
	last = win;
    return drawn_extents_y;
}

int edit_row_to_ypixel (WEdit * edit, int row_search);

int edit_draw_this_line_proportional (WEdit * edit, long b, int row, int y, int start_column, int end_column, struct _book_mark **book_marks, int n_book_marks)
{E_
    int fg, bg;
    if (row < 0 || row >= edit->num_widget_lines)
	return 0;

    if (row + edit->start_line > edit->total_lines)
	b = edit->last_byte + 1;		/* force b out of range of the edit buffer for blanks lines */

    if (end_column > CWidthOf (edit->widget))
	end_column = CWidthOf (edit->widget);

    edit_get_syntax_color (edit, b - 1, &fg, &bg);

    y = edit_draw_proportional (edit,
			    (converttext_cb_t) convert_text,
			    (calctextpos_cb_t) calc_text_pos,
			    edit->start_col, CWindowOf (edit->widget),
			    end_column, b, row, y,
			    EditExposeRedraw ? start_column : 0, FONT_PER_CHAR(' ') * TAB_SIZE,
                            book_marks, n_book_marks);

    return y;
}


/*********************************************************************************/
/*         The remainder is for the text box widget                              */
/*********************************************************************************/

static int calc_text_pos_str (unsigned char *text, long b, long *q, int l)
{E_
    int x = 0, xn = 0;
    C_wchar_t c = 0, d;
    for (;;) {
        int consumed;
	d = c;
	c = (unsigned char) text[b];
	switch (c) {
	case '\0':
	case '\n':
	    *q = b;
	    return x;
	case '\t':
	    xn = next_tab_pos (x);
	    if (xn > l)
	        goto done;
	    b++;
	    break;
	case '\r':
	    b++;
	    break;
	case '\b':
	    if (d)
		xn = x - FONT_PER_CHAR(d);
	    if (xn > l)
	        goto done;
	    b++;
	    break;
	default:
            consumed = utf8_to_wchar_t_one_char_safe (&c, (const char *) text + b, strnlen((const char *) text + b, 6));
	    if (!FONT_PER_CHAR (c))
		c = ' ';
	    xn = x + FONT_PER_CHAR(c);
	    if (xn > l)
	        goto done;
	    b += consumed;
	    break;
	}
	x = xn;
    }
  done:
    *q = b;
    return x;
}

int prop_font_strcolmove (unsigned char *str, int i, int column)
{E_
    long q;
    CPushFont ("editor", 0);
    calc_text_pos_str (str, i, &q, column * FONT_MEAN_WIDTH);
    CPopFont ();
    return q;
}


/* b is the beginning of the line. l is the length in pixels up to a point 
   on some character which is unknown. The character pos is returned in
   *q and the characters pixel x pos from b is return'ed. */
int calc_text_pos2 (CWidget * w, long b, long *q, int l)
{E_
    int r;
    CStr s;
    s = w->textbox_funcs ? ((*w->textbox_funcs->textbox_text_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2)) : w->text;
    CPushFont ("editor", 0);
    r = calc_text_pos_str ((unsigned char *) s.data, b, q, l);
    CPopFont ();
    return r;
}

int highlight_this_line;

/* this is for the text widget (i.e. nroff formatting) */
void convert_text2 (CWidget * w, long b, long q, cache_type * line, cache_type * eol, int x, int x_max, int row, struct _book_mark *bookmarks_not_used, int n_bookmarks_not_used)
{E_
    C_wchar_t c = 0, d;
    cache_type *p, *bol;
    long m1, m2;
    CStr str;

    str = w->textbox_funcs ? ((*w->textbox_funcs->textbox_text_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2)) : w->text;

    m1 = min (w->mark1, w->mark2);
    m2 = max (w->mark1, w->mark2);

    p = line;
    bol = line;
    p->c.ch = p->_style = 0;
    for (;;) {
        int consumed;
	d = c;
	c = (unsigned char) str.data[q];
	p[1].c.ch = p[1]._style = 0;
	p->c.fg = p->c.bg = 0xFF;	/* default background colors */
	if (highlight_this_line)
	    p->c.style |= MOD_HIGHLIGHTED;
	if (q >= m1 && q < m2)
	    p->c.style |= MOD_MARKED;
	switch (c) {
	case '\0':
	case '\n':
	    (p++)->c.ch = ' ';
	    if (p >= eol)
		goto end_loop;
	    if (highlight_this_line) {
		q--;
		x += FONT_PER_CHAR (' ');
	    } else
		return;
	    q++;
	    break;
	case '\t':
#if 0
            if ((w->options & TEXTBOX_TAB_AS_ARROW)) {
		(p++)->c.ch = '\t';
		if (p >= eol)
		    goto end_loop;
#define ARROW_WIDTH     (FONT_HEIGHT * 4)
		x += ARROW_WIDTH;
            } else
#endif
            if (FIXED_FONT) {
		cache_type s;
		int i;
		i = next_tab_pos (x) - x;
		x += i;
		s = *p;
		while (i > 0) {
		    i -= FONT_PER_CHAR (' ');
#warning backport this fix??
		    *p = s;
		    (p++)->c.ch = ' ';
		    if (p >= eol)
			goto end_loop;
		    p->c.ch = p->_style = 0;
		}
	    } else {
		(p++)->c.ch = '\t';
		if (p >= eol)
		    goto end_loop;
		x = next_tab_pos (x);
	    }
	    q++;
	    break;
	case '\r':
	    q++;
	    break;
	case '\b':
	    if (d) {
		--p;
		if (p < bol)
		    goto end_loop;
		x -= FONT_PER_CHAR (d);
		if (d == '_')
		    p->c.style |= MOD_ITALIC;
		else
		    p->c.style |= MOD_BOLD;
	    }
	    q++;
	    break;
	default:
            consumed = utf8_to_wchar_t_one_char_safe (&c, (const char *) str.data + q, str.len - q);
	    if (!FONT_PER_CHAR (c)) {
		c = ' ';
		p->c.style |= MOD_ABNORMAL;
	    }
	    x += FONT_PER_CHAR (c);
	    (p++)->c.ch = c;
	    if (p >= eol)
		goto end_loop;
	    q += consumed;
	    break;
	}
	if (x > x_max)
	    break;
    }
  end_loop:
    p->c.ch = p->_style = 0;
}




