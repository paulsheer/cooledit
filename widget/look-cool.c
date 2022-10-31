/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* look-cool.c - look 'n feel type: COOL
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <assert.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef __FreeBSD__
#include <sys/sysmacros.h>
#endif

#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"
#include "coollocal.h"
#include "remotefs.h"


extern struct look *look;

/* {{{ search replace dialog */

extern int replace_scanf;
extern int replace_regexp;
extern int replace_all;
extern int replace_prompt;
extern int replace_whole;
extern int replace_case;
extern int replace_backwards;
extern int search_create_bookmark;

struct look_cool_list {
    struct file_entry *l;
    int n;
};

int look_cool_search_replace_dialog (Window parent, int x, int y, CStr *search_text, CStr *replace_text, CStr *arg_order, const char *heading, int option)
{E_
    int cancel = 0;
    Window win;
    XEvent xev;
    CEvent cev;
    CState s;
    int xh, yh, h, xb, ys, yc, yb, yr;
    CWidget *m;
    int text_input_width ;

    CBackupState (&s);
    CDisable ("*");

    win = CDrawHeadedDialog ("replace", parent, x, y, heading);
    CGetHintPos (&xh, &h);

/* NLS hotkey ? */
    CIdent ("replace")->position = WINDOW_ALWAYS_RAISED;
/* An input line comes after the ':' */
    (CDrawText ("replace.t1", win, xh, h, _(" Enter search text : ")))->hotkey = 'E';

    CGetHintPos (0, &yh);
    (m = CDrawTextInput ("replace.sinp", win, xh, yh, 10, AUTO_HEIGHT, 8192, search_text))->hotkey = 'E';

    if (replace_text) {
	CGetHintPos (0, &yh);
	(CDrawText ("replace.t2", win, xh, yh, _(" Enter replace text : ")))->hotkey = 'n';
	CGetHintPos (0, &yh);
	(CDrawTextInput ("replace.rinp", win, xh, yh, 10, AUTO_HEIGHT, 8192, replace_text))->hotkey = 'n';
	CSetToolHint ("replace.t2", _("You can enter regexp substrings with %s\n(not \\1, \\2 like sed) then use \"Enter...order\""));
	CSetToolHint ("replace.rinp", _("You can enter regexp substrings with %s\n(not \\1, \\2 like sed) then use \"Enter...order\""));
	CGetHintPos (0, &yh);
	(CDrawText ("replace.t3", win, xh, yh, _(" Enter argument (or substring) order : ")))->hotkey = 'o';
	CGetHintPos (0, &yh);
	(CDrawTextInput ("replace.ainp", win, xh, yh, 10, AUTO_HEIGHT, 256, arg_order))->hotkey = 'o';
/* Tool hint */
	CSetToolHint ("replace.ainp", _("Enter the order of replacement of your scanf\nformat specifiers or regexp substrings, eg 3,1,2"));
	CSetToolHint ("replace.t3", _("Enter the order of replacement of your scanf\nformat specifiers or regexp substrings, eg 3,1,2"));
    }
    CGetHintPos (0, &yh);
    ys = yh;
/* The following are check boxes */
    CDrawSwitch ("replace.ww", win, xh, yh, replace_whole, _(" Whole words only "), 0);
    CGetHintPos (0, &yh);
    CDrawSwitch ("replace.case", win, xh, yh, replace_case, _(" Case sensitive "), 0);
    yc = yh;
    CGetHintPos (0, &yh);
    CDrawSwitch ("replace.reg", win, xh, yh, replace_regexp, _(" Regular expression "), 1);
    CSetToolHint ("replace.reg", _("See the regex man page for how\nto compose a regular expression"));
    CSetToolHint ("replace.reg.label", _("See the regex man page for how\nto compose a regular expression"));
    yb = yh;
    CGetHintPos (0, &yh);
    CGetHintPos (&xb, 0);
    if (option & SEARCH_DIALOG_OPTION_BACKWARDS) {
	CDrawSwitch ("replace.bkwd", win, xh, yh, replace_backwards, _(" Backwards "), 0);
/* Tool hint */
	CSetToolHint ("replace.bkwd", _("Warning: Searching backward can be slow"));
	CSetToolHint ("replace.bkwd.label", _("Warning: Searching backward can be slow"));
    }
    if (replace_text) {
	yr = ys;
	if (option & SEARCH_DIALOG_OPTION_BACKWARDS)
	    yr = yc;
    } else {
	if (option & SEARCH_DIALOG_OPTION_BACKWARDS) {
	    if (option & SEARCH_DIALOG_OPTION_BOOKMARK)
		yr = yb;
	    else
		yr = yh;
	} else {
	    if (option & SEARCH_DIALOG_OPTION_BOOKMARK)
		yr = yc;
	    else
		yr = yb;
	}
    }

    if (replace_text) {
	CDrawSwitch ("replace.pr", win, xb, yr, replace_prompt, _(" Prompt on replace "), 0);
/* Tool hint */
	CSetToolHint ("replace.pr", _("Ask before making each replacement"));
	CGetHintPos (0, &yr);
	CDrawSwitch ("replace.all", win, xb, yr, replace_all, _(" Replace all "), 0);
/* Tool hint */
	CSetToolHint ("replace.all", _("Replace repeatedly"));
	CGetHintPos (0, &yr);
    }
    if (option & SEARCH_DIALOG_OPTION_BOOKMARK) {
	CDrawSwitch ("replace.bkmk", win, xb, yr, search_create_bookmark, _(" Bookmarks "), 0);
/* Tool hint */
	CSetToolHint ("replace.bkmk", _("Create bookmarks at all lines found"));
	CSetToolHint ("replace.bkmk.label", _("Create bookmarks at all lines found"));
	CGetHintPos (0, &yr);
    }
    CDrawSwitch ("replace.scanf", win, xb, yr, replace_scanf, _(" Scanf expression "), 1);
/* Tool hint */
    CSetToolHint ("replace.scanf", _("Allows entering of a C format string,\nsee the scanf man page"));

    get_hint_limits (&x, &y);
    CDrawPixmapButton ("replace.ok", win, x - WIDGET_SPACING - TICK_BUTTON_WIDTH, h, PIXMAP_BUTTON_TICK);
    CDrawPixmapButton ("replace.cancel", win, x - WIDGET_SPACING - TICK_BUTTON_WIDTH, h + WIDGET_SPACING + TICK_BUTTON_WIDTH, PIXMAP_BUTTON_CROSS);
/* Tool hint */
    CSetToolHint ("replace.ok", _("Begin search, Enter"));
    CSetToolHint ("replace.cancel", _("Abort this dialog, Esc"));
    CSetSizeHintPos ("replace");
    CMapDialog ("replace");

    m = CIdent ("replace");
    text_input_width = m->width - WIDGET_SPACING * 3 - 4 - TICK_BUTTON_WIDTH ;
    CSetWidgetSize ("replace.sinp", text_input_width, (CIdent ("replace.sinp"))->height);
    if (replace_text) {
	CSetWidgetSize ("replace.rinp", text_input_width, (CIdent ("replace.rinp"))->height);
	CSetWidgetSize ("replace.ainp", text_input_width, (CIdent ("replace.ainp"))->height);
    }
    CFocus (CIdent ("replace.sinp"));

    for (;;) {
	CNextEvent (&xev, &cev);
	if (!CIdent ("replace")) {
            cancel = 1;
	    break;
	}
	if (!strcmp (cev.ident, "replace.cancel") || cev.command == CK_Cancel) {
            cancel = 2;
	    break;
	}
	if (!strcmp (cev.ident, "replace.reg") || !strcmp (cev.ident, "replace.scanf")) {
	    if (CIdent ("replace.reg")->keypressed || CIdent ("replace.scanf")->keypressed) {
		if (!(CIdent ("replace.case")->keypressed)) {
		    CIdent ("replace.case")->keypressed = 1;
		    CExpose ("replace.case");
		}
	    }
	}
	if (!strcmp (cev.ident, "replace.ok") || cev.command == CK_Enter) {
	    if (replace_text) {
		replace_all = CIdent ("replace.all")->keypressed;
		replace_prompt = CIdent ("replace.pr")->keypressed;
                CStr_free(replace_text);
		*replace_text = CStr_dupstr (CIdent ("replace.rinp")->text);
                CStr_free(arg_order);
		*arg_order = CStr_dupstr (CIdent ("replace.ainp")->text);
	    }
            CStr_free(search_text);
	    *search_text = CStr_dupstr (CIdent ("replace.sinp")->text);
	    replace_whole = CIdent ("replace.ww")->keypressed;
	    replace_case = CIdent ("replace.case")->keypressed;
	    replace_scanf = CIdent ("replace.scanf")->keypressed;
	    replace_regexp = CIdent ("replace.reg")->keypressed;

	    if (option & SEARCH_DIALOG_OPTION_BACKWARDS) {
		replace_backwards = CIdent ("replace.bkwd")->keypressed;
	    } else {
		replace_backwards = 0;
	    }

	    if (option & SEARCH_DIALOG_OPTION_BOOKMARK) {
		search_create_bookmark = CIdent ("replace.bkmk")->keypressed;
	    } else {
		search_create_bookmark = 0;
	    }

	    break;
	}
    }
    CDestroyWidget ("replace");
    CRestoreState (&s);
    return cancel;
}

/* }}} search replace dialog */

/* {{{ file list stuff */

static void destroy_filelist (CWidget * w)
{E_
    if (w->hook) {
        struct look_cool_list *fe = (struct look_cool_list *) w->hook;
        if (fe->l)
            free (fe->l);
	free (w->hook);
	w->hook = 0;
    }
}

/* This doesn't apply to solaris: */
/*
 * struct dirent {
 * 	long		d_ino;
 * 	__kernel_off_t	d_off;
 * 	unsigned short	d_reclen;
 * 	char		d_name[256];
 * };
 */

