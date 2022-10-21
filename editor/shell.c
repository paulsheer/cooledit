/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* shell.c - user defined shell commands 
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include "editoptions.h"
#include "stringtools.h"
#include "shell.h"
#include "pool.h"
#include "remotefs.h"

#define shell_error_dialog(h,t) CErrorDialog(e->widget->winid,20,20,h,"%s",t)

#undef gettext_noop
#define gettext_noop(x) x


extern struct look *look;

int save_options_section (const char *file, const char *section, const char *text);
static void save_scripts (void);
static int shell_string_to_option (const char *s);
static char *shell_option_to_string (int option_flags);
extern char *editor_options_file;
extern char *init_font;
extern Window main_window;


struct modifier_map_ {
    const char *tag;
    unsigned long flag;
};
#define SHELL_ALT_MASK_UNSET            0x12345678UL

struct modifier_map_ modifier_map[] = {
    {"Ctrl", (unsigned long) ControlMask},
    {"Alt", SHELL_ALT_MASK_UNSET},
    {"Shift", (unsigned long) ShiftMask},
    {"0x00000001", 0x00000001UL},
    {"0x00000002", 0x00000002UL},
    {"0x00000004", 0x00000004UL},
    {"0x00000008", 0x00000008UL},
    {"0x00000010", 0x00000010UL},
    {"0x00000020", 0x00000020UL},
    {"0x00000040", 0x00000040UL},
    {"0x00000080", 0x00000080UL},
    {"0x00000100", 0x00000100UL},
    {"0x00000200", 0x00000200UL},
    {"0x00000400", 0x00000400UL},
    {"0x00000800", 0x00000800UL},
    {"0x00001000", 0x00001000UL},
    {"0x00002000", 0x00002000UL},
    {"0x00004000", 0x00004000UL},
    {"0x00008000", 0x00008000UL},
    {"0x00010000", 0x00010000UL},
    {"0x00020000", 0x00020000UL},
    {"0x00040000", 0x00040000UL},
    {"0x00080000", 0x00080000UL},
    {"0x00100000", 0x00100000UL},
    {"0x00200000", 0x00200000UL},
    {"0x00400000", 0x00400000UL},
    {"0x00800000", 0x00800000UL},
    {"0x01000000", 0x01000000UL},
    {"0x02000000", 0x02000000UL},
    {"0x04000000", 0x04000000UL},
    {"0x08000000", 0x08000000UL},
    {"0x10000000", 0x10000000UL},
    {"0x20000000", 0x20000000UL},
    {"0x40000000", 0x40000000UL},
    {"0x80000000", 0x80000000UL}
};

static unsigned long shell_alt_mask = SHELL_ALT_MASK_UNSET;

void shell_set_alt_modifier (unsigned long modifier)
{E_
    int i;
    shell_alt_mask = modifier;
    for (i = 0; i < sizeof (modifier_map) / sizeof (modifier_map[0]); i++)
	if (!strcmp ("Alt", modifier_map[i].tag)) {
	    modifier_map[i].flag = modifier;
            break;
        }
}

static char *shell_modifier_to_string (unsigned long modifiers)
{E_
    static char s[1024];
    int i, l;
    s[0] = '\0';
    for (i = 0; i < sizeof (modifier_map) / sizeof (modifier_map[0]); i++)
	if ((modifier_map[i].flag & modifiers)) {
            modifiers &= ~(modifier_map[i].flag);
	    strcat (s, modifier_map[i].tag);
	    strcat (s, ",");
	}
    l = strlen (s);
    if (l > 0 && s[l - 1] == ',')
	s[l - 1] = '\0';
    return s;
}

static unsigned long shell_string_to_modifier (const char *s)
{E_
    int i;
    unsigned long r = 0;
    for (i = 0; i < sizeof (modifier_map) / sizeof (modifier_map[0]); i++)
	if (strstr (s, modifier_map[i].tag))
	    r |= modifier_map[i].flag;
    return r;
}


#define NoOtherMask             0

/* Here are the default example shells */
struct shell_cmd default_scripts[] =
{
    {
	" C/C++: Complete ",
	"C/C++ Complete...\tCtrl-.",
	'~',
	'.',
        0,
	ControlMask,
	"",
	SHELL_OPTION_COMPLETE_WORD | SHELL_OPTION_SAVE_EDITOR_FILE,
	0,
	"#!/bin/sh\n" \
	"\n" \
	"test -f %p/compile_options || { ( echo 'For compiler options like includes and defines' ; \n" \
	"echo '(for example -I../include or -DHAVE_STRDUP)' ; \n" \
	"echo 'you need to create a file compile_options under the directory' ; \n" \
	"echo '%p' ) 1>&2 ; exit 0 ; } \n" \
	"\n" \
	"cd %p\n" \
	"\n" \
	"CLANG_SYSTEM_INCLUDE=`echo '#include <stdio.h>' | \\\n" \
	"    clang -xc -v - 2>&1 | \\\n" \
	"    sed '/include.*search starts here/,/End of search list/!D' | \\\n" \
	"    grep llvm.*include | \\\n" \
	"    head -1`\n" \
	"\n" \
	"coolcref -I`echo $CLANG_SYSTEM_INCLUDE` %p/compile_options %p/%f %L %C complete 2>/dev/null\n" \
	"\n" 
    },
    {
	" C/C++: Goto Decl ",
	"C/C++ Goto Decl...\tCtrl-/",
	'~',
	'/',
        0,
	ControlMask,
	"",
	SHELL_OPTION_GOTO_FILE_LINE_COLUMN | SHELL_OPTION_SAVE_EDITOR_FILE,
	0,
	"#!/bin/sh\n" \
	"\n" \
	"test -f %p/compile_options || { ( echo 'For compiler options like includes and defines' ; \n" \
	"echo '(for example -I../include or -DHAVE_STRDUP)' ; \n" \
	"echo 'you need to create a file compile_options under the directory' ; \n" \
	"echo '%p' ) 1>&2 ; exit 0 ; } \n" \
	"\n" \
	"cd %p\n" \
	"\n" \
	"CLANG_SYSTEM_INCLUDE=`echo '#include <stdio.h>' | \\\n" \
	"    clang -xc -v - 2>&1 | \\\n" \
	"    sed '/include.*search starts here/,/End of search list/!D' | \\\n" \
	"    grep llvm.*include | \\\n" \
	"    head -1`\n" \
	"\n" \
	"coolcref -I`echo $CLANG_SYSTEM_INCLUDE` %p/compile_options %p/%f %L %C\n" \
	"\n" 
    },
    {
	" CLang Check ",
	"CLang Check...\tAlt-.",
	'~',
	'.',
        1,
	NoOtherMask,
	"",
	SHELL_OPTION_ANNOTATED_BOOKMARKS | SHELL_OPTION_SAVE_EDITOR_FILE,
	0,
        "#!/bin/sh\n" \
        "\n" \
        "cd %p\n" \
        "\n" \
        "if test -f compile_options ; then\n" \
        "    S=`cat compile_options`\n" \
        "    COPT=`echo $S | sed -e 's/\\\\\\\\/\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/g' | sed -e 's/\\\"/\\\\\\\\\\\\\"/g'`\n" \
        "else\n" \
        "    COPT=''\n" \
        "    echo '%f:1:1 create file %p/compile_options with included -I and -D options for your build'\n" \
        "fi\n" \
        "\n" \
        "T=~/.cedit/clang/compile_commands.json\n" \
        "\n" \
        "mkdir -p ~/.cedit/clang\n" \
        "echo '[' > $T\n" \
        "echo '  {' >> $T\n" \
        "echo \"    \"command\": \\\"gcc -c $COPT %f\\\",\" >> $T\n" \
        "echo '    \"directory\": \"%p\",' >> $T\n" \
        "echo '    \"file\": \"%f\"' >> $T\n" \
        "echo '  }' >> $T\n" \
        "echo ']' >> $T\n" \
        "\n" \
        "# -extra-arg=\"-Weverything\" \n" \
        "\n" \
        "clang-check -p ~/.cedit/clang %f > ~/.cedit/clang/out 2>&1\n" \
        "\n" \
        "cat ~/.cedit/clang/out\n" \
        "\n"
    },
    {
	" Sed ",
	"Sed...\tCtrl-Alt-d",
	'~',
	XK_d,
        1,
	ControlMask,
	gettext_noop (" Enter sed arguments (see sed manpage) : "),
	SHELL_OPTION_SAVE_BLOCK | SHELL_OPTION_REQUEST_ARGUMENTS |
	SHELL_OPTION_DELETE_BLOCK | SHELL_OPTION_INSERT_STDOUT |
	SHELL_OPTION_DISPLAY_ERROR_FILE | SHELL_OPTION_CHECK_ERROR_FILE,
	0,
	"#!/bin/sh\n" \
	"cat %b | sed %a 2>%e\n"
    },
    {
	gettext_noop (" Indent "),
	gettext_noop ("'indent' C Formatter\tShift-F9"),
	'~',
	XK_F9,
        0,
	ShiftMask,
	"",
	SHELL_OPTION_SAVE_BLOCK | SHELL_OPTION_DELETE_BLOCK |
	SHELL_OPTION_DISPLAY_ERROR_FILE | SHELL_OPTION_CHECK_ERROR_FILE |
	SHELL_OPTION_INSERT_BLOCK_FILE,
	0,
	"#!/bin/sh\n" \
	"I=indent\n" \
	"if which gindent >/dev/null 2>&1 ; then I=gindent ; fi\n" \
	"$I -kr -pcs %b 2>%e\n"
    },
    {
	gettext_noop (" Sort "),
	gettext_noop ("Sort...\tAlt-t"),
	'~',
	XK_t,
        1,
	NoOtherMask,
	gettext_noop (" Enter sort options (see sort manpage) : "),
	SHELL_OPTION_SAVE_BLOCK | SHELL_OPTION_REQUEST_ARGUMENTS |
	SHELL_OPTION_DELETE_BLOCK | SHELL_OPTION_INSERT_STDOUT |
	SHELL_OPTION_DISPLAY_ERROR_FILE | SHELL_OPTION_CHECK_ERROR_FILE,
	0,
	"#!/bin/sh\n" \
	"sort %a %b 2>%e\n"
    },
    {
	" Ispell/Aspell ",
	gettext_noop ("'ispell/aspell' Spell Check\tCtrl-p"),
	'~',
	XK_p,
        0,
	ControlMask,
	"",
	SHELL_OPTION_SAVE_BLOCK | SHELL_OPTION_DELETE_BLOCK |
	SHELL_OPTION_INSERT_BLOCK_FILE,
	0,
	"#!/bin/sh\n" \
        "which ispell >/dev/null 2>&1 && {\n"
	"   exec " XTERM_CMD " -e ispell %b\n"
        "}\n"
        "which aspell >/dev/null 2>&1 && {\n"
	"   exec " XTERM_CMD " -e aspell -c %b\n"
        "}\n"
    },
    {
	" Make ",
	gettext_noop ("Run make in curr. dir\tAlt-F7"),
	'~',
	XK_F7,
	1,
        NoOtherMask,
	"",
	SHELL_OPTION_DISPLAY_STDOUT_CONTINUOUS |
	SHELL_OPTION_DISPLAY_STDERR_CONTINUOUS,
	0,
	"#!/bin/sh\n" \
	"cd %d\n" \
	"make\n" \
	"echo Done\n"
    },
    {
	gettext_noop (" Terminal Application "),
	gettext_noop ("Run Terminal App...\tCtrl-Alt-e"),
	'~',
	XK_e,
	1,
        ControlMask,
	gettext_noop (" Enter command : "),
	SHELL_OPTION_REQUEST_ARGUMENTS |
	SHELL_OPTION_RUN_IN_BACKGROUND,
	0,
	"#!/bin/sh\n" \
	XTERM_CMD " -e %a\n"
    }
};

