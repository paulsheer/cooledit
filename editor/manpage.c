/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* manpage.c - draws an interactive man page browser
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"

#include "coollocal.h"
#include "loadfile.h"
#include "manpage.h"

#include "edit.h"
#include "editcmddef.h"

#define MANPAGE_RELOAD_LAST     ((const char *) (void *) 1UL)
CWidget *CManpageDialog (Window in, int x, int y, int columns, int lines, const char *manpage);
static int current_column_width (void);

/* must be a power of 2 */
#define NUM_HISTORY 64
#define LAST_HISTORY            ((history_current - 1) & (NUM_HISTORY - 1))

static struct history {
    char *text;
    char *manpage;
    int line;
    int columns;
} history[NUM_HISTORY] =

{
    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },

    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },

    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },

    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    },
    {
	0, 0, 0, 0
    }
};

static unsigned int history_current = 0;

#define my_is_letter(ch) (isalpha (ch) || ch == '\b' || ch == '_' || ch == '-')

void check_prev_next (void)
{E_
    if (history[history_current & (NUM_HISTORY - 1)].text)
	CIdent ("mandisplayfile.next")->disabled = 0;
    else {
	CIdent ("mandisplayfile.next")->disabled = 1;
	if (CGetFocus () == CIdent ("mandisplayfile.next")->winid) {
	    CFocus (CIdent ("mandisplayfile.text"));
	    CExpose ("mandisplayfile.next");
	}
    }
    if (history[(history_current - 2) & (NUM_HISTORY - 1)].text)
	CIdent ("mandisplayfile.prev")->disabled = 0;
    else {
	CIdent ("mandisplayfile.prev")->disabled = 1;
	if (CGetFocus () == CIdent ("mandisplayfile.prev")->winid) {
	    CFocus (CIdent ("mandisplayfile.text"));
	    CExpose ("mandisplayfile.prev");
	}
    }
}

static void free_history_item (struct history *h)
{E_
    if (h->text || h->manpage) {
        assert (h->manpage);
        assert (h->text);
        free (h->text);
        h->text = NULL;
        free (h->manpage);
        h->manpage = NULL;
    }
}

static void add_to_history (char *text, const char *manpage, int columns)
{E_
    struct history *h;
    history_current &= (NUM_HISTORY - 1);
    free_history_item (&history[history_current]);
    free_history_item (&history[(history_current + 1) & (NUM_HISTORY - 1)]);
    h = &history[history_current++];
    h->text = (char *) text;
    h->manpage = (char *) strdup (manpage);
    h->line = 0;
    h->columns = columns;
}

int mansearch_callback (CWidget * w, XEvent * x, CEvent * c);
int mansearchagain_callback (CWidget * w, XEvent * x, CEvent * c);

int calc_text_pos2 (CWidget * w, long b, long *q, int l);

int manpage_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    int p, q;
    unsigned char m[128] = "";
    char *t;
    if (c->command == CK_Cancel || !strcmp (c->ident, "mandisplayfile.clear")
	|| !strcmp (c->ident, "mandisplayfile.done")) {
	CDestroyWidget ("mandisplayfile");
	return 0;
    }
    if (c->command == CK_Find) {
	mansearch_callback (w, x, c);
    }
    if (c->command == CK_Find_Again) {
	mansearchagain_callback (w, x, c);
    }
    if (c->double_click) {
	unsigned char *text;
        CStr s;
	long lq;
        s = CGetTextBoxText(w);
	text = (unsigned char *) s.data;
	q = strmovelines ((char *) text, w->current, c->yt - w->firstline, w->width);
	calc_text_pos2 (w, q, &lq, c->x - 4);
	p = q = lq;
	if (my_is_letter (text[q])) {
	    while (--q >= 0)
		if (!my_is_letter (text[q]))
		    break;
	    q++;
	    while (text[++p])
		if (!my_is_letter (text[p]))
		    break;
	    strncpy ((char *) m, (char *) text + q, min (p - q, 127));
	    m[min (p - q, 127)] = 0;
	    t = str_strip_nroff ((char *) m, 0);
	    if (*t != '-')
		CManpageDialog (0, 0, 0, 0, 0, t);
	    free (t);
	}
    }
    check_prev_next ();
    return 0;
}

