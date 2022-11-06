/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* print.c - GUI frontend to the postscript printing
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "stringtools.h"
#include "coolwidget.h"
#include "edit.h"
#include "editoptions.h"
#include "postscript.h"
#include "find.h"
#include "print.h"


#if 0
static char *font_list[] =
{E_
    "AvantGarde-Book",
    "AvantGarde-BookOblique",
    "AvantGarde-Demi",
    "AvantGarde-DemiOblique",
    "Bookman-Demi",
    "Bookman-DemiItalic",
    "Bookman-Light",
    "Bookman-LightItalic",
    "Courier",
    "Courier-Bold",
    "Courier-BoldOblique",
    "Courier-Oblique",
    "Helvetica",
    "Helvetica-Bold",
    "Helvetica-BoldOblique",
    "Helvetica-Narrow",
    "Helvetica-Narrow-Bold",
    "Helvetica-Narrow-BoldOblique",
    "Helvetica-Narrow-Oblique",
    "Helvetica-Oblique",
    "NewCenturySchlbk-Bold",
    "NewCenturySchlbk-BoldItalic",
    "NewCenturySchlbk-Italic",
    "NewCenturySchlbk-Roman",
    "Palatino-Bold",
    "Palatino-BoldItalic",
    "Palatino-Italic",
    "Palatino-Roman",
    "Times-Bold",
    "Times-BoldItalic",
    "Times-Italic",
    "Times-Roman",
    "Utopia-Bold",
    "Utopia-BoldItalic",
    "Utopia-Italic",
    "Utopia-Regular",
    0};
#endif

static WEdit *print_editor = 0;
static long print_current_pos = 0;

static unsigned char *print_get_next_line (unsigned char *old)
{E_
    int l, len;
    unsigned char *r = 0;
    if (print_current_pos < print_editor->last_byte) {
	l = edit_eol (print_editor, print_current_pos);
	r = edit_get_block (print_editor, print_current_pos, l, &len);
	print_current_pos = l + 1;
    }
    if (old) {
	memset (old, 0, strlen ((char *) old));
	free (old);
    }
    return r;
}

void print_dialog_cannot_open (unsigned char *m)
{E_
#warning backport this fix
    CErrorDialog (0, 0, 0, _ (" Print "), " %s: \n %s ", get_sys_error (_ (" Error trying to open print file. ")), m);
}

int print_dialog_exists (unsigned char *m)
{E_
    char s[2048];
    sprintf (s, " %s: \n %s ", _ ("File exists, shall I overwrite?"), m);
    if (CQueryDialog (0, 0, 0, _ (" Print "), s, (" Yes "), (" No "), NULL) == 0)
	return 1;
    return 0;
}

