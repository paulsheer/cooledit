/* dialog.c - draws various useful dialog boxes
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

#define MID_X 20
#define MID_Y 20

extern struct look *look;

/* Yuriy Elkin: (from mc/src/util.c) */
char *get_sys_error (const char *s)
{
    const char *error_msg;
    if (errno) {
#ifdef HAVE_STRERROR
	error_msg = _ (strerror (errno));
#else
	extern int sys_nerr;
	extern char *sys_errlist[];
	if ((0 <= errno) && (errno < sys_nerr))
	    error_msg = _ (sys_errlist[errno]);
	else
/* The returned value, 'errno' has an unknown meaning */
	    error_msg = _ ("strange errno");
#endif
	return catstrs (s, "\n [", error_msg, "] ", NULL);
    }
    return (char *) s;
}

/* error messages displayed before the main window is mapped must
   be displayed on the root window to be seen */
Window find_mapped_window (Window w)
{
    CWidget *wdt;
    if (w == CRoot)
	return CRoot;
    if (!w)
	w = CFirstWindow;
    if ((wdt = CWidgetOfWindow (w)))
	if (!wdt->mapped)
	    return CRoot;
    return w;
}

static char *replace_tabs (const char *p)
{
    char *r, *q;
    const char *t;
    int n = 0, line = 0;
    for (t = p; *t; t++) {
	if (*t == '\t')
	    n += 8;
	else
	    n++;
    }
    q = r = malloc (n + 1);
    while (*p) {
	if (*p == '\n') {
	    *q++ = '\n';
	    line = 0;
#if 1
	} else if (*p == '\t') {
	    do {
		*q++ = ' ';
		line++;
	    } while ((line % 8));
#endif
	} else if (*p < ' ') {
	    *q++ = '?';
	    line++;
	} else {
	    *q++ = *p;
	    line++;
	}
	p++;
    }
    *q = '\0';
    return r;
}

static void CErrorDialog_ (Window in, int x, int y, const char *heading, int fixed, const char *fmt, va_list pa)
{
    static int inside = 0;
    char *str, *str_raw;
    Window win;
    CWidget *t;
    CEvent cwevent;
    CState s;

    if (inside)
        return;

    inside = 1;

    CPushFont ("widget", 0);

    str_raw = vsprintf_alloc (fmt, pa);

    str = replace_tabs (str_raw);
    free (str_raw);

    if (!in) {
	x = MID_X;
	y = MID_Y;
    }
    in = find_mapped_window (in);
    CBackupState (&s);
    CDisable ("*");
    win = CDrawHeadedDialog ("_error", in, x, y, heading);
    CGetHintPos (&x, &y);
    if (fixed)
	t = CDrawTextFixed ("", win, x, y, "%s", str);
    else
	t = CDrawText ("", win, x, y, "%s", str);
    t->position = POSITION_CENTRE;
    free (str);
    CGetHintPos (0, &y);
    ((*look->draw_exclam_cancel_button) ("_clickhere", win, -50, y))->position = POSITION_CENTRE;
    CIdent ("_error")->position = WINDOW_UNMOVEABLE | WINDOW_ALWAYS_RAISED;
    CSetSizeHintPos ("_error");
    CMapDialog ("_error");
    CFocus (CIdent ("_clickhere"));
    do {
	CNextEvent (NULL, &cwevent);
	if (!CIdent ("_error"))
	    break;
    } while (strcmp (cwevent.ident, "_clickhere") && cwevent.command != CK_Cancel);

    CPopFont ();
    
    CDestroyWidget ("_error");
    CRestoreState (&s);

    inside = 0;
}

void CErrorDialog (Window in, int x, int y, const char *heading, const char *fmt, ...)
{
    va_list pa;
    va_start (pa, fmt);
    CErrorDialog_ (in, x, y, heading, 0, fmt, pa);
    va_end (pa);
}

void CErrorDialogTxt (Window in, int x, int y, const char *heading, const char *fmt, ...)
{
    va_list pa;
    va_start (pa, fmt);
    CErrorDialog_ (in, x, y, heading, 1, fmt, pa);
    va_end (pa);
}

