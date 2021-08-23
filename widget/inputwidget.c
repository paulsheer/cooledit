/* inputwidget.c
   Copyright (C) 1996-2018 Paul Sheer
 */



#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <X11/Xatom.h>
#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"

#include "coollocal.h"

#include "edit.h"
#include "editcmddef.h"
#include "mousemark.h"
#include "pool.h"



int eh_textinput (CWidget * w, XEvent * xevent, CEvent * cwevent);
void input_mouse_mark (CWidget * w, XEvent * event, CEvent * ce);
int count_one_utf8_char (const char *s);
int count_one_utf8_char_sloppy (const char *s);

#define INPUT_INSERT_FLUSH              (-1)
static void input_insert (CWidget * w, int c);

extern struct look *look;


/* {{{  history stuff: draws a history of inputs a widget of the same ident */


#define MAX_HIST_WIDGETS 128

struct textinput_history {
    char ident[32];
    int last;
    CStr input[NUM_SELECTION_HISTORY];
};

static struct textinput_history *history_widgets[MAX_HIST_WIDGETS] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int n_history_widgets = 0;

static void add_to_history (struct textinput_history *h, CStr s, int allow_blank_lines)
{
    int i, j;
    if (!s.len)
	return;
    if (!s.len && !allow_blank_lines)
	return;
    for (i = h->last - 1; i >= 0; i--) {
        if (s.len == h->input[i].len && !memcmp (h->input[i].data, s.data, s.len)) {	/* avoid adding duplicates */
            CStr c;
            c = h->input[i];
            if (i < h->last - 1)
                for (j = i; j < h->last - 1; j++)	/* shift all entries up one place */
                    h->input[j] = h->input[j + 1];
            h->input[h->last - 1] = c;	/* move new entry to top */
            return;
        }
    }
    if (h->last == NUM_SELECTION_HISTORY) {
	CStr_free (&h->input[0]);
        for (j = 0; j < h->last - 1; j++)	/* shift all entries up one place */
	    h->input[j] = h->input[j + 1];
	h->last--;
    }
    h->input[h->last] = CStr_dupstr(s);
    h->last++;
}

static void add_to_widget_history (const char *ident, CStr s)
{
    int i;
    int allow_blank_lines = 0;
    allow_blank_lines = (strchr (ident, '+') != 0);
    for (i = 0; i < MAX_HIST_WIDGETS; i++) {
	if (!history_widgets[i])
	    break;
	if (!strcmp (history_widgets[i]->ident, ident)) {
	    add_to_history (history_widgets[i], s, allow_blank_lines);
	    return;
	}
    }

    if (n_history_widgets == MAX_HIST_WIDGETS) {	/* shift up one */
	for (i=0;i<history_widgets[0]->last;i++)
	    CStr_free (&history_widgets[0]->input[i]);
	free (history_widgets[0]);
	Cmemmove (history_widgets, history_widgets + 1, (MAX_HIST_WIDGETS - 1) * sizeof (struct textinput_history *));
	n_history_widgets--;
    }

/* create a new history */
    history_widgets[n_history_widgets] = CMalloc (sizeof (struct textinput_history));
    memset (history_widgets[n_history_widgets], 0, sizeof (struct textinput_history));
    strcpy (history_widgets[n_history_widgets]->ident, ident);
    add_to_history (history_widgets[n_history_widgets], s, allow_blank_lines);
    n_history_widgets++;

}

void CAddToTextInputHistory (const char *ident, CStr s)
{
    add_to_widget_history (ident, s);
}

/*
   Returns a newline separate list of all 'input' in the history.
 */
static void get_history_list (const char *ident, int reverse, struct textinput_history *h)
{
    int i, j;
    for (i = 0; i < n_history_widgets; i++) {
	if (!strcmp (history_widgets[i]->ident, ident)) {
            struct textinput_history *c;
            c = history_widgets[i];
            h->last = c->last;
	    if (reverse) {
		for (j = 0; j < c->last; j++)
                    h->input[j] = c->input[j];
	    } else {
		for (j = 0; j < c->last; j++)
                    h->input[j] = c->input[c->last - 1 - j];
	    }
	    return;
	}
    }
    h->last = 0;
}

/* result must not be free'd */
/* returns the last text inputted in the input widget named ident */
CStr CLastInput (const char *ident)
{
    int i;
    CStr empty;
    empty.data = "";
    empty.len = 0;
    for (i = 0; i < n_history_widgets; i++) {
	if (!strcmp (history_widgets[i]->ident, ident)) {
	    if (!history_widgets[i]->last)
		return empty;
	    return history_widgets[i]->input[history_widgets[i]->last - 1];
	}
    }
    return empty;
}

