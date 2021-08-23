/* smalledit.c - stripped down cooledit.c and demo of the Coolwidget library
   Copyright (C) 1996-2018 Paul Sheer
 */


/*
 * Comments:
 * 
 * This program consists of 5 sections:
 * 
 * - {{{ command-line options
 *     Processing of command-line using Coolwidget's command-line utility,
 *     well as declaration of some global variables.
 * - {{{ utilities
 *     Any string mamipulation functions needed.
 * - {{{ menu callbacks
 *     Actions taken by menu clicks.
 * - {{{ main
 *     Initialisation and main loop.
 * 
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"
#include "edit.h"
#include "editcmddef.h"
#include "loadfile.h"
#include "editoptions.h"
#include "cmdlineopt.h"
#include "shell.h"
#include "pool.h"

/* default window sizes */
#define START_WIDTH	80
#define START_HEIGHT	25

#undef gettext_noop
#define gettext_noop(x) x

/* {{{ command-line options */

#ifdef HAVE_DND
extern int option_dnd_version;
#endif

/* editor widget: */
CWidget *edit = 0;

/* main and first window */
Window main_window = 0;

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

extern int option_auto_spellcheck;

/* other things on the command line */
static char *command_line_args[] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void usage (void)
{
    printf ( \
	_("Smalledit version %s\nUsage:\n" \
	"A stripped down version of cooledit.\n" \
	"smalledit [options] [[+<line>] <file>]\n" \
	"-d, -display <display>                   the X server to display on\n" \
	"-f, -fn, -font <font-name>               default: 8x13bold\n" \
	"-wordwrap, --word-wrap <length>          default: 72\n" \
	"-typew, --type-writer                    type-writer word wrap\n" \
	"-spell, --spell-checking                 spell-checking enabled\n" \
	"-autop, --auto-paragraph                 word processor paragraphing\n" \
	"-i, --international-characters           not longer an option. always display.\n" \
	"-dnd-old                                 use dnd version 0 instead of version 1\n" \
	"-t, -tab, --tab-spacing <spacing>        set tab spacing\n" \
	"-h, -H, -?, --help                       print this message to stdout\n" \
	"-V, -v, --version                        print versiom info\n" \
	"\n"), \
	VERSION);
}

void version (void)
{
    printf (_("Smalledit version %s\n"), VERSION);
}

struct prog_options cooledit_options[] =
{
    {' ', "", "", ARG_STRINGS, 0, command_line_args, 0},
#ifdef HAVE_DND
    {0, "-dnd-old", "--dnd-old", ARG_CLEAR, 0, 0, &option_dnd_version},
#endif
    {'f', "-fn", "-font", ARG_STRING, &option_font2, 0, 0},
    {0, "-wordwrap", "--word-wrap", ARG_INT, 0, 0, &option_word_wrap_line_length},
    {0, "-typew", "--type-writer", ARG_SET, 0, 0, &option_typewriter_wrap},
    {0, "-autop", "--auto-paragraph", ARG_SET, 0, 0, &option_auto_para_formatting},
    {0, "-spell", "--spell-checking", ARG_SET, 0, 0, &option_auto_spellcheck},
    {'t', "-tab", "--tab-spacing", ARG_INT, 0, 0, &option_tab_spacing},
    {'h', "-?", "--help", ARG_SET, 0, 0, &get_help},
    {'H', "-help", "--help", ARG_SET, 0, 0, &get_help},
    {'V', "-v", "--version", ARG_SET, 0, 0, &get_version},
    {'d', "", "-display", ARG_STRING, &option_display, 0, 0},
    {0, 0, 0, 0, 0, 0, 0}
};

static struct cmdline_option_free cmdline_fl;

