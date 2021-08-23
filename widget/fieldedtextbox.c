/* fieldedtextbox.c - for drawing a scrollable text window widget
   Copyright (C) 1996-2018 Paul Sheer
 */



/*
 *    format:
 *
 *    the printed fields can have the following:
 *
 *    '\t': can only occur at the begining and end, or at the beginning.
 *              At the beginning signals right justification. At the beginning
 *              and the end signals centering. eg "\tcentred\t" or "\tleft"
 *    "\v%d": insert pixmap %d at the point. Pixmaps are a special variety
 *              defined by the create_text_pixmap() function. The value passed
 *              to this function must appear after the \v and must be less
 *              than 128.
 *    "\f%d": advance forward %d pixels. %d must be less than 128.
 *    "\r%c": puts %c in 'italic' color, default: green.
 *    "\b%c": puts %c in 'bold' color, default: blue.
 */


#define BDR 8

#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <X11/Xatom.h>
#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"
#include "edit.h"
#include "editcmddef.h"

#include "coollocal.h"
#include "mousemark.h"
#include "pool.h"


extern struct look *look;

extern int option_text_fg_normal;
extern int option_text_fg_bold;
extern int option_text_fg_italic;

extern int option_text_bg_normal;
extern int option_text_bg_marked;
extern int option_text_bg_highlighted;

/* for this widget
   ->column holds the pixel width of the longest line
 */

/* char ** (*get_line) (void *data, int line_number, int *num_fields, int *tagged); */

#define pixmap_width(x) 0

int eh_fielded_textbox (CWidget * w, XEvent * xevent, CEvent * cwevent);
static int calc_text_pos_fielded_textbox (CWidget * w, long b, long *q, int l);
int mbrtowc_utf8_to_wchar (C_wchar_t * c, const char *t, int n, void *x /* no shifting with utf8 */ );

/* these are not printable in this module */
#define this_is_printable(c) (!strchr ("\r\b\t", c))

static int this_text_width (char *s)
{
    int l = 0;
    char *p;
    for (p = s; *p;) {
	if (*p == '\v') {
	    l += pixmap_width (++p);
	    p++;
	} else if (*p == '\f') {
	    l += *(++p);
	    p++;
	} else if (this_is_printable (*p)) {
	    C_wchar_t c = 0;
	    int r;
	    r = mbrtowc_utf8_to_wchar (&c, p, strnlen (p, 6), 0);
	    if (!r)
		break;
	    if (r == -1) {
		l += FONT_PER_CHAR ('?');
		p++;
	    } else {
		l += FONT_PER_CHAR (c);
		p += r;
	    }
	} else {
            p++;
        }
    }
    return l;
}

#define INTER_FIELD_SPACING	6
#define LINE_OFFSET		0

/* result must be free'd */
static int *get_field_sizes (void *data, int *num_lines, int *max_width,
			     char **(*get_line) (void *, int, int *, int *))
{
    char **fields;
    int tagged, i, tab[256], *result, num_fields, max_num_fields = 0, line_number;

    memset (tab, 0, sizeof (tab));

    *num_lines = 0;

    for (line_number = 0;; line_number++) {
	fields = (*get_line) (data, line_number, &num_fields, &tagged);
	if (!fields)
	    break;
	(*num_lines)++;
	if (max_num_fields < num_fields)
	    max_num_fields = num_fields;
	for (i = 0; i < num_fields; i++) {
	    int l;
	    if (!fields[i])
		break;
	    l = this_text_width (fields[i]);
	    if (tab[i] < l)
		tab[i] = l;
	}
    }
    *max_width = 0;
    for (i = 0; i < max_num_fields; i++)
	tab[i] += INTER_FIELD_SPACING;
    for (i = 0; i < max_num_fields; i++)
	*max_width += tab[i];

    result = CMalloc ((max_num_fields + 1) * sizeof (int));
    memcpy (result, tab, max_num_fields * sizeof (int));
    result[max_num_fields] = 0;
    return result;
}

