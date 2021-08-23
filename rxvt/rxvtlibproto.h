/*--------------------------------*-C-*---------------------------------*
 * File:	rxvtlibproto.h
 *----------------------------------------------------------------------*
 * $Id: command.c,v 1.85.2.23 1999/08/12 16:32:39 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1992      John Bovey, University of Kent at Canterbury <jdb@ukc.ac.uk>
 *				- original version
 * Copyright (C) 1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 * 				- extensive modifications
 * Copyright (C) 1995      Garrett D'Amore <garrett@netcom.com>
 *				- vt100 printing
 * Copyright (C) 1995      Steven Hirsch <hirsch@emba.uvm.edu>
 *				- X11 mouse report mode and support for
 *				  DEC "private mode" save/restore functions.
 * Copyright (C) 1995      Jakub Jelinek <jj@gnu.ai.mit.edu>
 *				- key-related changes to handle Shift+function
 *				  keys properly.
 * Copyright (C) 1997      MJ Olesen <olesen@me.queensu.ca>
 *				- extensive modifications
 * Copyright (C) 1997      Raul Garcia Garcia <rgg@tid.es>
 *				- modification and cleanups for Solaris 2.x
 *				  and Linux 1.2.x
 * Copyright (C) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
 * Copyright (C) 1998      Geoff Wing <gcw@pobox.com>
 * Copyright (C) 1998      Alfredo K. Kojima <kojima@windowmaker.org>
 * Copyright (C) 1996-2017 Paul Sheer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*/

/*************************************************************************/
/* This code has only the vaguest resemblance to rxvt-2.6.1 - Paul Sheer */
/*************************************************************************/



void rxvtlib_init (rxvtlib *o);

int             getdtablesize (void);
void            privileges (int mode);
RETSIGTYPE      Child_signal (int unused);
RETSIGTYPE      Exit_signal (int sig);
void            clean_exit (void);
int             rxvtlib_get_pty (rxvtlib *o);
int             rxvtlib_get_tty (rxvtlib *o);
void            debug_ttymode (const ttymode_t * ttymode);
void            get_ttymode (ttymode_t * tio);
void            rxvtlib_run_command (rxvtlib *o, const char *const *argv, int do_sleep);
void            rxvtlib_get_ourmods (rxvtlib *o);
void            rxvtlib_init_command (rxvtlib *o, const char *const *argv, int do_sleep);
void            rxvtlib_init_xlocale (rxvtlib *o);
void            rxvtlib_tt_winsize (rxvtlib *o, int fd);
void            rxvtlib_tt_resize (rxvtlib *o);
void            rxvtlib_lookup_key (rxvtlib *o, XEvent * ev);
unsigned int    rxvtlib_cmd_write (rxvtlib *o, const unsigned char *str, unsigned int count);
unsigned char   rxvtlib_cmd_getc (rxvtlib *o);
void            rxvtlib_mouse_report (rxvtlib *o, const XButtonEvent * ev);
void            rxvtlib_process_x_event (rxvtlib *o, XEvent * ev);
int             rxvtlib_check_our_parents (rxvtlib *o);
void            rxvtlib_tt_printf (rxvtlib *o, const char *fmt, ...);
FILE           *rxvtlib_popen_printer (rxvtlib *o);
int             pclose_printer (FILE * stream);
void            rxvtlib_process_print_pipe (rxvtlib *o);
void            rxvtlib_process_escape_seq (rxvtlib *o);
void            rxvtlib_process_csi_seq (rxvtlib *o);
void            rxvtlib_process_window_ops (rxvtlib *o, const int *args, int nargs);
void            rxvtlib_process_xterm_seq (rxvtlib *o);
void            rxvtlib_process_terminal_mode (rxvtlib *o, int mode, int priv, unsigned int nargs,
				       const int *arg);
