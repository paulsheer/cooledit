/* cooleditmenus.c - additional menu items for cooledit
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include "edit.h"

#include "editcmddef.h"
#include "copyright.h"
#include "pool.h"

extern Window main_window;

void menu_man_cmd (unsigned long ignore)		{ edit_man_page_cmd (CGetEditMenu()->editor); }
void menu_print_dialog (unsigned long ignore)		{ cooledit_print_dialog (CGetEditMenu()->editor); }
void menu_insert_shell_output (unsigned long ignore)	{ edit_insert_shell_output (CGetEditMenu()->editor); }

void menu_jump_to_file (unsigned long ignored);
void edit_a_script_cmd (unsigned long ignored);
void delete_a_script_cmd (unsigned long ignored);
void new_script_cmd (unsigned long ignored);
#ifdef HAVE_PYTHON
void menu_python_reload (unsigned long ignored);
#endif
CWidget *CManpageDialog (Window in, int x, int y, int width, int height, const char *manpage);

void edit_man_cooledit (unsigned long ignored)
{
    CWidget *w;
    w = CManpageDialog (0, 0, 0, 80, 25, "cooledit");
    if (w)
	w->position |= WINDOW_UNMOVEABLE;
    CFocus (CIdent ("mandisplayfile.text"));
}

#if 0
void edit_gnu_license (unsigned long line)
{
    POOL *p;
    int i;
    p = pool_init ();
    for (i = 0; copyright[i]; i++)
	pool_printf (p, "%s\n", copyright[i]);
    pool_null (p);
    CTextboxMessageDialog (main_window, 20, 20, 77, 23, _ (" Copying "), (char *) pool_start (p), (int) line);
    pool_free (p);
}
#endif

static char **latin1_get_line (void *data, int line_number, int *num_fields, int *tagged)
{
    *num_fields = 5;
    *tagged = 0;
    if (line_number >= sizeof(latin1_help) / sizeof (latin1_help[0]))
        return 0;
    return latin1_help[line_number];
}

void latin1_help_table (unsigned long line)
{
    CFieldedTextboxMessageDialog (main_window, 20, 20, 77, 23, _ (" Latin 1 Composing "), latin1_get_line, 0, (void *) 1);
}

void menu_change_directory_cmd (unsigned long ignore)
{
    edit_change_directory ();
}

CWidget *Cdrawlearnkeys (Window parent, int x, int y, int columns, int lines);

void menu_define_key_cmd (unsigned long ignored)
{
    Window win;
    int x, y;
    CState s;
    CEvent cwevent;
    CBackupState (&s);
    CDisable ("*");
    win = CDrawHeadedDialog ("definekey", main_window, 20, 20, _(" Define Keys "));
    CGetHintPos (&x, &y);
    Cdrawlearnkeys (win, x, y, 40, 25);
    get_hint_limits (&x, &y);
    CDrawPixmapButton ("_clickhere", win, x / 2 - 24, y, PIXMAP_BUTTON_TICK);
    CSetToolHint ("_clickhere", _("Accept key defines and close"));
    CSetSizeHintPos ("definekey");
    CMapDialog ("definekey");
    CFocus (CIdent("_clickhere"));
    do {
	CNextEvent (NULL, &cwevent);
    } while (strcmp (cwevent.ident, "_clickhere"));
    CDestroyWidget ("definekey");
    CRestoreState (&s);
}

void menu_general_options_cmd (unsigned long ignored)
{
    draw_options_dialog (CRoot, 20, 20);
}

void menu_switches_options_cmd (unsigned long ignored)
{
    draw_switches_dialog (CRoot, 20, 20);
}

void save_mode_options_dialog (Window parent, int x, int y);
void menu_syntax_highlighting_dialog  (Window parent, int x, int y);

void menu_save_mode_cmd (unsigned long ignored)
{
    save_mode_options_dialog (main_window, 20, 20);
}

void menu_syntax_highlighting_cmd (unsigned long ignored)
{
    menu_syntax_highlighting_dialog (main_window, 20, 20);
}

void edit_about_cmd (unsigned long ignored)
{
    CMessageDialog (main_window, 20, 20, TEXT_CENTRED, _(" About "),
      _("\n" \
      "Cooledit  version  %s\n" \
      "\n" \
      "A user-friendly text editor written for The X Window System.\n" \
      "\n" \
      "Copyright (C) 1996-2018 Paul Sheer\n" \
      "\n"), VERSION);
}

void bookmark_select (void);

void CDrawCooleditMenuButtons (Window parent, int x, int y)
{
    CGetHintPos (&x, 0);

    CDrawMenuButton ("menu.optionsmenu", parent, main_window, x, y, AUTO_SIZE, 5,
		     _(" Options "),
		     _("Define keys..."), (int) '~', menu_define_key_cmd, 0L,
		     _("General..."), (int) '~', menu_general_options_cmd, 0L,
		     _("Switches..."), (int) '~', menu_switches_options_cmd, 0L,
		     _("Save mode..."), (int) '~', menu_save_mode_cmd, 0L,
		     _("Syntax highlighting..."), (int) '~', menu_syntax_highlighting_cmd, 0L
	);
/* Toolhint for the 'Options' menu button */
    CSetToolHint ("menu.optionsmenu", _("Configure, redefine keys"));

    CGetHintPos (&x, 0);
    CDrawMenuButton ("menu.scripts", parent, main_window, x, y, AUTO_SIZE, 4,
		     _(" Scripts "),
		     _("Edit a script..."), (int) '~', edit_a_script_cmd, 0L,
		   _("Delete a script..."), (int) '~', delete_a_script_cmd, 0L,
		     _("New script..."), (int) '~', new_script_cmd, 0L,
		     "", (int) ' ', 0, 0
	);