void CMessageDialog (Window in, int x, int y, unsigned long options, const char *heading, const char *fmt,...)
{
    va_list pa;
    char *str;
    Window win;
    CEvent cwevent;
    CState s;

    CPushFont ("widget", 0);

    va_start (pa, fmt);
    str = vsprintf_alloc (fmt, pa);
    va_end (pa);

    in = find_mapped_window (in);

    CBackupState (&s);
    CDisable ("*");
    win = CDrawHeadedDialog ("_error", in, x, y, heading);
    CGetHintPos (&x, &y);
    (CDrawText ("", win, x, y, "%s", str))->options = options;
    free (str);
    CGetHintPos (0, &y);
    ((*look->draw_tick_cancel_button) ("_clickhere", win, -50, y))->position = POSITION_CENTRE;
    CCentre ("_clickhere");
    CIdent ("_error")->position = WINDOW_UNMOVEABLE | WINDOW_ALWAYS_RAISED;
    CSetSizeHintPos ("_error");
    CMapDialog ("_error");
    CFocus (CIdent ("_clickhere"));
    do {
	CNextEvent (NULL, &cwevent);
	if (!CIdent ("_error"))
	    break;
    } while (strcmp (cwevent.ident, "_clickhere") && cwevent.command != CK_Cancel && cwevent.command != CK_Enter);

    CPopFont ();

    CDestroyWidget ("_error");
    CRestoreState (&s);
}

/* draws a scrollable text box with a button to clear. Can be used to give long help messages */
void CTextboxMessageDialog (Window in, int x, int y, int columns, int lines, const char *heading, const char *text, int line)
{
    Window win;
    CEvent cwevent;
    CState s;
    int width, height;

    CPushFont ("editor", 0);
    CTextSize (&width, &height, text);
    width = min (columns * FONT_MEAN_WIDTH, width) + 1 + 6;
    height = min (lines * FONT_PIX_PER_LINE, height) + 1 + 6;
    CPopFont ();

    if (!in) {
	x = MID_X;
	y = MID_Y;
    }
    in = find_mapped_window (in);

    CBackupState (&s);
    CDisable ("*");
    win = CDrawHeadedDialog ("_error", in, x, y, heading);
    CGetHintPos (&x, &y);
    CDrawTextbox ("_textmessbox", win, x, y, width, height, line, 0, text, 0);
    CGetHintPos (0, &y);
    ((*look->draw_tick_cancel_button) ("_clickhere", win, -50, y))->position = POSITION_CENTRE;
    CCentre ("_clickhere");
    CIdent ("_error")->position = WINDOW_UNMOVEABLE | WINDOW_ALWAYS_RAISED;
    CSetSizeHintPos ("_error");
    CMapDialog ("_error");
    CFocus (CIdent ("_clickhere"));
    do {
	CNextEvent (NULL, &cwevent);
	if (!CIdent ("_error"))
	    break;
    } while (strcmp (cwevent.ident, "_clickhere") && cwevent.command != CK_Cancel && cwevent.command != CK_Enter);
    CDestroyWidget ("_error");
    CRestoreState (&s);
}

/* draws a scrollable text box with a button to clear. Can be used to give long help messages */
void CFieldedTextboxMessageDialog (Window in, int x, int y, int columns, int lines, const char *heading,
                            char **(*get_line) (void *, int, int *, int *), long options, void *data)
{
    Window win;
    CEvent cwevent;
    CState s;
    int width, height;

    CPushFont ("editor", 0);
    width = columns * FONT_MEAN_WIDTH + 1 + 6;
    height = lines * FONT_PIX_PER_LINE + 1 + 6;
    CPopFont ();

    if (!in) {
	x = MID_X;
	y = MID_Y;
    }
    in = find_mapped_window (in);

    CBackupState (&s);
    CDisable ("*");
    win = CDrawHeadedDialog ("_error", in, x, y, heading);
    CGetHintPos (&x, &y);
    CDrawFieldedTextbox ("_textmessbox", win, x, y, width, height, 0, 0, get_line, options, data);
    CGetHintPos (0, &y);
    ((*look->draw_tick_cancel_button) ("_clickhere", win, -50, y))->position = POSITION_CENTRE;
    CCentre ("_clickhere");
    CIdent ("_error")->position = WINDOW_UNMOVEABLE | WINDOW_ALWAYS_RAISED;
    CSetSizeHintPos ("_error");
    CMapDialog ("_error");
    CFocus (CIdent ("_clickhere"));
    do {
	CNextEvent (NULL, &cwevent);
	if (!CIdent ("_error"))
	    break;
    } while (strcmp (cwevent.ident, "_clickhere") && cwevent.command != CK_Cancel && cwevent.command != CK_Enter);
    CDestroyWidget ("_error");
    CRestoreState (&s);
}


