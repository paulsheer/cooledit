/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* editwidget.c
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include "edit.h"

#ifndef MIDNIGHT
#include <X11/Xmd.h>		/* CARD32 */
#include <X11/Xatom.h>
#include "app_glob.c"
#include "coollocal.h"
#include "editcmddef.h"
#include "mousemark.h"
#include "stringtools.h"
#endif

#ifndef MIDNIGHT

extern int EditExposeRedraw;
CWidget *wedit = 0;

void shell_output_kill_jobs (WEdit * edit);
void CChildWait (pid_t p);


void edit_destroy_callback (CWidget * w)
{E_
    if (w) {
	shell_output_kill_jobs (w->editor);
	edit_clean (w->editor);
	if (w->editor)
	    free (w->editor);
	w->editor = NULL;
    } else
/* NLS ? */
	CError ("Trying to destroy non-existing editor widget.\n");
}

#define CLOSE_ON_NO_DATA

#define SHELL_INPUT_BUF_SIZE 1024

static void shell_output_write_callback (int fd, fd_set * reading, fd_set * writing, fd_set * error, void *data)
{E_
    unsigned char s[SHELL_INPUT_BUF_SIZE];
    int i;
    WEdit *edit;
    long start_mark, end_mark;
    edit = (WEdit *) data;
/* if the process is available for reading, and there is not selected text, then
 remove the watch on the process's stdin, and close the stdin */
    if (eval_marks (edit, &start_mark, &end_mark)) {
	struct shell_job *j;
	CRemoveWatch (fd, shell_output_write_callback, WATCH_WRITING);
	for (j = edit->jobs; j; j = j->next)
	    if (j->in == fd) {
		if (j->close_on_error)
		    close (fd);
		break;
	    }
	return;
    }
    for (i = 0; i < SHELL_INPUT_BUF_SIZE && start_mark + i < end_mark; i++)
	s[i] = edit_get_byte (edit, start_mark + i);
    while ((i = write (fd, s, i)) < 0 && errno == EINTR);
    if (i <= 0) {
	struct shell_job *j;
	for (j = edit->jobs; j; j = j->next)
	    if (j->in == fd || j->out == fd) {
		if (CChildExitted (j->pid, 0))
		    shell_output_kill_job (edit, j->pid, 0);
		break;
	    }
	return;
    }
    edit_cursor_move (edit, start_mark - edit->curs1);
    while (i--)
	edit_delete (edit);
    edit->force |= REDRAW_PAGE;
    edit_update_screen (edit);
}

#define SHELL_OUTPUT_BUF_SIZE 16384

static void shell_output_read_callback (int fd, fd_set * reading, fd_set * writing, fd_set * error, void *data)
{E_
    WEdit *edit;
    unsigned char s[SHELL_OUTPUT_BUF_SIZE];
    int i, n, move_mark = 0;
    long start_mark, end_mark;
    edit = (WEdit *) data;
    if (!eval_marks (edit, &start_mark, &end_mark))
	move_mark = (start_mark == edit->curs1);
    while ((i = read (fd, s, SHELL_OUTPUT_BUF_SIZE)) < 0 && errno == EINTR);
    if (i <= 0) {
	struct shell_job *j;
	for (j = edit->jobs; j; j = j->next)
	    if (j->in == fd || j->out == fd) {
		if (CChildExitted (j->pid, 0))
		    shell_output_kill_job (edit, j->pid, 0);
		break;
	    }
	return;
    }
    for (n = 0; n < i; n++)
	edit_insert (edit, s[n]);
/* must insert BEFORE and not INTO the selection */
    if (move_mark)
	edit_set_markers (edit, start_mark + i, end_mark + i, -1, -1);
    edit->force |= REDRAW_PAGE;
    edit_update_screen (edit);
}

void shell_output_add_job (WEdit * edit, int in, int out, pid_t pid, char *name, int close_on_error)
{E_
    struct shell_job *j;
    long start_mark, end_mark;

    if (edit->mark2 < 0)        /* second marker is not sticky with the cursor */
        edit_mark_cmd(edit, 0); /* unstick from cursor */

    CAddWatch (out, shell_output_read_callback, WATCH_READING, edit);
    if (!eval_marks (edit, &start_mark, &end_mark))
	CAddWatch (in, shell_output_write_callback, WATCH_WRITING, edit);
    else if (close_on_error) {
	close (in);
	in = -1;
    }
    j = malloc (sizeof (*j));
    memset (j, 0, sizeof (*j));
    j->next = edit->jobs;
    j->in = in;
    j->out = out;
    j->close_on_error = close_on_error;
    j->name = (char *) strdup (name);
    j->pid = pid;
    edit->jobs = j;
}

static void shell_output_destroy_job (struct shell_job *j, int send_term)
{E_
    if (j->out >= 0) {
	CRemoveWatch (j->out, shell_output_read_callback, WATCH_READING);
	close (j->out);
    }
    if (j->in >= 0) {
	CRemoveWatch (j->in, shell_output_write_callback, WATCH_WRITING);
	close (j->in);
    }
    if (send_term && j->pid > 0) {
	if (!kill (j->pid, SIGTERM))
	    CChildWait (j->pid);
    }
    free (j->name);
    memset (j, 0, sizeof (*j));
    free (j);
}

void shell_output_kill_jobs (WEdit * edit)
{E_
    struct shell_job *j, *n;
    if (!edit)
	return;
    for (j = edit->jobs; j; j = n) {
	n = j->next;
	shell_output_destroy_job (j, 1);
    }
    edit->jobs = 0;
}

void shell_output_kill_job (WEdit * edit, long pid, int send_term)
{E_
    struct shell_job **p;
    for (p = &edit->jobs; *p;) {
	if ((*p)->pid == (pid_t) pid) {
	    struct shell_job *t;
	    t = *p;
            *p = (*p)->next;
	    shell_output_destroy_job (t, send_term);
	} else {
	    p = &((*p)->next);
	}
    }
}

void link_hscrollbar_to_editor (CWidget * scrollbar, CWidget * editor, XEvent * xevent, CEvent * cwevent, int whichscrbutton);
int edit_get_line_height (WEdit * edit, int row);

static int edit_ypixel_to_row (WEdit * edit, int y_search)
{E_
    int row, y;

    if (y_search < 0)
        return y_search / FONT_PIX_PER_LINE;    /* allow scrolling up on select with mouse drag */

    for (y = 0, row = 0;; row++) {
        int h;
        h = edit_get_line_height (edit, edit->start_line + row);
        if (y + h >= y_search)
            break;
        y += h;
    }
    return row + 1;
}

