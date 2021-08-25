/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* coolman.c - displays man pages using the man system command
   Copyright (C) 1996-2022 Paul Sheer
 */


/*
 * Comments:
 * 
 * This program consists of 5 sections:
 * 
 * - {{{ command-line options
 *     Processing of command-line using Coolwidget's command-line utility,
 *     well as declaration of some global variables.
 * - {{{ menu callbacks
 *     Actions taken by menu clicks.
 * - {{{ main
 *     Initialisation and main loop.
 * 
 */

#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#include "coolwidget.h"
#include "manpage.h"
#include "cmdlineopt.h"
#include "stringtools.h"

/* {{{ command-line options */
#ifdef HAVE_DND
extern int option_dnd_version;
#endif

/* options file */
char *editor_options_file = 0;

/* main and first window */
Window main_window = 0;

/* shell command to run */
extern char *option_man_cmdline;

/* argv[0] */
char *argv_nought = 0;

/* font from the library */
extern char *init_font;

/* server from command line */
char *option_display = 0;

/* font from the command line */
char *option_font2 = 0;

static int get_help = 0;
static int get_version = 0;

int option_utf_interpretation2 = 1;
int option_locale_encoding = 0;

/* other things on the command line */
static char *command_line_args[] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void usage (void)
{E_
    printf ( \
	_("Coolman version %s\nUsage:\n" \
	"coolman [options] <man-page>\n" \
	"-d, -display <display>                   the X server to display on\n" \
	"-f, -fn, -font <font-name>               default: 8x13bold\n" \
	"-dnd-old                                 use dnd version 0 instead of version 1\n" \
	"-c, -cmd, --command <man-command>        how to run man, default: \"man -a %%m\"\n" \
	"-h, -H, -?, --help                       print this message to stdout\n" \
	"-V, -v, --version                        print versiom info\n" \
	"\n"), \
	VERSION);
}

void version (void)
{E_
    printf (_("Coolman version %s\n"), VERSION);
}

struct prog_options coolman_options[] =
{
    {' ', "", "", ARG_STRINGS, 0, command_line_args, 0},
#ifdef HAVE_DND
    {0, "-dnd-old", "--dnd-old", ARG_CLEAR, 0, 0, &option_dnd_version},
#endif
    {'c', "-cmd", "--command", ARG_STRING, &option_man_cmdline, 0, 0},
    {'f', "-fn", "-font", ARG_STRING, &option_font2, 0, 0},
    {'h', "-?", "--help", ARG_SET, 0, 0, &get_help},
    {'H', "-help", "--help", ARG_SET, 0, 0, &get_help},
    {'V', "-v", "--version", ARG_SET, 0, 0, &get_version},
    {'d', "", "-display", ARG_STRING, &option_display, 0, 0},
    {0, 0, 0, 0, 0, 0, 0}
};

static struct cmdline_option_free cmdline_fl;

/* here we use our own function (which is better than get_opt() or get_opt_long()) */
static void process_command_line (int argc, char **argv)
{E_
    int error;
    error = get_cmdline_options (argc, argv, coolman_options, &cmdline_fl);

    if (error) {
	fprintf (stderr, _("%s: error processing commandline argument %d\n"), argv[0], error);
	usage ();
	exit (1);
    }
    if (get_help)
	usage ();
    if (get_version)
	version ();
    if (get_help || get_version) {
        get_cmdline_options_free_list (&cmdline_fl);
	exit (0);
    }
}

/* }}} command-line options */


/* {{{ menu callbacks */

int mansearch_callback (CWidget * w, XEvent * x, CEvent * c);
int open_man (char *def);

int help_callback (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    return open_man ("coolman");
}

int run_main_callback (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    switch (fork ()) {
    case 0:
	CDisableAlarm ();
	execlp (argv_nought, argv_nought, "-f", init_font, "-c", option_man_cmdline, 
#ifdef HAVE_DND
		option_dnd_version ? 0 : "-dnd-old", 
#endif
	NULL);
	exit (1);
    case -1:
	CErrorDialog (0, 0, 0, _(" Run 'coolman' "), get_sys_error (_(" Error trying to fork process ")));
	CFocus (CIdent ("mandisplayfile.text"));
	return 1;
    }
    CFocus (CIdent ("mandisplayfile.text"));
    return 1;
}