/*
 * 
 * #define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
 * #define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
 * #define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
 * #define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
 * #define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
 * #define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
 * #define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)
 * 
 * #define S_IRWXU 00700
 * #define S_IRUSR 00400
 * #define S_IWUSR 00200
 * #define S_IXUSR 00100
 * 
 * #define S_IRWXG 00070
 * #define S_IRGRP 00040
 * #define S_IWGRP 00020
 * #define S_IXGRP 00010
 * 
 * #define S_IRWXO 00007
 * #define S_IROTH 00004
 * #define S_IWOTH 00002
 * #define S_IXOTH 00001
 */


#ifdef HAVE_STRFTIME
/* We want our own dates for NLS */
#undef HAVE_STRFTIME
#endif

#undef gettext_noop
#define gettext_noop(x) x

void get_file_time (char *timestr, time_t file_time, int l);

static char **get_filelist_line (void *data, const int line_number, int *num_fields, int *tagged)
{E_
    struct file_entry *directentry;
    struct look_cool_list *fe;
    static char *fields[10], size[24], mode[65], timestr[32];
    static char name[520], *n;
    struct stat *s;
    struct portable_stat *ps;
    mode_t m;

    *num_fields = 4;		/* name, size, date, mode only (for the mean time) */

    fe = (struct look_cool_list *) data;
    if (line_number >= fe->n)
	return 0;
    directentry = fe->l;

    ps = &directentry[line_number].pstat;
    s = &ps->ustat;
    m = s->st_mode;
    n = name;
    strcpy (name, directentry[line_number].name);
    fields[0] = name;
    if (((int) m & S_IFMT) == S_IFCHR || ((int) m & S_IFMT) == S_IFBLK) {
        sprintf (size, "\t%lu, %3lu", (unsigned long) major (s->st_rdev), (unsigned long) minor (s->st_rdev));
    } else {
        sprintf (size, "\t%llu", (unsigned long long) s->st_size);
    }
    fields[1] = size;

    get_file_time (timestr, s->st_mtime, 0);
    fields[2] = timestr;

    pstat_to_mode_string (ps, mode);

    if (S_ISLNK (m)) {
	int l, i;
	char *p;
	p = directentry[line_number].name;
	l = strlen (n);
	for (i = 0; i < l; i++) {
	    *n++ = '\b';
	    *n++ = *p++;
	}
	*n++ = '\0';
    } else if (m & (S_IXUSR | S_IXGRP | S_IXOTH)) {
	int l, i;
	char *p;
	p = directentry[line_number].name;
	l = strlen (n);
	for (i = 0; i < l; i++) {
	    *n++ = '\r';
	    *n++ = *p++;
	}
	*n++ = '\0';
    }
    fields[3] = mode;
    fields[*num_fields] = 0;
    if (directentry[line_number].options & FILELIST_TAGGED_ENTRY)
	*tagged = 1;
    return fields;
}


CWidget *look_cool_draw_file_list (const char *identifier, Window parent, int x, int y,
			int width, int height, int line, int column,
			struct file_entry *directentry,
			long options)
{E_
    struct look_cool_list *fe;
    CWidget *w;
    int n;

    for (n = 0; directentry && !(directentry[n].options & FILELIST_LAST_ENTRY); n++);	/* count entries */

    fe = CMalloc (sizeof (struct look_cool_list));
    fe->l = CMalloc (sizeof (struct file_entry) * (n + 1));
    memcpy (fe->l, directentry, sizeof (struct file_entry) * n);
    memset (&fe->l[n], '\0', sizeof (struct file_entry));
    fe->n = n;

    w = CDrawFieldedTextbox (identifier, parent, x, y,
			     width, height, line, column,
			     get_filelist_line,
			     options, fe);
    w->hook = (void *) fe;
    w->destroy = destroy_filelist;

    return w;
}

CWidget *look_cool_redraw_file_list (const char *identifier, struct file_entry *directentry, int preserve)
{E_
    struct look_cool_list *fe;
    CWidget *w;
    int n;

    for (n = 0; directentry && !(directentry[n].options & FILELIST_LAST_ENTRY); n++);	/* count entries */

    w = CIdent (identifier);
    fe = (struct look_cool_list *) w->hook;
    free (fe->l);
    fe->l = CMalloc (sizeof (struct file_entry) * (n + 1));
    memcpy (fe->l, directentry, sizeof (struct file_entry) * n);
    memset (&fe->l[n], '\0', sizeof (struct file_entry));
    fe->n = n;

    w = CRedrawFieldedTextbox (identifier, preserve);

    return w;
}

struct file_entry *look_cool_get_file_list_line (CWidget * w, int line)
{E_
    struct look_cool_list *fe;
    static struct file_entry r;
    memset (&r, 0, sizeof (r));
    fe = (struct look_cool_list *) w->hook;
    if (line >= fe->n || line < 0)
	r.options = FILELIST_LAST_ENTRY;
    else
	r = fe->l[line];
    return &r;
}



/* }}} file list stuff */

/* {{{ file browser stuff */

extern int option_file_browser_width;
extern int option_file_browser_height;

static char *mime_majors[3] =
{"url", "text", 0};

/* walk up through the directories until we find one that exists: */
static struct file_entry *get_file_entry_list_host_change (int changed_host, char *host, char *dir, const char *filter, char *errmsg)
{
    struct file_entry *filelist = NULL;
    for (;;) {
	filelist = get_file_entry_list (host, dir, FILELIST_FILES_ONLY, filter, errmsg);
        if (!changed_host)
            break;
#warning  we need to distinguish between network errors and file system errors and not retry if this is a network error
        if (!filelist && dir[0] == '/' && !dir[1])
            break;
	if (!filelist) {
	    char *s;
	    s = strrchr (dir, '/');
	    if (s) {
		*s = '\0';
                if (!dir[0]) {
                    dir[0] = '/';
                    dir[1] = '\0';
                }
		continue;
	    }
	}
	break;
    }
    return filelist;
}


static Window draw_file_browser (const char *identifier, Window parent, int x, int y,
		    char *host, const char *directory, const char *file, const char *label)
{E_
    CWidget * w;
    struct file_entry *filelist = 0, *directorylist = 0;
    char *resolved_path, *p;
    int y2, x2, x3, y3;
    Window win;
    char errmsg[REMOTEFS_ERR_MSG_LEN] = "";
    char dir[MAX_PATH_LEN + 1];

    Cstrlcpy (dir, directory, MAX_PATH_LEN);

    if (parent == CRoot)
	win = CDrawMainWindow (identifier, label);
    else
	win = CDrawHeadedDialog (identifier, parent, x, y, label);
    (CIdent (identifier))->options |= WINDOW_ALWAYS_RAISED;
    CHourGlass (CFirstWindow);
    filelist = get_file_entry_list_host_change (1, host, dir, CLastInput (catstrs (identifier, ".filt", NULL)).data, errmsg);
    CUnHourGlass (CFirstWindow);
    if (!filelist || !(directorylist = get_file_entry_list (host, dir, FILELIST_DIRECTORIES_ONLY, "", errmsg))) {
	CErrorDialog (parent, 20, 20, _(" File browser "), "%s\n [%s] ", _(" Unable to read directory "), errmsg);
	CDestroyWidget (identifier);
	goto error;
    }
    CGetHintPos (&x, &y);
    resolved_path = pathdup (host, dir, errmsg);
    if (!resolved_path) {
        CErrorDialog (parent, 20, 20, _(" File browser "), "%s\n [%s] ", _(" Unable to read directory "), errmsg);
        goto error;
    }
    p = resolved_path + strlen (resolved_path) - 1;
    if (*p != '/') {
	*++p = '/';
	*++p = '\0';
    }
    (CDrawText (catstrs (identifier, ".dir", NULL), win, x, y, "%s", resolved_path))->position |= POSITION_FILL;
    free (resolved_path);
    CGetHintPos (0, &y);
    reset_hint_pos (x, y);
    y3 = y;
    (w = CDrawFilelist (catstrs (identifier, ".fbox", NULL), win, x, y,
		  FONT_MEAN_WIDTH * option_file_browser_width + 7, FONT_PIX_PER_LINE * option_file_browser_height + 6, 0, 0, filelist, TEXTBOX_FILE_LIST))->position |= POSITION_WIDTH | POSITION_HEIGHT;
    xdnd_set_type_list (CDndClass, w->winid, xdnd_typelist_send[DndFiles]);
    (CIdent (catstrs (identifier, ".fbox", NULL)))->options |= TEXTBOX_MARK_WHOLE_LINES;

/* the vertical scroll bar is named the same as the text box, with a ".vsc" at the end: */
    CSetMovement (catstrs (identifier, ".fbox.vsc", NULL), POSITION_HEIGHT | POSITION_RIGHT);
    CSetMovement (catstrs (identifier, ".fbox.hsc", NULL), POSITION_WIDTH | POSITION_BOTTOM);
    CGetHintPos (&x2, &y2);
    x3 = x2;
    (w = CDrawFilelist (catstrs (identifier, ".dbox", NULL), win, x2, y + TICK_BUTTON_WIDTH + WIDGET_SPACING,
		  FONT_MEAN_WIDTH * 24 + 7, y2 - WIDGET_SPACING * 3 - 12 - y - TICK_BUTTON_WIDTH, 0, 0, directorylist, TEXTBOX_FILE_LIST))->position |= POSITION_HEIGHT | POSITION_RIGHT;
    xdnd_set_type_list (CDndClass, w->winid, xdnd_typelist_send[DndFiles]);

/* Toolhint */
    CSetToolHint (catstrs (identifier, ".dbox", NULL), _("Double click to enter directories"));
    (CIdent (catstrs (identifier, ".dbox", NULL)))->options |= TEXTBOX_MARK_WHOLE_LINES;
    CSetMovement (catstrs (identifier, ".dbox.vsc", NULL), POSITION_HEIGHT | POSITION_RIGHT);
    CSetMovement (catstrs (identifier, ".dbox.hsc", NULL), POSITION_RIGHT | POSITION_BOTTOM);
    CGetHintPos (&x2, &y2);
    (CDrawText (catstrs (identifier, ".msg", NULL), win, x, y2, _("Alt-Ins for clip history, Shift-Up for history")))->position |= POSITION_FILL | POSITION_BOTTOM;
    CGetHintPos (0, &y2);
    (w = CDrawTextInputP (catstrs (identifier, ".finp", NULL), win, x, y2,
		    WIDGET_SPACING * 2 - 2, AUTO_HEIGHT, 256, file))->position |= POSITION_FILL | POSITION_BOTTOM;
    xdnd_set_type_list (CDndClass, w->winid, xdnd_typelist_send[DndFile]);
    w->funcs->types = DndFile;
    w->funcs->mime_majors = mime_majors;

    CGetHintPos (0, &y2);

    if (host) {
/* Label for IP input line. For example, "192.168.0.99" */
        (CDrawText (catstrs (identifier, ".hstx", NULL), win, x, y2, _("IP : ")))->position |= POSITION_BOTTOM;
        CGetHintPos (&x, 0);
        (CDrawTextInputP (catstrs (identifier, ".host", NULL), win, x, y2,
		        FONT_MEAN_WIDTH * 24, AUTO_HEIGHT, 255, host))->position |= POSITION_BOTTOM;
        CGetHintPos (&x, 0);
/* Toolhint */
        CSetToolHint (catstrs (identifier, ".hstx", NULL), _("IP address of remote host running remotefs export tool, or " REMOTEFS_LOCAL));
        CSetToolHint (catstrs (identifier, ".host", NULL), _("IP address of remote host running remotefs export tool, or " REMOTEFS_LOCAL));
    }

/* Label for file filter input line. For example, to list files matching '*.c' */
    (CDrawText (catstrs (identifier, ".filx", NULL), win, x, y2, _("Filter : ")))->position |= POSITION_BOTTOM;
    CGetHintPos (&x, 0);
    (CDrawTextInputP (catstrs (identifier, ".filt", NULL), win, x, y2,
		    WIDGET_SPACING * 2 - 2, AUTO_HEIGHT, 256, TEXTINPUT_LAST_INPUT))->position |= POSITION_FILL | POSITION_BOTTOM;

/* Toolhint */
    CSetToolHint (catstrs (identifier, ".filt", NULL), _("List only files matching this shell filter"));
    CSetToolHint (catstrs (identifier, ".filx", NULL), _("List only files matching this shell filter"));
    (CDrawPixmapButton (catstrs (identifier, ".ok", NULL), win, x3, y3,
		       PIXMAP_BUTTON_TICK))->position |= POSITION_RIGHT;
/* Toolhint */
    CSetToolHint (catstrs (identifier, ".ok", NULL), _("Accept, Enter"));

    (CDrawPixmapButton (catstrs (identifier, ".cancel", NULL), win, x2 - WIDGET_SPACING * 2 - TICK_BUTTON_WIDTH - 20, y3,
		       PIXMAP_BUTTON_CROSS))->position |= POSITION_RIGHT;
/* Toolhint */
    CSetToolHint (catstrs (identifier, ".cancel", NULL), _("Abort this dialog, Escape"));
    CSetSizeHintPos (identifier);
    CMapDialog (identifier);
    y = (CIdent (identifier))->height;
    CSetWindowResizable (identifier, FONT_MEAN_WIDTH * 40, min (FONT_PIX_PER_LINE * 5 + 210, y), 1600, 1200);	/* minimum and maximum sizes */

  error:
    if (directorylist)
	free (directorylist);
    if (filelist)
	free (filelist);
    return win;
}