/* returns the position in the edit buffer of a window click */
long edit_get_click_pos (WEdit * edit, int x, int y)
{E_
    long click, row;
/* (1) goto to left margin */
    click = edit_bol (edit, edit->curs1);

    row = edit_ypixel_to_row (edit, y);

/* (1) move up or down */
    if (row > (edit->curs_row + 1))
	click = edit_move_forward (edit, click, row - (edit->curs_row + 1), 0);
    if (row < (edit->curs_row + 1))
	click = edit_move_backward (edit, click, (edit->curs_row + 1) - row);

/* (3) move right to x pos */
    click = edit_move_forward3 (edit, click, x - edit->start_col - 1, 0);
    return click;
}

void edit_translate_xy (int xs, int ys, int *x, int *y)
{E_
    *x = xs - EDIT_TEXT_HORIZONTAL_OFFSET;
    *y = (ys - EDIT_TEXT_VERTICAL_OFFSET - option_text_line_spacing / 2 - 1);
}

extern int just_dropped_something;

static void edit_update_screen_ (WEdit * e, int event_type);

void mouse_redraw (WEdit * edit, long click, int event_type)
{E_
    edit->force |= REDRAW_PAGE | REDRAW_LINE;
    edit_update_curs_row (edit);
    edit_update_curs_col (edit);
    edit->prev_col = edit_get_col (edit);
    edit_update_screen_ (edit, event_type);
    edit->search_start = click;
}

static void xy (int x, int y, int *x_return, int *y_return)
{E_
    edit_translate_xy (x, y, x_return, y_return);
}

static long cp (WEdit * edit, int x, int y)
{E_
    return edit_get_click_pos (edit, x, y);
}

/* return 1 if not marked */
static int marks (WEdit * edit, long *start, long *end)
{E_
    return eval_marks (edit, start, end);
}

int column_highlighting = 0;

static int erange (WEdit * edit, long start, long end, int click)
{E_
    if (column_highlighting) {
	int x;
	x = edit_move_forward3 (edit, edit_bol (edit, click), 0, click);
	if ((x >= edit->column1 && x < edit->column2)
	    || (x > edit->column2 && x <= edit->column1))
	    return (start <= click && click < end);
	else
	    return 0;
    }
    return (start <= click && click < end);
}

static void fin_mark (WEdit * edit)
{E_
    if (edit->mark2 < 0)
	edit_mark_cmd (edit, 0);
}

static void move_mark (WEdit * edit)
{E_
    edit_mark_cmd (edit, 1);
    edit_mark_cmd (edit, 0);
}

static void release_mark (WEdit * edit, XEvent * event)
{E_
    if (edit->mark2 < 0)
	edit_mark_cmd (edit, 0);
    else
	edit_mark_cmd (edit, 1);
    if (edit->mark1 != edit->mark2 && event) {
	edit_get_selection (edit);
	XSetSelectionOwner (CDisplay, XA_PRIMARY, CWindowOf (edit->widget), event->xbutton.time);
	XSetSelectionOwner (CDisplay, ATOM_ICCCM_P2P_CLIPBOARD, CWindowOf (edit->widget), event->xbutton.time);
    }
#ifdef GTK
    else {
	edit->widget->editable.has_selection = TRUE;
    }
#endif
}

static char *get_block (WEdit * edit, long start_mark, long end_mark, int *type, int *l)
{E_
    char *t;
    t = (char *) edit_get_block (edit, start_mark, end_mark, l);
    if (strlen (t) < *l)
	*type = DndRawData;	/* if there are nulls in the data, send as raw */
    else
	*type = DndText;	/* else send as text */
    return t;
}

static void move (WEdit * edit, long click, int y)
{E_
    edit_cursor_move (edit, click - edit->curs1);
}

static void dclick (WEdit * edit, XEvent * event)
{E_
    edit_mark_cmd (edit, 1);
    edit_right_word_move (edit, 1);
    edit_mark_cmd (edit, 0);
    edit_left_word_move (edit, 1);
    release_mark (edit, event);
}

static void redraw (WEdit * edit, long click, int event_type)
{E_
    mouse_redraw (edit, click, event_type);
}

void edit_insert_column_of_text (WEdit * edit, unsigned char *data, int size, int width);

/* strips out the first i chars and returns a null terminated string, result must be free'd */
char *filename_from_url (char *data, int size, int i)
{E_
    char *p, *f;
    int l;
    for (p = data + i; (unsigned long) p - (unsigned long) data < size && *p && *p != '\n'; p++);
    l = (unsigned long) p - (unsigned long) data - i;
    f = malloc (l + 1);
    memcpy (f, data + i, l);
    f[l] = '\0';
    return f;
}

static int insert_drop (WEdit * e, Window from, unsigned char *data, int size, int xs, int ys, Atom type, Atom action)
{E_
    long start_mark = 0, end_mark = 0;
    int x, y;

    edit_translate_xy (xs, ys, &x, &y);
/* musn't be able to drop into a block, otherwise a single click will copy a block: */
    if (eval_marks (e, &start_mark, &end_mark))
	goto fine;
    if (start_mark > e->curs1 || e->curs1 >= end_mark)
	goto fine;
    if (column_highlighting) {
	if (!((x >= e->column1 && x < e->column2)
	      || (x > e->column2 && x <= e->column1)))
	    goto fine;
    }
    return 1;
  fine:
    if (from == e->widget->winid && action == CDndClass->XdndActionMove) {
	edit_block_move_cmd (e);
	edit_mark_cmd (e, 1);
	return 0;
    } else if (from == e->widget->winid) {
	edit_block_copy_cmd (e);
	return 0;
    } else {			/*  data from another widget, or from another application */
	edit_push_action (e, KEY_PRESS, e->start_display);
	if (type == XInternAtom (CDisplay, "url/url", False)) {
	    if (!strncmp ((char *) data, "file:/", 6)) {
		char *f;
		edit_insert_file (e, f = filename_from_url ((char *) data, size, strlen ("file:")));
		free (f);
	    } else {
		while (size--)
		    edit_insert_ahead (e, data[size]);
	    }
	} else {
	    if (column_highlighting) {
		edit_insert_column_of_text (e, data, size, abs (e->column2 - e->column1));
	    } else {
		while (size--)
		    edit_insert_ahead (e, data[size]);
	    }
	}
    }
    CExpose (e->widget->ident);
    return 0;
}

