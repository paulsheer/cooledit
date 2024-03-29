/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* wordproc.c - word-processor mode for the editor: does dynamic
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include "edit.h"
#include "stringtools.h"

#ifdef MIDNIGHT
#define tab_width option_tab_spacing
#endif

struct WStr_c_ {
    unsigned char c;
    int charwidth;
};

struct WStr_ {
    struct WStr_c_ *data;
    int len;
};

typedef struct WStr_ WStr;

int line_is_blank (WEdit * edit, long line);
int edit_width_of_long_printable (int c);


#define NO_FORMAT_CHARS_START "-+*\\,.;:&>"

static long line_start (WEdit * edit, long line)
{E_
    static long p = -1, l = 0;
    int c;
    if (p == -1 || labs (l - line) > labs (edit->curs_line - line)) {
	l = edit->curs_line;
	p = edit->curs1;
    }
    if (line < l)
	p = edit_move_backward (edit, p, l - line);
    else if (line > l)
	p = edit_move_forward (edit, p, line - l, 0);
    l = line;
    p = edit_bol (edit, p);
    while (strchr ("\t ", c = edit_get_byte (edit, p)))
	p++;
    return p;
}

static int bad_line_start (WEdit * edit, long p)
{E_
    int c;
    c = edit_get_byte (edit, p);
    if (c == '.') {		/* `...' is acceptable */
	if (edit_get_byte (edit, p + 1) == '.')
	    if (edit_get_byte (edit, p + 2) == '.')
		return 0;
	return 1;
    }
    if (c == '-') {
	if (edit_get_byte (edit, p + 1) == '-')
	    if (edit_get_byte (edit, p + 2) == '-')
		return 0;	/* `---' is acceptable */
	return 1;
    }
    if (strchr (NO_FORMAT_CHARS_START, c))
	return 1;
    return 0;
}

static long begin_paragraph (WEdit * edit, long p, int force)
{E_
    int i;
    for (i = edit->curs_line - 1; i > 0; i--) {
	if (line_is_blank (edit, i)) {
	    i++;
	    break;
	}
	if (force) {
	    if (bad_line_start (edit, line_start (edit, i))) {
		i++;
		break;
	    }
	}
    }
    return edit_move_backward (edit, edit_bol (edit, edit->curs1), edit->curs_line - i);
}

static long end_paragraph (WEdit * edit, long p, int force)
{E_
    int i;
    for (i = edit->curs_line + 1; i < edit->total_lines; i++) {
	if (line_is_blank (edit, i)) {
	    i--;
	    break;
	}
	if (force)
	    if (bad_line_start (edit, line_start (edit, i))) {
		i--;
		break;
	    }
    }
    return edit_eol (edit, edit_move_forward (edit, edit_bol (edit, edit->curs1), i - edit->curs_line, 0));
}

/* returns 1 on error */
static int get_paragraph (WEdit * edit, long p, long q, int indent, WStr *r)
{E_
    struct WStr_c_ *s;
    r->data = (struct WStr_c_ *) malloc ((q - p + 1) * sizeof (struct WStr_c_));
    if (!r->data)
	return 1;
    for (s = r->data; p < q; p++, s++) {
	if (indent)
	    if (edit_get_byte (edit, p - 1) == '\n')
		while (strchr ("\t ", edit_get_byte (edit, p)))
		    p++;
        for (;;) {
            C_wchar_t c;
	    s->c = edit_get_byte (edit, p);
            if ((c = edit_get_wide_byte (edit, p)) == -1) {
                s->charwidth = 0;
                s++, p++;
                if (!(p < q))
                    goto out;
                continue;
            }
            s->charwidth = edit_width_of_long_printable (c == '\n' ? ' ' : c);
            break;
        }
    }
  out:

    r->len = (int) (s - r->data);
    return 0;
}

static void strip_newlines (WStr *t)
{E_
    int i;
    for (i = 0; i < t->len; i++)
        if (t->data[i].c == '\n')
            t->data[i].c = ' ';
}

/* 
   This is a copy of the function 
   int calc_text_pos (WEdit * edit, long b, long *q, int l)
   in propfont.c  :(
   It calculates the number of chars in a line specified to length l in pixels
 */
extern int tab_width;
static inline int next_tab_pos (int x)
{E_
    return x += tab_width - x % tab_width;
}

static int line_pixel_length (WStr *t, long b, int l)
{E_
    int x = 0, xn = 0;
    struct WStr_c_ *c;
    for (;;) {
        if (b >= t->len)
            break;
	c = &t->data[b];
	switch (c->c) {
	case '\n':
	    return b;
	case '\t':
	    xn = next_tab_pos (x);
	    break;
	default:
	    xn = x + c->charwidth;
	    break;
	}
	if (xn > l)
	    break;
	x = xn;
	b++;
    }
    return b;
}

/* find the start of a word */
static int next_word_start (const WStr *t, int q)
{E_
    int i;
    for (i = q;; i++) {
        if (i >= t->len)
            return -1;
	switch (t->data[i].c) {
	case '\n':
	    return -1;
	case '\t':
	case ' ':
	    for (;; i++) {
                if (i >= t->len)
                    return -1;
		if (t->data[i].c == '\n')
		    return -1;
		if (t->data[i].c != ' ' && t->data[i].c != '\t')
		    return i;
	    }
	    break;
	}
    }
}