/*
   Draws a scrollable text box with a button to clear.
   the text box contains lines of text gotten from get_line().
   Returns the number of the line the user double clicked on,
   or pressed enter or space on. get_line() must return a null
   terminated string without newlines. The string may exist
   statically within the getline() function and be overwritten
   with each new call to getline().
   Returns -1 on cancel.
   With heading = 0, this behaves like trivialselection below.
 */
int CListboxDialog (Window in, int x, int y, int columns, int lines,
     const char *heading, int start_line, int cursor_line, int num_lines,
		    char *(*get_line) (void *data, int line), void *data)
{
    Window win;
    CEvent cwevent;
    CState s;
    int width, height, len, i;
    char *text, *p;

    CPushFont ("editor", 0);
    width = columns * FONT_MEAN_WIDTH + 1 + 6;
    height = lines * FONT_PIX_PER_LINE + 1 + 6;
    CPopFont ();

    if (!in) {
	x = MID_X;
	y = MID_Y;
    }
    in = find_mapped_window (in);
    CBackupState (&s);
    CDisable ("*");

    len = 0;
    for (i = 0; i < num_lines; i++)
	len += strlen ((*get_line) (data, i)) + 1;
    p = text = CMalloc (len + 2);
    p[0] = '\0';
    for (i = 0; i < num_lines; i++) {
        char *q;
        q = (*get_line) (data, i);
	strcpy (p, q);
	p += strlen (p);
        if (i < num_lines - 1)
	    *p++ = '\n';
        *p = '\0';
    }
    i = -1;
    if (heading)
	win = CDrawHeadedDialog ("_error", in, x, y, heading);
    else
	win = CDrawDialog ("_error", in, x, y);
    CGetHintPos (&x, &y);
    (CDrawTextbox ("_textmessbox", win, x, y, width, height, start_line, 0, text, TEXTBOX_MAN_PAGE))->cursor = cursor_line;
    CGetHintPos (0, &y);
    if (heading) {
	((*look->draw_cross_cancel_button) ("_clickhere", win, -50, y))->position = POSITION_CENTRE;
	CCentre ("_clickhere");
    }
    CIdent ("_error")->position = WINDOW_ALWAYS_RAISED;
    CSetSizeHintPos ("_error");
    CMapDialog ("_error");
    CFocus (CIdent ("_textmessbox"));
    do {
	CNextEvent (NULL, &cwevent);
	if (heading) {
	    if (!strcmp (cwevent.ident, "_clickhere"))
		break;
	} else {
	    if (cwevent.key == XK_Tab || cwevent.key == XK_ISO_Left_Tab)
		break;
	}
	if (!strcmp (cwevent.ident, "_textmessbox"))
	    if (cwevent.key == XK_space || cwevent.command == CK_Enter ||
		cwevent.double_click) {
		i = (CIdent ("_textmessbox"))->cursor;
		break;
	    }
	if (!CIdent ("_error"))
	    break;
    } while (cwevent.command != CK_Cancel);
    CDestroyWidget ("_error");
    CRestoreState (&s);
    free (text);
    return i;
}


/*
   Draws a scrollable text box. Returns the line you pressed enter on
   or double-clicked on. Result must not be free'd. Result must be
   copied ASAP. Returns 0 if cancelled or clicked elsewhere.
   Result must be less than 1024 bytes long.
 */