static char *mime_majors[2] =
{"text", 0};

struct mouse_funcs edit_mouse_funcs =
{
    0,
    (void (*)(int, int, int *, int *)) xy,
    (long (*)(void *, int, int)) cp,
    (int (*)(void *, long *, long *)) marks,
    (int (*)(void *, long, long, long)) erange,
    (void (*)(void *)) fin_mark,
    (void (*)(void *)) move_mark,
    (void (*)(void *, XEvent *)) release_mark,
    (char *(*)(void *, long, long, int *, int *)) get_block,
    (void (*)(void *, long, int)) move,
    0,
    (void (*)(void *, XEvent *)) dclick,
    (void (*)(void *, long, int)) redraw,
    (int (*)(void *, Window, unsigned char *, int, int, int, Atom, Atom)) insert_drop,
    (void (*)(void *)) edit_block_delete,
    DndText,
    mime_majors
};

static void render_book_marks (CWidget * w);
extern int option_editor_bg_normal;
void edit_tri_cursor (Window win);

/* starting_directory is for the filebrowser */
CWidget *CDrawEditor (const char *identifier, Window parent, int x, int y,
		      int width, int height, const char *text, const char *filename,
		      const char *starting_host, const char *starting_directory, unsigned int options, unsigned long text_size)
{E_
    static int made_directory = 0;
    int extra_space_for_hscroll = 0;
    int max_x = 0;
    CWidget *w;
    WEdit *e;

    CPushFont ("editor", 0);
    if (options & EDITOR_HORIZ_SCROLL)
	extra_space_for_hscroll = 8;
    wedit = w = CSetupWidget (identifier, parent, x, y,
			      width + EDIT_FRAME_W, height + EDIT_FRAME_H,
			      C_EDITOR_WIDGET, INPUT_KEY, color_palette (option_editor_bg_normal), 1);

    xdnd_set_dnd_aware (CDndClass, w->winid, 0);
    xdnd_set_type_list (CDndClass, w->winid, xdnd_typelist_send[DndText]);

    edit_tri_cursor (w->winid);
    w->options = options | WIDGET_TAKES_SELECTION;

    w->destroy = edit_destroy_callback;
    if (filename)
	w->label = (char *) strdup (filename);
    else
	w->label = (char *) strdup ("");

    if (!made_directory) {
	mkdir (catstrs (local_home_dir, EDIT_DIR, NULL), 0700);
	made_directory = 1;
    }
    e = w->editor = CMalloc (sizeof (WEdit));
    memset (e, '\0', sizeof (*e));
    w->funcs = mouse_funcs_new (w->editor, &edit_mouse_funcs);

    if (!w->editor) {
/* Not essential to translate */
	CError (_("Error initialising editor.\n"));
	CPopFont ();
	return 0;
    }
    w->editor->widget = w;
    w->editor =
	edit_init (e, height / FONT_PIX_PER_LINE, width / FONT_MEAN_WIDTH, filename, text, starting_host, starting_directory,
		   text_size, 1);
    w->funcs->data = (void *) w->editor;
    if (!w->editor) {
	free (e);
	CDestroyWidget (w->ident);
	CPopFont ();
	return 0;
    }
    edit_clear_macro (&e->macro);
    e->widget = w;

    if (!(options & EDITOR_NO_SCROLL)) {
	w->vert_scrollbar = CDrawVerticalScrollbar (catstrs (identifier, ".vsc", NULL), parent,
						    x + width + EDIT_FRAME_W + WIDGET_SPACING, y, height + EDIT_FRAME_H,
						    AUTO_WIDTH, 0, 0);
	CSetScrollbarCallback (w->vert_scrollbar->ident, w->ident, link_scrollbar_to_editor);
	w->vert_scrollbar->scroll_bar_extra_render = render_book_marks;
	CGetHintPos (&max_x, 0);
    }
    set_hint_pos (x + width + EDIT_FRAME_W + WIDGET_SPACING,
		  y + height + EDIT_FRAME_H + WIDGET_SPACING + extra_space_for_hscroll);
    if (extra_space_for_hscroll) {
	w->hori_scrollbar = CDrawHorizontalScrollbar (catstrs (identifier, ".hsc", NULL), parent,
						      x, y + height + EDIT_FRAME_H, width + EDIT_FRAME_W,
						      AUTO_HEIGHT, 0, 0);
	CSetScrollbarCallback (w->hori_scrollbar->ident, w->ident, link_hscrollbar_to_editor);
    }
    CGetHintPos (0, &y);
    if (!(options & EDITOR_NO_TEXT)) {
	CPushFont ("widget", 0);
	CDrawStatus (catstrs (identifier, ".text", NULL), parent, x, y, width + EDIT_FRAME_W, e->filename);
	CPopFont ();
    }
    CGetHintPos (0, &y);
    if (!max_x)
	CGetHintPos (&max_x, 0);
    set_hint_pos (max_x, y);
    CPopFont ();
    return w;
}

#if 0

/* starting_directory is for the filebrowser */
void CRedrawEditor (const char *identifier, Window parent, int x, int y, int width, int height, unsigned int options, unsigned long text_size)
{E_
    int extra_space_for_hscroll = 0;
    int max_x = 0;
    CWidget *w;

    CPushFont ("editor", 0);
    if (options & EDITOR_HORIZ_SCROLL)
	extra_space_for_hscroll = 8;
    w = CRedrawWidget (identifier, x, y, width + EDIT_FRAME_W, height + EDIT_FRAME_H,
			C_EDITOR_WIDGET, color_palette (option_editor_bg_normal), 1);

    w->options = options | WIDGET_TAKES_SELECTION;

    edit_adjust (w->editor, height / FONT_PIX_PER_LINE, width / FONT_MEAN_WIDTH, 1);

    if (!(options & EDITOR_NO_SCROLL)) {
	CRedrawVerticalScrollbar (catstrs (identifier, ".vsc", NULL), x + width + EDIT_FRAME_W + WIDGET_SPACING, y, height + EDIT_FRAME_H, AUTO_WIDTH, 0, 0);
	CGetHintPos (&max_x, 0);
    }
    set_hint_pos (x + width + EDIT_FRAME_W + WIDGET_SPACING,
		  y + height + EDIT_FRAME_H + WIDGET_SPACING + extra_space_for_hscroll);
    if (extra_space_for_hscroll) {
	CRedrawHorizontalScrollbar (catstrs (identifier, ".hsc", NULL), x, y + height + EDIT_FRAME_H,
                                    width + EDIT_FRAME_W, AUTO_HEIGHT, 0, 0);
    }
    CGetHintPos (0, &y);
    if (!(options & EDITOR_NO_TEXT)) {
	CPushFont ("widget", 0);
	CRedrawStatus (catstrs (identifier, ".text", NULL), parent, x, y, width + EDIT_FRAME_W);
	CPopFont ();
    }
    CGetHintPos (0, &y);
    if (!max_x)
	CGetHintPos (&max_x, 0);
    set_hint_pos (max_x, y);
    CPopFont ();
}

