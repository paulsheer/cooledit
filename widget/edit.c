/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* edit.c
   Copyright (C) 1996-2022 Paul Sheer
 */


#define _EDIT_C THIS_IS

#include "inspect.h"
#include <config.h>
#if defined(NEEDS_IO_H)
#    include <io.h>
#    include <fcntl.h>
#endif
#include "edit.h"
#include "stringtools.h"
#include "dirtools.h"
#include "remotefs.h"

#ifdef SCO_FLAVOR
#	include <sys/timeb.h>
#endif /* SCO_FLAVOR */
#include <time.h>	/* for ctime() */

/*
 *
 * here's a quick sketch of the layout: (don't run this through indent.)
 * 
 * (b1 is buffers1 and b2 is buffers2)
 * 
 *                                       |
 * \0\0\0\0\0m e _ f i l e . \nf i n . \n|T h i s _ i s _ s o\0\0\0\0\0\0\0\0\0
 * ______________________________________|______________________________________
 *                                       |
 * ...  |  b2[2]   |  b2[1]   |  b2[0]   |  b1[0]   |  b1[1]   |  b1[2]   | ...
 *      |->        |->        |->        |->        |->        |->        |
 *                                       |
 *           _<------------------------->|<----------------->_
 *                   WEdit->curs2        |   WEdit->curs1
 *           ^                           |                   ^
 *           |                          ^|^                  |
 *         cursor                       |||                cursor
 *                                      |||
 *                              file end|||file beginning
 *                                       |
 *                                       |
 * 
 *           _
 * This_is_some_file
 * fin.
 *
 *
 */

/*
   returns a byte from any location in the file.
   Returns '\n' if out of bounds.
 */

static int push_action_disabled = 0;

#ifdef NO_INLINE_GETBYTE

int edit_get_byte (WEdit * edit, long byte_index)
{E_
    unsigned long p;
    if (byte_index >= (edit->curs1 + edit->curs2) || byte_index < 0)
	return '\n';

    if (byte_index >= edit->curs1) {
	p = edit->curs1 + edit->curs2 - byte_index - 1;
	return edit->buffers2[p >> S_EDIT_BUF_SIZE][EDIT_BUF_SIZE - (p & M_EDIT_BUF_SIZE) - 1];
    } else {
	return edit->buffers1[byte_index >> S_EDIT_BUF_SIZE][byte_index & M_EDIT_BUF_SIZE];
    }
}

#endif

char *edit_get_buffer_as_text (WEdit * e)
{E_
    int l, i;
    char *t;
    l = e->curs1 + e->curs2;
    t = CMalloc (l + 1);
    for (i = 0; i < l; i++)
	t[i] = edit_get_byte (e, i);
    t[l] = 0;
    return t;
}

/* returns a buffer containing the current line and the offset of the
cursor from the left margin in bytes: */
char *edit_get_current_line_as_text (WEdit * e, long *length, long *cursor)
{E_
    long start, end;
    char *r, *p;
    start = edit_bol (e, e->curs1);
    end = edit_eol (e, e->curs1);
    if (length)
	*length = end - start;
    if (cursor)
	*cursor = e->curs1 - start;
    p = r = (char *) CMalloc (end - start + 1);
    while (start < end)
	*p++ = edit_get_byte (e, start++);
    *p = '\0';
    return r;
}


/* Note on CRLF->LF translation: */

#if MY_O_TEXT
#error MY_O_TEXT is depreciated.
#endif

/* 
   The edit_open_file (previously edit_load_file) function uses
   init_dynamic_edit_buffers to load a file. This is unnecessary
   since you can just as well fopen the file and insert the
   characters one by one. The real reason for
   init_dynamic_edit_buffers (besides allocating the buffers) is
   as an optimisation - it uses raw block reads and inserts large
   chunks at a time. It is hence extremely fast at loading files.
   Where we might not want to use it is if we were doing
   CRLF->LF translation or if we were reading from a pipe.
 */

/* Initialisation routines */

static int init_dynamic_edit_buffers_text (WEdit * edit, const char *host, const char *text)
{E_
    long buf;
    int buf2;

    edit->curs2 = edit->last_byte;

    buf2 = edit->curs2 >> S_EDIT_BUF_SIZE;

    edit->buffers2[buf2] = CMalloc (EDIT_BUF_SIZE);

    memcpy (edit->buffers2[buf2] + EDIT_BUF_SIZE - (edit->curs2 & M_EDIT_BUF_SIZE), text, edit->curs2 & M_EDIT_BUF_SIZE);
    text += edit->curs2 & M_EDIT_BUF_SIZE;

    for (buf = buf2 - 1; buf >= 0; buf--) {
        edit->buffers2[buf] = CMalloc (EDIT_BUF_SIZE);
        memcpy (edit->buffers2[buf], text, EDIT_BUF_SIZE);
        text += EDIT_BUF_SIZE;
    }

    edit->curs1 = 0;
    return 0;
}


struct loader_data {
    WEdit *edit;
    long total;
    long chunklen;
    long buf;
    long buf2;
    unsigned char *p;
    int done;
};

static int edit_sock_reader (struct action_callbacks *o, const unsigned char *buf, int buflen, long long filelen, char *errmsg)
{E_
    struct loader_data *ld;

    ld = (struct loader_data *) o->hook;

    if (ld->done) {
        strcpy (errmsg, "File size changed while loading");
        return -1;
    }

    while (buflen > 0) {
        if ((ld->buf == ld->buf2 && ld->chunklen == (ld->edit->curs2 & M_EDIT_BUF_SIZE)) ||
            (ld->buf < ld->buf2 && ld->chunklen == EDIT_BUF_SIZE)) {
            if (!ld->buf) {
                ld->done = 1;
                if (buflen) {
                    strcpy (errmsg, "File size changed while loading");
                    return -1;
                }
                break;
            }
            ld->buf--;
            ld->p = ld->edit->buffers2[ld->buf] = CMalloc (EDIT_BUF_SIZE);
            ld->chunklen = 0;
        }
        *ld->p++ = *buf++;
        ld->total++;
        buflen--;
        ld->chunklen++;
    }

    return 0;
}

static int init_dynamic_edit_buffers_file (WEdit * edit, const char *host, const char *filename)
{E_
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    struct loader_data ld;
    struct action_callbacks o;
    struct remotefs *u;

    memset (&ld, '\0', sizeof (ld));
    memset (&o, '\0', sizeof (o));

    edit->curs2 = edit->last_byte;

    ld.edit = edit;
    ld.buf2 = edit->curs2 >> S_EDIT_BUF_SIZE;
    ld.buf = ld.buf2;

    edit->buffers2[ld.buf] = CMalloc (EDIT_BUF_SIZE);

    ld.p = edit->buffers2[ld.buf2] + EDIT_BUF_SIZE - (edit->curs2 & M_EDIT_BUF_SIZE);

    o.hook = (void *) &ld;
    o.sock_reader = edit_sock_reader;

    u = remotefs_lookup (host, NULL);
    if ((*u->remotefs_readfile) (u, &o, filename, errmsg)) {
        edit_error_dialog (_(" Error "), catstrs (_(" Failed trying to open file for reading: "), filename, " \n [", errmsg, "]", NULL));
        return 1;
    }

    if (ld.total != edit->last_byte) {
        edit_error_dialog (_(" Error "), catstrs (_(" Failed trying to open file for reading: "), filename, " \n [", "File size changed while loading", "]", NULL));
        return 1;
    }

    edit->curs1 = 0;
    return 0;
}

static int init_dynamic_edit_buffers (WEdit * edit, const char *host, const char *filename, const char *text)
{E_
    int j;

    for (j = 0; j <= MAXBUFF; j++) {
        edit->buffers1[j] = NULL;
        edit->buffers2[j] = NULL;
    }

    if (filename)
        return init_dynamic_edit_buffers_file (edit, host, filename);
    return init_dynamic_edit_buffers_text (edit, host, text);
}

/* detecting an error on save is easy: just check if every byte has been written. */
/* detecting an error on read, is not so easy 'cos there is not way to tell
   whether you read everything or not. */
/* FIXME: add proper `triple_pipe_open' to read, write and check errors. */
static struct edit_filters {
    char *read, *write, *extension;
} all_filters[] = {

    {
	"bzip2 -cd %s", "bzip2 > %s", ".bz2"
    },
    {
	"gzip -cd %s", "gzip > %s", ".gz"
    },
    {
	"compress -cd %s", "compress > %s", ".Z"
    }
};

static int edit_find_filter (const char *filename)
{E_
    int i, l;
    if (!filename)
	return -1;
    l = strlen (filename);
    for (i = 0; i < sizeof (all_filters) / sizeof (struct edit_filters); i++) {
	int e;
	e = strlen (all_filters[i].extension);
	if (l > e)
	    if (!strcmp (all_filters[i].extension, filename + l - e))
		return i;
    }
    return -1;
}

char *edit_get_filter (const char *filename)
{E_
    int i, l;
    char *p;
    i = edit_find_filter (filename);
    if (i < 0)
	return 0;
    l = strlen (filename);
    p = malloc (strlen (all_filters[i].read) + l + 2);
    sprintf (p, all_filters[i].read, filename);
    return p;
}

char *edit_get_write_filter (char *writename, const char *filename)
{E_
    int i, l;
    char *p;
    i = edit_find_filter (filename);
    if (i < 0)
	return 0;
    l = strlen (writename);
    p = malloc (strlen (all_filters[i].write) + l + 2);
    sprintf (p, all_filters[i].write, writename);
    return p;
}


long edit_insert_stream (WEdit * edit, int fd, const pid_t *the_pid)
{E_
    int len;
    long total = 0;
    for (;;) {
	char *p, *q;
	len = 8192;
	q = p = read_pipe (fd, &len, the_pid);
	if (!len) {
	    free (p);
	    return total;
	}
	total += len;
	while (len--)
	    edit_insert (edit, *p++);
	free (q);
    }
    return total;
}

long edit_write_stream (WEdit * edit, FILE * f)
{E_
    long i;
    for (i = 0; i < edit->last_byte; i++) {
	int r;
	while ((r = fputc (edit_get_byte (edit, i), f)) == -1 && errno == EINTR);
	if (r < 0)
	    break;
    }
    return i;
}

#define TEMP_BUF_LEN 1024

/* inserts a file at the cursor, returns 1 on success */
int edit_insert_file (WEdit * edit, const char *filename)
{E_
    char *p;
    if ((p = edit_get_filter (filename))) {
        pid_t the_pid;
	long current = edit->curs1;
	int f, g;
	char *a[8];
	a[0] = "/bin/sh";
	a[1] = "-c";
	a[2] = p;
	a[3] = 0;
	if ((the_pid = triple_pipe_open (0, &f, &g, 0, "sh", a)) > 0) {
	    edit_insert_stream (edit, f, &the_pid);
	    edit_cursor_move (edit, current - edit->curs1);
	    free (p);
	    p = read_pipe (g, 0, &the_pid);
	    if (strlen (p)) {
		edit_error_dialog (_ (" Error "), catstrs (_ (" Error reading from pipe: "), p, " ", NULL));
		free (p);
		close (f);
		close (g);
		return 0;
	    }
	    close (f);
	    close (g);
	} else {
	    edit_error_dialog (_ (" Error "), get_sys_error (catstrs (_ (" Failed trying to open pipe for reading: "), p, " ", NULL)));
	    free (p);
	    return 0;
	}
	free (p);
    } else {
	int i, file, blocklen;
	long current = edit->curs1;
	unsigned char *buf;
	if ((file = open ((char *) filename, O_RDONLY)) == -1)
	    return 0;
	buf = malloc (TEMP_BUF_LEN);
	while ((blocklen = read (file, (char *) buf, TEMP_BUF_LEN)) > 0) {
	    for (i = 0; i < blocklen; i++)
		edit_insert (edit, buf[i]);
	}
	edit_cursor_move (edit, current - edit->curs1);
	free (buf);
	close (file);
	if (blocklen)
	    return 0;
    }
    return 1;
}

