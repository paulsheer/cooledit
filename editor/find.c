/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* find.c - find files as a front end to find ... -exec grep ...
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "stringtools.h"
#include "coolwidget.h"
#include "edit.h"
#include "editcmddef.h"
#include "loadfile.h"
#include "editoptions.h"
#include "cmdlineopt.h"
#include "shell.h"

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
#include "find.h"

#define MID_X 20
#define MID_Y 20

int execute_background_display_output (const char *title, const char *s, const char *name);

Window find_mapped_window (Window w);

int CInputsWithOptions (Window parent, int x, int y, const char *heading, \
	    char **inputs[], char *input_labels[], char *input_names[], \
    char *input_tool_hint[], int *check_values[], char *check_labels[], \
			char *check_tool_hints[], int options, int width)
{E_
    char *f = 0;
    Window win;
    XEvent xev;
    CEvent cev;
    CState s;
    int xh, yh, ys;
    int k, labels_in_column, cancel = 0;

    CBackupState (&s);
    CDisable ("*");

    if (options & INPUTS_WITH_OPTIONS_FREE_STRINGS)
	for (k = 0; inputs[k]; k++)
	    if (!*inputs[k])
		*inputs[k] = (char *) strdup ("");

    if (!parent) {
	x = MID_X;
	y = MID_Y;
    }
    parent = find_mapped_window (parent);
    win = CDrawHeadedDialog ("_IWO", parent, x, y, heading);

    CIdent ("_IWO")->position = WINDOW_ALWAYS_RAISED;

    CGetHintPos (&xh, 0);
    for (k = 0; inputs[k] && input_labels[k] && input_names[k]; k++) {
	char *hot, id[32] = "_IWO.t.x", *l;
        CStr last;
	l = (char *) strdup (_ (input_labels[k]));
	hot = strchr (l, '&');
	if (!hot)
	    hot = "";
	else
	    Cmemmove (hot, hot + 1, strlen (hot + 1) + 1);
	id[7] = 'A' + k;
	CGetHintPos (0, &yh);
	(CDrawText (id, win, xh, yh, _ (l)))->hotkey = *hot;
	CGetHintPos (0, &yh);
        last = CStr_dupstr(CLastInput (input_names[k]));
	if (!last.len || input_names[k][0] == '~') {
            CStr_free(&last);
	    last = CStr_dup(*inputs[k]);
        }
	if ((options & (INPUTS_WITH_OPTIONS_BROWSE_LOAD << k)) || (options & (INPUTS_WITH_OPTIONS_BROWSE_SAVE << k)) || (options & (INPUTS_WITH_OPTIONS_BROWSE_DIR << k))) {
	    int xt;
	    char bi[33];
	    (CDrawTextInput (input_names[k], win, xh, yh, FONT_MEAN_WIDTH * width - WIDGET_SPACING - 8 - CImageTextWidth (_ (" Browse... "), 11), AUTO_HEIGHT, 256, &last))->hotkey = *hot;
	    CGetHintPos (&xt, 0);
	    if (options & (INPUTS_WITH_OPTIONS_BROWSE_LOAD << k))
		sprintf (bi, "%s.bl", input_names[k]);
	    else if (options & (INPUTS_WITH_OPTIONS_BROWSE_SAVE << k))
		sprintf (bi, "%s.bs", input_names[k]);
	    else
		sprintf (bi, "%s.bd", input_names[k]);
	    CDrawButton (bi, win, xt, yh, AUTO_SIZE, _ (" Browse... "));
	} else {
	    (CDrawTextInput (input_names[k], win, xh, yh, FONT_MEAN_WIDTH * width, AUTO_HEIGHT, 256, &last))->hotkey = *hot;
	}
	if (input_tool_hint) {
	    CSetToolHint (id, _ (input_tool_hint[k]));
	    CSetToolHint (input_names[k], _ (input_tool_hint[k]));
	}
        CStr_free(&last);
	free (l);
    }

    for (k = 0; check_values[k] && check_labels[k]; k++);
    labels_in_column = (k + 1) / 2;

    CGetHintPos (0, &ys);

/* The following are check boxes */
    get_hint_limits (&x, &y);
    for (k = 0; check_values[k] && check_labels[k]; k++) {
	char id[32] = "_IWO.c.x";
	id[7] = 'A' + k;
	CGetHintPos (0, &yh);
	if (k == labels_in_column)
	    yh = ys;
	CDrawSwitch (id, win, xh + ((k >= labels_in_column) ? x / 2 : 0), yh, *check_values[k], _ (check_labels[k]), 0);
	if (check_tool_hints) {
	    CSetToolHint (id, _ (check_tool_hints[k]));
	    strcat (id, ".label");
	    CSetToolHint (id, _ (check_tool_hints[k]));
	    id[8] = '\0';
	}
    }

    get_hint_limits (&x, &yh);
    CDrawPixmapButton ("_IWO.ok", win, x / 3 - TICK_BUTTON_WIDTH / 2, yh, PIXMAP_BUTTON_TICK);
    CSetToolHint ("_IWO.ok", _ ("Execute, Enter"));
    CDrawPixmapButton ("_IWO.cancel", win, 2 * x / 3 - TICK_BUTTON_WIDTH / 2, yh, PIXMAP_BUTTON_CROSS);
    CSetToolHint ("_IWO.cancel", _ ("Abort this dialog, Esc"));
    CSetSizeHintPos ("_IWO");
    CMapDialog ("_IWO");

    if (CIdent (input_names[0]))
	CFocus (CIdent (input_names[0]));

    for (;;) {
	CNextEvent (&xev, &cev);
	if (!CIdent ("_IWO")) {
	    cancel = 1;
	    break;
	}
	if (!strcmp (cev.ident, "_IWO.cancel") || cev.command == CK_Cancel) {
	    cancel = 1;
	    break;
	}
	if (!strcmp (cev.ident, "_IWO.ok") || cev.command == CK_Enter) {
	    for (k = 0; inputs[k] && input_labels[k] && input_names[k]; k++) {
		if (options & INPUTS_WITH_OPTIONS_FREE_STRINGS) {
		    if (*inputs[k])
			free (*inputs[k]);
		    *inputs[k] = (char *) strdup (CIdent (input_names[k])->text.data);
		    if (!(*inputs[k])[0]) {
			free (*inputs[k]);
			*inputs[k] = 0;
		    }
		} else
		    *inputs[k] = (char *) strdup (CIdent (input_names[k])->text.data);
	    }
	    for (k = 0; check_values[k] && check_labels[k]; k++) {
		char id[32] = "_IWO.c.x";
		id[7] = 'A' + k;
		*check_values[k] = CIdent (id)->keypressed;
	    }
	    break;
	}
#warning maybe backport this fix
	if (strendswith (cev.ident, ".bl")) {
	    f = CGetLoadFile (parent, 20, 20, current_dir, "", _ (" Browse "), 0 /* localhost */);
	    goto there;
	}
	if (strendswith (cev.ident, ".bs")) {
	    f = CGetSaveFile (parent, 20, 20, current_dir, "", _ (" Browse "), 0 /* localhost */);
	    goto there;
	}
	if (strendswith (cev.ident, ".bd")) {
	    f = CGetDirectory (parent, 20, 20, current_dir, "", _ (" Browse "), 0 /* localhost */);
	  there:
	    if (f) {
		CWidget *ttw;
		char tt[33];
		strcpy (tt, cev.ident);
		tt[strlen (tt) - 3] = '\0';
		ttw = CIdent (tt);
                CStr_free(&ttw->text);
		ttw->text = CStr_dup(f);;
		ttw->cursor = ttw->text.len;
		CExpose (tt);
		CFocus (ttw);
                free(f);
	    }
	}
    }
    CDestroyWidget ("_IWO");
    CRestoreState (&s);
    if (options & INPUTS_WITH_OPTIONS_FREE_STRINGS)
	for (k = 0; inputs[k]; k++)
	    if (*inputs[k])
		if (cancel || !(*inputs[k])[0]) {
		    free (*inputs[k]);
		    *inputs[k] = 0;
		}
    return cancel;
}

