/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* coolwidget.h - main header file
   Copyright (C) 1996-2022 Paul Sheer
 */


#ifndef COOL_WIDGET_H
#define COOL_WIDGET_H

#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <locale.h>

#include "gettext.h"

#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

#include "dirtools.h"

#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xfuncproto.h>
#include "global.h"
#include "vgagl.h"
#include "lkeysym.h"

#include "stringtools.h"
#include "app_glob.c"
#include "drawings.h"
#include "3dkit.h"
#include "xdnd.h"
#include "mousemark.h"
#include "font.h"
 
#define ALRM_PER_SECOND 50
#define NICE_FILENAME_CHARS "+-.0123456789:=@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~"

typedef struct initialisation {
    char *name, *display, *geometry, *font, *widget_font, *bg;
    char *_8bit_term_font;
    char *fg_red, *fg_green, *fg_blue;	/* string doubles */
#define CINIT_OPTION_USE_GREY 1
#define CINIT_OPTION_VERBOSE 2
#define CINIT_OPTION_WAIT_FOR_DISPLAY 4
    unsigned long options;
    char *look;
} CInitData;


#define CEdit WEdit

/* edit this */
/* #define u_32bit_t unsigned long */
#define u_32bit_t unsigned int
#define word unsigned short
#define byte unsigned char

#define WIDGET_MAGIC_BEGIN 0x6e065f4d
#define WIDGET_MAGIC_END 0x54f560e9

#define TEXT_SET_COLUMN		1
#define TEXT_SET_LINE		2
#define TEXT_SET_POS		3
#define TEXT_SET_CURSOR_LINE	4

/* These are all the widget kinds (the kind member of the widget structure) */
enum { \
    C_NOT_A_WIDGET, C_BUTTON_WIDGET, C_WINDOW_WIDGET, C_BAR_WIDGET, C_SUNKEN_WIDGET, \
    C_VERTSCROLL_WIDGET, C_HORISCROLL_WIDGET, C_HORSCROLL_WIDGET, C_TEXTINPUT_WIDGET, \
    C_TEXTBOX_WIDGET, C_TEXT_WIDGET, C_BWIMAGE_WIDGET, C_SPREAD_WIDGET, \
    C_PROGRESS_WIDGET, C_BITMAP_WIDGET, C_BITMAPBUTTON_WIDGET, C_SWITCH_WIDGET, \
    C_8BITIMAGE_WIDGET, C_THREED_WIDGET, C_PICTURE_WIDGET, C_EDITOR_WIDGET, \
    C_MENU_WIDGET, C_MENU_BUTTON_WIDGET, C_ALARM_WIDGET, C_FIELDED_TEXTBOX_WIDGET, \
    C_TOOLHINT_WIDGET, C_ICON_WIDGET, C_STATUS_WIDGET, C_RXVT_WIDGET, C_UNICODE_WIDGET
};

/* the user can start creating his widgets from 100: */
#define C_LAST_WIDGET 100

#define Button12345Mask		(Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)

/*
   Here are some addition events that you may recieve or send using
   CNextEvent instead of XNextEvent. (LASTEvent = 35)
 */
/* this comes every 1/CGetCursorBlinkRate() of a second */
#define AlarmEvent		(LASTEvent + 1)
/* This you won't recieve ---  it is used for joining small expose regions together */
#define InternalExpose		(LASTEvent + 2)
/* Send this event to the editor to force the editor widget to execute a command */
#define EditorCommand		(LASTEvent + 3)
/* Comes every 1/ALRM_PER_SECOND of a second (see initapp.c for correct amount) */
#define TickEvent		(LASTEvent + 4)
/* a button repeat event happens when a mouse button is pressed and held down */
#define ButtonRepeat		(LASTEvent + 5)
/* When you recieve this it is because the window manager wants the app to
   quit. You would recieve this when the user has pressed on the main window. */
#define QuitApplication		(LASTEvent + 6)
#define CLASTEvent		(LASTEvent + 7)

/* Library is limited to this number of widgets at once */
#define MAX_NUMBER_OF_WIDGETS	1024

/* one of the (long int) "CoolBlue" colors (0-15) that make up the windows, buttons etc */
#define color_widget(i)		color_pixels[i].raw
/* one of a 3x3x3 (RxGxB) color palette. eg R=2, G=1, B=0 is color_palette(19). */
#define N_WIDGET_COLORS         16
#define N_FAUX_COLORS            27
#define color_palette(i)	color_pixels[(i) + N_WIDGET_COLORS].raw
#define color_palette_name(i)	color_pixels[(i) + N_WIDGET_COLORS].name
/* 0-64 grey levels (not supprted unless specified in config.h) */
#define color_grey(i)		color_pixels[(i) + N_WIDGET_COLORS + N_FAUX_COLORS].raw

int allocate_color (char *color);

/* draw a line in the window d */
#define CLine(d, x, y, w, h)		XDrawLine(CDisplay, d, CGC, x, y, w, h)
/* rectangle */
#define CRectangle(d, x, y, w, h)	XFillRectangle(CDisplay, d, CGC, x, y, w, h)
/* rectangle */
#define CBox(d, x, y, w, h)		XDrawRectangle(CDisplay, d, CGC, x, y, w, h)
/* set the foreground color */
#define CSetColor(c)			XSetForeground(CDisplay, CGC, c)
/* set the background color */
#define CSetBackgroundColor(c)		XSetBackground(CDisplay, CGC, c)
/* width of text in pixels in the current font */
int CImageTextWidth (const char *s, int l);
/* width of a string in pixels in the current font Wide Character */
int CImageTextWidthWC (XChar2b * s, C_wchar_t * swc, int l);
/* width of a string in pixels in the current font */
int CImageStringWidth (const char *s);
/* draw image string */
int CImageString (Window w, int x, int y, const char *s);
/* draw image text */
int CImageText (Window w, int x, int y, const char *s, int l);
/* draw image string Wide Character - supply both wc and normal string formats */
int CImageTextWC (Window w, int x, int y, XChar2b * s, C_wchar_t * swc, int l);


