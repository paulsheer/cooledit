/* editmenu.c
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include "edit.h"

#include "editcmddef.h"


#ifdef MIDNIGHT


extern int edit_key_emulation;
extern WEdit *wedit;
extern WButtonBar *edit_bar;
extern Dlg_head *edit_dlg;
extern WMenu *edit_menubar;

#undef edit_message_dialog
#define edit_message_dialog(w,x,y,h,s) query_dialog (h, s, 0, 1, _("&Ok"))
#define CFocus(x) 

static void menu_cmd (int i)
{
    send_message (wedit->widget.parent, (Widget *) wedit, WIDGET_COMMAND, i);
}

static void menu_key (int i)
{
    send_message (wedit->widget.parent, (Widget *) wedit, WIDGET_KEY, i);
}

void edit_wrap_cmd ()
{
    char *f;
    char s[12];
    sprintf (s, "%d", option_word_wrap_line_length);
    f = input_dialog (_(" Word wrap "), 
/* Not essential to translate */
    _(" Enter line length, 0 for off: "), s);
    if (f) {
	if (*f) {
	    option_word_wrap_line_length = atoi (f);
	    free (f);
	}
    }
}

void edit_about_cmd ()
{
    edit_message_dialog (wedit->mainid, 20, 20, " About ",
		      "\n"
		      "                Cooledit  v3.11.5\n"
		      "\n"
		      " Copyright (C) 1996-2000 the Free Software Foundation\n"
		      "\n"
		      "       A user friendly text editor written\n"
		      "           for the Midnight Commander.\n"
	);
}

void menu_mail_cmd (void)		{ menu_cmd (CK_Mail); }
void menu_load_cmd (void)		{ menu_cmd (CK_Load); }
void menu_new_cmd (void)		{ menu_cmd (CK_New); }
void menu_save_cmd (void)		{ menu_cmd (CK_Save); }
void menu_save_as_cmd (void)		{ menu_cmd (CK_Save_As); }
void menu_insert_file_cmd (void)	{ menu_cmd (CK_Insert_File); }
void menu_quit_cmd (void)		{ menu_cmd (CK_Exit); }
void menu_mark_cmd (void)		{ menu_cmd (CK_Mark); }
void menu_markcol_cmd (void)		{ menu_cmd (CK_Column_Mark); }
void menu_ins_cmd (void)		{ menu_cmd (CK_Toggle_Insert); }
void menu_copy_cmd (void)		{ menu_cmd (CK_Copy); }
void menu_move_cmd (void)		{ menu_cmd (CK_Move); }
void menu_delete_cmd (void)		{ menu_cmd (CK_Remove); }
void menu_cut_cmd (void)		{ menu_cmd (CK_Save_Block); }
void menu_search_cmd (void)		{ menu_cmd (CK_Find); }
void menu_search_again_cmd (void)	{ menu_cmd (CK_Find_Again); }
void menu_replace_cmd (void)		{ menu_cmd (CK_Replace); }
void menu_begin_record_cmd (void)	{ menu_cmd (CK_Begin_Record_Macro); }
void menu_end_record_cmd (void)		{ menu_cmd (CK_End_Record_Macro); }
void menu_wrap_cmd (void)		{ edit_wrap_cmd (); }
void menu_exec_macro_cmd (void)		{ menu_key (XCTRL ('a')); }
void menu_exec_macro_delete_cmd (void)	{ menu_cmd (CK_Delete_Macro); }
void menu_c_form_cmd (void)		{ menu_key (KEY_F (19)); }
void menu_ispell_cmd (void)		{ menu_cmd (CK_Pipe_Block (1)); }
void menu_sort_cmd (void)		{ menu_cmd (CK_Sort); }
void menu_date_cmd (void)		{ menu_cmd (CK_Date); }
void menu_undo_cmd (void)		{ menu_cmd (CK_Undo); }
void menu_beginning_cmd (void)		{ menu_cmd (CK_Beginning_Of_Text); }
void menu_end_cmd (void)		{ menu_cmd (CK_End_Of_Text); }
void menu_refresh_cmd (void)		{ menu_cmd (CK_Refresh); }
void menu_goto_line (void)		{ menu_cmd (CK_Goto); }
void menu_goto_bracket (void)		{ menu_cmd (CK_Match_Bracket); }
void menu_lit_cmd (void)		{ menu_key (XCTRL ('q')); }
void menu_format_paragraph (void)	{ menu_cmd (CK_Paragraph_Format); }
void edit_options_dialog (void);
void menu_options (void)		{ edit_options_dialog (); }