static void compose_line (void *data, int line_number, unsigned char *line, int *tab,
		     char **(*get_line) (void *, int, int *, int *), int *tagged)
{
    char **fields;
    int i, num_fields;

    *line = 0;
    *tagged = 0;

    if (!data)
	return;

    fields = (*get_line) (data, line_number, &num_fields, tagged);

    if (!fields)
	return;

    for (i = 0; i < num_fields; i++) {
	int l = 0, t, centred = 0;
	char *p;
	p = fields[i];
	t = tab[i] - this_text_width (p) - INTER_FIELD_SPACING;
	if (t < 0)
	    t = 0;
	if (p[0] == '\t') {
	    p++;
	    if (p[strlen (p) - 1] == '\t') {
		l = t - (t / 2);
		t /= 2;
		centred = 1;
	    } else {
		l = t;
		t = 0;
	    }
	}
	for (;;) {
	    l -= 127;
	    if (l >= 0) {
		*line++ = '\f';
		*line++ = (unsigned char) 127;
	    } else {
		l += 127;
		if (l) {
		    *line++ = '\f';
		    *line++ = (unsigned char) l;
		}
		break;
	    }
	}
	strcpy ((char *) line, p);
	line += strlen (p) - centred;
	if (!fields[i + 1])
	    break;
	t += INTER_FIELD_SPACING;
	for (;;) {
	    t -= 127;
	    if (t >= 0) {
		*line++ = '\f';
		*line++ = (unsigned char) 127;
	    } else {
		t += 127;
		if (t) {
		    *line++ = '\f';
		    *line++ = (unsigned char) t;
		}
		break;
	    }
	}
    }
    *line = 0;
}

static unsigned char *compose_line_cached (void *data, int l, int *tab, char **(*get_line) (void *, int, int *, int *), int *tagged)
{
    static unsigned char line[4096];
    static int c_tagged, c_l = -1;
    if (c_l == l) {
	*tagged = c_tagged;
	return line;
    }
    compose_line (data, l, line, tab, get_line, tagged);
    c_l = l;
    c_tagged = *tagged;
    return line;
}

static long count_fielded_textbox_lines (CWidget * wdt);
void render_fielded_textbox (CWidget * w, int redrawall);

void link_scrollbar_to_fielded_textbox (CWidget * scrollbar, CWidget * textbox, XEvent * xevent, CEvent * cwevent, int whichscrbutton)
{
    int redrawtext = 0, count, c;
    static int r = 0;
    if ((xevent->type == ButtonRelease || xevent->type == MotionNotify) && whichscrbutton == 3) {
	redrawtext = CSetTextboxPos (textbox, TEXT_SET_LINE, (double) scrollbar->firstline * textbox->numlines / 65535.0);
    } else if (xevent->type == ButtonPress && (cwevent->button == Button1 || cwevent->button == Button2)) {
	switch (whichscrbutton) {
	case 1:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_LINE, textbox->firstline - (textbox->height / FONT_PIX_PER_LINE - 2));
	    break;
	case 2:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_LINE, textbox->firstline - 1);
	    break;
	case 5:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_LINE, textbox->firstline + 1);
	    break;
	case 4:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_LINE, textbox->firstline + (textbox->height / FONT_PIX_PER_LINE - 2));
	    break;
	}
    }
    if (xevent->type == ButtonRelease)
	render_fielded_textbox (textbox, 0);
    else {
	c = CCheckWindowEvent (0, ButtonReleaseMask | ButtonMotionMask, 1);
	if (redrawtext) {
	    if (!c) {
		render_fielded_textbox (textbox, 0);
		r = 0;
	    } else {
		r = 1;
	    }
	} else if (c && r) {
	    render_fielded_textbox (textbox, 0);
	    r = 0;
	}
    }
    count = count_fielded_textbox_lines (textbox);
    if (!count)
	count = 1;
    scrollbar->firstline = (double) 65535.0 *textbox->firstline / (textbox->numlines ? textbox->numlines : 1);
    scrollbar->numlines = (double) 65535.0 *count / (textbox->numlines ? textbox->numlines : 1);
}

