/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* editoptions.c - save and load init file sections and do learn keys
   Copyright (C) 1996-2022 Paul Sheer
 */


#define EDIT_OPTIONS_C

#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "loadfile.h"
#include "stringtools.h"
#include "editoptions.h"
#include "editcmddef.h"

/* the rest are just for cb_learnkeys */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include "app_glob.c"
#include "coolwidget.h"
#include "shell.h"

extern char *editor_options_file;
extern Window main_window;

#define MAX_KEY_TEXT_SIZE 16384
/*: more than enough to ever run into problems */

#undef gettext_noop
#define gettext_noop(x) x

static struct key_list klist[] =
{
/* Do not change */
    {gettext_noop("\t \t"), 0, 0, 0, 0, 0, 0, 0},
/* The following are key descriptions for the 'Define Keys' dialog. You decide on their priority */
    {gettext_noop("\t.C.U.R.S.O.R .M.O.V.E.M.E.N.T.S\t"), 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("No_Command"), CK_No_Command, 0, 0, 0, 0, 0, 0},
    {gettext_noop("BackSpace"), CK_BackSpace, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Delete"), CK_Delete, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Return"), CK_Enter, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Page_Up"), CK_Page_Up, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Page_Down"), CK_Page_Down, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Left"), CK_Left, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Right"), CK_Right, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Word_Left"), CK_Word_Left, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Word_Right"), CK_Word_Right, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Up"), CK_Up, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Down"), CK_Down, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Home"), CK_Home, 0, 0, 0, 0, 0, 0},
    {gettext_noop("End"), CK_End, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Tab"), CK_Tab, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Undo"), CK_Undo, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Beginning_Of_Text"), CK_Beginning_Of_Text, 0, 0, 0, 0, 0, 0},
    {gettext_noop("End_Of_Text"), CK_End_Of_Text, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Scroll_Up"), CK_Scroll_Up, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Scroll_Down"), CK_Scroll_Down, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Enter"), CK_Return, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Begin_Page"), CK_Begin_Page, 0, 0, 0, 0, 0, 0},
    {gettext_noop("End_Page"), CK_End_Page, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Delete_Word_Left"), CK_Delete_Word_Left, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Delete_Word_Right"), CK_Delete_Word_Right, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Paragraph_Up"), CK_Paragraph_Up, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Paragraph_Down"), CK_Paragraph_Down, 0, 0, 0, 0, 0, 0},
    {"\t \t", 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("\t.F.I.L.E .C.O.M.M.A.N.D.S\t"), 0, 0, 0, 0, 0, 0},
    {gettext_noop("Save"), CK_Save, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Load"), CK_Load, 0, 0, 0, 0, 0, 0},
    {gettext_noop("New"), CK_New, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Save_As"), CK_Save_As, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Jump_To_File"), CK_Jump_To_File, 0, 0, 0, 0, 0, 0},
    {"\t \t", 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("\t.B.L.O.C.K .C.O.M.M.A.N.D.S\t"), 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Mark"), CK_Mark, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Column_Mark"), CK_Column_Mark, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Copy"), CK_Copy, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Move"), CK_Move, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Remove"), CK_Remove, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Unmark"), CK_Unmark, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Save_Block"), CK_Save_Block, 0, 0, 0, 0, 0, 0},
    {"\t \t", 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("\t.S.E.A.R.C.H .A.N.D .R.E.P.L.A.C.E\t"), 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Find"), CK_Find, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Find_Again"), CK_Find_Again, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Replace"), CK_Replace, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Replace_Again"), CK_Replace_Again, 0, 0, 0, 0, 0, 0},
    {"\t \t", 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("\t.B.O.O.K .M.A.R.K.S\t"), 0, 0, 0, 0, 0, 0},
    {gettext_noop("Toggle_Bookmark"), CK_Toggle_Bookmark, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Flush_Bookmarks"), CK_Flush_Bookmarks, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Next_Bookmark"), CK_Next_Bookmark, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Prev_Bookmark"), CK_Prev_Bookmark, 0, 0, 0, 0, 0, 0},
    {"\t \t", 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("\t.D.E.B.U.G\t"), 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Debug_Start"), CK_Debug_Start, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Debug_Stop"), CK_Debug_Stop, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Debug_Toggle_Break"), CK_Debug_Toggle_Break, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Debug_Clear"), CK_Debug_Clear, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Debug_Next"), CK_Debug_Next, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Debug_Step"), CK_Debug_Step, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Debug_Back_Trace"), CK_Debug_Back_Trace, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Debug_Continue"), CK_Debug_Continue, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Debug_Enter_Command"), CK_Debug_Enter_Command, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Debug_Until_Curser"), CK_Debug_Until_Curser, 0, 0, 0, 0, 0, 0},
    {"\t \t", 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("\t.M.I.S.C.E.L.L.A.N.E.O.U.S\t"), 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Insert_File"), CK_Insert_File, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Exit"), CK_Exit, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Toggle_Insert"), CK_Toggle_Insert, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Help"), CK_Help, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Date"), CK_Date, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Refresh"), CK_Refresh, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Goto"), CK_Goto, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Delete_Line"), CK_Delete_Line, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Delete_To_Line_End"), CK_Delete_To_Line_End, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Delete_To_Line_Begin"), CK_Delete_To_Line_Begin, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Man_Page"), CK_Man_Page, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Mail"), CK_Mail, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Find_File"), CK_Find_File, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Ctags"), CK_Ctags, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Complete"), CK_Complete, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Paragraph_Format"), CK_Paragraph_Format, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Paragraph_Indent_Mode"), CK_Paragraph_Indent_Mode, 0, 0, 0, 0, 0, 0},