char *CTrivialSelectionDialog (Window in, int x, int y, int columns, int lines, const char *text, int line, int cursor_line)
{
    Window win;
    CEvent cwevent;
    XEvent xevent;
    CState s;
    int width, height;
    char *p = 0;
    CWidget *w;

    memset (&xevent, 0, sizeof (xevent));

    CPushFont ("editor", 0);
    width = columns * FONT_MEAN_WIDTH + 1 + 6;
    height = lines * FONT_PIX_PER_LINE + 1 + 6;
    CPopFont ();

    CBackupState (&s);
    CDisable ("*");
    win = CDrawDialog ("_select", in, x, y);
    CGetHintPos (&x, &y);
    w = CDrawTextbox ("_textmessbox", win, x, y, width, height, line, 0, text, 0);
    w->cursor = cursor_line;
    CGetHintPos (0, &y);
    CIdent ("_select")->position = WINDOW_UNMOVEABLE | WINDOW_ALWAYS_RAISED;
    CSetSizeHintPos ("_select");
    CMapDialog ("_select");
    CFocus (CIdent ("_textmessbox"));
    do {
	CNextEvent (&xevent, &cwevent);
	if (xevent.xany.window == w->winid) {
	    if (!strcmp (cwevent.ident, "_textmessbox") && (cwevent.command == CK_Enter || cwevent.double_click)) {
		p = CGetTextBoxLine (w, w->cursor);
		break;
	    }
	} else if (xevent.xany.type == ButtonPress
		   && cwevent.kind != C_VERTSCROLL_WIDGET
		   && cwevent.kind != C_HORISCROLL_WIDGET
		   && cwevent.kind != C_WINDOW_WIDGET) {
	    CSendEvent (&xevent);	/* resend this to get processed, here the widget is disabled */
	    break;
	}
	if (!CIdent ("_select"))
	    break;
    } while (cwevent.command != CK_Cancel && cwevent.key != XK_KP_Tab && cwevent.key != XK_Tab);
    CDestroyWidget ("_select");
    CRestoreState (&s);
    return p;
}


void CFatalErrorDialog (int x, int y, const char *fmt,...)
{
    va_list pa;
    char *str;
    Window win;
    CEvent cwevent;
    CState s;
    Window in;

    va_start (pa, fmt);
    str = vsprintf_alloc (fmt, pa);
    va_end (pa);

    fprintf (stderr, "%s: %s\n", CAppName, str);
    in = find_mapped_window (0);

    if (CDisplay) {
	CBackupState (&s);
	CDisable ("*");
	win = CDrawHeadedDialog ("fatalerror", in, x, y," Fatal Error ");
	CGetHintPos (&x, &y);
	CDrawText ("fatalerror.text", win, x, y, "%s", str);
	CCentre ("fatalerror.text");
	CGetHintPos (0, &y);
	((*look->draw_cross_cancel_button) ("clickhere", win, -50, y))->position = POSITION_CENTRE;
	CCentre ("clickhere");
	CIdent ("fatalerror")->position = WINDOW_UNMOVEABLE | WINDOW_ALWAYS_RAISED;
	CSetSizeHintPos ("fatalerror");
	CMapDialog ("fatalerror");
	CFocus (CIdent ("clickhere"));
	do {
	    CNextEvent (NULL, &cwevent);
	    if (!CIdent ("fatalerror"))
		abort ();
	} while (strcmp (cwevent.ident, "clickhere"));
    }
    abort ();
}