/* find the start of a word */
static int word_start (const WStr *t, int q)
{E_
    int i = q;
    assert (i >= 0);
    if (q >= t->len)
        return -1;
    if (t->data[q].c == ' ' || t->data[q].c == '\t')
	return next_word_start (t, q);
    for (;;) {
	int c;
	if (!i)
	    return -1;
	c = t->data[i - 1].c;
	if (c == '\n')
	    return -1;
	if (c == ' ' || c == '\t')
	    return i;
	i--;
    }
}

/* replaces ' ' with '\n' to properly format a paragraph */
static void format_this (WStr *t, int indent)
{E_
    int q = 0, ww;
    strip_newlines (t);
    ww = option_word_wrap_line_length * FONT_MEAN_WIDTH - indent;
    if (ww < FONT_MEAN_WIDTH * 2)
	ww = FONT_MEAN_WIDTH * 2;
    for (;;) {
	int p;
	q = line_pixel_length (t, q, ww);
	if (q >= t->len)
	    break;
	if (t->data[q].c == '\n')
	    break;
	p = word_start (t, q);
	if (p == -1)
	    q = next_word_start (t, q);	        /* Return the end of the word if the beginning
						   of the word is at the beginning of a line 
						   (i.e. a very long word) */
	else
	    q = p;
	if (q == -1)	/* end of paragraph */
	    break;
	if ((q - 1) >= 0 && (q - 1) < t->len)
	    t->data[q - 1].c = '\n';
    }
}

static void replace_at (WEdit * edit, long q, int c)
{E_
    edit_cursor_move (edit, q - edit->curs1);
    edit_delete (edit);
    edit_insert_ahead (edit, c);
}

void edit_insert_indent (WEdit * edit, int indent);

/* replaces a block of text */
static void put_paragraph (WEdit * edit, WStr *t, long p, long q, int indent)
{E_
    long cursor;
    int i, c = 0;
    cursor = edit->curs1;
    if (indent)
	while (strchr ("\t ", edit_get_byte (edit, p)))
	    p++;
    for (i = 0; i < t->len; i++, p++) {
	if (i && indent) {
	    if (t->data[i - 1].c == '\n' && c == '\n') {
		while (strchr ("\t ", edit_get_byte (edit, p)))
		    p++;
	    } else if (t->data[i - 1].c == '\n') {
		long curs;
		edit_cursor_move (edit, p - edit->curs1);
		curs = edit->curs1;
		edit_insert_indent (edit, indent);
		if (cursor >= curs)
		    cursor += edit->curs1 - p;
		p = edit->curs1;
	    } else if (c == '\n') {
		edit_cursor_move (edit, p - edit->curs1);
		while (strchr ("\t ", edit_get_byte (edit, p))) {
		    edit_delete (edit);
		    if (cursor > edit->curs1)
			cursor--;
		}
		p = edit->curs1;
	    }
	}
	c = edit_get_byte (edit, p);
	if (c != t->data[i].c)
	    replace_at (edit, p, t->data[i].c);
    }
    edit_cursor_move (edit, cursor - edit->curs1);	/* restore cursor position */
}

int edit_indent_width (WEdit * edit, long p);

static int test_indent (WEdit * edit, long p, long q)
{E_
    int indent;
    indent = edit_indent_width (edit, p++);
    if (!indent)
	return 0;
    for (; p < q; p++)
	if (edit_get_byte (edit, p - 1) == '\n')
	    if (indent != edit_indent_width (edit, p))
		return 0;
    return indent;
}

static void new_behaviour_message (WEdit * edit)
{E_
    char *filename;
    int fd;
    filename = catstrs (local_home_dir, PARMESS_FILE, NULL);
    if ((fd = open (filename, O_RDONLY)) < 0) {
	edit_message_dialog (_(" Format Paragraph "), "\
This message will not be displayed again\n\
\n\
The new \"format paragraph\" command will format\n\
text inside the selected region if one is highlighted.\n\
Otherwise it would try to find the bounds of the\n\
current paragraph using heuristics.");
	fd = creat (filename, 0400);
    }
    close (fd);
}

void format_paragraph (WEdit * edit, int force)
{E_
    long p, q;
    WStr t;
    int indent = 0;
    if (option_word_wrap_line_length < 2)
	return;
    if (force)
	new_behaviour_message (edit);
    if (force && !eval_marks (edit, &p, &q)) {
	p = edit_bol (edit, p);
	q = edit_eol (edit, q);
    } else {
	if (line_is_blank (edit, edit->curs_line))
	    return;
	p = begin_paragraph (edit, edit->curs1, force);
	q = end_paragraph (edit, edit->curs1, force);
    }
    CPushFont ("editor", 0);
    indent = test_indent (edit, p, q);
    if (get_paragraph (edit, p, q, indent, &t))
	return;
    if (!force) {
	int i;
	if (t.len > 0 && strchr (NO_FORMAT_CHARS_START, t.data[0].c)) {
	    free (t.data);
	    return;
	}
	for (i = 0; i < t.len - 1; i++) {
	    if (t.data[i].c == '\n') {
		if (strchr (NO_FORMAT_CHARS_START "\t ", t.data[i + 1].c)) {
		    free (t.data);
		    return;
		}
	    }
	}
    }
    format_this (&t, indent);
    put_paragraph (edit, &t, p, q, indent);
    free (t.data);
    CPopFont ();
}










