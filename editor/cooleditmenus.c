/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* cooleditmenus.c - additional menu items for cooledit
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include "edit.h"

#include "manpage.h"
#include "debug.h"
#include "shell.h"
#include "print.h"
#include "editoptions.h"
#include "editcmddef.h"
#include "edit.h"
#include "copyright.h"
#include "pool.h"

extern Window main_window;

void bookmark_select (void);

void menu_man_cmd (unsigned long ignore)		{ edit_man_page_cmd (CGetEditMenu()->editor); }
void menu_print_dialog (unsigned long ignore)		{ cooledit_print_dialog (CGetEditMenu()->editor); }
void menu_insert_shell_output (unsigned long ignore)	{ edit_insert_shell_output (CGetEditMenu()->editor); }
void menu_bookmark_select (unsigned long ignore)	{ bookmark_select (); }

void menu_jump_to_file (unsigned long ignored);
void edit_a_script_cmd (unsigned long ignored);
void delete_a_script_cmd (unsigned long ignored);
void new_script_cmd (unsigned long ignored);
#ifdef HAVE_PYTHON
void menu_python_reload (unsigned long ignored);
#endif

void edit_man_cooledit (unsigned long ignored)
{E_
    CWidget *w;
    w = CManpageDialog (0, 0, 0, 80, 25, "cooledit");
    if (w)
	w->position |= WINDOW_UNMOVEABLE;
    CFocus (CIdent ("mandisplayfile.text"));
}

#if 0
void edit_gnu_license (unsigned long line)
{E_
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

struct latin_help_info {
#define LATIN_HELP_INFO_MAGIC   0x32409857
    unsigned int magic;
    char ***r;
    int n;
};

static char **latin_get_line (void *data, int line_number, int *num_fields, int *tagged)
{E_
    struct latin_help_info *w;
    w = (struct latin_help_info *) data;
    assert (w->magic == LATIN_HELP_INFO_MAGIC);
    *num_fields = 5;
    *tagged = 0;
    assert (line_number >= 0);
    if (line_number >= w->n)
        return 0;
    return w->r[line_number];
}

void latin_help_table (unsigned long line)
{E_
    const char *msg = " Non-ASCII Key Composing to Unicode: use Alt-\\ and any order two keys... \n (See COMPOSING INTERNATIONAL CHARACTERS in the man page for non-Latin composing) ";
    struct latin_help_info w;
    memset (&w, '\0', sizeof (w));
    w.magic = LATIN_HELP_INFO_MAGIC;
    compose_help (&w.r, &w.n);
    CFieldedTextboxMessageDialog (main_window, 20, 20, 77, 23, msg, latin_get_line, 0, (void *) &w);
    free_compose_help (w.r, w.n);
}

void menu_change_directory_cmd (unsigned long ignore)
{E_
    edit_change_directory ();
}

CWidget *Cdrawlearnkeys (Window parent, int x, int y, int columns, int lines);

void menu_define_key_cmd (unsigned long ignored)
{E_
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
{E_
    draw_options_dialog (CRoot, 20, 20);
}

void menu_switches_options_cmd (unsigned long ignored)
{E_
    draw_switches_dialog (CRoot, 20, 20);
}

void save_mode_options_dialog (Window parent, int x, int y);
void menu_syntax_highlighting_dialog  (Window parent, int x, int y);

void menu_save_mode_cmd (unsigned long ignored)
{E_
    save_mode_options_dialog (main_window, 20, 20);
}

void menu_syntax_highlighting_cmd (unsigned long ignored)
{E_
    menu_syntax_highlighting_dialog (main_window, 20, 20);
}

void edit_about_cmd (unsigned long ignored)
{E_
    CMessageDialog (main_window, 20, 20, TEXT_CENTRED, _(" About "),
      _("\n" \
      "Cooledit  version  %s\n" \
      "\n" \
      "A user-friendly text editor written for The X Window System.\n" \
      "\n" \
      "Copyright (C) 1996-2022 Paul Sheer\n" \
      "\n"), VERSION);
}


void CDrawCooleditMenuButtons (Window parent, int x, int y)
{E_
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
		     _("Non-ASCII key composing..."), (int) '~', latin_help_table, 0L
	);
    CGetHintPos (&x, &y);
    set_hint_pos (x + WIDGET_SPACING, y);

/* Toolhint for the 'Readme' menu button */
    CSetToolHint ("menu.readme", _("Help, Copyright and useful info"));
#ifdef HAVE_PYTHON
    CAddMenuItem ("menu.commandmenu", "", ' ', (callfn) NULL, 0);
    CAddMenuItem ("menu.commandmenu", _("Reload Python scripts"), '~', menu_python_reload, 0);
#endif
    CAddMenuItem ("menu.commandmenu", "", ' ', (callfn) NULL, 0);
    CAddMenuItem ("menu.commandmenu", _("Shell command\tEscape"), '~', menu_insert_shell_output, 0);
    CAddMenuItem ("menu.commandmenu", _("Terminal\tF11/Shift-F1"), '~', CEditMenuCommand, CK_Terminal);
    CAddMenuItem ("menu.commandmenu", _("8-Bit Terminal"), 0, CEditMenuCommand, CK_8BitTerminal);
    CAddMenuItem ("menu.commandmenu", _("Show manual page...\tCtrl-F1"), '~', menu_man_cmd, 0);
    CAddMenuItem ("menu.commandmenu", _("Change current directory..."), '~', menu_change_directory_cmd, 0);
    CAddMenuItem ("menu.commandmenu", _("Complete\tCtrl-Tab"), '~', CEditMenuCommand, CK_Complete);
    CAddMenuItem ("menu.commandmenu", _("Insert unicode...\tAlt-i"), '~', CEditMenuCommand, CK_Insert_Unicode);
    CAddMenuItem ("menu.commandmenu", _("paragraph indent mode\tShift-F6"), '~', CEditMenuCommand, CK_Paragraph_Indent_Mode);

    CInsertMenuItemAfter ("menu.filemenu", _("New"), _("Jump to file\tCtrl-j"), '~', menu_jump_to_file, CK_Jump_To_File);
    CAddMenuItem ("menu.filemenu", "", ' ', (callfn) NULL, 0);
    CAddMenuItem ("menu.filemenu", _("Print..."), '~', menu_print_dialog, 0);
    CAddMenuItem ("menu.filemenu", "", ' ', (callfn) NULL, 0);
    CAddMenuItem ("menu.filemenu", _("Find file...\tCtrl-Alt-f"), '~', CEditMenuCommand, CK_Find_File);
    CAddMenuItem ("menu.filemenu", _("Ctags code index...\tCtrl-Alt-i"), '~', CEditMenuCommand, CK_Ctags);

    CAddMenuItem ("menu.editmenu", "", ' ', (callfn) NULL, 0);
    CAddMenuItem ("menu.editmenu", _("List bookmarks..."), '~', menu_bookmark_select, 0);
}