#define HISTORY_LINES 10

static int clip_lines (int lines, int num_lines)
{
    if (lines > num_lines)
	lines = num_lines;
    if (lines > HISTORY_LINES)
	lines = HISTORY_LINES;
    if (lines < 1)
	lines = 1;
    return lines;
}

static const char *escape_header = "nulls and newlines are escaped with backslash\n";

/* gets a list of all input histories and all widget for use in saving
   to an options file (if you want to save the state of an application, 
   for example. result must be free'd */
char *get_all_lists (void)
{
    int i, j;
    POOL *p;

    p = pool_init();

    pool_write (p, (unsigned char *) escape_header, strlen(escape_header));
    for (i = 0; i < n_history_widgets; i++) {
        pool_write (p, (unsigned char *) history_widgets[i]->ident, strlen(history_widgets[i]->ident));
        pool_write (p, (unsigned char *) "\n", 1);
	for (j = 0; j < history_widgets[i]->last; j++) {
            unsigned char c;
            CStr *s;
            int k;
            pool_write (p, (unsigned char *) "\t", 1);
            s = &history_widgets[i]->input[j];
            for (k = 0; k < s->len; k++) {
                c = s->data[k];
                if (c == '\\') {
                    pool_write (p, (unsigned char *) "\\\\", 2);
                } else if (c == '\0') {
                    pool_write (p, (unsigned char *) "\\0", 2);
                } else if (c == '\n') {
                    pool_write (p, (unsigned char *) "\\n", 2);
                } else {
                    pool_write (p, &c, 1);
                }
            }
            pool_write (p, (unsigned char *) "\n", 1);
	}
    }
    pool_write (p, (unsigned char *) "", 1);
    return (char *) pool_break (p);
}

void free_all_lists (void)
{
    int i, j;
    for (i = 0; i < n_history_widgets; i++) {
	for (j = 0; j < history_widgets[i]->last; j++)
	    CStr_free (&history_widgets[i]->input[j]);
	free (history_widgets[i]);
    }
}

void put_all_lists (const char *s)
{
    char ident[33];
    CStr str;
    int escape = 0;
    if (!s)
	return;
    if (!strncmp(s, escape_header, strlen(escape_header))) {
        escape = 1;
        s += strlen(escape_header);
    }
    while (*s) {
        char *t;
	t = strchr (s, '\n');
	if (!t)
	    return;
	if (t - s > 32)
	    return;
	memset (ident, '\0', sizeof (ident));
	memcpy (ident, s, t - s);
/* printf("ident = %s\n", ident); */
	s = t + 1;
	while (*s == '\t') {
	    POOL *p;
            s++;
	    t = strchr (s, '\n');
	    if (!t)
		return;
/* printf("       [%.*s]\n", (int) (t - s), s); */
	    p = pool_init ();
	    while (*s != '\n') {
		unsigned char c;
		if (escape) {
		    if (*s == '\\') {
                        if (s[1] == '\\') {
                            c = '\\';
                            s += 2;
                        } else if (s[1] == '0') {
                            c = '\0';
                            s += 2;
                        } else if (s[1] == 'n') {
                            c = '\n';
                            s += 2;
                        } else {
                            c = *s++;
                        }
		    } else {
                        c = *s++;
		    }
		} else {
                    c = *s++;
		}
		pool_write (p, &c, 1);
	    }
            str.data = (char *) pool_start(p);
            str.len = pool_length(p);
	    add_to_widget_history (ident, str);
            pool_free (p);
            if (!*s)
                return;
            s++;
	}
    }
}

char *selection_get_line (void *data, int line);

