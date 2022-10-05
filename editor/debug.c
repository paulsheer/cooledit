/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* debug.c - interactive debugger
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_GRP_H
#    include <grp.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "stringtools.h"
#include "coolwidget.h"
#include "edit.h"
#include "editcmddef.h"
#include "loadfile.h"
#include "editoptions.h"
#include "cmdlineopt.h"
#include "shell.h"
#include "widget/pool.h"
#include "find.h"
#include "rxvt/rxvtexport.h"
#include "debug.h"

extern struct look *look;

#define DEBUG_TIME_OUT		15
#define DEBUG_BREAKPOINT_COLOR	18
#define DEBUG_CURRENT_COLOR	6

#define ACTION_PROMPT		1
#define ACTION_MESSAGE		2
#define ACTION_BREAKPOINT	3
#define ACTION_INFO_SOURCE	4
#define ACTION_BACKTRACE	5
#define ACTION_RUNNING		6
#define ACTION_STEP		7
#define ACTION_INFO_PROGRAM	8
#define ACTION_VERSION		9
#define ACTION_CONDITION	10
#define ACTION_QUIT		11
#define ACTION_SHOW		12
#define ACTION_UNTIL		13
#define ACTION_VARIABLE		14
#define ACTION_LASTVAR		15
#define ACTION_WATCH		16
#define ACTION_CONFIRM_BREAKPOINT	17
#define ACTION_DENY_BREAKPOINT	18
#define ACTION_BREAKPOINT_CLEAR	19

#define GDB_VERSION1		"GNU gdb 4.17"
#define GDB_VERSION2		"GNU gdb 4.18"
#define GDB_VERSION3		"GNU gdb 5.0"

#define CLEAR_PROMPT		"1834559546\n(gdb) "
#define CLEAR_FLUSH		"p 1834559546\n"

static void debug_read_callback (int fd, fd_set * reading, fd_set * writing, fd_set * error, void *data);
static void debug_write_callback (int fd, fd_set * reading, fd_set * writing, fd_set * error, void *data);

#define MAX_DEBUG_CMD_LEN	MAX_PATH_LEN
#define MAX_QUEUED_COMMANDS	100
#define MAX_VARIABLES		60

typedef struct struct_debug {
    int x, y, r;
    Window w;
    int in;
    int out;
    char *prompt;
    char *get_line;
    char *get_source;
    char *get_backtrace;
    struct debug_query {
	char *query;
	char *response;
    } query[30];
    char *progname;
    char *debugger;
    char *args;
    pid_t pid;
    pid_t child;
    struct _command {
	int action;
	char *command;
	char *editor;
	int line;
    } command[MAX_QUEUED_COMMANDS];
    struct _variable {
	char *name;
	char *output;
	int watch;
	int changed;
    } variable[MAX_VARIABLES];
    int last_variable_displayed;
    int n_commands;
    int action;
    POOL *pool;
    int line;
    char *file;
    char *condition;
    int show_output, stop_at_main, show_on_stdout;
/*    int ignore_SIGTTOU; */
    pid_t xterm_pid;
    int break_point_line;
    char *break_point_editor;
} Debug;

extern CWidget *edit[50];
extern int current_edit;
extern int last_edit;

static int debug_query_yes_no (const char *heading, const char *str)
{E_
    return CQueryDialog (0, 20, 20, heading, str, "Yes", "No", NULL) == 0;
}

static void debug_message (Debug * d, const char *heading, const char *str, int options)
{E_
    if (d->pid) {
	CRemoveWatch (d->in, debug_write_callback, WATCH_WRITING);
	CRemoveWatch (d->out, debug_read_callback, WATCH_READING);
    }
    CMessageDialog (0, 20, 20, options, heading, "%s", str);
    if (d->pid) {
	if (d->n_commands && !d->action)
	    CAddWatch (d->in, debug_write_callback, WATCH_WRITING, 0);
	CAddWatch (d->out, debug_read_callback, WATCH_READING, 0);
    }
}

static void debug_error2 (Debug * d, const char *par1, const char *par2)
{E_
    char *t;
    if (d->pid) {
	CRemoveWatch (d->in, debug_write_callback, WATCH_WRITING);
	CRemoveWatch (d->out, debug_read_callback, WATCH_READING);
    }
    t = malloc (strlen (par1) + strlen (par2) + 80);
    if (*par2)
	sprintf (t, " %s: %s ", par1, par2);
    else
	sprintf (t, " %s ", par1);
    CErrorDialogTxt (0, 0, 0, _ (" Debug Message "), "%s", t);
    free (t);
    if (d->pid) {
	if (d->n_commands && !d->action)
	    CAddWatch (d->in, debug_write_callback, WATCH_WRITING, 0);
	CAddWatch (d->out, debug_read_callback, WATCH_READING, 0);
    }
}

#define debug_error1(d, x)		debug_error2(d, x, "")

static void xdebug_cursor_bookmark_flush (void)
{E_
    int i;
    for (i = 0; i < last_edit; i++)
	if (edit[i]->editor)
	    book_mark_flush (edit[i]->editor, DEBUG_CURRENT_COLOR);
}

static void xdebug_append_command (Debug * d, char *s, int action, char *editor, int line)
{E_
    if (d->n_commands < MAX_QUEUED_COMMANDS - 2) {
	if (action == ACTION_BREAKPOINT_CLEAR) {
	    const char *t;
	    char *p;
	    t = CLEAR_FLUSH;
	    p = (char *) malloc (strlen (s) + strlen (t) + 1);
	    strcpy (p, s);
	    strcat (p, t);
	    d->command[d->n_commands].command = p;
	} else {
	    d->command[d->n_commands].command = (char *) strdup (s);
	}
	d->command[d->n_commands].action = action;
	if (editor)
	    d->command[d->n_commands].editor = (char *) strdup (editor);
	else
	    d->command[d->n_commands].editor = 0;
	d->command[d->n_commands].line = line;
	d->n_commands++;
	if (!d->action)
	    CAddWatch (d->in, debug_write_callback, WATCH_WRITING, 0);
    }
}

static void xdebug_flush_commands (Debug * d)
{E_
    int i;
    for (i = 0; i < d->n_commands; i++) {
	if (d->command[i].editor)
	    free (d->command[i].editor);
	free (d->command[i].command);
	d->command[i].command = 0;
    }
    d->action = 0;
    d->n_commands = 0;
    d->last_variable_displayed = 0;
    CRemoveWatch (d->in, debug_write_callback, WATCH_WRITING);
}


/* {{{ variables */

static void xdebug_show_variables (Debug * d)
{E_
    int i;
    char s[1024];
    for (i = 0; d->variable[i].name; i++) {
	sprintf (s, "p %s\n", d->variable[i].name);
	xdebug_append_command (d, s, d->variable[i + 1].name ? ACTION_VARIABLE : ACTION_LASTVAR, 0, 0);
    }
}

