/* percentsubs.c - perform % substitution in shell script
   Copyright (C) 1996-2018 Paul Sheer
 */

   
#include <config.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <editor/shell.h>
#include "pool.h"

extern char *init_font;

#warning needs to distinguish between local and remote home directories
char *hme (char *text)
{
    return catstrs (local_home_dir, text, NULL);
}

/*
   Result must be free'd. This substitutes all "%f", "%d",...
   found in the script (null-terminated) with the proper strings
   or filenames
 */
char *substitute_strings (char *text, char *cmdline_options, char *editor_file, int current_line, int current_column)
{
    char *result, *p, *q, *r;

    r = result = CMalloc (8192 + strlen (text));
    p = q = text;

    for (;;) {
	q = strchr (p, '%');
	if (!q) {
	    strcpy (r, p);
	    break;
	}
	memcpy (r, p, (unsigned long) q - (unsigned long) p);
	r += (unsigned long) q - (unsigned long) p;
	*r = 0;
	q++;
	switch (*q) {
	    char *t;
	case '%':
	    r[0] = '%';
	    r[1] = 0;
	    break;
	case SHELL_SUBS_CURRENT_DIRECTORY:
	    strcpy (r, current_dir);
	    break;
	case SHELL_SUBS_FONT_FIXED:
	    if (FIXED_FONT)
		strcpy (r, init_font);
	    else
		strcpy (r, "8x13bold");
	    break;
	case SHELL_SUBS_FONT:
	    strcpy (r, init_font);
	    break;
	case SHELL_SUBS_EDITOR_FILE:
	case SHELL_SUBS_EDITOR_FILE_NAME:
	    t = strrchr (editor_file, '/');
	    if (t)
		t++;
	    else
		t = editor_file;
	    strcpy (r, t);
	    if (*q == SHELL_SUBS_EDITOR_FILE_NAME) {
		t = strrchr (r, '.');
		if (t)
		    *t = 0;
	    }
	    break;
        case SHELL_SUBS_EDITOR_FILE_LINE:
            sprintf (r, "%d", current_line);
	    break;
        case SHELL_SUBS_EDITOR_FILE_COLUMN:
            sprintf (r, "%d", current_column);
	    break;
	case SHELL_SUBS_EDITOR_FILE_EXTENSION:
	    t = strrchr (editor_file, '/');
	    if (t)
		t++;
	    else
		t = editor_file;
	    t = strrchr (t, '.');
	    if (!t)
		t = "";
	    strcpy (r, t);
	    break;
	case SHELL_SUBS_EDITOR_FILE_PATH:
	    strcpy (r, editor_file);
	    t = strrchr (r, '/');
	    if (t) {
		if ((unsigned long) t == (unsigned long) r)
		    *t++ = '/';
		*t = 0;
	    }
	    break;
	case SHELL_SUBS_TEMP_FILE:
	    strcpy (r, hme (TEMP_FILE));
	    break;
	case SHELL_SUBS_BLOCK_FILE:
	    strcpy (r, hme (BLOCK_FILE));
	    break;
	case SHELL_SUBS_CLIP_FILE:
	    strcpy (r, hme (CLIP_FILE));
	    break;
	case SHELL_SUBS_ERROR_FILE:
	    strcpy (r, hme (ERROR_FILE));
	    break;
	case SHELL_SUBS_ARGUMENTS:
	    strcpy (r, cmdline_options);
	    break;
	default:
/* leave unchanged */
	    r[0] = '%';
	    r[1] = *q;
	    r[2] = 0;
	    break;
	}
	r += strlen (r);
	p = q + 1;
    }
    return result;
}