void find_file (void)
{E_
    char s[4096] = "#!/bin/sh\n";
    char *inputs[10] =
    {
	gettext_noop ("."),
	gettext_noop ("*"),
	gettext_noop (""),
	0
    };
    char *input_labels[10] =
    {
	gettext_noop ("&Starting directory"),
	gettext_noop ("Filenames matching &glob expression"),
	gettext_noop ("&Containing"),
	0
    };
    char *check_labels[10] =
    {
	gettext_noop ("Containing reg. exp."),
	gettext_noop ("Containing case insens."),
	gettext_noop ("Search follows symlinks"),
	gettext_noop ("Search is case insens."),
	0
    };
    char *check_tool_hints[10] =
    {
    gettext_noop ("Enter regular expression pattern to find within file"),
    gettext_noop ("Match case insensitively when searching within files"),
	gettext_noop ("Dive into symlinks to directories"),
	gettext_noop ("Filenames are matched case insensitively"),
	0
    };
    char *input_names[10] =
    {
	gettext_noop ("find-start_dir"),
	gettext_noop ("find-glob_express"),
	gettext_noop ("find-containing"),
	0
    };
    char *input_tool_hint[10] =
    {
	gettext_noop ("Starting directory for the recursive search"),
	gettext_noop ("Find files that match glob expressions such as *.[ch] or *.doc"),
	gettext_noop ("Check if file contains this sequence\n(don't forget that you can match whole words eg. \\<the\\>)"),
	0
    };
    static int checks_values[10] =
    {
	0, 0, 0, 0, 0
    };
    int *checks_values_result[10];
    char **inputs_result[10];
    int r;

    inputs_result[0] = &inputs[0];
    inputs_result[1] = &inputs[1];
    inputs_result[2] = &inputs[2];
    inputs_result[3] = &inputs[3];
    inputs_result[4] = 0;

    checks_values_result[0] = &checks_values[0];
    checks_values_result[1] = &checks_values[1];
    checks_values_result[2] = &checks_values[2];
    checks_values_result[3] = &checks_values[3];
    checks_values_result[4] = 0;

    r = CInputsWithOptions (0, 0, 0, _ ("Find File"), inputs_result, input_labels, input_names, input_tool_hint, checks_values_result, check_labels, check_tool_hints, INPUTS_WITH_OPTIONS_BROWSE_DIR_1, 60);
    if (r)
	return;
    strcat (s, "find ");
    strcat (s, inputs[0]);
    if (checks_values[2])
	strcat (s, " -follow");
    strcat (s, " -type f ");
    if (inputs[1][0]) {
	if (checks_values[3]) {
	    strcat (s, "-iname '");
	    strcat (s, inputs[1]);
	} else {
	    strcat (s, "-name '");
	    strcat (s, inputs[1]);
	}
	strcat (s, "' ");
    } else {
	strcat (s, "-name '*' ");	/* needed for non-GNU find commands */
    }
    if (inputs[2][0]) {
	strcat (s, "-exec ");
	if (checks_values[0]) {
	    strcat (s, "grep -s -n ");
	} else {
	    strcat (s, "fgrep -s -n ");
	}
	if (checks_values[1])
	    strcat (s, "-i ");
	if (inputs[2][0] == '-')
	    strcat (s, "-e ");
	strcat (s, "'");
	strcat (s, inputs[2]);
	strcat (s, "' 'Not any such file here at all' {} \\; ");
    }
    strcat (s, "\necho Done\n");
    if (inputs[0])
	free (inputs[0]);
    if (inputs[1])
	free (inputs[1]);
    if (inputs[2])
	free (inputs[2]);
    execute_background_display_output (_ (" Find File "), s, "FindfIlEmAgiC");
}