static int check_file_access (WEdit *edit, const char *host, const char *filename, struct portable_stat *st)
{E_
    char errmsg[REMOTEFS_ERR_MSG_LEN] = "";
    struct remotefs *u;

    u = remotefs_lookup (host, NULL);
    if ((*u->remotefs_checkordinaryfileaccess) (u, filename, SIZE_LIMIT, st, errmsg)) {
        edit_error_dialog (_ (" Error "), errmsg);
        return 1;
    }

    return 0;
}

/* returns 1 on error */
static int edit_open_file (WEdit * edit, const char *host, const char *filename, const char *text, unsigned long text_size)
{E_
    struct portable_stat st;
    if (text) {
	edit->last_byte = text_size;
	filename = 0;
    } else {
	int r;
	r = check_file_access (edit, host, filename, &st);
#if defined(MIDNIGHT) || defined(GTK)
	if (r == 2)
	    return edit->delete_file = 1;
#endif
	if (r)
	    return 1;
	edit->stat = st;
        edit->test_file_on_disk_for_changes_m_time = edit->stat.ustat.st_mtime;
	edit->last_byte = st.ustat.st_size;
    }
    return init_dynamic_edit_buffers (edit, host, filename, text);
}

#ifdef MIDNIGHT
#define space_width 1
#else
int space_width;
extern int option_long_whitespace;

void edit_set_space_width (int s)
{E_
    space_width = s;
}

#endif

edit_file_is_open_fn_t edit_file_is_open = 0;

/* fills in the edit struct. returns 0 on fail. Pass edit as NULL for this */
WEdit *edit_init (WEdit * edit, int lines, int columns, const char *filename, const char *text, const char *host, const char *dir, unsigned long text_size, int new_window)
{E_
    char *f;
    int to_free = 0;
    int use_filter = 0;
#ifndef MIDNIGHT
    if (option_long_whitespace)
	edit_set_space_width (FONT_PER_CHAR(' ') * 2);
    else
	edit_set_space_width (FONT_PER_CHAR(' '));
#endif
    if (!edit) {
	edit = malloc (sizeof (WEdit));
	memset (edit, 0, sizeof (WEdit));
	to_free = 1;
    }
    memset (&(edit->from_here), 0, (unsigned long) &(edit->to_here) - (unsigned long) &(edit->from_here));
#ifndef MIDNIGHT
    edit->max_column = columns * FONT_MEAN_WIDTH;
#endif
    edit->num_widget_lines = lines;
    edit->num_widget_columns = columns;
    edit->stat.ustat.st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    edit->stat.ustat.st_uid = getuid ();
    edit->stat.ustat.st_gid = getgid ();
    edit->bracket = -1;
    edit->last_get_mb_rule = -2;
    if (!dir)
	dir = "";
    if (!host || !*host)
	host = REMOTEFS_LOCAL;
    f = (char *) filename;
    if (filename) {
	f = catstrs (dir, filename, NULL);
	if (edit_file_is_open) {
            int o;
            o = (*edit_file_is_open) (host, f, new_window);
	    if (o == -1) {      /* error */
		if (to_free)
		    free (edit);
		return 0;
	    }
	    if (o == 1) {      /* already open */
		if (to_free)
		    free (edit);
		return 0;
	    }
        }
    }
    if (edit_find_filter (f) < 0) {
	if (edit_open_file (edit, host, f, text, text_size)) {
/* edit_load_file already gives an error message */
	    if (to_free)
		free (edit);
	    return 0;
	}
    } else {
	use_filter = 1;
	if (edit_open_file (edit, host, 0, "", 0)) {
	    if (to_free)
		free (edit);
	    return 0;
	}
    }
    edit->force |= REDRAW_PAGE;
    if (filename) {
	filename = catstrs (dir, filename, NULL);
	if (edit_split_filename (edit, host, (char *) filename)) {
	    if (to_free)
		free (edit);
	    return 0;
	}
    } else {
	edit->filename = (char *) strdup ("");
	edit->dir = (char *) strdup (dir);
        edit->host = (char *) strdup (host);
    }
    edit->stack_size = START_STACK_SIZE;
    edit->stack_size_mask = START_STACK_SIZE - 1;
    edit->undo_stack = (struct undo_element *) malloc ((edit->stack_size + 10) * sizeof (struct undo_element));
    memset (edit->undo_stack, '\0', (edit->stack_size + 10) * sizeof (struct undo_element));
    edit->total_lines = edit_count_lines (edit, 0, edit->last_byte);
    if (use_filter) {
	struct portable_stat st;
	push_action_disabled = 1;
	if (check_file_access (edit, edit->host, filename, &st)) {
	    edit_clean (edit);
	    if (to_free)
		free (edit);
	    return 0;
	}
	edit->stat = st;
        edit->test_file_on_disk_for_changes_m_time = edit->stat.ustat.st_mtime;
	if (!edit_insert_file (edit, f)) {
	    edit_clean (edit);
	    if (to_free)
		free (edit);
	    return 0;
	}
/* FIXME: this should be an unmodification() function */
	push_action_disabled = 0;
    }
    edit->modified = 0;
    edit_load_syntax (edit, 0, 0);
    {
	int fg, bg;
	edit_get_syntax_color (edit, -1, &fg, &bg);
    }
    return edit;
}

/* clear the edit struct, freeing everything in it. returns 1 on success */
int edit_clean (WEdit * edit)
{E_
    if (edit) {
	int j = 0;
	edit_free_syntax_rules (edit);
	edit_get_wide_byte (edit, -1);
	book_mark_flush (edit, -1);
	for (; j <= MAXBUFF; j++) {
	    if (edit->buffers1[j] != NULL)
		free (edit->buffers1[j]);
	    if (edit->buffers2[j] != NULL)
		free (edit->buffers2[j]);
	}

	if (edit->undo_stack)
	    free (edit->undo_stack);
	if (edit->filename)
	    free (edit->filename);
	if (edit->dir)
	    free (edit->dir);
	if (edit->host)
	    free (edit->host);
/* we don't want to clear the widget */
	memset (&(edit->from_here), 0, (unsigned long) &(edit->to_here) - (unsigned long) &(edit->from_here));
	return 1;
    }
    return 0;
}


/* returns 1 on success */
int edit_renew (WEdit * edit)
{E_
    int lines = edit->num_widget_lines;
    int columns = edit->num_widget_columns;
    char *dir;
    char *host;
    WEdit *r;

    if (edit->dir)
	dir = (char *) strdup (edit->dir);
    else
	dir = 0;
    host = (char *) strdup (edit->host);
    edit_clean (edit);
    r = edit_init (edit, lines, columns, 0, "", host, dir, 0, 0);
    if (dir)
        free (dir);
    if (host)
        free (host);
    return r != NULL;
}

void edit_clear_macro (struct macro_rec *macro)
{E_
    int i;
    for (i = 0; i < macro->macro_i; i++)
        CStr_free (&macro->macro[i].ch);
    macro->macro_i = -1;
}

/* returns 1 on success, if returns 0, the edit struct would have been free'd */
int edit_reload (WEdit * edit, const char *filename, const char *text, const char *host, const char *dir, unsigned long text_size)
{E_
    WEdit *e;
    int lines = edit->num_widget_lines;
    int columns = edit->num_widget_columns;
    e = malloc (sizeof (WEdit));
    memset (e, 0, sizeof (WEdit));
    e->widget = edit->widget;
    edit_clear_macro (&e->macro);
    if (!edit_init (e, lines, columns, filename, text, host, dir, text_size, 0)) {
	free (e);
	return 0;
    }
    edit_clean (edit);
    memcpy (edit, e, sizeof (WEdit));
    free (e);
    return 1;
}


void show_undo_stack (WEdit * edit, const char *action)
{E_
#if 0
    int i;
    if (option_max_undo > 32)
        option_max_undo = 32;
    printf("\n");
    printf("%s\n", action);
    printf("|");
    for (i = 0; i < 32 && i < edit->stack_size; i++) {
        int c;
        c = edit->undo_stack[i].command;
        printf("%c%03ld|", c ? c : ' ', edit->undo_stack[i].param);
    }
    printf("\n");
    for (i = 0; i < 32 && i < edit->stack_size; i++) {
        printf("%c%c   ", i == edit->stack_bottom ? '[' : ' ', i == edit->stack_pointer ? '^' : ' ');
    }
    printf("\n");
#endif
}

#define WMOD(x)         ((x) & edit->stack_size_mask)

void edit_push_action (WEdit * edit, int command, long param)
{E_
    unsigned long sp = edit->stack_pointer;
    struct undo_element c;

    c.command = command;
    c.param = param;

/* first enlarge the stack if necessary */
    if (sp > edit->stack_size - 10) {	/* say */
	if (option_max_undo < START_STACK_SIZE)
	    option_max_undo = START_STACK_SIZE;
	if (edit->stack_size < option_max_undo) {
            struct undo_element *t;
            int new_stack_alloc;
            new_stack_alloc = edit->stack_size * 2 + 10;
	    t = malloc (new_stack_alloc * sizeof (struct undo_element));
	    if (t) {
		memcpy (t, edit->undo_stack, sizeof (struct undo_element) * edit->stack_size);
                memset (&t[edit->stack_size], '\0', (new_stack_alloc - edit->stack_size) * sizeof (struct undo_element));
		free (edit->undo_stack);
		edit->undo_stack = t;
		edit->stack_size <<= 1;
		edit->stack_size_mask = edit->stack_size - 1;
	    }
	}
    }

    if (push_action_disabled)
	return;

#define SAMECMD(a,b)    ((a).command == (b).command && (a).param == (b).param)

    if (sp != edit->stack_bottom && WMOD (sp - 1) != edit->stack_bottom) {
	struct undo_element *spm1;
        spm1 = &(edit->undo_stack[WMOD (sp - 1)]);
	if (spm1->command == REPEAT_COMMAND && WMOD (sp - 2) != edit->stack_bottom) {
	    struct undo_element *spm2;
            spm2 = &(edit->undo_stack[WMOD (sp - 2)]);
	    if (SAMECMD (*spm2, c)) {
		if (c.command == KEY_PRESS)
		    goto out;     /* --> no need to push multiple do-nothings */
		spm1->param++;  /* add one repeat */
                goto out;
	    }
	    else if ((c.command == CURS_LEFT && spm2->command == CURS_RIGHT)
		     || (c.command == CURS_RIGHT && spm2->command == CURS_LEFT)) {	/* a left then a right anihilate each other */
		if (spm1->param == 2)
		    edit->stack_pointer = WMOD (edit->stack_pointer - 1);
		else
		    spm1->param--;
		goto out;
	    }
	} else {
	    if (SAMECMD (*spm1, c)) {
		if (c.command == KEY_PRESS)
		    goto out;	/* --> no need to push multiple do-nothings */
		edit->undo_stack[sp].command = REPEAT_COMMAND;
		edit->undo_stack[sp].param = 2;
		goto check_bottom;
	    }
	    else if ((c.command == CURS_LEFT && spm1->command == CURS_RIGHT)
		     || (c.command == CURS_RIGHT && spm1->command == CURS_LEFT)) {	/* a left then a right anihilate each other */
		edit->stack_pointer = WMOD (edit->stack_pointer - 1);
		goto out;
	    }
	}
    }
    edit->undo_stack[sp] = c;

  check_bottom:

    edit->stack_pointer = WMOD (edit->stack_pointer + 1);

/*if the sp wraps round and catches the stack_bottom then erase the first set of actions on the stack to make space - by moving stack_bottom forward one "key press" */
    if (WMOD (edit->stack_pointer + 2) == edit->stack_bottom || WMOD (edit->stack_pointer + 3) == edit->stack_bottom) {
	for (;;) {
	    edit->stack_bottom = WMOD (edit->stack_bottom + 1);
            if (edit->undo_stack[edit->stack_bottom].command == KEY_PRESS)
                break;
            if (edit->stack_bottom == edit->stack_pointer) {
                edit->stack_bottom = edit->stack_pointer = 0;
                break;
            }
	}
    }

/*If a single key produced enough pushes to wrap all the way round then we would notice that the [stack_bottom] does not contain KEY_PRESS. The stack is then initialised: */
    if (edit->undo_stack[edit->stack_bottom].command != KEY_PRESS)
	edit->stack_bottom = edit->stack_pointer = 0;

  out:
    show_undo_stack (edit, "PUSH");
    return;
}