void record_line (void)
{E_
    CWidget *w;
    if (!(w = CIdent ("mandisplayfile.text")))
	return;
    if (!history[LAST_HISTORY].text)
	return;
    history[LAST_HISTORY].line = w->firstline;
}

char *option_man_cmdline = MAN_CMD;

extern char **environ;

static char **set_env_var (char *new_var[], int nn)
{E_
    int exists, n, i, j, k, *l;
    char **r;
    l = (int *) alloca (nn * sizeof (int));
    for (n = 0; environ[n]; n++);
    for (k = 0; k < nn; k++)
        l[k] = strcspn (new_var[k], "=");
    r = (char **) malloc (sizeof (const char *) * (n + nn + 2));
    for (i = j = 0; j < n; j++) {
        assert (environ[j]);
        for (exists = k = 0; k < nn && !exists; k++)
            exists = !strncmp (environ[j], new_var[k], l[k]);
        if (!exists)
            r[i++] = environ[j];
    }
    for (k = 0; k < nn; k++)
        r[i++] = new_var[k];
    r[i] = NULL;
    return r;
}

static char *run_man_shell_cmd (const char *manpage, int columns)
{E_
    int man_pipe = -1;
    char *t = NULL;
    char *argv[4];
    char ev1[64];
    char ev2[64];
    char *new_env_var[2];
    char **the_env;
    pid_t the_pid;
    Window win = 0;
    CWidget *w;
    char *r;

    snprintf (ev1, sizeof (ev1), "MANWIDTH=%d", columns - 2);  /* For FreeBSD. Requires a little white-space "2". */
    snprintf (ev2, sizeof (ev2), "COLUMNS=%d", columns);
    new_env_var[0] = ev1;
    new_env_var[1] = ev2;

    the_env = set_env_var (new_env_var, 2);

    w = CIdent ("mandisplayfile");
    if (w)
        win = w->winid;

/*  See  configure.ac  where MAN_CMD is set to:  */
/*  man -a -Tutf8 -Z %m | grotty -c  */
    argv[0] = "sh";
    argv[1] = "-c";
    argv[2] = r = replace_str (option_man_cmdline, "%m", manpage);
    argv[3] = NULL;

    if (win)
        CHourGlass (win);
    CHourGlass (CFirstWindow);
    if ((the_pid = triple_pipe_open_env (0, &man_pipe, 0, 1, argv[0], argv, the_env)) <= 0) {	/* "1" is to pipe both stderr AND stdout into man_pipe */
        CErrorDialog (CFirstWindow, 20, 20, _(" Manual page "), _(" Fail trying to run man, check 'option_man_cmdline' in the file ~/.cedit/.cooledit.ini "));
        if (win)
            CUnHourGlass (win);
        CUnHourGlass (CFirstWindow);
        free (r);
        free (the_env);
        return 0;
    }
    t = read_pipe (man_pipe, 0, &the_pid);
    close (man_pipe);
    free (r);
    free (the_env);
    if (win)
        CUnHourGlass (win);
    CUnHourGlass (CFirstWindow);
    return t;
}

int manpageprev_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    record_line ();
    history_current--;
    check_prev_next ();
    CRedrawTextbox ("mandisplayfile.text", history[LAST_HISTORY].text, 1);
    CSetTextboxPos (CIdent ("mandisplayfile.text"), TEXT_SET_LINE, history[LAST_HISTORY].line);
    CFocus (CIdent ("mandisplayfile.text"));
    return 0;
}

int manpagenext_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    record_line ();
    history_current++;
    check_prev_next ();
    CRedrawTextbox ("mandisplayfile.text", history[LAST_HISTORY].text, 1);
    CSetTextboxPos (CIdent ("mandisplayfile.text"), TEXT_SET_LINE, history[LAST_HISTORY].line);
    CFocus (CIdent ("mandisplayfile.text"));
    return 0;
}

int manpagereload_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    CManpageDialog (0, 0, 0, 0, 0, NULL);
    CFocus (CIdent ("mandisplayfile.text"));
    return 0;
}

extern int replace_scanf;
extern int replace_regexp;
extern int replace_whole;
extern int replace_case;

int text_get_byte (unsigned char *text, long index)
{E_
    return text[index];
}