/* here we use our own function (which is better than get_opt() or get_opt_long()) */
static void process_command_line (int argc, char **argv)
{
    int error;
    option_auto_spellcheck = 0;
    error = get_cmdline_options (argc, argv, cooledit_options, &cmdline_fl);

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

/* {{{ utilities */

static char *get_full_filename (const char *f)
{
    char *s, *p;

    if (*f == '/')
	return (char *) f;

    s = malloc (2048);
    if (getcwd (s, 2000)) {
	p = (char *) strdup (catstrs (s, "/", f, NULL));
	free (s);
	return p;
    }
    free (s);
    return 0;
}

/* }}} utilities */


/* {{{ menu callbacks */

void run_main_callback (unsigned long ignore)
{
    switch (fork ()) {
    case 0:
	CDisableAlarm ();
	execlp (argv_nought, argv_nought, "-f", init_font, NULL);
	exit (1);
    case -1:
	CErrorDialog (0, 0, 0, _(" Run 'smalledit' "), get_sys_error (_(" Error trying to fork process ")));
	return;
    default:
	return;
    }
}

void menu_exit (unsigned long ignore)
{
    CEditMenuCommand (CK_Exit);
}

void save_and_exit_callback (unsigned long ignore)
{
    CEditMenuCommand (CK_Save);
    menu_exit (0);
}

int about_callback (CWidget *w, XEvent *xe, CEvent *ce)
{
    CMessageDialog (main_window, 20, 20, TEXT_CENTRED, _(" About "),
      _("\n" \
      "Smalledit  version  %s\n" \
      "\n" \
      "A cut down version of Cooledit -\n" \
      "a text editor written for The X Window System.\n" \
      "\n" \
      "Copyright (C) 1996-2017 Paul Sheer\n" \
      "\n" \
      " Smalledit comes with ABSOLUTELY NO WARRANTY; for details \n" \
      " click on 'no Warranty' in the File menu. \n" \
      " This is free software, and you are welcome to redistribute it under \n" \
      " certain conditions; click on 'Copying' for details. \n" \
      " Smalledit is meant to demonstrate the Coolwidget X Window Library. \n" \
      " Coolwidget is a high level API that contains all basic \n" \
      " widgets and dialogs needed for writing X applications. \n" \
      " This editor was written in only 400 lines. \n") , VERSION);
    CFocus (edit);
    return 1;
}


/* }}} menu callbacks */

void load_trivial_options (void);
extern Atom ATOM_WM_NAME;

/* {{{ change my name by sasha*/

void set_main_window_name( char* filename )
{
  char* full_title ;
  CWidget *wdt = CWidgetOfWindow( main_window );
    
    full_title = malloc( 8+ strlen(filename)+1);
    sprintf( full_title, "edit: %s", filename );
    if( wdt->label ) free( wdt->label );
    wdt->label = full_title ;
    XSetIconName (CDisplay, main_window, wdt->label);
    XStoreName (CDisplay, main_window, wdt->label);
    XChangeProperty (CDisplay, main_window, ATOM_WM_NAME, XA_STRING, 8,
		     PropModeReplace, (unsigned char *) full_title, strlen (full_title));
}
/* }}} change my name by sasha */

/* {{{ main */

int main (int argc, char **argv)
{
    CInitData cooledit_startup;
    CEvent cwevent;
    XEvent xevent;
    char *window_title;
    int columns, lines, x, x2, y, l = 0;

    window_title = (char *) strdup ("");

    columns = START_WIDTH;
    lines = START_HEIGHT;

/* needed for execlp */
    argv_nought = (char *) strdup (argv[0]);

/* locale settings */
#ifdef LC_CTYPE
    if (!setlocale (LC_CTYPE, ""))
	fprintf (stderr, _ ("%s: cannot set locale, see setlocale(3), current locale is %s\n"), argv_nought, setlocale (LC_MESSAGES, 0));
    setlocale (LC_TIME, "");
    setlocale (LC_MESSAGES, "");
#endif
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);

/* load some options from /.cedit/.cooledit.ini */
    load_trivial_options ();

/* use our special utility */
    process_command_line (argc, argv);

    get_home_dir ();

/* intialise the library */
    memset (&cooledit_startup, 0, sizeof (cooledit_startup));
    cooledit_startup.name = argv_nought;
    cooledit_startup.display = option_display;
    cooledit_startup.font = option_font2;
    CInitialise (&cooledit_startup);

/* create main window */
    main_window = CDrawMainWindow ("smalledit", "Smalledit");

/* draw the predefined edit menu buttons */
    CGetHintPos (&x, &y);
    CDrawEditMenuButtons ("smalledit", main_window, main_window, x, y);

/* add some buttons of our own */
    CAddMenuItem ("smalledit.filemenu", "", ' ', 0, 0);
    CAddMenuItem ("smalledit.filemenu", "", ' ', 0, 0);
    CAddMenuItem ("smalledit.filemenu",
		  _ ("New main window\tShift-F3"), 'm', run_main_callback, 0);
    CAddMenuItem ("smalledit.filemenu",
		  _ ("Exit\tF10"), 'E', menu_exit, 0);
    CAddMenuItem ("smalledit.filemenu",
	       _ ("Save and exit\tAlt-x"), 'x', save_and_exit_callback, 0);

    CGetHintPos (&x2, 0);
    CDrawButton ("about", main_window, x2, y, AUTO_SIZE, _ (" About "));
    CAddCallback ("about", about_callback);

    CGetHintPos (0, &y);

/* draw the editor widget */
    CPushFont ("editor", 0);
    if (command_line_args[0]) {
	char *q;
	int i = 0;
	if (command_line_args[0][0] == '+') {
	    l = atoi (command_line_args[0] + 1) - 1;
	    if (l < 0)
		l = 0;
	    i = 1;
	}
	q = get_full_filename (command_line_args[i]);
        set_main_window_name( q );
	edit = CDrawEditor ("editor", main_window, x, y, columns * FONT_MEAN_WIDTH,
			    lines * FONT_PIX_PER_LINE, 0, q, 0, 0, 0, 0);
    }
    if (!edit)
	edit = CDrawEditor ("editor", main_window, x, y, columns * FONT_MEAN_WIDTH,
	 lines * FONT_PIX_PER_LINE, "", 0, get_full_filename (""), 0, 0, 0);
    CPopFont ();

/* move to line */
    if (edit && l) {
	edit_move_display (edit->editor, l);
	edit_move_to_line (edit->editor, l);
    }
/* the edit menu refers to this editor */
    CSetEditMenu (edit->ident);

/* first menu to pull up */
    CSetLastMenu (CIdent ("smalledit.filemenu"));

/* set widget behaviour on resizing of main window */
    edit->position |= (POSITION_WIDTH | POSITION_HEIGHT);
    (CIdent ("editor.text"))->position |= (POSITION_BOTTOM | POSITION_WIDTH);
    (CIdent ("editor.vsc"))->position |= (POSITION_RIGHT | POSITION_HEIGHT);

/* resize main window to hold all widgets */
    CSetSizeHintPos ("smalledit");

/* set min and max window sizes */
    CSetWindowResizable ("smalledit", 200, 200, 3200, 2400);

/* show the window */
    CMapDialog ("smalledit");

/* now run */
    for (;;) {
	CNextEvent (&xevent, &cwevent);
	if (edit) {
	    if (edit->editor->stopped)
		break;
	} else
	    break;

/* skip these events */
	if (xevent.type == Expose || !xevent.type || xevent.type == AlarmEvent
	    || xevent.type == InternalExpose || xevent.type == TickEvent)
	    continue;

	if (edit)
	    if (edit->editor->filename)
		if (strcmp (window_title, edit->editor->filename)) {
		    free (window_title);
		    window_title = (char *) strdup (edit->editor->filename);
		    set_main_window_name( window_title );
		}
	switch (xevent.type) {
	case FocusIn:
	    if (xevent.xany.window == main_window) {
		CFocus (edit);
		continue;
	    }
	case KeyPress:
	    if (cwevent.kind == C_EDITOR_WIDGET || cwevent.kind == C_MENU_BUTTON_WIDGET) {
		if (cwevent.kind != C_MENU_BUTTON_WIDGET) {
		    switch ((int) cwevent.command) {
		    case CK_Menu:	/* pull down the menu */
			CMenuSelectionDialog (CGetLastMenu ());
			break;
		    case CK_Run_Another:	/* new editor window */
			run_main_callback (0);
			break;
		    case CK_Check_Save_And_Quit:
			menu_exit (0);
			break;
		    case CK_Save_And_Quit:
			save_and_exit_callback (0);
		    }
		    break;
		}
	    }
	    break;
	}
	if (xevent.type == QuitApplication) {	/* recieved because the user did a close with the WM. */
	    int i;
	    if (!edit->editor->modified) {
		menu_exit (0);
	    } else {
		i = CQueryDialog (0, 0, 0,
				  _ (" Quit "), _ (" Quit Smalledit ? "), _ (" Cancel quit "), _ (" Quit and save all "),
				  _ (" Quit, do not save "), NULL);
		switch (i) {
		case 0:
		    break;
		case 1:
		    save_and_exit_callback (0);
		    break;
		case 2:
		    CDestroyWidget (edit->ident);
		    edit = 0;
		    break;
		}
		continue;
	    }
	}
    }

/* close connection to the X display */
    CShutdown ();
    get_cmdline_options_free_list (&cmdline_fl);
    return 0;
}

/* }}} main */

