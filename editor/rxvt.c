/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* rxvt.c - terminal emulator widget top-level functions
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include "rxvt/rxvtlib.h"
#include "coolwidget.h"
#include "rxvt.h"
#include "editoptions.h"
#include "find.h"
#include "font.h"
#include "xim.h"
#include "stringtools.h"

struct rxvt_startup_options rxvt_startup_options = {0, 1, 0, 0, 1, ""};

struct rxvts {
    rxvtlib *rxvt;
    struct rxvts *next;
    int killed;
} *rxvt_list = 0;

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
        if (!strcmp (l->rxvt->cterminal_io.host, "localhost"))
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
	    memcpy (&l->rxvt->xevent, xevent, sizeof (*xevent));
	    l->rxvt->fds_available++;
	    if (XFilterEvent(xevent, 0)) {
		xevent->type = 0;
		xevent->xany.window = 0;
		return 1;
	    }
	    rxvt_process_x_event (l->rxvt);
	    if (l->rxvt->killed) {
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

int rxvt_have_pid (const char *host, pid_t pid)
{E_
    struct rxvts *l;
    if (!rxvt_list || !pid)
	return 0;
    for (l = rxvt_list->next; l; l = l->next)
	if (l->rxvt->cmd_pid == pid && !strcmp (host, l->rxvt->cterminal_io.host))
	    return 1;
    return 0;
}

/* rxvt's need to interoperate */
static void rxvt_selection_clear (void)
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
    CPushFont ("rxvt", 0);
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

static rxvtlib *rxvt_allocate (const char *host, Window win, int c, char **a, int do_sleep, unsigned long rxvt_options)
{E_
    char errmsg[CTERMINAL_ERR_MSG_LEN];
    rxvtlib *rxvt;
    struct rxvts *l;
    struct rxvts *i;
    rxvt = (rxvtlib *) malloc (sizeof (rxvtlib));
    rxvtlib_init (rxvt, rxvt_options);
    user_selection_clear = (void (*)(void)) rxvt_selection_clear;
    rxvt->parent_window = win;

    if (!rxvt_list) {
	rxvt_list = l = malloc (sizeof (struct rxvts));
	rxvt_list->next = 0;
	rxvt_list->rxvt = 0;
	rxvt_list->killed = 0;
    }
    for (i = rxvt_list; i->next; i = i->next);
    l = (i->next = malloc (sizeof (struct rxvts)));
    l->next = 0;
    l->killed = 0;
    l->rxvt = rxvt;

    errmsg[0] = '\0';
    rxvtlib_main (rxvt, host, c, (const char *const *) a, do_sleep, errmsg);
    if (rxvt->killed) {
        assert (i->next == l);
        free (i->next);
        i->next = 0;
        if (errmsg[0])
	    CErrorDialog(0, 0, 0, _(" Open Terminal "), " Error trying to open terminal: \n [%s] ", errmsg);
        rxvtlib_destroy_windows (rxvt);
        rxvtlib_shut (rxvt);
	free (rxvt);
	return 0;
    }
    return rxvt;
}

extern char *init_font;

static char **rxvt_args (char **argv)
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

rxvtlib *rxvt_start (const char *host, Window win, char **argv, int do_sleep, unsigned long rxvt_options)
{E_
    int a = 0;
    rxvtlib *rxvt;
    char **b;
    b = rxvt_args (argv);
    while (b[a])
	a++;
    rxvt = rxvt_allocate (host, win, a, b, do_sleep, rxvt_options);
    if (rxvt) {
	rxvtlib_main_loop (rxvt);
	rxvtlib_update_screen (rxvt);
    }
    free (b);
    return rxvt;
}

void rxvt_kill (pid_t p)
{E_
    struct rxvts *l;
    if (!rxvt_list)
	return;
    for (l = rxvt_list->next; l; l = l->next)
	if (l->rxvt->cmd_pid == p)
            if (l->rxvt->cterminal_io.remotefs)
                (*l->rxvt->cterminal_io.remotefs->remotefs_shellkill) (l->rxvt->cterminal_io.remotefs, p);
}

void rxvt_get_tty_name (rxvtlib * rxvt, char *p)
{E_
    strcpy (p, rxvt->cterminal_io.ttydev);
}

void rxvt_get_pid (rxvtlib * rxvt, pid_t *pid, const char *host)
{E_
    *pid = rxvt->cmd_pid;
    assert (!strcmp (host, rxvt->cterminal_io.host));
}

#if 0
Window rxvt_get_main_window (rxvtlib *rxvt)
{E_
    return rxvt->TermWin.parent[0];
}
#endif

static int rxvt_startup_dialog_ (struct rxvt_startup_options *opt);

void save_options (void);
const char *get_default_editor_font_large (void);
const char *get_default_8bit_term_font_large (void);
void cooledit_main_loop (void);
char *get_all_lists (void);
extern char *editor_options_file;

int rxvt_startup_dialog (const char *host, char *shell_script)
{E_
    static enum font_encoding rxvt_8bit_encoding = FONT_ENCODING_8BIT;
    static enum font_encoding rxvt_encoding = FONT_ENCODING_UTF8;
    unsigned long rxvt_options = 0;

    remotefs_set_die_on_error ();

    if (host) {
        Cstrlcpy (rxvt_startup_options.host, host, sizeof (rxvt_startup_options.host));
    } else {
        char *p;
        if (rxvt_startup_dialog_ (&rxvt_startup_options))
            return -1;

        save_options ();
        save_options_section (editor_options_file, "[Input Histories]", p = get_all_lists ());
        free (p);
    }

    if (rxvt_startup_options.large_font) {
        CFontLazy ("rxvt", get_default_editor_font_large (), NULL, &rxvt_encoding);
        CFontLazy ("rxvt8bit", get_default_8bit_term_font_large (), NULL, &rxvt_8bit_encoding);
    }

    if (rxvt_startup_options.term_8bit) {
        rxvt_options |= RXVT_OPTIONS_TERM8BIT;
        CPushFont ("rxvt8bit", 0);
    } else {
        CPushFont ("rxvt", 0);
    }
    if (rxvt_startup_options.backspace_ctrl_h)
        rxvt_options |= RXVT_OPTIONS_BACKSPACE_CTRLH;
    if (rxvt_startup_options.backspace_127)
        rxvt_options |= RXVT_OPTIONS_BACKSPACE_127;
    if (rxvt_startup_options.x11_forwarding)
        rxvt_options |= RXVT_OPTIONS_X11_FORWARDING;

    if (shell_script && *shell_script) {
        char *arg[4] = {"sh", "-c", shell_script, NULL};
        rxvt_start (rxvt_startup_options.host, CRoot, arg, 0, rxvt_options);
    } else {
        rxvt_start (rxvt_startup_options.host, CRoot, 0, 0, rxvt_options);
    }

    while (rxvt_list && rxvt_list->next)
        cooledit_main_loop ();

    shutdown(ConnectionNumber (CDisplay), 2);
    close(ConnectionNumber (CDisplay));

    exit (remotefs_get_die_exit_code ());
}


static int rxvt_startup_dialog_ (struct rxvt_startup_options *opt)
{E_

    char *inputs[10] =
    {
        gettext_noop ("localhost"),
        0
    };
    char *input_labels[10] =
    {
        gettext_noop ("IP address of remote or 'localhost'"),
        0
    };
    char *check_labels[10] =
    {
        gettext_noop ("8-bit terminal"),
        gettext_noop ("Large font"),
        gettext_noop ("Force backspace to ^H"),
        gettext_noop ("Force backspace to ^?"),
        gettext_noop ("Enable X11 forwarding"),
        0
    };
    char *check_tool_hints[10] =
    {
        gettext_noop ("This is for legacy systems that do no support Unicode"),
        gettext_noop ("For explicitly specifying a font use the -font option on the command-line"),
        gettext_noop ("Some terminals have incorrect backspace interpretation,\nso force generation of a 0x8 code point on backspace"),
        gettext_noop ("Some terminals have incorrect backspace interpretation,\nso force generation of a 0x7F code point on backspace"),
        gettext_noop ("Will set the DISPLAY environment variable to forward\nX graphical applications back through the secure\nchannel to display locally"),
        0
    };
    int check_group[10] =
    {
        0,
        0,
        1,
        1,
        0,
    };
    char *input_names[10] =
    {
        gettext_noop ("rxvtremoteip"),
        0
    };
    char *input_tool_hint[10] =
    {
        gettext_noop ("The remote IP must be running remotefs (or REMOTEFS.EXE for MS Windows).\nUse the -remote option on the command-line to skip this dialog"),
        0
    };
    int *checks_values_result[10];
    char **inputs_result[10];
    int r;

    inputs_result[0] = &inputs[0];
    inputs_result[1] = 0;
    checks_values_result[0] = &opt->term_8bit;
    checks_values_result[1] = &opt->large_font;
    checks_values_result[2] = &opt->backspace_ctrl_h;
    checks_values_result[3] = &opt->backspace_127;
    checks_values_result[4] = &opt->x11_forwarding;
    checks_values_result[5] = 0;

    r = CInputsWithOptions (0, 0, 0, _ (" Start Terminal "), inputs_result, input_labels, input_names, input_tool_hint, checks_values_result, check_labels, check_tool_hints, check_group, 0, 60);
    if (r)
        return 1;
    Cstrlcpy (opt->host, inputs[0], sizeof (opt->host));
    return 0;
}