#endif

static void render_book_marks (CWidget * w)
{E_
    struct _book_mark *p;
    WEdit *edit;
    int l;
    char i[32];
    if (!w)
	return;
    strcpy (i, CIdentOf (w));
    *(strstr (i, ".vsc")) = '\0';
    edit = (CIdent (i))->editor;
    if (!edit->book_mark)
	return;
    l = CHeightOf (w) - 10 * CWidthOf (w) / 3 - 10;
    for (p = edit->book_mark; p->next; p = p->next);
    for (; p->prev; p = p->prev) {
	int y = (CWidthOf (w) + 2 * CWidthOf (w) / 3 + 4) + (int) ((double) l * p->line / edit->total_lines);
	CSetColor (color_palette (((p->c & 0xFF00) >> 8) ? ((p->c & 0xFF00) >> 8) : (p->c & 0xFF)));
	CLine (CWindowOf (w), 5, y, CWidthOf (w) - 6, y);
    }
}

void update_scroll_bars (WEdit * e)
{E_
    int i, x1, x2;
    CWidget *scroll;
    CPushFont ("editor", 0);
    scroll = e->widget->vert_scrollbar;
    if (scroll) {
	i = e->total_lines - e->start_line + 1;
	if (i > e->num_widget_lines)
	    i = e->num_widget_lines;
	if (e->total_lines) {
	    x1 = (double) 65535.0 *e->start_line / (e->total_lines + 1);
	    x2 = (double) 65535.0 *i / (e->total_lines + 1);
	} else {
	    x1 = 0;
	    x2 = 65535;
	}
	if (x1 != scroll->firstline || x2 != scroll->numlines) {
	    scroll->firstline = x1;
	    scroll->numlines = x2;
	    EditExposeRedraw = 1;
	    render_scrollbar (scroll);
	    EditExposeRedraw = 0;
	}
    }
    scroll = e->widget->hori_scrollbar;
    if (scroll) {
	i = e->max_column - (-e->start_col) + 1;
	if (i > e->num_widget_columns * FONT_MEAN_WIDTH)
	    i = e->num_widget_columns * FONT_MEAN_WIDTH;
	x1 = (double) 65535.0 *(-e->start_col) / (e->max_column + 1);
	x2 = (double) 65535.0 *i / (e->max_column + 1);
	if (x1 != scroll->firstline || x2 != scroll->numlines) {
	    scroll->firstline = x1;
	    scroll->numlines = x2;
	    EditExposeRedraw = 1;
	    render_scrollbar (scroll);
	    EditExposeRedraw = 0;
	}
    }
    CPopFont ();
}

void edit_mouse_mark (WEdit * edit, XEvent * event, int double_click)
{E_
    CPushFont ("editor", 0);
    edit_update_curs_row (edit);
    edit_update_curs_col (edit);
    if (event->type != MotionNotify) {
	edit_push_action (edit, KEY_PRESS, edit->start_display);
	if (edit->mark2 == -1)
	    edit_push_action (edit, MARK_1, edit->mark1);	/* mark1 must be following the cursor */
    }
    if (event->type == ButtonPress) {
	edit->highlight = 0;
	edit->found_len = 0;
    }
    mouse_mark (
		   event,
		   double_click,
		   edit->widget->funcs
	);
    CPopFont ();
}

