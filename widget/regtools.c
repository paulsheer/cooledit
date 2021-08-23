/* regtools.c - regexp front end convenience functions
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "stringtools.h"
#include "regex.h"


/* 1 if string matches
   0 if string doesn't match
   -1 if error in pattern */

char *regtools_old_pattern = 0;

int regexp_match (char *pattern, char *string, int match_type)
{
    static regex_t r;
    static int old_type;
    int    rval;

    if (!regtools_old_pattern || strcmp (regtools_old_pattern, pattern) || old_type != match_type){
	if (regtools_old_pattern){
	    regfree (&r);
	    free (regtools_old_pattern);
	    regtools_old_pattern = 0;
	}
	pattern = convert_pattern (pattern, match_type, 0);
	if (regcomp (&r, pattern, REG_EXTENDED|REG_NOSUB))
	    return -1;
	regtools_old_pattern = (char *) malloc (strlen (pattern) + 1);
	strcpy (regtools_old_pattern, pattern);
	old_type = match_type;
    }
    rval = !regexec (&r, string, 0, NULL, 0);
    return rval;
}

int easy_patterns = 1;

static char *maybe_start_group (char *d, int do_group, int *was_wildcard)
{
    if (!do_group)
	return d;
    if (*was_wildcard)
	return d;
    *was_wildcard = 1;
    *d++ = '\\';
    *d++ = '(';
    return d;
}

static char *maybe_end_group (char *d, int do_group, int *was_wildcard)
{
    if (!do_group)
	return d;
    if (!*was_wildcard)
	return d;
    *was_wildcard = 0;
    *d++ = '\\';
    *d++ = ')';
    return d;
}

/* If shell patterns are on converts a shell pattern to a regular
   expression. Called by regexp_match and mask_rename. */
/* Shouldn't we support [a-fw] type wildcards as well ?? */
char *convert_pattern (char *pattern, int match_type, int do_group)
{
    char *s, *d;
    static char new_pattern [100];
    int was_wildcard = 0;

    if (easy_patterns){
	d = new_pattern;
	if (match_type == match_file)
	    *d++ = '^';
	for (s = pattern; *s; s++, d++){
	    switch (*s){
	    case '*':
		d = maybe_start_group (d, do_group, &was_wildcard);
		*d++ = '.';
		*d   = '*';
		break;
		
	    case '?':
		d = maybe_start_group (d, do_group, &was_wildcard);
		*d = '.';
		break;
		
	    case '.':
		d = maybe_end_group (d, do_group, &was_wildcard);
		*d++ = '\\';
		*d   = '.';
		break;

	    default:
		d = maybe_end_group (d, do_group, &was_wildcard);
		*d = *s;
		break;
	    }
	}
	d = maybe_end_group (d, do_group, &was_wildcard);
	if (match_type == match_file)
	    *d++ = '$';
	*d = 0;
	return new_pattern;
    } else
	return pattern;
}
