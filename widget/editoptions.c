/* editoptions.c
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include "edit.h"

#define OPT_DLG_H 15
#define OPT_DLG_W 72

#ifndef USE_INTERNAL_EDIT
#define USE_INTERNAL_EDIT 1
#endif

#include "../src/main.h"	/* extern int option_this_and_that ... */

char *key_emu_str[] =
{N_("Intuitive"), N_("Emacs"), NULL};

char *wrap_str[] =
{N_("None"), N_("Dynamic paragraphing"), N_("Type writer wrap"), NULL};

extern int option_syntax_highlighting;

void i18n_translate_array (char *array[])
{
    size_t maxlen = 0;

    while (*array!=NULL) {
	*array = _(*array);
        array++;
    }
}

void edit_options_dialog (void)
{
    char wrap_length[32], tab_spacing[32], *p, *q;
    int wrap_mode = 0;
    int tedit_key_emulation = edit_key_emulation;
    int toption_fill_tabs_with_spaces = option_fill_tabs_with_spaces;
    int tedit_confirm_save = edit_confirm_save;
    int tedit_syntax_highlighting = option_syntax_highlighting;
    int toption_return_does_auto_indent = option_return_does_auto_indent;
    int toption_backspace_through_tabs = option_backspace_through_tabs;
    int toption_fake_half_tabs = option_fake_half_tabs;

    QuickWidget quick_widgets[] =
    {
/*0 */
	{quick_button, 6, 10, OPT_DLG_H - 3, OPT_DLG_H, N_("&Cancel"), 0, B_CANCEL, 0,
	 0, XV_WLAY_DONTCARE, NULL},
/*1 */
	{quick_button, 2, 10, OPT_DLG_H - 3, OPT_DLG_H, N_("&Ok"), 0, B_ENTER, 0,
	 0, XV_WLAY_DONTCARE, NULL},
/*2 */
	{quick_label, OPT_DLG_W / 2, OPT_DLG_W, OPT_DLG_H - 4, OPT_DLG_H, N_("Word wrap line length : "), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
/*3 */
	{quick_input, OPT_DLG_W / 2 + 24, OPT_DLG_W, OPT_DLG_H - 4, OPT_DLG_H, "", OPT_DLG_W / 2 - 4 - 24, 0,
	 0, 0, XV_WLAY_DONTCARE, "i"},
/*4 */
	{quick_label, OPT_DLG_W / 2, OPT_DLG_W, OPT_DLG_H - 5, OPT_DLG_H, N_("Tab spacing : "), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
/*5 */
	{quick_input, OPT_DLG_W / 2 + 24, OPT_DLG_W, OPT_DLG_H - 5, OPT_DLG_H, "", OPT_DLG_W / 2 - 4 - 24, 0,
	 0, 0, XV_WLAY_DONTCARE, "i"},
/*6 */
#if !defined(MIDNIGHT) || defined(HAVE_SYNTAXH)
#define OA 1
	{quick_checkbox, OPT_DLG_W / 2 + 1, OPT_DLG_W, OPT_DLG_H - 7, OPT_DLG_H, N_("synta&X highlighting"), 8, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
#else
#define OA 0
#endif
/*7 */
	{quick_checkbox, OPT_DLG_W / 2 + 1, OPT_DLG_W, OPT_DLG_H - 8, OPT_DLG_H, N_("confir&M before saving"), 6, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
/*8 */
	{quick_checkbox, OPT_DLG_W / 2 + 1, OPT_DLG_W, OPT_DLG_H - 9, OPT_DLG_H, N_("fill tabs with &Spaces"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
/*9 */
	{quick_checkbox, OPT_DLG_W / 2 + 1, OPT_DLG_W, OPT_DLG_H - 10, OPT_DLG_H, N_("&Return does autoindent"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
/*10 */
	{quick_checkbox, OPT_DLG_W / 2 + 1, OPT_DLG_W, OPT_DLG_H - 11, OPT_DLG_H, N_("&Backspace through tabs"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
/*11 */
	{quick_checkbox, OPT_DLG_W / 2 + 1, OPT_DLG_W, OPT_DLG_H - 12, OPT_DLG_H, N_("&Fake half tabs"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
/*12 */
	{quick_radio, 5, OPT_DLG_W, OPT_DLG_H - 6, OPT_DLG_H, "", 3, 0,
	 0, wrap_str, XV_WLAY_DONTCARE, "wrapm"},
/*13 */
	{quick_label, 4, OPT_DLG_W, OPT_DLG_H - 7, OPT_DLG_H, N_("Wrap mode"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
/*14 */
	{quick_radio, 5, OPT_DLG_W, OPT_DLG_H - 11, OPT_DLG_H, "", 2, 0,
       0, key_emu_str, XV_WLAY_DONTCARE, "keyemu"},
/*15 */
	{quick_label, 4, OPT_DLG_W, OPT_DLG_H - 12, OPT_DLG_H, N_("Key emulation"), 0, 0,
	 0, 0, XV_WLAY_DONTCARE, NULL},
	{0}};

    static int i18n_flag = 0;
    
    if (!i18n_flag) {
	i18n_translate_array (key_emu_str);
	i18n_translate_array (wrap_str);
	i18n_flag = 1;
    }

    sprintf (wrap_length, "%d", option_word_wrap_line_length);
    sprintf (tab_spacing, "%d", option_tab_spacing);

    quick_widgets[3].text = wrap_length;
    quick_widgets[3].str_result = &p;
    quick_widgets[5].text = tab_spacing;
    quick_widgets[5].str_result = &q;
    quick_widgets[5 + OA].result = &tedit_syntax_highlighting;
    quick_widgets[6 + OA].result = &tedit_confirm_save;
    quick_widgets[7 + OA].result = &toption_fill_tabs_with_spaces;
    quick_widgets[8 + OA].result = &toption_return_does_auto_indent;
    quick_widgets[9 + OA].result = &toption_backspace_through_tabs;
    quick_widgets[10 + OA].result = &toption_fake_half_tabs;

    if (option_auto_para_formatting)
	wrap_mode = 1;
    else if (option_typewriter_wrap)
	wrap_mode = 2;
    else
	wrap_mode = 0;

    quick_widgets[11 + OA].result = &wrap_mode;
    quick_widgets[11 + OA].value = wrap_mode;

    quick_widgets[13 + OA].result = &tedit_key_emulation;
    quick_widgets[13 + OA].value = tedit_key_emulation;

    {
	QuickDialog Quick_options =
	{OPT_DLG_W, OPT_DLG_H, -1, 0, N_(" Editor options "),
	 "", "quick_input", 0};

	Quick_options.widgets = quick_widgets;

	if (quick_dialog (&Quick_options) != B_CANCEL) {
	    if (p) {
		option_word_wrap_line_length = atoi (p);
		free (p);
	    }
	    if (q) {
		option_tab_spacing = atoi (q);
		if (option_tab_spacing < 0)
		    option_tab_spacing = 2;
		option_tab_spacing += option_tab_spacing & 1;
		free (q);
	    }
	    option_syntax_highlighting = *quick_widgets[5 + OA].result;
	    edit_confirm_save = *quick_widgets[6 + OA].result;
	    option_fill_tabs_with_spaces = *quick_widgets[7 + OA].result;
	    option_return_does_auto_indent = *quick_widgets[8 + OA].result;
	    option_backspace_through_tabs = *quick_widgets[9 + OA].result;
	    option_fake_half_tabs = *quick_widgets[10 + OA].result;

	    if (*quick_widgets[11 + OA].result == 1) {
		option_auto_para_formatting = 1;
		option_typewriter_wrap = 0;
	    } else if (*quick_widgets[11 + OA].result == 2) {
		option_auto_para_formatting = 0;
		option_typewriter_wrap = 1;
	    } else {
		option_auto_para_formatting = 0;
		option_typewriter_wrap = 0;
	    }

	    edit_key_emulation = *quick_widgets[13 + OA].result;

	    return;
	} else {
	    return;
	}
    }
}