int quit_callback (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    CShutdown ();
    exit (0);
    return 1;
}

int about_callback (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    CMessageDialog (main_window, 20, 20, TEXT_CENTRED, _(" About "),
		    _("\n" \
		    "Coolman  version  %s\n" \
		    "\n" \
		    "A man page reader\n" \
		    "\n" \
		    "Copyright (C) 1996-2022 Paul Sheer\n" \
		    "\n" \
	   " Coolman comes with ABSOLUTELY NO WARRANTY; for details \n" \
	" see the file COPYING in the source distribution. \n"), VERSION);
    CFocus (man);
    return 1;
}

/* }}} menu callbacks */


void load_trivial_options (void);

/* {{{ main */

int main (int argc, char **argv)
{
    CInitData coolman_startup;
    CEvent cwevent;
    XEvent xevent;
    int x, x2, y;

#ifdef LC_CTYPE 
    setlocale (LC_CTYPE, "");
    setlocale (LC_TIME, "");
    setlocale (LC_MESSAGES, "");
#endif
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);

/* needed for execlp */
    argv_nought = (char *) strdup (argv[0]);

/* load some options from /.cedit/.cooledit.ini */
    load_trivial_options ();

/* use our special utility */
    process_command_line (argc, argv);

    get_home_dir ();

/* intialise the library */
    memset (&coolman_startup, 0, sizeof (coolman_startup));
    coolman_startup.name = argv_nought;
    coolman_startup.display = option_display;
    coolman_startup.font = option_font2;
    CInitialise (&coolman_startup);

    set_editor_encoding (option_utf_interpretation2, option_locale_encoding);

/* create main window */
    main_window = CDrawMainWindow ("coolman", "Coolman");

/* draw the predefined edit menu buttons */
    CGetHintPos (&x, &y);
    (CDrawButton ("new", main_window, x, y, AUTO_SIZE, _(" New Window ")))->hotkey = 'W';
    CAddCallback ("new", run_main_callback);
    CGetHintPos (&x2, 0);
    (CDrawButton ("about", main_window, x2, y, AUTO_SIZE, _(" About ")))->hotkey = 'b';
    CAddCallback ("about", about_callback);
    CGetHintPos (&x2, 0);
    CDrawButton ("quit", main_window, x2, y, AUTO_SIZE, _(" Quit "));
    CAddCallback ("quit", quit_callback);
    CGetHintPos (&x2, 0);
    CDrawButton ("help", main_window, x2, y, AUTO_SIZE, _(" Help "));
    CAddCallback ("help", help_callback);
    CGetHintPos (0, &y);

/* draw the man page textbox */
    if (command_line_args[0]) {
	man = CManpageDialog (main_window, x, y, START_WIDTH, START_HEIGHT, command_line_args[0]);
    } else {
	char *page;
	page = CInputDialog ("getman", 0, 0, 0, 200, "", _(" Open New Window "), _(" Enter a man page to open : "));
	if (!page)
	    goto fin;
	if (!*page)
	    goto fin;
	man = CManpageDialog (main_window, x, y, START_WIDTH, START_HEIGHT, page);
	free (page);
    }

/* resize main window to hold all widgets */
    CSetSizeHintPos ("coolman");

/* set min and max window sizes */
    CSetWindowResizable ("coolman", 200, 200, 3200, 2400);

/* show the window */
    CMapDialog ("coolman");

/* now run */
    for (;;) {
	CNextEvent (&xevent, &cwevent);
/* skip these events */
	if (xevent.type == Expose || !xevent.type || xevent.type == AlarmEvent
	    || xevent.type == InternalExpose || xevent.type == TickEvent)
	    continue;

	switch (xevent.type) {
	case FocusIn:
	    if (xevent.xany.window == main_window) {
		CFocus (man);
		continue;
	    }
	case QuitApplication:	/* recieved because the user did a close with the WM. */
	    goto fin;
	    break;
	case KeyPress:
	    if (cwevent.key == XK_F10)
		goto fin;
	    if (cwevent.key == '/')
		mansearch_callback (0, 0, 0);
	    break;
	}
    }

  fin:
/* close connection to the X display */
    CShutdown ();
    get_cmdline_options_free_list (&cmdline_fl);
    return 0;
}

/* }}} main */