/*
   TODO: if the user undos until the stack bottom, and the stack has not wrapped,
   then the file should be as it was when he loaded up. Then set edit->modified to 0.
 */
static struct undo_element pop_action (WEdit * edit)
{E_
    struct undo_element c;
    unsigned long sp = edit->stack_pointer;
    if (sp == edit->stack_bottom)
	goto wall;
    sp = WMOD (sp - 1);
    c = edit->undo_stack[sp];
    if (c.command == REPEAT_COMMAND) {
	if (sp == edit->stack_bottom)
	    goto wall;
	if (edit->undo_stack[sp].param == 2)
	    edit->stack_pointer = sp;
	else
	    edit->undo_stack[sp].param--;
        show_undo_stack (edit, "POP");
	return edit->undo_stack[WMOD (sp - 1)];
    }
    edit->stack_pointer = WMOD (edit->stack_pointer - 1);
    show_undo_stack (edit, "POP");
    return c;

  wall:
    c.command = STACK_BOTTOM;
    c.param = 0;
    return c;
}

/* is called whenever a modification is made by one of the four routines below */
static inline void appearance_modification (WEdit * edit, long p)
{E_
    edit->screen_modified = 1;
    if (edit->last_get_mb_rule > p - 1) {
	edit->last_get_mb_rule = p - 1;
	edit->mb_invalidate = 1;
    }
    if (edit->last_get_mb_rule > p - 1) {
	edit->last_get_rule = p - 1;
	edit->syntax_invalidate = 1;
    }
}

/* is called whenever a modification is made by one of the four routines below */
static inline void edit_modification (WEdit * edit, long p)
{E_
    edit->caches_valid = 0;
    edit->modified = 1;
    appearance_modification (edit, p);
}

void edit_appearance_modification (WEdit * edit)
{E_
    appearance_modification (edit, 0);
}

/*
   Basic low level single character buffer alterations and movements at the cursor.
   Returns char passed over, inserted or removed.
 */

void edit_insert (WEdit * edit, int c)
{E_
/* check if file has grown to large */
    if (edit->last_byte >= SIZE_LIMIT)
	return;

/* first we must update the position of the display window */
    if (edit->curs1 < edit->start_display) {
	edit->start_display++;
	if (c == '\n')
	    edit->start_line++;
    }
/* now we must update some info on the file and check if a redraw is required */
    if (c == '\n') {
	if (edit->book_mark) {
            if (edit->curs1 < 1 || edit->buffers1[(edit->curs1 - 1) >> S_EDIT_BUF_SIZE][(edit->curs1 - 1) & M_EDIT_BUF_SIZE] == '\n')
	        book_mark_inc (edit, edit->curs_line - 1);
            else
	        book_mark_inc (edit, edit->curs_line);
	    edit->force |= REDRAW_LINE_ABOVE;
        }
	edit->curs_line++;
	edit->total_lines++;
	edit->force |= REDRAW_LINE_ABOVE | REDRAW_AFTER_CURSOR;
    }
/* tell that we've modified the file */
    edit_modification (edit, edit->curs1);

/* save the reverse command onto the undo stack */
    edit_push_action (edit, BACKSPACE, 0);

/* update markers */
    edit->mark1 += (edit->mark1 > edit->curs1);
    edit->mark2 += (edit->mark2 > edit->curs1);

/* add a new buffer if we've reached the end of the last one */
    if (!(edit->curs1 & M_EDIT_BUF_SIZE))
	edit->buffers1[edit->curs1 >> S_EDIT_BUF_SIZE] = malloc (EDIT_BUF_SIZE);

/* perfprm the insertion */
    edit->buffers1[edit->curs1 >> S_EDIT_BUF_SIZE][edit->curs1 & M_EDIT_BUF_SIZE] = (unsigned char) c;

/* update file length */
    edit->last_byte++;

/* update cursor position */
    edit->curs1++;
}


/* same as edit_insert and move left */
void edit_insert_ahead (WEdit * edit, int c)
{E_
    if (edit->last_byte >= SIZE_LIMIT)
	return;
    if (edit->curs1 < edit->start_display) {
	edit->start_display++;
	if (c == '\n')
	    edit->start_line++;
    }
    if (c == '\n') {
	if (edit->book_mark) {
            if (edit->curs1 < 1 || edit->buffers1[(edit->curs1 - 1) >> S_EDIT_BUF_SIZE][(edit->curs1 - 1) & M_EDIT_BUF_SIZE] == '\n')
	        book_mark_inc (edit, edit->curs_line - 1);
            else
	        book_mark_inc (edit, edit->curs_line);
	    edit->force |= REDRAW_LINE_ABOVE;
        }
	edit->total_lines++;
	edit->force |= REDRAW_AFTER_CURSOR;
    }
    edit_modification (edit, edit->curs1);
    edit_push_action (edit, ACT_DELETE, 0);

    edit->mark1 += (edit->mark1 >= edit->curs1);
    edit->mark2 += (edit->mark2 >= edit->curs1);

    if (!((edit->curs2 + 1) & M_EDIT_BUF_SIZE))
	edit->buffers2[(edit->curs2 + 1) >> S_EDIT_BUF_SIZE] = malloc (EDIT_BUF_SIZE);
    edit->buffers2[edit->curs2 >> S_EDIT_BUF_SIZE][EDIT_BUF_SIZE - (edit->curs2 & M_EDIT_BUF_SIZE) - 1] = c;

    edit->last_byte++;
    edit->curs2++;
}


int edit_delete (WEdit * edit)
{E_
    int p;
    if (!edit->curs2)
	return 0;

    edit->mark1 -= (edit->mark1 > edit->curs1);
    edit->mark2 -= (edit->mark2 > edit->curs1);

    p = edit->buffers2[(edit->curs2 - 1) >> S_EDIT_BUF_SIZE][EDIT_BUF_SIZE - ((edit->curs2 - 1) & M_EDIT_BUF_SIZE) - 1];

    if (!(edit->curs2 & M_EDIT_BUF_SIZE)) {
	free (edit->buffers2[edit->curs2 >> S_EDIT_BUF_SIZE]);
	edit->buffers2[edit->curs2 >> S_EDIT_BUF_SIZE] = NULL;
    }
    edit->last_byte--;
    edit->curs2--;

    if (p == '\n') {
	if (edit->book_mark) {
            if (edit->curs1 < 1 || edit->buffers1[(edit->curs1 - 1) >> S_EDIT_BUF_SIZE][(edit->curs1 - 1) & M_EDIT_BUF_SIZE] == '\n')
                book_mark_dec (edit, edit->curs_line);
            else
	        book_mark_dec (edit, edit->curs_line + 1);
	    edit->force |= REDRAW_LINE_ABOVE;
        }
	edit->total_lines--;
	edit->force |= REDRAW_AFTER_CURSOR;
    }
    edit_push_action (edit, INSERT_AHEAD, p);
    if (edit->curs1 < edit->start_display) {
	edit->start_display--;
	if (p == '\n')
	    edit->start_line--;
    }
    edit_modification (edit, edit->curs1);

    return p;
}


int edit_backspace (WEdit * edit)
{E_
    int p;
    if (!edit->curs1)
	return 0;

    edit->mark1 -= (edit->mark1 >= edit->curs1);
    edit->mark2 -= (edit->mark2 >= edit->curs1);

    p = *(edit->buffers1[(edit->curs1 - 1) >> S_EDIT_BUF_SIZE] + ((edit->curs1 - 1) & M_EDIT_BUF_SIZE));
    if (!((edit->curs1 - 1) & M_EDIT_BUF_SIZE)) {
	free (edit->buffers1[edit->curs1 >> S_EDIT_BUF_SIZE]);
	edit->buffers1[edit->curs1 >> S_EDIT_BUF_SIZE] = NULL;
    }
    edit->last_byte--;
    edit->curs1--;

    if (p == '\n') {
	if (edit->book_mark) {
            if (edit->curs1 < 1 || edit->buffers1[(edit->curs1 - 1) >> S_EDIT_BUF_SIZE][(edit->curs1 - 1) & M_EDIT_BUF_SIZE] == '\n')
	        book_mark_dec (edit, edit->curs_line - 2);
            else
	        book_mark_dec (edit, edit->curs_line - 1);
	    edit->force |= REDRAW_LINE_ABOVE;
        }
	edit->curs_line--;
	edit->total_lines--;
	edit->force |= REDRAW_AFTER_CURSOR;
    }
    edit_push_action (edit, INSERT_BEHIND, p);

    if (edit->curs1 < edit->start_display) {
	edit->start_display--;
	if (p == '\n')
	    edit->start_line--;
    }
    edit_modification (edit, edit->curs1);

    return p;
}

int edit_delete_wide (WEdit * edit)
{E_
    struct mb_rule r;
    r = get_mb_rule (edit, edit->curs1);
    edit_delete (edit);
    while (r.end--)
	edit_delete (edit);
    return r.ch;
}

void edit_insert_unicode (WEdit * edit, C_wchar_t wc)
{E_
    unsigned char *c;
    c = wcrtomb_wchar_to_utf8 (wc);
    if (!*c) {
	edit_insert (edit, *c);
    } else {
	for (; *c; c++)
	    edit_insert (edit, *c);
    }
}

long edit_backspace_wide (WEdit * edit)
{E_
    int i;
    long c = 0;
    if (!edit->curs1)
	return 0;
    for (i = edit->curs1 - 1; i >= 0; i--)
	if ((c = edit_get_wide_byte (edit, i)) != -1)
	    break;
    while (edit->curs1 > i)
	edit_backspace (edit);
    return c;
}

static int right_of_four_spaces (WEdit *edit);
void edit_tab_cmd (WEdit * edit);