void            rxvtlib_process_sgr_mode (rxvtlib *o, unsigned int nargs, const int *arg);
void            rxvtlib_process_graphics (rxvtlib *o);
void            rxvtlib_AddToCNQueue (rxvtlib *o, int width, int height);
int             rxvtlib_RemoveFromCNQueue (rxvtlib *o, int width, int height);
void            rxvtlib_main_loop (rxvtlib *o);
void            rxvtlib_tt_write (rxvtlib *o, const unsigned char *d, int len);
void            rxvtlib_setSize (rxvtlib *o, XRectangle * size);
void            rxvtlib_setColor (rxvtlib *o, unsigned int *fg, unsigned int *bg);
void            rxvtlib_IMSendSpot (rxvtlib *o);
void            rxvtlib_setTermFontSet (rxvtlib *o);
void            rxvtlib_setPreeditArea (rxvtlib *o, XRectangle * preedit_rect,
				XRectangle * status_rect,
				XRectangle * needed_rect);
void            rxvtlib_IMDestroyCallback (XIM xim, XPointer client_data,
				   XPointer call_data);
void            rxvtlib_IMInstantiateCallback (Display * display, XPointer client_data,
				       XPointer call_data);
void            rxvtlib_IMSetStatusPosition (rxvtlib *o);
void            rxvtlib_XProcessEvent (rxvtlib *o, Display * display);
void            rxvtlib_Gr_NewWindow (rxvtlib *o, int nargs, int args[]);
void            rxvtlib_Gr_ClearWindow (rxvtlib *o, grwin_t * grwin);
void            rxvtlib_Gr_Text (rxvtlib *o, grwin_t * grwin, grcmd_t * data);
void            rxvtlib_Gr_Geometry (rxvtlib *o, grwin_t * grwin, grcmd_t * data);
void            rxvtlib_Gr_DestroyWindow (rxvtlib *o, grwin_t * grwin);
void            rxvtlib_Gr_Dispatch (rxvtlib *o, grwin_t * grwin, grcmd_t * data);
void            rxvtlib_Gr_Redraw (rxvtlib *o, grwin_t * grwin);
void            rxvtlib_Gr_ButtonReport (rxvtlib *o, int but, int x, int y);
void            rxvtlib_Gr_do_graphics (rxvtlib *o, int cmd, int nargs, int args[],
				unsigned char *text);
void            rxvtlib_Gr_scroll (rxvtlib *o, int count);
void            rxvtlib_Gr_ClearScreen (rxvtlib *o);
void            rxvtlib_Gr_ChangeScreen (rxvtlib *o);
void            rxvtlib_Gr_expose (rxvtlib *o, Window win);
void            rxvtlib_Gr_Resize (rxvtlib *o, int w, int h);
void            rxvtlib_Gr_reset (rxvtlib *o);
int             rxvtlib_Gr_Displayed (rxvtlib *o);
/* void            rxvt_update_wtmp (const char *fname, const struct utmp *putmp); */
void            rxvtlib_makeutent (rxvtlib *o, const char *pty, const char *hostname);
void            rxvtlib_cleanutent (rxvtlib *o);
XErrorHandler   xerror_handler (const Display * display,
				const XErrorEvent * event);