static int how_much_matches (const char *a, const char *b)
{E_
    int n = 0;
    while (*a && *b && *a++ == *b++)
        n++;
    return n;
}

/* returns 0 on fail */
static int goto_partial_file_name (CWidget * list, char *text)
{E_
    int i = 0;
    struct file_entry *fe = 0;
    char *e;
    int max_match = -1;
    int found_matchiest = -1;

    for (;;) {
        int n;
	if (!strlen (text))
	    break;
	if (list->kind == C_FIELDED_TEXTBOX_WIDGET) {
	    fe = CGetFilelistLine (list, i);
	    e = fe->name;
	} else {
	    e = CGetTextBoxLine (list, i);
	    if (e)
		while (*e == '/')
		    e++;
	}
	if (!e)
	    break;
        n = how_much_matches (e, text);
        if (max_match < n) {
            max_match = n;
            found_matchiest = i;
        }
	if (list->kind == C_FIELDED_TEXTBOX_WIDGET) {
	    if (fe->options & FILELIST_LAST_ENTRY)
		break;
	} else {
	    if (i >= list->numlines - 1)
		break;
	}
	i++;
    }
    if (max_match >= 1) {
        CSetTextboxPos (list, TEXT_SET_CURSOR_LINE, found_matchiest);
        CSetTextboxPos (list, TEXT_SET_LINE, found_matchiest);
        return 1;
    }
    return 0;
}

/* options */
#define GETFILE_GET_DIRECTORY		1
#define GETFILE_GET_EXISTING_FILE	2
#define GETFILE_BROWSER			4
#define GETFILE_WITH_REMOTE		8



/*
   Returns "" on no file entered and NULL on exit (i.e. Cancel button pushed)
   else returns the file or directory. Result must be immediately copied.
   Result must not be free'd.
 */