#ifdef HAVE_PYTHON
    {gettext_noop("Type_Load_Python"), CK_Type_Load_Python, 0, 0, 0, 0, 0, 0},
#endif
    {gettext_noop("Util"), CK_Util, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Match_Bracket"), CK_Match_Bracket, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Terminal"), CK_Terminal, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Terminal_App"), CK_Terminal_App, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Insert Unicode"), CK_Insert_Unicode, 0, 0, 0, 0, 0, 0},
    {"\t \t", 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("\t.A.P.P.L.I.C.A.T.I.O.N .C.O.N.T.R.O.L\t"), 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Cancel"), CK_Cancel, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Save_Desktop"), CK_Save_Desktop, 0, 0, 0, 0, 0, 0},
    {gettext_noop("New_Window"), CK_New_Window, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Cycle"), CK_Cycle, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Menu"), CK_Menu, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Save_And_Quit"), CK_Save_And_Quit, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Run_Another"), CK_Run_Another, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Check_Save_And_Quit"), CK_Check_Save_And_Quit, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Maximize"), CK_Maximize, 0, 0, 0, 0, 0, 0},
    {"\t \t", 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("\t.M.A.C.R.O\t"), 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Begin_Record_Macro"), CK_Begin_Record_Macro, 0, 0, 0, 0, 0, 0},
    {gettext_noop("End_Record_Macro"), CK_End_Record_Macro, 0, 0, 0, 0, 0, 0},
    {"\t \t", 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("\t.H.I.G.H.L.I.G.H.T .C.O.M.M.A.N.D.S\t"), 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Page_Up_Highlight"), CK_Page_Up_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Page_Down_Highlight"), CK_Page_Down_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Left_Highlight"), CK_Left_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Right_Highlight"), CK_Right_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Word_Left_Highlight"), CK_Word_Left_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Word_Right_Highlight"), CK_Word_Right_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Up_Highlight"), CK_Up_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Down_Highlight"), CK_Down_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Home_Highlight"), CK_Home_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("End_Highlight"), CK_End_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Beginning_Of_Text_Highlight"), CK_Beginning_Of_Text_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("End_Of_Text_Highlight"), CK_End_Of_Text_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Begin_Page_Highlight"), CK_Begin_Page_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("End_Page_Highlight"), CK_End_Page_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Scroll_Up_Highlight"), CK_Scroll_Up_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Scroll_Down_Highlight"), CK_Scroll_Down_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Paragraph_Up_Highlight"), CK_Paragraph_Up_Highlight, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Paragraph_Down_Highlight"), CK_Paragraph_Down_Highlight, 0, 0, 0, 0, 0, 0},
    {"\t \t", 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("\t.X .C.L.I.P.B.O.A.R.D .O.P.E.R.A.T.I.O.N.S\t"), 0, 0, 0, 0, 0, 0, 0},
    {gettext_noop("XStore"), CK_XStore, 0, 0, 0, 0, 0, 0},
    {gettext_noop("XCut"), CK_XCut, 0, 0, 0, 0, 0, 0},
    {gettext_noop("XPaste"), CK_XPaste, 0, 0, 0, 0, 0, 0},
    {gettext_noop("Selection_History"), CK_Selection_History, 0, 0, 0, 0, 0, 0},
    {"", 0, 0, 0, 0, 0, 0, 0}
};


