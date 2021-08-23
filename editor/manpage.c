/* manpage.c - draws an interactive man page browser
   Copyright (C) 1996-2018 Paul Sheer
 */


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

#include "edit.h"
#include "editcmddef.h"

CWidget *CManpageDialog (Window in, int x, int y, int columns, int lines, const char *manpage);

/* must be a power of 2 */
#define NUM_HISTORY 16
static struct history {
    char *text;
    int line;
} history[NUM_HISTORY] =

{
    {
	0, 0
    },
    {
	0, 0
    },
    {
	0, 0
    },
    {
	0, 0
    },

    {
	0, 0
    },
    {
	0, 0
    },
    {
	0, 0
    },
    {
	0, 0
    },

    {
	0, 0
    },
    {
	0, 0
    },
    {
	0, 0
    },
    {
	0, 0
    },

    {
	0, 0
    },
    {
	0, 0
    },
    {
	0, 0
    },
    {
	0, 0
    }
};

static unsigned int history_current = 0;

#define my_is_letter(ch) (isalpha (ch) || ch == '\b' || ch == '_' || ch == '-')

void check_prev_next (void)
{
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

void add_to_history (char *t)
{
    char **h;
    history_current &= (NUM_HISTORY - 1);
    if (history[history_current].text)
	free (history[history_current].text);
    h = &(history[(history_current + 1) & (NUM_HISTORY - 1)].text);
    if (*h) {
	free (*h);
	*h = 0;
    }
    history[history_current++].text = (char *) strdup (t);
}

int mansearch_callback (CWidget * w, XEvent * x, CEvent * c);
int mansearchagain_callback (CWidget * w, XEvent * x, CEvent * c);

int manpageclear_callback (CWidget * w, XEvent * x, CEvent * c)
{
    int i;
    CDestroyWidget ("mandisplayfile");
    for (i = 0; i < NUM_HISTORY; i++) {
	if (history[i].text) {
	    free (history[i].text);
	    history[i].text = 0;
	    history[i].line = 0;
	}
    }
    history_current = 0;
    return 0;
}

int calc_text_pos2 (CWidget * w, long b, long *q, int l);

int manpage_callback (CWidget * w, XEvent * x, CEvent * c)
{
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
{
    CWidget *w;
    if (!(w = CIdent ("mandisplayfile.text")))
	return;
    if (!history[(history_current - 1) & (NUM_HISTORY - 1)].text)
	return;
    history[(history_current - 1) & (NUM_HISTORY - 1)].line = w->firstline;
}

int manpageprev_callback (CWidget * w, XEvent * x, CEvent * c)
{
    record_line ();
    history_current--;
    check_prev_next ();
    CRedrawTextbox ("mandisplayfile.text", history[(history_current - 1) & (NUM_HISTORY - 1)].text, 1);
    CSetTextboxPos (CIdent ("mandisplayfile.text"), TEXT_SET_LINE, history[(history_current - 1) & (NUM_HISTORY - 1)].line);
    CFocus (CIdent ("mandisplayfile.text"));
    return 0;
}

int manpagenext_callback (CWidget * w, XEvent * x, CEvent * c)
{
    record_line ();
    history_current++;
    check_prev_next ();
    CRedrawTextbox ("mandisplayfile.text", history[(history_current - 1) & (NUM_HISTORY - 1)].text, 1);
    CSetTextboxPos (CIdent ("mandisplayfile.text"), TEXT_SET_LINE, history[(history_current - 1) & (NUM_HISTORY - 1)].line);
    CFocus (CIdent ("mandisplayfile.text"));
    return 0;
}

extern int replace_scanf;
extern int replace_regexp;
extern int replace_whole;
extern int replace_case;

int text_get_byte (unsigned char *text, long index)
{
    return text[index];
}

void Ctextboxsearch (CWidget * w, int again)
{
    int cancel = 0;
    CStr old = {0, 0};
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
{
    Ctextboxsearch (CIdent ("mandisplayfile.text"), 0);
    CFocus (CIdent ("mandisplayfile.text"));
    return 0;
}

int mansearchagain_callback (CWidget * w, XEvent * x, CEvent * c)
{
    Ctextboxsearch (CIdent ("mandisplayfile.text"), 1);
    CFocus (CIdent ("mandisplayfile.text"));
    return 0;
}

char *option_man_cmdline = MAN_CMD;

CWidget *CManpageDialog (Window in, int x, int y, int columns, int lines, const char *manpage)
{
    CWidget *res;
    char *t;
    char *argv[] =
    {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    int i = 0, man_pipe, fre = 1, line = 0;

    record_line ();

    if (manpage) {
        pid_t the_pid;
	Window win = 0;
	CWidget *w;
	char *p, *q, *r;
	w = CIdent ("mandisplayfile");
	if (w)
	    win = w->winid;
	i = 0;
	p = q = (char *) strdup (option_man_cmdline);
	r = p;
	for (i = 0; i < 32; i++) {
	    q = strchr (p, ' ');
	    if (strcmp (p, "%m"))
		argv[i] = p;
	    else
		argv[i] = (char *) manpage;
	    if (!q)
		break;
	    *q = 0;
	    p = q + 1;
	}
	if (win)
	    CHourGlass (win);
	CHourGlass (CFirstWindow);
	if ((the_pid = triple_pipe_open (0, &man_pipe, 0, 1, argv[0], argv)) <= 0) {	/* "1" is to pipe both stderr AND stdout into man_pipe */
	    CErrorDialog (CFirstWindow, 20, 20, _(" Manual page "), _(" Fail trying to run man, check 'option_man_cmdline' in the file ~/.cedit/.cooledit.ini "));
	    if (win)
		CUnHourGlass (win);
	    free (r);
	    return 0;
	}
	free (r);
	t = read_pipe (man_pipe, 0, &the_pid);
	close (man_pipe);
	if (win)
	    CUnHourGlass (win);
	CUnHourGlass (CFirstWindow);
    } else {
	if (!(t = history[(history_current - 1) & (NUM_HISTORY - 1)].text))
	    return 0;
	line = history[(history_current - 1) & (NUM_HISTORY - 1)].line;
	fre = 0;
    }

    if (!t) {
	CErrorDialog (CFirstWindow, 20, 20, _(" Manual Page "), get_sys_error (_(" Error reading from pipe, check 'option_man_cmdline' in the file ~/.cedit/.cooledit.ini ")));
	return 0;
    } else if (*t) {
	if (fre)
	    add_to_history (t);
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
	    (CDrawButton ("mandisplayfile.search", win, x, y, AUTO_WIDTH, AUTO_HEIGHT, _(" Search ")))->position = POSITION_BOTTOM;
	    CGetHintPos (&x, 0);
	    (CDrawButton ("mandisplayfile.again", win, x, y, AUTO_WIDTH, AUTO_HEIGHT, _(" Again ")))->position = POSITION_BOTTOM;
	    if (!in) {
		CGetHintPos (&x, 0);
		(CDrawPixmapButton ("mandisplayfile.done", win, x, y, PIXMAP_BUTTON_TICK))->position = POSITION_BOTTOM;
		CGetHintPos (&x, 0);
		(CDrawPixmapButton ("mandisplayfile.clear", win, x, y, PIXMAP_BUTTON_CROSS))->position = POSITION_BOTTOM;
		CSetSizeHintPos ("mandisplayfile");
		CMapDialog ("mandisplayfile");
		CSetWindowResizable ("mandisplayfile", FONT_MEAN_WIDTH * 15, FONT_PIX_PER_LINE * 15, 1600, 1200);	/* minimum and maximum sizes */
	    }
	    CAddCallback ("mandisplayfile.done", manpage_callback);
	    CAddCallback ("mandisplayfile.clear", manpageclear_callback);
	    CAddCallback ("mandisplayfile.text", manpage_callback);
	    CAddCallback ("mandisplayfile.next", manpagenext_callback);
	    CAddCallback ("mandisplayfile.prev", manpageprev_callback);
	    CAddCallback ("mandisplayfile.search", mansearch_callback);
	    CAddCallback ("mandisplayfile.again", mansearchagain_callback);
	}
	check_prev_next ();
	if (t && fre)
	    free (t);
    } else {
	CErrorDialog (CFirstWindow, 20, 20, _(" Manual page "), _(" Fail trying to popen man, check 'option_man_cmdline' in the file ~/.cedit/.cooledit.ini "));
	return 0;
    }
    res = CIdent ("mandisplayfile");
    if (res)
	return res;
    return CIdent ("mandisplayfile.text");
}