void edit_backspace_tab (WEdit * edit, int whole_tabs_only)
{E_
    int i;
    if (whole_tabs_only) {
	int indent;
	/* count the number of columns of indentation */
	indent = edit_move_forward3 (edit, edit_bol (edit, edit->curs1), 0, edit->curs1);
	for (;;) {
	    int c;
	    c = edit_get_byte (edit, edit->curs1 - 1);
	    if (!isspace (c) || c == '\n')
		break;
	    edit_backspace (edit);
	}
	while (edit_move_forward3 (edit, edit_bol (edit, edit->curs1), 0, edit->curs1) <
	       (indent - space_width * (option_fake_half_tabs ? HALF_TAB_SIZE : TAB_SIZE)))
	    edit_tab_cmd (edit);
	return;
    }
    if (option_fake_half_tabs && right_of_four_spaces (edit)) {
	for (i = 0; i < HALF_TAB_SIZE; i++)
	    edit_backspace (edit);
	return;
    }
    edit_backspace (edit);
}

void edit_wide_char_align (WEdit * edit)
{E_
    while (edit_get_wide_byte (edit, edit->curs1) == -1)
	edit_cursor_move (edit, 1);
}

void edit_wide_char_align_left (WEdit * edit)
{E_
    while (edit_get_wide_byte (edit, edit->curs1) == -1)
	edit_cursor_move (edit, -1);
}


/* moves the cursor right or left: increment positive or negative respectively */
int edit_cursor_move (WEdit * edit, long increment)
{E_
/* this is the same as a combination of two of the above routines, with only one push onto the undo stack */
    int c;

    if (increment < 0) {
	for (; increment < 0; increment++) {
	    if (!edit->curs1)
		return -1;

	    edit_push_action (edit, CURS_RIGHT, 0);

	    c = edit_get_byte (edit, edit->curs1 - 1);
	    if (!((edit->curs2 + 1) & M_EDIT_BUF_SIZE))
		edit->buffers2[(edit->curs2 + 1) >> S_EDIT_BUF_SIZE] = malloc (EDIT_BUF_SIZE);
	    edit->buffers2[edit->curs2 >> S_EDIT_BUF_SIZE][EDIT_BUF_SIZE - (edit->curs2 & M_EDIT_BUF_SIZE) - 1] = c;
	    edit->curs2++;
	    c = edit->buffers1[(edit->curs1 - 1) >> S_EDIT_BUF_SIZE][(edit->curs1 - 1) & M_EDIT_BUF_SIZE];
	    if (!((edit->curs1 - 1) & M_EDIT_BUF_SIZE)) {
		free (edit->buffers1[edit->curs1 >> S_EDIT_BUF_SIZE]);
		edit->buffers1[edit->curs1 >> S_EDIT_BUF_SIZE] = NULL;
	    }
	    edit->curs1--;
	    if (c == '\n') {
		edit->curs_line--;
		edit->force |= REDRAW_LINE_BELOW;
	    }
	}

	return c;
    } else if (increment > 0) {
	for (; increment > 0; increment--) {
	    if (!edit->curs2)
		return -2;

	    edit_push_action (edit, CURS_LEFT, 0);

	    c = edit_get_byte (edit, edit->curs1);
	    if (!(edit->curs1 & M_EDIT_BUF_SIZE))
		edit->buffers1[edit->curs1 >> S_EDIT_BUF_SIZE] = malloc (EDIT_BUF_SIZE);
	    edit->buffers1[edit->curs1 >> S_EDIT_BUF_SIZE][edit->curs1 & M_EDIT_BUF_SIZE] = c;
	    edit->curs1++;
	    c = edit->buffers2[(edit->curs2 - 1) >> S_EDIT_BUF_SIZE][EDIT_BUF_SIZE - ((edit->curs2 - 1) & M_EDIT_BUF_SIZE) - 1];
	    if (!(edit->curs2 & M_EDIT_BUF_SIZE)) {
		free (edit->buffers2[edit->curs2 >> S_EDIT_BUF_SIZE]);
		edit->buffers2[edit->curs2 >> S_EDIT_BUF_SIZE] = 0;
	    }
	    edit->curs2--;
	    if (c == '\n') {
		edit->curs_line++;
		edit->force |= REDRAW_LINE_ABOVE;
	    }
	}
	return c;
    } else
	return -3;
}

/* These functions return positions relative to lines */

/* returns index of last char on line + 1 */
long edit_eol (WEdit * edit, long current)
{E_
    if (current < edit->last_byte) {
	for (;; current++)
	    if (edit_get_byte (edit, current) == '\n')
		break;
    } else
	return edit->last_byte;
    return current;
}

/* returns index of first char on line */
long edit_bol (WEdit * edit, long current)
{E_
    if (current > 0) {
	for (;; current--)
	    if (edit_get_byte (edit, current - 1) == '\n')
		break;
    } else
	return 0;
    return current;
}


int edit_count_lines (WEdit * edit, long current, int upto)
{E_
    int lines = 0;
    if (upto > edit->last_byte)
	upto = edit->last_byte;
    if (current < 0)
	current = 0;
    while (current < upto)
	if (edit_get_byte (edit, current++) == '\n')
	    lines++;
    return lines;
}


/* If lines is zero this returns the count of lines from current to upto. */
/* If upto is zero returns index of lines forward current. */
long edit_move_forward (WEdit * edit, long current, int lines, long upto)
{E_
    if (upto) {
	return edit_count_lines (edit, current, upto);
    } else {
	int next;
	if (lines < 0)
	    lines = 0;
	while (lines--) {
	    next = edit_eol (edit, current) + 1;
	    if (next > edit->last_byte)
		break;
	    else
		current = next;
	}
	return current;
    }
}


/* Returns offset of 'lines' lines up from current */
long edit_move_backward (WEdit * edit, long current, int lines)
{E_
    if (lines < 0)
	lines = 0;
    current = edit_bol (edit, current);
    while((lines--) && current != 0)
	current = edit_bol (edit, current - 1);
    return current;
}

#ifdef MIDNIGHT
/* If cols is zero this returns the count of columns from current to upto. */
/* If upto is zero returns index of cols across from current. */
long edit_move_forward3 (WEdit * edit, long current, int cols, long upto)
{E_
    long p, q;
    int col = 0;

    if (upto) {
	q = upto;
	cols = -10;
    } else
	q = edit->last_byte + 2;

    for (col = 0, p = current; p < q; p++) {
	int c;
	if (cols != -10) {
	    if (col == cols)
		return p;
	    if (col > cols)
		return p - 1;
	}
	c = edit_get_byte (edit, p);
	if (c == '\r')
	    continue;
	else
	if (c == '\t')
	    col += TAB_SIZE - col % TAB_SIZE;
	else
	    col++;
	/*if(edit->nroff ... */
	if (c == '\n') {
	    if (upto)
		return col;
	    else
		return p;
	}
    }
    return (float) col;
}
#endif

/* returns the current column position of the cursor */
int edit_get_col (WEdit * edit)
{E_
    return edit_move_forward3 (edit, edit_bol (edit, edit->curs1), 0, edit->curs1);
}


/* Scrolling functions */

void edit_update_curs_row (WEdit * edit)
{E_
    edit->curs_row = edit->curs_line - edit->start_line;
}

void edit_update_curs_col (WEdit * edit)
{E_
    long b;
    b = edit_bol (edit, edit->curs1);
    edit->curs_col = edit_move_forward3(edit, b, 0, edit->curs1);
    edit->curs_charcolumn = edit->curs1 - b;
}

/*moves the display start position up by i lines */
void edit_scroll_upward (WEdit * edit, unsigned long i)
{E_
    int lines_above = edit->start_line;
    if (i > lines_above)
	i = lines_above;
    if (i) {
	edit->start_line -= i;
	edit->start_display = edit_move_backward (edit, edit->start_display, i);
	edit->force |= REDRAW_PAGE;
	edit->force &= (0xfff - REDRAW_CHAR_ONLY);
    }
    edit_update_curs_row (edit);
}

int edit_get_line_height (WEdit * edit, int row);

/* returns 1 if could scroll, 0 otherwise */
void edit_scroll_downward (WEdit * edit, int i)
{E_
    int lines_below;
    lines_below = edit->total_lines - edit->start_line;
    if (lines_below > 0) {
        if (edit->start_line + i < edit->total_lines - edit->num_widget_lines) {
            /* optimization -- we are no where near then bottom page */
        } else {
	    int top, y = 0;

/* Now makes sure the last line is on the bottom margin if the
user attempt to scroll too far: */
            CPushFont("editor");
	    for (top = edit->total_lines;; top--) {
	        y += edit_get_line_height (edit, top);
	        if (y > CHeightOf (edit->widget) - EDIT_FRAME_H) {
                    top++;
                    break;
                }
/* This can happen if the user enlarges the window so that the
last edit line is in the middle of the window and the bottom half
of the window is blank: */
                if (top <= edit->start_line)
                    break;
	    }
            CPopFont();
	    if (edit->start_line + i > top)
	        i = top - edit->start_line;
        }
        if (i > 0) {
	    edit->start_line += i;
	    edit->start_display = edit_move_forward (edit, edit->start_display, i, 0);
	    edit->force |= REDRAW_PAGE;
	    edit->force &= (0xfff - REDRAW_CHAR_ONLY);
        }
    }
    edit_update_curs_row (edit);
}

void edit_scroll_right (WEdit * edit, int i)
{E_
    edit->force |= REDRAW_PAGE;
    edit->force &= (0xfff - REDRAW_CHAR_ONLY);
    edit->start_col -= i;
}

void edit_scroll_left (WEdit * edit, int i)
{E_
    if (edit->start_col) {
	edit->start_col += i;
	if (edit->start_col > 0)
	    edit->start_col = 0;
	edit->force |= REDRAW_PAGE;
	edit->force &= (0xfff - REDRAW_CHAR_ONLY);
    }
}

/* high level cursor movement commands */

static int is_in_indent (WEdit *edit)
{E_
    long p = edit_bol (edit, edit->curs1);
    while (p < edit->curs1)
	if (!strchr (" \t", edit_get_byte (edit, p++)))
	    return 0;
    return 1;
}

static int left_of_four_spaces (WEdit *edit);

void edit_move_to_prev_col (WEdit * edit, long p)
{E_
    edit_cursor_move (edit, edit_move_forward3 (edit, p, edit->prev_col, 0) - edit->curs1);

    if (is_in_indent (edit) && option_fake_half_tabs) {
	edit_update_curs_col (edit);
	if (space_width)
	if (edit->curs_col % (HALF_TAB_SIZE * space_width)) {
	    int q = edit->curs_col;
	    edit->curs_col -= (edit->curs_col % (HALF_TAB_SIZE * space_width));
	    p = edit_bol (edit, edit->curs1);
	    edit_cursor_move (edit, edit_move_forward3 (edit, p, edit->curs_col, 0) - edit->curs1);
	    if (!left_of_four_spaces (edit))
		edit_cursor_move (edit, edit_move_forward3 (edit, p, q, 0) - edit->curs1);
	}
    }
}


/* move i lines */
void edit_move_up (WEdit * edit, unsigned long i, int scroll)
{E_
    long p, l = edit->curs_line;

    if (i > l)
	i = l;
    if (i) {
	if (i > 1)
	    edit->force |= REDRAW_PAGE;
	if (scroll)
	    edit_scroll_upward (edit, i);

	p = edit_bol (edit, edit->curs1);
	edit_cursor_move (edit, (p = edit_move_backward (edit, p, i)) - edit->curs1);
	edit_move_to_prev_col (edit, p);

	edit->search_start = edit->curs1;
	edit->found_len = 0;
    }
}