static menu_entry FileMenu[] =
{
    {' ', N_("&Open/load...     C-o"), 'O', menu_load_cmd},
    {' ', N_("&New              C-n"), 'N', menu_new_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Save              F2"), 'S', menu_save_cmd},
    {' ', N_("save &As...       F12"), 'A', menu_save_as_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Insert file...   F15"), 'I', menu_insert_file_cmd},
    {' ', N_("copy to &File...  C-f"), 'F', menu_cut_cmd},
    {' ', "", ' ', 0},
    {' ', N_("a&Bout...            "), 'B', edit_about_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Quit             F10"), 'Q', menu_quit_cmd}
 };

static menu_entry FileMenuEmacs[] =
{
    {' ', N_("&Open/load...     C-o"), 'O', menu_load_cmd},
    {' ', N_("&New            C-x k"), 'N', menu_new_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Save              F2"), 'S', menu_save_cmd},
    {' ', N_("save &As...       F12"), 'A', menu_save_as_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Insert file...   F15"), 'I', menu_insert_file_cmd},
    {' ', N_("copy to &File...     "), 'F', menu_cut_cmd},
    {' ', "", ' ', 0},
    {' ', N_("a&Bout...            "), 'B', edit_about_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Quit             F10"), 'Q', menu_quit_cmd}
};

static menu_entry EditMenu[] =
{
    {' ', N_("&Toggle Mark       F3"), 'T', menu_mark_cmd},
    {' ', N_("&Mark Columns    S-F3"), 'T', menu_markcol_cmd},
    {' ', "", ' ', 0},
    {' ', N_("toggle &Ins/overw Ins"), 'I', menu_ins_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Copy              F5"), 'C', menu_copy_cmd},
    {' ', N_("&Move              F6"), 'M', menu_move_cmd},
    {' ', N_("&Delete            F8"), 'D', menu_delete_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Undo             C-u"), 'U', menu_undo_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Beginning     C-PgUp"), 'B', menu_beginning_cmd},
    {' ', N_("&End           C-PgDn"), 'E', menu_end_cmd}
};

static menu_entry EditMenuEmacs[] =
{
    {' ', N_("&Toggle Mark       F3"), 'T', menu_mark_cmd},
    {' ', N_("&Mark Columns    S-F3"), 'T', menu_markcol_cmd},
    {' ', "", ' ', 0},
    {' ', N_("toggle &Ins/overw Ins"), 'I', menu_ins_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Copy              F5"), 'C', menu_copy_cmd},
    {' ', N_("&Move              F6"), 'M', menu_move_cmd},
    {' ', N_("&Delete            F8"), 'D', menu_delete_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Undo             C-u"), 'U', menu_undo_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Beginning     C-PgUp"), 'B', menu_beginning_cmd},
    {' ', N_("&End           C-PgDn"), 'E', menu_end_cmd}
};

static menu_entry SearReplMenu[] =
{
    {' ', N_("&Search...         F7"), 'S', menu_search_cmd},
    {' ', N_("search &Again     F17"), 'A', menu_search_again_cmd},
    {' ', N_("&Replace...        F4"), 'R', menu_replace_cmd}
};

static menu_entry SearReplMenuEmacs[] =
{
    {' ', N_("&Search...         F7"), 'S', menu_search_cmd},
    {' ', N_("search &Again     F17"), 'A', menu_search_again_cmd},
    {' ', N_("&Replace...        F4"), 'R', menu_replace_cmd}
};

static menu_entry CmdMenu[] =
{
    {' ', N_("&Goto line...            M-l"), 'G', menu_goto_line},
    {' ', N_("goto matching &Bracket   M-b"), 'B', menu_goto_bracket},
    {' ', "", ' ', 0},
    {' ', N_("insert &Literal...       C-q"), 'L', menu_lit_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Refresh screen          C-l"), 'R', menu_refresh_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Start record macro      C-r"), 'S', menu_begin_record_cmd},
    {' ', N_("&Finish record macro...  C-r"), 'F', menu_end_record_cmd},
    {' ', N_("&Execute macro...   C-a, KEY"), 'E', menu_exec_macro_cmd},
    {' ', N_("delete macr&O...            "), 'O', menu_exec_macro_delete_cmd},
    {' ', "", ' ', 0},
    {' ', N_("insert &Date/time           "), 'D', menu_date_cmd},
    {' ', "", ' ', 0},
    {' ', N_("format p&Aragraph        M-p"), 'A', menu_format_paragraph},
    {' ', N_("'ispell' s&Pell check    C-p"), 'P', menu_ispell_cmd},
    {' ', N_("sor&T...                 M-t"), 'T', menu_sort_cmd},
    {' ', N_("'indent' &C Formatter    F19"), 'C', menu_c_form_cmd},
    {' ', N_("&Mail...                    "), 'M', menu_mail_cmd}
};

static menu_entry CmdMenuEmacs[] =
{
    {' ', N_("&Goto line...            M-l"), 'G', menu_goto_line},
    {' ', N_("goto matching &Bracket   M-b"), 'B', menu_goto_bracket},
    {' ', "", ' ', 0},
    {' ', N_("insert &Literal...       C-q"), 'L', menu_lit_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Refresh screen          C-l"), 'R', menu_refresh_cmd},
    {' ', "", ' ', 0},
    {' ', N_("&Start record macro      C-r"), 'S', menu_begin_record_cmd},
    {' ', N_("&Finish record macro...  C-r"), 'F', menu_end_record_cmd},
    {' ', N_("&Execute macro... C-x e, KEY"), 'E', menu_exec_macro_cmd},
    {' ', N_("delete macr&O...            "), 'o', menu_exec_macro_delete_cmd},
    {' ', "", ' ', 0},
    {' ', N_("insert &Date/time           "), 'D', menu_date_cmd},
    {' ', "", ' ', 0},
    {' ', N_("format p&Aragraph        M-p"), 'a', menu_format_paragraph},
    {' ', N_("'ispell' s&Pell check    M-$"), 'P', menu_ispell_cmd},
    {' ', N_("sor&T...                 M-t"), 'T', menu_sort_cmd},
    {' ', N_("'indent' &C Formatter    F19"), 'C', menu_c_form_cmd}
};

extern void menu_save_mode_cmd (void);

static menu_entry OptMenu[] =
{
    {' ', N_("&General...  "), 'G', menu_options},
    {' ', N_("&Save mode..."), 'S', menu_save_mode_cmd}
#if 0
    {' ', N_("&Layout..."),    'L', menu_layout_cmd}
#endif
};

static menu_entry OptMenuEmacs[] =
{
    {' ', N_("&General...  "), 'G', menu_options},
    {' ', N_("&Save mode..."), 'S', menu_save_mode_cmd}
#if 0
    {' ', N_("&Layout..."),    'L', menu_layout_cmd}
#endif
};

#define menu_entries(x) sizeof(x)/sizeof(menu_entry)

Menu EditMenuBar[N_menus];

void edit_init_menu_normal (void)
{
    EditMenuBar[0] = create_menu (_(" File "), FileMenu, menu_entries (FileMenu));
    EditMenuBar[1] = create_menu (_(" Edit "), EditMenu, menu_entries (EditMenu));
    EditMenuBar[2] = create_menu (_(" Sear/Repl "), SearReplMenu, menu_entries (SearReplMenu));
    EditMenuBar[3] = create_menu (_(" Command "), CmdMenu, menu_entries (CmdMenu));
    EditMenuBar[4] = create_menu (_(" Options "), OptMenu, menu_entries (OptMenu));
}

void edit_init_menu_emacs (void)
{
    EditMenuBar[0] = create_menu (_(" File "), FileMenuEmacs, menu_entries (FileMenuEmacs));
    EditMenuBar[1] = create_menu (_(" Edit "), EditMenuEmacs, menu_entries (EditMenuEmacs));
    EditMenuBar[2] = create_menu (_(" Sear/Repl "), SearReplMenuEmacs, menu_entries (SearReplMenuEmacs));
    EditMenuBar[3] = create_menu (_(" Command "), CmdMenuEmacs, menu_entries (CmdMenuEmacs));
    EditMenuBar[4] = create_menu (_(" Options "), OptMenuEmacs, menu_entries (OptMenuEmacs));
}

void edit_done_menu (void)
{
    int i;
    for (i = 0; i < N_menus; i++)
	destroy_menu (EditMenuBar[i]);
}


void edit_drop_menu_cmd (WEdit * e, int which)
{
    if (edit_menubar->active)
	return;
    edit_menubar->active = 1;
    edit_menubar->dropped = drop_menus;
    edit_menubar->previous_selection = which >= 0 ? which : dlg_item_number (edit_dlg);
    if (which >= 0)
	edit_menubar->selected = which;
    dlg_select_widget (edit_dlg, edit_menubar);
}


void edit_menu_cmd (WEdit * e)
{
    edit_drop_menu_cmd (e, -1);
}


int edit_drop_hotkey_menu (WEdit * e, int key)
{
    int m = 0;
    switch (key) {
    case ALT ('f'):
	if (edit_key_emulation == EDIT_KEY_EMULATION_EMACS)
	    return 0;
	m = 0;
	break;
    case ALT ('e'):
	m = 1;
	break;
    case ALT ('s'):
	m = 2;
	break;
    case ALT ('c'):
	m = 3;
	break;
    case ALT ('o'):
	m = 4;
	break;
    default:
	return 0;
    }

    edit_drop_menu_cmd (e, m);
    return 1;
}


#else				/* !MIDNIGHT */


extern CWidget *wedit;

void CSetEditMenu (const char *ident)
{
    wedit = CIdent (ident);
}

CWidget *CGetEditMenu (void)
{
    return wedit;
}

static void menu_cmd (unsigned long i)
{
    XEvent e;
    if (wedit) {
	memset (&e, 0, sizeof (XEvent));
	e.type = EditorCommand;
	e.xkey.keycode = i;
	e.xkey.window = wedit->winid;
	CFocus (wedit);
	CSendEvent (&e);
    }
}

void CEditMenuCommand (int i)
{
    menu_cmd ((unsigned long) i);
}

static void menu_key (KeySym i, int state)
{
    int cmd;
    if (edit_translate_key (0, i, state, &cmd, 0, 0)) {
	if (cmd > 0)
	    menu_cmd (cmd);
    }
}

static void menu_ctrl_key (unsigned long i)
{
    menu_key ((KeySym) i, ControlMask);
}

void CDrawEditMenuButtons (const char *ident, Window parent, Window focus_return, int x, int y)
{
    int d;

    CDrawMenuButton (catstrs (ident, ".filemenu", NULL), parent, focus_return, x, y, AUTO_WIDTH, AUTO_HEIGHT, 8,
/* The following are menu options. Do not change the key bindings (eg. Ctrl-o) and preserve '\t' */
		     _(" File "),
		     _("Open...\tCtrl-o"), (int) '~', menu_cmd, (unsigned long) CK_Load,
		     _("New\tCtrl-n"), (int) '~', menu_cmd, (unsigned long) CK_New,
		     "", (int) ' ', (void *) 0, 0L,
		     _("Save\tF2"), (int) '~', menu_cmd, (unsigned long) CK_Save,
		     _("Save as...\tF12"), (int) '~', menu_cmd, (unsigned long) CK_Save_As,
		     "", (int) ' ', (void *) 0, 0L,
		     _("Insert file...\tF15"), (int) '~', menu_cmd, (unsigned long) CK_Insert_File,
		     _("Copy to file...\tCtrl-f"), (int) '~', menu_cmd, (unsigned long) CK_Save_Block
	);
/* Tool hint */
    CSetToolHint (catstrs (ident, ".filemenu", NULL), _("Disk operations and file indexing/searching"));

    CGetHintPos (&x, &d);

    CDrawMenuButton (catstrs (ident, ".editmenu", NULL), parent, focus_return, x, y, AUTO_WIDTH, AUTO_HEIGHT, 20,
		     _(" Edit "),
		     _("Toggle mark\tF3"), (int) '~', menu_cmd, (unsigned long) CK_Mark,
		     _("Toggle mark columns\tCtrl-b"), (int) '~', menu_cmd, (unsigned long) CK_Column_Mark,
		     "", (int) ' ', (void *) 0, 0L,
		     _("Toggle book mark\tCtrl-Alt-Ins"), (int) '~', menu_cmd, (unsigned long) CK_Toggle_Bookmark,
		     _("Previous book mark\tCtrl-Alt-Up"), (int) '~', menu_cmd, (unsigned long) CK_Prev_Bookmark,
		     _("Next book mark\tCtrl-Alt-Down"), (int) '~', menu_cmd, (unsigned long) CK_Next_Bookmark,
		     _("Flush book marks"), (int) '~', menu_cmd, (unsigned long) CK_Flush_Bookmarks,
		     "", (int) ' ', (void *) 0, 0L,
		     _("Toggle insert/overwrite\tIns"), (int) '~', menu_cmd, (unsigned long) CK_Toggle_Insert,
		     "", (int) ' ', (void *) 0, 0L,
		     _("Copy block to cursor\tF5"), (int) '~', menu_cmd, (unsigned long) CK_Copy,
		     _("Move block to cursor\tF6"), (int) '~', menu_cmd, (unsigned long) CK_Move,
		     _("Delete block\tF8/Ctrl-Del"), (int) '~', menu_cmd, (unsigned long) CK_Remove,
		     "", (int) ' ', (void *) 0, 0L,
		     _("Copy block to clipbrd\tCtrl-Ins"), (int) '~', menu_cmd, (unsigned long) CK_XStore,
		     _("Cut block to clipbrd\tShift-Del"), (int) '~', menu_cmd, (unsigned long) CK_XCut,
		     _("Paste block from clipbrd\tShift-Ins"), (int) '~', menu_cmd, (unsigned long) CK_XPaste,
		     _("Selection history...\tAlt-Ins"), (int) '~', menu_cmd, (unsigned long) CK_Selection_History,
		     "", (int) ' ', (void *) 0, 0L,
		     _("Undo\tCtrl-BackSpace"), (int) '~', menu_cmd, (unsigned long) CK_Undo
	);
/* Tool hint */
    CSetToolHint (catstrs (ident, ".editmenu", NULL), _("Manipulating blocks of text"));

    CGetHintPos (&x, &d);

    CDrawMenuButton (catstrs (ident, ".searchmenu", NULL), parent, focus_return, x, y, AUTO_WIDTH, AUTO_HEIGHT, 4,
		     _(" Srch/Replce "),
		     _("Search...\tF7"), (int) '~', menu_cmd, (unsigned long) CK_Find,
		     _("Search again\tF17"), (int) '~', menu_cmd, (unsigned long) CK_Find_Again,
		     _("Replace...\tF4"), (int) '~', menu_cmd, (unsigned long) CK_Replace,
		     _("Replace again\tF14"), (int) '~', menu_cmd, (unsigned long) CK_Replace_Again
	);
/* Tool hint */
    CSetToolHint (catstrs (ident, ".searchmenu", NULL), _("Search for and replace text"));

    CGetHintPos (&x, &d);

    CDrawMenuButton (catstrs (ident, ".commandmenu", NULL), parent, focus_return, x, y, AUTO_WIDTH, AUTO_HEIGHT, 12,
		     _(" Command "),
		     _("Goto line...\tAlt-l"), (int) '~', menu_cmd, (unsigned long) CK_Goto,
		     _("Goto matching bracket\tAlt-b"), (int) '~', menu_cmd, (unsigned long) CK_Match_Bracket,
		     "", (int) ' ', (void *) 0, 0L,
		     _("Start record macro\tCtrl-r"), (int) '~', menu_cmd, (unsigned long) CK_Begin_Record_Macro,
		     _("Finish record macro...\tCtrl-r"), (int) '~', menu_cmd, (unsigned long) CK_End_Record_Macro,
		     _("Execute macro...\tCtrl-a, KEY"), (int) '~', menu_ctrl_key, (unsigned long) XK_a,
		     _("Delete macro...\t"), (int) '~', menu_cmd, (unsigned long) CK_Delete_Macro,
		     "", (int) ' ', (void *) 0, 0L,
		     _("Insert date/time\tCtrl-d"), (int) '~', menu_cmd, (unsigned long) CK_Date,
		     _("Format paragraph\tAlt-p"), (int) '~', menu_cmd, (unsigned long) CK_Paragraph_Format,
		     "", (int) ' ', (void *) 0, 0L,
		     _("Refresh display\tCtrl-l"), (int) '~', menu_cmd, (unsigned long) CK_Refresh
	);
/* Tool hint */
    CSetToolHint (catstrs (ident, ".commandmenu", NULL), _("Macros and internal commands"));
}


#endif				/* !MIDNIGHT */

