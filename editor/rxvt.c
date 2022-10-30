/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* rxvt.c - terminal emulator widget top-level functions
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include "rxvt/rxvtlib.h"
#include "coolwidget.h"
#include "font.h"
#include "xim.h"

struct rxvts {
    rxvtlib *rxvt;
    struct rxvts *next;
    int killed;
} *rxvt_list = 0;

extern void rxvt_fd_write_watch (int fd, fd_set * reading, fd_set * writing, fd_set * error,
				 void *data);
extern void rxvt_fd_read_watch (int fd, fd_set * reading, fd_set * writing, fd_set * error,
				void *data);

int rxvt_event (XEvent * xevent)
{E_
    pid_t mine = 0;
    Window win;
    struct rxvts *l, *prev = 0;
    if (!rxvt_list)
	return 0;
    if (!mine)
        mine = getpid ();
    win = xevent->xany.window;
    for (l = rxvt_list->next, prev = rxvt_list; l; l = l->next) {
#warning finish socket death must be the same as CChildExitted()
        if (!strcmp (l->rxvt->host, "localhost"))
 	    l->killed |= CChildExitted (l->rxvt->cmd_pid, 0);
	if (l->killed || l->rxvt->killed) {
	    struct rxvts *next;
	    next = l->next;
	    CRemoveWatch (l->rxvt->cmd_fd, NULL, 3);
            rxvtlib_destroy_windows (l->rxvt);
	    prev->next = next;
	    rxvtlib_shut (l->rxvt);
	    free (l->rxvt);
	    memset (l, 0, sizeof (*l));
	    free (l);
	    l = prev;
	} else if (win && (l->rxvt->TermWin.vt == win
			   || l->rxvt->TermWin.parent[0] == win
			   || l->rxvt->TermWin.parent[1] == win
			   || l->rxvt->TermWin.parent[2] == win
			   || l->rxvt->TermWin.parent[3] == win
			   || l->rxvt->scrollBar.win == win
			   || l->rxvt->menuBar.win == win)) {
	    l->rxvt->x_events_pending = 1;
	    memcpy (&l->rxvt->xevent, xevent, sizeof (*xevent));
	    l->rxvt->fds_available++;
	    if (XFilterEvent(xevent, 0)) {
		xevent->type = 0;
		xevent->xany.window = 0;
		return 1;
	    }
	    rxvt_process_x_event (l->rxvt);
	    if (l->rxvt->killed) {
printf("%s:%d: CRemoveWatch both\n", __FILE__, __LINE__);
		CRemoveWatch (l->rxvt->cmd_fd, NULL, 3);
		return 1;
	    }
	    rxvtlib_update_screen (l->rxvt);
	    if (win != DefaultRootWindow (l->rxvt->Xdisplay)
		&& xevent->type != ClientMessage
		&& xevent->type != SelectionNotify
		&& xevent->type != SelectionRequest
		&& xevent->type != SelectionClear) {
		xevent->type = 0;
		xevent->xany.window = 0;
		return 1;
	    }
	    return 0;
	}
	prev = l;
    }
    return 0;
}

int rxvt_have_pid (pid_t pid)
{E_
#warning finish - the pid is remote
    struct rxvts *l;
    if (!rxvt_list)
	return 0;
    for (l = rxvt_list->next; l; l = l->next)
	if (l->rxvt->cmd_pid == pid)
	    return 1;
    return 0;
}

/* rxvt's need to interoperate */
void rxvt_selection_clear (void)
{E_
    struct rxvts *l;
    if (!rxvt_list)
	return;
    for (l = rxvt_list->next; l; l = l->next) {
	rxvtlib *o;
	o = l->rxvt;
	if (o->selection.text)
	    FREE (o->selection.text);
	o->selection.text = NULL;
	o->selection.len = 0;
    }
    return;
}

/* Case 1: create an input context on first creation of the rxvt terminal for the case when input method registration has ALREADY happened */
void rxvt_set_input_context (rxvtlib *o, XIMStyle input_style)
{
    CPushFont ("rxvt");
    create_input_context_ ("rxvt", input_style, &o->Input_Context, o->TermWin.parent[0]);
    CPopFont ();
}

/* Case 2: create an input context when a new input method becomes available AFTER the rxvt terminal has was created */
static void rxvt_set_input_context_cb (XIMStyle input_style)
{
    struct rxvts *l;
    if (!rxvt_list)
	return;
    for (l = rxvt_list->next; l; l = l->next) {
        rxvt_set_input_context (l->rxvt, input_style);
    }
}

static void rxvt_destroy_input_context_cb (void)
{
    struct rxvts *l;
    if (!rxvt_list)
	return;
    for (l = rxvt_list->next; l; l = l->next) {
        destroy_input_context_ (&l->rxvt->Input_Context);
    }
}

void rxvt_init (void)
{
    xim_set_input_cb (rxvt_set_input_context_cb, rxvt_destroy_input_context_cb);
}