void link_scrollbar_to_editor (CWidget * scrollbar, CWidget * editor, XEvent * xevent, CEvent * cwevent, int whichscrbutton)
{E_
    int i, start_line;
    WEdit *e;
    e = editor->editor;
    if (!e)
	return;
    if (!e->widget->vert_scrollbar)
	return;
    CPushFont ("editor", 0);
    start_line = e->start_line;
    if ((xevent->type == ButtonRelease || xevent->type == MotionNotify) && whichscrbutton == 3) {
	edit_move_display (e, (double) scrollbar->firstline * e->total_lines / 65535.0 + 1);
    } else if (xevent->type == ButtonPress && (cwevent->button == Button1 || cwevent->button == Button2)) {
	switch (whichscrbutton) {
	case 1:
	    edit_move_display (e, e->start_line - e->num_widget_lines + 1);
	    break;
	case 2:
	    edit_move_display (e, e->start_line - 1);
	    break;
	case 5:
	    edit_move_display (e, e->start_line + 1);
	    break;
	case 4:
	    edit_move_display (e, e->start_line + e->num_widget_lines - 1);
	    break;
	}
    }
    if (e->total_lines)
	scrollbar->firstline = (double) 65535.0 *e->start_line / (e->total_lines + 1);
    else
	scrollbar->firstline = 0;
    i = e->total_lines - e->start_line + 1;
    if (i > e->num_widget_lines)
	i = e->num_widget_lines;
    if (e->total_lines)
	scrollbar->numlines = (double) 65535.0 *i / (e->total_lines + 1);
    else
	scrollbar->numlines = 65535;
    if (start_line != e->start_line) {
	e->force |= REDRAW_PAGE | REDRAW_LINE;
	set_cursor_position (0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	if (CCheckWindowEvent (0, ButtonReleaseMask | ButtonMotionMask, 1)) {
	    CPopFont ();
	    return;
	}
    }
    if (e->force) {
	edit_render_event (e, xevent->type);
	edit_status (e);
    }
    CPopFont ();
}

void link_hscrollbar_to_editor (CWidget * scrollbar, CWidget * editor, XEvent * xevent, CEvent * cwevent, int whichscrbutton)
{E_
    int i, start_col;
    WEdit *e;
    e = editor->editor;
    if (!e)
	return;
    if (!e->widget->hori_scrollbar)
	return;
    CPushFont ("editor", 0);
    start_col = (-e->start_col);
    if ((xevent->type == ButtonRelease || xevent->type == MotionNotify) && whichscrbutton == 3) {
	e->start_col = (double) scrollbar->firstline * e->max_column / 65535.0 + 1;
	e->start_col -= e->start_col % FONT_MEAN_WIDTH;
	if (e->start_col < 0)
	    e->start_col = 0;
	e->start_col = (-e->start_col);
    } else if (xevent->type == ButtonPress && (cwevent->button == Button1 || cwevent->button == Button2)) {
	switch (whichscrbutton) {
	case 1:
	    edit_scroll_left (e, (e->num_widget_columns - 1) * FONT_MEAN_WIDTH);
	    break;
	case 2:
	    edit_scroll_left (e, FONT_MEAN_WIDTH);
	    break;
	case 5:
	    edit_scroll_right (e, FONT_MEAN_WIDTH);
	    break;
	case 4:
	    edit_scroll_right (e, (e->num_widget_columns - 1) * FONT_MEAN_WIDTH);
	    break;
	}
    }
    scrollbar->firstline = (double) 65535.0 *(-e->start_col) / (e->max_column + 1);
    i = e->max_column - (-e->start_col) + 1;
    if (i > e->num_widget_columns * FONT_MEAN_WIDTH)
	i = e->num_widget_columns * FONT_MEAN_WIDTH;
    scrollbar->numlines = (double) 65535.0 *i / (e->max_column + 1);
    if (start_col != (-e->start_col)) {
	e->force |= REDRAW_PAGE | REDRAW_LINE;
	set_cursor_position (0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	if (CCheckWindowEvent (0, ButtonReleaseMask | ButtonMotionMask, 1)) {
	    CPopFont ();
	    return;
	}
    }
    if (e->force) {
	edit_render_event (e, xevent->type);
	edit_status (e);
    }
    CPopFont ();
}

/* 
   This section comes from rxvt-2.21b1/src/screen.c by
   Robert Nation <nation@rocket.sanders.lockheed.com> &
   mods by mj olesen <olesen@me.QueensU.CA>

   Changes made for cooledit
 */
void selection_send (XSelectionRequestEvent * rq)
{E_
    XEvent ev;
    static Atom xa_targets = None;
    if (xa_targets == None)
	xa_targets = XInternAtom (CDisplay, "TARGETS", False);

    ev.xselection.type = SelectionNotify;
    ev.xselection.property = None;
    ev.xselection.display = rq->display;
    ev.xselection.requestor = rq->requestor;
    ev.xselection.selection = rq->selection;
    ev.xselection.target = rq->target;
    ev.xselection.time = rq->time;

    if (rq->target == xa_targets) {
	/*
	 * On some systems, the Atom typedef is 64 bits wide.
	 * We need to have a typedef that is exactly 32 bits wide,
	 * because a format of 64 is not allowed by the X11 protocol.
	 *
	 * XXX: yes, but Xlib requires that you pass it 64 bits for 32bit
	 * quantities on 64 bit archs.
	 */
	/* typedef CARD32 Atom32; */

	Atom target_list[3];

	target_list[0] = xa_targets;
	target_list[1] = XA_STRING;
	target_list[2] = ATOM_UTF8_STRING;

	XChangeProperty (CDisplay, rq->requestor, rq->property,
		xa_targets, 32, PropModeReplace,
			 (unsigned char *) target_list,
			 sizeof (target_list) / sizeof (target_list[0]));
	ev.xselection.property = rq->property;
    } else if (rq->target == XA_STRING) {
	XChangeProperty (CDisplay, rq->requestor, rq->property,
			 XA_STRING, 8, PropModeReplace,
			 (unsigned char *) edit_selection.data, edit_selection.len);
	ev.xselection.property = rq->property;
    } else if (rq->target == ATOM_UTF8_STRING) {
	XChangeProperty (CDisplay, rq->requestor, rq->property,
			 ATOM_UTF8_STRING, 8, PropModeReplace,
			 (unsigned char *) edit_selection.data, edit_selection.len);
	ev.xselection.property = rq->property;
    }
    XSendEvent (CDisplay, rq->requestor, False, 0, &ev);
}

/*{{{ paste selection */

/* repeated in xdnd.c and rxvt */
static int paste_prop_internal (void *data, void (*insert) (void *, int), Window win,
				unsigned long prop, int delete_prop)
{E_
    long nread = 0;
    unsigned long nitems;
    unsigned long bytes_after;
    do {
	Atom actual_type;
	int actual_fmt, i;
	unsigned char *s = 0;
	if (XGetWindowProperty (CDisplay, win, prop,
				nread / 4, 65536, delete_prop,
				AnyPropertyType, &actual_type, &actual_fmt,
				&nitems, &bytes_after, &s) != Success) {
	    XFree (s);
	    return 1;
	}
	nread += nitems;
	for (i = 0; i < nitems; i++)
	    (*insert) (data, s[i]);
	XFree (s);
    } while (bytes_after);
    if (!nread)
	return 1;
    return 0;
}


/*
 * make multiple attempts to get the selection starting first with UTF-8
 */
static void paste_convert_selection_ (Window w, int start)
{E_
    static Atom convertions[10];
    static int n_convertions = 0;
    static int i = 0;
    static Window win;
    if (start) {
	i = 0;
	win = w;
        n_convertions = 0;
        if (get_editor_encoding () == FONT_ENCODING_UTF8) {
	    convertions[n_convertions++] = ATOM_UTF8_STRING;
	    convertions[n_convertions++] = XA_STRING;
        } else {
	    convertions[n_convertions++] = XA_STRING;
	    convertions[n_convertions++] = ATOM_UTF8_STRING;
        }
    } else {
        if (i >= n_convertions)
            return;
    }
    XConvertSelection (CDisplay, XA_PRIMARY, convertions[i], XInternAtom (CDisplay, "VT_SELECTION", False),
		       win, CurrentTime);
    i++;
}

void paste_convert_selection(Window w)
{E_
    paste_convert_selection_(w, 1);
}

/*
 * Respond to a notification that a primary selection has been sent
 */
void paste_prop (void *data, void (*insert) (void *, int), Window win, unsigned long prop,
		 int delete_prop)
{E_
    struct timeval tv, tv_start;
    unsigned long bytes_after;
    Atom actual_type;
    int actual_fmt;
    unsigned long nitems;
    unsigned char *s = 0;
    if (prop == None) {
        paste_convert_selection_(0, 0);
	return;
    }
    if (XGetWindowProperty
	(CDisplay, win, prop, 0, 8, False, AnyPropertyType, &actual_type, &actual_fmt, &nitems,
	 &bytes_after, &s) != Success) {
	XFree (s);
	return;
    }
    XFree (s);
    if (actual_type != XInternAtom (CDisplay, "INCR", False)) {
	paste_prop_internal (data, insert, win, prop, delete_prop);
	return;
    }
    XDeleteProperty (CDisplay, win, prop);
    gettimeofday (&tv_start, 0);
    for (;;) {
	long t;
	fd_set r;
	XEvent xe;
	if (XCheckMaskEvent (CDisplay, PropertyChangeMask, &xe)) {
	    if (xe.type == PropertyNotify && xe.xproperty.state == PropertyNewValue) {
/* time between arrivals of data */
		gettimeofday (&tv_start, 0);
		if (paste_prop_internal (data, insert, win, prop, True))
		    break;
	    }
	} else {
	    tv.tv_sec = 0;
	    tv.tv_usec = 10000;
	    FD_ZERO (&r);
	    FD_SET (ConnectionNumber (CDisplay), &r);
	    select (ConnectionNumber (CDisplay) + 1, &r, 0, 0, &tv);
	    if (FD_ISSET (ConnectionNumber (CDisplay), &r))
		continue;
	}
	gettimeofday (&tv, 0);
	t = (tv.tv_sec - tv_start.tv_sec) * 1000000L + (tv.tv_usec - tv_start.tv_usec);
/* no data for three seconds, so quit */
	if (t > 3000000L)
	    break;
    }
}

void selection_paste (WEdit * edit, Window win, unsigned prop, int delete_prop)
{E_
    long c;
    c = edit->curs1;
    paste_prop ((void *) edit,
		(void (*)(void *, int)) edit_insert,
		win, prop, delete_prop);
    edit_cursor_move (edit, c - edit->curs1);
    edit->force |= REDRAW_COMPLETELY | REDRAW_LINE;
}

/*}}} */

void (*user_selection_clear) (void) = 0;

void selection_replace (CStr new_selection)
{E_
    CStr_free(&edit_selection);
    edit_selection = CStr_cpy(new_selection.data, new_selection.len);
    if (user_selection_clear)
	(*user_selection_clear) ();
}

void selection_clear (void)
{E_
    CStr_free(&edit_selection);
    if (user_selection_clear)
	(*user_selection_clear) ();
}

static void edit_update_screen_ (WEdit * e, int event_type)
{E_
    if (!e)
	return;
    if (!e->force)
	return;

    CPushFont ("editor", 0);
    edit_scroll_screen_over_cursor (e);
    edit_update_curs_row (e);
    edit_update_curs_col (e);
    update_scroll_bars (e);
    edit_status (e);

    if (e->force & REDRAW_COMPLETELY)
	e->force |= REDRAW_PAGE;

/* pop all events for this window for internal handling */
    if (e->force & (REDRAW_CHAR_ONLY | REDRAW_COMPLETELY)) {
	edit_render_event (e, event_type);
    } else if (CCheckSimilarEventsPending (0, event_type, 1)) {
	e->force |= REDRAW_PAGE;
	CPopFont ();
	return;
    } else {
	edit_render_event (e, event_type);
    }
    CPopFont ();
}

void edit_update_screen (WEdit * e)
{E_
    edit_update_screen_ (e, KeyPress | KeyRelease);
}

static int edit_is_command_that_doesnt_modify_the_file (int command)
{
    switch (command) {
    case CK_Save:
    case CK_Exit:
    case CK_Load:
    case CK_Menu:
    case CK_Close_Last:
    case CK_Maximize:
    case CK_Terminal:
    case CK_Debug_Start:
    case CK_Debug_Stop:
    case CK_Debug_Toggle_Break:
    case CK_Debug_Clear:
    case CK_Debug_Next:
    case CK_Debug_Step:
    case CK_Debug_Back_Trace:
    case CK_Debug_Continue:
    case CK_Debug_Enter_Command:
    case CK_Debug_Until_Curser:
        return 1;
    }
    return 0;
}

int eh_editor (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    WEdit *e = w->editor;
    int r = 0;
    static int old_tab_spacing = -1;
    long ks, macro_command;

    if (!e)
	return 0;

    if (old_tab_spacing != option_tab_spacing)
	e->force |= REDRAW_COMPLETELY + REDRAW_LINE;
    old_tab_spacing = option_tab_spacing;

    if (xevent->type == KeyPress) {
	if (xevent->xkey.keycode == 0x31 && xevent->xkey.state == 0xD) {
	    CSetColor (color_palette (18));
	    CRectangle (w->winid, 0, 0, w->width, w->height);
	}
    }
    switch (xevent->type) {
    case SelectionNotify:
	selection_paste (e, xevent->xselection.requestor, xevent->xselection.property, True);
	r = 1;
	break;
    case SelectionRequest:
	selection_send (&(xevent->xselectionrequest));
	return 1;
/*  case SelectionClear:   ---> This is handled by coolnext.c: CNextEvent() */
	break;
    case ButtonPress:
	CFocus (w);
	edit_render_tidbits (w);
    case ButtonRelease:
	if (xevent->xbutton.state & ControlMask) {
	    if (!column_highlighting)
		edit_push_action (e, COLUMN_OFF, 0);
	    column_highlighting = 1;
	} else {
	    if (column_highlighting)
		edit_push_action (e, COLUMN_ON, 0);
	    column_highlighting = 0;
	}
    case MotionNotify:
	if (xevent->type == MotionNotify && !(xevent->xmotion.state & Button12345Mask))
	    return 0;
	resolve_button (xevent, cwevent);
	if (cwevent->button == Button4 || cwevent->button == Button5) {
	    if (xevent->type == ButtonPress)
                return 0;
	    if (xevent->type == ButtonRelease) {
	        /* ahaack: wheel mouse mapped as button 4 and 5 */
                if ((xevent->xbutton.state & ShiftMask))
	            r = edit_execute_key_command (e, (cwevent->button == Button5) ? CK_Scroll_Down : CK_Scroll_Up, CStr_const ("", 0));
                else
	            r = edit_execute_key_command (e, (cwevent->button == Button5) ? CK_Quarter_Page_Down : CK_Quarter_Page_Up, CStr_const ("", 0));
                e->force |= REDRAW_PAGE;
	        break;
            }
	}
	edit_mouse_mark (e, xevent, cwevent->double_click);
	break;
    case Expose:
	edit_render_expose (e, &(xevent->xexpose));
	return 1;
    case FocusIn:
	CSetCursorColor (e->overwrite ? color_palette (24) : color_palette (19));
        e->test_file_on_disk_for_changes = 1;
        /* fall through */
    case FocusOut:
	edit_render_tidbits (w);
	e->force |= REDRAW_CHAR_ONLY | REDRAW_LINE;
	edit_render_keypress (e);
	return 1;
	break;
    case KeyRelease:
#if 0
	if (column_highlighting) {
	    column_highlighting = 0;
	    e->force = REDRAW_COMPLETELY | REDRAW_LINE;
	    edit_mark_cmd (e, 1);
	}
#endif
        return 0;       /* creates unnecessary refreshes. especially with Shift and wheel mouse */
    case EditorCommand:
	cwevent->ident = w->ident;
	cwevent->command = xevent->xkey.keycode;
        cwevent->xlat_len = 0;
    case KeyPress:
	cwevent->ident = w->ident;
        if (e->test_file_on_disk_for_changes && (cwevent->command > 0 || cwevent->xlat_len > 0)) {
            if (!edit_is_command_that_doesnt_modify_the_file (cwevent->command)) {
                if (!edit_is_movement_command(cwevent->command) && !CCheckGlobalHotKey (xevent, cwevent, 0)) {
                    e->test_file_on_disk_for_changes = 0;       /* simple state machine to check on KeyPress after Focus */
                    if (cwevent->command > 0) {
                        if (edit_check_change_on_disk(w->editor, EDIT_CHANGE_ON_DISK__ON_COMMAND)) {
                            r = 1;
                            break;
                        }
                    } else {    /* it looks stupid to have 3 options for a key press, so use a dialog with a two option Open/Ignore */
                        if (edit_check_change_on_disk(w->editor, EDIT_CHANGE_ON_DISK__ON_KEYPRESS)) {
                            r = 1;
                            break;
                        }
                    }
                }
            }
	}
	if (cwevent->command == CK_Toggle_Macro) {
	    cwevent->command = e->macro.macro_i < 0 ? CK_Begin_Record_Macro : CK_End_Record_Macro;
	} else if ((ks = CKeySymMod (xevent)) > 0) {
	    macro_command = CK_Macro (ks);
	    if (macro_command > 0 && edit_check_macro_exists (e, ks)) {
		cwevent->command = macro_command;
		cwevent->xlat_len = 0;
	    }
	}
        if (edit_translate_key_exit_keycompose ()) {
            e->force |= REDRAW_CHAR_ONLY;
        } else if (edit_translate_key_in_key_compose ()) {
            e->force |= REDRAW_CHAR_ONLY;
	    edit_update_screen (e);
            return 0;
        }
	if (cwevent->command <= 0 && !cwevent->xlat_len) {
            if (cwevent->key == XK_Shift_L || cwevent->key == XK_Shift_R)
                return 0;       /* creates unnecessary refreshes. especially with Shift and wheel mouse */
	    break;
        }
	if (cwevent->command <= 0 && ((cwevent->state & MyAltMask) || (cwevent->state & ControlMask)))
	    if ((r = CHandleGlobalKeys (w, xevent, cwevent)))
		break;
	r = edit_execute_key_command (e, cwevent->command, CStr_const (cwevent->xlat, cwevent->xlat_len));
	if (r)
	    edit_update_screen (e);
	return r;
	break;
    default:
	return 0;
    }
    edit_update_screen_ (e, xevent->type);
    return r;
}

#else

WEdit *wedit;
WButtonBar *edit_bar;
Dlg_head *edit_dlg;
WMenu *edit_menubar;

int column_highlighting = 0;

static int edit_callback (Dlg_head * h, WEdit * edit, int msg, int par);

static int edit_mode_callback (struct Dlg_head *h, int id, int msg)
{E_
    return 0;
}

int edit_event (WEdit * edit, Gpm_Event * event, int *result)
{E_
    *result = MOU_NORMAL;
    edit_update_curs_row (edit);
    edit_update_curs_col (edit);
    if (event->type & (GPM_DOWN | GPM_DRAG | GPM_UP)) {
	if (event->y > 1 && event->x > 0
	    && event->x <= edit->num_widget_columns
	    && event->y <= edit->num_widget_lines + 1) {
	    if (edit->mark2 != -1 && event->type & (GPM_UP | GPM_DRAG))
		return 1;	/* a lone up mustn't do anything */
	    if (event->type & (GPM_DOWN | GPM_UP))
		edit_push_key_press (edit);
	    edit_cursor_move (edit, edit_bol (edit, edit->curs1) - edit->curs1);
	    if (--event->y > (edit->curs_row + 1))
		edit_cursor_move (edit,
				  edit_move_forward (edit, edit->curs1, event->y - (edit->curs_row + 1), 0)
				  - edit->curs1);
	    if (event->y < (edit->curs_row + 1))
		edit_cursor_move (edit,
				  +edit_move_backward (edit, edit->curs1, (edit->curs_row + 1) - event->y)
				  - edit->curs1);
	    edit_cursor_move (edit, (int) edit_move_forward3 (edit, edit->curs1,
		       event->x - edit->start_col - 1, 0) - edit->curs1);
	    edit->prev_col = edit_get_col (edit);
	    if (event->type & GPM_DOWN) {
		edit_mark_cmd (edit, 1);	/* reset */
		edit->highlight = 0;
	    }
	    if (!(event->type & GPM_DRAG))
		edit_mark_cmd (edit, 0);
	    edit->force |= REDRAW_COMPLETELY;
	    edit_update_curs_row (edit);
	    edit_update_curs_col (edit);
	    edit_update_screen (edit);
	    return 1;
	}
    }
    return 0;
}



int menubar_event (Gpm_Event * event, WMenu * menubar);		/* menu.c */

int edit_mouse_event (Gpm_Event * event, void *x)
{E_
    int result;
    if (edit_event ((WEdit *) x, event, &result))
	return result;
    else
	return menubar_event (event, edit_menubar);
}

extern Menu EditMenuBar[5];

int edit (const char *_file, int line)
{E_
    static int made_directory = 0;
    int framed = 0;
    int midnight_colors[4];
    char *text = 0;

    if (option_backup_ext_int != -1) {
	option_backup_ext = malloc (sizeof (int) + 1);
	option_backup_ext[sizeof (int)] = '\0';
	memcpy (option_backup_ext, (char *) &option_backup_ext_int, sizeof (int));
    }
    if (!made_directory) {
	mkdir (catstrs (home_dir, EDIT_DIR, NULL), 0700);
	made_directory = 1;
    }
    if (_file) {
	if (!(*_file)) {
	    _file = 0;
	    text = "";
	}
    } else
	text = "";

    if (!(wedit = edit_init (NULL, LINES - 2, COLS, _file, text, "", 0))) {
	message (1, _ (" Error "), get_error_msg (""));
	return 0;
    }
    wedit->x_macro_i = -1;

    /* Create a new dialog and add it widgets to it */
    edit_dlg = create_dlg (0, 0, LINES, COLS, midnight_colors,
			   edit_mode_callback, "[Internal File Editor]",
			   "edit",
			   DLG_NONE);

    edit_dlg->raw = 1;		/*so that tab = '\t' key works */

    init_widget (&(wedit->widget), 0, 0, LINES - 1, COLS,
		 (callback_fn) edit_callback,
		 (destroy_fn) edit_clean,
		 (mouse_h) edit_mouse_event, 0);

    widget_want_cursor (wedit->widget, 1);

    edit_bar = buttonbar_new (1);

    if (!framed) {
	switch (edit_key_emulation) {
	case EDIT_KEY_EMULATION_NORMAL:
	    edit_init_menu_normal ();	/* editmenu.c */
	    break;
	case EDIT_KEY_EMULATION_EMACS:
	    edit_init_menu_emacs ();	/* editmenu.c */
	    break;
	}
	edit_menubar = menubar_new (0, 0, COLS, EditMenuBar, N_menus);
    }
    add_widget (edit_dlg, wedit);

    if (!framed)
	add_widget (edit_dlg, edit_menubar);

    add_widget (edit_dlg, edit_bar);
    edit_move_display (wedit, line - 1);
    edit_move_to_line (wedit, line - 1);

    run_dlg (edit_dlg);

    if (!framed)
	edit_done_menu ();	/* editmenu.c */

    destroy_dlg (edit_dlg);

    return 1;
}

static void edit_my_define (Dlg_head * h, int idx, char *text,
			    void (*fn) (WEdit *), WEdit * edit)
{E_
    define_label_data (h, (Widget *) edit, idx, text, (buttonbarfn) fn, edit);
}


void cmd_F1 (WEdit * edit)
{E_
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (1));
}

void cmd_F2 (WEdit * edit)
{E_
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (2));
}