unsigned char *wcrtomb_ucs4_to_utf8 (C_wchar_t c);

#define SCREEN_ASPECT_RATIO 1.333

/* some standard colors */
/* the color of the "flat" of a window */
#define COLOR_FLAT ((*look->get_button_flat_color) ())

/* used for NeXTish drawings */
#define COLOR_DARK  color_widget(5)
#define COLOR_LIGHT color_widget(8)

#define COLOR_WHITE color_widget(15)
#define COLOR_BLACK color_widget(0)

/* SelectInput for various types of widgets */
#define INPUT_EXPOSE (ExposureMask | StructureNotifyMask | VisibilityChangeMask | \
			LeaveWindowMask | EnterWindowMask | PointerMotionMask)

#define INPUT_KEY (ExposureMask | ButtonPressMask | ButtonReleaseMask | \
			ButtonMotionMask | StructureNotifyMask | PropertyChangeMask | \
			LeaveWindowMask | EnterWindowMask | PointerMotionMask)
#define INPUT_MOTION INPUT_KEY
#define INPUT_BUTTON INPUT_KEY

/* internal */
#define WINDOW_MAPPED 1
#define WINDOW_FOCUS_WHEN_MAPPED 2

/* menu callback function */
typedef void (*callfn) (unsigned long data);

struct menu_item {
    char *text;
    char hot_key;
    callfn call_back;
    unsigned long data;		/* data for user. it is passed to callfn */
};

#define MENU_ITEM_BASIC(text,hot_key,callback,data) \
    text, hot_key, callback, data

/* not supported
   #define MENU_ITEM_OPTION(text,hot_key,option) \
   text, hotkey, (callfn) 1, (int *) option
   #define MENU_ITEM_SECOND_LEVEL(text,hotkey,menuitems) \
   text, hotkey, (callfn) 2, (struct menu_item *) menuitems
 */

#define ClassOfVisual(v) ((v)->class)

/* spacing between widgets in pixels */
#define WIDGET_SPACING option_interwidget_spacing
#define WIDGET_FOCUS_RING ((*look->get_focus_ring_size) ())
#define WINDOW_EXTRA_SPACING ((*look->get_extra_window_spacing) ())

/* spacing between the bevel and the text of the text widget */
#define TEXT_RELIEF 3

/* don't change this without adjusting white fill in inputwidget.c */
#define TEXTINPUT_RELIEF 1
#define BUTTON_RELIEF 2

/* auto widget sizing (use instead of width or height to work out the width
   of widgets that have text in them) */
#define AUTO_WIDTH		-32000
#define AUTO_HEIGHT		-32001
#define AUTO_SIZE		AUTO_WIDTH, AUTO_HEIGHT

#define TEXTINPUT_LAST_INPUT	((void *) 1)

/* font offsets for drawing */
#define FONT_OFFSET_X 0

#define EDIT_FRAME_W 7
#define EDIT_FRAME_H 7

/* if this gets changed, the cursor rendering (upside down "L") in editdraw and
   cooledit.c must be adjusted so that the cursor is erased properly */
#define FONT_OFFSET_Y		FONT_BASE_LINE


/*
   A reduced event. This structure is returned by CNextEvent, and
   contains most of the things you may need. Anything more is contained
   in XEvent.
 */
typedef struct {
/* widget's identification string */
    char *ident;
    int i;

/* data */
    int x;
    int y;
    int xt;
    int yt;

    Window window;

/* enumerated above */
    int kind;
    int type;

/* if a key was pressed, this is the KeySym */
    long key;
#define MAX_KBUF        128
    char xlat[MAX_KBUF];	/* translation */
    int xlat_len;
    Time time;
    unsigned int button;

/* 1 for a double click */
    int double_click;
    unsigned int state;

/* if text was returned by the event */
    char *text;

/* if the event was already handled by a callback routine */
    char handled;

/* if the event coused an editor command */
    int command;		/* editor commands */
} CEvent;

struct textbox_funcs_struct {
    CStr (*textbox_text_cb) (void *, void *);
    void (*textbox_free_cb) (void *, void *);
    void *hook1;
    void *hook2;
};

/* This is the structure of a widget. It is presently 232 bytes
   long. With 100 widgets on at once, this is only 23k, so the
   inefficiency in having only one structure is justified. */