void            rxvtlib_color_aliases (rxvtlib *o, int idx);
void            rxvtlib_set_colorfgbg (rxvtlib *o);
void            rxvtlib_Get_Colours (rxvtlib *o);
void            rxvtlib_Create_Windows (rxvtlib *o, int argc, const char *const *argv);
void            rxvtlib_resize_subwindows (rxvtlib *o, int width, int height);
void            rxvtlib_resize_all_windows (rxvtlib *o);
void            rxvtlib_resize_window (rxvtlib *o, unsigned int width, unsigned int height);
void            rxvtlib_set_widthheight (rxvtlib *o, unsigned int width, unsigned int height);
void            rxvtlib_szhints_set (rxvtlib *o);
void            rxvtlib_szhints_recalc (rxvtlib *o);
void            rxvtlib_set_title (rxvtlib *o, const char *str);
void            rxvtlib_set_iconName (rxvtlib *o, const char *str);
void            rxvtlib_set_window_color (rxvtlib *o, int idx, const char *color);
void            rxvtlib_xterm_seq (rxvtlib *o, int op, const char *str);
void            rxvtlib_change_font (rxvtlib *o, int init, const char *fontname);
void            rxvtlib_init_vars (rxvtlib *o);
const char    **rxvtlib_init_resources (rxvtlib *o, int argc, const char *const *argv);
void            rxvtlib_init_env (rxvtlib *o);
int             rxvtlib_main (rxvtlib *o, int argc, const char *const *argv, int do_sleep);
menuitem_t     *menuitem_find (const menu_t * menu, const char *name);
void            rxvtlib_menuitem_free (rxvtlib *o, menu_t * menu, menuitem_t * item);
int             action_type (action_t * action, unsigned char *str);
int             rxvtlib_action_dispatch (rxvtlib *o, action_t * action);
int             rxvtlib_menuarrow_find (rxvtlib *o, char name);
void            rxvtlib_menuarrow_free (rxvtlib *o, char name);
void            rxvtlib_menuarrow_add (rxvtlib *o, char *string);
menuitem_t     *rxvtlib_menuitem_add (rxvtlib *o, menu_t * menu, const char *name,
			      const char *name2, const char *action);
char           *rxvtlib_menu_find_base (rxvtlib *o, menu_t ** menu, char *path);
menu_t         *rxvtlib_menu_delete (rxvtlib *o, menu_t * menu);
menu_t         *rxvtlib_menu_add (rxvtlib *o, menu_t * parent, char *path);
void            rxvtlib_drawbox_menubar (rxvtlib *o, int x, int len, int state);
void            rxvtlib_drawtriangle (rxvtlib *o, int x, int y, int state);
void            rxvtlib_drawbox_menuitem (rxvtlib *o, int y, int state);
void            rxvtlib_print_menu_ancestors (rxvtlib *o, menu_t * menu);
void            rxvtlib_print_menu_descendants (rxvtlib *o, menu_t * menu);
void            rxvtlib_menu_show (rxvtlib *o);
void            rxvtlib_menu_display (rxvtlib *o, void (*update) (rxvtlib *));
void            rxvtlib_menu_hide_all (rxvtlib *o);
void            rxvtlib_menu_hide (rxvtlib *o);
void            rxvtlib_menu_clear (rxvtlib *o, menu_t * menu);
void            rxvtlib_menubar_clear (rxvtlib *o);
bar_t          *rxvtlib_menubar_find (rxvtlib *o, const char *name);
int             rxvtlib_menubar_push (rxvtlib *o, const char *name);
void            rxvtlib_menubar_remove (rxvtlib *o, const char *name);
void            action_decode (FILE * fp, action_t * act);
void            rxvtlib_menu_dump (rxvtlib *o, FILE * fp, menu_t * menu);
void            rxvtlib_menubar_dump (rxvtlib *o, FILE * fp);
void            rxvtlib_menubar_read (rxvtlib *o, const char *filename);
void            rxvtlib_menubar_dispatch (rxvtlib *o, char *str);
void            rxvtlib_draw_Arrows (rxvtlib *o, int name, int state);
void            rxvtlib_menubar_expose (rxvtlib *o);
int             rxvtlib_menubar_mapping (rxvtlib *o, int map);
int             rxvtlib_menu_select (rxvtlib *o, XButtonEvent * ev);
void            rxvtlib_menubar_select (rxvtlib *o, XButtonEvent * ev);
void            rxvtlib_menubar_control (rxvtlib *o, XButtonEvent * ev);
void            rxvtlib_map_menuBar (rxvtlib *o, int map);
void            rxvtlib_create_menuBar (rxvtlib *o, Cursor cursor);
void            rxvtlib_Resize_menuBar (rxvtlib *o, int x, int y, unsigned int width,
				unsigned int height);