static char *handle_browser (const char *identifier, CEvent * cwevent, int options, int *init)
{E_
    char *r = "";
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    char host[256] = REMOTEFS_LOCAL;
    struct portable_stat st;
    char *q;
    char *idd = catstrs (identifier, ".dbox", NULL);
    char *idf = catstrs (identifier, ".fbox", NULL);
    static char estr[MAX_PATH_LEN + 1];
    CWidget *directory = CIdent (catstrs (identifier, ".dir", NULL));
    CWidget *filelist = CIdent (idf);
    CWidget *directorylist = CIdent (idd);
    CWidget *textinput = CIdent (catstrs (identifier, ".finp", NULL));
    CWidget *filterinput = CIdent (catstrs (identifier, ".filt", NULL));
    CWidget *ipinput = NULL;
    static char last_filterinput[256 + 1] = "";
    static char last_ipinput[256 + 1] = "";
    static Window last_focus = 0;
    int reread_filelist = 0;

#define HOST            (ipinput ? ipinput->text.data : "")

    if ((options & GETFILE_WITH_REMOTE)) {
        ipinput = CIdent (catstrs (identifier, ".host", NULL));
        strcpy (host, HOST);
    }

    CSetDndDirectory (directory->text.data);

    if (cwevent->type == ButtonPress || cwevent->type == KeyPress)
	CRedrawText (catstrs (identifier, ".msg", NULL), _("Alt-Ins for clip history, Shift-Up for history"));

    if (!*init) {
        *init = 1;
        last_focus = CGetFocus();
        strcpy (last_filterinput, filterinput->text.data);
        strcpy (last_ipinput, HOST);
    }
    if (ipinput && CGetFocus() != ipinput->winid && last_focus == ipinput->winid)
        reread_filelist = 1;
    if (!strcmp (cwevent->ident, filterinput->ident) && cwevent->command == CK_Enter)
        reread_filelist = 1;
    if (CGetFocus() != filterinput->winid && last_focus == filterinput->winid)
        reread_filelist = 1;
    if (ipinput && !strcmp (cwevent->ident, ipinput->ident) && cwevent->command == CK_Enter)
        reread_filelist = 1;
    last_focus = CGetFocus();

    if (reread_filelist) {
        int changed_host = 0;
        char dir[MAX_PATH_LEN + 1];
	struct file_entry *f;
	if (ipinput && !ipinput->text.len) {
            CStr_free(&ipinput->text);
            ipinput->text = CStr_dup(REMOTEFS_LOCAL);
	    CExpose (ipinput->ident);
	}
	if (!filterinput->text.len) {
            CStr_free(&filterinput->text);
            filterinput->text = CStr_dup("*");
	    CExpose (filterinput->ident);
	}
        if (!strcmp (last_filterinput, filterinput->text.data) && !strcmp (last_ipinput, HOST)) {
            r = "";
            goto out;
        }

        if (strcmp (last_ipinput, HOST))
            changed_host = 1;

        strcpy (last_filterinput, filterinput->text.data);
        strcpy (last_ipinput, HOST);
	CHourGlass (CFirstWindow);
        Cstrlcpy (dir, directory->text.data, MAX_PATH_LEN);
        f = get_file_entry_list_host_change (changed_host, ipinput ? ipinput->text.data : 0, dir, filterinput->text.data, errmsg);
	CRedrawFilelist (idf, f, 0);
        if (f && strcmp (dir, directory->text.data)) {
            CRedrawText (catstrs (identifier, ".dir", NULL), "%s", dir);
            free (f);
            CRedrawFilelist (catstrs (identifier, ".dbox", NULL), f = get_file_entry_list (ipinput ? ipinput->text.data : 0, dir, FILELIST_DIRECTORIES_ONLY, "", errmsg), 0);
        }

	if (f)
	    free (f);
        else
	    CRedrawText (catstrs (identifier, ".msg", NULL), "[%s]", errmsg);
	CUnHourGlass (CFirstWindow);
        r = "";
        goto out;
    }

    if (!strcmp (cwevent->ident, idf) && !(options & (GETFILE_GET_DIRECTORY | GETFILE_BROWSER))) {
	if (cwevent->button == Button1 || cwevent->command == CK_Enter) {
	    q = (CGetFilelistLine (filelist, filelist->cursor))->name;
	    CDrawTextInputP (textinput->ident, CIdent (identifier)->winid, textinput->x, textinput->y,
			    textinput->width, textinput->height, 256, q);
	} else if (cwevent->xlat_len > 0) {
	    textinput_insert (textinput, CStr_const (cwevent->xlat, cwevent->xlat_len));
	    if (goto_partial_file_name (filelist, textinput->text.data))
		CExpose (filelist->ident);
	    CExpose (textinput->ident);
	} else if (cwevent->command == CK_BackSpace && textinput->cursor > 0) {
            textinput->cursor--;
            textinput->text.len = textinput->cursor;
	    textinput->text.data[textinput->cursor] = '\0';
	    if (goto_partial_file_name (filelist, textinput->text.data))
		CExpose (filelist->ident);
	    CExpose (textinput->ident);
	}
    }
    if (!strcmp (cwevent->ident, idd)) {
	if (cwevent->button == Button1 || cwevent->command == CK_Enter) {
	    q = (CGetFilelistLine (directorylist, directorylist->cursor))->name;
	    CDrawTextInputP (catstrs (identifier, ".finp", NULL), CIdent (identifier)->winid, textinput->x, textinput->y,
			textinput->width, textinput->height, 256, q);
	} else if (cwevent->xlat_len > 0) {
	    textinput_insert (textinput, CStr_const (cwevent->xlat, cwevent->xlat_len));
	    if (goto_partial_file_name (directorylist, textinput->text.data))
		CExpose (directorylist->ident);
	    CExpose (textinput->ident);
	} else if (cwevent->command == CK_BackSpace && textinput->cursor > 0) {
            textinput->cursor--;
            textinput->text.len = textinput->cursor;
	    textinput->text.data[textinput->cursor] = '\0';
	    if (goto_partial_file_name (directorylist, textinput->text.data))
		CExpose (directorylist->ident);
	    CExpose (textinput->ident);
	}
    }
    if (!strcmp (cwevent->ident, textinput->ident)) {
	switch ((int) cwevent->command) {
	case CK_Enter:
	    if (!(options & (GETFILE_GET_DIRECTORY | GETFILE_BROWSER)))
		if (!strncmp ((CGetFilelistLine (filelist, filelist->cursor))->name, textinput->text.data, strlen (textinput->text.data))) {
		    CFocus (filelist);
		    CExpose (filelist->ident);
                    r = "";
                    goto out;
		}
	    break;
	case CK_Down:
	    if (textinput->keypressed) {
		if (goto_partial_file_name (filelist, textinput->text.data)) {
		    CFocus (filelist);
		    CExpose (filelist->ident);
		    break;
		}
	    }
	    CFocus (filelist);
	    CSetTextboxPos (filelist, TEXT_SET_CURSOR_LINE, 0);
	    CExpose (filelist->ident);
	    break;
	case CK_Up:
	    if (textinput->keypressed) {
		if (goto_partial_file_name (filelist, textinput->text.data)) {
		    CFocus (filelist);
		    CExpose (filelist->ident);
		    break;
		}
	    }
	    CFocus (filelist);
	    CSetTextboxPos (filelist, TEXT_SET_CURSOR_LINE, 999999);
	    CExpose (filelist->ident);
	    break;
	case CK_Page_Down:
	    CFocus (filelist);
	    CSetTextboxPos (filelist, TEXT_SET_CURSOR_LINE, filelist->height / FONT_PIX_PER_LINE - 1);
	    CExpose (filelist->ident);
	    break;
	case CK_Page_Up:
	    CFocus (filelist);
	    CSetTextboxPos (filelist, TEXT_SET_CURSOR_LINE, filelist->numlines - filelist->height / FONT_PIX_PER_LINE + 1);
	    CExpose (filelist->ident);
	    break;
	default:
	    if (cwevent->xlat_len > 0)
		if (goto_partial_file_name (filelist, textinput->text.data))
		    CExpose (filelist->ident);
	    break;
	}
    }
    if (cwevent->command == CK_Cancel || !strcmp (cwevent->ident, catstrs (identifier, ".cancel", NULL)))
	return 0;

    if (options & GETFILE_GET_DIRECTORY) {
	if (!strcmp (cwevent->ident, catstrs (identifier, ".ok", NULL))) {
	    char *resolved_path;
            char errmsg[REMOTEFS_ERR_MSG_LEN] = "";
	    resolved_path = pathdup (ipinput ? ipinput->text.data : 0, directory->text.data, errmsg);
            if (!resolved_path) {
	        CRedrawText (catstrs (identifier, ".msg", NULL), "[%s]", errmsg);
                r = "";
                goto out;
            }
	    strcpy (estr, resolved_path);
	    free (resolved_path);
            r = estr;
            goto out;
	}
    }
    if (!strcmp (cwevent->ident, catstrs (identifier, ".ok", NULL)) || cwevent->command == CK_Enter
	|| (cwevent->double_click && !strcmp (cwevent->ident, catstrs (identifier, ".host", NULL)))
	|| (cwevent->double_click && !strcmp (cwevent->ident, catstrs (identifier, ".finp", NULL)))
	|| (cwevent->double_click && !strcmp (cwevent->ident, catstrs (identifier, ".dbox", NULL)))
	|| (cwevent->double_click && !strcmp (cwevent->ident, catstrs (identifier, ".fbox", NULL)))) {
	char *resolved_path;
        char errmsg[REMOTEFS_ERR_MSG_LEN] = "";
        int just_not_there = 0;
        struct remotefs *u;
        remotefs_error_code_t error_code = 0;
        char dir[MAX_PATH_LEN + 1];

        Cstrlcpy (dir, directory->text.data, MAX_PATH_LEN);
        u = remotefs_lookup (ipinput ? ipinput->text.data : 0, dir);
        if (strcmp (dir, directory->text.data))
            CRedrawText (catstrs (identifier, ".dir", NULL), "%s", dir);

	if (*textinput->text.data == '/' || *textinput->text.data == '~') {
	    strcpy (estr, textinput->text.data);
	} else {
            if (!ipinput || !strcmp (host, ipinput->text.data)) {
	        strcpy (estr, directory->text.data);
	        strcat (estr, "/");
	        strcat (estr, textinput->text.data);
            } else {
	        strcpy (estr, remotefs_home_dir (u));
            }
	}
	resolved_path = pathdup (ipinput ? ipinput->text.data : 0, estr, errmsg);
        if (!resolved_path) {
            CRedrawText (catstrs (identifier, ".msg", NULL), "[%s]", errmsg);
            r = "";
            goto out;
        }
	strcpy (estr, resolved_path);
	free (resolved_path);
	textinput->keypressed = 0;
	q = estr + strlen (estr) - 1;
	if (!estr[0]) {
            r = "";
            goto out;
        }
	if ((*u->remotefs_stat) (u, estr, &st, &just_not_there, &error_code, errmsg)) {
	    CRedrawText (catstrs (identifier, ".msg", NULL), "[%s]", errmsg);
            r = "";
            goto out;
	}
        if (just_not_there) {
	    CRedrawText (catstrs (identifier, ".msg", NULL), "[%s]", errmsg);
/* The user wanted a directory, but typed in one that doesn't exist */
            if (*q != '/' && !(options & GETFILE_GET_EXISTING_FILE) && !(options & (GETFILE_GET_DIRECTORY | GETFILE_BROWSER))) {
/* user wants a new file */
                r = estr;
                goto out;
            }
            r = "";
            goto out;
        }
/* ********* */
	if (S_ISDIR (st.ustat.st_mode)) {
	    struct file_entry *g = 0, *f = 0;
	    CHourGlass (CFirstWindow);
	    g = get_file_entry_list (ipinput ? ipinput->text.data : 0, estr, FILELIST_FILES_ONLY, filterinput->text.data, errmsg);
	    CUnHourGlass (CFirstWindow);
	    if (g) {
		CRedrawFilelist (catstrs (identifier, ".fbox", NULL), g, 0);
		CRedrawFilelist (catstrs (identifier, ".dbox", NULL), f = get_file_entry_list (ipinput ? ipinput->text.data : 0, estr, FILELIST_DIRECTORIES_ONLY, "", errmsg), 0);
		if (*q != '/') {
		    *++q = '/';
		    *++q = '\0';
		}
		CRedrawText (catstrs (identifier, ".dir", NULL), "%s", estr);
		if (options & GETFILE_BROWSER) {
                    CStr s;
                    s.data = estr;
                    s.len = strlen(estr);
		    CAddToTextInputHistory (textinput->ident, s);
                }
                if (!f)
		    CRedrawText (catstrs (identifier, ".msg", NULL), "[%s]", errmsg);
	    } else {
	        CRedrawText (catstrs (identifier, ".msg", NULL), "[%s]", errmsg);
            }
	    if (g)
		free (g);
	    if (f)
		free (f);
	    r = "";
            goto out;
	} else {
	    if (options & (GETFILE_GET_DIRECTORY | GETFILE_BROWSER)) {
		CRedrawText (catstrs (identifier, ".msg", NULL), "[%s]", errmsg);
		r = "";
                goto out;
	    }
	    r = estr;	/* entry exists and is a file */
            goto out;
	}
    }

  out:
    return r;
}

Window find_mapped_window (Window w);

/* result must be free'd */
char *look_cool_get_file_or_dir (Window parent, int x, int y,
       char *host, const char *dir, const char *file, const char *label, int options)
{E_
    CEvent cwevent;
    XEvent xevent;
    CState s;
    CWidget *w;
    int init = 0;

    CBackupState (&s);
    CDisable ("*");
    CEnable ("_cfileBr*");

    parent = find_mapped_window (parent);
    if (!(x | y)) {
	x = 20;
	y = 20;
    }
    draw_file_browser ("CGetFile", parent, x, y, host, dir, file, label);

    CFocus (CIdent ("CGetFile.finp"));

    file = "";
    do {
	CNextEvent (&xevent, &cwevent);
	if (xevent.type == Expose || !xevent.type
	    || xevent.type == InternalExpose || xevent.type == TickEvent)
	    continue;
	if (!CIdent ("CGetFile")) {
	    file = 0;
	    break;
	}
	if (xevent.type == Expose || !xevent.type || xevent.type == AlarmEvent
	  || xevent.type == InternalExpose || xevent.type == TickEvent) {
	    file = "";
	    continue;
	}
	file = handle_browser ("CGetFile", &cwevent, options | (host ? GETFILE_WITH_REMOTE : 0), &init);
	if (!file)
	    break;
    } while (!(*file));

    if (file && host && (w = CIdent ("CGetFile.host")))
        strcpy (host, w->text.data);

/* here we want to add the complete path to the text-input history: */
    w = CIdent ("CGetFile.finp");
    if (w) {
        CStr_free(&w->text);
        w->text = CStr_dup (file ? file : "");
    }
    w = CIdent ("CGetFile.fbox");
    if (w) {
	option_file_browser_width = (w->width - 7) / FONT_MEAN_WIDTH;
	if (option_file_browser_width < 10)
	    option_file_browser_width = 10;
	option_file_browser_height = (w->height - 6) / FONT_PIX_PER_LINE;
	if (option_file_browser_height < 10)
	    option_file_browser_height = 10;
    }
    CDestroyWidget ("CGetFile");	/* text is added to history 
					   when text-input widget is destroyed */

    CRestoreState (&s);

    if (file) {
	return (char *) strdup (file);
    } else
	return 0;
}