static int xdebug_add_variable (Debug * d, char *s)
{E_
    int i;
    for (i = 0; d->variable[i].name; i++);
    if (i >= MAX_VARIABLES - 1) {
	debug_error1 (d, _ ("Max variables exceeded"));
	return 1;
    }
    d->variable[i].output = 0;
    d->variable[i].name = (char *) strdup (s);
    d->variable[i + 1].name = 0;
    d->variable[i + 1].output = 0;
    return 0;
}

static void xdebug_remove_variable (Debug * d, int i)
{E_
    if (d->variable[i].name) {
	free (d->variable[i].name);
	if (d->variable[i].output)
	    free (d->variable[i].output);
    }
    Cmemmove (&d->variable[i], &d->variable[i + 1], (MAX_VARIABLES - i) * sizeof (struct _variable));
}

static char **xdebug_get_line (void *data, int line, int *numfields, int *tagged)
{E_
    static char *fields[10];
    Debug *d = (Debug *) data;
    *numfields = 3;
    if (!d->variable[line].name || !d->variable[line].output)
	return 0;
    *tagged = (d->variable[line].watch != 0);
    fields[0] = d->variable[line].name;
    fields[1] = d->variable[line].changed ? "\bX" : " ";
    fields[2] = d->variable[line].output;
    fields[3] = 0;
    return fields;
}

static int xdebug_varlist_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    Debug *d;
    d = (Debug *) w->hook;
    if (c->command == CK_Delete || c->command == CK_BackSpace) {
	xdebug_remove_variable (d, w->cursor);
	CRedrawFieldedTextbox ("debug_vars", 1);
    }
    if (c->command == CK_Toggle_Insert && !d->action && !d->n_commands) {
	char s[MAX_DEBUG_CMD_LEN];
	if (d->variable[w->cursor].watch) {
	    sprintf (s, "delete %d\n", d->variable[w->cursor].watch);
	    xdebug_append_command (d, s, ACTION_WATCH, 0, w->cursor);
	} else {
	    sprintf (s, "watch %s\n", d->variable[w->cursor].name);
	    xdebug_append_command (d, s, ACTION_WATCH, 0, w->cursor);
	}
    }
    return 0;
}

static void xdebug_display_variable (Debug * d, char *message, int action)
{E_
    char *p;
    int i;
    if (*message == '$') {
	p = message;
	message = strstr (message, "= ");
	if (message)
	    message += 2;
	else
	    message = p;
    }
    for (p = message, i = 0; *p && i < 255; p++, i++) {
	if (isspace (*p))
	    *p = ' ';
	else if (!isprint ((unsigned char) *p))
	    *p = '?';
    }
    *p = '\0';
    if (d->variable[d->last_variable_displayed].output) {
	d->variable[d->last_variable_displayed].changed = strcmp (message, d->variable[d->last_variable_displayed].output);
	free (d->variable[d->last_variable_displayed].output);
    }
    d->variable[d->last_variable_displayed++].output = (char *) strdup (message);
    if (action == ACTION_LASTVAR) {
	d->last_variable_displayed = 0;
	if (CIdent ("debug_vars")) {
	    CRedrawFieldedTextbox ("debug_vars", 1);
	} else {
	    Window win;
	    CWidget *w;
	    int x, y;
	    win = CDrawMainWindow ("debug_vars.win", _ ("Variables"));
	    CGetHintPos (&x, &y);
	    CPushFont ("editor", 0);
	    w = CDrawFieldedTextbox ("debug_vars", win, x, y, FONT_MEAN_WIDTH * 50 + 7,
		FONT_PIX_PER_LINE * 15 + 6, 0, 0, xdebug_get_line, 0, d);
	    w->hook = d;
	    CSetMovement ("debug_vars", POSITION_WIDTH | POSITION_HEIGHT);
	    CAddCallback ("debug_vars", xdebug_varlist_callback);
	    CSetMovement ("debug_vars.vsc", POSITION_HEIGHT | POSITION_RIGHT);
	    CSetMovement ("debug_vars.hsc", POSITION_WIDTH | POSITION_BOTTOM);
	    CSetSizeHintPos ("debug_vars.win");
	    CMapDialog ("debug_vars.win");
	    CSetWindowResizable ("debug_vars.win", FONT_MEAN_WIDTH * 20, FONT_PIX_PER_LINE * 10, 1600, 1200);
	    CSetToolHint ("debug_vars", _ ("Use Del to delete from this list. Use Ins to\nmark variable as a watchpoint. This will cause\ndebugger to break if the variable changes."));
	    CPopFont ();
	}
    }
}

/* }}} variables */

/* returns 1 on error */
static void xdebug_break_point_special (Debug * d, char *cmd, char *editor, int line, int action)
{E_
    CWidget *w;
    if ((w = CIdent (editor))) {
	char s[MAX_DEBUG_CMD_LEN];
	sprintf (s, "%s %s:%d\n", cmd, w->editor->filename, line);
	xdebug_append_command (d, s, action, editor, line);
    }
}

static void xdebug_break_point (Debug * d, char *cmd, char *editor, int line)
{E_
    xdebug_break_point_special (d, cmd, editor, line, *cmd == 'b' ? ACTION_BREAKPOINT : ACTION_BREAKPOINT_CLEAR);
}

static void xdebug_set_break_points (Debug * d)
{E_
    int i;
    for (i = 0; i < last_edit; i++) {
	struct _book_mark *b;
	if (!edit[i]->editor->book_mark)
	    continue;
	for (b = edit[i]->editor->book_mark; b->prev; b = b->prev);	/* rewind */
	for (; b; b = b->next)
	    if (b->line >= 0 && b->c == DEBUG_BREAKPOINT_COLOR)
		xdebug_break_point (d, "b", edit[i]->ident, b->line + 1);
    }
}

int goto_error (char *message, int raise_wm_window);

static int xdebug_set_current (char *file, int line)
{E_
    char s[MAX_DEBUG_CMD_LEN];
    sprintf (s, "%s:%d", file, line);
    return goto_error (s, 0);
}

static void xdebug_goto_line (Debug * d, char *file, int line)
{E_
    xdebug_cursor_bookmark_flush ();
    if (!xdebug_set_current (file, line))
	book_mark_insert (edit[current_edit]->editor, edit[current_edit]->editor->curs_line, DEBUG_CURRENT_COLOR, 0, 0, 0);
    edit[current_edit]->editor->force |= REDRAW_PAGE;
    edit_render_keypress (edit[current_edit]->editor);
    edit_status (edit[current_edit]->editor);
}

void goto_file_dialog (const char *heading, const char *tool_hint, const char *text);

static Debug debug_session;

static void debug_finish (unsigned long x);