char *hme (char *text);
char *substitute_strings (char *text, char *cmdline_options, char *editor_file, int current_line, int current_column);



/* {{{ dynamic display of shell output in a dialog box */

#define MAX_RUNNING_SHELLS 32
static struct running_shell {
    pid_t shell_pid;
    int shell_pipe;
    POOL *shell_pool;
    const char *shell_name;
    CWidget *w;
    int killme;
} running_shell[MAX_RUNNING_SHELLS];

static void kill_process (pid_t p)
{E_
    if (p)
	kill (p, SIGTERM);
}


/* one of these must be non-zero */
static int find_shell (pid_t p, const char *name, CWidget * w)
{E_
    int i;
    for (i = 0; i < MAX_RUNNING_SHELLS; i++) {
	if (p)
	    if (running_shell[i].shell_pid == p)
		return i;
	if (name && running_shell[i].shell_name)
	    if (*name)
		if (!strcmp (name, running_shell[i].shell_name))
		    return i;
	if (w)
	    if ((unsigned long) w == (unsigned long) running_shell[i].w)
		return i;
    }
    return -1;
}

static int new_shell (const char *name)
{E_
    int i;
    i = find_shell (0, name, 0);
    if (i < 0) {
	for (i = 0; i < MAX_RUNNING_SHELLS; i++)
	    if (!running_shell[i].shell_name) {
		memset (&running_shell[i], 0, sizeof (struct running_shell));
		running_shell[i].shell_pipe = -1;
		running_shell[i].shell_name = name;	/* FIXME: strdup() then free() later - static is ok for now */
		return i;
	    }
    }
    return -1;
}

void text_free (void *x)
{E_
    if (x)
	free (x);
}

static void shell_pool_update (int fd, fd_set * reading, fd_set * writing, fd_set * error, void *data);

void shell_free_pool (int i)
{E_
    if (i < 0)
	return;
    if (running_shell[i].shell_pipe >= 0) {
	CRemoveWatch (running_shell[i].shell_pipe , shell_pool_update, WATCH_READING);
	close (running_shell[i].shell_pipe);
    }
    memset (&running_shell[i], 0, sizeof (struct running_shell));
    running_shell[i].shell_pipe = -1;
}

/* kills an executing shell whose output is being dynamically displayed */
void set_to_kill (pid_t p)
{E_
    int i;
    i = find_shell (p, 0, 0);
    if (i >= 0)
	running_shell[i].killme = 1;
}

/* kills an executing shell whose output is being dynamically displayed */
static int kill_shell (pid_t p, char *name, CWidget * w)
{E_
    int i;
    i = find_shell (p, name, w);
    if (i < 0)
	return -1;
    kill_process (running_shell[i].shell_pid);
    set_to_kill (running_shell[i].shell_pid);
    return i;
}

static int restart_shell (pid_t p, const char *name, CWidget * w)
{E_
    int i;
    i = find_shell (p, name, w);
    if (i < 0)
	return new_shell (name);
    if (running_shell[i].shell_pipe >= 0) {
	CRemoveWatch (running_shell[i].shell_pipe, shell_pool_update, WATCH_READING);
	close (running_shell[i].shell_pipe);
    }
    kill_process (running_shell[i].shell_pid);
    running_shell[i].shell_pid = 0;
    running_shell[i].shell_pipe = -1;
    running_shell[i].killme = 0;
    return i;
}

void insert_text_into_current (char *text, int len, int delete_word_left);

#define COMPLETE_LIST_SEPARATOR         "  ==>  "

static int complete_select_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    if (c->double_click || (c->command == CK_Enter && !c->handled)) {
	char *q, *p;
        q = strdup(CGetTextBoxLine(w, w->cursor));
	CDestroyWidget ("complete");
        p = strstr (q, COMPLETE_LIST_SEPARATOR);
        if (p)
            insert_text_into_current (q, p - q, 1);
        free(q);
    }
    if (c->command == CK_Cancel)
	CDestroyWidget ("complete");
    return 0;
}

static int complete_done_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    CDestroyWidget ("complete");
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

static char *format_complete_list (const char *s, const char *token)
{E_
    int c, color = 0, newline = 1;
    POOL *p;
    p = pool_init ();

    while ((c = *s)) {
        if (newline) {
            if (strncmp (token, s, strlen (token))) {
                while (*s && *s != '\n')
                    s++;
                if (*s == '\n')
                    s++;
                continue;
            }
        }
	if (c == '\n') {
	    color = 0;
	    pool_printf (p, "\n");
	    s++;
            newline = 1;
	    continue;
	}
        newline = 0;
	if (c == '\t') {
	    color = 1;
	    pool_printf (p, "%s", COMPLETE_LIST_SEPARATOR);
	    s++;
	    continue;
	}

	if (color) {
	    pool_printf (p, "_\b%c", (int) (unsigned char) c);
	    s++;
	} else {
	    unsigned char ch;
            ch = (unsigned char) c;
	    pool_write (p, &ch, 1);
	    s++;
	}
    }

    pool_null (p);
    return (char *) pool_break (p);
}

static void complete_dialog (char *text, const char *token)
{E_
    const char *heading, *tool_hint, *formatted_text;
    int x, y;
    Window win;
    CWidget *w;
    heading = "Select Text";
    tool_hint = "These are the possible completion text items";
    if (CIdent ("complete"))
	return;
    win = CDrawHeadedDialog ("complete", main_window, 20, 20, heading);
    CIdent ("complete")->position = WINDOW_ALWAYS_RAISED;
    CGetHintPos (&x, &y);
    CPushFont ("editor", 0);
    formatted_text = format_complete_list (text, token);
    free (text);
    w = CDrawTextboxManaged ("complete.text", win, x, y, 70 * FONT_MEAN_WIDTH + EDIT_FRAME_W, 20 * FONT_PIX_PER_LINE + EDIT_FRAME_H, 0, 0, const_get_text_cb, const_free_text, (void *) formatted_text, (void *) strlen(formatted_text), 0);
    if (tool_hint)
	CSetToolHint ("complete.text", tool_hint);
    w->position |= POSITION_HEIGHT | POSITION_WIDTH;
    (CIdent ("complete.text.vsc"))->position |= POSITION_HEIGHT | POSITION_RIGHT;
    CGetHintPos (0, &y);
    (CDrawPixmapButton ("complete.done", win, 0, y, PIXMAP_BUTTON_CROSS))->position = POSITION_BOTTOM | POSITION_CENTRE;
    CCentre ("complete.done");
    CSetSizeHintPos ("complete");
    CSetWindowResizable ("complete", FONT_MEAN_WIDTH * 15, FONT_PIX_PER_LINE * 15, 1600, 1200);
    CPopFont ();
    CMapDialog ("complete");
    CAddCallback ("complete.text", complete_select_callback);
    CAddCallback ("complete.done", complete_done_callback);
    CFocus (CIdent ("complete.text"));
}