struct key_list *get_command_list (void)
{E_
    return klist;
}

/* returns 0 if not found */
int get_defined_key (struct key_list k_list[],
		     unsigned int state, unsigned int keycode)
{E_
    int i;
    for (i = 0;; i++) {
	if (!k_list[i].key_name[0])
	    break;
	if ((k_list[i].state0 == state && k_list[i].keycode0 == keycode)
	    || (k_list[i].state1 == state && k_list[i].keycode1 == keycode)
	    || (k_list[i].state2 == state && k_list[i].keycode2 == keycode))
	    return k_list[i].command;
    }
    return 0;
}

int get_script_number_from_key (unsigned int state, KeySym keysym);

int user_defined_key (unsigned int state, unsigned int keycode, KeySym keysym)
{E_
    if (keycode) {
	int i;
#ifdef HAVE_PYTHON
/* python user bindings take precedence over any other bindings */
	i = coolpython_key (state, keycode, keysym);
	if (i >= 0)
	    return CK_User_Command (i + MAX_NUM_SCRIPTS);
#endif
	i = get_script_number_from_key (state, keysym);
	if (i >= 0)
	    return CK_User_Command (i);
	return get_defined_key (klist, state, keycode);
    }
    return 0;
}

/* returns a pointer to the '[' that begins a section */
char *find_section (char *t, const char *section)
{E_
    char *p;
    int l;
    l = strlen (section);

    if (!Cstrncasecmp (t, section, l))
	return t;
    for (p = t; *p; p++)
	if (*p == '\n')
	    if (*(p + 1) == '[')
		if (!Cstrncasecmp (++p, section, l))
		    return p;
    return 0;
}


/* returns a pointer to the first line of the section after the "[Bla Bla]".
   return 0 on error or section not available, result must be free'd */
char *get_options_section (const char *file, const char *section)
{E_
    char *t, *p;

    t = loadfile (file, 0);
    if (!t)
	return 0;

    p = find_section (t, section);

    if (p) {
	char *q;
	p = strchr (p, '\n');
	if (p) {
	    if (*p) {
		p++;
		q = strstr (p, "\n[");
		if (q)
		    *q = 0;
		p = (char *) strdup (p);
	    } else {
		p = 0;
	    }
	}
    }
    free (t);
    return p;
}

/* return -1 on error, 0 on success */
int save_options_section (const char *file, const char *section, const char *text)
{E_
    char *t, *p, *result;

    t = loadfile (file, 0);
    if (!t)			/* file does not exist, savefile() will create it */
	t = (char *) strdup ("\n\n");

    p = find_section (t, section);
    if (p) {
	*p++ = 0;
	p = strstr (p, "\n[");
	result = catstrs (t, section, "\n", text, p, NULL);
    } else {
	result = catstrs (section, "\n", text, "\n", t, NULL);
    }
    free (t);
    return savefile (file, result, strlen (result), 0600);
}



/* return -1 on error */
int load_user_defined_keys (struct key_list k_list[], const char *file)
{E_
    char kname[128];
    struct key_list kl;
    char *s, *p;

    p = s = get_options_section (file, "[Key Defines]");
    if (!s)
	return -1;

    for (;;) {
	int i;
	*kname = 0;
	kl.state0 = 0;
	kl.state1 = 0;
	kl.state2 = 0;
	kl.keycode0 = 0;
	kl.keycode1 = 0;
	kl.keycode2 = 0;

	i = sscanf (p, "%s %x %x %x %x %x %x", kname, &kl.state0, &kl.keycode0,
		    &kl.state1, &kl.keycode1, &kl.state2, &kl.keycode2);
	if (i >= 3)
	    for (i = 0; k_list[i].key_name[0]; i++) {
		if (!Cstrcasecmp (kname, k_list[i].key_name)) {
		    k_list[i].state0 = kl.state0;
		    k_list[i].state1 = kl.state1;
		    k_list[i].state2 = kl.state2;
		    k_list[i].keycode0 = kl.keycode0;
		    k_list[i].keycode1 = kl.keycode1;
		    k_list[i].keycode2 = kl.keycode2;
		    break;
		}
	    }
	p = strchr (p, '\n');
	if (!p)
	    break;
	p++;
	i++;
    }
    free (s);
    return 0;
}