void debug_callback (void)
{E_
    static int animation = 0;
    static int refresh = 0;

    if ((animation++) % 50) {
	if (debug_session.action) {
	    CSetColor (color_widget (4));
	    XFillArc (CDisplay, debug_session.w, CGC, debug_session.x, debug_session.y, debug_session.r * 2, debug_session.r * 2, (animation * 487) % (360 * 64) - 180 * 64, 90 * 64);
	    XFillArc (CDisplay, debug_session.w, CGC, debug_session.x, debug_session.y, debug_session.r * 2, debug_session.r * 2, (animation * 487 + 180 * 64) % (360 * 64) - 180 * 64, 90 * 64);
	    CSetColor (color_widget (13));
	    XFillArc (CDisplay, debug_session.w, CGC, debug_session.x, debug_session.y, debug_session.r * 2, debug_session.r * 2, (animation * 487 + 90 * 64) % (360 * 64) - 180 * 64, 90 * 64);
	    XFillArc (CDisplay, debug_session.w, CGC, debug_session.x, debug_session.y, debug_session.r * 2, debug_session.r * 2, (animation * 487 + 270 * 64) % (360 * 64) - 180 * 64, 90 * 64);
	    refresh = 1;
	} else if (refresh) {
	    refresh = 0;
	    CSetColor (COLOR_FLAT);
	    XFillArc (CDisplay, debug_session.w, CGC, debug_session.x, debug_session.y, debug_session.r * 2, debug_session.r * 2, 0, 360 * 64);
	    CSendExpose (debug_session.w, 0, 0, 1000, 1000);
	}
    }
}

static char *from_start (char *s, char *p, int n)
{E_
    p = strstr (s, p);
    if (!p)
	return 0;
    if ((unsigned long) p > (unsigned long) s + n)
	return 0;
    return p;
}

static char *from_end (char *s, char *p, int n)
{E_
    int i, l, k;
    if ((l = strlen (s)) >= (k = strlen (p)) + n)
	for (i = l - k; i >= l - k - n; i--)
	    if (!memcmp (s + i, p, k))
		return s + i;
    return 0;
}


/*

   *     means alnum string
   A?    means optional A
   +     any single character

 */
static int match_pattern (char *s, const char *pattern)
{E_
    int l = 0;
    while (*pattern) {
	if (*pattern == '*') {
	    while (C_ALNUM (*s)) {
		s++;
		l++;
	    }
	    pattern++;
	} else if (*pattern == '+') {
	    if (!*s)
		return 0;
	    s++;
	    l++;
	    pattern++;
	} else if (pattern[1] == '?') {
	    if (*s == *pattern) {
		s++;
		l++;
	    }
	    pattern += 2;
	} else {
	    if (*s != *pattern)
		return 0;
	    s++;
	    l++;
	    pattern++;
	}
    }
    return l;
}

static char *from_end_pattern (char *s, const char *pattern, int n)
{E_
    char *e;
    for (e = s + strlen (s) - 1; e >= s; e--) {
	int match_len;
	match_len = match_pattern (e, pattern);
        if (match_len)
	    if (strlen (e + match_len) <= n) {
/* printf("FOUNDR: [%s]\n", pattern); */
	        return e;
            }
    }
    return 0;
}

static char *from_start_pattern (char *s, const char *pattern, int n)
{E_
    int i = 0;
    while (*s && i <= n) {
	int match_len;
	match_len = match_pattern (s, pattern);
	if (match_len) {
/* printf ("FOUNDS: [%s]\n", pattern); */
	    return s;
	}
        s++;
        i++;
    }
    return 0;
}

static void xdebug_check_watch_point (Debug * d, char *m)
{E_
    char *p, *q, *s, *r;
    CWidget *w;
    p = strstr (m, "\nHardware watchpoint ");
    if (!p)
	return;
    w = CIdent ("debug_vars");
    if (w) {
	CSetTextboxPos (w, TEXT_SET_CURSOR_LINE, atoi (p + 12));
	CRedrawFieldedTextbox ("debug_vars", 1);
    }
    r = strchr (p, ':');
    if (!r)
	return;
    r += 2;
    p = strchr (r, '\n');
    if (!p)
	return;
    *p++ = '\0';
    p = strchr (p, '=');
    if (!p)
	return;
    p += 2;
    q = strchr (p, '\n');
    if (!q)
	return;
    *q++ = '\0';
    q = strchr (q, '=');
    if (!q)
	return;
    q += 2;
    *(strchr (q, '\n')) = '\0';
    s = malloc (strlen (p) + strlen (q) + 100);
    strcpy (s, _ ("Variable: "));
    strcat (s, r);
    strcat (s, "\n\n");
    strcat (s, p);
    strcat (s, "\n     --->     \n");
    strcat (s, q);
    debug_message (d, _ (" Watchpoint "), s, TEXT_CENTRED);
    free (s);
}

int str_empty_space (const char *p)
{E_
    for (; *p; p++)
	if (((unsigned char) *p) > ' ')
	    return 0;
    return 1;
}