static int draw_text_input_history (CWidget * text_input, CStr *r)
{
    CWidget *w;
    int x, y;
    int i;
    int columns, lines;
    struct textinput_history h;

    memset(&h, '\0', sizeof(h));

    if (text_input->options & TEXTINPUT_PASSWORD)	/* password lines, not allowed a history! */
	return 1;

    x = text_input->x;
    CPushFont ("editor", 0);
    columns = (text_input->width - WIDGET_SPACING * 3 - 4 - 6 - 20) / FONT_MEAN_WIDTH;
    w = CWidgetOfWindow (text_input->parentid);

    if (!w) {
    	CPopFont ();
	return 0;
    }

    if (text_input->y > w->height / 2) {
	get_history_list (text_input->ident, 1, &h);
	lines = (text_input->y - 2 - WIDGET_SPACING * 2 - 4 - 6) / FONT_PIX_PER_LINE;
	lines = clip_lines (lines, h.last);
	y = text_input->y - lines * FONT_PIX_PER_LINE - WIDGET_SPACING * 2 - 4 - 6;
        i = CListboxDialog (w->winid, x, y, columns, lines, 0, h.last - lines, h.last - 1, h.last, selection_get_line, (void *) &h.input);
    } else {
	get_history_list (text_input->ident, 0, &h);
	lines = (w->height - text_input->height - text_input->y - 2 - WIDGET_SPACING * 2 - 4 - 6) / FONT_PIX_PER_LINE;
	lines = clip_lines (lines, h.last);
	y = text_input->y + text_input->height;
        i = CListboxDialog (w->winid, x, y, columns, lines, 0, 0, 0, h.last, selection_get_line, (void *) &h.input);
    }

    CPopFont ();        

    if (i < 0)
        return 1;

    *r = h.input[i];

    return 0;
}

static int draw_selection_history (CWidget * text_input, CStr *r)
{
    CWidget *w;
    int x, y;
    int columns, lines;
    int cancel;
    x = text_input->x;
    CPushFont ("editor", 0);
    columns = (text_input->width - WIDGET_SPACING * 3 - 4 - 6 - 20) / FONT_MEAN_WIDTH;
    w = CWidgetOfWindow (text_input->parentid);
    if (!w) {
	CPopFont ();
	return 0;
    }
    if (text_input->y > w->height / 2) {
	lines = (text_input->y - 2 - WIDGET_SPACING * 2 - 4 - 6) / FONT_PIX_PER_LINE;
	y = text_input->y - lines * FONT_PIX_PER_LINE - WIDGET_SPACING * 2 - 4 - 6;
    } else {
	lines =
	    (w->height - text_input->height - text_input->y - 2 - WIDGET_SPACING * 2 - 4 -
	     6) / FONT_PIX_PER_LINE;
	y = text_input->y + text_input->height;
    }
    cancel = edit_get_text_from_selection_history (w->winid, x, y, columns, lines, r);
    CPopFont ();
    return cancel;
}

static char *draw_selection_completion (CWidget * text_input)
{
    CWidget *w;
    char *r;
    int x, y;
    int columns, lines;
    x = text_input->x;
    CPushFont ("editor", 0);
    columns = (text_input->width - WIDGET_SPACING * 3 - 4 - 6 - 20) / FONT_MEAN_WIDTH;
    w = CWidgetOfWindow (text_input->parentid);
    if (!w) {
	CPopFont ();
	return 0;
    }
    if (text_input->y > w->height / 2) {
	lines = (text_input->y - 2 - WIDGET_SPACING * 2 - 4 - 6) / FONT_PIX_PER_LINE;
	y = text_input->y - lines * FONT_PIX_PER_LINE - WIDGET_SPACING * 2 - 4 - 6;
    } else {
	lines =
	    (w->height - text_input->height - text_input->y - 2 - WIDGET_SPACING * 2 - 4 -
	     6) / FONT_PIX_PER_LINE;
	y = text_input->y + text_input->height;
    }
    r = user_file_list_complete (w->winid, x, y, columns, lines, text_input->text.data);
    CPopFont ();
    return r;
}

void render_passwordinput (CWidget * wdt)
{
    int wc, k, l, w = wdt->width, h = wdt->height;
    Window win;
    char *password;

    CPushFont ("editor", 0);

    win = wdt->winid;

    CSetBackgroundColor (COLOR_WHITE);
    CSetColor (COLOR_BLACK);
    password = (char *) strdup (wdt->text.data);
    memset (password, '*', strlen (wdt->text.data));
    CImageString (win, FONT_OFFSET_X + 3 + TEXTINPUT_RELIEF,
		      FONT_OFFSET_Y + 3 + TEXTINPUT_RELIEF,
		      password);
    CSetColor (COLOR_WHITE);
    l = CImageStringWidth (password);
    k = min (l, w - 6);
    memset (password, 0, strlen (password));
    free (password);
    CRectangle (win, 3, 3, k, option_text_line_spacing + 1);
    CLine (win, 3, 4, 3, h - 5);
    CLine (win, 3, h - 4, k + 3, h - 4);
    CRectangle (win, k + 3, 3, w - 6 - k, h - 6);
    (*look->render_passwordinput_tidbits) (wdt, win == CGetFocus ());
    wc = 3 + TEXTINPUT_RELIEF + 1 + CImageTextWidth (password, wdt->cursor);
    set_cursor_position (win, wc, 5, 0, h - 5, CURSOR_TYPE_TEXTINPUT, 0, 0, 0, 0);
    CPopFont ();
    return;
}