struct cool_widget {
    char ident[33];		/*unique identifying string given by user */
/* for debugging */
    u_32bit_t magic_begin;

/* essentials */
    Window winid;		/* X integer window id of widget */
    Window parentid;		/* parent window of window */
    Window mainid;		/* main window of window */
    int (*eh) (struct cool_widget *, XEvent *, CEvent *);	/* internal event handler */
    int (*callback_before) (struct cool_widget *, XEvent *, CEvent *);	/* user event handler called before internal handler */
    int (*callback) (struct cool_widget *, XEvent *, CEvent *);		/* user event handler */
    void (*destroy) (struct cool_widget *);	/*only a few widgets need special distruction */
    void (*resize) (int, int, int, int, int *, int *, int *, int *);	/* when the parent is resized,
									   the new size and position are 
									   returned by this function */
    void (*render) (struct cool_widget *w);
    char ** (*get_line) (void *data, int line_number, int *num_fields, int *tagged);	/* returns a text box line */

/* void resize (
 *          old_parent_w, old_parent_h, 
 *          new_parent_w, new_parent_h, 
 *          int *new_this_widget_x, int *new_this_widget_y, 
 *          int *new_this_widget_w, int *new_this_widget_h
 *      );
 */
    void (*scroll_bar_link) (struct cool_widget *, struct cool_widget *, XEvent *, CEvent *, int);
    void (*scroll_bar_extra_render) (struct cool_widget *);

/*
 * void scroll_bar_link (scrollbar, widget, xevent, cwevent, which_button_1_to_5);
 */
/* basics */
    int width, height;		/* of window --- just to save looking it up */
    int x, y;			/* position in parent --- top left corner */
    int kind;			/* one of C??????_WIDGET above */
    char disabled;		/* displayed, but not functioning */
    char takes_focus;		/* can this widget take input focus? */
    char mapped;		/* has window yet been mapped */
    char nothing;		/* pad */

/* data */
    char *label;		/* text that gets drawn into button */
    char *graphic;		/* Possibly a bitmap to go onto the button */
    int *tab;			/* columns for spreadsheat widget */
    CStr text;			/* text goes into textbox textinput and text widgets */
    char *headings;		/* headings of spreadsheet columns */
    GraphicsContext *gl_graphicscontext;	/*for svgalib image widgets */
    XImage *ximage;		/* for X images picture widgets */
    Pixmap pixmap;		/* for pixmaps */
    CPicture *pic;		/* for lines, circles, rects and arcs. */
    TD_Solid *solid;
    char *toolhint;		/* hints for buttons */
    struct editor_widget *editor;
    struct menu_item *menu;

/* Positions. What they are used for depends on the kind of widget. See coolwidget.c for an explanation */
    long cursor;
    long column;
    long numlines;
    long resize_gran;
    long firstline;
    long current;
    long firstcolumn;
    long mark1, mark2;
    long search_start;
    int search_len;
    Window last_child_focussed;	/* This is for main windows. It records
				   the last child within this main window
				   that has the focus. If the window
				   manager switches focus to another
				   window, then when focus is switch back,
				   this will have kept the correct child to
				   recieve focus. */

/* settings */
    unsigned long options;
#define BUTTON_HIGHLIGHT		(1<<1)
#define BUTTON_PRESSED			(1<<2)

#define MENU_AUTO_PULL_UP		(1<<3)

#define EDITOR_NO_FILE			(1<<3)
#define EDITOR_NO_SCROLL		(1<<4)
#define EDITOR_NO_TEXT			(1<<5)
#define EDITOR_HORIZ_SCROLL		(1<<6)

/* #define FILELIST_LAST_ENTRY		(1<<8)  See dirtools.h */
#define FILELIST_TAGGED_ENTRY		(1<<9)
/* default is sort by name */
#define FILELIST_SORT_UNSORTED		(1<<10)
#define FILELIST_SORT_EXTENSIONS	(1<<11)
#define FILELIST_SORT_SIZE		(1<<12)
#define FILELIST_SORT_MDATE		(1<<13)
#define FILELIST_SORT_CDATE		(1<<14)
/* #define FILELIST_FILES_ONLY		(1<<15) See dirtools.h */
/* #define FILELIST_DIRECTORIES_ONLY	(1<<16) See dirtools.h */
#define FILELIST_ALL_FILES		(FILELIST_DIRECTORIES_ONLY|FILELIST_FILES_ONLY)

/* these musn't be within the first 8 bits (see bitmapbutton.c) */ 
#define RADIO_INVERT_GROUP		(1<<8)
#define RADIO_ONE_ALWAYS_ON		(1<<9)
#define SWITCH_PICTURE_TYPE		(1<<10)

#define TEXTBOX_FILE_LIST		(1<<1)
#define TEXTBOX_MAN_PAGE		(1<<2)
#define TEXTBOX_MARK_WHOLE_LINES	(1<<3)
#define TEXTBOX_NO_CURSOR		(1<<4)
#define TEXTBOX_NO_KEYS			(1<<5)
#define TEXTBOX_WRAP			(1<<7)
#if 0
#define TEXTBOX_TAB_AS_ARROW            (1<<8)
#endif

#define TEXTINPUT_PASSWORD		(1<<3)
#define TEXTINPUT_NUMBERS		(1<<4)
#define TEXTINPUT_NOHISTORY		(1<<5)

#define TEXT_CENTRED			(1<<3)
#define TEXT_FIXED			(1<<4)

#define WINDOW_HAS_HEADING		(1<<1)
#define WINDOW_SIZE_HINTS_SET		(1<<2)
#define WINDOW_USER_POSITION		(1<<3)
#define WINDOW_USER_SIZE		(1<<4)
#define WINDOW_NO_BORDER		(1<<5)

/* must be higher than all other options */
#define WIDGET_HOTKEY_ACTIVATES		(1<<17)
#define WIDGET_TAKES_FOCUS_RING		(1<<18)
#define WIDGET_TAKES_SELECTION		(1<<19)
#define WIDGET_FREE_USER_ON_DESTROY	(1<<20)

    unsigned long position;
#define WINDOW_ALWAYS_RAISED	(1<<0)	/* remains on top when even after CRaise'ing other windows */
#define WINDOW_ALWAYS_LOWERED	(1<<1)	/* remins on bottom */
#define WINDOW_UNMOVEABLE	(1<<2)	/* cannot be moved by clicking on the window's background */
#define WINDOW_RESIZABLE	(1<<3)	/* can be resized (has cosmetic in the lower right corner) */

/* these tell the widgets behaviour on resizing its parent window */
#define POSITION_RIGHT		(1<<4)	/* moves to follow the right border */
#define POSITION_WIDTH		(1<<5)	/* resizes to follow the right border */
#define POSITION_BOTTOM		(1<<6)
#define POSITION_HEIGHT		(1<<7)
#define POSITION_CENTRE		(1<<8)	/* centres from left to right */
#define POSITION_FILL		(1<<9)	/* fills to right border */

#define WINDOW_MAXIMISED	(1<<10)

/* links to other widgets as needed */
    struct cool_widget *hori_scrollbar;
    struct cool_widget *vert_scrollbar;
    struct cool_widget *textbox;
    struct cool_widget *textinput;
    char droppedmenu[33];
    struct mouse_funcs *funcs;
    struct textbox_funcs_struct *textbox_funcs;

    char keypressed;			/* has a key been pressed since this widgets creation (used for text input) */
    char resized;			/* has the widget just been resized? you can check this before rendering, and then reset to 0 */
    unsigned short hotkey;		/* sometimes used */