int is_blank (WEdit * edit, long offset)
{E_
    long s, f;
    int c;
    s = edit_bol (edit, offset);
    f = edit_eol (edit, offset) - 1;
    while (s <= f) {
	c = edit_get_byte (edit, s++);
	if (!isspace (c))
	    return 0;
    }
    return 1;
}


/* returns the offset of line i */
long edit_find_line (WEdit * edit, int line)
{E_
    int i, j = 0;
    int m = 2000000000;
    if (!edit->caches_valid) {
	for (i = 0; i < N_LINE_CACHES; i++)
	    edit->line_numbers[i] = edit->line_offsets[i] = 0;
/* three offsets that we *know* are line 0 at 0 and these two: */
	edit->line_numbers[1] = edit->curs_line;
	edit->line_offsets[1] = edit_bol (edit, edit->curs1);
	edit->line_numbers[2] = edit->total_lines;
	edit->line_offsets[2] = edit_bol (edit, edit->last_byte);
	edit->caches_valid = 1;
    }
    if (line >= edit->total_lines)
	return edit->line_offsets[2];
    if (line <= 0)
	return 0;
/* find the closest known point */
    for (i = 0; i < N_LINE_CACHES; i++) {
	int n;
	n = abs (edit->line_numbers[i] - line);
	if (n < m) {
	    m = n;
	    j = i;
	}
    }
    if (m == 0)
	return edit->line_offsets[j];	/* know the offset exactly */
    if (m == 1 && j >= 3)
	i = j;			/* one line different - caller might be looping, so stay in this cache */
    else
	i = 3 + (rand () % (N_LINE_CACHES - 3));
    if (line > edit->line_numbers[j])
	edit->line_offsets[i] = edit_move_forward (edit, edit->line_offsets[j], line - edit->line_numbers[j], 0);
    else
	edit->line_offsets[i] = edit_move_backward (edit, edit->line_offsets[j], edit->line_numbers[j] - line);
    edit->line_numbers[i] = line;
    return edit->line_offsets[i];
}

int line_is_blank (WEdit * edit, long line)
{E_
    return is_blank (edit, edit_find_line (edit, line));
}

/* moves up until a blank line is reached, or until just 
   before a non-blank line is reached */
static void edit_move_up_paragraph (WEdit * edit, int scroll)
{E_
    int i;
    if (edit->curs_line <= 1) {
	i = 0;
    } else {
	if (line_is_blank (edit, edit->curs_line)) {
	    if (line_is_blank (edit, edit->curs_line - 1)) {
		for (i = edit->curs_line - 1; i; i--)
		    if (!line_is_blank (edit, i)) {
			i++;
			break;
		    }
	    } else {
		for (i = edit->curs_line - 1; i; i--)
		    if (line_is_blank (edit, i))
			break;
	    }
	} else {
	    for (i = edit->curs_line - 1; i; i--)
		if (line_is_blank (edit, i))
		    break;
	}
    }
    edit_move_up (edit, edit->curs_line - i, scroll);
}

/* move i lines */
void edit_move_down (WEdit * edit, int i, int scroll)
{E_
    long p, l = edit->total_lines - edit->curs_line;

    if (i > l)
	i = l;
    if (i) {
	if (i > 1)
	    edit->force |= REDRAW_PAGE;
	if (scroll)
	    edit_scroll_downward (edit, i);
	p = edit_bol (edit, edit->curs1);
	edit_cursor_move (edit, (p = edit_move_forward (edit, p, i, 0)) - edit->curs1);
	edit_move_to_prev_col (edit, p);

	edit->search_start = edit->curs1;
	edit->found_len = 0;
    }
}

/* moves down until a blank line is reached, or until just
   before a non-blank line is reached */
static void edit_move_down_paragraph (WEdit * edit, int scroll)
{E_
    int i;
    if (edit->curs_line >= edit->total_lines - 1) {
	i = edit->total_lines;
    } else {
	if (line_is_blank (edit, edit->curs_line)) {
	    if (line_is_blank (edit, edit->curs_line + 1)) {
		for (i = edit->curs_line + 1; i; i++)
		    if (!line_is_blank (edit, i) || i > edit->total_lines) {
			i--;
			break;
		    }
	    } else {
		for (i = edit->curs_line + 1; i; i++)
		    if (line_is_blank (edit, i) || i >= edit->total_lines)
			break;
	    }
	} else {
	    for (i = edit->curs_line + 1; i; i++)
		if (line_is_blank (edit, i) || i >= edit->total_lines)
		    break;
	}
    }
    edit_move_down (edit, i - edit->curs_line, scroll);
}

static void edit_begin_page (WEdit *edit)
{E_
    edit_update_curs_row (edit);
    edit_move_up (edit, edit->curs_row, 0);
}

static void edit_end_page (WEdit *edit)
{E_
    edit_update_curs_row (edit);
    edit_move_down (edit, edit->num_widget_lines - edit->curs_row - 1, 0);
}


/* goto beginning of text */
static void edit_move_to_top (WEdit * edit)
{E_
    if (edit->curs_line) {
	edit_cursor_move (edit, -edit->curs1);
	edit_move_to_prev_col (edit, 0);
	edit->force |= REDRAW_PAGE;
	edit->search_start = 0;
	edit_update_curs_row(edit);
    }
}


/* goto end of text */
static void edit_move_to_bottom (WEdit * edit)
{E_
    if (edit->curs_line < edit->total_lines) {
	edit_cursor_move (edit, edit->curs2);
	edit->start_display = edit->last_byte;
	edit->start_line = edit->total_lines;
	edit_update_curs_row(edit);
	edit_scroll_upward (edit, edit->num_widget_lines - 1);
	edit->force |= REDRAW_PAGE;
    }
}

/* goto beginning of line */
static void edit_cursor_to_bol (WEdit * edit)
{E_
    edit_cursor_move (edit, edit_bol (edit, edit->curs1) - edit->curs1);
    edit->search_start = edit->curs1;
    edit->prev_col = edit_get_col (edit);
}

/* goto end of line */
static void edit_cursor_to_eol (WEdit * edit)
{E_
    edit_cursor_move (edit, edit_eol (edit, edit->curs1) - edit->curs1);
    edit->search_start = edit->curs1;
    edit->prev_col = edit_get_col (edit);
}

/* move cursor to line 'line' */
void edit_move_to_line (WEdit * e, long line)
{E_
    if(line < e->curs_line)
	edit_move_up (e, e->curs_line - line, 0);
    else
	edit_move_down (e, line - e->curs_line, 0);
    edit_scroll_screen_over_cursor (e);
}

/* scroll window so that first visible line is 'line' */
void edit_move_display (WEdit * e, long line)
{E_
    if(line < e->start_line)
	edit_scroll_upward (e, e->start_line - line);
    else
	edit_scroll_downward (e, line - e->start_line);
}

/* save markers onto undo stack */
void edit_push_markers (WEdit * edit)
{E_
    edit_push_action (edit, MARK_1, edit->mark1);
    edit_push_action (edit, MARK_2, edit->mark2);
}

void free_selections (void)
{E_
    int i;
    for (i = 0; i < n_selection_history; i++)
        CStr_free(&selection_history[i]);
    n_selection_history = 0;
}

/* return -1 on nothing to store or error, zero otherwise */
void edit_get_selection (WEdit * edit)
{E_
    long start_mark, end_mark;
    CStr *c;
    int i;
    if (eval_marks (edit, &start_mark, &end_mark))
	return;
    if (n_selection_history && selection_history[0].len >= 65536) {	/* else loose it -- too much memory */
        CStr_free(&selection_history[0]);
    } else {
        int i;
        if (n_selection_history == NUM_SELECTION_HISTORY) {
            CStr_free (&selection_history[n_selection_history - 1]);
        } else {
            n_selection_history++;
        }
        for (i = n_selection_history - 2; i >= 0; i--)
            selection_history[i + 1] = selection_history[i];
    }
    c = &selection_history[0];
    c->len = end_mark - start_mark;
    c->data = CMalloc (c->len + 1);
    for (i = 0; i < c->len; i++)
	c->data[i] = edit_get_byte (edit, start_mark + i);
    c->data[c->len] = '\0';
    selection_replace (selection_history[0]);
}

void edit_set_markers (WEdit * edit, long m1, long m2, int c1, int c2)
{E_
    edit->mark1 = m1;
    edit->mark2 = m2;
    edit->column1 = c1;
    edit->column2 = c2;
}


/* highlight marker toggle */
void edit_mark_cmd (WEdit * edit, int unmark)
{E_
    edit_push_markers (edit);
    if (unmark) {
	edit_set_markers (edit, 0, 0, 0, 0);
	edit->force |= REDRAW_PAGE;
    } else {
	if (edit->mark2 >= 0) {
	    edit_set_markers (edit, edit->curs1, -1, edit->curs_col, edit->curs_col);
	    edit->force |= REDRAW_PAGE;
	} else
	    edit_set_markers (edit, edit->mark1, edit->curs1, edit->column1, edit->curs_col);
    }
}

static unsigned long my_type_of (int c)
{E_
    int x, r = 0;
    char *p;
    c &= 0xFF;
    if (!c)
	return 0;
    if (c == '!') {
	if (*option_chars_move_whole_word == '!')
	    return 0x40000000UL;
	return 0x80000000UL;
    }
    if (isupper (c))
	c = 'A';
    else if (islower (c))
	c = 'a';
    else if (isalpha (c))
	c = 'a';
    else if (isdigit (c))
	c = '0';
    else if (isspace (c))
	c = ' ';
    for (x = 1, p = option_chars_move_whole_word; *p; p++) {
        if (*p == '!')
	    x <<= 1;
        else if (*p == c)
	    r |= x;
    }
    return r;
}

void edit_left_word_move (WEdit * edit, int s)
{E_
    for (;;) {
	int c1, c2;
	edit_cursor_move (edit, -1);
	if (!edit->curs1)
	    break;
	c1 = edit_get_byte (edit, edit->curs1 - 1);
	c2 = edit_get_byte (edit, edit->curs1);
	if (!(my_type_of (c1) & my_type_of (c2)))
	    break;
	if (isspace (c1) && !isspace (c2))
	    break;
	if (s)
	    if (!isspace (c1) && isspace (c2))
		break;
    }
}

static void edit_left_word_move_cmd (WEdit * edit)
{E_
    edit_left_word_move (edit, 0);
    edit->force |= REDRAW_PAGE;
}

void edit_right_word_move (WEdit * edit, int s)
{E_
    for (;;) {
	int c1, c2;
	edit_cursor_move (edit, 1);
	if (edit->curs1 >= edit->last_byte)
	    break;
	c1 = edit_get_byte (edit, edit->curs1 - 1);
	c2 = edit_get_byte (edit, edit->curs1);
	if (!(my_type_of (c1) & my_type_of (c2)))
	    break;
	if (isspace (c1) && !isspace (c2))
	    break;
	if (s)
	    if (!isspace (c1) && isspace (c2))
		break;
    }
}

static void edit_right_word_move_cmd (WEdit * edit)
{E_
    edit_right_word_move (edit, 0);
    edit->force |= REDRAW_PAGE;
}


static void edit_right_delete_word (WEdit * edit)
{E_
    int c1, c2;
    for (;;) {
	if (edit->curs1 >= edit->last_byte)
	    break;
	c1 = edit_delete_wide (edit);
	c2 = edit_get_byte (edit, edit->curs1);
	if ((wc_isspace (c1) == 0) != (isspace (c2) == 0))
	    break;
	if (!(my_type_of (c1) & my_type_of (c2)))
	    break;
    }
}

