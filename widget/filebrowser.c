/* filebrowser.c - draws an interactive file browser
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"

#include "editcmddef.h"


extern struct look *look;

#define GETFILE_GET_DIRECTORY		1
#define GETFILE_GET_EXISTING_FILE	2
#define GETFILE_BROWSER			4

#define FILE_BROWSER_START_WIDTH	40
#define FILE_BROWSER_START_HEIGHT	15

int option_file_browser_width = FILE_BROWSER_START_WIDTH;
int option_file_browser_height = FILE_BROWSER_START_HEIGHT;

void CDrawBrowser (const char *ident, Window parent, int x, int y,
		   const char *dir, const char *file, const char *label, char *host)
{
    (*look->draw_browser) (ident, parent, x, y, host, dir, file, label);
}

char *CGetFile (Window parent, int x, int y,
		const char *dir, const char *file, const char *label, char *host)
{
    return (*look->get_file_or_dir) (parent, x, y, host, dir, file, label, 0);
}

char *CGetDirectory (Window parent, int x, int y,
		     const char *dir, const char *file, const char *label, char *host)
{
    return (*look->get_file_or_dir) (parent, x, y, host, dir, file, label, GETFILE_GET_DIRECTORY);
}

char *CGetSaveFile (Window parent, int x, int y,
		    const char *dir, const char *file, const char *label, char *host)
{
    return (*look->get_file_or_dir) (parent, x, y, host, dir, file, label, 0);
}

char *CGetLoadFile (Window parent, int x, int y,
		    const char *dir, const char *file, const char *label, char *host)
{
    return (*look->get_file_or_dir) (parent, x, y, host, dir, file, label, GETFILE_GET_EXISTING_FILE);
}