void cmd_F3 (WEdit * edit)
{E_
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (3));
}

void cmd_F4 (WEdit * edit)
{E_
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (4));
}

void cmd_F5 (WEdit * edit)
{E_
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (5));
}

void cmd_F6 (WEdit * edit)
{E_
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (6));
}

void cmd_F7 (WEdit * edit)
{E_
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (7));
}

void cmd_F8 (WEdit * edit)
{E_
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (8));
}

void cmd_F9 (WEdit * edit)
{E_
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (9));
}

void cmd_F10 (WEdit * edit)
{E_
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (10));
}

void edit_labels (WEdit * edit)
{E_
    Dlg_head *h = edit->widget.parent;

    edit_my_define (h, 1, _ ("Help"), cmd_F1, edit);
    edit_my_define (h, 2, _ ("Save"), cmd_F2, edit);
    edit_my_define (h, 3, _ ("Mark"), cmd_F3, edit);
    edit_my_define (h, 4, _ ("Replac"), cmd_F4, edit);
    edit_my_define (h, 5, _ ("Copy"), cmd_F5, edit);
    edit_my_define (h, 6, _ ("Move"), cmd_F6, edit);
    edit_my_define (h, 7, _ ("Search"), cmd_F7, edit);
    edit_my_define (h, 8, _ ("Delete"), cmd_F8, edit);
    if (!edit->have_frame)
	edit_my_define (h, 9, _ ("PullDn"), edit_menu_cmd, edit);
    edit_my_define (h, 10, _ ("Quit"), cmd_F10, edit);

    redraw_labels (h, (Widget *) edit);
}