static int cb_browser (CWidget * w, XEvent * x, CEvent * c)
{E_
    char id[32], *s;
    int init = 0;
    strcpy (id, w->ident);
    s = strchr (id, '.');
    if (s)
	*s = 0;
    if (!handle_browser (id, c, GETFILE_BROWSER | GETFILE_WITH_REMOTE, &init)) {
	w = CIdent (catstrs (id, ".finp", NULL));
	if (w) {
            CStr_free(&w->text);
            w->text = CStr_dup("");
        }
	CDestroyWidget (id);
    }
    return 0;
}

void look_cool_draw_browser (const char *ident, Window parent, int x, int y,
		   char *host, const char *dir, const char *file, const char *label)
{E_
    if (!(parent | x | y)) {
	parent = CFirstWindow;
	x = 20;
	y = 20;
    }

    draw_file_browser (ident, parent, x, y, host, dir, file, label);

    CAddCallback (catstrs (ident, ".dbox", NULL), cb_browser);
    CAddCallback (catstrs (ident, ".fbox", NULL), cb_browser);
    CAddCallback (catstrs (ident, ".finp", NULL), cb_browser);
    CAddCallback (catstrs (ident, ".host", NULL), cb_browser);
    CAddCallback (catstrs (ident, ".filt", NULL), cb_browser);
    CAddCallback (catstrs (ident, ".ok", NULL), cb_browser);
    CAddCallback (catstrs (ident, ".cancel", NULL), cb_browser);

    CFocus (CIdent (catstrs (ident, ".finp", NULL)));
}


/* }}} file browser stuff */

int find_menu_hotkey (struct menu_item m[], int this, int num);

/* outermost bevel */
#define BEVEL_MAIN	2
/* next-outermost bevel */
#define BEVEL_IN 	4
#define BEVEL_OUT	5
/* between items, and between items and next-outermost bevel */
#define SPACING		4
/* between items rectangle and text */
#define RELIEF		(TEXT_RELIEF + 1)

#define S		SPACING
/* between window border and items */
#define O		(BEVEL_OUT + SPACING)
/* total height of an item */

/* size of bar item */
#define BAR_HEIGHT	8
#define ITEM_BEVEL_TYPE 1
#define ITEM_BEVEL      1

#define H		(FONT_PIX_PER_LINE + RELIEF * 2)

#define B		BAR_HEIGHT

const char *get_default_widget_font (void);

static void look_cool_get_menu_item_extents (int n, int j, struct menu_item m[], int *border, int *relief, int *y1, int *y2)
{E_
    int i, n_items = 0, n_bars = 0;

    *border = O;
    *relief = RELIEF;

    if (!n || j < 0) {
	*y1 = O;
	*y2 = *y1 + H;
    } else {
	int not_bar;
	not_bar = (m[j].text[2] != '\0');
	for (i = 0; i < j; i++)
	    if (m[i].text[2])
		n_items++;
	    else
		n_bars++;
	*y1 = O + n_items * (H + S) + n_bars * (B + S) + (not_bar ? 0 : 2);
	*y2 = *y1 + (not_bar ? H : (B - 4));
    }
}

static void look_cool_menu_draw (Window win, int w, int h, struct menu_item m[], int n, int light)
{E_
    int i, y1, y2, offset = 0;
    static int last_light = 0, last_n = 0;
    static Window last_win = 0;

    render_bevel (win, 0, 0, w - 1, h - 1, BEVEL_MAIN, 0);
    render_bevel (win, BEVEL_IN, BEVEL_IN, w - 1 - BEVEL_IN, h - 1 - BEVEL_IN, BEVEL_OUT - BEVEL_IN, 1);

    if (last_win == win && last_n != n) {
	XClearWindow (CDisplay, win);
    } else if (last_light >= 0 && last_light < n) {
	int border, relief;
	look_cool_get_menu_item_extents (n, last_light, m, &border, &relief, &y1, &y2);
	CSetColor (COLOR_FLAT);
	CRectangle (win, O - 1, y1 - 1, w - O * 2 + 2, y2 - y1 + 2);
    }
    last_win = win;
    last_n = n;
    CPushFont ("widget", 0);
    for (i = 0; i < n; i++) {
	int border, relief;
	look_cool_get_menu_item_extents (n, i, m, &border, &relief, &y1, &y2);
	if (i == light && m[i].text[2]) {
	    offset = 1;
	    CSetColor (color_widget (11));
	    CRectangle (win, O + 1, y1 + 1, w - O * 2 - 2, y2 - y1 - 2);
	    render_bevel (win, O - 1, y1 - 1, w - O, y2, 2, 0);
	} else {
	    if (!(m[i].text[2]))
		render_bevel (win, O + 6, y1, w - O - 1 - 6, y2 - 1, 2, 0);
	    else
		render_bevel (win, O, y1, w - O - 1, y2 - 1, ITEM_BEVEL, ITEM_BEVEL_TYPE);
	    offset = 0;
	}
	if (m[i].text[2]) {

	    char *u;
	    u = strrchr (m[i].text, '\t');
	    if (u)
		*u = 0;
	    CSetColor (COLOR_BLACK);
	    if (m[i].hot_key == '~')
		m[i].hot_key = find_menu_hotkey (m, i, n);
	    if (i == light)
		CSetBackgroundColor (color_widget (11));
	    else
		CSetBackgroundColor (COLOR_FLAT);
	    drawstring_xy_hotkey (win, RELIEF + O - offset,
			  RELIEF + y1 - offset, m[i].text, m[i].hot_key);
	    if (u) {
		drawstring_xy (win, RELIEF + O + (w - (O + RELIEF) * 2 - CImageStringWidth (u + 1)) - offset,
			       RELIEF + y1 - offset, u + 1);
		*u = '\t';
	    }
	}
    }
    last_light = light;
    CPopFont ();
}

static void look_cool_render_menu_button (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;
    int x = 0, y = 0;

    Window win = wdt->winid;

    if (wdt->disabled)
	goto disabled;
    if (wdt->options & BUTTON_PRESSED) {
	render_bevel (win, x, y, x + w - 1, y + h - 1, 2, 1);
    } else if (wdt->options & BUTTON_HIGHLIGHT) {
	CSetColor (COLOR_FLAT);
	XDrawRectangle (CDisplay, win, CGC, x + 1, y + 1, w - 3, h - 3);
	render_bevel (win, x, y, x + w - 1, y + h - 1, 1, 0);
    } else {
  disabled:
	CSetColor (COLOR_FLAT);
	XDrawRectangle (CDisplay, win, CGC, x, y, w - 1, h - 1);
	XDrawRectangle (CDisplay, win, CGC, x + 1, y + 1, w - 3, h - 3);
    }

    if (!wdt->label)
	return;
    if (!(*(wdt->label)))
	return;
    CPushFont ("widget", 0);
    CSetColor (COLOR_BLACK);
    CSetBackgroundColor (COLOR_FLAT);
    drawstring_xy_hotkey (win, x + 2 + BUTTON_RELIEF, y + 2 + BUTTON_RELIEF, wdt->label, wdt->hotkey);
    CPopFont ();
}

static void look_cool_render_button (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;
    int x = 0, y = 0;

    Window win = wdt->winid;
#define BUTTON_BEVEL 2

    if (wdt->disabled)
	goto disabled;
    if (wdt->options & BUTTON_PRESSED) {
	render_bevel (win, x, y, x + w - 1, y + h - 1, BUTTON_BEVEL, 1);
    } else if (wdt->options & BUTTON_HIGHLIGHT) {
	CSetColor (COLOR_FLAT);
	XDrawRectangle (CDisplay, win, CGC, x + 1, y + 1, w - 3, h - 3);
	render_bevel (win, x, y, x + w - 1, y + h - 1, 1, 0);
    } else {
      disabled:
	render_bevel (win, x, y, x + w - 1, y + h - 1, BUTTON_BEVEL, 0);
    }

    if (!wdt->label)
	return;
    if (!(*(wdt->label)))
	return;
    CPushFont ("widget", 0);
    CSetColor (COLOR_BLACK);
    CSetBackgroundColor (COLOR_FLAT);
    drawstring_xy_hotkey (win, x + 2 + BUTTON_RELIEF, y + 2 + BUTTON_RELIEF, wdt->label, wdt->hotkey);
    CPopFont ();
}

static void look_cool_render_bar (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;

    Window win = wdt->winid;

    CSetColor (COLOR_FLAT);
    CLine (win, 1, 1, w - 2, 1);
    render_bevel (win, 0, 0, w - 1, h - 1, 1, 1);
}

void look_cool_render_sunken_bevel (Window win, int x1, int y1, int x2, int y2, int thick, int sunken)
{E_
    int i;

    if ((sunken & 2)) {
	CSetColor (COLOR_FLAT);
	CRectangle (win, x1 + thick, y1 + thick, x2 - x1 - 2 * thick + 1, y2 - y1 - 2 * thick + 1);
    }

    i = /* thick - 1 */ 0;

    CSetColor (color_widget (11));
    CLine (win, x1 + i, y2 - i, x2 - 1 - i, y2 - i);
    CLine (win, x2 - i, y1 + i, x2 - i, y2 - i - 1);

    CSetColor (color_widget (7));
    CLine (win, x1, y1, x1, y2 - 1);
    CLine (win, x1, y1, x2 - 1, y1);

    if (thick > 1) {
	CSetColor (color_widget (4));
	for (i = 1; i < thick; i++) {
	    CLine (win, x1 + i + 1, y1 + i, x2 - 1 - i, y1 + i);
	    CLine (win, x1 + i, y1 + i, x1 + i, y2 - 1 - i);
	}
	CSetColor (color_widget (13));
	for (i = 1; i < thick; i++) {
	    CLine (win, x2 - i, y1 + i, x2 - i, y2 - i - 1);
	    CLine (win, x1 + i, y2 - i, x2 - i - 1, y2 - i);
	}
    }
    CSetColor (color_widget (14));
    for (i = 0; i < thick; i++)
	XDrawPoint (CDisplay, win, CGC, x2 - i, y2 - i);
}