/* Toolhint for the 'Scripts' menu button */
    CSetToolHint ("menu.scripts", _("Launch an application"));

    CGetHintPos (&x, 0);
    debug_menus (parent, main_window, x, y);

    CGetHintPos (&x, 0);
    CDrawMenuButton ("menu.readme", parent, main_window, x, y, AUTO_SIZE, 3,
		     _(" Readme "),
		     _("Help..."), (int) '~', edit_man_cooledit, 0L,
		     _("About..."), (int) '~', edit_about_cmd, 0L,
		     _("Latin1 Key composing..."), (int) '~', latin1_help_table, 0L
	);
    CGetHintPos (&x, &y);
    set_hint_pos (x + WIDGET_SPACING, y);

/* Toolhint for the 'Readme' menu button */
    CSetToolHint ("menu.readme", _("Help, Copyright and useful info"));
#ifdef HAVE_PYTHON
    CAddMenuItem ("menu.commandmenu", "", ' ', (void *) 0, 0L);
    CAddMenuItem ("menu.commandmenu",
		  _("Reload Python scripts"), '~', menu_python_reload, 0L);
#endif
    CAddMenuItem ("menu.commandmenu", "", ' ', (void *) 0, 0L);
    CAddMenuItem ("menu.commandmenu",
		_("Shell command\tEscape"), '~', (void (*) (unsigned long data)) menu_insert_shell_output, (unsigned long) 0);
    CAddMenuItem ("menu.commandmenu",
		_("Terminal\tShift-F1"), '~', (void (*) (unsigned long data)) CEditMenuCommand, (unsigned long) CK_Terminal);
    CAddMenuItem ("menu.commandmenu",
		  _("Show manual page...\tCtrl-F1"), '~', menu_man_cmd, 0L);
    CAddMenuItem ("menu.commandmenu",
	      _("Change current directory..."), '~', menu_change_directory_cmd, 0L);
    CAddMenuItem ("menu.commandmenu",
		_("Complete\tCtrl-Tab"), '~', (void (*) (unsigned long data)) CEditMenuCommand, (unsigned long) CK_Complete);
    CAddMenuItem ("menu.commandmenu",
		_("Insert unicode...\tAlt-i"), '~', (void (*) (unsigned long data)) CEditMenuCommand, (unsigned long) CK_Insert_Unicode);
    CAddMenuItem ("menu.commandmenu",
		_("paragraph indent mode\tShift-F6/F16"), '~', (void (*) (unsigned long data)) CEditMenuCommand, (unsigned long) CK_Paragraph_Indent_Mode);

    CInsertMenuItemAfter ("menu.filemenu", _("New"),
		_("Jump to file\tCtrl-j"), '~', menu_jump_to_file, (unsigned long) CK_Jump_To_File);
    CAddMenuItem ("menu.filemenu", "", ' ', (void *) 0, 0);
    CAddMenuItem ("menu.filemenu",
		_("Print..."), '~', menu_print_dialog, 0L);
    CAddMenuItem ("menu.filemenu", "", ' ', (void *) 0, 0);
    CAddMenuItem ("menu.filemenu",
		_("Find file...\tCtrl-Alt-f"), '~', (void (*) (unsigned long data)) CEditMenuCommand, (unsigned long) CK_Find_File);
    CAddMenuItem ("menu.filemenu",
		_("Ctags code index...\tCtrl-Alt-i"), '~', (void (*) (unsigned long data)) CEditMenuCommand, (unsigned long) CK_Ctags);

    CAddMenuItem ("menu.editmenu", "", ' ', (void *) 0, 0L);
    CAddMenuItem ("menu.editmenu",
		_("List bookmarks..."), '~', (void (*) (unsigned long data)) bookmark_select, 0L);
}