#if 0
int rxvt_alive (pid_t p)
{E_
    struct rxvts *l;
    if (!rxvt_list)
	return 0;
    for (l = rxvt_list->next; l; l = l->next)
	if (l->rxvt->cmd_pid == p && !l->killed && !l->rxvt->killed)
	    return 1;
    return 0;
}
#endif

extern void (*user_selection_clear) (void);

static rxvtlib *rxvt_allocate (const char *host, Window win, int c, char **a, int do_sleep, int charset_8bit)
{E_
    char errmsg[CTERMINAL_ERR_MSG_LEN];
    rxvtlib *rxvt;
    struct rxvts *l;
    rxvt = (rxvtlib *) malloc (sizeof (rxvtlib));
    rxvtlib_init (rxvt, charset_8bit);
    user_selection_clear = (void (*)(void)) rxvt_selection_clear;
    rxvt->parent_window = win;
    errmsg[0] = '\0';
    rxvtlib_main (rxvt, host, c, (const char *const *) a, do_sleep, errmsg);
    if (rxvt->killed) {
        if (errmsg[0])
	    CErrorDialog(0, 0, 0, _(" Open Terminal "), " Error trying to open terminal: \n [%s] ", errmsg);
        rxvtlib_destroy_windows (rxvt);
	free (rxvt);
	return 0;
    }
    if (!rxvt_list) {
	rxvt_list = l = malloc (sizeof (struct rxvts));
	rxvt_list->next = 0;
	rxvt_list->rxvt = 0;
	rxvt_list->killed = 0;
    }
    for (l = rxvt_list; l->next; l = l->next);
    l = (l->next = malloc (sizeof (struct rxvts)));
    l->next = 0;
    l->killed = 0;
    l->rxvt = rxvt;
    return rxvt;
}

extern char *init_font;

char **rxvt_args (char **argv)
{E_
    char **a;
    char *b[] =
	{ "rxvt", "-fg", "white", "-bg", "black", "-font", "8x13bold", "-sl", "30000", "-si", "+sk", "-e", 0 };
    int i = 0, j, k;
    if (argv)
	for (i = 0; argv[i]; i++);
    CPushFont ("editor", 0);
    if (CIsFixedFont () && init_font) {
	for (k = 0; b[k] && strcmp (b[k], "-font"); k++);
	if (k != i)
	    b[k + 1] = init_font;
    }
    CPopFont ();
    for (j = 0; b[j]; j++);
    a = malloc ((i + j + 1) * sizeof (char *));
    memcpy (a, b, j * sizeof (char *));
    if (argv)
	memcpy (a + j, argv, (i + 1) * sizeof (char *));
    if (!i)
	a[--j] = 0;		/* shell */
    return a;
}

#if 0
void rxvt_resize_window (rxvtlib * rxvt, int w, int h)
{E_
    XResizeWindow (rxvt->Xdisplay, rxvt->TermWin.parent[0], w, h);
    rxvtlib_resize_window (rxvt, w, h);
}
#endif

void rxvtlib_shutall (void)
{E_
    struct rxvts *l;
    if (!rxvt_list)
	return;
    l = rxvt_list->next;
    free (rxvt_list);
    rxvt_list = 0;
    while (l) {
	struct rxvts *next;
	next = l->next;
	CRemoveWatch (l->rxvt->cmd_fd, 0, 3);
	XDestroyWindow (l->rxvt->Xdisplay, l->rxvt->TermWin.parent[0]);
	XDestroyWindow (l->rxvt->Xdisplay, l->rxvt->TermWin.vt);
	XDestroyWindow (l->rxvt->Xdisplay, l->rxvt->scrollBar.win);
	rxvtlib_shut (l->rxvt);
	free (l->rxvt);
	memset (l, 0, sizeof (*l));
	free (l);
	l = next;
    }
}

rxvtlib *rxvt_start (const char *host, Window win, char **argv, int do_sleep, int charset_8bit)
{E_
    int a = 0;
    rxvtlib *rxvt;
    char **b;
    b = rxvt_args (argv);
    while (b[a])
	a++;
    rxvt = rxvt_allocate (host, win, a, b, do_sleep, charset_8bit);
    if (rxvt) {
	rxvtlib_main_loop (rxvt);
	rxvtlib_update_screen (rxvt);
    }
    free (b);
    return rxvt;
}

void rxvt_get_tty_name (rxvtlib * rxvt, char *p)
{E_
    strcpy (p, rxvt->ttydev);
}

void rxvt_get_pid_host (rxvtlib * rxvt, pid_t *pid, char *host, int host_len)
{E_
    *pid = rxvt->cmd_pid;
    Cstrlcpy (host, rxvt->host, host_len);
}

#if 0
Window rxvt_get_main_window (rxvtlib *rxvt)
{E_
    return rxvt->TermWin.parent[0];
}
#endif