void goto_error (char *message, int raise_wm_window);

/* if you double click on a line of gcc output, this will take you to the file */
static int goto_file_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    if (c->double_click || (c->command == CK_Enter && !c->handled)) {
	int width;
	char *q;
        CStr s;
	CPushFont ("editor", 0);
	width = w->options & TEXTBOX_WRAP ? (w->width - TEXTBOX_BDR) / FONT_MEAN_WIDTH : 32000;
	CPopFont ();
        s = (*w->textbox_funcs->textbox_text_cb) (w->textbox_funcs->hook1, w->textbox_funcs->hook2);
	q = strline (s.data, strmovelines (s.data, w->current, w->cursor - w->firstline, width));
	goto_error (q, 1);
    }
    return 0;
}

static char *nm (int i, char *a, char *b, char *c)
{E_
    static char id[36];
    sprintf (id, "%.3d%s", i, a);
    if (b) {
	strcat (id, ".");
	strcat (id, b);
	if (c) {
	    strcat (id, ".");
	    strcat (id, c);
	}
    }
    return id;
}

/* if the dynamic output dialog's tick button is click, then terminate: */
static int display_file_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    shell_free_pool (kill_shell (0, 0, CIdent (nm (atoi (w->ident), "shelldisplaytext", "text", 0))));
    CDestroyWidget (nm (atoi (w->ident), "shelldisplaytext", 0, 0));
    return 1;
}

#define CHUNK 8192

static void shell_pool_update (int fd, fd_set * reading, fd_set * writing, fd_set * error, void *data)
{E_
    CWidget *w;
    struct running_shell *r;
    int i, count = 0, do_redraw = 0;
    int old_len;
    r = &running_shell[i = (int) (unsigned long) data];
    if (r->shell_pipe != fd) {
	printf ("huh??\n");
    }
/* first check if we are still on the screen */
    w = CIdent (nm (i, "shelldisplaytext", "text", 0));
    if (!w || !r->shell_pool) { /* defensive programming */
	shell_free_pool (kill_shell (0, 0, w));
	return;
    }
/* read a little */
    old_len = pool_length(r->shell_pool);
    for (;;) {
	int c, j;
	unsigned char *p;
	if (pool_freespace (r->shell_pool) < CHUNK + 1) {
	    pool_advance (r->shell_pool, CHUNK + 1);
	    do_redraw = 1;	/* must redraw cause pool has changed */
	}
	while ((c = read (fd, pool_current (r->shell_pool), CHUNK)) == -1 && errno == EINTR);
/* translate unreadables */
	for (j = 0, p = (unsigned char *) pool_current (r->shell_pool); j < c; j++, p++) {
	    if (*p == '\t')
		*p = ' ';
	    else if (*p == '\n')
		*p = '\n';
	    else if (!FONT_PER_CHAR(*p))
		*p = '?';
	}
	if (c <= 0)
	    break;
	count += c;
	pool_current (r->shell_pool) += c;
	break;
    }
    pool_null (r->shell_pool);	/* adds a zero to the end */
/* do we need to refresh? : */
    if (count || do_redraw) {
        int more_lines;
/* we do a manual appending of the text for optimization purposes, instead of CRedrawTextbox */
	CPushFont ("editor", 0);
        more_lines = strcountlines ((char *) pool_start (r->shell_pool) + old_len, 0, 2000000000, w->options & TEXTBOX_WRAP ? (w->width - TEXTBOX_BDR) / FONT_MEAN_WIDTH : 32000);
	w->numlines += more_lines;
/* only redraw if we crossed a newline */
	if (more_lines) {
	    CExpose (w->ident);
	    if (w->winid != CGetFocus ()) {
		if (w->numlines > w->firstline + (w->height / FONT_PIX_PER_LINE - 1))
		    CSetTextboxPos (w, TEXT_SET_LINE, w->numlines - (w->height / FONT_PIX_PER_LINE - 1));
            }
	}
	CPopFont ();
    }
    r->killme |= CChildExitted (r->shell_pid, 0);
    if (!count && r->killme) {
	CRemoveWatch (fd, shell_pool_update, WATCH_READING);
	close (fd);
	memset (r, 0, sizeof (struct running_shell));
	r->shell_pipe = -1;
    }
}

static CStr shell_get_text_cb (void *hook1, void *hook2)
{E_
    POOL *shell_pool;
    CStr s;
    shell_pool = (POOL *) hook1;
    if (!shell_pool) {
        s.data = "";
        s.len = 0;
        return s;
    }
    pool_null (shell_pool);
    s.data = (char *) pool_start (shell_pool);
    if (!s.data) {
        s.data = (char *) "";
        s.len = 0;
        return s;
    }
    s.len = pool_length (shell_pool);
    return s;
}

static void shell_free_text (void *hook1, void *hook2)
{E_
    POOL *shell_pool;
    shell_pool = (POOL *) hook1;
    if (shell_pool)
        pool_free (shell_pool);
}

/* draws a textbox dialog for showing the shells output */
static void shell_display_output (int i, const char *heading, int (*select_line_callback) (CWidget *, XEvent *, CEvent *))
{E_
    if (CIdent (nm (i, "shelldisplaytext", 0, 0))) {	/* exists ? */
	CRedrawTextboxManaged (nm (i, "shelldisplaytext", "text", 0), shell_get_text_cb, shell_free_text, (void *) running_shell[i].shell_pool, 0, 0);
	CRedrawText (nm (i, "shelldisplaytext", "header", 0), heading);
	CTryFocus (CIdent (nm (i, "shelldisplaytext", "text", 0)), 1);
    } else {
	int x, y;
	Window win;
	CWidget *w;
	win = CDrawMainWindow (nm (i, "shelldisplaytext", 0, 0), heading);
	CGetHintPos (&x, &y);
	CPushFont ("editor", 0);
	running_shell[i].w = w = CDrawTextboxManaged (nm (i, "shelldisplaytext", "text", 0), win, x, y, 80 * FONT_MEAN_WIDTH + 7, 25 * FONT_PIX_PER_LINE + 6, 0, 0, shell_get_text_cb, shell_free_text, (void *) running_shell[i].shell_pool, 0, TEXTBOX_WRAP);
/* Toolhint */
	CSetToolHint (nm (i, "shelldisplaytext", "text", 0), _ ("Double click on file:line type messages to goto the\nfile and line number.  Note that the file will not\nauto-load unless there is a full path in the message."));
	w->position |= POSITION_HEIGHT | POSITION_WIDTH;
	(CIdent (nm (i, "shelldisplaytext", "text", "vsc")))->position |= POSITION_HEIGHT | POSITION_RIGHT;
	CGetHintPos (0, &y);
	(CDrawPixmapButton (nm (i, "shelldisplaytext", "done", 0), win, 0, y, PIXMAP_BUTTON_TICK))->position = POSITION_BOTTOM | POSITION_CENTRE;
/* Toolhint */
	CSetToolHint (nm (i, "shelldisplaytext", "done", 0), _ ("Kill the running script"));
	CCentre (nm (i, "shelldisplaytext", "done", 0));
	CSetSizeHintPos (nm (i, "shelldisplaytext", 0, 0));
	CSetWindowResizable (nm (i, "shelldisplaytext", 0, 0), FONT_MEAN_WIDTH * 15, FONT_PIX_PER_LINE * 15, 1600, 1200);	/* minimum and maximum sizes */
	CPopFont ();
	CMapDialog (nm (i, "shelldisplaytext", 0, 0));
	CAddCallback (nm (i, "shelldisplaytext", "text", 0), select_line_callback);
	CAddCallback (nm (i, "shelldisplaytext", "done", 0), display_file_callback);
	CFocus (CIdent (nm (i, "shelldisplaytext", "done", 0)));
    }
}

/* }}} dynamic display of shell output in a dialog box */

static char *hme_i (char *h, int i)
{E_
    static char s[MAX_PATH_LEN];
    sprintf (s, "%s-%d", hme (h), i);
    return s;
}

/* returns non-zero on error */
int execute_background_display_output (const char *title, const char *s, const char *name)
{E_
    char *argv[] =
    {0, 0};
    pid_t p;
    int i;
    i = restart_shell (0, name, 0);
    argv[0] = hme_i (SCRIPT_FILE, i);
    savefile (argv[0], s, strlen (s), 0700);
    if ((p = triple_pipe_open (0, &running_shell[i].shell_pipe, 0, 1, argv[0], argv)) < 0)
	return 1;
    running_shell[i].shell_pid = p;
    running_shell[i].shell_pool = pool_init ();
    pool_null (running_shell[i].shell_pool);
    shell_display_output (i, title, goto_file_callback);
    CAddWatch (running_shell[i].shell_pipe, shell_pool_update, WATCH_READING, (void *) (unsigned long) i);
    return 0;
}