    unsigned long fg;			/* colors */
    unsigned long bg;

/* used for internal widgets for additional data */
    void *hook;

/* user structure. you can put addition data that you might need in here */
    void *user;
    void (*free_user) (void *);
    XIC input_context;
    void *rxvt;
    Pixmap pixmap_mask;

/* for debugging */
    u_32bit_t magic_end;
};

typedef struct cool_widget CWidget;

/* you may want to use these */
#define CTextOf(w) ((w)->text)
#define CLabelOf(w) ((w)->label)
#define CUserOf(w) ((w)->user)
#define CHeightOf(w) ((w)->height)
#define CWidthOf(w) ((w)->width)
#define CXof(w) ((w)->x)
#define CYof(w) ((w)->y)
#define CWindowOf(w) ((w)->winid)
#define CParentOf(w) ((w)->parentid)
#define CIdentOf(w) ((w)->ident)
#define CWindowOf(w) ((w)->winid)
#define CHeightOf(w) ((w)->height)
#define CWidthOf(w) ((w)->width)
#define COptionsOf(w) ((w)->options)

/* internal */
typedef struct disabled_state {
    u_32bit_t state[(MAX_NUMBER_OF_WIDGETS + 31) / 32];
    u_32bit_t mask[(MAX_NUMBER_OF_WIDGETS + 31) / 32];
} CState;

/*
   The only global variables for the widgets. This is the actual array
   of pointers that holds the malloced widget structures
 */

#ifdef COOL_WIDGET_C
int last_widget;		/* gives length of widget list */
CWidget *widget[MAX_NUMBER_OF_WIDGETS];		/* first widget is at 1 */
#else
extern int last_widget;
extern CWidget *widget[MAX_NUMBER_OF_WIDGETS];
#endif

/* CIndex(i) used to return a pointer to the widget i */
#define CIndex(i) widget[i]

/* returns a pointer to the widget called ident or 0 if not found */
CWidget *CIdent (const char *ident);

/* returns a pointer to the widget of window win */
CWidget *CWidgetOfWindow (Window win);

/* Returns the widgets window or 0 if not found */
Window CWindowOfWidget (const char *ident);

/* Returns the first parent, grandparent, etc, of the window that is of the C_WIDGET_WINDOW type */
CWidget *CDialogOfWindow (Window window);

/* Returns top level window of the widget */
CWidget *CMainOfWindow (Window window);

/* Initialise. Opens connection to the X display, processing -display -font, and -geom args
   sets up GC's, visual's and so on */
void CInitialise (CInitData * config);

/* returns non-zero if a child exitted */
int CChildExitted (pid_t p, int *status);

/* Call when app is done. This undraws all widgets, free's all data
   and closes the connection to the X display */
void CShutdown (void);

/* Prints an error to stderr, or to a window if one can be created, then exits */
void CError (const char *fmt,...);

/* fd watch for internal select */
#define CAddWatch(fd, cb, how, data) _CAddWatch(__FILE__, __LINE__, fd, cb, how, data)
int _CAddWatch (char *file, int line, int fd, void (*callback) (int, fd_set *, fd_set *, fd_set *, void *), int how, void *data);
void CRemoveWatch (int fd, void (*callback) (int, fd_set *, fd_set *, fd_set *, void *), int how);
int CCheckWatch (int fd, void (*callback) (int, fd_set *, fd_set *, fd_set *, void *), int how);
#define WATCH_READING	(1<<0)
#define WATCH_WRITING	(1<<1)
#define WATCH_ERROR	(1<<2)

/* Normal malloc with check for 0 return */
void *CMalloc (size_t size);
void *CDebugMalloc (size_t x, int line, const char *file);

/* get UTF-8 sequence from user selected glyph */
unsigned char *CGetUnichar (Window in, char *heading);

/* Draw a panel onto which widgets will be drawn, this must not be a main window */
Window CDrawDialog (const char *identifier, Window parent, int x, int y);

/* Draw a panel with a heading and a seperator line. The position below the
   seperator line is recorded in h, start drawing in the window from there. */
Window CDrawHeadedDialog (const char *identifier, Window parent, int x, int y, const char *label);
void CMapDialog (const char *ident);

#define CDrawMainWindow(a,b) CDrawHeadedDialog (a, CRoot, 0, 0, b)
void CSetBackgroundPixmap (const char *ident, const char *data[], int w, int h, char start_char);

void CSetWindowResizable (const char *ident, int min_width, int min_height, int max_width, int max_height);

/* returns the direct child of the root window */
Window CGetWMWindow (Window win);
void CRaiseWMWindow (char *ident);

/* Draw a button */
CWidget *CDrawButton (const char *identifier, Window parent, int x, int y,
		      int width, int height, const char *label);

/* Set tool hint for a widget */
void CSetToolHint (const char *ident, const char *text);

/* Draw a button with a bitmap on it, (see dialog.c for example) */
CWidget *CDrawBitmapButton (const char *identifier, Window parent, int x, int y,
			    int width, int height, unsigned long fg, unsigned long bg, const unsigned char data[]);
#define TICK_BUTTON_WIDTH 44
#define PIXMAP_BUTTON_TICK TICK_BUTTON_WIDTH, TICK_BUTTON_WIDTH, tick_bits, '0'
#define PIXMAP_BUTTON_SAVE TICK_BUTTON_WIDTH, TICK_BUTTON_WIDTH, save_pixmap, '0'
#define PIXMAP_BUTTON_CROSS TICK_BUTTON_WIDTH, TICK_BUTTON_WIDTH, cross_bits, '0'
#define PIXMAP_BUTTON_EXCLAMATION TICK_BUTTON_WIDTH, TICK_BUTTON_WIDTH, exclam_bits, '0'

CWidget *CDrawPixmapButton (const char *identifier, Window parent,
    int x, int y, int width, int height, const char *data[], char start_char);

/* Draws a toggle switch, pass on as the default setting */
CWidget *CDrawSwitch (const char *identifier, Window parent, int x, int y, int on, const char *label, int group);

