/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* cooledit.c - main file of cooledit
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "stringtools.h"
#include "coolwidget.h"
#include "edit.h"
#include "editcmddef.h"
#include "loadfile.h"
#include "editoptions.h"
#include "cmdlineopt.h"
#include "shell.h"
#include "igloo.h"
#include "widget/pool.h"
#include "rxvt/rxvtexport.h"
#include "debug.h"
#include "find.h"
#include "aafont.h"
#include "postscript.h"
#include "remotefs.h"
#include "remotefspassword.h"

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif


extern struct look *look;

int start_width = 80;
int start_height = 30;
char *option_look = "cool";

/* error handler */
#ifdef DEBUG
int er_handler (Display * c, XErrorEvent * e)
{E_
/* NLS ? */
    char err[128];
    XGetErrorText (c, e->error_code, err, 128);
    if (e->request_code == 42)	/* input focus errors are skipped */
	return 0;
    fprintf (stderr, "cooledit: Error detected.\n  %s\n", err);
    fprintf (stderr, "  Protocal request: %d\n", e->request_code);
    fprintf (stderr, "  Resource ID:      0x%x\n", (unsigned int) e->resourceid);
    fprintf (stderr, "Ignoring error.\n");
    return 0;
}
#endif

#if (RETSIGTYPE==void)
#define handler_return return
#else
#define handler_return return 1
#endif

extern Atom ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW, ATOM_WM_NAME, ATOM_WM_NORMAL_HINTS;
char *argv_nought = "cooledit";
Window main_window;

/* {{{ signal handling */

static void main_loop (void);

static RETSIGTYPE userone_handler (int x)
{E_
    CErrorDialog (main_window, 20, 20, \
/* heads the dialog box for when a SIGUSR1 Signal is recieved: SIGUSR1 */
		  _(" Signal SIGUSR1 Recieved "), 
		_("%s: SIGUSR1 recieved\n" \
	"You may have interrupted Cooledit because a search was taking too\n" \
	"long, or because you executed a recursive macro. In the latter\n" \
	"case, you should exit ASAP. Otherwise, if nothing peculiar was happening\n" \
	"Cooledit is probably still stable. Cooledit will now continue executing"), \
	argv_nought);
    signal (SIGUSR1, userone_handler);
    CEnable ("*");
    XUngrabPointer (CDisplay, CurrentTime);
    main_loop ();		/* continue */
    exit (0);
    handler_return;
}

static RETSIGTYPE quit_handler (int x)
{E_
/* %s = cooledit */
    fprintf (stderr, _("%s: quiting without saving\n"), argv_nought);
#if 0
    CShutdown ();		/* needed just to close the connection to the display */
#else
    shutdown(ConnectionNumber (CDisplay), 2);
    close(ConnectionNumber (CDisplay));
/*    XCloseDisplay (CDisplay); sometimes core dumps */
#endif
    exit (0);
    handler_return;
}

void set_to_kill (pid_t p);
void shell_output_set_to_kill (pid_t p);

static void set_signals (void)
{E_
    signal (SIGPIPE, SIG_IGN);
    signal (SIGHUP, SIG_IGN);
    signal (SIGSTOP, SIG_IGN);
    signal (SIGUSR1, userone_handler);
#if 1
    signal (SIGQUIT, quit_handler);
    signal (SIGINT, quit_handler);
    signal (SIGTERM, quit_handler);
#else
    signal (SIGQUIT, SIG_DFL);
    signal (SIGINT, SIG_DFL);
    signal (SIGTERM, SIG_DFL);
#endif
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
#endif				/* SIGTSTP */
}

/* }}} signal handling */

/* {{{ NLS support */

struct linguas {
    char *name;
    char *abbr;
} languages[] = {
    {"Chinese", "zh"},
    {"Czech", "cs"},
    {"Danish", "da"},
    {"Dutch", "nl"},
    {"English", "en"},
    {"Esperanto", "eo"},
    {"Finnish", "fi"},
    {"French", "fr"},
    {"German", "de"},
    {"Hungarian", "hu"},
    {"Irish", "ga"},
    {"Italian", "it"},
    {"Indonesian", "id"},
    {"Japanese", "ja"},
    {"Korean", "ko"},
    {"Latin", "la"},
    {"Norwegian", "no"},
    {"Persian", "fa"},
    {"Polish", "pl"},
    {"Portuguese", "pt"},
    {"Russian", "ru"},
    {"Slovenian", "sl"},
    {"Spanish", "es"},
    {"Swedish", "sv"},
    {"Turkish", "tr"}
};


/* }}} NLS support */


/* {{{ hint message on title bar */

void get_next_hint_message (void)
{E_
    static int i = -1;
    static int n;
    char label[128];
    static char *hints[] =
    {
/* HINTSTART */
/* The following are hints that go on the title bar: eg "Cooledit - Hint: Undo key-for-key with Ctrl-Backspace" */
	gettext_noop("To drag and drop, highlight text, then click and drag from within the selection"),
	gettext_noop("Dragging with the right mouse button will move text"),
	gettext_noop("Dragging with the left mouse button will copy text"),
	gettext_noop("Use Shift-Insert to copy text from other applications"),
	gettext_noop("Use Alt-Insert to bring up a history of selected text"),
	gettext_noop("Function keys F11-F20 mean Shift with a function key F1-F10"),
	gettext_noop("See how to do regular expression substring replacements in the man page"),
	gettext_noop("Use Shift-Up/Down in input widgets to get a history of inputs"),
	gettext_noop("Setup your personal file list ~/.cedit/filelist so that Jump To File works"),
	gettext_noop("Use Ctrl-b to highlight columns of text"),
	gettext_noop("Run frequently used apps from a hot key: see the Scripts menu"),
	gettext_noop("Get your keypad to work by redefining keys in the Options menu"),
	gettext_noop("Drag a file name from the 'File browser' in the Window menu to insert it"),
	gettext_noop("Use a macro to record repeatedly used key sequences - see the Command menu"),
	gettext_noop("Use a TTF, OTF or PCF font directly from a file using -fn myfont.ttf:18"),
	gettext_noop("Get a list of demo fonts with:  cooledit -font h"),
	gettext_noop("On Linux add XKBOPTIONS=\"compose:ralt\" to /etc/default/keyboard for key composing"),
	gettext_noop("Undo key-for-key with Ctrl-Backspace"),
	gettext_noop("Drag function prototypes and other info from the man page browser"),
	gettext_noop("Double click on words in the man page browser to search for new man pages"),
	gettext_noop("Search for [Options] in  ~/.cedit/.cooledit.ini  and change settings"),
	gettext_noop("Edit the example Scripts to see how to create your own"),
	gettext_noop("Double click on gcc error messages after running Make or Cc"),
	gettext_noop("Turn off these messages from the Options menu: 'Hint time...' = 0"),
	gettext_noop("Hold down the Ctrl key while mouse-highlighting text to manipulate columns"),
	gettext_noop("You can drag and drop columns of text between windows with the Ctrl key"),
	gettext_noop("See cooledit.1 for a list of the many extended editing/movement keys"),
	gettext_noop("Compose international characters with Right-Control"),
	gettext_noop("Insert (hexa)decimal literals with Control-q and the (hexa)decimal digits (then h)"),
	gettext_noop("Read all these hint messages in editor/cooledit.c in the source distribution"),
	gettext_noop("Use drag-and-drop to and from input widgets"),
	gettext_noop("Open a file by dragging its name from the 'File browser' onto the background"),
	gettext_noop("Use Shift-Ins to paste into input widgets"),
	gettext_noop("If Cooledit halts, restore by sending SIGUSR1:   kill -1 <pid>"),
	gettext_noop("For fun, hit Ctrl-Shift-Alt-~ to see how text is redrawn"),
	gettext_noop("Turn off the toolbar on new windows from the 'Options' menu"),
	gettext_noop("Disable the 'Window' menu from the 'Options' menu"),
	gettext_noop("If a macro is defined to an unused key, it will execute without having to use Ctrl-A"),
	gettext_noop("Select *columns* of text by holding the control key down through mouse highlighting"),
	gettext_noop("Use Ctrl-Tab to complete the start of string, C-function or LaTeX-macro"),
	gettext_noop("Turn off tool hints from the Options menu"),
	gettext_noop("Use `Auto paragraph...' and `Word wrap...' to edit paragraphs like a WP"),
	gettext_noop("Try your hand at adding new Cooledit features with Python"),
	gettext_noop("Mark locations in files with the book mark commands in the Edit menu"),
	gettext_noop("Debug programs with Alt-F5"),
	gettext_noop("Execute explicit debugger commands with Alt-F1"),
	gettext_noop("Turn off spell checking from the Options menu"),
	gettext_noop("Find files containing regular expressions with Ctrl-Alt-F"),
	gettext_noop("Delete debugger variables with Del in the variable dialog"),
	gettext_noop("Add debugger watches with Ins in the variable dialog"),
	gettext_noop("Make a variable watch point by marking it with Ins in the variable dialog"),
	gettext_noop("Change the default size of the undo stack in ~/.cedit/.cooledit.ini"),
	gettext_noop("Bookmark all matching strings with the `Bookmarks' switch in the `Search' dialog"),
	gettext_noop("Setup ~/.cedit/filelist so that Ctrl-Tab completes in your file-browser"),
	gettext_noop("Start internal XTerms with \"Terminal\" in the command menu"),
	gettext_noop("Use the coolproject command to start projects"),
	gettext_noop("Press Escape to enter shell commands"),
	gettext_noop("Select text then use Escape to pipe through a shell command"),
    };
/* HINTEND */

    if (i < 0) {
	n = sizeof (hints) / sizeof (char *);
	i = time (0) % n;
    } else
	i = (i + 1) % n;

    strcpy (label, _("Cooledit - Hint: "));
    strcat (label, gettext (hints[i]));
    XChangeProperty (CDisplay, main_window, ATOM_WM_NAME, XA_STRING, 8,
		     PropModeReplace, (unsigned char *) label, strlen (label));
}

/* }}} hint message on title bar */


void edit_change_directory (void)
{E_
    char *new_dir;
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    char host[256] = REMOTEFS_LOCAL;
#warning deal with host
    new_dir = CGetDirectory (main_window, 40, 40, current_dir, "", 
/* heads the 'Change Directory' dialog box */
    _(" Change Directory "), host);
    if (new_dir) {
	if (change_directory (new_dir, errmsg) < 0) { /* which should never happen */

/* heads the 'Change Directory' dialog box */
	    CErrorDialog(main_window, 20, 20, _(" Change directory "), " Error return from chdir. \n [%s]", errmsg);
	}
    }
}

/* {{{	command-line options */

int load_setup (const char *file);
int save_setup (const char *file);

static char **command_line_files;

char *editor_options_file = 0;
extern char *option_preferred_visual;
extern int option_force_own_colormap;
extern int option_force_default_colormap;
extern int option_invert_colors;
extern int option_invert_crome;
extern int option_invert_red_green;
extern int option_invert_green_blue;
extern int option_invert_red_blue;