static void edit_left_delete_word (WEdit * edit)
{E_
    int c1, c2;
    for (;;) {
	if (edit->curs1 <= 0)
	    break;
	c1 = edit_backspace_wide (edit);
	c2 = edit_get_byte (edit, edit->curs1 - 1);
	if ((wc_isspace (c1) == 0) != (isspace (c2) == 0))
	    break;
	if (!(my_type_of (c1) & my_type_of (c2)))
	    break;
    }
}

extern int column_highlighting;

/*
   the start column position is not recorded, and hence does not
   undo as it happed. But who would notice.
 */
void edit_do_undo (WEdit * edit)
{E_
    push_action_disabled = 1;	/* don't record undo's onto undo stack! */

    for (;;) {
        struct undo_element ac;

        ac = pop_action (edit);

	switch (ac.command) {
	case STACK_BOTTOM:
	    goto done_undo;
	case CURS_RIGHT:
	    edit_cursor_move (edit, 1);
	    break;
	case CURS_LEFT:
	    edit_cursor_move (edit, -1);
	    break;
	case BACKSPACE:
	    edit_backspace (edit);
	    break;
	case ACT_DELETE:
	    edit_delete (edit);
	    break;
	case COLUMN_ON:
	    column_highlighting = 1;
	    break;
	case COLUMN_OFF:
	    column_highlighting = 0;
	    break;
        case INSERT_BEHIND:
	    edit_insert (edit, ac.param);
	    break;
        case INSERT_AHEAD:
	    edit_insert_ahead (edit, ac.param);
            break;
        case MARK_1:
	    edit->mark1 = ac.param;
	    edit->column1 = edit_move_forward3 (edit, edit_bol (edit, edit->mark1), 0, edit->mark1);
	    break;
        case MARK_2:
	    edit->mark2 = ac.param;
	    edit->column2 = edit_move_forward3 (edit, edit_bol (edit, edit->mark2), 0, edit->mark2);
            break;
        case REPEAT_COMMAND:
            abort ();   /* not possible */
            break;
        case KEY_PRESS:
            if (edit->start_display > ac.param) {
	        edit->start_line -= edit_count_lines (edit, ac.param, edit->start_display);
	        edit->force |= REDRAW_PAGE;
            } else if (edit->start_display < ac.param) {
	        edit->start_line += edit_count_lines (edit, edit->start_display, ac.param);
	        edit->force |= REDRAW_PAGE;
            }
            edit->start_display = ac.param;	/* see push and pop above */
            edit_update_curs_row (edit);
	    edit->force |= REDRAW_PAGE;
            goto done_undo;
	}

	edit->force |= REDRAW_PAGE;
    }

  done_undo:;
    push_action_disabled = 0;
}

static void edit_delete_to_line_end (WEdit * edit)
{E_
    for (;;) {
	if (edit_get_byte (edit, edit->curs1) == '\n')
	    break;
	if (!edit->curs2)
	    break;
	edit_delete (edit);
    }
}

static void edit_delete_to_line_begin (WEdit * edit)
{E_
    for (;;) {
	if (edit_get_byte (edit, edit->curs1 - 1) == '\n')
	    break;
	if (!edit->curs1)
	    break;
	edit_backspace (edit);
    }
}

void edit_delete_line (WEdit * edit)
{E_
    int c;
    do {
	c = edit_delete (edit);
    } while (c != '\n' && c);
    do {
	c = edit_backspace (edit);
    } while (c != '\n' && c);
    if (c)
	edit_insert (edit, '\n');
}

static void insert_spaces_tab (WEdit * edit, int half)
{E_
    int i;
    edit_update_curs_col (edit);
    i = ((edit->curs_col / (option_tab_spacing * space_width / (half + 1))) + 1) * (option_tab_spacing * space_width / (half + 1)) - edit->curs_col;
    while (i > 0) {
	edit_insert (edit, ' ');
	i -= space_width;
    }
}

static int is_aligned_on_a_tab (WEdit * edit)
{E_
    edit_update_curs_col (edit);
    if ((edit->curs_col % (TAB_SIZE * space_width)) && edit->curs_col % (TAB_SIZE * space_width) != (HALF_TAB_SIZE * space_width))
	return 0;		/* not alligned on a tab */
    return 1;
}

static int right_of_four_spaces (WEdit *edit)
{E_
    int i, ch = 0;
    for (i = 1; i <= HALF_TAB_SIZE; i++)
	ch |= edit_get_byte (edit, edit->curs1 - i);
    if (ch == ' ')
	return is_aligned_on_a_tab (edit);
    return 0;
}

static int left_of_four_spaces (WEdit *edit)
{E_
    int i, ch = 0;
    for (i = 0; i < HALF_TAB_SIZE; i++)
	ch |= edit_get_byte (edit, edit->curs1 + i);
    if (ch == ' ')
	return is_aligned_on_a_tab (edit);
    return 0;
}

int edit_indent_width (WEdit * edit, long p)
{E_
    long q = p;
    while (strchr ("\t ", edit_get_byte (edit, q)) && q < edit->last_byte - 1)	/* move to the end of the leading whitespace of the line */
	q++;
    return edit_move_forward3 (edit, p, 0, q);	/* count the number of columns of indentation */
}

void edit_insert_indent (WEdit * edit, int indent)
{E_
#ifndef MIDNIGHT
    indent /= space_width;
#endif
    if (!option_fill_tabs_with_spaces) {
	while (indent >= TAB_SIZE) {
	    edit_insert (edit, '\t');
	    indent -= TAB_SIZE;
	}
    }
    while (indent-- > 0)
	edit_insert (edit, ' ');
}

void edit_auto_indent (WEdit * edit, int extra, int no_advance)
{E_
    long p;
    int indent;
    p = edit->curs1;
    while (isspace (edit_get_byte (edit, p - 1)) && p > 0)	/* move back/up to a line with text */
	p--;
    indent = edit_indent_width (edit, edit_bol (edit, p));
    if (edit->curs_col < indent && no_advance)
	indent = edit->curs_col;
    edit_insert_indent (edit, indent + (option_fake_half_tabs ? HALF_TAB_SIZE : TAB_SIZE) * space_width * extra);
}

static void edit_double_newline (WEdit * edit)
{E_
    edit_insert (edit, '\n');
    if (edit_get_byte (edit, edit->curs1) == '\n')
	return;
    if (edit_get_byte (edit, edit->curs1 - 2) == '\n')
	return;
    edit->force |= REDRAW_PAGE;
    edit_insert (edit, '\n');
}

void edit_tab_cmd (WEdit * edit)
{E_
    int i;

    if (option_fake_half_tabs) {
	if (is_in_indent (edit)) {
	    /*insert a half tab (usually four spaces) unless there is a
	       half tab already behind, then delete it and insert a 
	       full tab. */
	    if (!option_fill_tabs_with_spaces && right_of_four_spaces (edit)) {
		for (i = 1; i <= HALF_TAB_SIZE; i++)
		    edit_backspace (edit);
		edit_insert (edit, '\t');
	    } else {
		insert_spaces_tab (edit, 1);
	    }
	    return;
	}
    }
    if (option_fill_tabs_with_spaces) {
	insert_spaces_tab (edit, 0);
    } else {
	edit_insert (edit, '\t');
    }
    return;
}

void format_paragraph (WEdit * edit, int force);

static void check_and_wrap_line (WEdit * edit)
{E_
    int curs, c;
    if (!option_typewriter_wrap)
	return;
    edit_update_curs_col (edit);
#ifdef MIDNIGHT
    if (edit->curs_col < option_word_wrap_line_length)
	return;
#else
    CPushFont ("editor", 0);
    c = FONT_MEAN_WIDTH;
    CPopFont ();
    if (edit->curs_col < option_word_wrap_line_length * c)
	return;
#endif
    curs = edit->curs1;
    for (;;) {
	curs--;
	c = edit_get_byte (edit, curs);
	if (c == '\n' || curs <= 0) {
	    edit_insert (edit, '\n');
	    return;
	}
	if (c == ' ' || c == '\t') {
	    int current = edit->curs1;
	    edit_cursor_move (edit, curs - edit->curs1 + 1);
	    edit_insert (edit, '\n');
	    edit_cursor_move (edit, current - edit->curs1 + 1);
	    return;
	}
    }
}

void edit_execute_macro (WEdit * edit, struct macro_rec *macro);

/* either command or char_for_insertion must be passed as -1 */
int edit_execute_cmd (WEdit * edit, int command, CStr char_for_insertion);

#ifdef MIDNIGHT
int edit_translate_key (WEdit * edit, unsigned int x_keycode, long x_key, int x_state, int *cmd, long *ch)
{E_
    int command = -1;
    long char_for_insertion = -1;

#include "edit_key_translator.c"

    *cmd = command;
    *ch = char_for_insertion;

    if((command == -1 || command == 0) && char_for_insertion == -1)  /* unchanged, key has no function here */
	return 0;
    return 1;
}
#endif

void edit_push_key_press (WEdit * edit)
{E_
    edit_push_action (edit, KEY_PRESS,  edit->start_display);
    if (edit->mark2 == -1)
	edit_push_action (edit, MARK_1, edit->mark1);
}

/* this find the matching bracket in either direction, and sets edit->bracket */
static long edit_get_bracket (WEdit * edit, int in_screen, unsigned long furthest_bracket_search)
{E_
    const char *b = "{}{[][()(", *p = 0;
    int i = 1, a, inc = -1, c, d, n = 0;
    unsigned long j = 0;
    long q;
    edit_update_curs_row (edit);
    c = edit_get_byte (edit, edit->curs1);
    if (c)
        p = strchr (b, c);
/* no limit */
    if (!furthest_bracket_search)
	furthest_bracket_search--;
/* not on a bracket at all */
    if (!p)
	return -1;
/* the matching bracket */
    d = p[1];
/* going left or right? */
    if (strchr ("{[(", c))
	inc = 1;
    for (q = edit->curs1 + inc;; q += inc) {
/* out of buffer? */
	if (q >= edit->last_byte || q < 0)
	    break;
	a = edit_get_byte (edit, q);
/* don't want to eat CPU */
	if (j++ > furthest_bracket_search)
	    break;
/* out of screen? */
	if (in_screen) {
	    if (q < edit->start_display)
		break;
/* count lines if searching downward */
	    if (inc > 0 && a == '\n')
		if (n++ >= edit->num_widget_lines - edit->curs_row)	/* out of screen */
		    break;
	}
/* count bracket depth */
	i += (a == c) - (a == d);
/* return if bracket depth is zero */
	if (!i)
	    return q;
    }
/* no match */
    return -1;
}

static long last_bracket = -1;

static void edit_find_bracket (WEdit * edit)
{E_
    if (option_find_bracket) {
	edit->bracket = edit_get_bracket (edit, 1, 10000);
	if (last_bracket != edit->bracket)
	    edit->force |= REDRAW_PAGE;
	last_bracket = edit->bracket;
    }
}

static void edit_goto_matching_bracket (WEdit *edit)
{E_
    long q;
    q = edit_get_bracket (edit, 0, 0);
    if (q < 0)
	return;
    edit->bracket = edit->curs1;
    edit->force |= REDRAW_PAGE;
    edit_cursor_move (edit, q - edit->curs1);
}

/* this executes a command as though the user initiated it through a key press. */
/* callback with WIDGET_KEY as a message calls this after translating the key
   press */