/*
   Executes the shell in the background. The "Insert File" options
   will be ignored if this is called.
 */
static int execute_background_shell (struct shell_cmd *s, char *script, char *name)
{E_
    char *argv[] =
    {0, 0};
    pid_t p = 0;
    int i;

    i = restart_shell (0, name, 0);
    argv[0] = hme_i (SCRIPT_FILE, i);
    savefile (argv[0], script, strlen (script), 0700);

    if (s->options & SHELL_OPTION_DISPLAY_STDOUT_CONTINUOUS) {
	if ((p = triple_pipe_open (0, &running_shell[i].shell_pipe, 0,
	   (s->options & SHELL_OPTION_DISPLAY_STDERR_CONTINUOUS) ? 1 : 0,
				   hme_i (SCRIPT_FILE, i), argv)) < 0)
	    return 0;
    } else {
	if (!(s->options & SHELL_OPTION_DISPLAY_STDERR_CONTINUOUS)) {	/* no output desired, just run in background */
	    switch (fork ()) {
	    case 0:{
		    int nulldevice_wr, nulldevice_rd;
		    nulldevice_wr = open ("/dev/null", O_WRONLY);
		    nulldevice_rd = open ("/dev/null", O_RDONLY);
		    close (0);
		    if (dup (nulldevice_rd) == -1)
                        exit (1);
		    close (1);
		    if (dup (nulldevice_wr) == -1)
                        exit (1);
		    close (2);
                    if (dup (nulldevice_wr) == -1)
                        exit (1);
		    set_signal_handlers_to_default ();
		    execlp (argv[0], argv[0], NULL);
		    exit (0);	/* should never reach */
		}
	    case -1:
		return 0;
	    default:
		return 1;
	    }
	}
	if ((p = triple_pipe_open (0, 0, &running_shell[i].shell_pipe, 0, hme_i (SCRIPT_FILE, i), argv)) < 0)
	    return 0;
    }
    running_shell[i].shell_pid = p;
    running_shell[i].shell_pool = pool_init ();
    pool_null (running_shell[i].shell_pool);
    shell_display_output (i, catstrs (s->name, _ (" Output "), NULL), goto_file_callback);
    CAddWatch (running_shell[i].shell_pipe, shell_pool_update, WATCH_READING, (void *) (unsigned long) i);
    return 1;
}

char *read_pipe (int fd, int *len, const pid_t *child_pid);
int read_two_pipes (int fd1, int fd2, char **r1, int *len1, char **r2, int *len2, pid_t *child_pid);

/*
   Returns shell_output on success, 0 on error. Result must be free'd.
   Unlike the above routine, this blocks waiting for the shell to exit.
 */
static char *execute_foreground_shell (struct shell_cmd *s, char *script, char **err)
{E_
    pid_t p = 0;
    char *argv[] =
    {0, 0};
    int shell_foreground_pipe = -1;
    int shell_stderr_pipe = -1;
    char *t;

    *err = 0;

    argv[0] = hme (SCRIPT_FILE);
    savefile (argv[0], script, strlen (script), 0700);
    if (s->options & (SHELL_OPTION_GOTO_FILE_LINE_COLUMN | SHELL_OPTION_COMPLETE_WORD)) {
	if ((p = triple_pipe_open (0, &shell_foreground_pipe, &shell_stderr_pipe, 0, hme (SCRIPT_FILE), argv)) < 0)
	    return 0;
        read_two_pipes (shell_foreground_pipe, shell_stderr_pipe, &t, 0, err, 0, &p);
        if (*err && !**err) {   /* empty string */
            free (*err);
            *err = 0;
        }
    } else if (s->options & (SHELL_OPTION_INSERT_STDOUT | SHELL_OPTION_ANNOTATED_BOOKMARKS)) {
	if ((p = triple_pipe_open (0, &shell_foreground_pipe, 0, (s->options & (SHELL_OPTION_INSERT_STDERR | SHELL_OPTION_ANNOTATED_BOOKMARKS)) ? 1 : 0, hme (SCRIPT_FILE), argv)) < 0)
	    return 0;
	t = read_pipe (shell_foreground_pipe, 0, &p);
    } else {
	if ((p = triple_pipe_open (0, 0, &shell_foreground_pipe, 0, hme (SCRIPT_FILE), argv)) < 0)
	    return 0;
	t = read_pipe (shell_foreground_pipe, 0, &p);
	if (!(s->options & SHELL_OPTION_INSERT_STDERR)) {
	    if (t)
		free (t);
	    t = (char *) strdup ("");
	}
    }
    if (shell_foreground_pipe >= 0)
	close (shell_foreground_pipe);
    if (shell_stderr_pipe >= 0)
	close (shell_stderr_pipe);
    kill_process (p);
    return t;
}

#define BOOK_MARK_NOTE_COLOR            26
#define BOOK_MARK_WARNING_COLOR         (2*9 + 2*3 + 0*1)
#define BOOK_MARK_ERROR_COLOR           (2*9 + 1*3 + 1*1)


#define UNCOMMON_FNAME_CHAR(c)          ((unsigned char) (c) <= ' ' || strchr("\"\\/:*[]()|<>", (c)))

static const char *contains_whole_word (const char *s, const char *w)
{E_
    char c;
    const char *p = s;
    int l;
    l = strlen (w);
    c = *w;
    while (*p) {
	if (*p == c)
	    if (!strncmp (p, w, l))
		if (s == p || UNCOMMON_FNAME_CHAR (*(p - 1)))
		    if (UNCOMMON_FNAME_CHAR (*(p + l)))
			return p;
	p++;
    }
    return 0;
}

static void shell_out_parse_for_bookmark (WEdit * edit, struct shell_cmd *s, const char *q, int height)
{E_
    int len, line = 0, column = 0;

    len = strlen (q);

#define SKIP_NUMBER \
    while (*q >= '0' && *q <= '9') \
        q++;

/* Example:
../cooledit.c:1210:5: warning: implicit declaration of function 'edit_free_cache_lines' [-Wimplicit-function-declaration]
*/

/* lines must start with the filename */
    if (len < strlen (edit->filename) + 8)
        return;
    if (!(q = contains_whole_word (q, edit->filename)))
        return;
    q += strlen (edit->filename);

    if (*q != ':')
        return;
    q++;
    line = atoi (q);

    SKIP_NUMBER;

    if (*q == ':') {
        q++;
        column = atoi(q);
        SKIP_NUMBER;
    }

    while (*q && (*q <= ' ' || *q == ':'))
        q++;

    if (!Cstrncasecmp (q, "warning", 7))
        book_mark_insert (edit, line - 1, BOOK_MARK_WARNING_COLOR, height, q, column);
    else if (!Cstrncasecmp (q, "note:", 5))
        book_mark_insert (edit, line - 1, BOOK_MARK_NOTE_COLOR, height, q, column);
    else
        book_mark_insert (edit, line - 1, BOOK_MARK_ERROR_COLOR, height, q, column);
}

static void shell_create_bookmarks (WEdit * edit, struct shell_cmd *s, const char *p)
{E_
    int h;
    book_mark_flush (edit, BOOK_MARK_NOTE_COLOR);
    book_mark_flush (edit, BOOK_MARK_WARNING_COLOR);
    book_mark_flush (edit, BOOK_MARK_ERROR_COLOR);
    CPushFont ("bookmark");
    h = FONT_PIX_PER_LINE + 6;
    for (;;) {
        const char *eol;
        char *q;
        int l;
        eol = strchr(p, '\n');
        if (!eol)
            eol = p + strlen(p);
        l = (eol - p);
        q = (char *) malloc (l + 1);
        memcpy (q, p, l);
        q[l] = '\0';
        shell_out_parse_for_bookmark (edit, s, q, h);
        free (q);
        if (!*eol)
            break;
        p = eol + 1;
    }
    CPopFont ();
}


static char *get_complete_word (WEdit *e, long *curs_return)
{E_
    int curs;
    static char s[256];
    char *p;
    curs = e->curs1;
    p = &s[sizeof (s) - 1];
    *p = '\0';
    for (;;) {
	int ch;
	ch = edit_get_byte (e, curs - 1);
	if (!C_ALNUM (ch))
	    break;
	p--;
	curs--;
	*p = ch;
	if (p == &s[0]) {
	    *p = '\0';
	    break;
	}
    }
    if (curs_return)
        *curs_return = curs;
    return p;
}

int edit_save_block (WEdit * edit, const char *filename, long start, long finish);

