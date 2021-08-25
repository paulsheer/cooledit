/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* trivoptions.c - loads some trivial options from the ini file
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <my_string.h>
#include <stringtools.h>

#define OPTIONS_FILE "/.cedit/.cooledit.ini"

extern int option_tab_spacing ;
extern int option_fill_tabs_with_spaces;

extern int option_editor_fg_normal;
extern int option_editor_fg_bold;
extern int option_editor_fg_italic;
extern int option_editor_bg_normal;
extern int option_editor_bg_marked;
extern int option_editor_bg_highlighted;

extern int option_text_fg_normal;
extern int option_text_fg_bold;
extern int option_text_fg_italic;
extern int option_text_bg_normal;
extern int option_text_bg_marked;
extern int option_text_bg_highlighted;
extern int option_toolhint_milliseconds1;
extern int option_remote_timeout;

extern char *init_look;

struct options_struct {
    char *text;
    int *value;
} options[] =
{
	{"\noption_tab_spacing = ", &option_tab_spacing},
	{"\noption_fill_tabs_with_spaces = ", &option_fill_tabs_with_spaces},
	{"\noption_text_fg_normal = ", &option_text_fg_normal},
	{"\noption_text_fg_bold = ", &option_text_fg_bold},
	{"\noption_text_fg_italic = ", &option_text_fg_italic},
	{"\noption_text_bg_normal = ", &option_text_bg_normal},
	{"\noption_text_bg_marked = ", &option_text_bg_marked},
	{"\noption_text_bg_highlighted = ", &option_text_bg_highlighted},
	{"\noption_toolhint_milliseconds1 = ", &option_toolhint_milliseconds1},
	{"\noption_remote_timeout = ", &option_remote_timeout}
};

char *loadfile (const char *filename, long *filelen);

void load_trivial_options (void)
{E_
    char *f, *s, *e, *p;
    int i;
    f = loadfile (catstrs (getenv ("HOME"), OPTIONS_FILE, NULL), 0);
    if (!f)
	return;
    s = strstr (f, "[Options]\n");
    if (!s)
	return;
    e = strstr (f, "\n\n[");
    if (e)
	*e = '\0';
    for (i = 0; i < sizeof (options) / sizeof (struct options_struct); i++) {
	p = strstr (s, options[i].text);
	if (!p)
	    continue;
	p += strlen (options[i].text);
	*(options[i].value) = atoi (p);
    }
    p = strstr (s, "\noption_look = ");
    if (p) {
	int l;
	p += strlen ("\noption_look = ");
	l = strcspn (p, "\n");
	strncpy (init_look = (char *) malloc (l + 1), p, l);
	init_look[l] = '\0';
    }
    free (f);

#ifndef NEXT_LOOK
    option_editor_fg_normal = option_text_fg_normal;
    option_editor_fg_bold = option_text_fg_bold;
    option_editor_fg_italic = option_text_fg_italic;
    option_editor_bg_normal = option_text_bg_normal;
    option_editor_bg_marked = option_text_bg_marked;
    option_editor_bg_highlighted = option_text_bg_highlighted;
#endif
}