int utf8_to_wchar_t_one_char_safe (C_wchar_t * c, const char *t, int n);
int propfont_convert_to_long_printable (C_wchar_t c, C_wchar_t * t);
int propfont_width_of_long_printable (C_wchar_t c);

static int width_input_text (int max_width, char *p, int n)
{
    int l, x = 0;
    char *q = p + n;
    for (; p < q; p += l) {
	C_wchar_t c;
	int w;
        c = *p;
	if (c == '\0' || c == '\t' || c == '\n') {
            l = 1;
	    w = FONT_PER_CHAR ('^') + FONT_PER_CHAR (c + '@');
	} else {
	    l = utf8_to_wchar_t_one_char_safe (&c, p, q - p);
	    w = propfont_width_of_long_printable (c);
	}
	x += w;
	if (x > max_width)
	    break;
    }
    return x;
}

/* return right margin of last char */
static int draw_input_text (CWidget * wdt, int x, int y, int max_width, int m1, int m2)
{
    int n, l;
    long color = -1;
    char *p, *text;

    n = wdt->text.len;
    text = wdt->text.data;

    for (p = text + wdt->firstcolumn; p < text + n; p += l) {
	C_wchar_t c, lp[16];
	int w;
        int glyphs = 0;
        c = *p;
	if (c == '\0' || c == '\t' || c == '\n') {
            l = 1;
	    w = FONT_PER_CHAR ('^') + FONT_PER_CHAR (c + '@');
	    lp[0] = '^';
	    lp[1] = c + '@';
            glyphs = 2;
	} else {
	    l = utf8_to_wchar_t_one_char_safe (&c, p, text + n - p);
	    w = propfont_convert_to_long_printable (c, lp);
            while(lp[glyphs])
                glyphs++;
	}
	if (text + m1 <= p && p < text + m2 && color != COLOR_WHITE) {
            CSetBackgroundColor (COLOR_BLACK);
            CSetColor (COLOR_WHITE);
            color = 1;
	} else if (glyphs > 1 && color != color_widget(18)) {
            CSetBackgroundColor (COLOR_WHITE);
	    CSetColor (color_widget(18));
            color = 1;
	} else if (color != COLOR_BLACK) {
            CSetBackgroundColor (COLOR_WHITE);
	    CSetColor (COLOR_BLACK);
            color = 1;
        }
        CImageTextWC (wdt->winid, x, y, 0, lp, glyphs);
	x += w;
	if (x > max_width) {
	    x = max_width;
            break;
        }
    }
    if (color != COLOR_WHITE) {
        CSetBackgroundColor (COLOR_BLACK);
        CSetColor (COLOR_WHITE);
    }
    return x;
}

void render_textinput (CWidget * wdt)
{
    int wc, isfocussed = 0;
    int f, k, n;
    int m1, m2;
    int w = wdt->width, h = wdt->height;
    Window win;
    char *text;

    input_insert (wdt, INPUT_INSERT_FLUSH);

    if (wdt->options & TEXTINPUT_PASSWORD) {
	render_passwordinput (wdt);
	return;
    }
    
    CPushFont ("editor", 0);
    win = wdt->winid;
    isfocussed = (win == CGetFocus ());

/*This is a little untidy, but it will account for uneven font widths
   without having to think to hard */

    n = wdt->text.len;
    text = wdt->text.data;

    do {
	f = 0;
/*wc is the position of the cursor from the left of the input window */
        if (wdt->firstcolumn > wdt->cursor)
            wdt->firstcolumn = wdt->cursor;
	wc = 3 + TEXTINPUT_RELIEF + 1 + width_input_text(w, text + wdt->firstcolumn, wdt->cursor - wdt->firstcolumn);

	/*now lets make sure the cursor is well within the view */

/*except for when the cursor is at the end of the line */
        if (width_input_text(w, text + wdt->firstcolumn, n - wdt->firstcolumn) + 6 + TEXTINPUT_RELIEF * 2 + h <= w) {
            f = 0;
	} else if (wc > max (w - FONT_MEAN_WIDTH * 8 - h, w * 2 / 3 - h)) {
	    wdt->firstcolumn += count_one_utf8_char_sloppy(text + wdt->firstcolumn);
	    f = 1;
	}
	if (wc < min (FONT_MEAN_WIDTH * 8, w / 3)) {
	    wdt->firstcolumn--;
            while(wdt->firstcolumn > 0 && count_one_utf8_char(text + wdt->firstcolumn) < 0)
                wdt->firstcolumn--;
	    f = 1;
	    /*Unless of course we are at the beginning of the string */
	    if (wdt->firstcolumn <= 0) {
		wdt->firstcolumn = 0;
		f = 0;
	    }
	}
    } while (f);		/*recalculate if firstcolumn has changed */

    CSetColor (COLOR_WHITE);

    k = w - 3 - 3 - h;

/* top margin */
    CRectangle (win, 3, 3, k, option_text_line_spacing + 1);

/* bottom margin */
    CLine (win, 3, h - 4, k, h - 4);

/* left margin: */
    CLine (win, 3, 4, 3, h - 5);

/* now draw the visible part of the string */
    wdt->mark1 = min (wdt->mark1, n);
    wdt->mark2 = min (wdt->mark2, n);
    m1 = min (wdt->mark1, wdt->mark2);
    m2 = max (wdt->mark1, wdt->mark2);

    k = draw_input_text (wdt, FONT_OFFSET_X + 3 + TEXTINPUT_RELIEF, FONT_OFFSET_Y + 3 + TEXTINPUT_RELIEF, w - h - 3, m1, m2);

/* right margin */
    if (w - h - 3 - k > 0)
        CRectangle (win, k, 3, w - h - 3 - k, h - 6);

    (*look->render_textinput_tidbits) (wdt, isfocussed);

    set_cursor_position (win, wc, 5, 0, h - 5, CURSOR_TYPE_TEXTINPUT, 0, 0, 0, 0);
    CPopFont();
    return;
}