static void look_cool_render_raised_bevel (Window win, int x1, int y1, int x2, int y2, int thick, int sunken)
{E_
    int i;

    if ((sunken & 2)) {
	CSetColor (COLOR_FLAT);
	CRectangle (win, x1 + thick, y1 + thick, x2 - x1 - 2 * thick + 1, y2 - y1 - 2 * thick + 1);
    }

    i = thick - 1;

    CSetColor (color_widget (7));
    CLine (win, x1 + i, y2 - i, x2 - i - 1, y2 - i);
    CLine (win, x2 - i, y1 + i, x2 - i, y2 - i);

    CSetColor (color_widget (12));
    CLine (win, x1 + i, y1 + i, x1 + i, y2 - 1 - i);
    CLine (win, x1 + 1 + i, y1 + i, x2 - 1 - i, y1 + i);

    if (thick > 1) {
	CSetColor (color_widget (11));
	for (i = 0; i < thick - 1; i++) {
	    CLine (win, x1 + i + 1, y1 + i, x2 - 1 - i, y1 + i);
	    CLine (win, x1 + i, y1 + i + 1, x1 + i, y2 - 1 - i);
	}
	CSetColor (color_widget (4));
	for (i = 0; i < thick - 1; i++) {
	    CLine (win, x2 - i, y1 + i, x2 - i, y2 - i);
	    CLine (win, x1 + i, y2 - i, x2 - i - 1, y2 - i);
	}
    }
    CSetColor (color_widget (15));
    for (i = 0; i < thick; i++)
	XDrawPoint (CDisplay, win, CGC, x1 + i, y1 + i);
}

static void look_cool_draw_hotkey_understroke (Window win, int x, int y, int hotkey)
{E_
    CLine (win, x, y, x + FONT_PER_CHAR (hotkey), y);
    CLine (win, x - 1, y + 1, x + FONT_PER_CHAR (hotkey) / 2, y + 1);
    CLine (win, x - 1, y + 2, x + FONT_PER_CHAR (hotkey) / 4 - 1, y + 2);
}

static void look_cool_render_text (CWidget * wdt)
{E_
    Window win = wdt->winid;
    char text[1024], *p, *q;
    int hot, y, w = wdt->width, center = 0;

    if ((wdt->options & TEXT_FIXED))
	CPushFont ("editor", 0);
    else
	CPushFont ("widget", 0);

    CSetColor (COLOR_FLAT);
    CRectangle (win, 1, 1, w - 2, wdt->height - 2);
    CSetColor (COLOR_BLACK);

    hot = wdt->hotkey;		/* a letter that needs underlining */
    y = 1;			/* bevel */
    q = wdt->text.data;

    CSetBackgroundColor (COLOR_FLAT);
    for (;;) {
	p = strchr (q, '\n');
	if (!p) {	/* last line */
	    if (wdt->options & TEXT_CENTRED)
		center = (wdt->width - (TEXT_RELIEF + 1) * 2 - CImageTextWidth (q, strlen (q))) / 2;
	    drawstring_xy_hotkey (win, TEXT_RELIEF + 1 + center,
		 TEXT_RELIEF + y, q, hot);
	    break;
	} else {
	    int l;
	    l = min (1023, (unsigned long) p - (unsigned long) q);
	    memcpy (text, q, l);
	    text[l] = 0;
	    if (wdt->options & TEXT_CENTRED)
		center = (wdt->width - (TEXT_RELIEF + 1) * 2 - CImageTextWidth (q, l)) / 2;
	    drawstring_xy_hotkey (win, TEXT_RELIEF + 1 + center,
		 TEXT_RELIEF + y, text, hot);
	}
	y += FONT_PIX_PER_LINE;
	hot = 0;	/* only for first line */
	q = p + 1;	/* next line */
    }
    render_bevel (win, 0, 0, w - 1, wdt->height - 1, 1, 1);
    CPopFont ();
}

static void look_cool_render_window (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;

    Window win = wdt->winid;

    if (wdt->options & WINDOW_NO_BORDER)
	return;

    if (wdt->position & WINDOW_RESIZABLE) {
	CSetColor (color_widget (13));
	CLine (win, w - 4, h - 31, w - 31, h - 4);
	CLine (win, w - 4, h - 21, w - 21, h - 4);
	CLine (win, w - 4, h - 11, w - 11, h - 4);

	CLine (win, w - 4, h - 32, w - 32, h - 4);
	CLine (win, w - 4, h - 22, w - 22, h - 4);
	CLine (win, w - 4, h - 12, w - 12, h - 4);

	CSetColor (color_widget (3));
	CLine (win, w - 4, h - 27, w - 27, h - 4);
	CLine (win, w - 4, h - 17, w - 17, h - 4);
	CLine (win, w - 4, h -  7, w -  7, h - 4);

	CLine (win, w - 4, h - 28, w - 28, h - 4);
	CLine (win, w - 4, h - 18, w - 18, h - 4);
	CLine (win, w - 4, h -  8, w -  8, h - 4);
    }
    render_bevel (win, 0, 0, w - 1, h - 1, 2, 0);
    if (CRoot != wdt->parentid)
	if (win == CGetFocus ())
	    render_bevel (win, 4, 4, w - 5, h - 5, 3, 1);
}

static void look_cool_render_vert_scrollbar (Window win, int x, int y, int w, int h, int pos, int prop, int pos2, int prop2, int flags)
{E_
    int l = h - 10 * w / 3 - 5;

    render_bevel (win, 0, 0, w - 1, h - 1, 2, 1);
    CSetColor (COLOR_FLAT);
    CRectangle (win, 2, w + 2 * w / 3 + 2, w - 4, (l - 5) * pos / 65535);
    CRectangle (win, 2, w + 2 * w / 3 + 3 + l * (prop + pos) / 65535, w - 4, h - 1 - w - 2 * w / 3 - (w + 2 * w / 3 + 4 + l * (prop + pos) / 65535));

    if (flags & 32) {
	render_bevel (win, 2, 2, w - 3, w + 1, 2 - ((flags & 15) == 1), 2);
	render_bevel (win, 2, w + 2, w - 3, w + 2 * w / 3 + 1, 2 - ((flags & 15) == 2), 2);
	render_bevel (win, 2, h - 2 - w, w - 3, h - 3, 2 - ((flags & 15) == 4), 2);
	render_bevel (win, 2, h - 2 - w - 2 * w / 3, w - 3, h - 3 - w, 2 - ((flags & 15) == 5), 2);
	render_bevel (win, 2, w + 2 * w / 3 + 2 + (l - 5) * pos / 65535, w - 3, w + 2 * w / 3 + 7 + (l - 5) * (prop + pos) / 65535, 2 - ((flags & 15) == 3), 2);
    } else {
	render_bevel (win, 2, 2, w - 3, w + 1, 2, 2 | ((flags & 15) == 1));
	render_bevel (win, 2, w + 2, w - 3, w + 2 * w / 3 + 1, 2, 2 | ((flags & 15) == 2));
	render_bevel (win, 2, h - 2 - w, w - 3, h - 3, 2, 2 | ((flags & 15) == 4));
	render_bevel (win, 2, h - 2 - w - 2 * w / 3, w - 3, h - 3 - w, 2, 2 | ((flags & 15) == 5));
	if ((flags & 15) == 3) {
	    CSetColor (color_widget (5));
	    XDrawRectangle (CDisplay, win, CGC, 4, w + 2 * w / 3 + 4 + (l - 5) * pos2 / 65535, w - 10, 2 + (l - 5) * prop2 / 65535);
	}
	render_bevel (win, 2, w + 2 * w / 3 + 2 + (l - 5) * pos / 65535, w - 3, w + 2 * w / 3 + 7 + (l - 5) * (prop + pos) / 65535, 2, 2 | ((flags & 15) == 3));
    }
}

static void look_cool_render_hori_scrollbar (Window win, int x, int y, int h, int w, int pos, int prop, int flags)
{E_
    int l = h - 10 * w / 3 - 5, k;
    k = (l - 5) * pos / 65535;

    render_bevel (win, 0, 0, h - 1, w - 1, 2, 1);
    CSetColor (COLOR_FLAT);

    CRectangle (win, w + 2 * w / 3 + 2, 2, (l - 5) * pos / 65535, w - 4);
    CRectangle (win, w + 2 * w / 3 + 3 + l * (prop + pos) / 65535, 2, h - 1 - w - 2 * w / 3 - (w + 2 * w / 3 + 4 + l * (prop + pos) / 65535), w - 4);

    if (flags & 32) {
	render_bevel (win, 2, 2, w + 1, w - 3, 2 - ((flags & 15) == 1), 2);
	render_bevel (win, w + 2, 2, w + 2 * w / 3 + 1, w - 3, 2 - ((flags & 15) == 2), 2);
	render_bevel (win, h - 2 - w, 2, h - 3, w - 3, 2 - ((flags & 15) == 4), 2);
	render_bevel (win, h - 2 - w - 2 * w / 3, 2, h - 3 - w, w - 3, 2 - ((flags & 15) == 5), 2);
	render_bevel (win, w + 2 * w / 3 + 2 + k, 2, w + 2 * w / 3 + 7 + (l - 5) * (prop + pos) / 65535, w - 3, 2 - ((flags & 15) == 3), 2);
    } else {
	render_bevel (win, 2, 2, w + 1, w - 3, 2, 2 | ((flags & 15) == 1));
	render_bevel (win, w + 2, 2, w + 2 * w / 3 + 1, w - 3, 2, 2 | ((flags & 15) == 2));
	render_bevel (win, h - 2 - w, 2, h - 3, w - 3, 2, 2 | ((flags & 15) == 4));
	render_bevel (win, h - 2 - w - 2 * w / 3, 2, h - 3 - w, w - 3, 2, 2 | ((flags & 15) == 5));
	render_bevel (win, w + 2 * w / 3 + 2 + k, 2, w + 2 * w / 3 + 7 + (l - 5) * (prop + pos) / 65535, w - 3, 2, 2 | ((flags & 15) == 3));
    }
}

