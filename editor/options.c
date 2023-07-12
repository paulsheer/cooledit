/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* options.c
   Copyright (C) 1996-2022 Paul Sheer
 */



#include "inspect.h"
#include <config.h>
#include "stringtools.h"
#include "edit.h"
#include "debug.h"
#include "rxvt.h"
#include "cmdlineopt.h"
#include "editoptions.h"

extern Window main_window;

extern int option_save_setup_on_exit;
extern int option_suppress_load_files;
extern int option_toolbar;
extern int option_toolhint_milliseconds1;
extern int option_remote_timeout;

extern int option_text_fg_normal;
extern int option_text_fg_bold;
extern int option_text_fg_italic;

extern int option_text_bg_normal;
extern int option_text_bg_marked;
extern int option_text_bg_highlighted;
extern int option_hint_messages;
extern int option_cursor_blink_rate;
extern int option_pull_down_window_list;
extern int option_find_bracket;
extern int option_edit_right_extreme;
extern int option_edit_left_extreme;
extern int option_edit_top_extreme;
extern int option_edit_bottom_extreme;
extern int option_typing_replaces_selection;
extern int option_locale_encoding;
extern int option_utf_interpretation2;
extern int option_locale_encoding;
extern int option_reverse_levant;
extern int option_rgb_order;
extern int option_interchar_spacing;
extern int option_file_browser_width;
extern int option_file_browser_height;
extern int option_shell_command_line_sticky;
extern int option_shell_command_line_pty;
extern int option_invert_colors;
extern int option_invert_crome;

extern int option_invert_red_green;
extern int option_invert_green_blue;
extern int option_invert_red_blue;

extern int last_unichar_left;
extern int last_unichar_right;

extern int option_xor_cursor;
extern int option_flashing_cursor;
extern int option_long_whitespace;

extern int option_force_own_colormap;
extern int option_force_default_colormap;
extern int option_mouse_double_click;
extern int option_middle_button_pastes;
extern int option_syntax_highlighting;
extern int option_auto_spellcheck;

extern int option_smooth_scrolling;

extern int option_new_window_ask_for_file;

extern int option_never_raise_wm_windows;

#ifdef HAVE_DND
extern int option_dnd_version;
#endif

extern int option_interpret_numlock;

extern char *editor_options_file;
extern char *option_preferred_visual;


extern int option_color_0;
extern int option_color_1;
extern int option_color_2;
extern int option_color_3;
extern int option_color_4;
extern int option_color_5;
extern int option_color_6;
extern int option_color_7;
extern int option_color_8;
extern int option_color_9;
extern int option_color_10;
extern int option_color_11;
extern int option_color_12;
extern int option_color_13;
extern int option_color_14;
extern int option_color_15;
extern int option_color_16;
extern int option_color_17;
extern int option_color_18;
extern int option_color_19;
extern int option_color_20;
extern int option_color_21;
extern int option_color_22;
extern int option_color_23;
extern int option_color_24;
extern int option_color_25;
extern int option_color_26;

static int dummy = 0;