int option_suppress_load_files = 0;
int option_suppress_load_files_cmdline = 0;
int option_suppress_load_options = 0;
int option_save_setup_on_exit = 1;
int option_hint_messages = 120;
int option_cursor_blink_rate = 7;
int option_pull_down_window_list = 1;
int option_toolbar = 1;
int option_minimal_main_window = 0;
int option_utf_interpretation2 = 1;
int option_locale_encoding = 0;
extern int option_utf_interpretation2;
extern int option_locale_encoding;
extern int option_font_set;
extern char *option_geometry;
extern int option_use_xim;
int option_aa_font = -1;
extern int option_rgb_order;
extern int option_interchar_spacing;

int option_new_window_ask_for_file = 1;

char *option_display = 0;
char *option_geometry = 0;
char *option_background_color = "igloo";
char *option_foreground_red = 0;
char *option_foreground_green = 0;
char *option_foreground_blue = 0;
char *option_font2 = 0;
char *option_widget_font2 = 0;

static int option_command_line_doesnt_override = 0;
static int get_help = 0;
static int get_version = 0;
static int option_verbose = 0;

static char *option_server = NULL;


void usage (void)
{E_
    printf ( \
/* Translate only the description of what the options does, not the option itself */
	    _("Cooledit version %s\n" \
	    "A user-friendly text editor for the X Window System.\n" \
	    "Usage:\n" \
	    "%s [-AabCEhiPsSUVv?] [options] [[+<line>] <file>] [[+<line>] <file>] ...\n"), \
		    VERSION, PACKAGE);
    printf (_("-d, -display <display>                   the X server to display on\n"));
#ifdef GUESS_VISUAL
    printf (_("-vis, --visual <visual-class>            use   cooledit -vis h   for help\n" \
	    "-C, -cmap, --own-colormap                force use of own colormap\n" \
	    "-defcmap, --default-colormap             force use of default colormap\n"));
#endif
    printf (_("-g, -geom, -geometry <geometry>          window size and position\n" \
	    "-m, --minimal-main-window                main window border only\n" \
	    "-lines <n>                               number of text lines\n" \
	    "-columns <n>                             number of text columns\n" \
	    "--edit-bg <nn>                           <nn> 0-26 (rather edit .cooledit.ini)\n" \
	    "-bg, --background-color <color>          eg blue for solid blue, default: igloo\n" \
	    "-R, --foreground-red <value>             red component, default: 0.9\n" \
	    "-G, --foreground-green <value>           green component, default: 1.1\n" \
	    "-B, --foreground-blue <value>            blue component, default: 1.4\n" \
	    "-f, -fn, -font <font-name>               use   cooledit -font h   for help\n" \
	    "--widget-font <font-name>                font of widgets and controls\n" \
	    "-S, --suppress-load-files                don't load saved desktop\n" \
	    "-U, --suppress-load-options              don't load saved options\n" \
	    "-E, -no-override                         command line doesn't override init file\n" \
	    "-I, --use-initialisation-file <file>     default: ~/.cedit/.cooledit.ini\n" \
	    "-wordwrap, --word-wrap <length>          default: 72\n" \
	    "-typew, --type-writer                    type-writer word wrap\n" \
	    "-autop, --auto-paragraph                 word processor paragraphing\n" \
	    "-i, --all-characters                     no longer an option. now the default\n" \
	    "-noi --no-international-characters       default\n" \
	    "-t, -tab, --tab-spacing <spacing>        set tab spacing\n" \
	    "-s, -space, --space-filled-tabs          fill tabs with ascii 32 (a space)\n" \
	    "-nospace, --no-space-filled-tabs         default\n" \
	    "-a, --auto-indent                        return does an auto indent (default)\n" \
	    "-noautoi, --no-auto-indent               turn off auto indent\n" \
	    "-b, --backspace-through-tabs             backspace deletes to right margin\n" \
	    "-noback, --no-backspace-through-tabs     default\n" \
	    "-half, --fake-half-tabs                  emulate half tabs with spaces (default)\n" \
	    "-no-half, --no-fake-half-tabs            turn off half tabbing\n" \
	    "-toolbar                                 edit windows have a toolbar\n" \
	    "-no-toolbar                              disabled\n" \
	    "--no-xim                                 disable XIM support\n" \
	    "-utf8, --utf8-interpretation             interpret UTF-8 encoding\n" \
	    "-locale, --locale-encoding               use locale encoding interpretation\n" \
	    "-fontset, --fontset                      try load font as a X fontset\n" \
	    "--anti-aliasing                          depreciated, use <fontname>/3\n" \
	    "--red-first                              R-G-B LCD sub-pixel order aliasing\n" \
	    "--blue-first                             B-G-R LCD sub-pixel order aliasing\n" \
	    "--interchar-spacing <n>                  extra spacing between chars in pixels\n" \
	    "                                         (anti-aliased fonts only)\n" \
	    "--interwidget-spacing <n>                spacing between widgets in pixels\n" \
	    "--look [gtk|cool|next]                   look emulation. defaults to gtk\n" \
	    "--[no-]bright-invert                     invert the brightness of all colours\n" \
            "                                         (useful for white backgrounds)\n" \
	    "--[no-]chromin-invert                    invert the chrominance of all colours\n" \
	    "--[no-]swap-red-green                    swap red with green\n" \
	    "--[no-]swap-green-blue                   swap green with blue\n" \
	    "--[no-]swap-red-blue                     swap red with blue\n" \
	    "-A, -save-setup                          save setup on exit\n" \
	    "-P, -no-save-setup                       don't save setup on exit\n" \
	    "-W, --whole-chars-search <chars>         characters that constitute a whole word\n" \
	    "                            when searching, default: 0-9a-z_ (typed out in full)\n" \
	    "-w, --whole-chars-move <chars>           characters that constitute a whole word\n" \
	    "         when moving and deleting, default: 0-9a-z_; ,[](){} (typed out in full)\n" \
            "-server, --server <ip-range>             act as remote file-server for cooledit\n" \
            "           accepting connections from <ip-range>, eg. 10.0.0.0/24,192.168.0.0/16\n" \
	    "-verbose                                 print details of initialisation\n" \
	    "-h, -H, -?, --help                       print this message to stdout\n" \
	    "-V, -v, --version                        print versiom info\n" \
	    "\n"));
}

void version (void)
{E_
    printf (_("Cooledit version %s\n"), VERSION);
}

struct prog_options cooledit_options[] =
{
    {' ', "", "", ARG_STRINGS, 0, 0, 0},
    {0, "-bg", "--background-color", ARG_STRING, &option_background_color, 0, 0},
#ifdef GUESS_VISUAL
    {0, "-vis", "--visual", ARG_STRING, &option_preferred_visual, 0, 0},
    {'C', "-cmap", "--own-colormap",  ARG_SET, 0, 0, &option_force_own_colormap},
    {0, "-defcmap", "--default-colormap",  ARG_SET, 0, 0, &option_force_default_colormap},
#endif
    {0, "", "-toolbar", ARG_SET, 0, 0, &option_toolbar},
    {0, "", "-no-toolbar", ARG_CLEAR, 0, 0, &option_toolbar},
    {'R', "", "--foreground-red", ARG_STRING, &option_foreground_red, 0, 0},
    {'G', "", "--foreground-green", ARG_STRING, &option_foreground_green, 0, 0},
    {'B', "", "--foreground-blue", ARG_STRING, &option_foreground_blue, 0, 0},
    {'f', "-fn", "-font", ARG_STRING, &option_font2, 0, 0},
    {0, "", "--widget-font", ARG_STRING, &option_widget_font2, 0, 0},
    {'S', "", "--suppress-load-files", ARG_SET, 0, 0, &option_suppress_load_files_cmdline},
    {'U', "", "--suppress-load-options", ARG_SET, 0, 0, &option_suppress_load_options},
    {'E', "-no-override", "", ARG_SET, 0, 0, &option_command_line_doesnt_override},
    {'I', "", "--use-initialisation-file", ARG_STRING, &editor_options_file, 0, 0},
    {0, "-wordwrap", "--word-wrap", ARG_INT, 0, 0, &option_word_wrap_line_length},
    {0, "-typew", "--type-writer", ARG_SET, 0, 0, &option_typewriter_wrap},
    {0, "-autop", "--auto-paragraph", ARG_SET, 0, 0, &option_auto_para_formatting},
    {'t', "-tab", "--tab-spacing", ARG_INT, 0, 0, &option_tab_spacing},
    {'s', "-space", "--space-filled-tabs", ARG_SET, 0, 0, &option_fill_tabs_with_spaces},
    {0, "-nospace", "--no-space-filled-tabs", ARG_CLEAR, 0, 0, &option_fill_tabs_with_spaces},
    {'a', "-autoi", "--auto-indent", ARG_SET, 0, 0, &option_return_does_auto_indent},
    {0, "-noautoi", "--no-auto-indent", ARG_CLEAR, 0, 0, &option_return_does_auto_indent},
    {'b', "", "--backspace-through-tabs", ARG_SET, 0, 0, &option_backspace_through_tabs},
    {0, "-noback", "--no-backspace-through-tabs", ARG_CLEAR, 0, 0, &option_backspace_through_tabs},
    {0, "-half", "--fake-half-tabs", ARG_SET, 0, 0, &option_fake_half_tabs},
    {0, "-no-half", "--no-fake-half-tabs", ARG_CLEAR, 0, 0, &option_fake_half_tabs},
    {'A', "-save-setup", "", ARG_SET, 0, 0, &option_save_setup_on_exit},
    {'P', "-no-save-setup", "", ARG_CLEAR, 0, 0, &option_save_setup_on_exit},
    {'W', "", "--whole-chars-search", ARG_STRING, &option_whole_chars_search, 0, 0},
    {'w', "", "--whole-chars-move", ARG_STRING, &option_chars_move_whole_word, 0, 0},
    {'h', "-?", "--help", ARG_SET, 0, 0, &get_help},
    {'H', "-help", "--help", ARG_SET, 0, 0, &get_help},
    {'V', "-v", "--version", ARG_SET, 0, 0, &get_version},
    {0, "", "-verbose", ARG_SET, 0, 0, &option_verbose},
    {'d', "", "-display", ARG_STRING, &option_display, 0, 0},
    {'g', "-geom", "-geometry", ARG_STRING, &option_geometry, 0, 0},
    {0, "-lines", "", ARG_INT, 0, 0, &start_height},
    {0, "-columns", "", ARG_INT, 0, 0, &start_width},
    {'m', "--minimal-main-window", "", ARG_SET, 0, 0, &option_minimal_main_window},
    {0, "", "--no-xim", ARG_CLEAR, 0, 0, &option_use_xim},
    {0, "-utf8", "--utf8-interpretation", ARG_SET, 0, 0, &option_utf_interpretation2},
    {0, "-locale", "--locale-encoding", ARG_SET, 0, 0, &option_locale_encoding},
    {0, "-fontset", "--fontset", ARG_SET, 0, 0, &option_font_set},
    {0, "", "--red-first", ARG_CLEAR, 0, 0, &option_rgb_order},
    {0, "", "--blue-first", ARG_SET, 0, 0, &option_rgb_order},
    {0, "", "--anti-aliasing", ARG_SET, 0, 0, &option_aa_font},
    {0, "", "--no-anti-aliasing", ARG_CLEAR, 0, 0, &option_aa_font},
    {0, "", "--interchar-spacing", ARG_INT, 0, 0, &option_interchar_spacing},
    {0, "", "--edit-bg", ARG_INT, 0, 0, &option_editor_bg_normal},
    {0, "", "--interwidget-spacing", ARG_INT, 0, 0, &option_interwidget_spacing},
    {0, "", "--look", ARG_STRING, &option_look, 0, 0},
    {0, "", "--bright-invert", ARG_SET, 0, 0, &option_invert_colors},
    {0, "", "--no-bright-invert", ARG_CLEAR, 0, 0, &option_invert_colors},
    {0, "", "--chromin-invert", ARG_SET, 0, 0, &option_invert_crome},
    {0, "", "--no-chromin-invert", ARG_CLEAR, 0, 0, &option_invert_crome},
    {0, "", "--swap-red-green", ARG_SET, 0, 0, &option_invert_red_green},
    {0, "", "--no-swap-red-green", ARG_CLEAR, 0, 0, &option_invert_red_green},
    {0, "", "--swap-green-blue", ARG_SET, 0, 0, &option_invert_green_blue},
    {0, "", "--no-swap-green-blue", ARG_CLEAR, 0, 0, &option_invert_green_blue},
    {0, "", "--swap-red-blue", ARG_SET, 0, 0, &option_invert_red_blue},
    {0, "", "--no-swap-red-blue", ARG_CLEAR, 0, 0, &option_invert_red_blue},
    {0, "-server", "--server", ARG_STRING, &option_server, 0, 0},
    {0, 0, 0, 0, 0, 0, 0}
};