void ctags (void)
{E_
    char s[4096] = "#!/bin/sh\n";
    char types[20] = "";
    char *inputs[20] =
    {
	gettext_noop ("."),
	gettext_noop ("*"),
	gettext_noop (""),
	0
    };
    char *input_labels[20] =
    {
	gettext_noop ("&Starting directory"),
	gettext_noop ("Filenames matching &glob expression"),
	gettext_noop ("&Extra arguments"),
	0
    };
    char *check_labels[20] =
    {
	gettext_noop ("Recursive search"),
	gettext_noop ("Classes"),
	gettext_noop ("Macro definitions"),
	gettext_noop ("Enumerators"),
	gettext_noop ("Dunction definitions"),
	gettext_noop ("Enumeration names"),
	gettext_noop ("class and struct members"),
	gettext_noop ("Namespaces"),
	gettext_noop ("Function prototypes"),
	gettext_noop ("Structure name"),
	gettext_noop ("Typedefs"),
	gettext_noop ("Union names"),
	gettext_noop ("Variable definitions"),
	gettext_noop ("Extern and forward defs"),
	0
    };
    char *check_tool_hints[20] =
    {
      gettext_noop ("Dive into subdirectories to look for source files"),
	gettext_noop ("Find classes"),
	gettext_noop ("Find definitions (and #undef names)"),
	gettext_noop ("Find enumerators"),
	gettext_noop ("Find definitions"),
	gettext_noop ("Find numeration names"),
	gettext_noop ("Find class, struct or union members"),
	gettext_noop ("Find namespaces"),
	gettext_noop ("Find function prototypes and declarations"),
	gettext_noop ("Find structure names"),
	gettext_noop ("Find typedefs"),
	gettext_noop ("Find union names"),
	gettext_noop ("Find variable definitions"),
	gettext_noop ("Find extern and forward variable declarations"),
	0
    };
    char *input_names[20] =
    {
	gettext_noop ("ctags-start_dir"),
	gettext_noop ("ctags-glob_express"),
	gettext_noop ("ctags-extra"),
	0
    };
    char *input_tool_hint[20] =
    {
	gettext_noop ("Starting directory for recursive search"),
	gettext_noop ("Find files that match glob expressions\nsuch as *.[ch] or *.cc (default - autodetect)"),
	gettext_noop ("Extra arguments to pass to ctags - see ctags(1)"),
	0
    };
    static int checks_values[20] =
    {
	1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0
    };
    int *checks_values_result[20];
    char **inputs_result[20];
    int i, k, r;

    inputs_result[0] = &inputs[0];
    inputs_result[1] = &inputs[1];
    inputs_result[2] = &inputs[2];
    inputs_result[3] = &inputs[3];
    inputs_result[4] = 0;

    checks_values_result[0] = &checks_values[0];
    checks_values_result[1] = &checks_values[1];
    checks_values_result[2] = &checks_values[2];
    checks_values_result[3] = &checks_values[3];
    checks_values_result[4] = &checks_values[4];
    checks_values_result[5] = &checks_values[5];
    checks_values_result[6] = &checks_values[6];
    checks_values_result[7] = &checks_values[7];
    checks_values_result[8] = &checks_values[8];
    checks_values_result[9] = &checks_values[9];
    checks_values_result[10] = &checks_values[10];
    checks_values_result[11] = &checks_values[11];
    checks_values_result[12] = &checks_values[12];
    checks_values_result[13] = &checks_values[13];
    checks_values_result[14] = 0;

    r = CInputsWithOptions (0, 0, 0, _ ("Ctags Index"), inputs_result, input_labels, input_names, input_tool_hint, checks_values_result, check_labels, check_tool_hints, INPUTS_WITH_OPTIONS_BROWSE_DIR_1, 60);
    if (r)
	return;
    for (k = 0, i = 0; i < 13; i++) {
	types[k] = ("cdefgmnpstuvx")[i];
	if (checks_values[i + 1])
	    k++;
    }
    types[k] = '\0';
    if (!strlen (types)) {
	CMessageDialog (0, 20, 20, 0, _ ("Ctags Index"), _ ("You must specify at least one type to search for."));
	return;
    } else if (checks_values[0]) {
	sprintf (s, "find %s -name '%s' | ctags %s --c-types=%s -x -f - -L - | awk '{ print $1 \" \" $2 \" \" $4 \":\" $3  }'\necho Done\n", \
		 inputs[0], inputs[1] ? (*inputs[1] ? inputs[1] : "*") : "*", inputs[2], types);
    } else {
	sprintf (s, "ctags %s --c-types=%s -x -f - %s/%s | awk '{ print $1 \" \" $2 \" \" $4 \":\" $3  }'\necho Done\n", \
		 inputs[2], types, inputs[0], inputs[1] ? (*inputs[1] ? inputs[1] : "*") : "*");
    }
    if (inputs[0])
	free (inputs[0]);
    if (inputs[1])
	free (inputs[1]);
    if (inputs[2])
	free (inputs[2]);
    execute_background_display_output (_ (" Ctags Output "), s, "FindTagSmAgiC");
}