static struct {
    char *name;
    int *value;
    char *prompt;
#define TYPE_ON_OFF		1
#define TYPE_VALUE		2
#define TYPE_HIDDEN_VALUE	3
#define TYPE_HIDDEN_HEX_VALUE	4
#define TYPE_BLANK		5
    int type;
} integer_options [] = {
/* The following are check box labels */
	{"option_locale_encoding", &option_locale_encoding, gettext_noop(" Use locale encoding "), TYPE_ON_OFF},
	{"option_word_wrap_line_length", &option_word_wrap_line_length, gettext_noop(" Word wrap line length: "), TYPE_VALUE},
	{"option_tab_spacing", &option_tab_spacing, gettext_noop(" Tab spacing: "), TYPE_VALUE},
	{"option_fill_tabs_with_spaces", &option_fill_tabs_with_spaces, gettext_noop(" Fill tabs with spaces "), TYPE_ON_OFF},
	{"option_return_does_auto_indent", &option_return_does_auto_indent, gettext_noop(" Return does auto indent "), TYPE_ON_OFF},
	{"option_backspace_through_tabs", &option_backspace_through_tabs, gettext_noop(" Backspace through all tabs "), TYPE_ON_OFF},
	{"option_fake_half_tabs", &option_fake_half_tabs, gettext_noop(" Emulate half tabs with spaces "), TYPE_ON_OFF},
	{"option_save_setup_on_exit", &option_save_setup_on_exit, gettext_noop(" Save setup on exit "), TYPE_ON_OFF},
	{"option_suppress_load_files", &option_suppress_load_files, gettext_noop(" Don't load back desktop on startup "), TYPE_ON_OFF},
	{"option_save_mode", &option_save_mode, 0, 0},
	{"option_hint_messages", &option_hint_messages, gettext_noop(" Hint time on title bar "), TYPE_VALUE},
	{"option_cursor_blink_rate", &option_cursor_blink_rate, 0, 0},
	{"option_flashing_cursor", &option_flashing_cursor, gettext_noop(" Flashing cursor "), TYPE_ON_OFF},
	{"option_xor_cursor", &option_xor_cursor, gettext_noop(" Xor cursor "), TYPE_ON_OFF},
	{"option_pull_down_window_list", &option_pull_down_window_list, gettext_noop(" Pull down 'Window' menu "), TYPE_ON_OFF},
	{"option_find_bracket", &option_find_bracket, gettext_noop(" Highlight matching bracket "), TYPE_ON_OFF},
	{"option_never_raise_wm_windows", &option_never_raise_wm_windows, gettext_noop(" Never Raise WM Windows "), TYPE_ON_OFF},
	{"option_edit_right_extreme", &option_edit_right_extreme, 0, 0},
	{"option_edit_left_extreme", &option_edit_left_extreme, 0, 0},
	{"option_edit_top_extreme", &option_edit_top_extreme, 0, 0},
	{"option_edit_bottom_extreme", &option_edit_bottom_extreme, 0, 0},
	{"option_text_line_spacing", &option_text_line_spacing, 0, 0},
	{"option_force_own_colormap", &option_force_own_colormap, 0, 0},
	{"option_force_default_colormap", &option_force_default_colormap, 0, 0},
	{"option_mouse_double_click", &option_mouse_double_click, gettext_noop(" Mouse double click time-out "), TYPE_VALUE},
#ifdef HAVE_DND
	{"option_dnd_version", &option_dnd_version, 0, 0},
#endif
	{"option_max_undo", &option_max_undo, 0, 0},
#if 0
	{"option_interwidget_spacing", &option_interwidget_spacing, 0, 0},
#endif
	{"option_toolbar", &option_toolbar, gettext_noop(" Toolbar on edit windows "), TYPE_ON_OFF},
	{"option_interpret_numlock", &option_interpret_numlock, gettext_noop(" Interpret Num-Lock "), TYPE_ON_OFF},
	{"option_long_whitespace", &option_long_whitespace, gettext_noop(" Whitespace is doubled "), TYPE_ON_OFF},
	{"option_toolhint_milliseconds1", &option_toolhint_milliseconds1, gettext_noop(" Time to show button hints "), TYPE_VALUE},
	{"option_remote_timeout", &option_remote_timeout, gettext_noop(" Timeout for remote (ms) "), TYPE_VALUE},
	{"option_edit_right_extreme", &option_edit_right_extreme, gettext_noop(" Right cursor limit "), TYPE_VALUE},
	{"option_edit_left_extreme", &option_edit_left_extreme, gettext_noop(" Left cursor limit "), TYPE_VALUE},
	{"option_edit_top_extreme", &option_edit_top_extreme, gettext_noop(" Top cursor limit "), TYPE_VALUE},
	{"option_edit_bottom_extreme", &option_edit_bottom_extreme, gettext_noop(" Bottom cursor limit "), TYPE_VALUE},
	{"option_middle_button_pastes", &option_middle_button_pastes, gettext_noop(" Mouse middle button pastes "), TYPE_ON_OFF},
	{"option_new_window_ask_for_file", &option_new_window_ask_for_file, gettext_noop(" New windows ask for file "), TYPE_ON_OFF},
	{"option_smooth_scrolling", &option_smooth_scrolling, gettext_noop(" Smooth scrolling "), TYPE_ON_OFF},
	{"option_typing_replaces_selection", &option_typing_replaces_selection, gettext_noop(" Typing replaces selection "), TYPE_ON_OFF},
	{"option_utf_interpretation2", &option_utf_interpretation2, gettext_noop(" UTF8 Interpretation "), TYPE_ON_OFF},
	{"option_reverse_levant", &option_reverse_levant, gettext_noop(" Reverse Hebrew/Arabic/etc. "), TYPE_ON_OFF},
	{"last_unichar_left", &last_unichar_left, 0, TYPE_HIDDEN_VALUE},
	{"last_unichar_right", &last_unichar_right, 0, TYPE_HIDDEN_VALUE},
	{"option_rgb_order", &option_rgb_order, 0, TYPE_HIDDEN_VALUE},
	{"option_interchar_spacing", &option_interchar_spacing, 0, TYPE_HIDDEN_VALUE},
	{"option_file_browser_width", &option_file_browser_width, 0, TYPE_HIDDEN_VALUE},
	{"option_file_browser_height", &option_file_browser_height, 0, TYPE_HIDDEN_VALUE},
	{"option_shell_command_line_sticky", &option_shell_command_line_sticky, 0, TYPE_HIDDEN_VALUE},
	{"option_shell_command_line_pty", &option_shell_command_line_pty, 0, TYPE_HIDDEN_VALUE},
	{"option_invert_colors", &option_invert_colors, 0, TYPE_HIDDEN_VALUE},
	{"option_invert_crome", &option_invert_crome, 0, TYPE_HIDDEN_VALUE},
	{"option_invert_red_green", &option_invert_red_green, 0, TYPE_HIDDEN_VALUE},
	{"option_invert_green_blue", &option_invert_green_blue, 0, TYPE_HIDDEN_VALUE},
	{"option_invert_red_blue", &option_invert_red_blue, 0, TYPE_HIDDEN_VALUE},
	{"option_color_0", &option_color_0, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_1", &option_color_1, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_2", &option_color_2, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_3", &option_color_3, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_4", &option_color_4, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_5", &option_color_5, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_6", &option_color_6, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_7", &option_color_7, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_8", &option_color_8, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_9", &option_color_9, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_10", &option_color_10, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_11", &option_color_11, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_12", &option_color_12, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_13", &option_color_13, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_14", &option_color_14, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_15", &option_color_15, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_16", &option_color_16, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_17", &option_color_17, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_18", &option_color_18, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_19", &option_color_19, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_20", &option_color_20, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_21", &option_color_21, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_22", &option_color_22, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_23", &option_color_23, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_24", &option_color_24, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_25", &option_color_25, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_color_26", &option_color_26, 0, TYPE_HIDDEN_HEX_VALUE},
	{"option_syntax_highlighting", &option_syntax_highlighting, 0, 0},
	{"option_auto_spellcheck", &option_auto_spellcheck, 0, 0},

        {"option_replace_scanf", &option_replace_scanf, 0, TYPE_HIDDEN_VALUE},
        {"option_replace_regexp", &option_replace_regexp, 0, TYPE_HIDDEN_VALUE},
        {"option_replace_all", &option_replace_all, 0, TYPE_HIDDEN_VALUE},
        {"option_replace_prompt", &option_replace_prompt, 0, TYPE_HIDDEN_VALUE},
        {"option_replace_whole", &option_replace_whole, 0, TYPE_HIDDEN_VALUE},
        {"option_replace_case", &option_replace_case, 0, TYPE_HIDDEN_VALUE},
        {"option_replace_backwards", &option_replace_backwards, 0, TYPE_HIDDEN_VALUE},
        {"option_search_create_bookmark", &option_search_create_bookmark, 0, TYPE_HIDDEN_VALUE},

        {"options_debug_show_output", &debug_options.show_output, 0, TYPE_HIDDEN_VALUE},
        {"options_debug_stop_at_main", &debug_options.stop_at_main, 0, TYPE_HIDDEN_VALUE},
        {"options_debug_show_on_stdout", &debug_options.show_on_stdout, 0, TYPE_HIDDEN_VALUE},

        {"options_startup_term_8bit", &rxvt_startup_options.term_8bit, 0, TYPE_HIDDEN_VALUE},
        {"options_startup_large_font", &rxvt_startup_options.large_font, 0, TYPE_HIDDEN_VALUE},
        {"options_startup_backspace_ctrl_h", &rxvt_startup_options.backspace_ctrl_h, "rxvt/xterm, Force backspace to ^H", TYPE_ON_OFF},
        {"options_startup_backspace_127", &rxvt_startup_options.backspace_127, "rxvt/xterm, Force backspace to ^?", TYPE_ON_OFF},
        {"options_startup_x11_forwarding", &rxvt_startup_options.x11_forwarding, "rxvt/xterm, Enable X11 forwarding", TYPE_ON_OFF},

	{0, 0}
};