static void debug_read_callback (int fd, fd_set * reading, fd_set * writing, fd_set * error, void *data)
{E_
    CWidget *w;
    Debug *d;
    char *p;
    char buf[1025];
    int c, action, n, i;
    d = &debug_session;
    if (CChildExitted (d->pid, 0) || (!rxvt_have_pid (d->xterm_pid) && d->show_output)) {
	debug_finish (0);
	return;
    }
    do {
	c = read (d->out, buf, 1024);
    } while (c < 0 && (errno == EINTR || errno == EAGAIN));
    if (c <= 0) {
	debug_error1 (d, _ ("Error reading from the debugger"));
	debug_finish (0L);
	return;
    }
    buf[c] = '\0';
    n = 0;
    if (!d->pool)
	d->pool = pool_init ();
    pool_printf (d->pool, "%s", buf);
    if (d->show_on_stdout) {
	printf ("%s", buf);
	fflush (stdout);
    }
    for (i = 0; d->query[i].query; i++) {
	if (from_end_pattern ((char *) pool_start (d->pool), d->query[i].query, 1)) {
            char *response = (char *) "\n";
            if (d->query[i].response) {
                response = d->query[i].response;
            } else {
                if (!d->n_commands)
                    goto special;       /* hack to prompt the user with a dialog on "Make breakpoint pending on future shared library load? (y or [n])" */
                response = "n\n";
            }
	    pool_drop_last_line (d->pool);
	    if (d->show_on_stdout) {
		printf ("%s", response);
		fflush (stdout);
	    }
	    if (writeall (d->in, response, strlen (response)) != strlen (response)) {
		debug_error1 (d, _ ("Error writing to the debugger"));
		debug_finish (0L);
		return;
	    }
	    break;
	}
    }
    if (!d->action)
	goto fin_read;
    if (d->action == ACTION_BREAKPOINT_CLEAR)
	p = from_end ((char *) pool_start (d->pool), CLEAR_PROMPT, 0);
    else
	p = from_end ((char *) pool_start (d->pool), d->prompt, 0);
    if (!p)
	return;			/* we have not found a prompt, so the command must not have completed */
    *p = '\0';
    p = 0;

  special:
    action = d->action;
    d->action = 0;

    switch (action) {
    case ACTION_PROMPT:	/* just waiting for the prompt to return, then command is completed */
	pool_free (d->pool);
	d->pool = 0;
	break;

    case ACTION_QUIT:
    case ACTION_MESSAGE:	/* There was message from this command, display it */
	if (strspn ((char *) pool_start (d->pool), "\n\t \r") != strlen ((char *) pool_start (d->pool)))
	    debug_error1 (d, (char *) pool_start (d->pool));
	pool_free (d->pool);
	d->pool = 0;
	if (action == ACTION_QUIT)
	    debug_finish (0L);
	break;

    case ACTION_VERSION:
#if 0  /* I am removing this -- it always seems to work even if not 100%. */
/* 4.18 doesn't seem to do next's properly */
	if (!strstr ((char *) pool_start (d->pool), GDB_VERSION1) && !strstr ((char *) pool_start (d->pool), GDB_VERSION3)) {
	    static int once = 1;
	    if (once) {
/* NLS: <version> or <version> expected. You seem... */
		debug_error1 (d, catstrs (GDB_VERSION1 " ",  _ ("or"), " " GDB_VERSION3, _ (" expected. You seem to have a different debugger \n or version. If things don't work properly, this is why. \n This error will not display again in this session. "), NULL));
		once = 0;
	    }
	}
#endif
	if (!d->pool)	/* process was terminated */
	    break;
	if (from_end ((char *) pool_start (d->pool), "no debugging symbols found", 6)) {
	    debug_error1 (d, _ ("The debugger returned \"no debugging symbols found\" implying that \n the executable has been stripped, or was not compiled with \n the \"-g\" option for debugging"));
	    debug_finish (0L);
	    break;
	}
	pool_free (d->pool);
	d->pool = 0;
	break;

    case ACTION_WATCH:
	p = strstr ((char *) pool_start (d->pool), "watchpoint");
	if (p) {
	    d->variable[d->break_point_line].watch = atoi (p + 11);
	} else {
	    d->variable[d->break_point_line].watch = 0;
	}
	CRedrawFieldedTextbox ("debug_vars", 1);
	pool_free (d->pool);
	d->pool = 0;
	break;

    case ACTION_VARIABLE:
    case ACTION_LASTVAR:
	xdebug_display_variable (d, (char *) pool_start (d->pool), action);
	pool_free (d->pool);
	d->pool = 0;
	break;

    case ACTION_CONFIRM_BREAKPOINT:
        w = CIdent (d->break_point_editor);
        if (w) {
            if (!book_mark_query_color (w->editor, d->break_point_line - 1, DEBUG_BREAKPOINT_COLOR)) {
                book_mark_insert (w->editor, d->break_point_line - 1, DEBUG_BREAKPOINT_COLOR, 0, 0, 0);
                w->editor->force |= REDRAW_COMPLETELY;
                edit_render_keypress (edit[current_edit]->editor);
            }
        }
	pool_free (d->pool);
	d->pool = 0;
	break;

    case ACTION_DENY_BREAKPOINT:
        w = CIdent (d->break_point_editor);
        if (w) {
            while (!book_mark_clear (w->editor, d->break_point_line - 1, DEBUG_BREAKPOINT_COLOR));
            w->editor->force |= REDRAW_COMPLETELY;
            edit_render_keypress (w->editor);
        }
	pool_free (d->pool);
	d->pool = 0;
	break;

    case ACTION_CONDITION:
    case ACTION_BREAKPOINT:
    case ACTION_BREAKPOINT_CLEAR:

#define CLEAR_PROMPT_OFFSET 8

        w = CIdent (d->break_point_editor);
	if (from_start ((char *) pool_start (d->pool), "No symbol table is loaded", CLEAR_PROMPT_OFFSET)) {
	    debug_error1 (d, _ ("The debugger returned \"No symbol table is loaded\" \n implying that the executable does not \n exist or is badly formatted"));
	    debug_finish (0L);
	    break;
	}
	if (from_start ((char *) pool_start (d->pool), "Function \"main\" not defined", CLEAR_PROMPT_OFFSET) ||
	    from_start ((char *) pool_start (d->pool), "No source file specified", CLEAR_PROMPT_OFFSET)) {
	    debug_error1 (d, _ ("The debugger returned a message implying that \n the executable was not compiled with \n the \"-g\" option for debugging"));
	    debug_finish (0L);
	    break;
	}
        if ((p = from_end_pattern ((char *) pool_start (d->pool), "Make breakpoint pending on future shared library load+  ?( ?[?y]? or [?n]? ?)", CLEAR_PROMPT_OFFSET))) {
            if (debug_query_yes_no ("Cannot set break point", "Make breakpoint pending on future shared library load?")) {
                xdebug_append_command (&debug_session, "y\n", ACTION_CONFIRM_BREAKPOINT, d->break_point_editor, d->break_point_line);
            } else {
                xdebug_append_command (&debug_session, "n\n", ACTION_DENY_BREAKPOINT, d->break_point_editor, d->break_point_line);
            }
            pool_free (d->pool);
            d->pool = 0;
            break;
        }
        if (!p)
	    p = strstr ((char *) pool_start (d->pool), "Breakpoint ");
	if (p) {
	    p += 11;
	    n = atoi (p);
	    p += strspn (p, "0123456789");
	    p = from_start (p, " at ", 0);
	}
	if (p && action == ACTION_CONDITION) {
	    char *q;
	    q = malloc (strlen (d->condition) + 30);
	    sprintf (q, "condition %d %s\n", n, d->condition);
	    free (d->condition);
	    d->condition = 0;
	    xdebug_append_command (&debug_session, q, ACTION_MESSAGE, 0, 0);
	    free (q);
	}
        if (p) {
            if (w) {
                if (!book_mark_query_color (w->editor, d->break_point_line - 1, DEBUG_BREAKPOINT_COLOR)) {
                    book_mark_insert (w->editor, d->break_point_line - 1, DEBUG_BREAKPOINT_COLOR, 0, 0, 0);
                    w->editor->force |= REDRAW_COMPLETELY;
                    edit_render_keypress (edit[current_edit]->editor);
                }
            }
        } else {
            p = from_start ((char *) pool_start (d->pool), "Deleted breakpoint", CLEAR_PROMPT_OFFSET);
            if (!p)
                p = from_start ((char *) pool_start (d->pool), "No breakpoint ", CLEAR_PROMPT_OFFSET);
            if (!p) {
                p = from_start ((char *) pool_start (d->pool), "No source file named ", CLEAR_PROMPT_OFFSET);
                if (action == ACTION_BREAKPOINT_CLEAR)
                    p = 0;
            }
            if (p && w) {
                while (!book_mark_clear (w->editor, d->break_point_line - 1, DEBUG_BREAKPOINT_COLOR));
                w->editor->force |= REDRAW_COMPLETELY;
                edit_render_keypress (w->editor);
            }
        }
	if (!p) {
	    if (!str_empty_space ((char *) pool_start (d->pool)))
		debug_error1 (d, (char *) pool_start (d->pool));
	    if (!d->pool)
		break;
	    xdebug_flush_commands (d);
	}
	if (p)
	    xdebug_show_variables (d);
	pool_free (d->pool);
	d->pool = 0;
	break;

    case ACTION_INFO_SOURCE:
    	{
	    char *last_file;
	    last_file = d->file;
	    d->file = 0;
	    p = strstr ((char *) pool_start (d->pool), "Located in ");
	    if (p) {
		char *q;
		q = p + 11;
		p = strchr (q, '\n');
		if (p) {
		    *p = '\0';
		    d->file = (char *) strdup (q);
		}
	    } else {	/* try to see if we are still in the same source file */

/* ------ the functionality in this block is currently not useful ------ */
		char *base_name = 0;
		p = strstr ((char *) pool_start (d->pool), "Current source file is ");
		if (p) {
		    base_name = p + 23;
		    p = strchr (base_name, '\n');
		    if (!p) {
			base_name = 0;
		    } else {
			*p = '\0';
			if (last_file) {
			    p = (p = strrchr (last_file, '/')) ? p + 1 : last_file;	/* get the basename */
			    if (!strcmp (base_name, p)) {
				d->file = last_file;
				last_file = 0;
			    }
			}
		    }
		}
		if (!d->file && base_name)
		    d->file = user_file_list_search (0, 0, 0, base_name);
	    }
	    if (last_file)
		free (last_file);
	    p = strstr ((char *) pool_start (d->pool), "No current source file");
	    if (p) {
		xdebug_cursor_bookmark_flush ();
	    } else {
		if (d->file && d->line)	/* if we can't get the file number, issue a backtrace */
		    xdebug_goto_line (d, d->file, d->line);
		else
		    xdebug_append_command (d, d->get_backtrace, ACTION_BACKTRACE, 0, 0);
	    }
	    pool_free (d->pool);
	    d->pool = 0;
	}
	break;

    case ACTION_INFO_PROGRAM:
	p = strstr ((char *) pool_start (d->pool), "Pid");
	if (!p)
	    p = strstr ((char *) pool_start (d->pool), "Thread");
/* BUG FIX - gdb 5.0 says process instead of Pid */
	if (!p)
	    p = strstr ((char *) pool_start (d->pool), "process");
/* end BUG FIX */
	if (p) {
	    p += strcspn (p, "0123456789");
	    d->child = atoi (p);
	}
	if (p && (p = strstr ((char *) p, "LWP"))) {
	    p += strcspn (p, "0123456789");
	    d->child = atoi (p);
	}
	pool_free (d->pool);
	d->pool = 0;
	break;

    case ACTION_BACKTRACE:
    case ACTION_SHOW:
	p = strstr ((char *) pool_start (d->pool), "No stack");
	if ((unsigned long) p == (unsigned long) pool_start (d->pool)) {
	    debug_error1 (d, _ (" There is no stack "));
            pool_free (d->pool);
	} else {
	    goto_file_dialog (action == ACTION_BACKTRACE ? _ (" Backtrace ") : _ (" Output "), 0, (char *) pool_break (d->pool));
	}
	d->pool = 0;
	break;

    case ACTION_STEP:
    case ACTION_RUNNING:
    case ACTION_UNTIL:

/* program exits with code:

c
Continuing.
[Inferior 1 (process 1985) exited with code 01]

*/

/* program exits normally

c
Continuing.
[Inferior 1 (process 12399) exited normally]

*/

/* received signal

Program received signal SIGINT, Interrupt.
loop () at /home/paul/test.c:7
7           }
(gdb) 

*/

/* info source output

(gdb) info source
Current source file is /home/paul/test.c
Located in /home/paul/test.c
Contains 20 lines.
Source language is c.
Producer is GNU C11 5.4.0 20160609 -mtune=generic -march=x86-64 -g -O0 -fstack-protector-strong.
Compiled with DWARF 2 debugging format.
Does not include preprocessor macro info.
(gdb) 

*/

/* Ctrl-C ==>

Program received signal SIGINT, Interrupt.
loop () at test.c:7
7	    }
*/

	for (p = (char *) pool_start (d->pool); p; p++) {
	    if (!*p)
		break;
            if (!strncmp (p, "0x", 2)) {
/* {like: */
/* 0x8060bd0       2110    return -1; } */
                p += 2;
                p += strspn (p, "0123456789abcdef");
                while (*p == ' ' || *p == '\t')
                    p++;
            }
	    if (*p >= '0' && *p <= '9')
		d->line = atoi (p);
	    p = strchr (p, '\n');
	    if (!p)
		break;
	}
	xdebug_check_watch_point (d, (char *) pool_start (d->pool));
	p = from_end ((char *) pool_start (d->pool), "\nProgram exited normally", 3);
        if (!p)
            p = from_end_pattern ((char *) pool_start (d->pool), "[* * (* *) exited normally]", 3);
	if (!p)
	    p = from_end ((char *) pool_start (d->pool), "\nProgram exited with code", 7);
        if (!p)
            p = from_end_pattern ((char *) pool_start (d->pool), "[* * (* *) exited with code *]", 3);
	if (!p)
	    p = from_end ((char *) pool_start (d->pool), "No such process", 7);
	if (p) {
	    xdebug_cursor_bookmark_flush ();
	    d->child = 0;
	}
	if (!p) {		/* in all other circumstances, we must get the file and line number */
            if (d->line)	/* got the line, now be sure of the filename */
                xdebug_append_command (d, d->get_source, ACTION_INFO_SOURCE, 0, 0);
            else		/* don't have the line, so show a backtrace */
                xdebug_append_command (d, d->get_backtrace, ACTION_BACKTRACE, 0, 0);
            p = from_start ((char *) pool_start (d->pool), "terminate called after throwing an instance of", 3);
            if (!p)
                p = from_start_pattern ((char *) pool_start (d->pool), "Program received signal *", 20);
	}
	if (!p) {		/* in all other circumstances, we must get the file and line number */
	    p = from_start ((char *) pool_start (d->pool), "The program is not being run", 0);
            if (!p)
                p = from_start_pattern ((char *) pool_start (d->pool), "Program terminated with signal *", 3);
            if (!p)
                p = from_end_pattern ((char *) pool_start (d->pool), "The program no longer exists.\n", 3);
	    if (p) {
		xdebug_cursor_bookmark_flush ();
		d->child = 0;
		if (action == ACTION_UNTIL)
		    xdebug_break_point (d, "clear", d->break_point_editor, d->break_point_line + 1);
	    }
	}
	if (p) {
	    debug_error1 (d, p);
	} else if (action == ACTION_UNTIL) {
	    xdebug_break_point (d, "clear", d->break_point_editor, d->break_point_line + 1);
	}
	pool_free (d->pool);
	d->pool = 0;
	xdebug_show_variables (d);
	break;
    default:
	break;
    }
  fin_read:
    if (d->n_commands && !d->action)
	CAddWatch (d->in, debug_write_callback, WATCH_WRITING, 0);
}

