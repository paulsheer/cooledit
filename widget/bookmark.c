/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* bookmark.c
   Copyright (C) 1996-2022 Paul Sheer
 */



#include "inspect.h"
#include <config.h>
#include "edit.h"
#include "stringtools.h"

/* note, if there is more than one bookmark on a line, then they are
   appended after each other and the last one is always the one found
   by book_mark_found() i.e. last in is the one seen */

static inline struct _book_mark *double_marks (WEdit * edit, struct _book_mark *p)
{E_
    if (p->next)
	while (p->next->line == p->line)
	    p = p->next;
    return p;
}

static void free_bookmark (struct _book_mark *p)
{E_
    if (p->text)
        free (p->text);
    free (p);
}

/* returns the first bookmark on or before this line */
struct _book_mark *book_mark_find (WEdit * edit, int line)
{E_
    struct _book_mark *p, *q;
    if (!edit->book_mark) {
/* must have an imaginary top bookmark at line -1 to make things less complicated  */
	edit->book_mark = malloc (sizeof (struct _book_mark));
	memset (edit->book_mark, 0, sizeof (struct _book_mark));
	edit->book_mark->line = -1;
	return edit->book_mark;
    }

/* search in both directions at the same time. this allows efficient calls sequentially forward or backward */
    for (p = q = edit->book_mark; p || q;) {
	if (p && p->line > line)
	    p = 0;		/* gone past it going downward */
	if (q && q->next && q->next->line <= line)
	    q = 0;		/* gone past it going upward */
	if (p) {
	    if (p->line <= line) {
		if (p->next) {
		    if (p->next->line > line) {
			edit->book_mark = p;
			break;
		    }
		} else {
		    edit->book_mark = p;
		    break;
		}
	    }
	    p = p->next;
	}
	if (q) {
	    if (q->line <= line) {
		if (q->next) {
		    if (q->next->line > line) {
			edit->book_mark = q;
			break;
		    }
		} else {
		    edit->book_mark = q;
		    break;
		}
	    }
	    q = q->prev;
	}
    }

/* take the last book mark on the line */
    return double_marks (edit, edit->book_mark);
}

/* returns true if a bookmark exists at this line of colour c */
int book_mark_query_color (WEdit * edit, int line, int c)
{E_
    struct _book_mark *p;
    if (!edit->book_mark)
	return 0;
    for (p = book_mark_find (edit, line); p; p = p->prev) {
	if (p->line != line)
	    return 0;
	if (p->c == c)
	    return 1;
    }
    return 0;
}

/* returns the number of bookmarks at this line and a list of their colours in c 
   up to a maximum of 8 colours */
int book_mark_query_all (WEdit * edit, int line, struct _book_mark **c)
{E_
    int i;
    struct _book_mark *p;
    if (!edit->book_mark)
	return 0;
    for (i = 0, p = book_mark_find (edit, line); p && i < 8; p = p->prev, i++) {
	if (p->line != line)
	    return i;
	c[i] = p;
    }
    return i;
}

/* insert a bookmark at this line */
void book_mark_insert (WEdit * edit, int line, int c, int height, const char *text, int column_marker)
{E_
    struct _book_mark *p, *q;
    p = book_mark_find (edit, line);
    edit->force |= REDRAW_LINE;
    if (height)
        edit->force |= REDRAW_AFTER_CURSOR;
/* create list entry */
    q = malloc (sizeof (struct _book_mark));
    memset (q, 0, sizeof (struct _book_mark));
    q->line = line;
    q->c = c;
    q->height = height;
    q->column_marker = column_marker;
    if (text)
        q->text = Cstrdup (text);
    q->next = p->next;
/* insert into list */
    q->prev = p;
    if (p->next)
	p->next->prev = q;
    p->next = q;
#if !defined (GTK) && !defined (MIDNIGHT)
    render_scrollbar (edit->widget->vert_scrollbar);
#endif
}

/* remove a bookmark if there is one at this line matching this colour - c of -1 clear all */
/* returns non-zero on not-found */
int book_mark_clear (WEdit * edit, int line, int c)
{E_
    struct _book_mark *p, *q;
    int r = 1;
    int rend = 0;
    if (!edit->book_mark)
	return r;
    for (p = book_mark_find (edit, line); p; p = q) {
	q = p->prev;
	if (p->line == line && (p->c == c || c == -1)) {
	    r = 0;
	    edit->force |= REDRAW_LINE;
            if (p->height)
	        edit->force |= REDRAW_AFTER_CURSOR;
	    edit->book_mark = p->prev;
	    p->prev->next = p->next;
	    if (p->next)
		p->next->prev = p->prev;
	    rend = 1;
	    free_bookmark (p);
	    break;
	}
    }
/* if there is only our dummy book mark left, clear it for speed */
    if (edit->book_mark->line == -1 && !edit->book_mark->next) {
	free_bookmark (edit->book_mark);
	edit->book_mark = 0;
    }
#if !defined (GTK) && !defined (MIDNIGHT)
    if (rend)
	render_scrollbar (edit->widget->vert_scrollbar);
#endif
    return r;
}

/* clear all bookmarks matching this colour, if c is -1 clears all */
void book_mark_flush (WEdit * edit, int c)
{E_
    struct _book_mark *p, *q;
    int rend = 0;
    if (!edit->book_mark)
	return;
    edit->force |= REDRAW_PAGE;
    while (edit->book_mark->prev)
	edit->book_mark = edit->book_mark->prev;
    for (q = edit->book_mark->next; q; q = p) {
	p = q->next;
	if (q->c == c || c == -1) {
	    q->prev->next = q->next;
	    if (p)
		p->prev = q->prev;
	    rend = 1;
	    free_bookmark (q);
	}
    }
    if (!edit->book_mark->next) {
	free_bookmark (edit->book_mark);
	edit->book_mark = 0;
    }
#if !defined (GTK) && !defined (MIDNIGHT)
    if (rend)
	render_scrollbar (edit->widget->vert_scrollbar);
#endif
}

/* shift down bookmarks after this line */
void book_mark_inc (WEdit * edit, int line)
{E_
    int rend = 0;
    if (edit->book_mark) {
	struct _book_mark *p;
	p = book_mark_find (edit, line);
	for (p = p->next; p; p = p->next) {
	    p->line++;
	    rend = 1;
	}
    }
#if !defined (GTK) && !defined (MIDNIGHT)
    if (rend)
	render_scrollbar (edit->widget->vert_scrollbar);
#endif
}

/* shift up bookmarks after this line */
void book_mark_dec (WEdit * edit, int line)
{E_
    int rend = 0;
    if (edit->book_mark) {
	struct _book_mark *p;
	p = book_mark_find (edit, line);
	for (p = p->next; p; p = p->next) {
	    p->line--;
	    rend = 1;
	}
    }
#if !defined (GTK) && !defined (MIDNIGHT)
    if (rend)
	render_scrollbar (edit->widget->vert_scrollbar);
#endif
}