extern char *option_display;
extern char *option_geometry;
extern char *option_background_color;
extern char *option_foreground_red;
extern char *option_foreground_green;
extern char *option_foreground_blue;
extern char *option_font2;
extern char *option_widget_font2;
extern char *option_8bit_term_font;

extern char *option_man_cmdline;
extern char *option_alternate_dictionary;

extern char *option_look;

static struct {
    char *name;
    char **value;
} string_options [] = {
	{"option_look", &option_look},
	{"option_whole_chars_search", &option_whole_chars_search},
	{"option_chars_move_whole_word", &option_chars_move_whole_word},
#if 0   /* mmmmmmh it shouldn't remember the display */
	{"option_display", &option_display},
#endif
	{"option_geometry", &option_geometry},
	{"option_background_color", &option_background_color},
	{"option_foreground_red", &option_foreground_red},
	{"option_foreground_green", &option_foreground_green},
	{"option_foreground_blue", &option_foreground_blue},
	{"option_font5", &option_font2},
	{"option_widget_font4", &option_widget_font2},
	{"option_8bit_term_font", &option_8bit_term_font},
	{"option_backup_ext", &option_backup_ext},
	{"option_man_cmdline3", &option_man_cmdline},
	{"option_preferred_visual", &option_preferred_visual}, 
	{"option_alternate_dictionary", &option_alternate_dictionary}, 
	{0, 0}
};