static void look_cool_render_scrollbar (CWidget * wdt)
{E_
    int flags = wdt->options;
    if (!wdt)
	return;
    if (wdt->numlines < 0)
	wdt->numlines = 0;
    if (wdt->firstline < 0)
	wdt->firstline = 0;
    if (wdt->firstline > 65535)
	wdt->firstline = 65535;
    if (wdt->firstline + wdt->numlines >= 65535)
	wdt->numlines = 65535 - wdt->firstline;
    if (wdt->kind == C_VERTSCROLL_WIDGET) {
	look_cool_render_vert_scrollbar (wdt->winid,
			      wdt->x, wdt->y,
			      wdt->width, wdt->height,
			      wdt->firstline, wdt->numlines, wdt->search_start, wdt->search_len, flags);
    } else
	look_cool_render_hori_scrollbar (wdt->winid,
			      wdt->x, wdt->y,
			      wdt->width, wdt->height,
			      wdt->firstline, wdt->numlines, flags);
    if (wdt->scroll_bar_extra_render)
	(*wdt->scroll_bar_extra_render) (wdt);
}

/*
   Which scrollbar button was pressed: 3 is the middle button ?
 */
static int look_cool_which_scrollbar_button (int bx, int by, CWidget * wdt)
{E_
    int w, h;
    int pos = wdt->firstline;
    int prop = wdt->numlines;
    int l;

    if (wdt->kind == C_VERTSCROLL_WIDGET) {
	w = wdt->width;
	h = wdt->height;
    } else {
	int t = bx;
	bx = by;
	by = t;
	w = wdt->height;
	h = wdt->width;
    }
    l = h - 10 * w / 3 - 5;

    if (inbounds (bx, by, 2, 2, w - 3, w + 1))
	return 1;
    if (inbounds (bx, by, 2, w + 2, w - 3, w + 2 * w / 3 + 1))
	return 2;
    if (inbounds (bx, by, 2, h - 2 - w, w - 3, h - 3))
	return 4;
    if (inbounds (bx, by, 2, h - 2 - w - 2 * w / 3, w - 3, h - 3 - w))
	return 5;
    if (inbounds (bx, by, 2, w + 2 * w / 3 + 2 + (l - 5) * pos / 65535, w - 3, w + 2 * w / 3 + 7 + (l - 5) * (prop + pos) / 65535))
	return 3;
    return 0;
}

int look_cool_scrollbar_handler (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    static int buttonypos, y, whichscrbutton = 0;	/* which of the five scroll bar buttons was pressed */
    int xevent_xbutton_y, length, width;

    if (w->kind == C_VERTSCROLL_WIDGET) {
	xevent_xbutton_y = xevent->xbutton.y;
	length = w->height;
	width = w->width;
    } else {
	xevent_xbutton_y = xevent->xbutton.x;
	length = w->width;
	width = w->height;
    }

    switch (xevent->type) {
    case LeaveNotify:
    case Expose:
	w->options = 0;
	break;
    case ButtonRepeat:
	resolve_button (xevent, cwevent);
	if (cwevent->button == Button1 || cwevent->button == Button2) {
	    int b;
	    b = (*look->which_scrollbar_button) (cwevent->x, cwevent->y, w);
	    if (b == 3 || !b)
		return 0;
	    y = w->firstline;
	    buttonypos = xevent_xbutton_y;
	    w->options = whichscrbutton = b;
	    cwevent->ident = w->ident;
	    xevent->type = cwevent->type = ButtonPress;
	}
	break;
    case ButtonPress:
	resolve_button (xevent, cwevent);
	if (cwevent->button == Button1 || cwevent->button == Button2) {
	    buttonypos = xevent_xbutton_y;
	    y = w->firstline;
	    w->options = whichscrbutton =
		(*look->which_scrollbar_button) (cwevent->x, cwevent->y, w);
	    cwevent->ident = w->ident;
	    w->search_start = w->firstline;
	    w->search_len = w->numlines;
	}
	break;
    case ButtonRelease:
	resolve_button (xevent, cwevent);
	w->options = 32 + whichscrbutton;
	if (whichscrbutton == 3) {
	    y +=
		(double) (xevent_xbutton_y - buttonypos) * (double) 65535.0 / (length -
									       10 * width / 3 - 10);
	    w->firstline = y;
	    buttonypos = xevent_xbutton_y;
	}
	break;
    case MotionNotify:
	resolve_button (xevent, cwevent);
	if (cwevent->state & (Button1Mask | Button2Mask)) {
	    w->options = whichscrbutton;
	    if (whichscrbutton == 3) {
		y +=
		    (double) (xevent_xbutton_y - buttonypos) * (double) 65535.0 / (length -
										   10 * width / 3 -
										   10);
		w->firstline = y;
		buttonypos = xevent_xbutton_y;
	    }
	} else
	    w->options =
		32 + (*look->which_scrollbar_button) (xevent->xmotion.x, xevent->xmotion.y, w);
	break;
    default:
	return 0;
    }

    if (w->firstline > 65535)
	w->firstline = 65535;
    if (cwevent->state & (Button1Mask | Button2Mask) || cwevent->type == ButtonPress
	|| cwevent->type == ButtonRelease)
	if (w->scroll_bar_link && w->vert_scrollbar)
	    (*w->scroll_bar_link) (w, w->vert_scrollbar, xevent, cwevent, whichscrbutton);

    if (xevent->type != Expose || !xevent->xexpose.count)
	(*look->render_scrollbar) (w);

    return 0;
}

static void look_cool_init_scrollbar_icons (CWidget * w)
{E_
    return;
}

static int look_cool_get_scrollbar_size (int type)
{E_
    if (type == C_HORISCROLL_WIDGET)
	return 13;
    return 20;
}

extern char *init_fg_color_red;
extern char *init_fg_color_green;
extern char *init_fg_color_blue;

static void look_cool_get_button_color (XColor * color, int i)
{E_
    double r, g, b, min_wc;

    r = 1 / atof (init_fg_color_red);
    g = 1 / atof (init_fg_color_green);
    b = 1 / atof (init_fg_color_blue);

    min_wc = min (r, min (g, b));

    color->red = (float) 65535 *my_pow ((float) i / 20, r) * my_pow (0.75, -min_wc);
    color->green = (float) 65535 *my_pow ((float) i / 20, g) * my_pow (0.75, -min_wc);
    color->blue = (float) 65535 *my_pow ((float) i / 20, b) * my_pow (0.75, -min_wc);
    color->flags = DoRed | DoBlue | DoGreen;
}

static int look_cool_get_default_interwidget_spacing (void)
{E_
    return 4;
}

int look_cool_window_handler (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    static int windowx, windowy;
    static int wx = 0, wy = 0;
    static int wwidth = 0, wheight = 0;
    static int allowwindowmove = 0;
    static int allowwindowresize = 0;

    switch (xevent->type) {
    case ClientMessage:
	if (!w->disabled)
	    cwevent->ident = w->ident;
	break;
    case Expose:
	if (!xevent->xexpose.count)
	    render_window (w);
	break;
    case ButtonRelease:
	strcpy (cwevent->ident, w->ident);
	resolve_button (xevent, cwevent);
	allowwindowmove = 0;
	allowwindowresize = 0;
	break;
    case ButtonPress:
	strcpy (cwevent->ident, w->ident);
	resolve_button (xevent, cwevent);
	if (cwevent->double_click == 1) {
	    CWidget *c = CChildFocus (w);
	    if (c)
		CFocus (c);
	}
	if (cwevent->button == Button1 && !(w->position & WINDOW_ALWAYS_LOWERED)) {
	    XRaiseWindow (CDisplay, w->winid);
	    CRaiseWindows ();
	} else if (cwevent->button == Button2 && !(w->position & WINDOW_ALWAYS_RAISED)) {
	    XLowerWindow (CDisplay, w->winid);
	    CLowerWindows ();
	}
	windowx = xevent->xbutton.x_root - w->x;
	windowy = xevent->xbutton.y_root - w->y;
	wx = xevent->xbutton.x;
	wy = xevent->xbutton.y;
	wwidth = w->width;
	wheight = w->height;
	if (wx + wy > w->width + w->height - 33 && w->position & WINDOW_RESIZABLE)
	    allowwindowresize = 1;
	else
	    allowwindowmove = 1;
	break;
    case MotionNotify:
	resolve_button (xevent, cwevent);
	if (!(w->position & WINDOW_UNMOVEABLE) && allowwindowmove
	    && (cwevent->state & (Button1Mask | Button2Mask))) {
	    w->x = xevent->xmotion.x_root - windowx;
	    w->y = xevent->xmotion.y_root - windowy;
	    if (w->x + xevent->xmotion.x < 2)
		w->x = -wx + 2;
	    if (w->y + xevent->xmotion.y < 2)
		w->y = -wy + 2;
	    XMoveWindow (CDisplay, w->winid, w->x, w->y);
	}
	if ((w->position & WINDOW_RESIZABLE) && allowwindowresize
	    && (cwevent->state & (Button1Mask | Button2Mask))) {
	    int wi, he;
	    wi = wwidth + xevent->xmotion.x_root - windowx - w->x;
	    he = wheight + xevent->xmotion.y_root - windowy - w->y;

/* this is actually for the edit windows, and needs to be generalized */
	    if (wi < w->mark1)
		wi = w->mark1;
	    if (he < w->mark2)
		he = w->mark2;

	    wi -= w->firstcolumn;
	    wi -= wi % w->resize_gran;
	    wi += w->firstcolumn;
	    he -= w->firstline;
	    he -= he % w->numlines;
	    he += w->firstline;
	    w->position &= ~WINDOW_MAXIMISED;
	    CSetSize (w, wi, he);
	}
	break;
    }
    return 0;
}

extern Pixmap Cswitchon;
extern Pixmap Cswitchoff;

