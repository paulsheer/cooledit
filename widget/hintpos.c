/* hintpos.c - routines for easy positioning of widgets
   Copyright (C) 1996-2018 Paul Sheer
 */


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

void reset_hint_pos (int x, int y)
{
    hint_pos_x = x;
    hint_pos_y = y;
    hint_pos_max_x = x;
    hint_pos_max_y = y;
}

void  set_hint_pos (int x, int y)
{
    hint_pos_x = x;
    hint_pos_y = y;
    hint_pos_max_x = max(x, hint_pos_max_x);
    hint_pos_max_y = max(y, hint_pos_max_y);
}

void CGetHintPos (int *x, int *y)
{
    if (x)
	*x = hint_pos_x;
    if (y)
	*y = hint_pos_y;
}

void get_hint_limits (int *max_x, int *max_y)
{
    if (max_x)
	*max_x = hint_pos_max_x;
    if (max_y)
	*max_y = hint_pos_max_y;
}