void Ctextboxsearch (CWidget * w, int again)
{E_
    int cancel = 0;
    static CStr old = {0, 0};
    CStr exp;

    if (!w) {
	CStr_free(&old);
	return;
    }

    if (!old.data)  /* initialize */
        old = CStr_dup("");

    if (again)
	if (!old.len)
	    return;

    exp = CStr_dupstr(old);

    if (!again) {
	cancel = edit_search_replace_dialog (w->parentid, 20, 20, &exp, 0, 0, _(" Search "), 0);
    }

    if (cancel || !exp.len)
        goto out;

    CStr_free(&old);
    old = CStr_dupstr(exp);

    {
	    int len, l;
	    char *t;
	    long search_start;
            CStr s;
            s = CGetTextBoxText(w);

/* here we run strip on everything from here
   to the end of the file then search through
   the stripped text */
	    search_start = strmovelines (s.data, w->current, 1, 32000);
	    t = str_strip_nroff (s.data + search_start, &l);
	    search_start = edit_find (0, exp, &len, l, (int (*)(void *, long)) text_get_byte, (void *) t, 0);
	    if (search_start == -3) {
		CErrorDialog (w->mainid, 20, 20, _(" Error "), _(" Invalid regular expression. "));
	    } else if (search_start >= 0) {
		l = strcountlines (t, 0, search_start, 32000) + 1 + w->firstline;
		CSetTextboxPos (w, TEXT_SET_LINE, l);
		CExpose (w->ident);
	    } else {
		CErrorDialog (w->mainid, 20, 20, _(" Search "), _(" Search string not found. "));
	    }
	    free (t);
    }

  out:
    CStr_free(&exp);
}

int mansearch_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    Ctextboxsearch (CIdent ("mandisplayfile.text"), 0);
    CFocus (CIdent ("mandisplayfile.text"));
    return 0;
}

int mansearchagain_callback (CWidget * w, XEvent * x, CEvent * c)
{E_
    Ctextboxsearch (CIdent ("mandisplayfile.text"), 1);
    CFocus (CIdent ("mandisplayfile.text"));
    return 0;
}

/* main widget: */
CWidget *man = 0;

int open_man (char *def)
{E_
    Window parent;
    CWidget *w;
    char *page;
    w = CIdent ("mandisplayfile.text");
    parent = w ? w->winid : 0;
    page = CInputDialog ("getman", parent, 0, 0, 400, def, _(" Goto Manual Page "), _(" Enter a man page to open : "));
    CFocus (CIdent ("mandisplayfile.text"));
    if (!page)
	return 1;
    if (!*page)
	return 1;
    man = CManpageDialog (0, 0, 0, START_WIDTH, START_HEIGHT, page);
    free (page);
    return 1;
}

int mangoto_callback (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    CStr s;
    s = CLastInput ("getman.inpt_dlg");
    return open_man (s.data);
}

static int current_column_width (void)
{E_
    int columns = -1;
    CWidget *w;

    if (!(w = CIdent ("mandisplayfile.text")))
        return -1;

    CPushFont ("editor", 0);
    columns = (w->width - 7) / FONT_MEAN_WIDTH;
    CPopFont ();

    return columns < 40 ? 40 : columns;
}