/* returns a raw XK_key sym, or 0 on cancel */
XEvent *CRawkeyQuery (Window in, int x, int y, const char *heading, const char *fmt,...)
{
    va_list pa;
    char *str;
    XEvent *p = 0;
    Window win;
    CEvent cwevent;
    static XEvent xevent;
    CState s;

    va_start (pa, fmt);
    str = vsprintf_alloc (fmt, pa);
    va_end (pa);

    if (!in) {
	x = MID_X;
	y = MID_Y;
    }
    in = find_mapped_window (in);

    CBackupState (&s);
    CDisable ("*");
    win = CDrawHeadedDialog ("_rawkeydlg", in, x, y, heading);
    CGetHintPos (&x, &y);
    CDrawText ("_rawkeydlg.text", win, x, y, "%s", str);
    CGetHintPos (&x, 0);
    free (str);
    CDrawTextInputP ("_rawkeydlg.input", win, x, y, (FONT_MEAN_WIDTH) * 6, AUTO_HEIGHT, 256, "");
    CGetHintPos (0, &y);
    ((*look->draw_cross_cancel_button) ("_rawkeydlg.crosshere", win, -50, y))->position = POSITION_CENTRE;
    CCentre ("_rawkeydlg.crosshere");
    CSetSizeHintPos ("_rawkeydlg");
    CMapDialog ("_rawkeydlg");
    CFocus (CIdent ("_rawkeydlg.input"));
    CIdent ("_rawkeydlg")->position = WINDOW_ALWAYS_RAISED;
/* handler : */
    do {
	CNextEvent (&xevent, &cwevent);
	if (!CIdent ("_rawkeydlg"))
	    break;
	if (cwevent.command == CK_Cancel || !strcmp (cwevent.ident, "_rawkeydlg.crosshere"))
	    break;
	if (xevent.type == KeyPress) {
	    KeySym k;
	    k = CKeySym (&xevent);
	    if (k && !mod_type_key (k))
		p = &xevent;
	}
    } while (!p);

    CDestroyWidget ("_rawkeydlg");
    CRestoreState (&s);
    return p;
}

/*
   def is the default string in the textinput widget.
   Result must be free'd. Returns 0 on cancel.
 */
char *CInputDialog (const char *ident, Window in, int x, int y, int min_width, const char *def, const char *heading, const char *fmt,...)
{
    va_list pa;
    char *str, *p = 0;
    int w, h;
    Window win;
    CEvent cwevent;
    CState s;
    char inp_name[256];
    int browse = 0, xf, yf;

    min_width &= ~INPUT_DIALOG_BROWSE_MASK;
    browse = min_width & INPUT_DIALOG_BROWSE_MASK;

    va_start (pa, fmt);
    str = vsprintf_alloc (fmt, pa);
    va_end (pa);

    if (!in) {
	x = MID_X;
	y = MID_Y;
    }
    xf = x + 20;
    yf = y + 20;
    in = find_mapped_window (in);
    CTextSize (&w, &h, str);
    w = max (max (w, min_width), 130);
    CBackupState (&s);
    CDisable ("*");
    win = CDrawHeadedDialog ("_inputdialog", in, x, y, heading);
    CGetHintPos (&x, &y);
    CDrawText ("", win, x, y, "%s", str);
    CGetHintPos (0, &y);
    free (str);
    strcpy (inp_name, ident);
    inp_name[20] = '\0';
    strcat (inp_name, ".inpt_dlg");
    CDrawTextInputP (inp_name, win, x, y, w, AUTO_HEIGHT, 256, def);
    if (browse) {
	CGetHintPos (&x, 0);
	w += (CDrawButton ("_inputdialog.browse", win, x, y, AUTO_SIZE, _ (" Browse... ")))->width + WIDGET_SPACING;
    }
    CGetHintPos (0, &y);
    (*look->draw_tick_ok_button) ("_inputdialog.clickhere", win, (w + 16) / 4 - 22, y);
    (*look->draw_cross_cancel_button) ("_inputdialog.crosshere", win, 3 * (w + 16) / 4 - 22, y);
    CSetSizeHintPos ("_inputdialog");
    CMapDialog ("_inputdialog");
    CFocus (CIdent (inp_name));
    CIdent ("_inputdialog")->position = WINDOW_ALWAYS_RAISED;
/* handler : */
    do {
	CNextEvent (NULL, &cwevent);
	if (cwevent.command == CK_Cancel || !strcmp (cwevent.ident, "_inputdialog.crosshere"))
	    goto fin;
	if (cwevent.command == CK_Enter)
	    break;
	if (!strcmp (cwevent.ident, "_inputdialog.browse")) {
	    char *f = 0;
/* dialog title */
	    switch (browse) {
	    case INPUT_DIALOG_BROWSE_DIR:
		f = CGetDirectory (in, xf, yf, current_dir, "", _ (" Browse "), NULL);
		break;
	    case INPUT_DIALOG_BROWSE_SAVE:
		f = CGetSaveFile (in, xf, yf, current_dir, "", _ (" Browse "), NULL);
		break;
	    case INPUT_DIALOG_BROWSE_LOAD:
		f = CGetLoadFile (in, xf, yf, current_dir, "", _ (" Browse "), NULL);
		break;
	    }
	    if (f && *f) {
                CWidget *wdt;
                wdt = CIdent (inp_name);
                CStr_free(&wdt->text);
                wdt->text = CStr_dup(f);
                free(f);
                wdt->cursor = wdt->text.len;
                CExpose (inp_name);
            }
	    CFocus (CIdent (inp_name));
	}
	if (!CIdent ("_inputdialog"))
	    goto fin;
    } while (strcmp (cwevent.ident, "_inputdialog.clickhere"));
    p = (char *) strdup (CIdent (inp_name)->text.data);

  fin:
    CDestroyWidget ("_inputdialog");
    CRestoreState (&s);
    return p;
}