long get_key_state ()
{E_
    return (long) get_modifier ();
}

void edit_adjust_size (Dlg_head * h)
{E_
    WEdit *edit;
    WButtonBar *edit_bar;

    edit = (WEdit *) find_widget_type (h, (callback_fn) edit_callback);
    edit_bar = (WButtonBar *) edit->widget.parent->current->next->widget;
    widget_set_size (&edit->widget, 0, 0, LINES - 1, COLS);
    widget_set_size (&edit_bar->widget, LINES - 1, 0, 1, COLS);
    widget_set_size (&edit_menubar->widget, 0, 0, 1, COLS);

#ifdef RESIZABLE_MENUBAR
    menubar_arrange (edit_menubar);
#endif
}

void edit_update_screen (WEdit * e)
{E_
    edit_scroll_screen_over_cursor (e);

    edit_update_curs_col (e);
    edit_status (e);

/* pop all events for this window for internal handling */

    if (!is_idle ()) {
	e->force |= REDRAW_PAGE;
	return;
    }
    if (e->force & REDRAW_COMPLETELY)
	e->force |= REDRAW_PAGE;
    edit_render_keypress (e);
}

static int edit_callback (Dlg_head * h, WEdit * e, int msg, int par)
{E_
    switch (msg) {
    case WIDGET_INIT:
	e->force |= REDRAW_COMPLETELY;
	edit_labels (e);
	break;
    case WIDGET_DRAW:
	e->force |= REDRAW_COMPLETELY;
	e->num_widget_lines = LINES - 2;
	e->num_widget_columns = COLS;
    case WIDGET_FOCUS:
	edit_update_screen (e);
	return 1;
    case WIDGET_KEY:{
	    int cmd, ch;
	    if (edit_drop_hotkey_menu (e, par))		/* first check alt-f, alt-e, alt-s, etc for drop menus */
		return 1;
	    if (!edit_translate_key (e, 0, par, get_key_state (), &cmd, &ch))
		return 0;
	    edit_execute_key_command (e, cmd, ch);
	    edit_update_screen (e);
	}
	return 1;
    case WIDGET_COMMAND:
	edit_execute_key_command (e, par, -1);
	edit_update_screen (e);
	return 1;
    case WIDGET_CURSOR:
	widget_move (&e->widget, e->curs_row + EDIT_TEXT_VERTICAL_OFFSET, e->curs_col + e->start_col);
	return 1;
    }
    return default_proc (h, msg, par);
}

#endif