static void debug_write_callback (int fd, fd_set * reading, fd_set * writing, fd_set * error, void *data)
{E_
    Debug *d;
    d = &debug_session;
    if (CChildExitted (d->pid, 0) || (!rxvt_have_pid (d->xterm_pid) && d->show_output)) {
	debug_finish (0);
	return;
    }
    if (d->show_on_stdout) {
	printf ("%s", d->command[0].command);
	fflush (stdout);
    }
    if (d->break_point_editor)
	free (d->break_point_editor);
    d->break_point_editor = 0;
    d->break_point_line = 0;
/* no backtrace if no program running, all other commands are ok */
    if (!(!d->child && (d->command[0].action == ACTION_INFO_SOURCE || d->command[0].action == ACTION_BACKTRACE))) {
	if (writeall (d->in, d->command[0].command, strlen (d->command[0].command)) != strlen (d->command[0].command)) {
	    debug_error1 (d, _ ("Error writing to the debugger"));
	    debug_finish (0L);
	    return;
	}
	d->action = d->command[0].action;
	d->break_point_editor = d->command[0].editor;
	d->break_point_line = d->command[0].line;
    }
    free (d->command[0].command);
    if (d->action == ACTION_RUNNING || d->action == ACTION_STEP || d->action == ACTION_UNTIL)
	xdebug_cursor_bookmark_flush ();
    Cmemmove (&d->command[0], &d->command[1], d->n_commands * sizeof (struct _command));
    d->command[d->n_commands].action = 0;
    d->command[d->n_commands].command = 0;
    d->n_commands--;
    CRemoveWatch (d->in, debug_write_callback, WATCH_WRITING);
}