void text_input_destroy (CWidget * w)
{
    CAddToTextInputHistory (w->ident, w->text);
}

static void xy (int x, int y, int *x_return, int *y_return)
{
    *x_return = x - (3 + TEXTINPUT_RELIEF + 1);
    *y_return = 0;
}

static long cp (CWidget * wdt, int x, int y)
{
    int n, a = 0, l;
    char *p, *text;

    n = wdt->text.len;
    text = wdt->text.data;

    for (p = text + wdt->firstcolumn; p < text + n; p += l) {
	C_wchar_t c;
	int w;
        c = *p;
	if (c == '\0' || c == '\t' || c == '\n') {
            l = 1;
	    w = FONT_PER_CHAR ('^') + FONT_PER_CHAR (c + '@');
	} else {
	    l = utf8_to_wchar_t_one_char_safe (&c, p, text + n - p);
	    w = propfont_width_of_long_printable (c);
	}
	a += w;
	if (a > x)
	    return p - text;
    }
    return n;
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
    w->mark2 = w->mark1 = w->cursor;
}

static void fin_mark (CWidget * w)
{
    w->mark2 = w->mark1 = -1;
}

static void release_mark (CWidget * w, XEvent * event)
{
    w->mark2 = w->cursor;
    if (w->mark2 != w->mark1 && event) {
	XSetSelectionOwner (CDisplay, XA_PRIMARY, w->winid, event->xbutton.time);
	XSetSelectionOwner (CDisplay, ATOM_ICCCM_P2P_CLIPBOARD, w->winid, event->xbutton.time);
    }
}

static char *get_block (CWidget * w, long start_mark, long end_mark, int *type, int *l)
{
    char *t;
    if (w->options & TEXTINPUT_PASSWORD) {
	*type = DndText;
	*l = 0;
	return (char *) strdup ("");
    }
    *l = abs (w->mark2 - w->mark1);
    t = CMalloc (*l + 1);
    memcpy (t, w->text.data + min (w->mark1, w->mark2), *l);
    t[*l] = 0;
    if (*type == DndFile || *type == DndFiles) {
	char *s;
	int i;
	s = CDndFileList (t, l, &i);
	free (t);
	t = s;
    }
    return t;
}

static void move (CWidget * w, long click, int row)
{
    while(click > 0 && count_one_utf8_char(w->text.data + click) < 0)
        click--;
    w->cursor = click;
    if (w->mark2 == -1)
	w->mark1 = click;
    w->mark2 = click;
}

static void motion (CWidget * w, long click)
{
    w->mark2 = click;
}

char *filename_from_url (char *data, int size, int i);