/* Draw status line  - like a single line text widget but interprets colours */
CWidget *CDrawStatus (const char *identifier, Window parent, int x, int y, int w, char *str);

/* Draw a text input widget */
CWidget *CDrawTextInput (const char *identifier, Window parent, int x, int y,
		  int width, int height, int maxlen, const CStr *text);
CWidget *CDrawTextFixed (const char *identifier, Window parent, int x, int y, const char *fmt,...);
CWidget *CDrawTextInputP (const char *identifier, Window parent, int x, int y,
		     int width, int height, int maxlen, const char *text);
/* Adds a line of text to the history of a text input. */
void CAddToTextInputHistory (const char *ident, CStr s);
/* returns the most recent text inputted into a text widget of this name, do not free result */
CStr CLastInput (const char *ident);

/* draws a fielded (like a ledger) text box */
CWidget *CDrawFieldedTextbox (const char *identifier, Window parent, int x, int y,
			      int width, int height, int line, int column,
			      char **(*get_line) (void *, int, int *, int *),
			      long options, void *data);

struct file_entry *get_file_entry_list (const char *host, char *directory, unsigned long options_, const char *filter, char *errmsg);
char *user_file_list_search (Window parent, int x, int y, const char *base_name);
char *user_file_list_complete (Window parent, int x, int y, int lines, int columns, const char *base_name);
void get_file_time (char *timestr, time_t file_time, int l);

CWidget *CDrawFilelist (const char *identifier, Window parent, int x, int y,
			int width, int height, int line, int column,
			struct file_entry *directentry,
			long options);
CWidget *CRedrawFilelist (const char *identifier, struct file_entry *directentry, int preserve);
struct file_entry *CGetFilelistLine (CWidget *w, int line);
CWidget *CDrawTextbox (const char *identifier, Window parent, int x, int y, int width, int height, int line,
		       int column, const char *text, long options);

/* Draws a scrollable textbox, with its scrollbar. text is newline seperate */
CWidget *CDrawTextboxManaged (const char *identifier, Window parent, int x, int y,
		       int width, int height, int line, int column, 
                       CStr (*get_cb) (void *, void *),
                       void (*free_cb) (void *, void *),
                       void *hook1,
                       void *hook2,
                       int options);


#define TEXTBOX_BDR 8
char *CGetTextBoxLine (CWidget * w, int line);
CStr CGetTextBoxText (CWidget * w);
CWidget *CDrawManPage (const char *identifier, Window parent, int x, int y,
	  int width, int height, int line, int column, const char *text);
/* Change the text of the textbox. If preserve is 1, then the position in the text is not altered */
CWidget *CRedrawTextbox (const char *identifier, const char *text, int preserve);
CWidget *CRedrawTextboxManaged (const char *identifier, CStr (*get_cb) (void *, void *),
				void (*free_cb) (void *, void *), void *hook1, void *hook2, int preserve);
CWidget *CRedrawFieldedTextbox (const char *identifier, int preserve);
CWidget *CClearTextbox (const char *identifier);
/* Set the position of the text in the text-box, see coolwidget.c */
int CSetTextboxPos (CWidget * wdt, int which, long p);

/* Draws a thin horizontal raised ridge */
CWidget *CDrawBar (Window parent, int x, int y, int w);

/* Vertical scroll bar */
CWidget *CDrawVerticalScrollbar (const char *identifier, Window parent, int x, int y,
			       int length, int width, int pos, int prop);
CWidget *CDrawHorizontalScrollbar (const char *identifier, Window parent, int x, int y,
			       int length, int width, int pos, int prop);
void CSetScrollbarCallback (const char *scrollbar, const char *wiget,
			    void (*linktowiget) (CWidget *,
				    CWidget *, XEvent *, CEvent *, int));
/* eg: void link_scrollbar_to_textbox (CWidget * w, CWidget * textbox, 
   XEvent * xevent, CEvent * cwevent, int which_scrollbar_button_was_pressed_1_to_5); */


/* Draws one or more lines of text (separate by newlines) in a sunken panel. Use like printf() */
CWidget *CDrawText (const char *identifier, Window parent, int x, int y, const char *fmt,...);
CWidget *CDrawTextFixed (const char *identifier, Window parent, int x, int y, const char *fmt,...);
/* Will replace the text of an existing text widget. Unlike other widgets, multiple text widgets can have the same ident */
CWidget *CRedrawText (const char *identifier, const char *fmt,...);
void CTextSize (int *w, int *h, const char *str);

/* Draws a file browser and returns a filename, file is the default file name */
char *CGetFile (Window parent, int x, int y,
		const char *dir, const char *file, const char *label, char *host);
char *CGetDirectory (Window parent, int x, int y,
		   const char *dir, const char *file, const char *label, char *host);
char *CGetSaveFile (Window parent, int x, int y,
		    const char *dir, const char *file, const char *label, char *host);
char *CGetLoadFile (Window parent, int x, int y,
		    const char *dir, const char *file, const char *label, char *host);

/* Draws a directory browser and returns immediately */
void CDrawBrowser (const char *ident, Window parent, int x, int y,
		   const char *dir, const char *file, const char *label, char *host);


/* Draws a simple spreadsheat widget (not supprted) */
CWidget *CDrawSpreadSheet (const char *ident, Window parent, int x, int y, int w, int h, const char *spreadtext, const char *heading, int *columns);

/* Draws a full blown text editor, scrollbar and status line */
CWidget *CDrawEditor (const char *identifier, Window parent, int x, int y,
	   int width, int height, const char *text, const char *filename,
		const char *starting_host, const char *starting_directory, unsigned int options, unsigned long text_size);

void CSetCursorColor (unsigned long c);

/* Draws a menu button that may be pulled down if clicked on */
CWidget *CDrawMenuButton (const char *ident, Window parent, Window focus_return,
   int x, int y, int width, int height, int num_items, const char *label,...);