void link_h_scrollbar_to_fielded_textbox (CWidget * scrollbar, CWidget * textbox, XEvent * xevent, CEvent * cwevent, int whichscrbutton)
{
    int redrawtext = 0, c;
    static int r = 0;
    if ((xevent->type == ButtonRelease || xevent->type == MotionNotify) && whichscrbutton == 3) {
	redrawtext = CSetTextboxPos (textbox, TEXT_SET_COLUMN, (double) scrollbar->firstline * (textbox->column / FONT_MEAN_WIDTH) / 65535.0);
    } else if (xevent->type == ButtonPress && (cwevent->button == Button1 || cwevent->button == Button2)) {
	switch (whichscrbutton) {
	case 1:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_COLUMN, textbox->firstcolumn - (textbox->width / FONT_MEAN_WIDTH - 2));
	    break;
	case 2:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_COLUMN, textbox->firstcolumn - 1);
	    break;
	case 5:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_COLUMN, textbox->firstcolumn + 1);
	    break;
	case 4:
	    redrawtext = CSetTextboxPos (textbox, TEXT_SET_COLUMN, textbox->firstcolumn + (textbox->width / FONT_MEAN_WIDTH - 2));
	    break;
	}
    }
    if (xevent->type == ButtonRelease)
	render_fielded_textbox (textbox, 0);
    else {
	c = CCheckWindowEvent (0, ButtonReleaseMask | ButtonMotionMask, 1);
	if (redrawtext) {
	    if (!c) {
		render_fielded_textbox (textbox, 0);
		r = 0;
	    } else {
		r = 1;
	    }
	} else if (c && r) {
	    render_fielded_textbox (textbox, 0);
	    r = 0;
	}
    }
    scrollbar->firstline = (double) 65535.0 *(textbox->firstcolumn * FONT_MEAN_WIDTH) / textbox->column;
    scrollbar->numlines = (double) 65535.0 *(textbox->width - 6) / textbox->column;
}

void edit_translate_xy (int xs, int ys, int *x, int *y);
void selection_clear (void);
static long current;

static int text_ypixel_to_row (int y)
{
    int row;
    row = y / FONT_PIX_PER_LINE + 1;
    return row;
}

static void xy (int x, int y, int *x_return, int *y_return)
{
    edit_translate_xy (x, y, x_return, y_return);
}

static long cp (CWidget * w, int x, int y)
{
    long q, row;
    row = text_ypixel_to_row (y);
    row = (row + w->firstline - 1) << 16;
    if (row < 0)
	x = row = 0;
    if (w->options & TEXTBOX_MARK_WHOLE_LINES)
	x = 0;
    calc_text_pos_fielded_textbox (w, row, &q, --x);
    return q;
}

/* return 1 if not marked */
static int marks (CWidget * w, long *start, long *end)
{
    if (w->mark1 == w->mark2)
	return 1;
    *start = min (w->mark1, w->mark2);
    *end = max (w->mark1, w->mark2);
    return 0;
}

extern int range (CWidget * w, long start, long end, int click);

static void move_mark (CWidget * w)
{
    w->mark2 = w->mark1 = current;
}

static void fin_mark (CWidget * w)
{
    w->mark2 = w->mark1 = -1;
}

static void release_mark (CWidget * w, XEvent * event)
{
    w->mark2 = current;
    if (w->mark2 != w->mark1 && event) {
	selection_clear ();
	XSetSelectionOwner (CDisplay, XA_PRIMARY, w->winid, event->xbutton.time);
	XSetSelectionOwner (CDisplay, ATOM_ICCCM_P2P_CLIPBOARD, w->winid, event->xbutton.time);
    }
}