static int insert_drop (CWidget * w, Window from, unsigned char *data, int size, int xs, int ys, Atom type, Atom action)
{
    int cursor;
    char *f;
    int x, y, i;
    if (xs < 0 || ys < 0 || xs >= w->width || ys >= w->height)
	return 1;
    xy (xs, ys, &x, &y);
    f = filename_from_url ((char *) data, size, 0);
    data = (unsigned char *) f;
    cursor = w->cursor = cp (w, x, y);
    if (type == XInternAtom (CDisplay, "url/url", False) || \
	type == XInternAtom (CDisplay, "text/uri-list", False))
	if (!strncmp ((char *) data, "file:/", 6))
	    data += 5;
    input_insert (w, INPUT_INSERT_FLUSH);
    for (i = 0; i < size && data[i] != '\n' && data[i]; i++)
	input_insert (w, data[i] < ' ' ? ' ' : data[i]);
    input_insert (w, INPUT_INSERT_FLUSH);
    if (cursor > w->text.len)
	cursor = w->text.len;
    w->cursor = cursor;
    free (f);
    return 0;
}

static char *mime_majors[3] =
{"text", 0};

static struct mouse_funcs input_mouse_funcs =
{
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
    (int (*)(void *, Window, unsigned char *, int, int, int, Atom, Atom)) insert_drop,
    0,
    DndText,
    mime_majors
};

CWidget *CDrawTextInputP (const char *identifier, Window parent, int x, int y,
		          int width, int height, int maxlen, const char *text)
{
    CStr s;
    if (text == TEXTINPUT_LAST_INPUT) {
        s = CLastInput (identifier);
    } else {
        s.data = (char *) text;
        s.len = strlen(text);
    }
    return CDrawTextInput (identifier, parent, x, y, width, height, maxlen, &s);
}

/*
   This will reallocate a previous draw of the same identifier.
   so you can draw the same widget over and over without flicker
 */
CWidget *CDrawTextInput (const char *identifier, Window parent, int x, int y,
		     int width, int height, int maxlen, const CStr *text)
{
    CWidget *wdt;
    CStr s;

    if (text == TEXTINPUT_LAST_INPUT) {
        s = CStr_dupstr(CLastInput (identifier));
    } else {
        s = CStr_dupstr(*text);
    }

    CPushFont ("editor", 0);
    if (!(wdt = CIdent (identifier))) {
	if (width == AUTO_WIDTH)
	    width = width_input_text (102400, text->data, text->len) + 6 + TEXTINPUT_RELIEF * 2;
	if (height == AUTO_HEIGHT)
	    height = FONT_PIX_PER_LINE + 6 + TEXTINPUT_RELIEF * 2;

	set_hint_pos (x + width + WIDGET_SPACING, y + height + WIDGET_SPACING);

	wdt = CSetupWidget (identifier, parent, x, y,
	 width, height, C_TEXTINPUT_WIDGET, INPUT_KEY, COLOR_FLAT, 1);

/* For the text input widget we need enough memory allocated to the label
   for it to grow to maxlen, so reallocate it */

	wdt->text = s;
	wdt->cursor = s.len;
	wdt->firstcolumn = 0;
	wdt->destroy = text_input_destroy;
	wdt->options |= WIDGET_TAKES_SELECTION;
	wdt->funcs = mouse_funcs_new (wdt, &input_mouse_funcs);

	xdnd_set_dnd_aware (CDndClass, wdt->winid, 0);
	xdnd_set_type_list (CDndClass, wdt->winid, xdnd_typelist_send[DndText]);
    } else {			/*redraw the thing so it doesn't flicker if its redrawn in the same place.
				   Also, this doesn't need an undraw */
	CSetWidgetSize (identifier, width, height);
	wdt->x = x;
	wdt->y = y;
	XMoveWindow (CDisplay, wdt->winid, x, y);
	CStr_free (&wdt->text);
	wdt->text = s;
	wdt->cursor = s.len;
	wdt->firstcolumn = 0;
	wdt->keypressed = 0;
	render_textinput (wdt);
    }
    CPopFont();

    return wdt;
}

void paste_prop (void *data, void (*insert) (void *, int), Window win, unsigned prop, int delete);

void textinput_insert (CWidget * w, CStr c)
{
    int i;
    input_insert (w, INPUT_INSERT_FLUSH);
    for (i = 0; i < c.len; i++)
        input_insert (w, (unsigned char) c.data[i]);
    input_insert (w, INPUT_INSERT_FLUSH);
}