/* void            rmemset (void *p, unsigned char c, intp_t len); */
/* void            blank_line (text_t * et, rend_t * er, int width, rend_t efs); */
/* void            rxvtlib_blank_screen_mem (rxvtlib *o, text_t ** tp, rend_t ** rp, int row,
				  rend_t efs); */
void            rxvtlib_scr_reset (rxvtlib *o);
void            rxvtlib_scr_reset_realloc (rxvtlib *o);
void            rxvtlib_scr_release (rxvtlib *o);
void            rxvtlib_scr_poweron (rxvtlib *o);
void            rxvtlib_scr_cursor (rxvtlib *o, int mode);
int             rxvtlib_scr_change_screen (rxvtlib *o, int scrn);
void            rxvtlib_scr_color (rxvtlib *o, unsigned int color, unsigned int Intensity);
void            rxvtlib_scr_rendition (rxvtlib *o, int set, int style);
int             rxvtlib_scroll_text (rxvtlib *o, int row1, int row2, int count, int spec);
void            rxvtlib_scr_scroll_text (rxvtlib *o, int count);
void            rxvtlib_scr_add_lines (rxvtlib *o, const unsigned char *str, int nlines, int len);
void            rxvtlib_scr_backspace (rxvtlib *o);
void            rxvtlib_scr_tab (rxvtlib *o, int count);
void            rxvtlib_scr_backindex (rxvtlib *o);
void            rxvtlib_scr_forwardindex (rxvtlib *o);
void            rxvtlib_scr_gotorc (rxvtlib *o, int row, int col, int relative);
void            rxvtlib_scr_index (rxvtlib *o, int direction);
void            rxvtlib_scr_erase_line (rxvtlib *o, int mode);
void            rxvtlib_scr_erase_screen (rxvtlib *o, int mode);
void            rxvtlib_scr_E (rxvtlib *o);
void            rxvtlib_scr_insdel_lines (rxvtlib *o, int count, int insdel);
void            rxvtlib_scr_insdel_chars (rxvtlib *o, int count, int insdel);
void            rxvtlib_scr_scroll_region (rxvtlib *o, int top, int bot);
void            rxvtlib_scr_cursor_visible (rxvtlib *o, int mode);
void            rxvtlib_scr_autowrap (rxvtlib *o, int mode);
void            rxvtlib_scr_relative_origin (rxvtlib *o, int mode);
void            rxvtlib_scr_insert_mode (rxvtlib *o, int mode);
void            rxvtlib_scr_set_tab (rxvtlib *o, int mode);
void            rxvtlib_scr_rvideo_mode (rxvtlib *o, int mode);
void            rxvtlib_scr_report_position (rxvtlib *o);
void            rxvtlib_set_font_style (rxvtlib *o);
void            rxvtlib_scr_charset_choose (rxvtlib *o, int set);
void            rxvtlib_scr_charset_set (rxvtlib *o, int set, unsigned int ch);
void            eucj2jis (unsigned char *str, int len);
void            sjis2jis (unsigned char *str, int len);
void            big5dummy (unsigned char *str, int len);
void            gb2jis (unsigned char *str, int len);
void            rxvtlib_set_multichar_encoding (rxvtlib *o, const char *str);
int             rxvtlib_scr_get_fgcolor (rxvtlib *o);
int             rxvtlib_scr_get_bgcolor (rxvtlib *o);
void            rxvtlib_scr_expose (rxvtlib *o, int x, int y, int width, int height);
void            rxvtlib_scr_touch (rxvtlib *o);
int             rxvtlib_scr_move_to (rxvtlib *o, int y, int len);
int             rxvtlib_scr_page (rxvtlib *o, int direction, int nlines);
void            rxvtlib_scr_bell (rxvtlib *o);
void            rxvtlib_scr_printscreen (rxvtlib *o, int fullhist);
void            rxvtlib_scr_refresh (rxvtlib *o, int type);
void            rxvtlib_scr_clear (rxvtlib *o);
void            rxvtlib_scr_reverse_selection (rxvtlib *o);
void            rxvtlib_selection_check (rxvtlib *o, int check_more);
void            rxvtlib_PasteIt (rxvtlib *o, const unsigned char *data, unsigned int nitems);
void            rxvtlib_selection_paste (rxvtlib *o, Window win, unsigned int prop, int Delete);
void            rxvtlib_selection_request (rxvtlib *o, Time tm, int x, int y);
void            rxvtlib_selection_clear (rxvtlib *o);
void            rxvtlib_selection_make (rxvtlib *o, Time tm);
void            rxvtlib_selection_click (rxvtlib *o, int clicks, int x, int y);
void            rxvtlib_selection_start_colrow (rxvtlib *o, int col, int row);
void            rxvtlib_selection_delimit_word (rxvtlib *o, int dirn, const row_col_t * mark,
					row_col_t * ret);