static struct {
    char *name;
    int *value;
    char *cname;
} color_options [] = {
	{"option_editor_fg_normal", &option_editor_fg_normal, 0},
	{"option_editor_fg_bold", &option_editor_fg_bold, 0},
	{"option_editor_fg_italic", &option_editor_fg_italic, 0},
	{"option_editor_bg_normal", &option_editor_bg_normal, 0},
	{"option_editor_bg_abnormal2", &option_editor_bg_abnormal2, 0},
	{"option_editor_bg_marked", &option_editor_bg_marked, 0},
	{"option_editor_bg_marked_abnormal", &option_editor_bg_marked_abnormal, 0},
	{"option_editor_bg_highlighted", &option_editor_bg_highlighted, 0},
	{"option_editor_fg_cursor", &option_editor_fg_cursor, 0},
	{"option_text_fg_normal", &option_text_fg_normal, 0},
	{"option_text_fg_bold", &option_text_fg_bold, 0},
	{"option_text_fg_italic", &option_text_fg_italic, 0},
	{"option_text_bg_normal", &option_text_bg_normal, 0},
	{"option_text_bg_marked", &option_text_bg_marked, 0},
	{"option_text_bg_highlighted", &option_text_bg_highlighted, 0},
	{0, 0}
};


int save_options_section (const char *file, const char *section, const char *text);

int load_setup (const char *file)
{E_
    static char *options_section = 0;
    char *p, *q;
    int fin = 0;

    if (options_section) {
	free (options_section);
	options_section = 0;
    }
    if (!file)
	return 0;

    options_section = get_options_section (file, "[Options]");
    p = q = options_section;

    if (!options_section)
	return -1;

    for (fin = 0; !fin;) {
	if (*q == '\n' || !*q) {
	    int i;
	    if (!*q)
		fin = 1;
	    *q = 0;
	    for (i = 0; string_options[i].name; i++) {
		int l;
		l = strlen (string_options[i].name);
		l = strnlen (p, l);
		if (p[l] && strchr ("\t =", p[l])) {
		    if (!Cstrncasecmp (p, string_options[i].name, l)) {
			*(string_options[i].value) = p + l + strspn (p + l, " =\t");
			break;
		    }
		}
	    }
	    for (i = 0; integer_options[i].name; i++) {
		int l;
                if (integer_options[i].type == TYPE_BLANK)
                    continue;
		l = strlen (integer_options[i].name);
		l = strnlen (p, l);
		if (p[l] && strchr ("\t =", p[l])) {
		    if (!Cstrncasecmp (p, integer_options[i].name, l)) {
			*(integer_options[i].value) = strtol (p + l + strspn (p + l, " =\t"), (char **) 0, 0);
			break;
		    }
		}
	    }
	    for (i = 0; color_options[i].name; i++) {
		int l;
		l = strlen (color_options[i].name);
		l = strnlen (p, l);
		if (p[l] && strchr ("\t =", p[l])) {
		    if (!Cstrncasecmp (p, color_options[i].name, l)) {
			*(color_options[i].value) = atoi (p + l + strspn (p + l, " =\t"));
			color_options[i].cname = p + l + strspn (p + l, " =\t");
			break;
		    }
		}
	    }
	    p = (++q);
	} else {
	    q++;
	}
    }
    return 0;
}

void get_main_window_geometry (void)
{E_
    int x = 0, y = 0;
    unsigned int width = 0, height = 0, d;
    Window win, root;
    static char save_geom[80];
    int bitmask;

    if (!main_window)
        return;

    XGetGeometry (CDisplay, main_window, &root, &x, &y, &width, &height, &d, &d);
    win = CGetWMWindow (main_window);
    XGetGeometry (CDisplay, win, &root, &x, &y, &d, &d, &d, &d);

    if (option_geometry)
	bitmask = XParseGeometry (option_geometry, (int *) &d, (int *) &d, &d, &d);
    else
	bitmask = WidthValue | HeightValue;

    save_geom[0] = '\0';
    if ((bitmask & (WidthValue | HeightValue)) && width > 50 && height > 50) {
	strcat (save_geom, itoa (width));
	strcat (save_geom, "x");
	strcat (save_geom, itoa (height));
    }
    if (bitmask & (XValue | YValue)) {
	if (x >= 0)
	    strcat (save_geom, "+");
	strcat (save_geom, itoa (x));
	if (y >= 0)
	    strcat (save_geom, "+");
	strcat (save_geom, itoa (y));
    }
    option_geometry = save_geom;
}