static void input_insert (CWidget * w, int c)
{
    static unsigned char buf[4096];     /* implement a simple freeze-thaw caching system to speed inserts */
    static int buf_len = 0;
    CStr s;
    if (c == INPUT_INSERT_FLUSH && !buf_len)
	return;
    if (!w->keypressed) {
	w->keypressed = 1;
	w->cursor = 0;
	w->text.len = 0;
	w->text.data[0] = '\0';
    }
    if (w->cursor > w->text.len)
	w->cursor = w->text.len;
    if (c != INPUT_INSERT_FLUSH)
        buf[buf_len++] = c;
    if (c == INPUT_INSERT_FLUSH || buf_len == sizeof(buf)) {
	int i;
	s.len = w->text.len + buf_len;
	s.data = (char *) CMalloc (s.len + 1);
	memcpy (s.data, w->text.data, w->cursor);
	for (i = 0; i < buf_len; i++)
	    s.data[w->cursor + i] = buf[i];
	memcpy (s.data + w->cursor + buf_len, w->text.data + w->cursor, w->text.len - w->cursor);
	w->cursor += buf_len;
	s.data[s.len] = '\0';
	buf_len = 0;
	CStr_free (&w->text);
	w->text = s;
    }
}

static void xy (int x, int y, int *x_return, int *y_return);
static long cp (CWidget * wdt, int x, int y);
void selection_send (XSelectionRequestEvent * rq);
void paste_convert_selection (Window w);