static struct cmdline_option_free cmdline_fl;

static void process_command_line (int argc, char **argv)
{E_
    int error;
    error = get_cmdline_options (argc, argv, cooledit_options, &cmdline_fl);

    if (error) {
	fprintf (stderr, _("%s: error processing commandline argument %d\n"), argv[0], error);
	usage();
	exit (1);
    }

    if (get_help)
	usage();
    if (get_version)
	version();
    if (get_help || get_version) {
        get_cmdline_options_free_list (&cmdline_fl);
	exit (0);
    }
}


/* }}}	command-line options */


/* {{{  multiple edit windows */

/* maximum number of edit windows: */
#define N_EDIT 50

/* the editors (a stack of sorts) */
CWidget *edit[N_EDIT + 1] = {0, 0};

int current_edit = 0;		/* containing the focus */
int last_edit = 0;		/* number of editors open */



/* }}}  multiple edit windows */

/* from menu.c: */
void destroy_menu (CWidget * w);
void render_menu (CWidget * w);

/* local: */
void update_wlist (void);
void wlist_callback (unsigned long ignored);
static int write_config (int clean);
void current_to_top (void);

/* {{{ process make (and other shell) error message */

/* returns -1 on not found and line numbner = -1. Could return only one as -1 */
static int get_file_and_line_from_error (const char *host, char *message, int *line_number, char **new_file)
{E_
    int i, l;
    char *p;

    if (!last_edit)
	return 0;

    *line_number = -1;

    for (i = 0; message[i]; i++)
	if ((l = strspn (message + i, "0123456789")))
	    if (message[i + l] == ':' && message[i - 1] == ':') {
		*line_number = atoi (message + i);
		break;
	    }
    for (i = 0; message[i]; i++)
	if ((l = strspn (message + i, "0123456789")))
	    if (strchr (":;\",\n ", message[i + l]) && message[i - 1] == ':') {
		*line_number = atoi (message + i);
		break;
	    }
    for (i = 0; message[i]; i++)
	if ((l = strspn (message + i, "0123456789")))
	    if (strchr (":;\",\n ", message[i + l]) && strchr (":;\", ", message[i - 1])) {
		*line_number = atoi (message + i);
		break;
	    }
    if (!strncmp (message, "/", 1) || !strncmp (message, "./", 2))
	goto try_new;
    p = strrchr (message, ' ');
    if (p) {
	p++;
	if (!strncmp (p, "/", 1) || !strncmp (p, "./", 2)) {
            char errmsg[REMOTEFS_ERR_MSG_LEN];
	    char m[MAX_PATH_LEN], *c;
	    message = p;
	  try_new:
	    memset (m, 0, MAX_PATH_LEN);
	    if (!strncmp (message, "./", 2)) {
		get_current_wd (m, MAX_PATH_LEN - 1);
		c = strchr (message, ':');
		if (!c)
		    c = message + strlen (message);
		strncpy (m + strlen (m), message + 1, (unsigned long) c - (unsigned long) (message + 1));
	    } else {
		c = strchr (message, ':');
		if (c) {
		    if ((unsigned long) c > (unsigned long) message) {
			if (*(c - 1) == ')') {
			    char *q;
			    for (q = c - 2; (unsigned long) q > (unsigned long) message; q--)
				if (*q == '(') {
				    c = q;
				    break;
				}
			}
		    }
		} else {
		    c = message + strlen (message);
		}
		strncpy (m, message, (unsigned long) c - (unsigned long) message);
	    }
	    c = pathdup (host, m, errmsg);
            if (!c)
	        return -1;
	    for (i = 0; i < last_edit; i++)
		if (edit[i]->editor->filename)
		    if (*(edit[i]->editor->filename)) {
			char q[MAX_PATH_LEN];
			strcpy (q, edit[i]->editor->dir);
			if (q[strlen (q) - 1] == '/')
			    q[strlen (q) - 1] = '\0';
			strcat (q, "/");
			strcat (q, edit[i]->editor->filename);
			if (!strcmp (c, q)) {
			    free (c);
			    return i;
			}
		    }
	    if (new_file)
		*new_file = c;
	    return -1;
	}
    }
    for (i = 0; i < last_edit; i++)
	if (edit[i]->editor->filename)
	    if (*(edit[i]->editor->filename)) {
		p = strstr (message, edit[i]->editor->filename);
		if (!p)
		    continue;
/* now test if this is a whole word */
		if ((unsigned long) p > (unsigned long) message)
		    if (strchr ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_0123456789.", *(p - 1)))
			continue;
		if (strchr ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_0123456789.", p[strlen (edit[i]->editor->filename)]))
		    continue;
		return i;
	    }
    return -1;
}

static int new_file_callback (const char *host, const char *full_file_name,...);

void edit_move_to_line_easy (WEdit *e, int l)
{E_
    edit_move_to_line (e, l - 1);
    if (edit_count_lines (e, e->start_display, e->curs1) < e->num_widget_lines / 4)
	edit_move_display (e, l - e->num_widget_lines / 4 - 1);
    else if (edit_count_lines (e, e->start_display, e->curs1) > e->num_widget_lines * 3 / 4)
	edit_move_display (e, l - e->num_widget_lines * 3 / 4 - 1);
    e->force |= REDRAW_PAGE;
}


void insert_text_into_current (char *text, int len, int delete_word_left)
{E_
    WEdit *e;
    if (current_edit >= last_edit)
	return;
    if (!len)
	return;
    e = edit[current_edit]->editor;
    e->force |= REDRAW_PAGE;
    edit_push_action (e, KEY_PRESS, e->start_display);

    if (delete_word_left) {
        int ch;
        for (;;) {
            ch = edit_get_byte (e, e->curs1 - 1);
            if (!C_ALNUM (ch))
                break;
            edit_backspace (e);
        }
    }

    while (len--) {
	edit_insert (e, *text++);
    }

    edit_update_curs_col (e);
    edit_scroll_screen_over_cursor (e);
    e->force |= REDRAW_PAGE;
    edit_render_keypress (e);
}

int goto_error (char *message, int raise_wm_window)
{E_
    int ed, l;
    char *new_file = 0;
    ed = get_file_and_line_from_error (edit[current_edit]->editor->host, message, &l, &new_file);
    if (new_file) {
	if (!new_file_callback (edit[current_edit]->editor->host, new_file)) {
	    edit_move_to_line_easy (edit[current_edit]->editor, l);
	    CTryFocus (edit[current_edit], raise_wm_window);
	    free (new_file);
	    return 0;
	}
	free (new_file);
	return 1;
    } else if (ed >= 0 && l >= 0) {
	current_edit = ed;
	edit_move_to_line_easy (edit[current_edit]->editor, l);
	edit[ed]->editor->force |= REDRAW_COMPLETELY;
	XRaiseWindow (CDisplay, edit[current_edit]->parentid);
	CTryFocus (edit[current_edit], raise_wm_window);
	CRaiseWindows ();
	current_to_top ();
    }
    return 0;
}

static int bookmarks_select_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    if (c->double_click || (c->command == CK_Enter && !c->handled)) {
	char *q;
        q = strdup(CGetTextBoxLine(w, w->cursor));
	CDestroyWidget ("bookmarks");
	goto_error (q, 1);
	edit[current_edit]->editor->force |= REDRAW_COMPLETELY;
        edit_render_keypress (edit[current_edit]->editor);
        edit_status (edit[current_edit]->editor);
        free(q);
    }
    if (c->command == CK_Cancel) {
	CDestroyWidget ("bookmarks");
	edit[current_edit]->editor->force |= REDRAW_COMPLETELY;
    }
    return 0;
}

static int bookmarks_done_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    CDestroyWidget ("bookmarks");
    return 1;
}

static CStr const_get_text_cb (void *hook1, void *hook2)
{E_
    CStr s;
    s.data = (char *) hook1;
    s.len = (int) (long) hook2;
    return s;
}

static void const_free_text (void *hook1, void *hook2)
{E_
    free (hook1);
}

void goto_file_dialog (const char *heading, const char *tool_hint, const char *text)
{E_
    int x, y;
    Window win;
    CWidget *w;
    if (CIdent ("bookmarks"))
	return;
    win = CDrawHeadedDialog ("bookmarks", main_window, 20, 20, heading);
    CIdent ("bookmarks")->position = WINDOW_ALWAYS_RAISED;
    CGetHintPos (&x, &y);
    CPushFont ("editor", 0);
    w = CDrawTextboxManaged ("bookmarks.text", win, x, y, 70 * FONT_MEAN_WIDTH + EDIT_FRAME_W, 20 * FONT_PIX_PER_LINE + EDIT_FRAME_H, 0, 0, const_get_text_cb, const_free_text, (void *) text, (void *) strlen(text), 0);
    if (tool_hint)
	CSetToolHint ("bookmarks.text", tool_hint);
    w->position |= POSITION_HEIGHT | POSITION_WIDTH;
    (CIdent ("bookmarks.text.vsc"))->position |= POSITION_HEIGHT | POSITION_RIGHT;
    CGetHintPos (0, &y);
    (CDrawPixmapButton ("bookmarks.done", win, 0, y, PIXMAP_BUTTON_CROSS))->position = POSITION_BOTTOM | POSITION_CENTRE;
    CCentre ("bookmarks.done");
    CSetSizeHintPos ("bookmarks");
    CSetWindowResizable ("bookmarks", FONT_MEAN_WIDTH * 15, FONT_PIX_PER_LINE * 15, 1600, 1200);
    CPopFont ();
    CMapDialog ("bookmarks");
    CAddCallback ("bookmarks.text", bookmarks_select_callback);
    CAddCallback ("bookmarks.done", bookmarks_done_callback);
    CFocus (CIdent ("bookmarks.text"));
}

void cooledit_appearance_modification (void)
{E_
    int i;
    for (i = 0; i < last_edit; i++)
        edit_appearance_modification (edit[i]->editor);
}