/* This is called from the envokation dialog below */
static int run_shell (WEdit * e, struct shell_cmd *s, char *cmdline_options, char *name)
{E_
    char *complete_word = "";
    long complete_column = 0;
    struct stat st;
    long start_mark, end_mark;
    char *script = 0;
    char *output = 0;
    int i;
    int r = 0;

    i = find_shell (0, name, 0);
    if (i >= 0)
    if (!(s->options & (SHELL_OPTION_DISPLAY_STDOUT_CONTINUOUS |
			SHELL_OPTION_DISPLAY_STDERR_CONTINUOUS)))	/* --> these kill any running script automatically */
	if (running_shell[i].shell_pid) {
	    shell_error_dialog (_ (" Shell script "), _ (" A script is already running "));
	    return -1;
	}
    if (s->options & SHELL_OPTION_SAVE_BLOCK) {
	eval_marks (e, &start_mark, &end_mark);
	edit_save_block (e, hme (BLOCK_FILE), start_mark, end_mark);
    }
    if (s->options & SHELL_OPTION_SAVE_EDITOR_FILE)
        if (!edit_save_query_cmd (e))
	    return -1;

    if ((s->options & SHELL_OPTION_COMPLETE_WORD)) {
        long curs;
        complete_word = get_complete_word (e, &curs);
        complete_column = e->curs_charcolumn - (e->curs1 - curs);
    } else {
        complete_column = e->curs_charcolumn;
    }

    script = substitute_strings (s->script, cmdline_options, catstrs (e->dir, e->filename, NULL), e->curs_line + 1, complete_column + 1);

    if ((s->options & (SHELL_OPTION_DISPLAY_STDOUT_CONTINUOUS |
		       SHELL_OPTION_DISPLAY_STDERR_CONTINUOUS |
		       SHELL_OPTION_RUN_IN_BACKGROUND))) {
	if (!execute_background_shell (s, script, name)) {
	    shell_error_dialog (_ (" Shell script "), get_sys_error (_ (" Error trying to pipe script. ")));
	    r = -1;
            goto out;
	}
        r = 0;
        goto out;
    } else {
        char *err_out = 0;
	CHourGlass (main_window);
	output = execute_foreground_shell (s, script, &err_out);

	CUnHourGlass (main_window);
	if (err_out) {
	    shell_error_dialog (_ (" Shell stderr output "), err_out);
	    free (err_out);
	    r = -1;
            goto out;
	} else if (!output) {
	    shell_error_dialog (_ (" Shell script "), get_sys_error (_ (" Error trying to pipe script. ")));
	    r = -1;
            goto out;
	}
    }

    if ((s->options & SHELL_OPTION_ANNOTATED_BOOKMARKS)) {
        shell_create_bookmarks (e, s, output);
        r = 0;
        goto out;
    }

    if ((s->options & SHELL_OPTION_COMPLETE_WORD)) {
        complete_dialog (output, complete_word);
        output = 0;     /* dialog holds and frees */
        r = 0;
        goto out;
    }

    if ((s->options & SHELL_OPTION_GOTO_FILE_LINE_COLUMN)) {
        goto_error (output, 1);
        r = 0;
        goto out;
    }

    if (s->options & SHELL_OPTION_CHECK_ERROR_FILE) {
	if (stat (hme (ERROR_FILE), &st) == 0) {
	    if (st.st_size) {
		char *error;
		if (s->options & SHELL_OPTION_DISPLAY_ERROR_FILE) {
		    error = loadfile (hme (ERROR_FILE), 0);
		    if (error) {
			CTextboxMessageDialog (main_window, 20, 20, 80, 20, catstrs (s->name, _ (" Error "), NULL), error, 0);
			free (error);
		    }
		}
	        r = -1;
                goto out;
	    }
	}
    }
    if (s->options & SHELL_OPTION_DELETE_BLOCK)
	if (edit_block_delete_cmd (e)) {
	    r = 1;
            goto out;
        }

    if (output)
	if (*output)
	    for (i = strlen (output) - 1; i >= 0; i--)
		edit_insert_ahead (e, output[i]);

    if (s->options & SHELL_OPTION_INSERT_TEMP_FILE)
	if (!edit_insert_file (e, hme (TEMP_FILE)))
	    shell_error_dialog (_ (" Shell script "), get_sys_error (_ (" Error trying to insert temp file. ")));

    if (s->options & SHELL_OPTION_INSERT_BLOCK_FILE)
	if (!edit_insert_file (e, hme (BLOCK_FILE)))
	    shell_error_dialog (_ (" Shell script "), get_sys_error (_ (" Error trying to insert block file. ")));

    if (s->options & SHELL_OPTION_INSERT_CLIP_FILE)
	if (!edit_insert_file (e, hme (CLIP_FILE)))
	    shell_error_dialog (_ (" Shell script "), get_sys_error (_ (" Error trying to insert clip file. ")));

    if (s->options & SHELL_OPTION_DISPLAY_ERROR_FILE)
	if (stat (hme (ERROR_FILE), &st) == 0)
	    if (st.st_size) {	/* no error messages */
		char *error;
		error = loadfile (hme (ERROR_FILE), 0);
		if (error) {
		    CTextboxMessageDialog (main_window, 20, 20, 80, 25, s->name, error, 0);
		    free (error);
		}
	    }

  out:
    if (output)
        free (output);
    if (script)
        free (script);
    return r;
}


/*
   Main entry point. Request args if option is set and calls run_shell.
   Returns 0 on success, -1 on error and 1 on cancel.
 */
static int run_shell_dialog (WEdit * e, struct shell_cmd *s)
{E_
    char *cmdline_options;
    int r;
    long start_mark, end_mark;
    if (!s)
	return -1;
    if (s->options & SHELL_OPTION_SAVE_BLOCK)
	if (eval_marks (e, &start_mark, &end_mark)) {
	    shell_error_dialog (_ (" Shell Script "), _ (" Script requires some text to be highlighted. "));
	    return 1;
	}
    if (s->options & SHELL_OPTION_REQUEST_ARGUMENTS) {
	char *p, *q;
	p = (char *) strdup (s->name);	/* create a name for the input dialog by stripping spaces */
	for (q = p; *q; q++)
	    if (*q == ' ')
		Cmemmove (q, q + 1, strlen (q));
	if (strlen (p) > 20)
	    p[20] = 0;
	cmdline_options = CInputDialog (p, 0, 0, 0, 40 * FONT_MEAN_WIDTH, s->last_options ? s->last_options : "", s->name, s->prompt);
	free (p);
	if (!cmdline_options)
	    return 1;
	if (s->last_options)
	    free (s->last_options);
	s->last_options = cmdline_options;
    } else {
	cmdline_options = "";
    }
    r = run_shell (e, s, cmdline_options, s->menu);
    CRefreshEditor (e);
    return r;
}

static struct shell_cmd *scripts[MAX_NUM_SCRIPTS] =
{0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0};

/* loads from options file */
void load_scripts ()
{E_
    char *s = 0, *p, *q;
    unsigned char *r;
    int i = 0, n, upgrade = 0;
    int old_version;

    if (shell_alt_mask == SHELL_ALT_MASK_UNSET)
        abort ();

    memset (&running_shell, 0, sizeof (running_shell));

    s = get_options_section (editor_options_file, "[Shell Scripts]");
    if (!s || !*s)
	goto load_default_scripts;

#define BUILTING_SCRIPTS_VERSION        31

    if (!strncmp (s, "BUILTIN SHELL SCRIPTS VERSION ", 30)) {
        old_version = atoi (s + 30);
        if (old_version < BUILTING_SCRIPTS_VERSION)
            upgrade = 1;
        p = (s + strcspn(s, "\n"));
        if (*p == '\n')
            p++;
        q = p;
    } else {
        p = q = s;
        old_version = 1;
        upgrade = 1;
    }

    if (upgrade) {
	char fname[MAX_PATH_LEN];
	FILE *f;
	sprintf (fname, "%s/.cedit/.cooledit.ini.SCRIPTS%03d", local_home_dir, old_version);
	CMessageDialog (CRoot, 0, 0, 0, "Upgrade Info",
            "The [Shell Scripts] portion of your configuration file ~/.cedit/.cooledit.ini is out of date.\n"
            "\n"
            "When you close this message the entire contents of this section will be backed-up to\n"
            "%s for your reference.\n"
            "\n"
            "A new updated configuration file will be written. This will disable this message in the future.\n",
            fname);
        CDisableAlarm ();
	f = fopen (fname, "w+");
        if (!f)
            perror (fname);
        if (f) {        /* ignore error */
            char *v;
            fprintf (f, "[Shell Scripts]\n");
            for (v = s; *v; v++)
                if (fputc (*v, f) < 0)
                    break;
            fclose (f);
        }
        CEnableAlarm ();

	goto load_default_scripts;
    }

    for (i = 0; i < MAX_NUM_SCRIPTS; i++) {
	if (!*q || *q == '\n')
	    break;
	q = strchr (p, '\n');
	if (!q)
	    break;
	*q++ = 0;
	scripts[i] = CMalloc (sizeof (struct shell_cmd));
	memset (scripts[i], 0, sizeof (struct shell_cmd));
	strncpy (scripts[i]->name, p, 39);
	p = q;

	q = strchr (p, '\n');
	*q++ = 0;
	strncpy (scripts[i]->menu, p, 39);
	p = q;

	q = strchr (p, '\n');
	*q++ = 0;
	scripts[i]->menu_hot_key = *p;
	p = q;

	q = strchr (p, '\n');
	*q++ = 0;
	scripts[i]->key = (KeySym) atoi (p);
	p = q;

	q = strchr (p, '\n');
	*q++ = 0;
        if (*p >= '0' && *p <= '9')
	    scripts[i]->keyboard_state = atoi (p);
        else
	    scripts[i]->keyboard_state = shell_string_to_modifier (p);
	p = q;

	q = strchr (p, '\n');
	*q++ = 0;
	strncpy (scripts[i]->prompt, p, 159);
	p = q;

	q = strchr (p, '\n');
	*q++ = 0;
        if (*p >= '0' && *p <= '9')
            scripts[i]->options = atoi (p);
        else
            scripts[i]->options = shell_string_to_option (p);
	p = q;

	q = strchr (p, '\n');
	*q++ = 0;
	scripts[i]->last_options = (char *) strdup (p);
	p = q;

	q = strchr (p, '\n');
	*q++ = 0;
	scripts[i]->script = (char *) strdup (p);
	for (r = (unsigned char *) scripts[i]->script; *r; r++)
	    if (*r == 129)
		*r = '\n';
	p = q;
    }

  load_default_scripts:

    n = sizeof (default_scripts) / sizeof (struct shell_cmd);

    if (i < n) {
	for (; i < n; i++) {
	    scripts[i] = CMalloc (sizeof (struct shell_cmd));
	    memset (scripts[i], 0, sizeof (struct shell_cmd));
	    memcpy (scripts[i], &default_scripts[i], sizeof (struct shell_cmd));
            if (scripts[i]->alt_modifier)
                scripts[i]->keyboard_state |= shell_alt_mask;
	    scripts[i]->script = (char *) strdup (default_scripts[i].script);
	}
    }
    if (s)
	free (s);

    if (upgrade)
        save_scripts ();
}

