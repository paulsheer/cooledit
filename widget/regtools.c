/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* regtools.c - regexp front end convenience functions
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "stringtools.h"
#include "fnmatch.h"

/* 1 if string matches
   0 if string doesn't match
   -1 if error in pattern */

int glob_match (char *pattern, char *string)
{
    int r = my_fnmatch (pattern, string, 0);
    if (r == 0)
	return 1;
    if (r == FNM_NOMATCH)
	return 0;
    return -1;
}