/* this is a menu item: */
/* ...label, const char *text, int hot_key, callfn call_back, unsigned long data,...); */
void CSetMenuFocusReturn (const char *ident, Window w);
void CSetLastMenu (CWidget * button);
CWidget *CGetLastMenu (void);
void CMenuSelectionDialog (CWidget * button);
CWidget *get_pulled_menu (void);


/* Draws menu buttons for the editor. focus_return is where focus goes to if you escape from a menu */
void CDrawEditMenuButtons (const char *ident, Window parent, Window focus_return, int x, int y);
void CAddMenuItem (const char *ident, const char *text, int hot_key, callfn call_back, unsigned long data);
void CRemoveMenuItem (const char *ident, const char *text);
void CReplaceMenuItem (const char *ident, const char *old_text, const char *new_text, int hot_key, callfn call_back, unsigned long data);
void CRemoveMenuItemNumber (const char *ident, int i);
void CInsertMenuItem (const char *ident, const char *after, const char *text, int hot_key, callfn call_back, unsigned long data);
void CInsertMenuItemAfter (const char *ident, const char *after, const char *text, int hot_key, callfn call_back, unsigned long data);
int CHasMenuItem (const char *ident, const char *text); /* returns -1 or menu item number */

/* Draws a bitmap inside a sunken window */
CWidget *CDrawBitmap (const char *identifier, Window parent, int x, int y,
		      int width, int height, unsigned long fg, unsigned long bg, const unsigned char *data);

/* Draws a black and white picture 1 byte per pixel contiguous data */
CWidget *CDrawBWImage (const char *identifier, Window parent, int x, int y,
		       int width, int height, unsigned char *data);

XImage *CCreateImage (const char *data[], int width, int height, char start_char);
Pixmap CCreatePixmap (const char *data[], int width, int height, char start_char);
Pixmap CCreateClipMask (const char *data[], int width, int height, char start_char);


/* A window with inward bevels */
CWidget *CDrawSunkenPanel (const char *identifier, Window parent, int x, int y,
			   int width, int height, const char *label);

/* Draw a progress bar */
CWidget *CDrawProgress (const char *identifier, Window parent, int x, int y,
			int width, int height, int p);

/* Draws a picture, containing nothing. Allows lines, rectangles etc to
   be drawn into the picture */
CWidget *CDrawPicture (const char *identifier, Window parent, int x, int y,
		       int max_num_elements);


/* Destroy a widget. This will destroy all descendent widgets recursively */
int CDestroyWidget (const char *identifier);

/* Used internally, or for creating you own widgets, see coolwidget.c */
CWidget *CSetupWidget (const char *identifier, Window parent, int x, int y,
	    int width, int height, int kindofwidget, unsigned long input,
		       unsigned long bgcolor, int takes_focus);
/* call before drawing a widget to the root window to stop the window manager
    of creating a border for you, */
void CSetOverrideRedirect (void);
/* then clear after drawing the widget */
void CClearOverrideRedirect (void);


/* For resizing and reposition a widget */
void CSetWidgetSize (const char *ident, int w, int h);
void CSetSize (CWidget * wt, int w, int h);
void CSetWidgetPosition (const char *ident, int x, int y);

/* Forces the a widget to be entirely redrawn */
void CExpose (const char *ident);

/* Sends an expose event to a window */
void CExposeWindowArea (Window win, int count, int x, int y, int w, int h);

/* Sends an event to the coolwidget queue. Use instead of XSendEvent */
int CSendEvent (XEvent * e);


/* add a callback to a widget. Will be called if anything relevent happens
   to the widget. callback must return 1 if they handles a key press */
void CAddCallback (const char *ident, int (*callback) (CWidget *, XEvent *, CEvent *));
void CAddBeforeCallback (const char *ident, int (*callback) (CWidget *, XEvent *, CEvent *));

/* send the text box a command (such as XK_Left or XK_Down to scroll) */
int CTextboxCursorMove (CWidget * w, KeySym key);

/* forces all windows set to CALWAYS_NO_TOP (see above) to be raised */
void CRaiseWindows (void);
/* same for ALWAYS_ON_BOTTOM, call these after raising or lowering
   a window, to keep the "underneath" windows where they should be
   (eg the coolwidget logo in the top left of the screen */
void CLowerWindows (void);

/*
   The hinge of this whole library. This handles widget events and calls
   the callback routines. It must go in the main loop of a program.
   Returns cwevent->handled == 1 if the event was a key event and was
   handled, 0 otherwise.
 */
void CNextEvent (XEvent * xevent, CEvent * cwevent);

int CExposePending (Window w, XEvent * ev);

/* Any events left? */
int CPending (void);

/* whats next. check the local queue only, returns 0 on nothing pending */
int CPeekEventType (void);

/* key presses waiting ? */
int CKeyPending (void);

/* Any events left on coolwidgets own event queue? */
int CQueueSize (void);

/* Do not use the libc sleep command. This sleeps for t seconds,
   resolution is 1/50 of a second */
void CSleep (double t);

/* Do not use the libc system command */
int CSystem (const char *string);

/* Destroy all widgets */
void CDestroyAll (void);

/* All widgets may be either enabled or disabled, meaning they either
   recieve input from the mouse and keyboard, or not. This backs up the
   state of all widgets into the structure CState. See dialog.c for an
   example */
void CBackupState (CState * s);
/* This restore the state from the structure */
void CRestoreState (CState * s);

/* Disable a widget. ident may be a regular expression. */
void CDisable (const char *ident);
/* Enable */
void CEnable (const char *ident);

/* set the focus to a widget */
void CFocus (CWidget * w);

/* set the focus, but not if the window manager is focussed elsewhere,
and option_never_raise_wm_windows is set */
int CTryFocus (CWidget * w, int raise_wm_window);

/* set the focus to a window */
void CFocusWindow (Window win);

/* get the current focus */
Window CGetFocus (void);

/* get the current window of the input method */
Window CGetICFocus(void);

/* pull up or down a menu */
void CPullDown (CWidget * button);
void CPullUp (CWidget * button);