/* saves to options file */
static void save_scripts (void)
{E_
    char *s, *p;
    int i = 0, n;

    p = s = CMalloc (65536 * 2);	/* make longer if overwrites */

    sprintf (p, "BUILTIN SHELL SCRIPTS VERSION %d\n", BUILTING_SCRIPTS_VERSION);
    p += strlen (p);

    while (scripts[i]) {
	unsigned char *t, *r;
	t = (unsigned char *) strdup (scripts[i]->script);
	for (r = t; *r; r++)
	    if (*r == '\n')
		*r = 129;	/* replace newlines with 129 */
	sprintf (p, "%s\n%s\n%c\n%d\n%s\n%s\n%s\n%s\n%s\n%n",
		 scripts[i]->name,
		 scripts[i]->menu,
		 scripts[i]->menu_hot_key ? scripts[i]->menu_hot_key : ' ',
		 (int) scripts[i]->key,
		 shell_modifier_to_string (scripts[i]->keyboard_state),
		 scripts[i]->prompt,
		 shell_option_to_string (scripts[i]->options),
		 scripts[i]->last_options ? scripts[i]->last_options : "",
		 t, &n);
	free (t);
	p += n;
	i++;
    }
    *p++ = '\n';
    *p = 0;
    save_options_section (editor_options_file, "[Shell Scripts]", s);
    free (s);
}


#define N_ITEMS 4

extern CWidget *edit[];
extern int current_edit;

/* straight from the menu */
static void script_menu_callback (unsigned long ignored)
{E_
    int i;
    i = (CIdent ("menu.scripts"))->current - N_ITEMS;
    run_shell_dialog (edit[current_edit]->editor, scripts[i]);
}

/* this is called from edit_translate_key.c */
int get_script_number_from_key (unsigned int state, KeySym keysym)
{E_
    int i;
    for (i = 0; i < MAX_NUM_SCRIPTS; i++) {
	if (!scripts[i])
	    break;
	if (keysym < 256)
	    keysym = my_lower_case (keysym);
	if (scripts[i]->keyboard_state == state && scripts[i]->key == keysym)
	    return i;
    }
    return -1;
}

/* This is called from the editor: see main.c: edit_set_user_command (execute_script); */
void execute_script (WEdit * e, int i)
{E_
    run_shell_dialog (e, scripts[i]);
}

/* Updates updates the menu when a new shell has been added or removed */
void update_script_menu_items ()
{E_
    int i, n;
    n = (CIdent ("menu.scripts"))->numlines + 2;
    for (i = N_ITEMS; i < n; i++)
	CRemoveMenuItemNumber ("menu.scripts", N_ITEMS);

    for (n = 0; n < MAX_NUM_SCRIPTS; n++) {
	if (!scripts[n])
	    break;
	CAddMenuItem ("menu.scripts", scripts[n]->menu, scripts[n]->menu_hot_key, script_menu_callback, 0);
    }
}

#define NUM_OPTS                (sizeof (options) / sizeof(struct script_options))

struct script_options {
    const char *name;
    const char *toolhint;
    int flag;
    const char *tag;
};

static const struct script_options options[] =
{
/* 1 */
    {
        gettext_noop ("Save block on commence"), gettext_noop ("Save the current highlighted text to %b before executing"), SHELL_OPTION_SAVE_BLOCK, "SAVE_BLOCK"
    },
/* 2 */
    {
        gettext_noop ("Save editor file on commence"), gettext_noop ("Save the entire edit buffer to %f before executing"), SHELL_OPTION_SAVE_EDITOR_FILE, "SAVE_EDITOR_FILE"
    },
/* 3 */
    {
        gettext_noop ("Prompt for arguments on commence"), gettext_noop ("Prompt the user for a string to be replaced with %a"), SHELL_OPTION_REQUEST_ARGUMENTS, "REQUEST_ARGUMENTS"
    },
/* 4 */
    {
        gettext_noop ("Display script's stdout continuously"), gettext_noop ("Dynamically view the scripts output"), SHELL_OPTION_DISPLAY_STDOUT_CONTINUOUS, "DISPLAY_STDOUT_CONTINUOUS"
    },
/* 5 */
    {
        gettext_noop ("Display script's stderr continuously"), gettext_noop ("Dynamically view the scripts output"), SHELL_OPTION_DISPLAY_STDERR_CONTINUOUS, "DISPLAY_STDERR_CONTINUOUS"
    },
/* 6 */
    {
        gettext_noop ("Run in background"), "", SHELL_OPTION_RUN_IN_BACKGROUND, "RUN_IN_BACKGROUND"
    },
/* 7 */
    {
        gettext_noop ("Delete block on commence"), gettext_noop ("Use when you want to replace highlighted text"), SHELL_OPTION_DELETE_BLOCK, "DELETE_BLOCK"
    },
/* 8 */
    {
        gettext_noop ("Insert temp file on completion"), gettext_noop ("Types out file %t's contents on completion of the script"), SHELL_OPTION_INSERT_TEMP_FILE, "INSERT_TEMP_FILE"
    },
/* 9 */
    {
        gettext_noop ("Insert block file on completion"), gettext_noop ("Types out file %b's contents on completion of the script"), SHELL_OPTION_INSERT_BLOCK_FILE, "INSERT_BLOCK_FILE"
    },
/* 10 */
    {
        gettext_noop ("Insert clip file on completion"), gettext_noop ("Types out file %c's contents on completion of the script"), SHELL_OPTION_INSERT_CLIP_FILE, "INSERT_CLIP_FILE"
    },
/* 11 */
    {
        gettext_noop ("Insert stdout on completion"), gettext_noop ("Types out the script's output"), SHELL_OPTION_INSERT_STDOUT, "INSERT_STDOUT"
    },
/* 12 */
    {
        gettext_noop ("Insert stderr on completion"), gettext_noop ("Types out the script's output"), SHELL_OPTION_INSERT_STDERR, "INSERT_STDERR"
    },
/* 13 */
    {
        gettext_noop ("Display error file"), gettext_noop ("Displays %e on completion of the script"), SHELL_OPTION_DISPLAY_ERROR_FILE, "DISPLAY_ERROR_FILE"
    },
/* 14 */
    {
        gettext_noop ("Subs only if error file is empty"), gettext_noop ("Only replace the highlighted text if %e is empty"), SHELL_OPTION_CHECK_ERROR_FILE, "CHECK_ERROR_FILE"
    },
/* 15 */
    {
        gettext_noop ("Output becomes annotated bookmarks"), gettext_noop ("The output of the script be processed and text of the form <file>:<line> will be identified and bookmarks inserted into your code"), SHELL_OPTION_ANNOTATED_BOOKMARKS, "ANNOTATED_BOOKMARKS"
    },
/* 16 */
    {
        gettext_noop ("Parse output as goto file:line:col"), gettext_noop ("Output is as <fullpath>/<file>:<line>:<column> and file is loaded and cursor sent to position"), SHELL_OPTION_GOTO_FILE_LINE_COLUMN, "GOTO_FILE_LINE_COLUMN"
    },
/* 17 */
#warning use %<something> as current editor line and cursor
    {
        gettext_noop ("Word completion at cursor"), gettext_noop ("Output is used as a list of possible options to be displayed in a selection box"), SHELL_OPTION_COMPLETE_WORD, "COMPLETE_WORD"
    }
};

static char *shell_option_to_string (int option_flags)
{E_
    static char s[1024];
    int i, l;
    s[0] = '\0';
    for (i = 0; i < sizeof (options) / sizeof (options[0]); i++)
	if ((options[i].flag & option_flags)) {
	    strcat (s, options[i].tag);
	    strcat (s, ",");
	}
    l = strlen (s);
    if (l > 0 && s[l - 1] == ',')
	s[l - 1] = '\0';
    return s;
}