/* option_geometry is not strdup'ed here so don't free it */
int save_setup (const char *file)
{E_
    char *p, *s;
    int r, i;

    get_main_window_geometry ();

    p = s = CMalloc (256 * 1024);

    for (i = 0; string_options[i].name; i++) {
	if (*string_options[i].value) {
	    sprintf (p, "%s = %s\n%n", string_options[i].name, *string_options[i].value, &r);
	    p += r;
	}
    }
    for (i = 0; integer_options[i].name; i++) {
        if (integer_options[i].type == TYPE_BLANK)
            continue;
	if (integer_options[i].type == TYPE_HIDDEN_HEX_VALUE)
	    sprintf (p, "%s = 0x%X\n%n", integer_options[i].name, *integer_options[i].value, &r);
	else
	    sprintf (p, "%s = %d\n%n", integer_options[i].name, *integer_options[i].value, &r);
	p += r;
    }
    for (i = 0; color_options[i].name; i++) {
	if (color_options[i].cname)
	    sprintf (p, "%s = %s\n%n", color_options[i].name, color_options[i].cname, &r);
	else
	    sprintf (p, "%s = %d\n%n", color_options[i].name, *color_options[i].value, &r);
	p += r;
    }
    *p = 0;

    r = save_options_section (file, "[Options]", s);
    free (s);
    return r;
}

void save_options (void)
{E_
    if (save_setup (editor_options_file) < 0)
	CErrorDialog (main_window, 20, 20, _(" Save Options "), "%s", get_sys_error (
	catstrs (_(" Error trying to save : "), editor_options_file, " ", NULL)));
}

static char *short_name (char *a, char *b)
{E_
    static char r[128];
    strcpy (r, a);
    strcat (r, b);
    r[30] = 0;
    return r;
}

#define WHICH_SWITCHES	1
#define WHICH_GENERAL	2

static void assign_options (int which)
{E_
    int i = 0;
    static char whole_chars_search[128];
    static char whole_chars_move[128];
    static char alternate_dictionary[MAX_PATH_LEN];

    while (integer_options[i].value) {
        if (integer_options[i].type == TYPE_BLANK) {
            /* pass */
	} else
	if (integer_options[i].prompt) {
	    if (which == WHICH_SWITCHES) {
		if (integer_options[i].type == TYPE_ON_OFF)
		    *integer_options[i].value = (CIdent (short_name (integer_options[i].name, "")))->keypressed;
	    }
	    if (which == WHICH_GENERAL) {
		if (integer_options[i].type == TYPE_VALUE)
		    *integer_options[i].value = atoi ((CIdent (short_name (integer_options[i].name, "")))->text.data);
	    }
	}
	i++;
    }
    if (which == WHICH_SWITCHES) {
#ifdef HAVE_DND
	option_dnd_version = (CIdent ("dnd_version"))->keypressed;
#endif
	option_auto_para_formatting = (CIdent ("para_form"))->keypressed;
	option_typewriter_wrap = (CIdent ("typew_wrap"))->keypressed;
	option_auto_spellcheck = (CIdent ("auto_spell"))->keypressed;
	option_syntax_highlighting = (CIdent ("syntax_high"))->keypressed;
    }
    if (which == WHICH_GENERAL) {
	strncpy (alternate_dictionary, (CIdent ("options.a_dict"))->text.data, MAX_PATH_LEN - 1);
	option_alternate_dictionary = alternate_dictionary;
	alternate_dictionary[MAX_PATH_LEN - 1] = '\0';

	strncpy (whole_chars_search, (CIdent ("options.wc_search"))->text.data, 127);
	option_whole_chars_search = whole_chars_search;
	whole_chars_search[127] = '\0';

	strncpy (whole_chars_move, (CIdent ("options.wc_move"))->text.data, 127);
	option_chars_move_whole_word = whole_chars_move;
	whole_chars_move[127] = '\0';
    }
}

