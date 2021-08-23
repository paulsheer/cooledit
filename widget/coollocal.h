/* coollocal.h  - header for internel usage
   Copyright (C) 1996-2018 Paul Sheer
 */


#ifndef COOLLOCAL_H
#define COOLLOCAL_H

#include "xim.h"


extern Atom ATOM_ICCCM_P2P_CLIPBOARD;
extern Atom ATOM_UTF8_STRING;

/* Various rendering routines called internally */
void render_bevel (Window win, int x1, int y1, int x2, int y2, int thick, int sunken);
long render_textbox (CWidget * w, int redrawall);
void render_bitmap_button (CWidget * w, int state);
void render_bw_image (CWidget * w, int x, int y, int rendw, int rendh);
void render_button (CWidget * w);
void render_textinput (CWidget * w);
void render_window (CWidget * w);
void render_bar (CWidget * w);
void render_text (CWidget * w);
void render_status (CWidget * w, int expose);
void render_scrollbar (CWidget * w);
void render_sunken (CWidget * w);
int which_scrollbar_button (int bx, int by, CWidget * w);
long count_textbox_lines (CWidget * w, int all);
void render_progress (CWidget * w);
void render_switch (CWidget * w);
int (*default_event_handler (int kindofwidget)) (CWidget *, XEvent *, CEvent *);
CWidget **find_empty_widget_entry (void);
CWidget *allocate_widget (Window newwin,
			  const char *ident, Window parent, int x, int y,
			  int width, int height, int kindofwidget);
int eh_picture (struct cool_widget *w, XEvent * xevent, CEvent * cwevent);
void drawstring_xy (Window win, int x, int y, const char *text);
void drawstring_xy_hotkey (Window win, int x, int y, const char *text, int hotkey);
int run_callbacks (CWidget * w, XEvent * xevent, CEvent * cwevent);
void process_external_focus (Window win, int type);
void focus_window (Window win);
void link_scrollbar_to_textbox (CWidget * w, CWidget * textbox, XEvent * xevent, CEvent * cwevent, int whichscrbutton);
void link_scrollbar_to_editor (CWidget * w, CWidget * editor, XEvent * xevent, CEvent * cwevent, int whichscrbutton);
void destroy_picture (CWidget * w);
int edit_translate_key (unsigned int x_keycode, long x_key, int x_state, int *cmd, char *x_lat, int *x_lat_len);
void toggle_cursor (void);
unsigned char *font_wchar_to_charenc (C_wchar_t c, int *l);
KeySym key_sym_xlat (XEvent * ev, char *x_lat, int *x_lat_len);
void textinput_insert (CWidget * w, CStr c);
void cim_send_spot (Window window);
void cim_check_spot_change (Window win);
void childhandler_ (void);

int CCheckGlobalHotKey (XEvent * xevent, CEvent * cwevent, int do_focus);
void CDisableXIM(void);
void CEnableXIM(void);


/* returns non-zero if they keysym of one of the control keys (shift, capslock etc.) */
int mod_type_key (KeySym x);

Window my_XmuClientWindow (Display * dpy, Window win);

void render_focus_border (Window win);

void toggle_radio_button (CWidget * w);
char *whereis_hotchar (const char *labl, int hotkey);
int match_hotkey (KeySym a, KeySym b);
int find_hotkey (CWidget * w);
int find_letter_at_word_start (unsigned char *label, unsigned char *used_keys, int n);

void set_font_tab_width (int t);
int get_font_tab_width (void);

void destroy_focus_border (void);
void create_focus_border (CWidget * w, int border);
void render_rounded_bevel (Window win, int x1, int y1, int x2, int y2, int radius, int thick, int sunken);
long create_input_context (CWidget * w, XIMStyle input_style);
long set_status_position (CWidget * w);
XIMStyle get_input_style (void);

typedef long (*for_all_widgets_cb_t) (CWidget *, void *, void *);
long for_all_widgets (for_all_widgets_cb_t call_back, void *data1, void *data2);

unsigned char *wcrtomb_wchar_to_utf8 (C_wchar_t c);
enum font_encoding get_editor_encoding (void);
int set_editor_encoding (int utf_encoding, int locale_encoding);


struct style_struct {
    unsigned char fg;
    unsigned char bg;
    unsigned short style;
    unsigned int ch;
};

#if 0
typedef struct style_struct cache_type;
#endif

typedef union {
    struct {
	unsigned char fg;
	unsigned char bg;
	unsigned short style;
	unsigned int ch;
    } c;
    unsigned int _style;
} cache_type;

struct _book_mark;
typedef void (*converttext_cb_t) (void *, long, long, cache_type *, cache_type *, int, int, int, struct _book_mark **, int);
typedef int (*calctextpos_cb_t) (void *, long, long *, int);

int edit_draw_proportional (void *data,
	                        converttext_cb_t converttext,
	                        calctextpos_cb_t calctextpos,
				int scroll_right,
				Window win,
				int x_max,
				long b,
				int row,
				int y,
				int x_offset,
				int tabwidth,
                                struct _book_mark **book_marks,
                                int n_book_marks);


#endif				/* ! COOLLOCAL_H */