int load_keys (const char *file)
{E_
    return load_user_defined_keys (klist, file);
}


/* saves the klist key list into the options file in the section [Key Defines].
   saves only those keys that have at least one define. Returns -1 on error. */
int save_user_defined_keys (struct key_list k_list[], const char *file)
{E_
    char *s, *p;
    int n, i;

    p = s = malloc (MAX_KEY_TEXT_SIZE);

    if (!s)
	return -1;

    for (i = 0; k_list[i].key_name[0]; i++) {
	if (k_list[i].keycode2) {
	    sprintf (p, "%s\t%x %x %x %x %x %x\n%n", k_list[i].key_name, k_list[i].state0, k_list[i].keycode0,
		     k_list[i].state1, k_list[i].keycode1, k_list[i].state2, k_list[i].keycode2, &n);
	    p += n;
	} else if (k_list[i].keycode1) {
	    sprintf (p, "%s\t%x %x %x %x\n%n", k_list[i].key_name, k_list[i].state0, k_list[i].keycode0,
		     k_list[i].state1, k_list[i].keycode1, &n);
	    p += n;
	} else if (k_list[i].keycode0) {
	    sprintf (p, "%s\t%x %x\n%n", k_list[i].key_name, k_list[i].state0, k_list[i].keycode0, &n);
	    p += n;
	}
    }
    *p = 0;

    n = save_options_section (file, "[Key Defines]", s);
    free (s);
    return n;
}

/*
   This converts the klist list into a text block with each line
   containing a key define. The returned text is for display
   in a text box widget. Return 0 on error. Result must be free'd.
 */


char **get_key_text (void *data, int line, int *num_fields, int *tagged)
{E_
    struct key_list *get_klist;
    static char key_0[16];
    static char key_1[16];
    static char key_2[16];
    static char key_s[4] = "";
    static char *result[5] =
    {0, 0, 0, 0, 0};
    static int i = 0;

    get_klist = (struct key_list *) data;

    if (!get_klist[line].key_name[0])
	return 0;
    if (!key_s[0])
	sprintf (key_s, "\f%c", (unsigned char) CImageStringWidth ("99999"));

    if (!i)
	for (i = 0; get_klist[i].key_name[0]; i++) {
	    char *p;
	    strcpy (get_klist[i].key_name, _(get_klist[i].key_name));
	    for (p = get_klist[i].key_name; *p; p++)
		*p = *p == '.' ? '\b' : *p;
	}
    result[0] = get_klist[line].key_name;

    *num_fields = 4;
    *tagged = 0;

    if (get_klist[line].keycode0) {
	sprintf (key_0, "\t%d\t", get_klist[line].keycode0);
	result[1] = key_0;
	*tagged = 1;
    } else
	result[1] = key_s;

    if (get_klist[line].keycode1) {
	sprintf (key_1, "\t%d\t", get_klist[line].keycode1);
	result[2] = key_1;
	*tagged = 1;
    } else
	result[2] = key_s;

    if (get_klist[line].keycode2) {
	sprintf (key_2, "\t%d\t", get_klist[line].keycode2);
	result[3] = key_2;
	*tagged = 1;
    } else
	result[3] = key_s;

    return result;
}

static void move_down (struct key_list k_list[], CWidget * w)
{E_
    int i, j;
    CTextboxCursorMove (w, CK_Down);
    for (j = 0; j < 6; j++) {
	i = w->cursor;
	if (k_list[i].key_name[0])
	    if (*(k_list[i].key_name) == '\t')
		CTextboxCursorMove (w, CK_Down);
    }
}