/* this can be used to pass any command to the editor. Same as sendevent with
   msg = WIDGET_COMMAND and par = command  except the screen wouldn't update */
/* one of command or char_for_insertion must be passed as -1 */
/* commands are executed, and char_for_insertion is inserted at the cursor */
/* returns 0 if the command is a macro that was not found, 1 otherwise */
int edit_execute_key_command (WEdit * edit, int command, CStr char_for_insertion)
{E_
    int r;
    if (command == CK_Begin_Record_Macro) {
        edit_clear_macro (&edit->macro);
	edit->macro.macro_i = 0;
	edit->force |= REDRAW_CHAR_ONLY | REDRAW_LINE;
	return command;
    }
    if (command == CK_End_Record_Macro && edit->macro.macro_i != -1) {
	edit->force |= REDRAW_COMPLETELY;
	edit_save_macro_cmd (edit, &edit->macro);
	edit_clear_macro (&edit->macro);
	return command;
    }
    if (edit->macro.macro_i >= 0) {
        if (edit->macro.macro_i < MAX_MACRO_LENGTH - 1) {
            edit->macro.macro[edit->macro.macro_i].command = command;
            edit->macro.macro[edit->macro.macro_i].ch = /* command >= 0 CStr_dupstr ("", 0) : */ CStr_dupstr (char_for_insertion);
	    edit->macro.macro_i++;
        }
    }
/* record the beginning of a set of editing actions initiated by a key press */
    if (command != CK_Undo)
	edit_push_key_press (edit);

    r = edit_execute_cmd (edit, command, char_for_insertion);
#ifdef GTK
    if (edit->stopped && edit->widget->destroy_me) {
	(*edit->widget->destroy_me) (edit->widget->destroy_me_user_data);
	return 0;
    }
#endif
    if (column_highlighting)
	edit->force |= REDRAW_PAGE;

    return r;
}

#ifdef MIDNIGHT
static const char *shell_cmd[] = SHELL_COMMANDS_i
#else
static void (*user_commamd) (WEdit *, int) = 0;
void edit_set_user_command (void (*func) (WEdit *, int))
{E_
    user_commamd = func;
}

#endif

void edit_mail_dialog (WEdit * edit);
void edit_indent_left_right_paragraph (WEdit * edit);

int edit_is_movement_command(int command)
{E_
    switch (command) {
    case CK_Begin_Page:
    case CK_End_Page:
    case CK_Begin_Page_Highlight:
    case CK_End_Page_Highlight:
    case CK_Word_Left:
    case CK_Word_Right:
    case CK_Up:
    case CK_Down:
    case CK_Word_Left_Highlight:
    case CK_Word_Right_Highlight:
    case CK_Up_Highlight:
    case CK_Down_Highlight:
    case CK_Left:
    case CK_Right:
    case CK_Left_Highlight:
    case CK_Right_Highlight:
    case CK_Scroll_Up:
    case CK_Scroll_Up_Highlight:
    case CK_Scroll_Down:
    case CK_Scroll_Down_Highlight:
    case CK_Quarter_Page_Up:
    case CK_Quarter_Page_Down:
    case CK_Page_Up:
    case CK_Page_Up_Highlight:
    case CK_Page_Down:
    case CK_Page_Down_Highlight:
    case CK_Paragraph_Up:
    case CK_Paragraph_Up_Highlight:
    case CK_Paragraph_Down:
    case CK_Paragraph_Down_Highlight:
    case CK_Home:
    case CK_Home_Highlight:
    case CK_End:
    case CK_End_Highlight:
    case CK_Find:
    case CK_Find_Again:
    case CK_Goto:
    case CK_Match_Bracket:
    case CK_Toggle_Bookmark:
    case CK_Flush_Bookmarks:
    case CK_Next_Bookmark:
    case CK_Prev_Bookmark:
    case CK_Beginning_Of_Text:
    case CK_Beginning_Of_Text_Highlight:
    case CK_End_Of_Text:
    case CK_End_Of_Text_Highlight:
/*    case CK_Selection_History: */
    case CK_Toggle_Insert:
    case CK_Mark:
    case CK_Column_Mark:
    case CK_Unmark:
    case CK_Copy:
    case CK_XStore:
	return 1;
    }
    return 0;
}

/* 
   This executes a command at a lower level than macro recording.
   It also does not push a key_press onto the undo stack. This means
   that if it is called many times, a single undo command will undo
   all of them. It also does not check for the Undo command.
   Returns 0 if the command is a macro that was not found, 1
   otherwise.
 */