void cooledit_print_dialog (WEdit * editor)
{E_
    unsigned char margin[12], width[12], height[12], chars[12];
    unsigned char *margin_result = 0, *width_result = 0, *height_result = 0, *chars_result = 0;
    char *input_labels[20] =
    {
	gettext_noop ("&Print command"),
	gettext_noop ("or, Print to file"),
	gettext_noop ("Margin"),
	gettext_noop ("Chars per line"),
	gettext_noop ("Title"),
	gettext_noop ("Page height (mm)"),
	gettext_noop ("Page width (mm)"),
	0
    };
    char *check_labels[20] =
    {
	gettext_noop ("Landscape"),
	gettext_noop ("2 column"),
	gettext_noop ("3 column"),
	gettext_noop ("Landsc. foot-line"),
	gettext_noop ("Line numbering"),
	gettext_noop ("Show header"),
	gettext_noop ("Wrap Lines"),
	gettext_noop ("Plain text"),
	gettext_noop ("Bold"),
	0
    };
    char *check_tool_hints[20] =
    {
	gettext_noop ("Page on its side"),
	gettext_noop ("2 column format"),
	gettext_noop ("3 column format"),
	gettext_noop ("Puts a foot-line instead of a head-line in landscape mode"),
	gettext_noop ("Numbers every five lines"),
	gettext_noop ("Turn off to disable the header"),
	gettext_noop ("Otherwise lines are truncated"),
	gettext_noop ("Do not convert to PostScript"),
	gettext_noop ("Uses the postscript \"Courier-Bold\" font instead of \"Courier\""),
	0
    };
    char *input_names[20] =
    {
	gettext_noop ("print-command+"),
	gettext_noop ("print-file+"),
	gettext_noop ("print-margin"),
	gettext_noop ("print-char-per-line"),
	gettext_noop ("~print-title"),
	gettext_noop ("print-height"),
	gettext_noop ("print-width"),
	0
    };
    char *input_tool_hint[20] =
    {
	gettext_noop ("The is a pipe to send the print job to, usually: lpr -Plp"),
	gettext_noop ("This is a file to write the print job instead of using a command.\nIf you specify both, this takes priority."),
	gettext_noop ("Left and right margins"),
	gettext_noop ("Number of characters on a line"),
	gettext_noop ("Title to be displayed in header"),
	gettext_noop ("Page height in millimetres. Letter is 612 by 792. A4 is 595 by 842"),
	gettext_noop ("Page width in millimetres. Letter is 612 by 792. A4 is 595 by 842"),
	0
    };
    int *checks_values_result[20];
    unsigned char **inputs_result[20];
    int r = 0, two_column = 0, three_column = 0, bold = 0;

    sprintf ((char *) margin, "%d", postscript_option_right_margin);
    sprintf ((char *) width, "%d", postscript_option_page_right_edge);
    sprintf ((char *) height, "%d", postscript_option_page_top);
    sprintf ((char *) chars, "%d", postscript_option_chars_per_line);

    margin_result = (unsigned char *) strdup ((char *) margin);
    width_result = (unsigned char *) strdup ((char *) width);
    height_result = (unsigned char *) strdup ((char *) height);
    chars_result = (unsigned char *) strdup ((char *) chars);

    if (!postscript_option_pipe)
	postscript_option_pipe = (unsigned char *) strdup ("lpr");
    if (postscript_option_title) {
	if (editor->filename)
	    if (*editor->filename) {
		free (postscript_option_title);
		postscript_option_title = (unsigned char *) strdup (editor->filename);
	    }
    } else {
	postscript_option_title = (unsigned char *) strdup (editor->filename);
    }
    inputs_result[0] = &postscript_option_pipe;
    inputs_result[1] = &postscript_option_file;
    inputs_result[2] = &margin_result;
    inputs_result[3] = &chars_result;
    inputs_result[4] = &postscript_option_title;
    inputs_result[5] = &height_result;
    inputs_result[6] = &width_result;
    inputs_result[7] = 0;

    checks_values_result[0] = &postscript_option_landscape;
    checks_values_result[1] = &two_column;
    checks_values_result[2] = &three_column;
    checks_values_result[3] = &postscript_option_footline_in_landscape;
    checks_values_result[4] = &postscript_option_line_numbers;
    checks_values_result[5] = &postscript_option_show_header;
    checks_values_result[6] = &postscript_option_wrap_lines;
    checks_values_result[7] = &postscript_option_plain_text;
    checks_values_result[8] = &bold;
    checks_values_result[9] = 0;

    r = CInputsWithOptions (0, 0, 0, _ (" Print "), (char ***) inputs_result, input_labels, input_names, input_tool_hint, checks_values_result, check_labels, check_tool_hints, NULL, INPUTS_WITH_OPTIONS_BROWSE_SAVE_2 | INPUTS_WITH_OPTIONS_FREE_STRINGS, 50);
    if (r)
	goto print_done;

    if (!postscript_option_file && !postscript_option_pipe) {
	CErrorDialog (0, 0, 0, _ (" Print "), _ (" You must specify one of either a file or a print command. "));
	goto print_done;
    }
    postscript_option_columns = 1;
    if (two_column)
	postscript_option_columns = 2;
    if (three_column)
	postscript_option_columns = 3;
    if (bold)
	postscript_option_font = (unsigned char *) "Courier-Bold";
    else
	postscript_option_font = (unsigned char *) "Courier";

    if (margin_result) {
	postscript_option_right_margin = atoi ((char *) margin_result);
	free (margin_result);
    } else {
	postscript_option_right_margin = 40;
    }
    if (chars_result) {
	postscript_option_chars_per_line = atoi ((char *) chars_result);
	free (chars_result);
    } else {
	postscript_option_chars_per_line = 0;
    }
    if (height_result) {
	postscript_option_page_top = atoi ((char *) height_result);
	free (height_result);
    } else {
	postscript_option_page_top = 842;
    }
    if (width_result) {
	postscript_option_page_right_edge = atoi ((char *) width_result);
	free (width_result);
    } else {
	postscript_option_page_right_edge = 595;
    }
    postscript_get_next_line = print_get_next_line;
    postscript_dialog_exists = print_dialog_exists;
    postscript_dialog_cannot_open = print_dialog_cannot_open;
    print_current_pos = 0;
    print_editor = editor;

    CDisableAlarm ();
    postscript_print ();
    CEnableAlarm ();
    return;
  print_done:
    if (margin_result)
	free (margin_result);
    if (chars_result)
	free (chars_result);
    if (height_result)
	free (height_result);
    if (width_result)
	free (width_result);
    return;
}