/* set the editor that editmenu will send commands to */
void CSetEditMenu (const char *ident);
void CEditMenuCommand (unsigned long i);
CWidget *CGetEditMenu (void);
void CEditMenuKey (KeySym i, int state);

/* internal */
int CSendExpose (Window win, int x, int y, int w, int h);

/* cursor blink rate */
int CGetCursorBlinkRate (void);
void CSetCursorBlinkRate (int times_per_second);


/************* the rest is not properly documented **************/

/* check magic number to detect a memory leak */
int widget_check_magic (void);

/* convert button press to double click */
void resolve_button (XEvent * xevent, CEvent * cwevent);


#ifdef DRAWINGS_C
CWidget *CDrawTarget;
#else
extern CWidget *CDrawTarget;
#endif

/* Focus ordering */
CWidget *CFindFirstDescendent (Window win);
int find_next_child_of (Window win, Window child);
int find_previous_child_of (Window win, Window child);
int find_first_child_of (Window win);
int find_last_child_of (Window win);

/* returns the widgets index */
int widget_of_window (Window win);

/* returns the main window that a widget is a descendent of */
Window CFindParentMainWindow (Window win);

/* there are two cursor types */
#define CURSOR_TYPE_TEXTINPUT 1
#define CURSOR_TYPE_EDITOR 2

/* set the cursor position (internal) */
void set_cursor_position (Window win, int x, int y, int w, int h, int type, C_wchar_t chr, unsigned long fg, unsigned long bg, int style);

/* translates a key press to a keysym */
KeySym CKeySym (XEvent * e);
/* some by converts to a short with upper bits representing the state */
short CKeySymMod (XEvent * e);
/* get the current state of the keyboard and mouse */
unsigned int CGetKeyBoardState (void);

/* match regular expressions */
int regexp_match (char *pattern, char *string, int match_type);

/* gets a widgets position relative to some ancestor widget */
void CGetWindowPosition (Window win, Window ancestor, int *x_return, int *y_return);

CWidget *CNextFocus (CWidget * w);
CWidget *CPreviousFocus (CWidget * w);
CWidget *CChildFocus (CWidget * w);
int CHandleGlobalKeys (CWidget *w, XEvent * xevent, CEvent * cwevent);

struct hint_pos;
struct hint_pos *CPushHintPos (void);
void CPopHintPos (struct hint_pos *r);

void reset_hint_pos (int x, int y);
void set_hint_pos (int x, int y);
void CGetHintPos (int *x, int *y);
void get_hint_limits (int *max_x, int *max_y);
void CSetSizeHintPos (const char *ident);

/* disable alarm sets and unsets the signal handler: might
   be necesary with some system calls */
void CEnableAlarm (void);
void CDisableAlarm (void);

/* see pipe.c for what these do. triple_pipe_open is very useful for piping processes. */
/* set your own SIGCHLD handler though */
void set_signal_handlers_to_default (void);
char *read_pipe (int fd, int *len, const pid_t *child_pid);
pid_t triple_pipe_open (int *in, int *out, int *err, int mix, const char *file, char *const argv[]);
pid_t triple_pipe_open_env (int *in, int *out, int *err, int mix, const char *file, char *const argv[], char *const envp[]);
pid_t open_under_pty (int *in, int *out, char *line, const char *file, char *const argv[], char *errmsg);

/* see coolnext.c for details */
int CCheckWindowEvent (Window w, long event_mask, int do_sync);
long CUserInputEventToMask (int event_type);
int CCheckSimilarEventsPending (Window w, int event_type, int do_sync);

double my_log (double x);
double my_sqrt (double x);
double my_pow (double x, double y);


#ifdef DEBUG
#define CMalloc(x) CDebugMalloc(x, __LINE__, __FILE__)
#endif

/* #define FOCUS_DEBUG */

#ifdef FOCUS_DEBUG
#define CFocus(x) CFocusDebug(x,__LINE__,__FILE__)
#define CFocusWindow(x) CFocusWindowDebug(x,__LINE__,__FILE__)
#else
#define CFocus(x) CFocusNormal(x)
#define CFocusWindow(x) CFocusWindowNormal(x)
#endif


void CFocusDebug (CWidget * w, int line, char *file);
void CFocusNormal (CWidget * w);
void CFocusWindowDebug (Window w, int line, char *file);
void CFocusWindowNormal (Window w);
void CFocusLast (void);

/* get last decendent focussed with the specified main window */
Window *CGetLastFocussedInMain (Window main);

/* resizing stuff */
void CCentre (char *ident);
void CSetMovement (const char *ident, unsigned long position);

/* send an event to a window with a direct call */
int CSendMessage (CWidget * w, int msg);

/* set and unset the hourglass for this window */
void CHourGlass (Window win);
void CUnHourGlass (Window win);

void get_home_dir (void);


/* drag and drop stuff */

/* do a drag */
void CDrag (Window from, int data_type, unsigned char *data, int length, unsigned long pointer_state);

/* get drop text, type is returned */
int CGetDrop (XEvent * xe, unsigned char **data, unsigned long *size, int *x, int *y);

/* send a drop ack event */
void CDropAcknowledge (XEvent * xe);

/* check if event is a drop ack event */
int CIsDropAcknowledge (XEvent * xe, unsigned int *state);

/* find the pointer x and y position within the window it is over */
Window CQueryPointer (int *x, int *y, unsigned int *mask);

/* set and get the directory to be prependent onto drags */
#ifdef HAVE_DND
char *CDndDirectory (void);
char *CDndFileList (char *t, int *l, int *num_files);
#endif
void CSetDndDirectory (char *d);

#define CURSOR_HOUR 0
#define CURSOR_LEFT 1
#define CURSOR_MENU 2
Cursor CGetCursorID (int i);

void XDrawVericalString8x16 (Display * display, Drawable d, GC gc,
			 int x, int y, char *string, int length);

