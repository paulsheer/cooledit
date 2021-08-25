/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* filelist.c - for drawing a scrollable filelist with size, permissions etc.
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <my_string.h>
#include "stringtools.h"
#include <sys/types.h>

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#include <sys/stat.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "coolwidget.h"

extern struct look *look;

/* reset with timestr = 0 */
void get_file_time (char *timestr, time_t file_time, int l)
{E_
#ifndef HAVE_STRFTIME
    static char monthstr[12][8] =
    {
/* Month list */
	gettext_noop ("Jan"), gettext_noop ("Feb"), gettext_noop ("Mar"), gettext_noop ("Apr"), gettext_noop ("May"), gettext_noop ("Jun"), gettext_noop ("Jul"), gettext_noop ("Aug"), gettext_noop ("Sep"), gettext_noop ("Oct"), gettext_noop ("Nov"), gettext_noop ("Dec")
    };
#endif
    static struct tm tm_current =
    {-1};
    struct tm *tm;
#ifndef HAVE_STRFTIME
    static int i = 0;
#endif

    if (!timestr) {
	return;
    }
#ifndef HAVE_STRFTIME
    if (!i)
	for (i = 0; i < 12; i++)
	    strcpy (monthstr[i], _ (monthstr[i]));
#endif

    if (tm_current.tm_sec == -1) {
	time_t t;
	time (&t);
	memcpy ((void *) &tm_current, (void *) localtime (&t), sizeof (struct tm));
    }
    tm = localtime (&(file_time));
#if HAVE_STRFTIME
    if (l) {
	strftime (timestr, 31, "%b %e %H:%M %Y", tm);
    } else {
	if (tm->tm_year == tm_current.tm_year)	/* date with year and without time */
	    strftime (timestr, 31, "%b %d %H:%M", tm);
	else			/* date without year and with time */
	    strftime (timestr, 31, "%Y %b %d", tm);
    }
#else
    if (l) {
	sprintf (timestr, "%s %2d %.2d:%.2d %d", monthstr[tm->tm_mon],
	       tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_year + 1900);
    } else {
	if (tm->tm_year == tm_current.tm_year)	/* date with year and without time */
	    sprintf (timestr, "%s %.2d %.2d:%.2d", monthstr[tm->tm_mon],
		     tm->tm_mday, tm->tm_hour, tm->tm_min);
	else			/* date without year and with time */
	    sprintf (timestr, "%d %s %.2d", tm->tm_year + 1900,
		     monthstr[tm->tm_mon], tm->tm_mday);
    }
#endif
}


CWidget *CDrawFilelist (const char *identifier, Window parent, int x, int y,
			int width, int height, int line, int column,
			struct file_entry *directentry, long options)
{E_
    return (*look->draw_file_list) (identifier, parent, x, y, width, height, line, column,
				    directentry, options);
}

CWidget *CRedrawFilelist (const char *identifier, struct file_entry *directentry, int preserve)
{E_
    return (*look->redraw_file_list) (identifier, directentry, preserve);
}

struct file_entry *CGetFilelistLine (CWidget * w, int line)
{E_
    return (*look->get_file_list_line) (w, line);
}