void draw_options_dialog (Window parent, int x, int y)
{E_
    Window win;
    XEvent xev;
    CEvent cev;
    int xh, i;
    CState s;
    CWidget *w, *wdt;

    CBackupState (&s);
    CDisable ("*");

    win = CDrawHeadedDialog ("options", parent, x, y, _(" Options "));
    CGetHintPos (&x, &y);

    i = 0;
    xh = x;
    while (integer_options[i].value) {
	if (integer_options[i].prompt && integer_options[i].type == TYPE_VALUE) {
	    wdt = CDrawText (short_name ("T", integer_options[i].name), win, xh, y, _(integer_options[i].prompt));
	    CGetHintPos (&x, 0);
	    w = CDrawTextInputP (short_name (integer_options[i].name, ""), win, x, y, FONT_MEAN_WIDTH * 8 + FONT_PIX_PER_LINE + 4, AUTO_HEIGHT, 8, itoa (*integer_options[i].value));
	    w->position |= POSITION_FILL;
	    w->label = (char *) _(integer_options[i].prompt);        /* FIXME: memory leak? should not label be free'd? */
	    w->hotkey = find_hotkey (w);
	    w->label = 0;
	    wdt->hotkey = w->hotkey;
	    CGetHintPos (0, &y);
	}
	i++;
    }

    CGetHintPos (0, &y);
    CDrawText ("options.twc_search", win, xh, y, _(" Whole chars search: "));
    CGetHintPos (&x, 0);
    (CDrawTextInputP ("options.wc_search", win, x, y, FONT_MEAN_WIDTH * 16, AUTO_HEIGHT, 258, option_whole_chars_search)->position) |= POSITION_FILL;
    CGetHintPos (0, &y);
    CDrawText ("options.twc_move", win, xh, y, _(" Whole chars move: "));
    CGetHintPos (&x, 0);
    (CDrawTextInputP ("options.wc_move", win, x, y, FONT_MEAN_WIDTH * 16, AUTO_HEIGHT, 258, option_chars_move_whole_word)->position) |= POSITION_FILL;
    CGetHintPos (0, &y);
    CDrawText ("options.ta_dict", win, xh, y, _(" Ispell alternate dict: "));
    CGetHintPos (&x, 0);
    (CDrawTextInputP ("options.a_dict", win, x, y, FONT_MEAN_WIDTH * 16, AUTO_HEIGHT, 258, option_alternate_dictionary)->position) |= POSITION_FILL;

    CGetHintPos (0, &y);
    CDrawPixmapButton ("options.ok", win, xh, y, PIXMAP_BUTTON_TICK);
/* Toolhint */
    CSetToolHint ("options.ok", _("Apply options, Enter"));
    CGetHintPos (&xh, 0);
    CDrawPixmapButton ("options.cancel", win, xh, y, PIXMAP_BUTTON_CROSS);
/* Toolhint */
    CSetToolHint ("options.cancel", _("Abort dialog, Escape"));
    CGetHintPos (&xh, 0);
    CDrawPixmapButton ("options.save", win, xh, y, PIXMAP_BUTTON_SAVE);
/* Toolhint */
    CSetToolHint ("options.save", _("Save options"));
    CSetSizeHintPos ("options");
    CMapDialog ("options");

    CFocus (CIdent ("options.ok"));

    for (;;) {
	CNextEvent (&xev, &cev);
	if (!CIdent("options"))
	    break;
	if (!strcmp (cev.ident, "options.cancel") || cev.command == CK_Cancel)
	    break;
	if (!strcmp (cev.ident, "options.ok") || cev.command == CK_Enter) {
/* keypressed holds the whether switch is on or off */
	    assign_options (WHICH_GENERAL);
	    break;
	}
	if (!strcmp (cev.ident, "options.save")) {
/* keypressed holds the whether switch is on or off */
	    assign_options (WHICH_GENERAL);
            save_options ();
	}
    }
    w = CGetEditMenu ();
    if (w)
	CExpose (w->ident);
    CDestroyWidget ("options");
    CRestoreState (&s);
}

int set_editor_encoding (int utf_encoding, int locale_encoding);
void cooledit_appearance_modification (void);