static int shell_string_to_option (const char *s)
{E_
    int i, r = 0;
    for (i = 0; i < sizeof (options) / sizeof (options[0]); i++)
	if (strstr (s, options[i].tag))
	    r |= options[i].flag;
    return r;
}

/* Edits a scripts: returns 1 on cancel, 0 on success, -1 on error */
int edit_scripts_dialog (Window parent, int x, int y, struct shell_cmd *s)
{E_
    int i, r = 0;
    CState state;
    int xs, ys, x2, y2, yu;
    int max_sl = 0;
    char hot[2] = "\0\0";
    Window win;
    CWidget *w;
    CWidget *wdt[sizeof(options) / sizeof(options[0])];
    CEvent cw;

    hot[0] = s->menu_hot_key;

    CBackupState (&state);
    CDisable ("*");

#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))

    CPushFont("widget");

    win = CDrawHeadedDialog ("shellopt", parent, x, y, " Script Control ");
    CGetHintPos (&x, &y);
    yu = y;
    CDrawText ("shellopt.namet", win, x, y, " Name : ");
    CGetHintPos (&x2, &y2);
    CDrawTextInputP ("shellopt.name", win, x2, y, FONT_MEAN_WIDTH * 27, AUTO_HEIGHT, 39, s->name);


/* Toolhint */
    CSetToolHint ("shellopt.namet", _ ("Name to appear in the listbox"));
    CSetToolHint ("shellopt.name", _ ("Name to appear in the listbox"));

    CGetHintPos (0, &y);
    y = MAX(y, y2);

    CDrawText ("shellopt.menut", win, x, y, " Menu item : ");
    CGetHintPos (&x2, &y2);
    CDrawTextInputP ("shellopt.menu", win, x2, y, FONT_MEAN_WIDTH * 27, AUTO_HEIGHT, 39, s->menu);
/* Toolhint */
    CSetToolHint ("shellopt.menut", _ ("Menu item entry with hot-key"));
    CSetToolHint ("shellopt.menu", _ ("Menu item entry with hot-key"));
    CGetHintPos (0, &y);
    y = MAX(y, y2);

    CDrawText ("shellopt.hott", win, x, y, " Menu hotkey : ");
    CGetHintPos (&x2, &y2);
    CDrawTextInputP ("shellopt.hot", win, x2, y, FONT_MEAN_WIDTH * 10, AUTO_HEIGHT, 3, hot);
/* Toolhint */
    CSetToolHint ("shellopt.hott", _ ("Letter to underline"));
    CSetToolHint ("shellopt.hot", _ ("Letter to underline"));
    CGetHintPos (0, &y);
    y = MAX(y, y2);

    CDrawText ("shellopt.promptt", win, x, y, " Argument prompt : ");
    CGetHintPos (&x2, &y2);
    CDrawTextInputP ("shellopt.prompt", win, x2, y, FONT_MEAN_WIDTH * 24, AUTO_HEIGHT, 159, s->prompt);
/* Toolhint */
    CSetToolHint ("shellopt.promptt", _ ("Message for %a prompting"));
    CSetToolHint ("shellopt.prompt", _ ("Message for %a prompting"));
    CGetHintPos (0, &y);
    y = MAX(y, y2);

    xs = x;
    ys = y;

    for (i = 0; i < (NUM_OPTS + 1) / 2; i++) {
        int sl;
        sl = CImageStringWidth (_ (options[i].name));
        if (max_sl < sl)
            max_sl = sl;
	wdt[i] = CDrawSwitch (catstrs ("shellopt.", itoa (i), NULL), win, x, y, (s->options & options[i].flag) ? 1 : 0, _ (options[i].name), 0);
	CSetToolHint (wdt[i]->ident, _ (options[i].toolhint));
	CSetToolHint (catstrs (wdt[i]->ident, ".label", NULL), _ (options[i].toolhint));
	CGetHintPos (0, &y);
        y2 = y;
    }
    x = xs + max_sl + 32 + WIDGET_SPACING * 2 + 10;
    y = ys;
    for (i = (NUM_OPTS + 1) / 2; i < NUM_OPTS; i++) {
	wdt[i] = CDrawSwitch (catstrs ("shellopt.", itoa (i), NULL), win, x, y, (s->options & options[i].flag) ? 1 : 0, _ (options[i].name), 0);
	CSetToolHint (wdt[i]->ident, _ (options[i].toolhint));
	CSetToolHint (catstrs (wdt[i]->ident, ".label", NULL), _ (options[i].toolhint));
	CGetHintPos (0, &y);
    }
    CPopFont();
    y = MAX(y, y2);

    CPushFont ("editor", 0);
    w = CDrawEditor ("shellopt.edit", win, xs, y,
		     77 * FONT_MEAN_WIDTH, 10 * FONT_PIX_PER_LINE, s->script, 0, 0, 0, EDITOR_NO_TEXT | EDITOR_NO_FILE, strlen (s->script));
    CPopFont ();
/* Toolhint */
    CSetToolHint ("shellopt.edit", _ ("Enter your shell script here"));

    get_hint_limits (&x, &y);
    x -= WIDGET_SPACING * 2 + TICK_BUTTON_WIDTH * 2;
    CDrawPixmapButton ("shellopt.ok", win, x, yu, PIXMAP_BUTTON_TICK);
/* Toolhint */
    CSetToolHint ("shellopt.ok", _ ("Press to accept. You will then be\nprompted for the hot-key combination"));
    CGetHintPos (&x, 0);
    CDrawPixmapButton ("shellopt.cancel", win, x, yu, PIXMAP_BUTTON_CROSS);
/* Toolhint */
    CSetToolHint ("shellopt.cancel", _ ("Abort operation"));
    CSetSizeHintPos ("shellopt");
    CFocus (CIdent ("shellopt.name"));
    CMapDialog ("shellopt");

    while (1) {
	CNextEvent (0, &cw);
	if (!CIdent ("shellopt"))	/* destroyed by WM close */
	    break;
	if (!strcmp (cw.ident, "shellopt.cancel")) {
	    r = 1;
	    break;
	}

/* annoted bookmarks makes no sense with the other options except perhaps "request-arguments" */
	for (i = 0; i < NUM_OPTS; i++) {
	    if (!strcmp (cw.ident, wdt[i]->ident)) {
/* when "annotated" is depressed turn off everything except "request-arguments" and "save" */
	        if (wdt[i]->keypressed && options[i].flag == SHELL_OPTION_ANNOTATED_BOOKMARKS) {
                    int j;
                    for (j = 0; j < NUM_OPTS; j++) {
                        switch (options[j].flag) {
                        case SHELL_OPTION_SAVE_EDITOR_FILE:
                        case SHELL_OPTION_ANNOTATED_BOOKMARKS:
                        case SHELL_OPTION_REQUEST_ARGUMENTS:
                            break;
                        default:
                            if (wdt[j]->keypressed) {
                                wdt[j]->keypressed = 0;
                                CExpose (wdt[j]->ident);
                            }
                        }
                    }
                }
/* when anything except "request-arguments" is depressed, the turn off "annotated" */
	        if (wdt[i]->keypressed && options[i].flag != SHELL_OPTION_ANNOTATED_BOOKMARKS && options[i].flag != SHELL_OPTION_REQUEST_ARGUMENTS && options[i].flag != SHELL_OPTION_SAVE_EDITOR_FILE) {
                    int j;
                    for (j = 0; j < NUM_OPTS; j++) {
                        if (options[j].flag == SHELL_OPTION_ANNOTATED_BOOKMARKS) {
                            if (wdt[j]->keypressed) {
                                wdt[j]->keypressed = 0;
                                CExpose (wdt[j]->ident);
                            }
                        }
                    }
                }
            }
        }

	if (!strcmp (cw.ident, "shellopt.ok")) {
	    XEvent *p;
	    p = CRawkeyQuery ((CIdent ("shellopt.ok"))->mainid, 20, 20, _ (" Script Edit "), _ (" Press the key combination to envoke the script : "));
	    if (!p)
		continue;
            CDisableXIM();      /* to get raw key */
	    s->key = CKeySym (p);
            CEnableXIM();
	    s->keyboard_state = p->xkey.state;
	    s->options = 0;
	    for (i = 0; i < NUM_OPTS; i++)
		if (wdt[i]->keypressed)
		    s->options |= options[i].flag;
	    strncpy (s->name, (CIdent ("shellopt.name"))->text.data, 39);
	    strncpy (s->menu, (CIdent ("shellopt.menu"))->text.data, 39);
	    strncpy (s->prompt, (CIdent ("shellopt.prompt"))->text.data, 159);
	    s->menu_hot_key = (CIdent ("shellopt.hot"))->text.data[0];
	    if (s->script)
		free (s->script);
	    s->script = edit_get_buffer_as_text (w->editor);
	    break;
	}
    }
    CDestroyWidget ("shellopt");
    CRestoreState (&state);
    return r;
}

int edit_script (Window parent, int x, int y, int which_script)
{E_
    return edit_scripts_dialog (parent, x, y, scripts[which_script]);
}

static char *get_a_line (void *data, int line)
{E_
    static char t[128];
    struct shell_cmd **s;
    s = data;
    strcpy (t, s[line]->name);
    return t;
}