void bookmark_select (void)
{E_
    int i;
    POOL *p;
    p = pool_init ();
/* get all bookmarks */
    for (i = 0; i < last_edit; i++) {
	struct _book_mark *b;
	if (!edit[i]->editor->book_mark)
	    continue;
	for (b = edit[i]->editor->book_mark; b->prev; b = b->prev);	/* rewind */
	for (; b; b = b->next)
	    if (b->line >= 0)
		pool_printf (p, "%s%s:%d\n", edit[i]->editor->dir, edit[i]->editor->filename, b->line + 1);
    }
    pool_null (p);
    goto_file_dialog (_ (" Select Bookmark "), _ ("Double click on a bookmark to goto the file and line number"), (char *) pool_break (p));
}

/* }}} process make (and other shell) error message */

/* {{{  window stack manipulation */

/* moves the current editor to the top of the stack */
void current_to_top (void)
{E_
    CWidget *w = edit[current_edit];
    Cmemmove (&(edit[1]), &(edit[0]), current_edit * sizeof (CWidget *));
    edit[0] = w;
    current_edit = 0;
    CSetEditMenu (w->ident);	/* sets the editor to which the menu will send commands */
    update_wlist ();
}

int extents_width = 0, extents_height = 0;

#include "bitmap/toolbar.bitmap"

int toolbar_cmd[NUM_TOOL_BUTTONS] =
{
    CK_Maximize,
    CK_Exit,
    CK_Save,
    CK_Load,
    CK_XCut,
    CK_XStore,
    CK_XPaste,
    CK_Find,
    0
};

static char *toolbar_hints[NUM_TOOL_BUTTONS] =
{
/* Toolhint */
    gettext_noop ("Maximise the window, Alt-F6"),
    gettext_noop ("Close this edit window, F10"),
    gettext_noop ("Save this edit buffer, F2"),
    gettext_noop ("Load new file, Ctrl-O"),
    gettext_noop ("Delete, and copy to X clipboard, Shift-Del"),
    gettext_noop ("Copy to X clipboard, Ctrl-Ins"),
    gettext_noop ("Paste from X clipboard, Shift-Ins"),
    gettext_noop ("Search for text, F7"),
    gettext_noop ("Open the cooledit man page")
};

int tool_bar_callback (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    XEvent e;
    char ident[32], *p;
    strcpy (ident, ce->ident);
    p = strchr (ident, '.');
    *p = 0;
    memset (&e, 0, sizeof (XEvent));
    e.type = EditorCommand;
    e.xkey.keycode = toolbar_cmd[w->cursor];
    e.xkey.window = (CIdent (ident))->winid;
    CSendEvent (&e);
    return 1;
}

void edit_man_cooledit (unsigned long ignored);

int help_callback (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    edit_man_cooledit (0);
    return 1;
}

#define VERT_TEXT_OFFSET 7

int render_vert_text (CWidget * w)
{E_
    CWidget *wdt;
    wdt = CIdent (w->ident + 3);
    if (!wdt)
	return 0;
    if (!wdt->editor)
	return 0;
    if (!wdt->editor->syntax_type)
	return 0;
    CSetColor (color_palette (1));
    XDrawVericalString8x16 (CDisplay, w->winid, CGC, VERT_TEXT_OFFSET, NUM_TOOL_BUTTONS * 25 + 12,
	    wdt->editor->syntax_type, strlen (wdt->editor->syntax_type));
    return 1;
}

void change_syntax_type (CWidget * edi)
{E_
    CWidget *w;
    char x[34];
#ifdef HAVE_PYTHON
    XEvent e;
    memset (&e, 0, sizeof (XEvent));
    e.type = EditorCommand;
    e.xkey.keycode = CK_Type_Load_Python;
    e.xkey.window = edi->winid;
    CSendEvent (&e);
#endif
    strcpy (x, "win");
    strcat (x, edi->ident);
    w = CIdent (x);
    if (!w)
	return;
    CSetColor (COLOR_FLAT);
    CRectangle (w->winid, VERT_TEXT_OFFSET, NUM_TOOL_BUTTONS * 25 + 12, 16, 1024);
    CSendExpose (w->winid, VERT_TEXT_OFFSET, NUM_TOOL_BUTTONS * 25 + 12, 16, 1024);
}

int draw_vert_text (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    if (xe->type == Expose) {
	if (xe->xexpose.x > VERT_TEXT_OFFSET + 16)
	    return 0;
	return render_vert_text (w);
    }
    return 0;
}

#define NEW_WINDOW_FROM_TEXT ((char *) 1)

/* creates a new editor at 'number' in the stack and shifts up the stack */
/* returns one on success, 0 on error: usually file not found */
static int new_editor (int number, int x, int y, int columns, int lines, const char *f, const char *start_host, const char *d,...)
{E_
    static int edit_count = 0;
    int xe, ye;
    int i, modified = 0;
    CWidget *new_edit;
    Window win;
    char *t = 0, *p, *q;
    unsigned long size;

    if (last_edit >= N_EDIT) {
	CErrorDialog (0, 0, 0, _ (" Error "), _ (" You have opened the maximum number of possible edit windows "));
	return 0;
    }
    edit_count++;
    if (f == NEW_WINDOW_FROM_TEXT) {
	va_list ap;
	va_start (ap, d);
	t = va_arg (ap, char *);
	size = va_arg (ap, unsigned long);
	f = 0;
	va_end (ap);
	modified = 1;
    } else {
	size = 0;
	if (!d)
	    d = "";
	if (!f) {
	    f = 0;
	    t = "";
	} else if (!*f) {
	    f = 0;
	    t = "";
	} else if (!*d && *f != '/')
	    d = catstrs (current_dir, "/", NULL);
    }
    q = catstrs ("editor", itoa (edit_count), NULL);
    CPushFont ("editor", 0);
    win = CDrawDialog (p = catstrs ("wineditor", itoa (edit_count), NULL), main_window, x, y);
    CPopFont ();

    CGetHintPos (&xe, &ye);
    if (option_toolbar) {
	for (i = 0; i < NUM_TOOL_BUTTONS; i++) {
	    CWidget *w;
	    w = CDrawPixmapButton (catstrs (q, ".B", itoa (i), NULL), win,
			3, ye + i * 25, 25, 25, toolbar_buttons[i], '0');
	    if (toolbar_cmd[i] == CK_Util) {
		CDrawMenuButton (catstrs (q, ".util", NULL), w->winid, main_window,
		     25 + 10, -65, AUTO_SIZE, 0, _ (" Utility "), 0, 0, 0, 0);
	    }
	    w->takes_focus = 0;
	    w->cursor = i;	/* just to easily identify the button */
	    CAddCallback (w->ident, tool_bar_callback);
	    CSetToolHint (w->ident, _ (toolbar_hints[i]));
	}
/* man page "help" */
	CAddCallback (catstrs (q, ".B", itoa (NUM_TOOL_BUTTONS - 1), NULL), help_callback);
	CSetToolHint (catstrs (q, ".B", itoa (NUM_TOOL_BUTTONS - 1), NULL), _ (toolbar_hints[NUM_TOOL_BUTTONS - 1]));
	xe = 25 + 4;
    }
    edit_set_syntax_change_callback (change_syntax_type);
    reset_hint_pos (0, 0);
    CPushFont ("editor", 0);
    new_edit = CDrawEditor (q, win, xe, ye, columns * FONT_MEAN_WIDTH,
			    lines * FONT_PIX_PER_LINE, t, f, start_host, d, EDITOR_HORIZ_SCROLL, size);
    if (!new_edit) {
	CDestroyWidget (catstrs ("wineditor", itoa (edit_count), NULL));
	CPopFont ();
	return 0;
    }
    new_edit->position |= (POSITION_WIDTH | POSITION_HEIGHT);
    new_edit->editor->modified = modified;

    CSetSizeHintPos (p);
/* font height needed for resizing granularity - well, i guest thats why they call it hacking */
    CSetWindowResizable (p, 150, 150, 1600, 1200);
    CPopFont ();
    CMapDialog (p);
    CAddBeforeCallback (p, draw_vert_text);

    x = x + (CIdent (p))->width;
    y = y + (CIdent (p))->height;
    extents_width = max (x, extents_width);
    extents_height = max (y, extents_height);

    (CIdent (catstrs (q, ".text", NULL)))->position |= (POSITION_BOTTOM | POSITION_WIDTH);
    (CIdent (catstrs (q, ".vsc", NULL)))->position |= (POSITION_RIGHT | POSITION_HEIGHT);
    (CIdent (catstrs (q, ".hsc", NULL)))->position |= (POSITION_BOTTOM | POSITION_WIDTH);

    Cmemmove (&(edit[number + 1]), &(edit[number]), (last_edit - number) * sizeof (CWidget *));
    last_edit++;
#warning back port this fix:
/*    edit[last_edit] = 0; */
    edit[number] = new_edit;
    update_wlist ();
    return 1;
}

/* removes the editor at 'current_edit' */
void remove_current (int do_raise, int focus_next)
{E_
    CDestroyWidget (CWidgetOfWindow (edit[current_edit]->parentid)->ident);
    /* close up the hole in the stack: */
    Cmemmove (&(edit[current_edit]), &(edit[current_edit + 1]), (last_edit - current_edit) * sizeof (CWidget *));
    /* one less editor: */
    last_edit--;
    edit[last_edit] = 0;
    if (last_edit) {
	if (current_edit == last_edit)
	    current_edit--;
        if (focus_next)
	    CFocus (edit[current_edit]);	/* focus on the next */
	if (do_raise) {
	    XRaiseWindow (CDisplay, edit[current_edit]->parentid);
	    CRaiseWindows ();
	}
    }
    update_wlist ();
}

/* }}}  window stack manipulation */

void get_main_window_size (unsigned int *width, unsigned int *height)
{E_
    Window root;
    int x, y;
    unsigned int border, depth;
    XGetGeometry (CDisplay, main_window, &root, \
		  &x, &y, width, height, &border, &depth);
}

/* {{{  'Window' menu call backs */

/* returns non-zero if alrady open. !new_window means the user could be re-opening
the file currently under focus -- we don't want to annoy him by asking the obvious */
static int file_is_open (const char *host, const char *p_, int new_window)
{E_
    char *p;
    char s[MAX_PATH_LEN];
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    int r = 0, i, found = -1;
    if (!host)
        host = REMOTEFS_LOCAL;
    p = pathdup (host, p_, errmsg);
    if (!p) {
        CErrorDialog (0, 0, 0, _ (" Resolving Path "), _ (" Error connecting to %s: %s "), host, errmsg);
        return -1;
    }
    for (i = 0; i < last_edit && !r; i++)
	if (edit[i]->editor->dir && edit[i]->editor->filename && (new_window || i != current_edit)) {
	    strcpy (s, edit[i]->editor->dir);
            if (!*s || s[strlen (s) - 1] != '/')
	        strcat (s, "/");
	    strcat (s, edit[i]->editor->filename);
	    if (!strcmp (host, edit[i]->editor->host) && !strcmp (s, p)) {
                found = i;
                break;
            }
	}
    if (found != -1) {
	CPushFont ("widget", 0);
	r = CQueryDialog (0, 0, 0, _ (" Warning "), catstrs (_ (" This file is already open. Open another? "), "\n", p, NULL), _ ("Yes"), _ ("Go to the window"), _ ("Cancel"), NULL);
	CPopFont ();
    }
    free (p);
    if (r == 1 /* "Go to the window" */) {
	current_edit = found;
	XRaiseWindow (CDisplay, edit[current_edit]->parentid);
	CTryFocus (edit[current_edit], 0);
	CRaiseWindows ();
	current_to_top ();
    }
    return r;
}