static char *id[32] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void free_last_query_buttons (void)
{
    int i;
    for (i = 0; i < 32; i++)
	if (id[i]) {
	    free (id[i]);
	    id[i] = 0;
	}
}

/* returns -1 on widget-destroyed-without-a-button-pressed or cancel (i.e. Esc) */
int CQueryDialog (Window in, int x, int y, const char *heading, const char *descr, const char *first,...)
{
    va_list pa;
    char *textbox_text = 0;
    int i, buttons = 0, r = -1;
    Window win;
    CEvent cwevent;
    CState s;
    char *b[32];

    free_last_query_buttons ();
    va_start (pa, first);
    if (first == CQUERYDIALOG_ADD_TEXTBOX) {
        textbox_text = va_arg (pa, char *);
    } else {
        if ((b[buttons] = space_string (first)))
            buttons++;
    }
    while ((b[buttons] = space_string (va_arg (pa, char *))))
	 buttons++;
    va_end (pa);
    if (!buttons)
	return -1;

    if (!in) {
	x = MID_X;
	y = MID_Y;
    }
    in = find_mapped_window (in);
    CBackupState (&s);
    CDisable ("*");
    win = CDrawHeadedDialog ("_querydialog", in, x, y, heading);
    CGetHintPos (&x, &y);
    CDrawText ("_querydialog.text", win, x, y, "%s", descr);
    CGetHintPos (0, &y);
    if (textbox_text) {
        CPushFont ("editor", 0);
        CDrawTextbox ("_querydialog.tbox", win, x, y, FONT_MEAN_WIDTH * 60 + 30, (FONT_HEIGHT + TEXT_RELIEF * 2 + 2), 0, 0, textbox_text, TEXTBOX_MAN_PAGE);
        CPopFont ();
        CGetHintPos (0, &y);
    }
    for (i = 0; i < buttons; i++) {
	CDrawButton (id[i] = sprintf_alloc ("_query.%.20s", b[i]),
		     win, x, y, AUTO_WIDTH, AUTO_HEIGHT, b[i]);
	CGetHintPos (&x, 0);
    }

    CSetSizeHintPos ("_querydialog");
    CMapDialog ("_querydialog");
    CFocus (CIdent (catstrs ("_query.", b[0], NULL)));
    CIdent ("_querydialog")->position = WINDOW_ALWAYS_RAISED;
    for (; r < 0;) {
	CNextEvent (NULL, &cwevent);
	if (!CIdent ("_querydialog"))
	    break;
	if (!cwevent.handled && cwevent.command == CK_Cancel)
	    break;
	for (i = 0; i < buttons; i++) {
	    if (!strcmp (cwevent.ident, id[i])) {
		r = i;
		break;
	    }
	}
    }

    for (i = 0; i < buttons; i++)
	free (b[i]);

    CDestroyWidget ("_querydialog");
    CRestoreState (&s);
    return r;
}

