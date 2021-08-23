/* rxvtwidget.c - terminal emulator widget
   Copyright (C) 1996-2018 Paul Sheer
 */


#if 0

#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>

#include "stringtools.h"
#include "app_glob.c"
#include "edit.h"
#include "editcmddef.h"

#include "coolwidget.h"
#include "coollocal.h"
#include "mousemark.h"


CWidget *CDrawRxvt (const char *identifier, Window parent, int x, int y, char **argv)
{
    char **a;
    int i;
    XWindowAttributes wattr;
    CWidget *wdt;
    wdt = CSetupWidget (identifier, parent, x, y, 10, 10, C_RXVT_WIDGET, INPUT_KEY, COLOR_FLAT, 1);
    a = (char **) rxvt_args (argv);
    for (i = 0; a[i]; i++);
    wdt->rxvt = (void *) rxvt_allocate (wdt->winid, i, a);
    XGetWindowAttributes (CDisplay, rxvt_get_main_window (wdt->rxvt), &wattr);
    wdt->width = wattr.width;
    wdt->height = wattr.height;
    XResizeWindow (CDisplay, wdt->winid, wattr.width, wattr.height);
    rxvtlib_main_loop (wdt->rxvt);
    rxvtlib_update_screen (wdt->rxvt);
    free (a);
    set_hint_pos (x + wattr.width + WIDGET_SPACING, y + wattr.height + WIDGET_SPACING);
    rxvt_resize_window (wdt->rxvt, wattr.width, wattr.height);
    return wdt;
}

#endif