static int height_offset;

void fit_into_main_window (int *lines, int *columns, int x, int y)
{E_
    unsigned int width, height, f;
    get_main_window_size (&width, &height);
    CPushFont ("widget", 0);
    f = FONT_PIX_PER_LINE;
    CPopFont ();
    CPushFont ("editor", 0);
    while (*columns * FONT_MEAN_WIDTH + 25 + EDIT_FRAME_W + 4 + 2 + 20 + WIDGET_SPACING * 2 > width - x)
	(*columns)--;
    if (*columns * FONT_MEAN_WIDTH < 150)
	(*columns) = 150 / FONT_MEAN_WIDTH;
    while (*lines * FONT_PIX_PER_LINE + f + EDIT_FRAME_H + WIDGET_SPACING * 3 + 8 + TEXT_RELIEF * 2 + 3 + 3 + 3 + 2 > height - y)
	(*lines)--;
    if (*lines * FONT_PIX_PER_LINE < 250)
	*lines = 250 / FONT_PIX_PER_LINE;
    CPopFont ();
}

/* returns non-zero on error */
static int new_file_callback (const char *host, const char *full_file_name,...)
{E_
    int x, y, columns, lines, result = 1;
    char *d = 0;
    char errmsg[REMOTEFS_ERR_MSG_LEN];

    if (!full_file_name || full_file_name == NEW_WINDOW_FROM_TEXT) {
	if (last_edit) {
	    /* copy the current directory: */
	    d = (char *) pathdup (host, edit[current_edit]->editor->dir, errmsg);
	} else {
	    d = (char *) pathdup (host, local_home_dir, errmsg);
        }
        if (!d) {
            CErrorDialog (0, 0, 0, _ (" Resolving Path "), _ (" Error connecting to %s: %s "), host, errmsg);
            return result;
        }
    }

    CGetWindowPosition (edit[current_edit]->parentid, main_window, &x, &y);
    if (last_edit) {
	columns = edit[current_edit]->editor->num_widget_columns;
	lines = edit[current_edit]->editor->num_widget_lines;
    } else {
	columns = 400;
	lines = 200;
    }
    lines -= 2;

    fit_into_main_window (&lines, &columns, x, y);

    if (full_file_name == NEW_WINDOW_FROM_TEXT) {
	char *t;
	unsigned long size;
	va_list ap;
	va_start (ap, full_file_name);
	t = va_arg (ap, char *);
	size = va_arg (ap, unsigned long);
	x = new_editor (0, x, y, columns, lines, NEW_WINDOW_FROM_TEXT, host, d, (char *) t, (unsigned long) size);
	va_end (ap);
    } else {
	x = new_editor (0, x, y, columns, lines, full_file_name, host, d);
    }
    if (x) {
	current_edit = 0;
	CTryFocus (edit[current_edit], 0);
	result = 0;
    }
    if (d)
	free (d);
    return result;
}

void new_window_callback (unsigned long ignored)
{E_
    char *f = 0;
    char host[256];
    strcpy (host, edit[current_edit]->editor->host);
    if (option_new_window_ask_for_file)
	f = CGetLoadFile (main_window, 20, 20, edit[current_edit]->editor->dir, "", _(" Open "), host);
    new_file_callback (host, f);
    if (f)
	free (f);
}

void window_cycle_callback (unsigned long ignored)
{E_
    current_edit = (current_edit + 1) % last_edit;
    CFocus (edit[current_edit]);
    XRaiseWindow (CDisplay, edit[current_edit]->parentid);
    CRaiseWindows ();		/* brings ALWAYS_ON_TOP windows to the top */
    update_wlist ();
}

void save_desk_callback (unsigned long ignored)
{E_
    write_config (0);
}

char *get_all_lists (void);
void free_all_lists (void);
void free_all_scripts (void);
void put_all_lists (char *list);
void complete_command (CWidget * e);

void exit_app (unsigned long save)
{E_
    char *p;
    switch ((int) save) {
    case 0:
	break;
    case 1:
	if (write_config (1))
	    return;		/* saves all and tries to exit all editors */
	break;
    case 2:
	if (write_config (2))
	    return;		/* tries to exit all editors */
	break;
    }
    if (option_save_setup_on_exit)
	save_setup (editor_options_file);
    save_options_section (editor_options_file, "[Input Histories]", p = get_all_lists ());
    rxvtlib_shutall ();
    free (p);
    free_all_scripts ();
    complete_command (0);
    debug_shut ();
#ifdef HAVE_PYTHON
    coolpython_shut ();
#endif
    CShutdown ();
    CDisable (0);
    free (argv_nought);
    free_all_lists ();
    load_setup (0);
    catstrs_clean ();
    edit_free_cache_lines ();
    postscript_clean ();
    get_cmdline_options_free_list (&cmdline_fl);
    inspect_clean_exit ();
    XAaCacheClean ();
    exit (0);
}

/* number of 'Window' menu items */
#define WLIST_ITEMS 10

void add_to_focus_stack (Window w);

static void set_current (int new_current)
{E_
    current_edit = new_current;
    current_to_top ();
    XRaiseWindow (CDisplay, edit[current_edit]->parentid);
    CRaiseWindows ();
    add_to_focus_stack (edit[current_edit]->winid);
}

/* a file was selected in the menu, so focus and raise it */
void wlist_callback (unsigned long ignored)
{E_
    set_current (CIdent ("menu.wlist")->current - WLIST_ITEMS);
}

void close_window_callback (unsigned long ignored)
{E_
    CEditMenuCommand (CK_Exit);
}

void close_last_callback (unsigned long ignored)
{E_
    if (last_edit >= 1)
        set_current (last_edit - 1);
    CEditMenuCommand (CK_Exit);
}

void menu_browse_cmd (unsigned long ignored)
{E_
    char host[256] = REMOTEFS_LOCAL;
    int l;
    for (l = 0;; l++)
	if (!CIdent (catstrs ("_cfileBr", itoa (l), NULL)))
	    break;
#warning handle host
    CDrawBrowser (catstrs ("_cfileBr", itoa (l), NULL), CRoot, 0, 0, current_dir, "", _(" File Browser "), host);
    (CIdent (catstrs ("_cfileBr", itoa (l), NULL)))->position |= WINDOW_UNMOVEABLE;
}

void menu_jump_to_file (unsigned long ignored)
{E_
    long cursor, end_of_name;
    char *s, *f;

    s = edit_get_current_line_as_text (edit[current_edit]->editor, NULL, &cursor);
    while (cursor > 0 && s[cursor - 1] && strchr (NICE_FILENAME_CHARS "/", (unsigned char) s[cursor - 1]))
	cursor--;
    for (end_of_name = cursor;
	 s[end_of_name] && strchr (NICE_FILENAME_CHARS "/", (unsigned char) s[end_of_name]); end_of_name++);
    s[end_of_name] = '\0';
    f = s + cursor;
    if (*f == '/') {
	goto_error (f, 1);
    } else {
	char *t;
	t = user_file_list_search (0, 0, 0, f);
	if (t) {
	    goto_error (t, 1);
	    free (t);
	}
    }
    free (s);
}

extern char *init_font;
extern char *init_widget_font;
extern char *init_bg_color;

void run_main_callback (unsigned long ignored)
{E_
    char lines[10], columns[10];
    sprintf (lines, "%d", edit[current_edit]->editor->num_widget_lines);
    sprintf (columns, "%d", edit[current_edit]->editor->num_widget_columns);
    switch (fork()) {
	case 0:
	    set_signal_handlers_to_default ();
	    execlp (argv_nought, argv_nought, "-Smf", init_font, "--widget-font", init_widget_font, "-lines", lines, "-columns", columns, NULL);
	    exit (0);
	case -1:
	    CErrorDialog (0, 0, 0, _(" Run 'cooledit' "), get_sys_error (_(" Error trying to fork process ")));
	    return;
	default:
	    return;
    }
}

/* }}}  'Window' menu call backs */

/* {{{  'Window' menu update */


void init_usual_items (struct menu_item *m)
{E_
#define ADD_USUAL(t, h, cb, d)  \
    do {    m[i].text = (char *) strdup (t); \
            m[i].hot_key = (h); \
            m[i].call_back = (cb); \
            m[i].data = (d); \
            i = i + 1;                    } while (0)

    int i = 0;

    ADD_USUAL(" New window\tCtrl-F3 ", '~', new_window_callback, 0);
    ADD_USUAL(" New main window\tF13 ", '~', run_main_callback, 0);
    ADD_USUAL(" Window cycle\tCtrl-F6/Shift-Tab ", '~', window_cycle_callback, 0);
    ADD_USUAL(" Close window\tF10 ", '~', close_window_callback, 0);
    ADD_USUAL(" Close deepest window\tAlt-F10 ", '~', close_last_callback, 0);
    ADD_USUAL(" Close all and exit\tCtrl-F10", 0, exit_app, 1);
    ADD_USUAL(" Save all and exit\tAlt-x", 0, exit_app, 2);
    ADD_USUAL(" Save desktop\tCtrl-F2 ", '~', save_desk_callback, 0);
    ADD_USUAL(" File browser...\t", '~', menu_browse_cmd, 0);
    ADD_USUAL("  ", 0, NULL, 0);

    assert (i == WLIST_ITEMS);
}


void update_wlist (void)
{E_
    struct menu_item *m;
    CWidget *w, *drop = 0;
    int i;

    w = CIdent ("menu.wlist");

    destroy_menu (w);

    m = CMalloc ((last_edit + WLIST_ITEMS) * sizeof (struct menu_item));

    init_usual_items (m);

    if (last_edit > 0) {
	for (i = 0; i < last_edit; i++) {
	    m[i + WLIST_ITEMS].text = (char *) strdup (catstrs (" ", edit[i]->editor->filename, "  ", NULL));
	    m[i + WLIST_ITEMS].hot_key = 0;
	    m[i + WLIST_ITEMS].call_back = wlist_callback;
	    if (i == current_edit)
		m[i + WLIST_ITEMS].text[0] = '>';
	}
    }
    w->numlines = last_edit + WLIST_ITEMS;
    w->current = current_edit + WLIST_ITEMS;
    while (w->current >= w->numlines)
	w->current--;
    w->menu = m;

    if (w->droppedmenu[0])
        drop = CIdent (w->droppedmenu);
    if (drop) {
	drop->menu = m;
	drop->numlines = w->numlines;
	drop->current = w->current;
	render_menu (drop);
    }
    if (last_edit > 0)
	CSetEditMenu (edit[current_edit]->ident);
}

/* }}}  'Window' menu update */


/* {{{ configuration file handler */

#define SCANTYPE_FLOAT          1
#define SCANTYPE_LONG           2
#define SCANTYPE_STR            3