void draw_switches_dialog (Window parent, int x, int y)
{E_
    Window win;
    XEvent xev;
    CEvent cev;
    int xh, yh;
    int dy = 0;
    int i, n;
    CState s;
    CWidget *w;

    CBackupState (&s);
    CDisable ("*");

    win = CDrawHeadedDialog ("options", parent, x, y, _ (" Options "));
    CGetHintPos (&x, &y);

    n = i = 0;
    while (integer_options[i].value) {
	if (integer_options[i].prompt && integer_options[i].type == TYPE_ON_OFF)
	    n++;
	i++;
    }

    i = 0;
    xh = x;
    yh = y;
    while (integer_options[i].value) {
	if (integer_options[i].prompt && integer_options[i].type == TYPE_ON_OFF) {
	    if (n <= 0) {
		get_hint_limits (&xh, 0);	/* half on the left, half on the right */
		n = 9999;
		yh = y;
	    }
	    CDrawSwitch (short_name (integer_options[i].name, ""), win, xh, yh, *integer_options[i].value, _ (integer_options[i].prompt), 0);
            dy = yh;
	    CGetHintPos (0, &yh);
            dy = yh - dy;
	    n -= 2;
	} else if (integer_options[i].type == TYPE_BLANK) {
            yh += 40;
        }
	i++;
    }

#ifdef HAVE_DND
    get_hint_limits (0, &y);
    CDrawSwitch ("dnd_version_not", win, x, y, !option_dnd_version, _ (" Dnd version 0 "), 1 | RADIO_INVERT_GROUP);
    CDrawSwitch ("dnd_version", win, xh, y, option_dnd_version, _ (" Dnd version 1 "), 1 | RADIO_INVERT_GROUP);
#endif

    get_hint_limits (0, &y);
    CDrawSwitch ("para_form", win, x, y, option_auto_para_formatting, _ (" Auto paragraph formatting "), 2);
    CDrawSwitch ("typew_wrap", win, xh, y, option_typewriter_wrap, _ (" Type-writer wrap "), 2);
    CGetHintPos (0, &y);

    get_hint_limits (0, &y);
    CDrawSwitch ("syntax_high", win, x, y, option_syntax_highlighting, _ (" Syntax highlighting "), 0);
    CDrawSwitch ("auto_spell", win, xh, y, option_auto_spellcheck, _ (" Spellcheck as you type "), 0);
    CGetHintPos (0, &y);

    CDrawPixmapButton ("options.ok", win, x, y, PIXMAP_BUTTON_TICK);
/* Toolhint */
    CSetToolHint ("options.ok", _ ("Apply options, Enter"));
    CGetHintPos (&xh, 0);
    CDrawPixmapButton ("options.cancel", win, xh, y, PIXMAP_BUTTON_CROSS);
/* Toolhint */
    CSetToolHint ("options.cancel", _ ("Abort dialog, Escape"));
    CGetHintPos (&xh, 0);
    CDrawPixmapButton ("options.save", win, xh, y, PIXMAP_BUTTON_SAVE);
/* Toolhint */
    CSetToolHint ("options.save", _ ("Save options"));
    CSetSizeHintPos ("options");
    CMapDialog ("options");

    CFocus (CIdent ("options.ok"));

    for (;;) {
	CNextEvent (&xev, &cev);
	if (!CIdent ("options"))
	    break;
	if (!strcmp (cev.ident, "syntax_high")) {
/* turn off spell checkig if syntax high is on */
	    if ((CIdent ("auto_spell"))->keypressed && !(CIdent ("syntax_high"))->keypressed) {
		(CIdent ("auto_spell"))->keypressed = 0;
		render_switch (CIdent ("auto_spell"));
	    }
	}
	if (!strcmp (cev.ident, "auto_spell")) {
/* turn on syntax highlighting if spellchecking is on */
	    if ((CIdent ("auto_spell"))->keypressed && !(CIdent ("syntax_high"))->keypressed) {
		(CIdent ("syntax_high"))->keypressed = 1;
		render_switch (CIdent ("syntax_high"));
	    }
	}

	if (!strcmp (cev.ident, "auto_spell")) {
/* turn on syntax highlighting if spellchecking is on */
	    if ((CIdent ("auto_spell"))->keypressed && !(CIdent ("syntax_high"))->keypressed) {
		(CIdent ("syntax_high"))->keypressed = 1;
		render_switch (CIdent ("syntax_high"));
	    }
	}

	if (!strcmp (cev.ident, "option_utf_interpretation2")) {
	    if ((CIdent ("option_utf_interpretation2"))->keypressed && (CIdent ("option_locale_encoding"))->keypressed) {
		(CIdent ("option_locale_encoding"))->keypressed = 0;
		render_switch (CIdent ("option_locale_encoding"));
	    }
	}

	if (!strcmp (cev.ident, "option_locale_encoding")) {
	    if ((CIdent ("option_locale_encoding"))->keypressed && (CIdent ("option_utf_interpretation2"))->keypressed) {
		(CIdent ("option_utf_interpretation2"))->keypressed = 0;
		render_switch (CIdent ("option_utf_interpretation2"));
	    }
	}

	if (!strcmp (cev.ident, "options_startup_backspace_ctrl_h")) {
	    if ((CIdent ("options_startup_backspace_127"))->keypressed && (CIdent ("options_startup_backspace_ctrl_h"))->keypressed) {
		(CIdent ("options_startup_backspace_127"))->keypressed = 0;
		render_switch (CIdent ("options_startup_backspace_127"));
	    }
	}

	if (!strcmp (cev.ident, "options.cancel") || cev.command == CK_Cancel)
	    break;
	if (!strcmp (cev.ident, "options.ok") || cev.command == CK_Enter) {
/* keypressed holds the whether switch is on or off */
	    assign_options (WHICH_SWITCHES);
	    break;
	}
	if (!strcmp (cev.ident, "options.save")) {
/* keypressed holds the whether switch is on or off */
	    assign_options (WHICH_SWITCHES);
	    save_options ();
	}
    }
    w = CGetEditMenu ();
    if (w)
	CExpose (w->ident);
    CDestroyWidget ("options");
    CRestoreState (&s);

    if (set_editor_encoding (option_utf_interpretation2, option_locale_encoding))
        cooledit_appearance_modification ();
}


