/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* hintpos.c - routines for easy positioning of widgets
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"
#include "coollocal.h"


static int hint_pos_x = 0;
static int hint_pos_y = 0;
static int hint_pos_max_x = 0;
static int hint_pos_max_y = 0;

struct hint_pos {
    int x;
    int y;
    int max_x;
    int max_y;
};

void reset_hint_pos (int x, int y)
{E_
    hint_pos_x = x;
    hint_pos_y = y;
    hint_pos_max_x = x;
    hint_pos_max_y = y;
}

void  set_hint_pos (int x, int y)
{E_
    hint_pos_x = x;
    hint_pos_y = y;
    hint_pos_max_x = max(x, hint_pos_max_x);
    hint_pos_max_y = max(y, hint_pos_max_y);
}

struct hint_pos *CPushHintPos (void)
{E_
    struct hint_pos *r;
    r = (struct hint_pos *) CMalloc (sizeof (struct hint_pos));
    memset (r, '\0', sizeof (struct hint_pos));
    r->x = hint_pos_x;
    r->y = hint_pos_y;
    r->max_x = hint_pos_max_x;
    r->max_y = hint_pos_max_y;
    return r;
}

void CPopHintPos (struct hint_pos *r)
{E_
    hint_pos_x = r->x;
    hint_pos_y = r->y;
    hint_pos_max_x = r->max_x;
    hint_pos_max_y = r->max_y;
    free (r);
}

void CGetHintPos (int *x, int *y)
{E_
    if (x)
	*x = hint_pos_x;
    if (y)
	*y = hint_pos_y;
}

void get_hint_limits (int *max_x, int *max_y)
{E_
    if (max_x)
	*max_x = hint_pos_max_x;
    if (max_y)
	*max_y = hint_pos_max_y;
}