static int scan_left (const char *s, int i, const char *field, const char *allowed, void *out, int scantype)
{E_
    int l, field_offset = 0, value_offset = 0;
    char *out_;
    i--;
    for (;;) {
        if (i < 0)
            return -1;
        if (strchr (allowed, s[i]))
            break;
        i--;
    }
    value_offset = i + 1;
    for (;;) {
        if (i < 0)
            return -1;
        if (!strchr (allowed, s[i]))
            break;
        i--;
    }
    field_offset = i + 1;
    l = strlen (field);
    assert (l > 0);
    l--;
    for (;;) {
        if (i < 0)
            return -1;
        if (s[i] != field[l])
            return -1;
        if (!l)
            break;
        i--;
        l--;
    }
    switch (scantype) {
    case SCANTYPE_FLOAT:
        if (sscanf (s + field_offset, "%f", (float *) out) != 1)
            return -1;
        break;
    case SCANTYPE_LONG:
        if (sscanf (s + field_offset, "%ld", (long *) out) != 1)
            return -1;
        break;
    case SCANTYPE_STR:
        out_ = (char *) out;
        if (value_offset - field_offset > 255)
            return -1;
        memcpy (out_, s + field_offset, value_offset - field_offset);
        out_[value_offset - field_offset] = '\0';
        break;
    }
    return i;
}

/* scan from right to left in case of spaces in the filename */
static int scan_file_line(const char *s, char *d, char *f, float *x, float *y, long *w, long *h, long *c, long *l, char *host)
{E_
    int i, eol = 0, p, ret;

    while (s[eol] && s[eol] != '\n')
        eol++;
    ret = eol;
    if (s[ret] == '\n')
        ret++;
    if (ret < 4)
        return -1;

#define LEFT(cond) \
    do { \
        for (;;) { \
            char ch; \
            if (i <= 0) \
                return -2; \
            i--; \
            ch = s[i]; \
            if (!(cond)) { \
                i++; \
                break; \
            } \
        } \
    } while(0)

/* host is optional, all others are mandatory */
    i = scan_left(s, eol, "host=", "0123456789abcdefABCDEF:.", host, SCANTYPE_STR);
    if (i >= 0)
        eol = i;
    if ((eol = scan_left(s, eol, "topline=", "0123456789-", l, SCANTYPE_LONG)) < 0)
        return -2;
    if ((eol = scan_left(s, eol, "cursor=", "0123456789-", c, SCANTYPE_LONG)) < 0)
        return -2;
    if ((eol = scan_left(s, eol, "lines=", "0123456789-", h, SCANTYPE_LONG)) < 0)
        return -2;
    if ((eol = scan_left(s, eol, "columns=", "0123456789-", w, SCANTYPE_LONG)) < 0)
        return -2;
    if ((eol = scan_left(s, eol, "y=", "0123456789.-", y, SCANTYPE_FLOAT)) < 0)
        return -2;
    if ((eol = scan_left(s, eol, "x=", "0123456789.-", x, SCANTYPE_FLOAT)) < 0)
        return -2;

    i = eol;

    LEFT(ch == ' ');
    p = i;
    LEFT(ch != '/');
    strncpy(d, s, i);
    d[i] = '\0';
    while(s[i] == ' ')
        i++;
    strncpy(f, &s[i], p - i);
    f[p - i] = '\0';

    return ret;
}


void edit_about_cmd (void);

/* open all the files listed in the configuration file returns 0 on success */
int read_config (void)
{E_
    char f[MAX_PATH_LEN];
    char d[MAX_PATH_LEN];
    char host[256] = REMOTEFS_LOCAL;
    char *s, *r;
    long w, h, c, l;
    float x, y;
    int n = 0, num_read = 0, i;
    DIR *dir;
    dir = opendir (catstrs (local_home_dir, EDIT_DIR, "/.t", NULL));
    s = get_options_section (editor_options_file, "[Files]");
    if (!dir) {
	mkdir (catstrs (local_home_dir, EDIT_DIR, NULL), 0700);
	mkdir (catstrs (local_home_dir, EDIT_DIR, "/.t", NULL), 0700);
    }
    if (dir)
	closedir (dir);
    if (!s)
	return 1;
    r = s;
    for (;;) {
        strcpy (host, REMOTEFS_LOCAL);
        n = scan_file_line(s, d, f, &x, &y, &w, &h, &c, &l, host);
        if (n == -1)
            break;
        if (n == -2) {
	    fprintf (stderr, _ ("cooledit: error in initialisation file %s: line %d\n"), editor_options_file, num_read + 1);
	    CErrorDialog (main_window, 20, 20, _ (" Load Config "), _ (" Error in initialisation file %s: line %d "), editor_options_file, num_read + 1);
	    free (r);
	    return 1;
        }
	s += n;
	x = x * (float) FONT_MEAN_WIDTH + 0.5;
	y = y * (float) FONT_PIX_PER_LINE + 0.5;
	if (new_editor (num_read, (int) x, (int) y, w, h, f, host, d)) {
	    edit_move_display (edit[num_read]->editor, l);
	    edit_move_to_line (edit[num_read]->editor, c);
	    num_read++;
	}
	if (!n)
	    break;
    }

    current_edit = 0;
    for (i = last_edit - 1; i >= 0; i--)
	XRaiseWindow (CDisplay, edit[i]->parentid);

    CRaiseWindows ();
    update_wlist ();
    free (r);
    return 0;
}


/* format a line for the config file of the current editor */
void print_stuff (char *s, int slen)
{E_
    char hostconfig[256 + 64];
    CWidget *w;
    w = CWidgetOfWindow (edit[current_edit]->parentid);
    *s = 0;
    if (edit[current_edit])
	if (edit[current_edit]->editor->filename && edit[current_edit]->editor->dir)
	    if (*(edit[current_edit]->editor->filename) && *(edit[current_edit]->editor->dir)) {
                char *host;
                hostconfig[0] = '\0';
                if ((host = edit[current_edit]->editor->host) && *host && strcmp (host, REMOTEFS_LOCAL))
                    sprintf (hostconfig, " host=%s", host);
		snprintf (s, slen, "%s %s x=%f y=%f columns=%d lines=%d cursor=%ld topline=%ld%s\n",
			 edit[current_edit]->editor->dir,
			 edit[current_edit]->editor->filename,
			 (float) w->x / (float) FONT_MEAN_WIDTH, (float) w->y / (float) FONT_PIX_PER_LINE, edit[current_edit]->editor->num_widget_columns, edit[current_edit]->editor->num_widget_lines,
			 edit[current_edit]->editor->curs_line,
			 edit[current_edit]->editor->start_line,
                         hostconfig);
            }
}

/* returns 0 on success. Returns 1 on fail and 2 on user cancel */
/* write out the config file. clean = 1: also tries to exit each file. */
/* clean = 2: saves every file before trying to exit. */
static int write_config (int clean)
{E_
    char *f, *t;
    int result = 0;
    char s[1024];
    int i, unsaved, choice;

    for (unsaved = i = 0; i < last_edit; i++)
        unsaved += !!edit[i]->editor->modified;

    if (clean == 1) {
        if (unsaved) {
            char *m;
            m = sprintf_alloc (" You have %d modified files that have not been saved. \n Confirm close all windows without saving... ", unsaved);
            choice = CQueryDialog (edit[current_edit]->mainid, 20, 20, " Confirm ", m, "Cancel", "Confirm", "Save All", NULL);
            free (m);
            if (choice <= 0)
                return 2;
            if (choice == 2)
                clean = 2;
        } else {
            if (CQueryDialog (edit[current_edit]->mainid, 20, 20, " Confirm ", " Close all windows on the desktop... ", "Cancel", "Confirm", NULL) == 0)
                return 2;
        }
    } else if (clean == 2) {
        char *m;
        m = sprintf_alloc (" Save %d modified files and close all windows on the desktop... ", unsaved);
        choice = CQueryDialog (edit[current_edit]->mainid, 20, 20, " Confirm ", m, "Cancel", "Confirm", NULL);
        free (m);
        if (choice <= 0)
            return 2;
    }

    t = f = CMalloc (65536);
    *f = 0;
    current_to_top ();
    current_edit = 0;
    do {
	print_stuff (s, sizeof (s));
	if (clean) {
	    if (edit[current_edit]->editor->modified)
		XRaiseWindow (CDisplay, edit[current_edit]->parentid);
	    if (clean == 2 && edit[current_edit]->editor->modified)
		edit_execute_command (edit[current_edit]->editor, CK_Save, -1);
	    edit_execute_command (edit[current_edit]->editor, CK_Exit, -1);
	    if (edit[current_edit]->editor->stopped == 1) {
		int ce = current_edit;
		print_stuff (s, sizeof (s));	/* user may have changed the filename on exit */
		remove_current (0, 0);
		current_edit = ce;
	    } else {
		result = 2;
		print_stuff (s, sizeof (s));
		current_edit++;
	    }
	} else {
	    current_edit++;
	}
	if (*s) {
	    sprintf (f, "%s", s);
	    f += strlen (s);
	    *f = 0;
	}
    } while (current_edit < last_edit);

/* restack to make it look like something happened: */
    current_edit = last_edit - 1;
    if (current_edit > 10)
        current_edit = 10;
    while (current_edit >= 0) {
	XRaiseWindow (CDisplay, edit[current_edit]->parentid);
	current_edit--;
    };

    current_edit = 0;
    CRaiseWindows ();
    if (save_options_section (editor_options_file, "[Files]", t))
	CErrorDialog (main_window, 20, 20, _(" Save desktop "), get_sys_error (_(" Error trying to save file ")));
    free (t);
    update_wlist ();
    return result;
}

/* }} configuration file handler */

#ifdef HAVE_DND
/* {{ */

/*
   If a filename is dropped onto the main window, open an edit
   window with that file.
 */
int open_drop_file (XEvent * xevent, CEvent * cwevent)
{E_
    unsigned char *data;
    unsigned long size;
    int data_type, xs, ys;
    if (xevent->xany.type != ClientMessage)
	return 0;
    if (xevent->xany.window != main_window)
	return 0;
    data_type = CGetDrop (xevent, &data, &size, &xs, &ys);

    if (data_type == DndNotDnd)
	return 0;
    if (data_type == DndFile) {
	new_file_callback (host, (char *) data);
    } else {
	if (data_type == DndFiles) {
	    unsigned long i = size;
	    while (i--)
		data[i] = data[i] ? data[i] : '\n';
	} else if (data_type != DndRawData && data_type != DndText) {	/* we are going to allow nulls in DndText */
	    size = strnlen ((char *) data, size);
	}
	new_file_callback (host, NEW_WINDOW_FROM_TEXT, (char *) data, (unsigned long) size);
    }
    if (data)
	free (data);
    return 1;
}

#else

static struct drop {
    unsigned char *data;
    int size;
    Atom type;
} drop = {

    0, 0, 0
};

static int handle_drop (void *w_, Window from, unsigned char *data, int size, int xs, int ys, Atom type, Atom action)
{E_
    if (drop.data)
	free (drop.data);
    drop.data = CMalloc (size + 1);
    memcpy (drop.data, data, size);
    drop.size = size;
    drop.data[size] = '\0';
    drop.type = type;
    return 0;
}

/*
   If a filename is dropped onto the main window, open an edit
   window with that file.
 */
char *filename_from_url (char *data, int size, int i);