int cb_learnkeys (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    int i;
    KeySym x_key;

/* we must ignore control, alt, etc. */
    x_key = CKeySym (xe);

    if (mod_type_key (x_key))
	return 0;

    i = w->cursor;

    if (*(klist[i].key_name) != '\t') {
	if (xe->type == KeyPress) {
	    if (!klist[i].keycode0) {
		klist[i].keycode0 = xe->xkey.keycode;
		klist[i].state0 = xe->xkey.state;
	    } else if (!klist[i].keycode1) {
		klist[i].keycode1 = xe->xkey.keycode;
		klist[i].state1 = xe->xkey.state;
	    } else if (!klist[i].keycode2) {
		klist[i].keycode2 = xe->xkey.keycode;
		klist[i].state2 = xe->xkey.state;
	    }
	    move_down (klist, w);
	    CExpose ("_learnkeysbox");
	}
    } else {
	move_down (klist, w);
	CExpose ("_learnkeysbox");
    }
    return 1;			/* always handled. This will stop the tab key from being seen */
}


int cb_save (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    if (save_user_defined_keys (klist, editor_options_file))
	CErrorDialog (main_window, 20, 20, _(" Save keys "), get_sys_error (_(" Error trying to save file ")));
    return 0;
}

int cb_clear (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    int i;
    for (i = 0; klist[i].key_name[0]; i++) {
	klist[i].state0 = 0;
	klist[i].state1 = 0;
	klist[i].state2 = 0;
	klist[i].keycode0 = 0;
	klist[i].keycode1 = 0;
	klist[i].keycode2 = 0;
    }
    CFocus (CIdent ("_learnkeysbox"));
    CExpose ("_learnkeysbox");
    return 0;
}

int cb_clearline (CWidget * w, XEvent * xe, CEvent * ce)
{E_
    int i;
    w = CIdent ("_learnkeysbox");
    i = w->cursor;
    klist[i].state0 = 0;
    klist[i].state1 = 0;
    klist[i].state2 = 0;
    klist[i].keycode0 = 0;
    klist[i].keycode1 = 0;
    klist[i].keycode2 = 0;
    move_down (klist, w);
    CFocus (w);
    CExpose ("_learnkeysbox");
    return 0;
}

/* only allowed to draw one of these */
CWidget *Cdrawlearnkeys (Window parent, int x, int y, int columns, int lines)
{E_
    CWidget *w;

    CPushFont ("editor", 0);
    w = CDrawFieldedTextbox ("_learnkeysbox", parent, x, y,
	    AUTO_WIDTH, lines * FONT_PIX_PER_LINE + 6,
			     0, 0, get_key_text, TEXTBOX_NO_KEYS, (void *) klist);
    CPopFont ();
/* Tool hint */
    CSetToolHint ("_learnkeysbox", _("Click on an editing action and the press the key to bind it to"));

    CAddCallback ("_learnkeysbox", cb_learnkeys);
    CGetHintPos (0, &y);
    (CDrawButton ("_learnkeysbox.save", parent, x, y,
		  AUTO_WIDTH, AUTO_HEIGHT, " Save "))->takes_focus = 0;
/* Tool hint */
    CSetToolHint ("_learnkeysbox.save", _("Save key defines to your initialisation file"));
    CAddCallback ("_learnkeysbox.save", cb_save);
    CGetHintPos (&x, 0);
    (CDrawButton ("_learnkeysbox.clear", parent, x, y,
	       AUTO_WIDTH, AUTO_HEIGHT, _(" Clear all ")))->takes_focus = 0;
/* Tool hint */
    CSetToolHint ("_learnkeysbox.clear", _("Erase all user key definitions"));
    CAddCallback ("_learnkeysbox.clear", cb_clear);
    CGetHintPos (&x, 0);
    (CDrawButton ("_learnkeysbox.clearline", parent, x, y,
	      AUTO_WIDTH, AUTO_HEIGHT, _(" Clear line ")))->takes_focus = 0;
/* Tool hint */
    CSetToolHint ("_learnkeysbox.clearline", _("Erase key definition on this line"));
    CAddCallback ("_learnkeysbox.clearline", cb_clearline);
    return w;
}