static int xdebug_run_program (Debug * d)
{E_
    char *arg[10];
    int i = 0;
    struct stat s;
    xdebug_cursor_bookmark_flush ();
    if (stat (d->progname, &s)) {
	debug_error2 (d,
		      _
		      ("Error trying to stat executable: check that the filename and \n current directory are correct"),
		      strerror (errno));
	return 1;
	if (!(s.st_mode & S_IXUSR)) {
	    debug_error2 (d, _("Your program does not seem to have execute permissions"),
			  d->progname);
	    return 1;
	}
    }
    arg[i++] = d->debugger;
    if (d->show_output) {
	char *p, *gargv[2] = {0, 0};
	struct _rxvtlib *rxvt;
        gargv[0] = d->progname;
	rxvt = rxvt_start (CRoot, gargv, 1);
	if (!rxvt) {
	    d->xterm_pid = 0;
	    d->pid = 0;
	    debug_error1 (d, _("Could not open an rxvt"));
	    xdebug_flush_commands (d);
	    return 1;
	}
	p = (char *) strdup ("-tty=                    ");
	rxvt_get_tty_name (rxvt, p + 5);
	d->xterm_pid = (pid_t) rxvt_get_pid (rxvt);
	arg[i++] = p;
	arg[i++] = d->progname;
	arg[i] = 0;
	d->pid = triple_pipe_open (&d->in, &d->out, 0, 1, d->debugger, arg);
	free (p);
    } else {
	char *p;
	p = (char *) strdup ("-tty=                    ");
	arg[i++] = p;
	arg[i++] = d->progname;
	arg[i] = 0;
	d->pid = open_under_pty (&d->in, &d->out, p + 5, d->debugger, arg);
	free (p);
    }
    if (d->pid <= 0) {
	d->pid = 0;
	if (d->xterm_pid)
	    kill (d->xterm_pid, SIGTERM);
	d->xterm_pid = 0;
	d->pid = 0;
	debug_error1 (d, _("Could not open gdb"));
	xdebug_flush_commands (d);
	return 1;
    }
    d->child = 0;
    d->action = ACTION_VERSION;
    CAddWatch (d->out, debug_read_callback, WATCH_READING, 0);
    return 0;
}

static void xdebug_set_args (Debug * d, char *args)
{E_
    char *s;
    s = malloc (strlen (args) + 20);
    sprintf (s, "set args %s\n", args ? args : "");
    xdebug_append_command (d, s, ACTION_PROMPT, 0, 0);
    free (s);
}

static void xdebug_run (Debug * d)
{E_
    xdebug_append_command (d, "b main\n", ACTION_BREAKPOINT, 0, 0);
    xdebug_append_command (d, "r\n", ACTION_RUNNING, 0, 0);
    xdebug_append_command (d, "clear\n", ACTION_BREAKPOINT_CLEAR, 0, 0);
#if 0
/* programs get this signal when they ioctl the terminal given to them by gdb */
/* this prevents them quiting when using the `show output' option */
    if (d->ignore_SIGTTOU) {
	sprintf (s, "call signal (%d, (void *) %lu)\n", SIGTTOU, (unsigned long) SIG_IGN);
	xdebug_append_command (d, s, ACTION_PROMPT, 0, 0);
    }
#endif
    xdebug_append_command (d, "info program\n", ACTION_INFO_PROGRAM, 0, 0);
    if (!d->stop_at_main)
	xdebug_append_command (d, "c\n", ACTION_RUNNING, 0, 0);
}

static void debug_start (void)
{E_
    if (xdebug_run_program (&debug_session))
	return;
    xdebug_set_args (&debug_session, debug_session.args);
/*    xdebug_set_watch_points (&debug_session);	*/
    xdebug_set_break_points (&debug_session);
    xdebug_run (&debug_session);
}

static int debug_set_info (unsigned long x);

static void debug_attach (unsigned long x)
{E_
    char *s;
    char t[80];
    long p;
    if (debug_session.pid) {
	debug_error1 (&debug_session, _ ("Debugger is current running, use Kill or Detach first"));
	return;
    }
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    xdebug_cursor_bookmark_flush ();
    xdebug_flush_commands (&debug_session);
    if (!debug_session.progname)
	if (debug_set_info (0))
	    return;
    s = CInputDialog ("debugattach", 0, 0, 0, 30 * FONT_MEAN_WIDTH, TEXTINPUT_LAST_INPUT, _ (" Debug "), _ (" Enter process id to attach to : "));
    if (!s)
	return;
    p = atol (s);
    free (s);
    if (p <= 0)
	return;
    debug_session.show_output = 0;
    if (xdebug_run_program (&debug_session))
	return;
    xdebug_set_break_points (&debug_session);
    sprintf (t, "attach %ld\n", p);
    xdebug_append_command (&debug_session, t, ACTION_PROMPT, 0, 0);
    xdebug_append_command (&debug_session, "info program\n", ACTION_INFO_PROGRAM, 0, 0);
    xdebug_append_command (&debug_session, "c\n", ACTION_RUNNING, 0, 0);
}

void debug_detach (unsigned long x)
{E_
    if (!debug_session.pid) {
	debug_error1 (&debug_session, _ ("Debugger is not running"));
	return;
    }
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    xdebug_append_command (&debug_session, "detach\n", ACTION_QUIT, 0, 0);
    debug_session.child = 0;
}