static void open_drop_file (unsigned char *data, int size, Atom type)
{E_
    if (type == XInternAtom (CDisplay, "url/url", False)) {
	if (!strncmp ((char *) data, "file:", 5)) {
	    char *f;
#warning must extract hostname
	    f = filename_from_url ((char *) data, size, strlen ("file:"));
	    new_file_callback (0, f);
	    free (f);
	} else {
#warning must extract hostname
	    new_file_callback (0, NEW_WINDOW_FROM_TEXT, (char *) data, (unsigned long) size);
	}
    } else {
#warning must extract hostname
	new_file_callback (0, NEW_WINDOW_FROM_TEXT, (char *) data, (unsigned long) size);
    }
    return;
}

/* }} */
#endif

int editors_modified (void)
{E_
    int i, r = 0;
    for (i = 0; i < last_edit; i++)
	r |= edit[i]->editor->modified;
    return r;
}

static void cooledit_init (void)
{E_
    CInitData cooledit_startup;

    memset (&cooledit_startup, 0, sizeof (cooledit_startup));

    cooledit_startup.name = argv_nought;
    cooledit_startup.geometry = option_geometry;
    cooledit_startup.display = option_display;
    cooledit_startup.font = option_font2;
    cooledit_startup.widget_font = option_widget_font2;
    cooledit_startup.bg = option_background_color;
    cooledit_startup.fg_red = option_foreground_red;
    cooledit_startup.fg_green = option_foreground_green;
    cooledit_startup.fg_blue = option_foreground_blue;
    cooledit_startup.look = option_look;

    if (option_verbose)
	cooledit_startup.options = CINIT_OPTION_VERBOSE;

/* initialise: */
    CInitialise (&cooledit_startup);
}

#define DEFAULT_INI_FILE local_home_dir, "/.cedit/.cooledit.ini"
void get_home_dir (void);
void CDrawCooleditMenuButtons (Window parent, int x, int y);
extern int user_defined_key (unsigned int state, unsigned int keycode, KeySym keysym);
extern void execute_script (WEdit * e, int i);
#ifdef HAVE_PYTHON
extern void coolpython_command (WEdit * e, int i);
#endif
int is_focus_prev_key (KeySym k, int command, unsigned int state);

static int initial_columns, initial_lines;
static CEvent cooledit_cevent;
static XEvent cooledit_xevent;

#define approx_equ(a,b) (((a) < (b) + FONT_HEIGHT) && (a) > (b) - FONT_HEIGHT)

static int similar_size_to_main_window (CWidget * w)
{E_
    if (approx_equ (w->width, CIdent ("cooledit")->width) &&
      approx_equ (w->height, CIdent ("cooledit")->height - height_offset)
	&& approx_equ (w->x, 0) && approx_equ (w->y, height_offset))
	return 1;
    return 0;
}

static void maximise_window (char *ident)
{E_
    CWidget *w;
    int lines, columns, f;
    unsigned int wm, hm;
    char x[34];
    strcpy (x, "win");
    strcat (x, ident);
    get_main_window_size (&wm, &hm);
    CPushFont ("widget", 0);
    f = FONT_PIX_PER_LINE;
    CPopFont ();
    CPushFont ("editor", 0);
    lines = hm / FONT_PIX_PER_LINE;
    columns = wm / FONT_MEAN_WIDTH;
    fit_into_main_window (&lines, &columns, 0, height_offset);
    CSetWidgetSize (x, columns * FONT_MEAN_WIDTH + 25 + EDIT_FRAME_W + 4 + 2 + 20 + WIDGET_SPACING * 2,
		    lines * FONT_PIX_PER_LINE + f + EDIT_FRAME_H + WIDGET_SPACING * 3 + 8 + TEXT_RELIEF * 2 + 3 + 3 + 3 + 2);
    w = CIdent (x);
    CSetWidgetPosition (x, (wm - w->width) / 2, height_offset + (hm - height_offset - w->height) / 2);
    CPopFont ();
}


/* ----main_loop()--------------------------------------------------------- */

static void main_loop (void)
{E_
    for (;;) {
	CNextEvent (&cooledit_xevent, &cooledit_cevent);
	if (cooledit_xevent.type == TickEvent) {
	    debug_callback ();
	    continue;
	}
	if (cooledit_xevent.type == Expose || !cooledit_xevent.type
	    || cooledit_xevent.type == InternalExpose)
	    continue;
	switch (cooledit_xevent.type) {
	case AlarmEvent:{
		static int hint_count = 0;
		if (option_hint_messages)
		    if (!((hint_count++) % (CGetCursorBlinkRate () * option_hint_messages)))
			get_next_hint_message ();
		break;
	    }
	case EditorCommand:	/* send by us to the library */
	case KeyPress:
	    if (cooledit_cevent.handled)
		break;
	    if (cooledit_cevent.kind == C_EDITOR_WIDGET || cooledit_cevent.kind == C_MENU_BUTTON_WIDGET || !last_edit) {
		switch ((int) cooledit_cevent.command) {
		case CK_Insert_Unicode: {
			long c;
			if ((c = CUnicodeDialog (main_window, 20, 20, 0)) >= 0) {
			    edit_insert_unicode (edit[current_edit]->editor, c);
			    edit[current_edit]->editor->force |= REDRAW_PAGE;
			}
		    }
		    break;
		case CK_Terminal:
		    rxvt_start (CRoot, 0, 0);
		    break;
		case CK_Complete:
		    complete_command (edit[current_edit]);
		    break;
		case CK_Find_File:
		    find_file ();
		    break;
		case CK_Ctags:
		    ctags ();
		    break;
		case CK_Mail:
#if 0
		    do_mail (edit[current_edit]);
#endif
		    break;
		case CK_Save_Desktop:
		    write_config (0);	/* tries to exit all editors */
		    if (option_pull_down_window_list)
			CPullDown (CIdent ("menu.wlist"));
		    break;
		case CK_New_Window:
		    new_window_callback (0);
		    if (option_pull_down_window_list)
			CPullDown (CIdent ("menu.wlist"));
		    break;
		case CK_Jump_To_File:
		    menu_jump_to_file (0);
		    break;
		case CK_Menu:
		    break;
		default:
		    if (cooledit_cevent.command >= 350 && cooledit_cevent.command < 400)
			debug_key_command (cooledit_cevent.command);
		    else if (is_focus_prev_key (cooledit_cevent.key, cooledit_cevent.command, cooledit_xevent.xkey.state) ||
			cooledit_cevent.command == CK_Cycle) {
			window_cycle_callback (0);
			if (option_pull_down_window_list) {
			    CPullDown (CIdent ("menu.wlist"));
			    XSync (CDisplay, 0);
			}
		    } else if (cooledit_cevent.command == CK_Cancel) {
			edit_insert_shell_output (edit[current_edit]->editor);
		    } else {
			CPullUp (CIdent ("menu.wlist"));
		    }
		    break;
		}
		if (cooledit_cevent.kind != C_MENU_BUTTON_WIDGET) {
		    switch ((int) cooledit_cevent.command) {
		    case CK_Menu:	/* pull down the menu */
			CMenuSelectionDialog (CGetLastMenu ());
			break;
		    case CK_Check_Save_And_Quit:	/* save desktop and quit all */
			exit_app (1);
			break;
		    case CK_Run_Another:	/* new editor window */
			run_main_callback (0);
			break;
		    case CK_Save_And_Quit:
			exit_app (2);
			break;
		    }
		    break;
		}
	    }
	    break;
	    /* when you release the ctrl key, the window must go to the top of the list */
#ifdef HAVE_DND
	case ClientMessage:
	    open_drop_file (&cooledit_xevent, &cooledit_cevent);
	    break;
#endif
	case KeyRelease:
	    if (cooledit_cevent.handled)
		break;
	    if (cooledit_cevent.kind == C_EDITOR_WIDGET)
		if (mod_type_key (CKeySym (&cooledit_xevent))) {
		    current_to_top ();
		    CPullUp (CIdent ("menu.wlist"));
		}
	    break;
	    /* if you click on an edit window, it must go to the top, so... */
	case ButtonRelease:
	    if (cooledit_cevent.kind == C_WINDOW_WIDGET) {
		CWidget *w;
		w = CIdent (cooledit_cevent.ident);
		if (!w)
		    break;
		if (similar_size_to_main_window(w))
		    w->position |= POSITION_HEIGHT | POSITION_WIDTH;
		else
		    w->position &= ~(POSITION_HEIGHT | POSITION_WIDTH);
	    }
	    break;
	case ButtonPress:
	    if (cooledit_cevent.kind == C_EDITOR_WIDGET) {
		int i;
		/* find which one was clicked on: */
		for (i = 0; i < last_edit; i++)
		    if (edit[i]->winid == cooledit_cevent.window) {
			current_edit = i;
			break;
		    }
		XRaiseWindow (CDisplay, edit[current_edit]->parentid);
		CRaiseWindows ();
		current_to_top ();
	    }
	    break;
	}
	if (cooledit_xevent.type == QuitApplication) {
	    if (!editors_modified ()) {
		exit_app (0);
	    } else {
		int i;
		i = CQueryDialog (0, 0, 0,
				  _ (" Quit "), _ (" Quit Cooledit ? "), _ (" Cancel quit "), _ (" Quit and save all "),
				  _ (" Quit, do not save "), NULL);
		switch (i) {
		case 0:

		    break;
		case 1:
		    exit_app (2);
		    break;
		case 2:
		    exit_app (0);
		    break;
		}
	    }
	    continue;
	}
	if (cooledit_cevent.command) {
	    switch ((int) cooledit_cevent.command) {
#ifdef HAVE_PYTHON
	    case CK_Type_Load_Python:
		coolpython_typechange (cooledit_cevent.window);
		break;
#endif
	    case CK_Util:
		CMenuSelectionDialog (CIdent (catstrs (edit[current_edit]->ident, ".util", NULL)));
		break;
	    case CK_Load:
	    case CK_New:
	    case CK_Save_As:
		update_wlist ();
		break;
	    case CK_Man_Page:
		edit_man_page_cmd (CGetEditMenu ()->editor);
		break;
	    case CK_Maximize:
		maximise_window (cooledit_cevent.ident);
		break;
            case CK_Close_Last:
                close_last_callback (0);
                break;
	    }
	}
	/* if an editor has been exitted out of, it must be destroyed: */
	if (last_edit) {
	    if (edit[current_edit]->editor->stopped == 1) {
		char *d;
                char *host;
		d = (char *) strdup (edit[current_edit]->editor->dir);
		host = (char *) strdup (edit[current_edit]->editor->host);
		remove_current (1, 1);
		if (!last_edit) {
		    initial_lines = 200;
		    initial_columns = 400;
		    fit_into_main_window (&initial_lines, &initial_columns, 0, height_offset);
		    new_editor (0, 0, height_offset, initial_columns, initial_lines, 0, host, d);
		    CFocus (edit[current_edit]);
		}
		update_wlist ();
		free (host);
		free (d);
	    }
	}
	if (CDndClass->stage == XDND_DROP_STAGE_IDLE && drop.data) {
	    open_drop_file (drop.data, drop.size, drop.type);
	    free (drop.data);
	    drop.data = 0;
	}
    }
}

