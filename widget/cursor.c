/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* cursor.c - initialise mouse cursors
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pool.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include "coolwidget.h"

#include "cursor/hour.xbm"
#include "cursor/hour_mask.xbm"
#include "cursor/left_ptr.xbm"
#include "cursor/left_ptr_mask.xbm"
#include "cursor/hand.xbm"
#include "cursor/hand_mask.xbm"

#include "app_glob.c"

typedef struct {
    int width, height;
    unsigned char *image_data, *mask_data;
    int x, y;
    Pixmap image, mask;
    Cursor cursor;
} CursorData;

static CursorData cool_cursor[] =
{
    {hour_width, hour_height, hour_bits, hour_mask_bits,
     hour_x_hot, hour_y_hot, 0, 0, 0},
    {left_ptr_width, left_ptr_height, left_ptr_bits, left_ptr_mask_bits,
     left_ptr_x_hot, left_ptr_y_hot, 0, 0, 0},
    {hand_width, hand_height, hand_bits, hand_mask_bits,
     hand_x_hot, hand_y_hot, 0, 0, 0}
};

static int num_cursors = sizeof (cool_cursor) / sizeof (CursorData);

void CHourGlass (Window win)
{E_
    XDefineCursor (CDisplay, win, cool_cursor[CURSOR_HOUR].cursor);
    XSync (CDisplay, 0);
}

void CUnHourGlass (Window win)
{E_
    XUndefineCursor (CDisplay, win);
    XSync (CDisplay, 0);
}

void edit_tri_cursor (Window win)
{E_
    XDefineCursor (CDisplay, win, cool_cursor[CURSOR_LEFT].cursor);
    XSync (CDisplay, 0);
}

void menu_hand_cursor (Window win)
{E_
    XDefineCursor (CDisplay, win, cool_cursor[CURSOR_MENU].cursor);
    XSync (CDisplay, 0);
}

void init_cursors (void)
{E_
    int screen, i;
    Colormap colormap;
    Window root;
    XColor black, white;

    screen = DefaultScreen (CDisplay);
    colormap = DefaultColormap (CDisplay, screen);
    root = CRoot;

    black.pixel = BlackPixel (CDisplay, screen);
    white.pixel = WhitePixel (CDisplay, screen);
    XQueryColor (CDisplay, colormap, &black);
    XQueryColor (CDisplay, colormap, &white);

    for (i = 0; i < num_cursors; i++) {
	cool_cursor[i].image =
	    XCreateBitmapFromData (CDisplay, root,
				   (char *) cool_cursor[i].image_data,
				   cool_cursor[i].width,
				   cool_cursor[i].height);
	cool_cursor[i].mask =
	    XCreateBitmapFromData (CDisplay, root,
				   (char *) cool_cursor[i].mask_data,
				   cool_cursor[i].width,
				   cool_cursor[i].height);
	cool_cursor[i].cursor =
	    XCreatePixmapCursor (CDisplay, cool_cursor[i].image,
				 cool_cursor[i].mask,
				 &black, &white,
				 cool_cursor[i].x,
				 cool_cursor[i].y);
    }
}

Cursor CGetCursorID (int i)
{E_
    if (i < 0 || i >= num_cursors) {
	CErrorDialog (CRoot, 20, 20, "Error", "\nCGetCursorID called with parameter out of range.\n");
	return 0;
    }
    return cool_cursor[i].cursor;
}