CWidget *CManpageDialog (Window in, int x, int y, int columns, int lines, const char *manpage)
{E_
    char *t = NULL;
    char *do_free = NULL;
    struct history *h;
    CWidget *res;
    int line = 0;
    int columns_;

    record_line ();

    h = &history[LAST_HISTORY];

    if (manpage) {
        if ((columns_ = current_column_width ()) != -1)
            columns = columns_;
        do_free = t = run_man_shell_cmd (manpage, columns);
        if (t && *t) {
            add_to_history (t, manpage, columns);
            do_free = NULL;
        }
    } else {
        h = &history[LAST_HISTORY];
	if (!h->text) {
            res = NULL;
            goto out;
        }

        assert (h->manpage);
        assert (h->text);

        if ((columns_ = current_column_width ()) == -1 || columns_ == columns) {
            line = h->line;
            manpage = h->manpage;
            t = h->text;
        } else {
            columns = columns_;
            line = h->line;
            manpage = h->manpage;
            do_free = t = run_man_shell_cmd (manpage, columns);
            if (t && *t) {
                free (h->text);
                h->text = t;
                do_free = NULL;
            }
        }
    }

    if (!t) {
	CErrorDialog (CFirstWindow, 20, 20, _(" Manual Page "), get_sys_error (_(" Error reading from pipe, check 'option_man_cmdline' in the file ~/.cedit/.cooledit.ini ")));
        res = NULL;
        goto out;
    } else if (*t) {
	if (CIdent ("mandisplayfile.text")) {
	    CRedrawTextbox ("mandisplayfile.text", t, 0);
	} else {
	    Window win;
	    CWidget *w;
	    if (in) {
		win = in;
	    } else {
		win = CDrawMainWindow ("mandisplayfile", _(" Manual Page "));
		CGetHintPos (&x, &y);
	    }
	    CPushFont ("editor", 0);
	    w = CDrawTextbox ("mandisplayfile.text", win,
			      x, y, columns * FONT_MEAN_WIDTH + 7,
			      lines * FONT_PIX_PER_LINE, line, 0, t,
			      TEXTBOX_MAN_PAGE | TEXTBOX_NO_CURSOR);
	    CPopFont ();
/* Toolhint */
	    CSetToolHint ("mandisplayfile.text", _("Double click on words to open new man pages"));
	    w->position |= POSITION_HEIGHT | POSITION_WIDTH;
	    (CIdent ("mandisplayfile.text.vsc"))->position = POSITION_HEIGHT | POSITION_RIGHT;
	    CGetHintPos (0, &y);
	    (CDrawButton ("mandisplayfile.prev", win, x, y, AUTO_WIDTH, AUTO_HEIGHT, _(" Previous ")))->position = POSITION_BOTTOM;
	    CGetHintPos (&x, 0);
	    (CDrawButton ("mandisplayfile.next", win, x, y, AUTO_WIDTH, AUTO_HEIGHT, _(" Next ")))->position = POSITION_BOTTOM;
	    CGetHintPos (&x, 0);
	    (CDrawButton ("mandisplayfile.reload", win, x, y, AUTO_WIDTH, AUTO_HEIGHT, _(" Reload ")))->position = POSITION_BOTTOM;
	    CGetHintPos (&x, 0);
	    (CDrawButton ("mandisplayfile.search", win, x, y, AUTO_WIDTH, AUTO_HEIGHT, _(" Search ")))->position = POSITION_BOTTOM;
	    CGetHintPos (&x, 0);
	    (CDrawButton ("mandisplayfile.again", win, x, y, AUTO_WIDTH, AUTO_HEIGHT, _(" Again ")))->position = POSITION_BOTTOM;
            CGetHintPos (&x, 0);
            (CDrawButton ("mandisplayfile.goto", win, x, y, AUTO_WIDTH, AUTO_HEIGHT, _(" Goto ")))->position = POSITION_BOTTOM;
	    if (!in) {
		CGetHintPos (&x, 0);
		(CDrawPixmapButton ("mandisplayfile.done", win, x, y, PIXMAP_BUTTON_CROSS))->position = POSITION_BOTTOM;
		CSetSizeHintPos ("mandisplayfile");
		CMapDialog ("mandisplayfile");
		CSetWindowResizable ("mandisplayfile", FONT_MEAN_WIDTH * 15, FONT_PIX_PER_LINE * 15, 1600, 1200);	/* minimum and maximum sizes */
	    }
	    CAddCallback ("mandisplayfile.done", manpage_callback);
	    CAddCallback ("mandisplayfile.text", manpage_callback);
	    CAddCallback ("mandisplayfile.prev", manpageprev_callback);
	    CAddCallback ("mandisplayfile.next", manpagenext_callback);
	    CAddCallback ("mandisplayfile.reload", manpagereload_callback);
	    CAddCallback ("mandisplayfile.search", mansearch_callback);
	    CAddCallback ("mandisplayfile.again", mansearchagain_callback);
            CAddCallback ("mandisplayfile.goto", mangoto_callback);
	}
	check_prev_next ();
    } else {
	CErrorDialog (CFirstWindow, 20, 20, _(" Manual page "), _(" Fail trying to popen man, check 'option_man_cmdline' in the file ~/.cedit/.cooledit.ini "));
	res = NULL;
        goto out;
    }
    res = CIdent ("mandisplayfile");
    if (res)
	goto out;
    res = CIdent ("mandisplayfile.text");

  out:
    if (do_free)
        free (do_free);
    return res;
}