void save_mode_options_dialog (Window parent, int x, int y)
{E_
    Window win;
    XEvent xev;
    CEvent cev;
    int x2, y2;
    CState s;
    CWidget *quick, *safe, *backup;

    CBackupState (&s);
    CDisable ("*");

    win = CDrawHeadedDialog ("saving", parent, x, y, _(" Options "));
    CGetHintPos (&x, &y);

    quick = CDrawSwitch ("saving.Rquick", win, x, y, option_save_mode == 0, _(" Quick save "), 1 | RADIO_ONE_ALWAYS_ON);
/* Toolhint */
    CSetToolHint ("saving.Rquick", _("Truncates file then writes contents of editor"));
    CSetToolHint ("saving.Rquick.label", _("Truncates file then writes contents of editor"));
    CGetHintPos (0, &y2);

    safe = CDrawSwitch ("saving.Rsafe", win, x, y2, option_save_mode == 1, _(" Safe save "), 1 | RADIO_ONE_ALWAYS_ON);
/* Toolhint */
    CSetToolHint ("saving.Rsafe", _("Writes to temporary file, then renames if succesful"));
    CSetToolHint ("saving.Rsafe.label", _("Writes to temporary file, then renames if succesful"));
    CGetHintPos (0, &y);

    backup = CDrawSwitch ("saving.Rbackup", win, x, y, option_save_mode == 2, _(" Create backups "), 1 | RADIO_ONE_ALWAYS_ON);
/* Toolhint */
    CSetToolHint ("saving.Rbackup", _("Creates a backup file first"));
    CSetToolHint ("saving.Rbackup.label", _("Creates a backup file first"));
    CGetHintPos (0, &y2);

    CDrawText ("saving.ext", win, x, y2, _(" Backup file extension: "));
    CGetHintPos (&x2, 0);
    CDrawTextInputP ("saving.extti", win, x2, y2, FONT_MEAN_WIDTH * 16, AUTO_HEIGHT, 16, option_backup_ext);

    CGetHintPos (0, &y);
    CDrawPixmapButton ("saving.ok", win, x, y, PIXMAP_BUTTON_TICK);
    CGetHintPos (&x2, 0);
    CDrawPixmapButton ("saving.cancel", win, x2, y, PIXMAP_BUTTON_CROSS);
    CGetHintPos (&x2, 0);
    CDrawPixmapButton ("saving.save", win, x2, y, PIXMAP_BUTTON_SAVE);
    CSetSizeHintPos ("saving");
    CMapDialog ("saving");

    CFocus (CIdent ("saving.ok"));

    for (;;) {
	CNextEvent (&xev, &cev);
	if (!CIdent("saving"))
	    break;
	if (!strcmp (cev.ident, "saving.cancel") || cev.command == CK_Cancel)
	    break;
	if (!strcmp (cev.ident, "saving.ok") || cev.command == CK_Enter
	    || !strcmp (cev.ident, "saving.save")) {
/* keypressed holds the whether switch is on or off */
	    if (CIdent ("saving.extti"))
		option_backup_ext = (char *) strdup ((CIdent ("saving.extti"))->text.data);
	    if (quick->keypressed)
		option_save_mode = 0;
	    else if (safe->keypressed)
		option_save_mode = 1;
	    else if (backup->keypressed)
		option_save_mode = 2;
	    if (!strcmp (cev.ident, "saving.save")) {
		save_options ();
	    } else
		break;
	}
    }
    CDestroyWidget ("saving");
    CRestoreState (&s);
}


static char *syntax_get_line (void *data, int line)
{E_
    char **names;
    names = (char **) data;
    return names[line];
}

void menu_syntax_highlighting_dialog (Window parent, int x, int y)
{E_
    char *names[1024] =
    {"None", 0};
    int i, n;
    CWidget *w;

    w = CGetEditMenu ();
    if (!w)
	return;
    if (!w->editor)
	return;

    edit_load_syntax (0, names + 1, 0);
    for (n = 0; names[n]; n++);

    i = CListboxDialog (parent, x, y, 50, 20,
			_ (" Syntax Highlighting "), 0, 0, n,
			syntax_get_line, (void *) names);

    if (i >= 0) {
	if (!i) {
	    edit_free_syntax_rules (w->editor);
	} else {
	    edit_load_syntax (w->editor, 0, names[i]);
	}
	w->editor->explicit_syntax = 1;
    }
    for (n = 1; names[n]; n++)
	free (names[n]);
    CExpose (w->ident);
}