int edit_execute_cmd (WEdit * edit, int command, CStr char_for_insertion)
{E_
    int result = 1;
    edit->force |= REDRAW_LINE;
    if (edit->found_len || column_highlighting)
/* the next key press will unhighlight the found string, so update whole page */
	edit->force |= REDRAW_PAGE;

    if (command / 100 == 6) {	/* a highlight command like shift-arrow */
	column_highlighting = 0;
	if (!edit->highlight || (edit->mark2 != -1 && edit->mark1 != edit->mark2)) {
	    edit_mark_cmd (edit, 1);	/* clear */
	    edit_mark_cmd (edit, 0);	/* marking on */
	}
	edit->highlight = 1;
    } else {			/* any other command */
	if (edit->highlight)
	    edit_mark_cmd (edit, 0);	/* clear */
	edit->highlight = 0;
    }

/* first check for undo */
    if (command == CK_Undo) {
	edit_do_undo (edit);
	edit->found_len = 0;
	edit->prev_col = edit_get_col (edit);
	edit->search_start = edit->curs1;
	return 1;
    }
/* An ordinary key press */
    if (char_for_insertion.len > 0) {
        long start_mark, end_mark;
        int i;
        if (option_typing_replaces_selection && !eval_marks (edit, &start_mark, &end_mark)) {
            edit_block_delete (edit);
	} else if (edit->overwrite) {
	    if (edit_get_byte (edit, edit->curs1) != '\n')
		edit_delete_wide (edit);
	}
        for (i = 0; i < char_for_insertion.len; i++)
            edit_insert (edit, (unsigned char) char_for_insertion.data[i]);
	if (option_auto_para_formatting) {
	    format_paragraph (edit, 0);
	    edit->force |= REDRAW_PAGE;
	} else
	    check_and_wrap_line (edit);
	edit->found_len = 0;
	edit->prev_col = edit_get_col (edit);
	edit->search_start = edit->curs1;
	edit_find_bracket (edit);
	edit_check_spelling (edit);
	return 1;
    }
    switch (command) {
    case CK_Begin_Page:
    case CK_End_Page:
    case CK_Begin_Page_Highlight:
    case CK_End_Page_Highlight:
    case CK_Word_Left:
    case CK_Word_Right:
    case CK_Up:
    case CK_Down:
    case CK_Word_Left_Highlight:
    case CK_Word_Right_Highlight:
    case CK_Up_Highlight:
    case CK_Down_Highlight:
	if (edit->mark2 == -1)
	    break;		/*marking is following the cursor: may need to highlight a whole line */
    case CK_Left:
    case CK_Right:
    case CK_Left_Highlight:
    case CK_Right_Highlight:
	edit->force |= REDRAW_CHAR_ONLY;
    }

/* basic cursor key commands */
    switch (command) {
        long start_mark, end_mark;
    case CK_BackSpace:
        if (option_typing_replaces_selection && !eval_marks (edit, &start_mark, &end_mark)) {
            edit_block_delete (edit);
            break;
        }
	if (option_backspace_through_tabs && is_in_indent (edit)) {
	    while (edit_get_byte (edit, edit->curs1 - 1) != '\n'
		   && edit->curs1 > 0)
		edit_backspace (edit);
	    break;
	} else {
	    if (is_in_indent (edit)) {
		edit_backspace_tab (edit, 0);
		break;
	    }
	}
	edit_backspace_wide (edit);
	break;
    case CK_Delete:
        if (option_typing_replaces_selection && !eval_marks (edit, &start_mark, &end_mark)) {
            edit_block_delete (edit);
            break;
        }
	if (option_fake_half_tabs) {
	    int i;
	    if (is_in_indent (edit) && left_of_four_spaces (edit)) {
		for (i = 1; i <= HALF_TAB_SIZE; i++)
		    edit_delete (edit);
		break;
	    }
	}
	edit_delete_wide (edit);
	break;
    case CK_Delete_Word_Left:
	edit_left_delete_word (edit);
	break;
    case CK_Delete_Word_Right:
	edit_right_delete_word (edit);
	break;
    case CK_Delete_Line:
	edit_delete_line (edit);
	break;
    case CK_Delete_To_Line_End:
	edit_delete_to_line_end (edit);
	break;
    case CK_Delete_To_Line_Begin:
	edit_delete_to_line_begin (edit);
	break;
    case CK_Enter:
	if (option_auto_para_formatting) {
	    edit_double_newline (edit);
	    if (option_return_does_auto_indent)
		edit_auto_indent (edit, 0, 1);
	    format_paragraph (edit, 0);
	} else if (option_return_does_auto_indent) {
	    edit_insert (edit, '\n');
	    edit_auto_indent (edit, 0, 1);
	} else {
	    edit_insert (edit, '\n');
	}
	break;
    case CK_Return:
	edit_insert (edit, '\n');
	break;

    case CK_Quarter_Page_Up:
	edit_move_up (edit, edit->num_widget_lines / 4, 1);
	break;
    case CK_Quarter_Page_Down:
	edit_move_down (edit, edit->num_widget_lines / 4, 1);
	break;

    case CK_Page_Up:
    case CK_Page_Up_Highlight:
	edit_move_up (edit, edit->num_widget_lines - 1, 1);
	break;
    case CK_Page_Down:
    case CK_Page_Down_Highlight:
	edit_move_down (edit, edit->num_widget_lines - 1, 1);
	break;
    case CK_Left:
    case CK_Left_Highlight:
	if (option_fake_half_tabs) {
	    if (is_in_indent (edit) && right_of_four_spaces (edit)) {
		edit_cursor_move (edit, -HALF_TAB_SIZE);
		edit->force &= (0xFFF - REDRAW_CHAR_ONLY);
		break;
	    }
	}
	edit_cursor_move (edit, -1);
	edit_wide_char_align_left (edit);
	break;
    case CK_Right:
    case CK_Right_Highlight:
	if (option_fake_half_tabs) {
	    if (is_in_indent (edit) && left_of_four_spaces (edit)) {
		edit_cursor_move (edit, HALF_TAB_SIZE);
		edit->force &= (0xFFF - REDRAW_CHAR_ONLY);
		break;
	    }
	}
	edit_cursor_move (edit, 1);
	break;
    case CK_Begin_Page:
    case CK_Begin_Page_Highlight:
	edit_begin_page (edit);
	break;
    case CK_End_Page:
    case CK_End_Page_Highlight:
	edit_end_page (edit);
	break;
    case CK_Word_Left:
    case CK_Word_Left_Highlight:
	edit_left_word_move_cmd (edit);
	break;
    case CK_Word_Right:
    case CK_Word_Right_Highlight:
	edit_right_word_move_cmd (edit);
	break;
    case CK_Up:
    case CK_Up_Highlight:
	edit_move_up (edit, 1, 0);
	break;
    case CK_Down:
    case CK_Down_Highlight:
	edit_move_down (edit, 1, 0);
	break;
    case CK_Paragraph_Up:
    case CK_Paragraph_Up_Highlight:
	edit_move_up_paragraph (edit, 0);
	break;
    case CK_Paragraph_Down:
    case CK_Paragraph_Down_Highlight:
	edit_move_down_paragraph (edit, 0);
	break;
    case CK_Scroll_Up:
    case CK_Scroll_Up_Highlight:
	edit_move_up (edit, 1, 1);
	break;
    case CK_Scroll_Down:
    case CK_Scroll_Down_Highlight:
	edit_move_down (edit, 1, 1);
	break;
    case CK_Home:
    case CK_Home_Highlight:
	edit_cursor_to_bol (edit);
	break;
    case CK_End:
    case CK_End_Highlight:
	edit_cursor_to_eol (edit);
	break;

    case CK_Tab:
	edit_tab_cmd (edit);
	if (option_auto_para_formatting) {
	    format_paragraph (edit, 0);
	    edit->force |= REDRAW_PAGE;
	} else
	    check_and_wrap_line (edit);
	break;

    case CK_Toggle_Insert:
	edit->overwrite = (edit->overwrite == 0);
#ifndef MIDNIGHT
#ifdef GTK
/* *** */
#else
	CSetCursorColor (edit->overwrite ? color_palette (24) : color_palette (19));
#endif
#endif
	break;

    case CK_Mark:
	if (edit->mark2 >= 0) {
	    if (column_highlighting)
		edit_push_action (edit, COLUMN_ON, 0);
	    column_highlighting = 0;
	}
	edit_mark_cmd (edit, 0);
	break;
    case CK_Column_Mark:
	if (!column_highlighting)
	    edit_push_action (edit, COLUMN_OFF, 0);
	column_highlighting = 1;
	edit_mark_cmd (edit, 0);
	break;
    case CK_Unmark:
	if (column_highlighting)
	    edit_push_action (edit, COLUMN_ON, 0);
	column_highlighting = 0;
	edit_mark_cmd (edit, 1);
	break;

    case CK_Toggle_Bookmark:
	book_mark_clear (edit, edit->curs_line, BOOK_MARK_FOUND_COLOR);
	if (book_mark_query_color (edit, edit->curs_line, BOOK_MARK_COLOR))
	    book_mark_clear (edit, edit->curs_line, BOOK_MARK_COLOR);
	else
	    book_mark_insert (edit, edit->curs_line, BOOK_MARK_COLOR, 9, 0, 0);
	break;
    case CK_Flush_Bookmarks:
	book_mark_flush (edit, BOOK_MARK_COLOR);
	book_mark_flush (edit, BOOK_MARK_FOUND_COLOR);
	edit->force |= REDRAW_PAGE;
	break;
    case CK_Next_Bookmark:
	if (edit->book_mark) {
	    struct _book_mark *p;
	    p = (struct _book_mark *) book_mark_find (edit, edit->curs_line);
	    if (p->next) {
		p = p->next;
		if (p->line >= edit->start_line + edit->num_widget_lines || p->line < edit->start_line)
		    edit_move_display (edit, p->line - edit->num_widget_lines / 2);
		edit_move_to_line (edit, p->line);
	    }
	}
	break;
    case CK_Prev_Bookmark:
	if (edit->book_mark) {
	    struct _book_mark *p;
	    p = (struct _book_mark *) book_mark_find (edit, edit->curs_line);
	    while (p->line == edit->curs_line)
		if (p->prev)
		    p = p->prev;
	    if (p->line >= 0) {
		if (p->line >= edit->start_line + edit->num_widget_lines || p->line < edit->start_line)
		    edit_move_display (edit, p->line - edit->num_widget_lines / 2);
		edit_move_to_line (edit, p->line);
	    }
	}
	break;

    case CK_Beginning_Of_Text:
    case CK_Beginning_Of_Text_Highlight:
	edit_move_to_top (edit);
	break;
    case CK_End_Of_Text:
    case CK_End_Of_Text_Highlight:
	edit_move_to_bottom (edit);
	break;

    case CK_Copy:
	edit_block_copy_cmd (edit);
	break;
    case CK_Remove:
	edit_block_delete_cmd (edit);
	break;
    case CK_Move:
	edit_block_move_cmd (edit);
	break;

    case CK_XStore:
	edit_copy_to_X_buf_cmd (edit);
	break;
    case CK_XCut:
	edit_cut_to_X_buf_cmd (edit);
	break;
    case CK_XPaste:
	edit_paste_from_X_buf_cmd (edit);
	break;
    case CK_Selection_History:
	edit_paste_from_history (edit);
	break;

    case CK_Save_As:
#ifndef MIDNIGHT
/*	if (COptionsOf (edit->widget) & EDITOR_NO_FILE) */
	if (edit->widget->options & EDITOR_NO_FILE)
	    break;
#endif
	edit_save_as_cmd (edit);
	break;
    case CK_Save:
#ifndef MIDNIGHT
	if (COptionsOf (edit->widget) & EDITOR_NO_FILE)
	    break;
#endif
	edit_save_confirm_cmd (edit);
	break;
    case CK_Load:
#ifndef MIDNIGHT
	if (COptionsOf (edit->widget) & EDITOR_NO_FILE)
	    break;
#endif
	edit_load_cmd (edit);
	break;
    case CK_Save_Block:
	edit_save_block_cmd (edit);
	break;
    case CK_Insert_File:
	edit_insert_file_cmd (edit);
	break;

    case CK_Find:
	edit_search_cmd (edit, 0);
	break;
    case CK_Find_Again:
	edit_search_cmd (edit, 1);
	break;
    case CK_Replace:
	edit_replace_cmd (edit, 0);
	break;
    case CK_Replace_Again:
	edit_replace_cmd (edit, 1);
	break;

    case CK_Exit:
	edit_quit_cmd (edit);
	break;
    case CK_New:
	edit_new_cmd (edit);
	break;

    case CK_Help:
	edit_help_cmd (edit);
	break;

    case CK_Refresh:
	edit_refresh_cmd (edit);
	break;

    case CK_Date:{
	    time_t t;
#ifdef HAVE_STRFTIME
	    char s[1024];
#endif
	    time (&t);
#ifdef HAVE_STRFTIME
	    strftime (s, 1024, "%c", localtime (&t));
	    edit_printf (edit, s);
#else
	    edit_printf (edit, ctime (&t));
#endif
	    edit->force |= REDRAW_PAGE;
	    break;
	}
    case CK_Goto:
	edit_goto_cmd (edit);
	break;
    case CK_Paragraph_Format:
	format_paragraph (edit, 1);
	edit->force |= REDRAW_PAGE;
	break;
    case CK_Paragraph_Indent_Mode:
	edit_indent_left_right_paragraph (edit);
	edit->force |= REDRAW_PAGE;
	break;
    case CK_Delete_Macro:
	edit_delete_macro_cmd (edit);
	break;
    case CK_Match_Bracket:
	edit_goto_matching_bracket (edit);
	break;
#ifdef MIDNIGHT
    case CK_Sort:
	edit_sort_cmd (edit);
	break;
    case CK_Mail:
	edit_mail_dialog (edit);
	break;
#endif

/* These commands are not handled and must be handled by the user application */
#ifndef MIDNIGHT
    case CK_Sort:
    case CK_Mail:
    case CK_Find_File:
    case CK_Ctags:
    case CK_Terminal:
    case CK_8BitTerminal:
    case CK_Terminal_App:
#endif
    case CK_Complete:
    case CK_Cancel:
    case CK_Save_Desktop:
    case CK_New_Window:
    case CK_Jump_To_File:
    case CK_Cycle:
    case CK_Save_And_Quit:
    case CK_Check_Save_And_Quit:
    case CK_Run_Another:
    case CK_Insert_Unicode:
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
	result = 0;
	break;
    case CK_Menu:
#ifdef GTK
	if (edit->widget->menubar)
	    gtk_menu_popup (GTK_MENU(
		(GTK_MENU_ITEM (g_list_nth_data (GTK_MENU_BAR (edit->widget->menubar)->menu_shell.children, 0)))->submenu
	    ), 0, 0, 0, 0, 1, 0);
	result = 1;
#else
	result = 0;
#endif
	break;
    }

#ifdef MIDNIGHT
/* CK_Pipe_Block */
    if ((command / 1000) == 1)	/* a shell command */
	edit_block_process_cmd (edit, shell_cmd[command - 1000], 1);
/* ... */
#else
    if (IS_USER_COMMAND (command))	/* a user defined command */
	if (user_commamd)
	    (*user_commamd) (edit, command & 0xFFFF);
    if (IS_MACRO_COMMAND (command)) {	/* a macro command */
	struct macro_rec m;
	if ((result = edit_load_macro_cmd (edit, &m, command & 0xFFFF))) {
	    edit_execute_macro (edit, &m);
	    edit_clear_macro (&m);
	}
    }
#endif

/* any movement key could end up inside a wide char - fix */
    if (command / 100 == 6 || (command > 0 && command < 100))
	edit_wide_char_align (edit);

/* keys which must set the col position, and the search vars */
    switch (command) {
    case CK_Find:
    case CK_Find_Again:
    case CK_Replace:
    case CK_Replace_Again:
	edit->prev_col = edit_get_col (edit);
	return 1;
	break;
    case CK_Up:
    case CK_Up_Highlight:
    case CK_Down:
    case CK_Down_Highlight:
    case CK_Quarter_Page_Up:
    case CK_Quarter_Page_Down:
    case CK_Page_Up:
    case CK_Page_Up_Highlight:
    case CK_Page_Down:
    case CK_Page_Down_Highlight:
    case CK_Beginning_Of_Text:
    case CK_Beginning_Of_Text_Highlight:
    case CK_End_Of_Text:
    case CK_End_Of_Text_Highlight:
    case CK_Paragraph_Up:
    case CK_Paragraph_Up_Highlight:
    case CK_Paragraph_Down:
    case CK_Paragraph_Down_Highlight:
    case CK_Scroll_Up:
    case CK_Scroll_Up_Highlight:
    case CK_Scroll_Down:
    case CK_Scroll_Down_Highlight:
	edit->search_start = edit->curs1;
	edit->found_len = 0;
	edit_find_bracket (edit);
	edit_check_spelling (edit);
	return 1;
	break;
    default:
	edit->found_len = 0;
	edit->prev_col = edit_get_col (edit);
	edit->search_start = edit->curs1;
    }
    edit_find_bracket (edit);
    edit_check_spelling (edit);

    if (option_auto_para_formatting) {
	switch (command) {
	case CK_BackSpace:
	case CK_Delete:
	case CK_Delete_Word_Left:
	case CK_Delete_Word_Right:
	case CK_Delete_To_Line_End:
	case CK_Delete_To_Line_Begin:
	    format_paragraph (edit, 0);
	    edit->force |= REDRAW_PAGE;
	}
    }
    return result;
}


/* returns 0 if command is a macro that was not found, 1 otherwise */
int edit_execute_command (WEdit * edit, int command, ...)
{E_
    int r;
    r = edit_execute_cmd (edit, command, CStr_const ("", 0));
    edit_update_screen (edit);
    return r;
}

void edit_execute_macro (WEdit * edit, struct macro_rec *macro)
{E_
    static int call_depth = 0;
    int i;
    call_depth++;
    if (call_depth < 20) {
        edit->force |= REDRAW_PAGE;
        for (i = 0; i < macro->macro_i; i++)
            edit_execute_cmd (edit, macro->macro[i].command, macro->macro[i].ch);
        edit_update_screen (edit);
    }
    call_depth--;
}