/* result must be free'd */
static char *get_block (CWidget * w, long start_mark, long end_mark, int *type, int *l)
{
    POOL *p;
    int tagged, i;
    unsigned char c, *t;
    long x, y, a, b;
    void *data;
    CPushFont ("editor", 0);

    a = min (w->mark2, w->mark1);
    b = max (w->mark2, w->mark1);
    x = a & 0xFFFFL;
    y = a >> 16;

    p = pool_init ();

    for (;; y++) {
	unsigned char *text;
	if (y < w->numlines)
	    data = w->hook;
	else
	    data = 0;
	text = compose_line_cached (data, y, w->tab, w->get_line, &tagged);
	for (;; x++) {
	    if (y == (b >> 16))
		if (x >= (b & 0xFFFFL))
		    goto done;
	    if (y > (b >> 16))
	        goto done;
	    c = text[x];
	    if (!c) {
		c = '\n';
		pool_write (p, (unsigned char *) &c, 1);
		break;
	    }
	    if (c == '\f') {
		int j;
#ifdef HAVE_DND
		if (w->options & TEXTBOX_FILE_LIST) {	/* this is a filelist, so only get the first field: i.e. the file name */
#else
		if (*type == DndFiles || *type == DndFile) {	/* this is a filelist, so only get the first field: i.e. the file name */
#endif
		    c = '\n';
		    pool_write (p, (unsigned char *) "\n", 1);
		    break;
		}
    		j = text[++x];
		while ((j -= FONT_PER_CHAR(' ')) > 0)
		    pool_write (p, (unsigned char *) " ", 1);
		pool_write (p, (unsigned char *) " ", 1);
		continue;
	    }
	    if (c == '\v') {
		int j;
    		j = pixmap_width (text[++x]);
		while ((j -= FONT_PER_CHAR(' ')) > 0)
		    pool_write (p, (unsigned char *) " ", 1);
		continue;
	    }
	    if (this_is_printable (c))
		pool_write (p, (unsigned char *) &c, 1);
	}
	x = 0;
    }
  done:
    CPopFont ();
#ifdef HAVE_DND
    *type = DndText;
#endif
    *l = pool_length (p);
    pool_null (p);
#ifdef HAVE_DND
    if (!(w->options & TEXTBOX_FILE_LIST))
#else
    if (!(*type == DndFiles || *type == DndFile))
#endif
	return (char *) pool_break (p);
    t = (unsigned char *) CDndFileList ((char *) pool_start (p), l, &i);
    pool_free (p);
    if (i == 1)
	*type = DndFile;
    else
	*type = DndFiles;
    return (char *) t;
}

static void move (CWidget * w, long click, int y)
{
    int h, row;
    row = text_ypixel_to_row (y);
    current = click;
    if (w->mark2 == -1)
	w->mark1 = current;
    h = (w->height - BDR) / FONT_PIX_PER_LINE;
    if (row > h && w->firstline < w->numlines - h - 2)
	CSetTextboxPos (w, TEXT_SET_LINE, w->firstline + row - h);
    if (row < 1)
	CSetTextboxPos (w, TEXT_SET_LINE, w->firstline + row - 1);
    w->mark2 = click;
}

static void motion (CWidget * w, long click)
{
    w->mark2 = click;
}

struct mouse_funcs fielded_mouse_funcs = {
    0,
    (void (*)(int, int, int *, int *)) xy,
    (long (*)(void *, int, int)) cp,
    (int (*)(void *, long *, long *)) marks,
    (int (*)(void *, long, long, long)) range,
    (void (*)(void *)) fin_mark,
    (void (*)(void *)) move_mark,
    (void (*)(void *, XEvent *)) release_mark,
    (char *(*)(void *, long, long, int *, int *)) get_block,
    (void (*)(void *, long, int)) move,
    (void (*)(void *, long)) motion,
    0,
    0,
    0,
    0,
    DndFile
};

CWidget *CDrawFieldedTextbox (const char *identifier, Window parent, int x, int y,
			      int width, int height, int line, int column,
			      char **(*get_line) (void *, int, int *, int *),
			      long options, void *data)
{
    char *scroll;
    int numlines;
    CWidget *wdt;
    int *tab, wf;

    int w, h, x_hint, y_hint;
    CPushFont ("editor", 0);

    tab = get_field_sizes (data, &numlines, &wf, get_line);

    if (width == AUTO_WIDTH)
	w = wf + 6;
    else
	w = width;
    if (height == AUTO_HEIGHT)
	h = (max (1, numlines)) * FONT_PIX_PER_LINE + 6;
    else
	h = height;

    wdt = CSetupWidget (identifier, parent, x, y,
			w, h, C_FIELDED_TEXTBOX_WIDGET, INPUT_KEY,
			color_palette (option_text_bg_normal), 1);

    xdnd_set_type_list (CDndClass, wdt->winid, xdnd_typelist_send[DndText]);

    wdt->eh = eh_fielded_textbox;
    wdt->options = options | WIDGET_TAKES_SELECTION;

    wdt->firstline = line;
    wdt->firstcolumn = column;
    wdt->column = wf;
    wdt->cursor = 0;
    wdt->numlines = numlines;
    wdt->tab = tab;
    wdt->get_line = get_line;
    wdt->hook = data;
    wdt->funcs = mouse_funcs_new (wdt, &fielded_mouse_funcs);

    if (h > 80 && height != AUTO_HEIGHT) {
	wdt->vert_scrollbar = CDrawVerticalScrollbar (scroll = catstrs (identifier, ".vsc", NULL), parent,
			 x + w + WIDGET_SPACING, y, h, AUTO_WIDTH, 0, 0);
	CSetScrollbarCallback (wdt->vert_scrollbar->ident, wdt->ident, link_scrollbar_to_fielded_textbox);
	CGetHintPos (&x_hint, 0);
    } else {
	x_hint = x + w + WIDGET_SPACING;
    }

    if (w > 80 && width != AUTO_WIDTH) {
	wdt->hori_scrollbar = CDrawHorizontalScrollbar (scroll = catstrs (identifier, ".hsc", NULL), parent,
			 x, y + h + WIDGET_SPACING, w, (*look->get_fielded_textbox_hscrollbar_width) (), 0, 65535);
	CSetScrollbarCallback (wdt->hori_scrollbar->ident, wdt->ident, link_h_scrollbar_to_fielded_textbox);
	CGetHintPos (0, &y_hint);
    } else {
	y_hint = y + h + WIDGET_SPACING;
    }

    set_hint_pos (x_hint, y_hint);
    CPopFont ();
    return wdt;
}


CWidget *CRedrawFieldedTextbox (const char *identifier, int preserve)
{
    int numlines;
    CWidget *wdt;
    int *tab, w;

    CPushFont ("editor", 0);
    wdt = CIdent (identifier);
    tab = get_field_sizes (wdt->hook, &numlines, &w, wdt->get_line);

    if (!preserve) {
	wdt->firstline = 0;
	wdt->firstcolumn = 0;
	wdt->cursor = 0;
    }
    wdt->column = w;
    wdt->numlines = numlines;
    if (wdt->tab)
	free (wdt->tab);
    wdt->tab = tab;
    wdt->mark1 = wdt->mark2 = -1;

    CSetColor (color_palette (option_text_bg_normal));
    CRectangle (wdt->winid, 3, 3, wdt->width - 3 - 1, wdt->height - 3 - 1);
    CExpose (identifier);

    CPopFont ();
    return wdt;
}


int utf8_to_wchar_t_one_char_safe (C_wchar_t * c, const char *t, int n);


static int calc_text_pos_fielded_textbox (CWidget *w, long b, long *q, int l)
{
    int x = 0, xn = 0, tagged;
    C_wchar_t c;
    unsigned char *text;
    long k;
    void *data = 0;

    if ((b >> 16) < w->numlines)
	data = w->hook;
    text = compose_line_cached (data, b >> 16, w->tab, w->get_line, &tagged);

    k = b & 0xFFFFL;
    if (k == 0xFFFFL)
	k = 0;
    for (;;) {
        int consumed = 1;
	c = text[k];
	switch (c) {
	case '\0':
	case '\n':
	    *q = b;
	    return x;
	case '\f':
	    xn = x + text[k + 1];
	    if (xn > l) {
		*q = b;
		return x;
	    }
	    k += 2;
	    b += 2;
	    break;
	case '\b':
	case '\r':
            consumed = utf8_to_wchar_t_one_char_safe (&c, (const char *) text + k + 1, strnlen((const char *) text + k + 1, 6));
	    xn = x + FONT_PER_CHAR(c);
	    if (xn > l) {
		*q = b;
		return x;
	    }
	    k += 1 + consumed;
	    b += 1 + consumed;
	    break;
	case '\v':
	    xn = x + pixmap_width (text[k + 1]);
	    if (xn > l) {
		*q = b;
		return x;
	    }
	    k += 2;
	    b += 2;
	    break;
	default:
            consumed = utf8_to_wchar_t_one_char_safe (&c, (const char *) text + k, strnlen((const char *) text + k, 6));
	    xn = x + FONT_PER_CHAR(c);
	    if (xn > l) {
		*q = b;
		return x;
	    }
	    b += consumed;
	    k += consumed;
	    break;
	}
	x = xn;
    }
    return 0;
}

extern int highlight_this_line;

/* here upper 16 bits of q are the line, lowe 16, the column */
static void convert_text_fielded_textbox (CWidget * w, long bol, long q, cache_type *line, cache_type *eol, int x, int x_max, int row, struct _book_mark **bookmarks_not_used, int n_bookmarks_not_used)
{
    unsigned char *text;
    int tagged, bold = 0, italic = 0;
    C_wchar_t c;
    cache_type *p;
    long m1, m2, k;
    void *data = 0;

    m1 = min (w->mark1, w->mark2);
    m2 = max (w->mark1, w->mark2);

    if ((q >> 16) < w->numlines)
	data = w->hook;
    text = compose_line_cached (data, q >> 16, w->tab, w->get_line, &tagged);

    k = q & 0xFFFFL;
    if (k == 0xFFFFL)
	k = 0;

    p = line;
    p->c.ch = p->_style = 0;
    for (;;) {
        int consumed;
	c = text[k];
	p[1]._style = p[1].c.ch = 0;
	p->c.fg = p->c.bg = 0xFF;	/* default background colors */
	if (highlight_this_line)
	    p->c.style |= MOD_HIGHLIGHTED;
	if (tagged)
	    p->c.style |= MOD_INVERSE;
	if (q >= m1 && q < m2)
	    p->c.style |= MOD_MARKED;
	switch (c) {
	case '\0':
	case '\n':
	    (p++)->c.ch = ' ';
	    if (p >= eol)
		goto end_loop;
	    if (highlight_this_line || tagged) {
		x += FONT_PER_CHAR(' ');
	    } else
		return;
	    break;
	case '\f':
	    p->c.style |= MOD_TAB;
	    (p++)->c.ch = text[k + 1];
	    if (p >= eol)
		goto end_loop;
	    x += text[k + 1];
	    k += 2;
	    q += 2;
	    break;
	case '\b':
	    bold = 2;
            k++;
            q++;
	    break;
	case '\r':
	    italic = 2;
            k++;
            q++;
	    break;
	case '\v':
	    p->c.style |= MOD_TAB;
	    (p++)->c.ch = text[k + 1];
	    if (p >= eol)
		goto end_loop;
	    x += pixmap_width (text[k + 1]);
	    k += 2;
	    q += 2;
	    break;
	default:
            consumed = utf8_to_wchar_t_one_char_safe (&c, (const char *) text + k, strnlen((const char *) text + k, 6));
            k += consumed;
            q += consumed;
	    x += FONT_PER_CHAR(c);
	    p->c.ch = c;
	    if (italic > 0)
		p->c.style |= MOD_ITALIC;
	    if (bold > 0)
		p->c.style |= MOD_BOLD;
	    p++;
	    if (p >= eol)
		goto end_loop;
	    break;
	}
	italic--;
	bold--;
	if (x > x_max)
	    break;
    }
  end_loop:
    p->c.ch = p->_style = 0;
}


static void fielded_text_print_line (CWidget * w, long b, int row)
{
    edit_draw_proportional (w,
			    (converttext_cb_t) convert_text_fielded_textbox,
                            (calctextpos_cb_t) calc_text_pos_fielded_textbox,
			    -w->firstcolumn * FONT_MEAN_WIDTH, w->winid,
			    w->width, b, row, row * FONT_PIX_PER_LINE + EDIT_TEXT_VERTICAL_OFFSET, 0,
			    1, 0, 0);
}



/*
   ->firstline   is line number of the top line in the window.
   ->firstcolumn is column shift (positive).
   ->current     is actual char position of first line in display.
   ->numlines    is the total number of lines.
   ->cursor      is the number of the highlighted line.
   ->textlength  is the length of text excluding trailing NULL.
   First three must be initialised to proper values (e.g. 0, 0 and 0).
 */

extern int EditExposeRedraw;
extern int EditClear;
extern unsigned long edit_normal_background_color;

void render_fielded_textbox (CWidget * w, int redrawall)
{
    int row, height, isfocussed, curs, i, x;
    static Window last_win = 0;
    static int last_firstcolumn = 0;
    CPushFont ("editor", 0);
    if (redrawall) {
	EditExposeRedraw = 1;
	EditClear = 1;
    }
    if (last_win == w->winid && last_firstcolumn != w->firstcolumn) {
	x = 0;
        CSetColor (color_palette (option_text_bg_normal));
	for (i = 0;; i++) {
	    x += w->tab[i];
	    if (x >= w->column)
		break;
	    CLine (w->winid, x + LINE_OFFSET - (last_firstcolumn * FONT_MEAN_WIDTH), 3, x + LINE_OFFSET - (last_firstcolumn * FONT_MEAN_WIDTH), (w->numlines - w->firstline) * FONT_PIX_PER_LINE);
	}
    }
    last_firstcolumn = w->firstcolumn;
    last_win = w->winid;

    height = w->height / FONT_PIX_PER_LINE;

    isfocussed = (w->winid == CGetFocus ());
    curs = !(w->options & TEXTBOX_NO_CURSOR || w->mark1 != w->mark2);	/* don't draw the cursor line */

    edit_set_foreground_colors (color_palette (option_text_fg_normal), color_palette (option_text_fg_bold), color_palette (option_text_fg_italic));
    edit_set_background_colors (color_palette (option_text_bg_normal), color_palette (0), color_palette (option_text_bg_marked), color_palette (9), color_palette (option_text_bg_highlighted));

    for (row = 0; row < height; row++) {
	if (row + w->firstline == w->cursor && isfocussed && curs)
	    highlight_this_line = 1;
	else
	    highlight_this_line = 0;
	fielded_text_print_line (w, (row + w->firstline) << 16, row);
    }

    x = 0;
    CSetColor (COLOR_FLAT);
    for (i = 0;; i++) {
	if (!w->tab[i])
	    break;
	x += w->tab[i];
	if (x >= w->column)
	    break;
	CLine (w->winid, x + LINE_OFFSET - (w->firstcolumn * FONT_MEAN_WIDTH), 3, x + LINE_OFFSET - (w->firstcolumn * FONT_MEAN_WIDTH), (w->numlines - w->firstline) * FONT_PIX_PER_LINE + 3);
    }
    if ((w->numlines - w->firstline) * FONT_PIX_PER_LINE < w->height) {
	x = 0;
        CSetColor (color_palette (option_text_bg_normal));
	for (i = 0;; i++) {
	    if (!w->tab[i])
		break;
	    x += w->tab[i];
	    if (x >= w->column)
		break;
	    CLine (w->winid, x + LINE_OFFSET - (w->firstcolumn * FONT_MEAN_WIDTH), (w->numlines - w->firstline) * FONT_PIX_PER_LINE + 3, x + LINE_OFFSET - (w->firstcolumn * FONT_MEAN_WIDTH), w->height - 3);
	}
    }
    EditExposeRedraw = 0;
    EditClear = 0;

    (*look->render_fielded_textbox_tidbits) (w, isfocussed);

    CPopFont ();
    return;
}

/*
   Count the number of lines that would be printed
   by the above routine, but don't print anything.
   If all is non-zero then count all the lines.
 */
static long count_fielded_textbox_lines (CWidget * wdt)
{
    long height;
    height = wdt->height / FONT_PIX_PER_LINE;
    if (wdt->numlines - wdt->firstline < height)
	return wdt->numlines - wdt->firstline;
    return height;
}

static void fielded_text_mouse_mark (CWidget * w, XEvent * event, CEvent * ce)
{
    CPushFont ("editor", 0);
    mouse_mark (event, ce->double_click, w->funcs);
    CPopFont ();
}

/* gets selected text into selection structure, stripping nroff */
void fielded_text_get_selection (CWidget * w)
{
/* backport this fix: */
    int type = DndUnknown;
    CStr s;
    s.data = get_block (w, 0, 0, &type, &s.len);
    s.len = strlen(s.data);
    selection_replace (s);
    CStr_free(&s);
}


void selection_send (XSelectionRequestEvent * rq);

int eh_fielded_textbox (CWidget * w, XEvent * xevent, CEvent * cwevent)
{
    int handled = 0, redrawall = 0, count;

    switch (xevent->type) {
    case SelectionRequest:
	fielded_text_get_selection (w);
	selection_send (&(xevent->xselectionrequest));
	return 1;
    case Expose:
	if (!xevent->xexpose.count)
	    redrawall = 1;
	break;
    case ClientMessage:
	w->mark1 = w->mark2 = 0;
	break;
    case ButtonPress:
	CPushFont ("editor", 0);
	CFocus (w);
	if (xevent->xbutton.button == Button1)
	    w->cursor = (xevent->xbutton.y - BDR) / FONT_PIX_PER_LINE + w->firstline;
	if (w->cursor > w->numlines - 1)
	    w->cursor = w->numlines - 1;
	if (w->cursor < 0)
	    w->cursor = 0;
	cwevent->ident = w->ident;
	cwevent->xt = (xevent->xbutton.x - 7) / FONT_MEAN_WIDTH + w->firstcolumn;
	cwevent->yt = w->cursor;
	CPopFont ();
    case ButtonRelease:
    case MotionNotify:
	if (!xevent->xmotion.state && xevent->type == MotionNotify)
	    return 0;
	resolve_button (xevent, cwevent);
	fielded_text_mouse_mark (w, xevent, cwevent);
	break;
    case FocusIn:
    case FocusOut:
	break;
    case KeyPress:
	cwevent->ident = w->ident;
	if (!(TEXTBOX_NO_KEYS & w->options)) {
	    if (w->options & TEXTBOX_FILE_LIST && w->hook) {
		if (cwevent->key == XK_Insert || cwevent->key == XK_KP_Insert) {
		    if (w->mark1 == w->mark2) {
			struct file_entry *f;
			f = (struct file_entry *) w->hook;
			if (f[w->cursor].options & FILELIST_TAGGED_ENTRY)
			    f[w->cursor].options &= (0xFFFFFFFFUL - FILELIST_TAGGED_ENTRY);
			else
			    f[w->cursor].options |= FILELIST_TAGGED_ENTRY;
			CTextboxCursorMove (w, CK_Down);
			handled = 1;
			break;
		    }
		}
	    }
	    handled = CTextboxCursorMove (w, cwevent->command);
	}
	break;
    default:
	return 0;
    }

/* Now draw the changed text box, count will contain
   the number of textlines displayed */
    render_fielded_textbox (w, redrawall);
    count = count_fielded_textbox_lines (w);

/* now update the scrollbar position */
    if (w->vert_scrollbar && w->numlines) {
	w->vert_scrollbar->firstline = (double) 65535.0 *w->firstline / (w->numlines ? w->numlines : 1);
	w->vert_scrollbar->numlines = (double) 65535.0 *count / (w->numlines ? w->numlines : 1);
	w->vert_scrollbar->options = 0;
	render_scrollbar (w->vert_scrollbar);
    }
    if (w->hori_scrollbar && w->column) {
	w->hori_scrollbar->firstline = (double) 65535.0 *(w->firstcolumn * FONT_MEAN_WIDTH) / w->column;
	w->hori_scrollbar->numlines = (double) 65535.0 *(w->width - 6) / w->column;
	w->hori_scrollbar->options = 0;
	render_scrollbar (w->hori_scrollbar);
    }
    return handled;
}