int script_list_box_dialog (Window parent, int x, int y, const char *heading)
{E_
    int n;

    for (n = 0; n < MAX_NUM_SCRIPTS; n++)
	if (!scripts[n])
	    break;

    return CListboxDialog (parent, x, y, 30, 10, heading, 0, 0, n, get_a_line, scripts);
}

/* straight from menu */
void edit_a_script_cmd (unsigned long ignored)
{E_
    int i;
    i = script_list_box_dialog (main_window, 40, 40, _ (" Pick a Script to Edit "));
    if (i >= 0) {
	if (!edit_script (CRoot, 20, 20, i))
	    save_scripts ();
	update_script_menu_items ();
    }
}

void delete_script (int i)
{E_
    if (scripts[i]) {
	if (scripts[i]->script)
	    free (scripts[i]->script);
	if (scripts[i]->last_options)
	    free (scripts[i]->last_options);
	free (scripts[i]);
	scripts[i] = 0;
	Cmemmove (scripts + i, scripts + i + 1, (MAX_NUM_SCRIPTS - i - 1) * sizeof (struct shell_cmd *));
    }
}

/* called on application shutdown */
void free_all_scripts (void)
{E_
    while (scripts[0])
	delete_script (0);
}

/* straight from menu */
void delete_a_script_cmd (unsigned long ignored)
{E_
    int i;
    i = script_list_box_dialog (main_window, 20, 20, _ (" Pick a Script to Delete "));
    if (i >= 0) {
	delete_script (i);
	save_scripts ();
	update_script_menu_items ();
    }
}

/* straight from menu */
void new_script_cmd (unsigned long ignored)
{E_
    int n;
    for (n = 0; n < MAX_NUM_SCRIPTS; n++)
	if (!scripts[n])
	    break;
    if (n > MAX_NUM_SCRIPTS - 2) {
	CErrorDialog (0, 0, 0, _ (" New Script "), \
		      _ (" Max number of scripts has been reached, \n" \
		      " increase MAX_NUM_SCRIPTS in the file shell.h "));
	return;
    }
    scripts[n] = CMalloc (sizeof (struct shell_cmd));
    memset (scripts[n], 0, sizeof (struct shell_cmd));
    scripts[n]->script = (char *) strdup ("#!/bin/sh\n");
    if (edit_script (main_window, 20, 20, n)) {
        free (scripts[n]->script);
	free (scripts[n]);
	scripts[n] = 0;
    } else
	save_scripts ();
    update_script_menu_items ();
}

static CWidget *CDrawMiniSwitch (const char *identifier, Window parent, int x, int y, int d, int on)
{E_
    CWidget *w;
    w = CSetupWidget (identifier, parent, x, y, d, d, C_SWITCH_WIDGET, INPUT_BUTTON, COLOR_FLAT, 1);
    w->fg = COLOR_BLACK;
    w->bg = COLOR_FLAT;
    w->keypressed = on;
    w->render = render_switch;
    w->options |= WIDGET_TAKES_FOCUS_RING;
    return w;
}

pid_t open_under_pty (int *in, int *out, char *line, const char *file, char *const argv[]);

int option_shell_command_line_sticky = 0;
int option_shell_command_line_pty = 0;
void shell_output_add_job (WEdit * edit, int in, int out, pid_t pid, char *name, int close_on_error);

static struct shell_job *get_job (WEdit * edit, int n)
{E_
    struct shell_job *j;
    int i;
    for (i = 0, j = edit->jobs; j && i < n; j = j->next, i++);
    return j;
}

static char *list_jobs_get_line (void *data, int line)
{E_
    struct shell_job *j;
    j = get_job ((WEdit *) data, line);
    if (j)
	return j->name;
    return "";
}

static int list_jobs (void)
{E_
    struct shell_job *j;
    int i, c, n;
    WEdit *e;
    e = edit[current_edit]->editor;
    c = max (20, e->num_widget_columns - 5);
    for (n = 0, j = e->jobs; j; j = j->next, n++);
    i = CListboxDialog (edit[current_edit]->mainid, 20, 20, c, 10, 0, 0, 0, n, list_jobs_get_line, (void *) e);
    if (i >= 0) {
	j = get_job (e, i);
	if (j)
	    shell_output_kill_job (e, j->pid, 1);
    }
    return 0;
}

void edit_insert_shell_output (WEdit * edit)
{E_
    char id[33], q[1024], *p;
    CWidget *w, *v, *i, *b, *h, *c;
    int tolong = 0, done = 0;
    CState s;
    while (!done) {
	strcpy (id, CIdentOf (edit->widget));
	strcat (id, ".text");
	w = CIdent (id);
	if (!w)
	    return;
	CBackupState (&s);
	CDisable ("*");
	p = getenv ("PWD");
	get_current_wd (q, 1023);
	q[1023] = '\0';
	p = q;
	CPushFont ("widget", 0);
	while (*p && CImageStringWidth (p) > CWidthOf (w) * 2 / 3 - 20) {
	    tolong = 1;
	    p++;
	}
	CPopFont ();
	h = CDrawButton ("status_button", edit->widget->parentid, CXof (w), CYof (w), CHeightOf (w), CHeightOf (w), 0);
	CSetToolHint (h->ident, _("Click for list of running jobs.\nHit Enter on a job to kill it."));
	b =
	    CDrawMiniSwitch ("status_switch", edit->widget->parentid,
			     CXof (w) + CWidthOf (h), CYof (w), CHeightOf (w), option_shell_command_line_sticky);
	CSetToolHint (b->ident, _("If not depressed then input line will close on Enter."));
	c =
	    CDrawMiniSwitch ("status_switch2", edit->widget->parentid,
			     CXof (w) + CWidthOf (h) + CWidthOf (b), CYof (w), CHeightOf (w),
			     option_shell_command_line_pty);
	CSetToolHint (c->ident, _("If depressed then opens under a tty."));
	v =
	    CDrawText ("status_prompt", edit->widget->parentid,
		       CXof (w) + CWidthOf (b) + CWidthOf (c) + CWidthOf (h), CYof (w), "[%s%s]#", tolong ? "..." : "",
		       p);
	CSetToolHint (v->ident,
		      _("Current directory. This is the global current directory as set from the Command menu."));
	i =
	    CDrawTextInputP ("status_input", edit->widget->parentid,
			    CXof (w) + CWidthOf (b) + CWidthOf (c) + CWidthOf (v) + CWidthOf (h), CYof (w),
			    CWidthOf (edit->widget) - CWidthOf (v) - CWidthOf (h) - CWidthOf (b) - CWidthOf (c),
			    AUTO_HEIGHT, 32768, TEXTINPUT_LAST_INPUT);
	CSetToolHint (i->ident,
		      _
		      ("Enter shell command. Input will be piped to this command from\nselected text. Output will be inserted at editor cursor."));
	CFocus (i);
	edit->force |= REDRAW_PAGE;
	edit_render_keypress (edit);
	edit_push_action (edit, KEY_PRESS, edit->start_display);
	for (;;) {
	    XEvent xev;
	    CEvent cev;
	    CNextEvent (&xev, &cev);
	    option_shell_command_line_pty = c->keypressed;
	    if (xev.type == KeyPress && cev.command == CK_Enter && cev.kind == C_TEXTINPUT_WIDGET) {
		char *t;
		for (t = i->text.data; *t && isspace (*t); t++);
		if (!strncmp (t, "cd ", 3)) {
                    char errmsg[REMOTEFS_ERR_MSG_LEN];
		    for (t = t + 3; *t && isspace (*t); t++);
		    if (change_directory (t, errmsg) < 0)
			CErrorDialog (main_window, 20, 20,
				      _(" Change directory "), " Error return from chdir. \n [%s]", errmsg);
		    done = !b->keypressed;
		} else {
		    int in, out;
		    pid_t pid;
		    char *arg[5];
		    char line[80];
		    arg[0] = "/bin/sh";
		    arg[1] = "-c";
		    arg[2] = i->text.data;
		    arg[3] = 0;
		    if (option_shell_command_line_pty) {
			pid = open_under_pty (&in, &out, line, "/bin/sh", arg);
		    } else {
			pid = triple_pipe_open (&in, &out, 0, 1, "/bin/sh", arg);
		    }
		    shell_output_add_job (edit, in, out, pid, i->text.data, !option_shell_command_line_pty);
		    done = !b->keypressed;
		}
		break;
	    }
	    if (xev.type == KeyPress && cev.command == CK_Cancel) {
		done = 1;
		break;
	    }
	    if (xev.type == ButtonPress && cev.window != v->winid && cev.window != i->winid && cev.window != b->winid
		&& cev.window != c->winid && cev.window != h->winid) {
		done = 1;
		break;
	    }
	    if (!strcmp (cev.ident, "status_button"))
		list_jobs ();
	}
	option_shell_command_line_sticky = b->keypressed;
	CDestroyWidget ("status_prompt");
	CDestroyWidget ("status_input");
	CDestroyWidget ("status_button");
	CDestroyWidget ("status_switch");
	CDestroyWidget ("status_switch2");
	CRestoreState (&s);
    }
}