/* resturns 1 on cancel */
static int debug_set_info (unsigned long x)
{E_
    char *inputs[10] =
    {
	gettext_noop (""),
	gettext_noop (""),
	0
    };
    char *input_labels[10] =
    {
	gettext_noop ("Program name"),
	gettext_noop ("Enter program arguments (space for none)"),
	0
    };
    char *check_labels[10] =
    {
	gettext_noop ("Show program output"),
	gettext_noop ("Stop at main() function"),
	gettext_noop ("Show on stdout"),
#if 0
	gettext_noop ("Ignore SIGTTOU"),
#endif
	0
    };
    char *check_tool_hints[10] =
    {
	gettext_noop ("This will cause the program to be opened udner an rxvt terminal so that its output can be viewed."),
	gettext_noop ("This will cause the debugger to break at the first\nline of code in the program when issuing a `Start'."),
	gettext_noop ("This will cause all debugger commands to show at\nthe terminal where you started cooledit."),
#if 0
	gettext_noop ("Run `call signal()' inside gdb so that SIGTTOU signals are ignored. Without this gdb sometimes exits."),
#endif
	0
    };
    char *input_names[10] =
    {
	gettext_noop ("debugprogname"),
	gettext_noop ("debugargs+"),
	0
    };
    char *input_tool_hint[10] =
    {
	gettext_noop ("The executable that you would like to debug - this must be compiled with the -g option"),
	gettext_noop ("Arguments to be passed to the executable - can be empty"),
	0
    };
    int *checks_values_result[10];
    char **inputs_result[10];
    int r;
    inputs_result[0] = &inputs[0];
    inputs_result[1] = &inputs[1];
    inputs_result[2] = 0;
    checks_values_result[0] = &debug_session.show_output;
    checks_values_result[1] = &debug_session.stop_at_main;
    checks_values_result[2] = &debug_session.show_on_stdout;
#if 0
    checks_values_result[3] = &debug_session.ignore_SIGTTOU;
    checks_values_result[4] = 0;
#else
    checks_values_result[3] = 0;
#endif
    r = CInputsWithOptions (0, 0, 0, _ (" Debug : Program Data "), inputs_result, input_labels, input_names, input_tool_hint, checks_values_result, check_labels, check_tool_hints, INPUTS_WITH_OPTIONS_BROWSE_LOAD_1, 60);
    if (r)
	return 1;
    if (debug_session.progname)
	free (debug_session.progname);
    debug_session.progname = inputs[0];
    if (debug_session.args)
	free (debug_session.args);
    debug_session.args = inputs[1];
    if (debug_session.pid)
	debug_error1 (&debug_session, _ (" The debugger is currently running. You will have \n to kill the debugger for these changes to take effect. "));
    return 0;
}

/* ACTION_STEP is the same as ACTION_RUNNING except that if a user hits it twice in a row,
   it won't be prone to give "Program is currently running..." messages. */
static void debug_step (unsigned long c)
{E_
    char *cmd;
    cmd = (char *) c;
    if (!debug_session.pid) {
	debug_error1 (&debug_session, _ ("Debugger is not running"));
	return;
    }
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    xdebug_append_command (&debug_session, cmd, ACTION_STEP, 0, 0);
}

static void debug_command (unsigned long c)
{E_
    char *cmd;
    cmd = (char *) c;
    if (!debug_session.pid) {
	debug_error1 (&debug_session, _ ("Debugger is not running"));
	return;
    }
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    xdebug_append_command (&debug_session, cmd, ACTION_RUNNING, 0, 0);
}

static void debug_show_output (unsigned long c)
{E_
    char s[MAX_DEBUG_CMD_LEN];
    char *cmd;
    cmd = (char *) c;
    if (!debug_session.pid) {
	debug_error1 (&debug_session, _ ("Debugger is not running"));
	return;
    }
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    if (!cmd) {
	cmd = CInputDialog ("debugruncmd", 0, 0, 0, 50 * FONT_MEAN_WIDTH, TEXTINPUT_LAST_INPUT, _ (" Debug "), _ (" Enter debugger command : "));
	if (cmd) {
	    sprintf (s, "%s\n", cmd);
	    free (cmd);
	    cmd = s;
	}
    }
    if (!cmd)
	return;
    xdebug_append_command (&debug_session, cmd, ACTION_SHOW, 0, 0);
}

static void debug_break (unsigned long x)
{E_
    if (!debug_session.pid) {
	debug_error1 (&debug_session, _ ("Debugger is not running"));
	return;
    }
    if (!debug_session.action)	/* already stopped */
	return;
    if (debug_session.action == ACTION_RUNNING || debug_session.action == ACTION_STEP || debug_session.action == ACTION_UNTIL) {
	xdebug_flush_commands (&debug_session);
	if (debug_session.child) {
	    kill (debug_session.child, SIGINT);
	    debug_session.action = ACTION_RUNNING;
	}
    }
}

static void debug_finish (unsigned long x)
{E_
    xdebug_cursor_bookmark_flush ();
    xdebug_flush_commands (&debug_session);
    if (debug_session.pool)
	pool_free (debug_session.pool);
    debug_session.pool = 0;
    if (debug_session.pid) {
	kill (debug_session.pid, SIGTERM);
	debug_session.pid = 0;
	CRemoveWatch (debug_session.in, debug_write_callback, WATCH_WRITING);
	CRemoveWatch (debug_session.out, debug_read_callback, WATCH_READING);
	close (debug_session.in);
	close (debug_session.out);
	if (debug_session.xterm_pid) {
	    kill (debug_session.xterm_pid, SIGTERM);
	    debug_session.xterm_pid = 0;
	}
	if (debug_session.child) {
	    kill (debug_session.child, SIGKILL);
	    debug_session.child = 0;
	}
    }
}

static void debug_restart (unsigned long x)
{E_
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    xdebug_cursor_bookmark_flush ();
    xdebug_flush_commands (&debug_session);
    if (!debug_session.progname)
	if (debug_set_info (0))
	    return;
    if (!debug_session.pid) {
	debug_start ();
    } else {
	debug_session.child = 0;
	xdebug_append_command (&debug_session, "k\n", ACTION_PROMPT, 0, 0);
	xdebug_run (&debug_session);
    }
}

static void debug_clear_breakpoints (unsigned long x)
{E_
    int i;
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    if (debug_session.pid)
	xdebug_append_command (&debug_session, "d\n", ACTION_PROMPT, 0, 0);
    for (i = 0; i < last_edit; i++)
	book_mark_flush (edit[i]->editor, DEBUG_BREAKPOINT_COLOR);
}

static void debug_conditional (unsigned long x)
{E_
    if (!debug_session.pid) {
	debug_error1 (&debug_session, _ ("Debugger is not running"));
	return;
    }
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    if (debug_session.condition)
	free (debug_session.condition);
    debug_session.condition = CInputDialog ("debugcondition", 0, 0, 0, 50 * FONT_MEAN_WIDTH, TEXTINPUT_LAST_INPUT, _ (" Breakpoint "), _ (" Enter condition on which to break : "));
    if (debug_session.condition)
	xdebug_break_point_special (&debug_session, "b", edit[current_edit]->ident, edit[current_edit]->editor->curs_line + 1, ACTION_CONDITION);
}

static void debug_break_point (unsigned long x)
{E_
    char *cmd = "b";
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    if (book_mark_query_color (edit[current_edit]->editor, edit[current_edit]->editor->curs_line, DEBUG_BREAKPOINT_COLOR))
	cmd = "clear";
    if (debug_session.pid)
	xdebug_break_point (&debug_session, cmd, edit[current_edit]->ident, edit[current_edit]->editor->curs_line + 1);
    else {
	if (!strcmp (cmd, "clear")) {
	    while (!book_mark_clear (edit[current_edit]->editor, edit[current_edit]->editor->curs_line, DEBUG_BREAKPOINT_COLOR));
	    edit[current_edit]->editor->force |= REDRAW_COMPLETELY;
	    edit_render_keypress (edit[current_edit]->editor);
	} else {
	    book_mark_insert (edit[current_edit]->editor, edit[current_edit]->editor->curs_line, DEBUG_BREAKPOINT_COLOR, 0, 0, 0);
	    edit[current_edit]->editor->force |= REDRAW_COMPLETELY;
	    edit_render_keypress (edit[current_edit]->editor);
	}
    }
}