static void look_cool_render_switch (CWidget * wdt)
{E_
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    int x = 0, y = 0;

    CSetColor (COLOR_FLAT);
    CRectangle (win, x+5, y+5, w - 10, h - 10);

    CSetColor (wdt->fg);
    CSetBackgroundColor (wdt->bg);
    if (wdt->options & SWITCH_PICTURE_TYPE) {
	if (wdt->keypressed)
	    XCopyPlane (CDisplay, Cswitchon, win, CGC, 0, 0,
			w, h, x, y, 1);
	else
	    XCopyPlane (CDisplay, Cswitchoff, win, CGC, 0, 0,
			w, h, x, y, 1);
    } else {
	if (wdt->keypressed)
	{
	    render_bevel (win, x + 3, y + 3, x + w - 4, y + h - 4, 2, 1);
	}else
	    render_bevel (win, x + 3, y + 3, x + w - 4, y + h - 4, 2, 0);
    }
    if (wdt->options & (BUTTON_HIGHLIGHT | BUTTON_PRESSED))
	render_rounded_bevel (win, x, y, x + w - 1, y + h - 1, 7, 1, 1);
    else
	render_rounded_bevel (win, x, y, x + w - 1, y + h - 1, 7, 1, 0);
}

extern unsigned long edit_normal_background_color;

static void look_cool_edit_render_tidbits (CWidget * wdt)
{E_
    int isfocussed;
    int w = wdt->width, h = wdt->height;
    Window win;

    win = wdt->winid;
    isfocussed = (win == CGetFocus ());
    CSetColor (COLOR_FLAT);
    if (isfocussed) {
	render_bevel (win, 0, 0, w - 1, h - 1, 3, 1);	/*most outer border bevel */
    } else {
	render_bevel (win, 2, 2, w - 3, h - 3, 1, 1);	/*border bevel */
	render_bevel (win, 0, 0, w - 1, h - 1, 2, 0);	/*most outer border bevel */
    }
    CSetColor (edit_normal_background_color);
    CLine (CWindowOf (wdt), 3, 3, 3, CHeightOf (wdt) - 4);
}

CWidget *look_cool_draw_exclam_cancel_button (char *ident, Window win, int x, int y)
{E_
    CWidget *wdt;
    wdt = CDrawPixmapButton (ident, win, x, y, PIXMAP_BUTTON_EXCLAMATION);
    return wdt;
}

CWidget *look_cool_draw_tick_cancel_button (char *ident, Window win, int x, int y)
{E_
    CWidget *wdt;
    wdt = CDrawPixmapButton (ident, win, x, y, PIXMAP_BUTTON_TICK);
    return wdt;
}

CWidget *look_cool_draw_cross_cancel_button (char *ident, Window win, int x, int y)
{E_
    CWidget *wdt;
    wdt = CDrawPixmapButton (ident, win, x, y, PIXMAP_BUTTON_CROSS);
    return wdt;
}

static void look_cool_render_fielded_textbox_tidbits (CWidget * w, int isfocussed)
{E_
    if (isfocussed) {
	render_bevel (w->winid, 0, 0, w->width - 1, w->height - 1, 3, 1);	/*most outer border bevel */
    } else {
	render_bevel (w->winid, 2, 2, w->width - 3, w->height - 3, 1, 1);	/*border bevel */
	render_bevel (w->winid, 0, 0, w->width - 1, w->height - 1, 2, 0);	/*most outer border bevel */
    }
    CSetColor (edit_normal_background_color);
    CLine (w->winid, 3, 3, 3, w->height - 4);
}

static void look_cool_render_textbox_tidbits (CWidget * w, int isfocussed)
{E_
    if (isfocussed) {
	render_bevel (w->winid, 0, 0, w->width - 1, w->height - 1, 3, 1);	/*most outer border bevel */
    } else {
	render_bevel (w->winid, 2, 2, w->width - 3, w->height - 3, 1, 1);	/*border bevel */
	render_bevel (w->winid, 0, 0, w->width - 1, w->height - 1, 2, 0);	/*most outer border bevel */
    }
}

static void look_cool_render_passwordinput_tidbits (CWidget * wdt, int isfocussed)
{E_
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    if (isfocussed) {
	render_bevel (win, 0, 0, w - 1, h - 1, 3, 1);
    } else {
	render_bevel (win, 2, 2, w - 3, h - 3, 1, 1);
	render_bevel (win, 0, 0, w - 1, h - 1, 2, 0);
    }
}

static void look_cool_render_textinput_tidbits (CWidget * wdt, int isfocussed)
{E_
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    if (isfocussed) {
	render_bevel (win, 0, 0, w - h - 1, h - 1, 3, 1);	/*most outer border bevel */
    } else {
	render_bevel (win, 2, 2, w - h - 3, h - 3, 1, 1);	/*border bevel */
	render_bevel (win, 0, 0, w - h - 1, h - 1, 2, 0);	/*most outer border bevel */
    }
    if ((wdt->options & BUTTON_PRESSED) && !(wdt->options & TEXTINPUT_NOHISTORY)) {
	CRectangle (win, w - h + 2, 2, h - 4, h - 4);
	render_bevel (win, w - h, 0, w - 1, h - 1, 2, 3);
    } else if ((wdt->options & BUTTON_HIGHLIGHT) && !(wdt->options & TEXTINPUT_NOHISTORY)) {
	CRectangle (win, w - h + 1, 1, h - 2, h - 2);
	render_bevel (win, w - h, 0, w - 1, h - 1, 1, 2);
    } else {
	CRectangle (win, w - h + 2, 2, h - 4, h - 4);
	render_bevel (win, w - h, 0, w - 1, h - 1, 2, 2);
    }
}

extern struct focus_win focus_border;

static void render_focus_border_n (Window win, int i)
{E_
    int j;
    j = (i > 3) + 1;
    if (win == focus_border.top) {
	render_bevel (win, 0, 0, focus_border.width + 2 * WIDGET_FOCUS_RING - 1, focus_border.height + 2 * WIDGET_FOCUS_RING - 1, j, 0);
	render_bevel (win, i, i, focus_border.width + 2 * WIDGET_FOCUS_RING - 1 - i, focus_border.height + 2 * WIDGET_FOCUS_RING - 1 - i, 2, 1);
    } else if (win == focus_border.bottom) {
	render_bevel (win, 0, 0 - focus_border.height, focus_border.width + 2 * WIDGET_FOCUS_RING - 1, WIDGET_FOCUS_RING - 1, j, 0);
	render_bevel (win, i, i - focus_border.height, focus_border.width + 2 * WIDGET_FOCUS_RING - 1 - i, WIDGET_FOCUS_RING - 1 - i, 2, 1);
    } else if (win == focus_border.left) {
	render_bevel (win, 0, 0 - WIDGET_FOCUS_RING, focus_border.width + 2 * WIDGET_FOCUS_RING - 1, focus_border.height + WIDGET_FOCUS_RING - 1, j, 0);
	render_bevel (win, i, i - WIDGET_FOCUS_RING, focus_border.width + 2 * WIDGET_FOCUS_RING - 1 - i, focus_border.height + WIDGET_FOCUS_RING - 1 - i, 2, 1);
    } else if (win == focus_border.right) {
	render_bevel (win, 0 + WIDGET_FOCUS_RING - focus_border.width, 0 - WIDGET_FOCUS_RING, WIDGET_FOCUS_RING - 1, focus_border.height + WIDGET_FOCUS_RING - 1, j, 0);
	render_bevel (win, i + WIDGET_FOCUS_RING - focus_border.width, i - WIDGET_FOCUS_RING, WIDGET_FOCUS_RING - 1 - i, focus_border.height + WIDGET_FOCUS_RING - 1 - i, 2, 1);
    }
}

static void look_cool_render_focus_border (Window win)
{E_
    render_focus_border_n (win, focus_border.border);
}

static int look_cool_get_extra_window_spacing (void)
{E_
    return 2;
}

static int look_cool_get_focus_ring_size (void)
{E_
    return 4;
}

static unsigned long look_cool_get_button_flat_color (void)
{E_
    return color_widget(9);
}

static int look_cool_get_window_resize_bar_thickness (void)
{E_
    return 0;
}

static int look_cool_get_switch_size (void)
{E_
    return FONT_PIX_PER_LINE + TEXT_RELIEF * 2 + 2 + 4;
}

static int look_cool_get_fielded_textbox_hscrollbar_width (void)
{E_
    return 12;
}

struct look look_cool = {
    look_cool_get_default_interwidget_spacing,
    look_cool_menu_draw,
    look_cool_get_menu_item_extents,
    look_cool_render_menu_button,
    look_cool_render_button,
    look_cool_render_bar,
    look_cool_render_raised_bevel,
    look_cool_render_sunken_bevel,
    look_cool_draw_hotkey_understroke,
    get_default_widget_font,
    look_cool_render_text,
    look_cool_render_window,
    look_cool_render_scrollbar,
    look_cool_get_scrollbar_size,
    look_cool_init_scrollbar_icons,
    look_cool_which_scrollbar_button,
    look_cool_scrollbar_handler,
    look_cool_get_button_color,
    look_cool_get_extra_window_spacing,
    look_cool_window_handler,
    look_cool_get_focus_ring_size,
    look_cool_get_button_flat_color,
    look_cool_get_window_resize_bar_thickness,
    look_cool_render_switch,
    look_cool_get_switch_size,
    look_cool_draw_browser,
    look_cool_get_file_or_dir,
    look_cool_draw_file_list,
    look_cool_redraw_file_list,
    look_cool_get_file_list_line,
    look_cool_search_replace_dialog,
    look_cool_edit_render_tidbits,
    look_cool_draw_exclam_cancel_button,
    look_cool_draw_tick_cancel_button,
    look_cool_draw_cross_cancel_button,
    look_cool_draw_tick_cancel_button,
    look_cool_render_fielded_textbox_tidbits,
    look_cool_render_textbox_tidbits,
    look_cool_get_fielded_textbox_hscrollbar_width,
    look_cool_render_textinput_tidbits,
    look_cool_render_passwordinput_tidbits,
    look_cool_render_focus_border,
};