void            rxvtlib_selection_extend (rxvtlib *o, int x, int y, int flag);
void            rxvtlib_selection_adjust_kanji (rxvtlib *o);
void            rxvtlib_selection_extend_colrow (rxvtlib *o, int col, int row, int button3,
					 int buttonpress, int clickchange);
void            rxvtlib_selection_rotate (rxvtlib *o, int x, int y);
void            rxvtlib_selection_send (rxvtlib *o, const XSelectionRequestEvent * rq);
void            pixel_position (rxvtlib *o, int *x, int *y);
void            mouse_tracking (int report, int x, int y, int firstrow,
				int lastrow);
void            debug_PasteIt (unsigned char *data, int nitems);
void            rxvtlib_setPosition (rxvtlib *o, XPoint * pos);
void            rxvtlib_Draw_button (rxvtlib *o, int x, int y, int state, int dirn);
Pixmap          rxvtlib_renderPixmap (rxvtlib *o, char **data, int width, int height);
void            rxvtlib_init_scrollbar_stuff (rxvtlib *o);
void            rxvtlib_drawBevel (rxvtlib *o, Drawable d, int x, int y, int w, int h);
int             rxvtlib_scrollbar_show (rxvtlib *o, int update);
int             rxvtlib_scrollbar_mapping (rxvtlib *o, int map);
void            rxvtlib_map_scrollBar (rxvtlib *o, int map);
void            rxvtlib_usage (rxvtlib *o, int type);
void            rxvtlib_get_options (rxvtlib *o, int argc, const char *const *argv);
Bool            rxvtlib_define_key (rxvtlib *o, XrmDatabase * database, XrmBindingList bindings,
			    XrmQuarkList quarks, XrmRepresentation * type,
			    XrmValue * value, XPointer closure);
int             rxvtlib_parse_keysym (rxvtlib *o, const char *str, const char *arg);
void            rxvtlib_get_xdefaults (rxvtlib *o, FILE * stream, const char *name);
void            rxvtlib_extract_resources (rxvtlib *o, Display * display, const char *name);
int             rxvtlib_scale_pixmap (rxvtlib *o, const char *geom);
void            rxvtlib_resize_pixmap (rxvtlib *o);
Pixmap          rxvtlib_set_bgPixmap (rxvtlib *o, const char *file);
const char     *my_basename (const char *str);
void            print_error (const char *fmt, ...);
int             Str_match (const char *s1, const char *s2);
const char     *Str_skip_space (const char *str);
char           *Str_trim (char *str);
int             Str_escaped (char *str);
char           *File_search_path (const char *pathlist, const char *file,
				  const char *ext);
char           *rxvtlib_File_find (rxvtlib *o, const char *file, const char *ext);
void            rxvtlib_Draw_Shadow (rxvtlib *o, Window win, GC topShadow, GC botShadow, int x,
			     int y, int w, int h);
void            rxvtlib_Draw_Triangle (rxvtlib *o, Window win, GC topShadow, GC botShadow, int x,
			       int y, int w, int type);
char           *network_display (const char *display);