static void debug_add_var (unsigned int x)
{E_
    char *s;
    if (!debug_session.pid) {
	debug_error1 (&debug_session, _ ("Debugger is not running"));
	return;
    }
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    s = CInputDialog ("debugvariab", 0, 0, 0, 30 * FONT_MEAN_WIDTH, TEXTINPUT_LAST_INPUT, _ (" Debug "), _ (" Enter variable to display : "));
    if (!s)
	return;
    if (xdebug_add_variable (&debug_session, s)) {
	free (s);
	return;
    }
    free (s);
    xdebug_show_variables (&debug_session);
}

static void debug_until (unsigned int x)
{E_
    if (!debug_session.pid) {
	debug_error1 (&debug_session, _ ("Debugger is not running"));
	return;
    }
    if (debug_session.action) {
	debug_error1 (&debug_session, _ ("Program is currently running, use Control-C first"));
	return;
    }
    xdebug_break_point (&debug_session, "b", edit[current_edit]->ident, edit[current_edit]->editor->curs_line + 1);
    xdebug_append_command (&debug_session, "c\n", ACTION_UNTIL, edit[current_edit]->ident, edit[current_edit]->editor->curs_line);
}

void xdebug_init_animation (void)
{E_
    CWidget *w;
    w = CIdent ("menu.debugmenu");
    debug_session.r = w->height / 2;
    debug_session.x = w->width / 2 - debug_session.r;
    debug_session.y = w->height / 2 - debug_session.r;
    debug_session.w = w->winid;
}

void debug_init (void)
{E_
    memset (&debug_session, 0, sizeof (debug_session));
    debug_session.prompt = "(gdb) ";
    debug_session.get_line = "info line\n";
    debug_session.get_source = "info source\n";
    debug_session.get_backtrace = "backtrace\n";
    debug_session.query[0].query = "Make breakpoint pending on future shared library load+  ?( ?[?y]? or [?n]? ?)";
    debug_session.query[0].response = 0;  /* zero means don't consider further patterns */
    debug_session.query[1].query = "( ?[?y]? or [?n]? ?)";
    debug_session.query[1].response = "y\n";
    debug_session.query[2].query = "<return> to quit---";
    debug_session.query[2].response = "\n";
    debug_session.query[3].query = "c to continue without paging--";
    debug_session.query[3].response = "c\n";
    debug_session.query[4].query = 0;
    debug_session.query[4].response = 0;
    debug_session.debugger = "cooledit-gdb";
    debug_session.stop_at_main = 1;
#if 0
    debug_session.ignore_SIGTTOU = 1;
#endif
    xdebug_init_animation ();
}

void debug_shut (void)
{E_
    int i;
    debug_finish (0);
    CDestroyWidget ("debug_vars.win");
    if (debug_session.progname)
	free (debug_session.progname);
    if (debug_session.args)
	free (debug_session.args);
    if (debug_session.file)
	free (debug_session.file);
    if (debug_session.condition)
	free (debug_session.condition);
    if (debug_session.break_point_editor)
	free (debug_session.break_point_editor);
    for (i = 0; debug_session.variable[i].name && debug_session.variable[i].output; i++) {
	free (debug_session.variable[i].name);
	free (debug_session.variable[i].output);
    }
    memset (&debug_session, 0, sizeof (debug_session));
}

void debug_menus (Window parent, Window main_window, int x, int y)
{E_
    CDrawMenuButton ("menu.debugmenu", parent, main_window, x, y, AUTO_SIZE, 16,
			 _ (" Debug "),
			 _ ("Start\tAlt-F5"), (int) '~', debug_restart, 0L,
			 _ ("Stop\tCtrl-c"), (int) '~', debug_break, 0L,
			 _ ("Attach"), (int) '~', debug_attach, 0L,
			 _ ("Detach"), (int) '~', debug_detach, 0L,
			 _ ("Toggle breakpoint\tAlt-F2"), (int) '~', debug_break_point, 0L,
			 _ ("Insert conditional breakpoint"), (int) '~', debug_conditional, 0L,
			 _ ("Clear all breakpoints"), (int) '~', debug_clear_breakpoints, 0L,
			 _ ("Next\tAlt-F9"), (int) '~', debug_step, (unsigned long) "n\n",
			 _ ("Step\tAlt-F8"), (int) '~', debug_step, (unsigned long) "s\n",
			 _ ("List back trace"), (int) '~', debug_show_output, (unsigned long) "backtrace\n",
			 _ ("Continue\tAlt-F4"), (int) '~', debug_command, (unsigned long) "c\n",
			 _ ("Continue until cursor\tAlt-F3"), (int) '~', debug_until, 0L,
			 _ ("Kill debugger"), (int) '~', debug_finish, 0L,
			 _ ("Enter command...\tAlt-F1"), (int) '~', debug_show_output, 0L,
			 _ ("Display variable..."), (int) '~', debug_add_var, 0L,
			 _ ("Set info..."), (int) '~', debug_set_info, 0L
	);
    CSetToolHint ("menu.debugmenu", _ ("Interactive graphical debugger"));
}

/* hack: c is just to check if different keys are pressed - any unique
   thing can go in there. */
static int time_diff (unsigned long c)
{E_
    static unsigned long c_last = 0;
    int r = 0;
    static struct timeval tv =
    {0, 0};
    struct timeval tv_last;
    tv_last = tv;
    gettimeofday (&tv, 0);
    if ((double) ((double) tv.tv_usec / 1000000.0 + (double) tv.tv_sec) - \
     ((double) tv_last.tv_usec / 1000000.0 + (double) tv_last.tv_sec) < \
	(double) 0.5 && c == c_last)
	r = 1;
    c_last = c;
    return r;
}

void debug_key_command (int command)
{E_
    if (time_diff (command))
	if (debug_session.action) {
	    return;
	}
    if (command == CK_Debug_Stop) {
	debug_break (0);
	return;
    }
    if (debug_session.n_commands)
	return;
    switch (command) {
    case CK_Debug_Start:
	debug_restart (0);
	break;
    case CK_Debug_Toggle_Break:
	debug_break_point (0);
	break;
    case CK_Debug_Clear:
	debug_clear_breakpoints (0);
	break;
    case CK_Debug_Next:
	debug_step ((unsigned long) "n\n");
	break;
    case CK_Debug_Step:
	debug_step ((unsigned long) "s\n");
	break;
    case CK_Debug_Back_Trace:
	debug_show_output ((unsigned long) "backtrace\n");
	break;
    case CK_Debug_Continue:
	debug_command ((unsigned long) "c\n");
	break;
    case CK_Debug_Enter_Command:
	debug_show_output (0L);
	break;
    case CK_Debug_Until_Curser:
	debug_until (0L);
    }
}