#define DndNotDnd	-1
#define DndUnknown	0
#define DndRawData	1
#define DndFile		2
#define DndFiles	3
#define DndText		4
#define DndDir		5
#define DndLink		6
#define DndExe		7
#define DndURL		8
#define DndMIME         9

#define DndEND		10

#define dnd_null_term_type(d) \
	((d) == DndFile || (d) == DndText || (d) == DndDir || \
	(d) == DndLink || (d) == DndExe || (d) == DndURL)

char *CDndFileList (char *t, int *l, int *num_files);

/* dialogs */

void CFatalErrorDialog (int x, int y, const char *fmt,...);

#define INPUT_DIALOG_BROWSE_MASK	(3<<14)
#define INPUT_DIALOG_BROWSE_NONE	(0<<14)
#define INPUT_DIALOG_BROWSE_LOAD	(1<<14)
#define INPUT_DIALOG_BROWSE_SAVE	(2<<14)
#define INPUT_DIALOG_BROWSE_DIR		(3<<14)
char *CInputDialog (const char *ident, Window in, int x, int y, int min_width, const char *def, const char *heading, const char *fmt,...);

#define CQUERYDIALOG_ADD_TEXTBOX        ((void *) (long) 1)
int CQueryDialog (Window in, int x, int y, const char *heading, const char *descr, const char *first,...);
void CErrorDialog (Window in, int x, int y, const char *heading, const char *fmt,...);
void CErrorDialogTxt (Window in, int x, int y, const char *heading, const char *fmt,...);
void CMessageDialog (Window in, int x, int y, unsigned long options, const char *heading, const char *fmt,...);
void CTextboxMessageDialog (Window in, int x, int y, int width, int height, const char *heading, const char *text, int line);
void CFieldedTextboxMessageDialog (Window in, int x, int y, int columns, int lines, const char *heading,
                            char **(*get_line) (void *, int, int *, int *), long options, void *data);
int CListboxDialog (Window in, int x, int y, int columns, int lines,
     const char *heading, int start_line, int cursor_line, int num_lines,
		    char *(*get_line) (void *data, int line), void *data);
long CUnicodeDialog (Window in, int x, int y, char *heading);
char *get_sys_error (const char *s);
XEvent *CRawkeyQuery (Window in, int x, int y, const char *heading, const char *fmt,...);
char *CTrivialSelectionDialog (Window in, int x, int y, int columns, int lines, const char *text, int line, int cursor_line);
KeySym CKeycodeToKeysym (KeyCode keycode);

struct focus_win {
    Window top, bottom, left, right;
    Window current;
    int width, height;
    int border;
};

int inbounds (int x, int y, int x1, int y1, int x2, int y2);

struct look {
    int (*get_default_interwidget_spacing) (void);
    void (*menu_draw) (Window win, int w, int h, struct menu_item m[], int n, int light);
    void (*get_menu_item_extents) (int n, int j, struct menu_item m[], int *border, int *relief,
				   int *y1, int *y2);
    void (*render_menu_button) (CWidget * wdt);
    void (*render_button) (CWidget * wdt);
    void (*render_bar) (CWidget * wdt);
    void (*render_raised_bevel) (Window win, int x1, int y1, int x2, int y2, int thick, int sunken);
    void (*render_sunken_bevel) (Window win, int x1, int y1, int x2, int y2, int thick, int sunken);
    void (*draw_hotkey_understroke) (Window win, int x, int y, int hotkey);
    const char *(*get_default_widget_font) (void);
    void (*render_text) (CWidget * wdt);
    void (*render_window) (CWidget * wdt);
    void (*render_scrollbar) (CWidget * wdt);
    int (*get_scrollbar_size) (int type);
    void (*init_scrollbar_icons) (CWidget * wdt);
    int (*which_scrollbar_button) (int bx, int by, CWidget * wdt);
    int (*scrollbar_handler) (CWidget * w, XEvent * xevent, CEvent * cwevent);
    void (*get_button_color) (XColor * color, int i);
    int (*get_extra_window_spacing) (void);
    int (*window_handler) (CWidget * w, XEvent * xevent, CEvent * cwevent);
    int (*get_focus_ring_size) (void);
    unsigned long (*get_button_flat_color) (void);
    int (*get_window_resize_bar_thickness) (void);
    void (*render_switch) (CWidget * wdt);
    int (*get_switch_size) (void);
    void (*draw_browser) (const char *ident, Window parent, int x, int y, char *host, const char *dir,
			  const char *file, const char *label);
    char *(*get_file_or_dir) (Window parent, int x, int y, char *host, const char *dir, const char *file,
			      const char *label, int options);
    CWidget *(*draw_file_list) (const char *identifier, Window parent, int x, int y,
				int width, int height, int line, int column,
				struct file_entry * directentry, long options);
    CWidget *(*redraw_file_list) (const char *identifier, struct file_entry * directentry,
				  int preserve);
    struct file_entry *(*get_file_list_line) (CWidget * w, int line);
    int (*search_replace_dialog) (Window parent, int x, int y, CStr *search_text,
				   CStr *replace_text, CStr *arg_order, const char *heading,
				   int option);
    void (*edit_render_tidbits) (CWidget * wdt);
    CWidget *(*draw_exclam_cancel_button) (char *ident, Window win, int x, int y);
    CWidget *(*draw_tick_cancel_button) (char *ident, Window win, int x, int y);
    CWidget *(*draw_cross_cancel_button) (char *ident, Window win, int x, int y);
    CWidget *(*draw_tick_ok_button) (char *ident, Window win, int x, int y);
    void (*render_fielded_textbox_tidbits) (CWidget * w, int isfocussed);
    void (*render_textbox_tidbits) (CWidget * w, int isfocussed);
    int (*get_fielded_textbox_hscrollbar_width) (void);
    void (*render_textinput_tidbits) (CWidget * wdt, int isfocussed);
    void (*render_passwordinput_tidbits) (CWidget * wdt, int isfocussed);
    void (*render_focus_border) (Window win);
};

#include "edit.h"
#include "editcmddef.h"
#include "imagewidget.h"

#endif