int eh_textinput (CWidget * w, XEvent * xevent, CEvent * cwevent)
{
    int handled = 0, save_options;
    char *u = 0;

    CPushFont ("editor");

    input_insert (w, INPUT_INSERT_FLUSH);

    switch (xevent->type) {
    case FocusIn:
    case FocusOut:
	render_textinput (w);
	break;
    case SelectionRequest:
	selection_replace (w->text);
	selection_send (&(xevent->xselectionrequest));
	render_textinput (w);
        handled = 1;
        break;
    case SelectionNotify:
        {
            int cursor;
	    cursor = w->keypressed ? w->cursor : 0;
	    paste_prop ((void *) w, (void (*)(void *, int)) input_insert,
	    xevent->xselection.requestor, xevent->xselection.property, True);
            input_insert (w, INPUT_INSERT_FLUSH);
	    w->mark1 = w->mark2 = 0;
	    w->cursor = cursor;
	    render_textinput (w);
        }
	break;
    case ButtonPress:
	resolve_button (xevent, cwevent);
	if (!(w->options & TEXTINPUT_PASSWORD)) {
	    if (xevent->xbutton.x >= w->width - w->height) {
                CStr s;
		w->options &= 0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT;
		w->options |= BUTTON_PRESSED;
		if (!draw_text_input_history (w, &s)) {
                    CStr_free(&w->text);
                    w->text = CStr_dupstr(s);
		    w->keypressed = 1;
		    w->cursor = s.len;
		    w->firstcolumn = 0;
		}
	    } else {
		input_mouse_mark (w, xevent, cwevent);
		w->options &= 0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT;
	    }
	}
	render_textinput (w);
	CFocus (w);
    case ButtonRelease:
	if (!(w->options & TEXTINPUT_PASSWORD))
	    input_mouse_mark (w, xevent, cwevent);
	render_textinput (w);
	break;
    case Expose:
	if (xevent->xexpose.count)
            break;
    case EnterNotify:
	w->options &= 0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT;
	if (xevent->xbutton.x >= w->width - w->height)
	    w->options |= BUTTON_HIGHLIGHT;
	render_textinput (w);
	break;
    case MotionNotify:
	save_options = w->options;
	w->options &= ~(BUTTON_PRESSED | BUTTON_HIGHLIGHT);
	if (xevent->xmotion.x >= w->width - w->height) {
	    w->options |= BUTTON_HIGHLIGHT;
	    if (save_options != w->options)
		render_textinput (w);
            break;
	} else {
	    if (!xevent->xmotion.state) {
		if (save_options != w->options)
		    render_textinput (w);
                break;
	    }
	}
	if (!(w->options & TEXTINPUT_PASSWORD))
	    input_mouse_mark (w, xevent, cwevent);
	render_textinput (w);
	break;
    case LeaveNotify:
	w->options &= 0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT;
	render_textinput (w);
	break;
    case KeyPress:
	cwevent->ident = w->ident;
	cwevent->state = xevent->xkey.state;

	if (cwevent->command > 0) {
            long n;
            CStr s;
	    switch (cwevent->command) {
	    case CK_Complete:
                u = draw_selection_completion (w);
		if (u) {
                    CStr_free (&w->text);
                    w->text = CStr_dup (u);
		    w->cursor = 0;
                }
		handled = 1;
		break;
	    case CK_Selection_History:
		if (!draw_selection_history (w, &s))
                    for (n = 0; n < s.len; n++)
			input_insert (w, s.data[n]);
		handled = 1;
		break;
	    case CK_Insert_Unicode:
		u = (char *) CGetUnichar (CRoot, "Unicode characters");
		if (u)
		    while (*u)
			input_insert (w, *u++);
		handled = 1;
		break;
	    case CK_XPaste:
		if (!XGetSelectionOwner (CDisplay, XA_PRIMARY)) {
                    int cursor;
		    cursor = w->cursor;
		    paste_prop ((void *) w, (void (*)(void *, int)) input_insert,
				CRoot, XA_CUT_BUFFER0, False);
                    input_insert (w, INPUT_INSERT_FLUSH);
		    w->cursor = cursor;
		} else {
                    paste_convert_selection(w->winid);
                    handled = 0;
                    break;
		}
		handled = 1;
		break;
	    case CK_BackSpace:
		if (w->mark1 != w->mark2) {
		    Cmemmove (w->text.data + min (w->mark1, w->mark2), w->text.data + max (w->mark1, w->mark2), w->text.len - max (w->mark1, w->mark2) + 1);
                    w->text.len -= max (w->mark1, w->mark2);
		    w->cursor = min (w->mark1, w->mark2);
		} else if (w->cursor > 0) {
                    n = w->cursor;
                    while (w->cursor > 0) {
                        w->cursor--;
                        if (count_one_utf8_char(&w->text.data[w->cursor]) >= 0)
                            break;
                    }
		    Cmemmove (w->text.data + w->cursor, w->text.data + n, w->text.len - n + 1);
                    w->text.len -= (n - w->cursor);
		}
		handled = 1;
		break;
	    case CK_Left:
                while (w->cursor > 0) {
                    w->cursor--;
                    if (count_one_utf8_char(&w->text.data[w->cursor]) >= 0)
                        break;
                }
		handled = 1;
		break;
	    case CK_Down:
	    case CK_Up:
	    case CK_Down_Highlight:
	    case CK_Up_Highlight:
		if (cwevent->state & ShiftMask) {
		    w->options |= BUTTON_PRESSED;
		    if (!draw_text_input_history (w, &s)) {
                        CStr_free(&w->text);
                        w->text = CStr_dupstr(s);
			w->keypressed = 1;
			w->cursor = s.len;
			w->firstcolumn = 0;
		    }
		    w->options &= 0xFFFFFFFFUL - BUTTON_PRESSED - BUTTON_HIGHLIGHT;
		    handled = 1;
		}
		break;
	    case CK_Right:
	        w->cursor += count_one_utf8_char_sloppy(&w->text.data[w->cursor]);
                if (w->cursor > w->text.len)
                    w->cursor = w->text.len;
		handled = 1;
		break;
	    case CK_Delete:
		if (w->mark1 != w->mark2) {
		    Cmemmove (w->text.data + min (w->mark1, w->mark2), w->text.data + max (w->mark1, w->mark2), w->text.len - max (w->mark1, w->mark2) + 1);
                    w->text.len -= max (w->mark1, w->mark2);
		    w->cursor = min (w->mark1, w->mark2);
		} else if (w->cursor >= 0 && w->cursor < w->text.len) {
                    n = w->cursor;
	            n += count_one_utf8_char_sloppy(&w->text.data[w->cursor]);
		    Cmemmove (w->text.data + w->cursor, w->text.data + n, w->text.len - n + 1);
                    w->text.len -= (n - w->cursor);
		}
		handled = 1;
		break;
	    case CK_Home:
		w->cursor = 0;
		handled = 1;
		break;
	    case CK_End:
		w->cursor = w->text.len;
		handled = 1;
		break;
	    }
	    w->keypressed |= handled;
	}

	if (!handled && ((cwevent->state & MyAltMask) || (cwevent->state & ControlMask)) && (handled = CHandleGlobalKeys (w, xevent, cwevent))) {
            /* pass on insert */
        } else if (cwevent->xlat_len > 0) {
            if (!((w->options & TEXTINPUT_NUMBERS) && !(cwevent->xlat[0] >= '0' && cwevent->xlat[0] <= '9'))) {
                int l;
                for (l = 0; l < cwevent->xlat_len; l++)
                    input_insert (w, cwevent->xlat[l]);
            }
	    handled = 1;
	}

	if (handled) {
	    w->mark1 = w->mark2 = 0;
	    render_textinput (w);
	}
	cwevent->text = w->text.data;
    }
    input_insert (w, INPUT_INSERT_FLUSH);

    CPopFont ();

    return handled;
}

void input_mouse_mark (CWidget * w, XEvent * event, CEvent * ce)
{
    CPushFont ("editor", 0);
    mouse_mark (event, ce->double_click, w->funcs);
    CPopFont ();
}