static void custom_keys (WEdit * e, int i)
{E_
    if (i < MAX_NUM_SCRIPTS)
	execute_script (e, i);
#ifdef HAVE_PYTHON
    else
	coolpython_command (e, i - MAX_NUM_SCRIPTS);
#endif
}

static char *mime_majors[3] =
{"url", "text", 0};

/* main window only recieves drops, so... */
static struct mouse_funcs main_mouse_funcs =
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    handle_drop,
    0,
    DndText,
    mime_majors
};

extern int (*global_callback) (XEvent *x);
int rxvt_event (XEvent * xevent);
void rxvt_init (void);
const char *get_default_widget_font (void);
const char *get_default_editor_font (void);
const char *get_default_editor_font_large (void);



/* ----main()-------------------------------------------------------------- */
int main (int argc, char **argv)
{
    Window buttonwin;
    CWidget *w;
    int x, y, setsize = 0;
    char *example_fonts[] =
    {
	"-misc-fixed-bold-r-normal--13-120-75-75-c-80",
	"-*-courier-medium-r-normal--13-120-75-75-m-60",
	"-*-times-medium-r-*--14-*-*-*-p-*",
	"-*-helvetica-bold-r-*--14-*-*-*-p-*",
	"-*-charter-bold-r-*--14-*-*-*-p-*",
	"-*-times-medium-r-*--20-*-*-*-p-*",
	"-*-helvetica-bold-r-*--20-*-*-*-p-*",
	"-*-charter-bold-r-*--20-*-*-*-p-*",
	"-*-*-bold-*-normal--10-100-75-75-c-80",
	"-*-*-bold-r-normal--12-120-75-75-c-80",
	"-*-*-bold-r-normal-*-15-*-*-*-c-*",
	"-winfonts-arial-bold-r-*-*-45-*-*-*-*-*-*-*"
    };
    int n;

    init_inspect ();

    command_line_files = (char **) malloc ((argc + 1) * sizeof (char *));
    memset (command_line_files, 0, (argc + 1) * sizeof (char *));
    cooledit_options[0].strs = command_line_files;

    edit_file_is_open = file_is_open;

    argv_nought = (char *) strdup (argv[0]);


#ifdef LC_CTYPE
    if (!setlocale (LC_CTYPE, ""))
	fprintf (stderr, _ ("%s: cannot set locale, see setlocale(3), current locale is %s\n"), argv_nought, setlocale (LC_MESSAGES, 0));
    setlocale (LC_TIME, "");
    setlocale (LC_MESSAGES, "");
#endif
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);

    n = sizeof (example_fonts) / sizeof (char *);

    process_command_line (argc, argv);

    if (option_server && *option_server) {
        remotefs_serverize ("0.0.0.0", option_server);
        exit (0);
    }

    if (option_aa_font != -1) {
	fprintf (stderr, "cooledit: --[no-]anti-aliasing is depreciated, use <fontname>/3 instead\n");
	exit (1);
    }

    if (option_font2)
	if (!strcmp (option_font2, "?") || !strcmp (option_font2, "h")
	 || !strcmp (option_font2, "-?") || !strcmp (option_font2, "-h")) {
	    int i;
	    printf ("\n");
	    printf ("\n");
	    printf ("X11 Font Examples: \n");
	    for (i = 0; i < n; i++)
		printf ("\tcooledit -font '%s'\n", example_fonts[i]);
	    printf ("\tcooledit -font default\n" \
		    "\tcooledit -font large # use 9x15B instead of 8x13B\n" \
		    "\tcooledit -font 8x13bold\n" \
		    "\tcooledit -font 1-%d\n", n);
	    printf ("\n");
	    printf ("Examples using fonts on file on the local machine: \n");
	    printf ("\tcooledit -font NotoSansMono-Bold.ttf:14\n");
	    printf ("\tcooledit -font 8x13B.pcf.gz,NotoColorEmoji-OLD.ttf:35 --widget-font  NotoSans-Regular.ttf:14     # then hit  Alt-I  then  End  then  PgDn  until 0x1F300\n");
	    printf ("\tcooledit -font 9x15B.pcf.gz\n");
	    printf ("\tcooledit -font /usr/share/fonts/truetype/ttf-dejavu/DejaVuSans-Bold.ttf:20\n");
	    printf ("\n");
	    printf ("The default font is:\n");
	    printf ("----\n");
	    printf ("\tcooledit -font '%s'\n", get_default_editor_font ());
	    printf ("----\n");
	    printf ("\n");
	    exit (1);
	}

    get_home_dir ();
    password_init ();

    if (!editor_options_file)
	editor_options_file = (char *) strdup (catstrs (DEFAULT_INI_FILE, NULL));


    if (!option_suppress_load_options) {
	char *p;
	load_setup (editor_options_file);
	put_all_lists (p = get_options_section (editor_options_file, "[Input Histories]"));
	if (p)
	    free (p);
    }

    if (option_minimal_main_window)
	option_geometry = 0;

    if (!option_command_line_doesnt_override) {
        get_cmdline_options_free_list (&cmdline_fl);
	process_command_line (argc, argv);
    }

    if (option_font2) {
	if (!strcmp (option_font2, "default"))
	    option_font2 = (char *) strdup (get_default_editor_font ());
	if (!strcmp (option_font2, "large"))
	    option_font2 = (char *) strdup (get_default_editor_font_large ());
    }

    if (option_widget_font2)
	if (!strcmp (option_widget_font2, "default"))
	    option_widget_font2 = (char *) strdup (get_default_widget_font ());

    if (option_font2)
	if (strspn (option_font2, "0123456789") == strlen (option_font2)) {
	    int i;
	    i = atoi (option_font2);
	    if (i < 1 || i > n) {
		printf (_ ("Allowable range is 1 to %d\n"), n);
		exit (1);
	    }
	    option_font2 = (char *) strdup (example_fonts[i - 1]);
	}
    initial_columns = start_width;
    initial_lines = start_height;

    rxvt_init ();
    cooledit_init ();

    set_editor_encoding (option_utf_interpretation2, option_locale_encoding);

    main_window = CDrawMainWindow ("cooledit", "Cooledit");
    xdnd_set_dnd_aware (CDndClass, main_window, 0);
    w = CWidgetOfWindow (main_window);
    w->funcs = mouse_funcs_new (w, &main_mouse_funcs);

    if (!Cstrcasecmp (init_bg_color, "igloo"))
	CSetBackgroundPixmap ("cooledit", igloo_data, 220, 156, 'A');

    load_keys (editor_options_file);

#ifdef DEBUG
    XSetErrorHandler (er_handler);
#endif

/* draw window for the menubuttons to go on: */
    buttonwin = CDrawDialog ("menu", main_window, 0, 0);
/* this window must never be below anything: */
    CIdent ("menu")->position = WINDOW_ALWAYS_RAISED;
    CGetHintPos (&x, &y);
    CDrawMenuButton ("menu.wlist", buttonwin, main_window,
/* Title of the 'Window' pull down menu */
		     x, y, AUTO_SIZE, 1, _ (" Window "),
		     _ (" New window\tCtrl-F3 "), 0, 0, 0);
/* Toolhint for the 'Window' menu button */
    CSetToolHint ("menu.wlist", _ ("Manipulating the desktop"));
    CGetHintPos (&x, 0);
    CDrawEditMenuButtons ("menu", buttonwin, main_window, x, y);
    CGetHintPos (&x, 0);
    CDrawCooleditMenuButtons (buttonwin, x, y);

    CSetSizeHintPos ("menu");
    CMapDialog ("menu");
    height_offset = (CIdent ("menu"))->height + (CIdent ("menu"))->y;

    if (!option_suppress_load_files && !option_suppress_load_files_cmdline)
	read_config ();

    edit_set_user_key_function (user_defined_key);
    edit_set_user_command (custom_keys);

#ifdef HAVE_PYTHON
/* this must be done before loading files */
    coolpython_init (argc, argv);
#endif
    debug_init ();

    if (command_line_files[0]) {
	int i;
	long l = 0, num_read = 0;
	for (i = 0; command_line_files[i]; i++) {
	    if (command_line_files[i][0] == '+') {
		l = atoi (command_line_files[i] + 1) - 1;
		if (l < 0)
		    l = 0;
		continue;
	    }
	    else if (new_editor (num_read, 0, height_offset, initial_columns, initial_lines, command_line_files[i], 0, 0)) {
		edit_move_display (edit[num_read]->editor, l);
		edit_move_to_line (edit[num_read]->editor, l);
		num_read++;
	    }
	    l = 0;
	}
	if (edit[0])
	    XRaiseWindow (CDisplay, edit[0]->parentid);
	CRaiseWindows ();
	update_wlist ();
    }
    if (!last_edit) {
/* no windows are open (no config initial_lines or commandline files so) */
#if 0
	if (w->options & WINDOW_USER_SIZE)
	    initial_lines = initial_columns = 200;
	fit_into_main_window (&initial_lines, &initial_columns, 0, height_offset);
#endif
	new_editor (0, 0, height_offset, initial_columns, initial_lines, 0, 0, current_dir[0] ? current_dir : local_home_dir);
	current_edit = 0;
	update_wlist ();
	setsize = 1;
    }

    CSetEditMenu (edit[0]->ident);
    CSetLastMenu (CIdent ("menu.wlist"));
    current_to_top ();

    CSetCursorBlinkRate (option_cursor_blink_rate);

    shell_set_alt_modifier (alt_modifier_mask);
    load_scripts ();
    update_script_menu_items ();

    set_signals ();

    CFocus (edit[current_edit]);

    if (setsize || !(w->options & WINDOW_USER_SIZE))	/* this means that the size was specified by the user in 'geometry' */
	CSetWidgetSize ("cooledit", extents_width, extents_height);

    CPushFont ("editor", 0);
    CSetWindowResizable ("cooledit", 300, 300, 20000, 20000);
    CPopFont ();
/* Toolhint */
    CSetToolHint ("cooledit", _ ("Drop a filename or text onto this space"));

    CMapDialog ("cooledit");

    for (n = 0; n < last_edit; n++) {
	char p[32];
	strcpy (p, "win");
	strcat (p, edit[n]->ident);
	w = CIdent (p);
	if (!w)
	    continue;
	if (similar_size_to_main_window (w))
	    w->position |= POSITION_HEIGHT | POSITION_WIDTH;
	else
	    w->position &= ~(POSITION_HEIGHT | POSITION_WIDTH);
    }

#if 0
    {
	Window win;
	int xe, ye, t;
	win = CDrawDialog ("rxvtwin", main_window, 0, 0);
	CGetHintPos (&xe, &ye);
	w = (CWidget *) CDrawRxvt ("rxvt", win, xe, ye, 0);
	w->position |= POSITION_WIDTH | POSITION_HEIGHT;
	CSetSizeHintPos ("rxvtwin");
/* font height needed for resizing granularity - well, i guest thats why they call it hacking */
	CPushFont ("editor", 0);
	t = option_text_line_spacing;
	option_text_line_spacing = 0;
	CSetWindowResizable ("rxvtwin", 150, 150, 1600, 1200);
	option_text_line_spacing = t;
	CPopFont ();
	CMapDialog ("rxvtwin");
    }
#endif

    global_callback = rxvt_event;

    main_loop ();
    return 0;
}

