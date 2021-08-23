/* look-next.c
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "coolwidget.h"

#include "stringtools.h"
#include "app_glob.c"
#include "coollocal.h"


#ifdef NEXT_LOOK

#error Next look is no longer supported -- if you want it to work, please send me a patch

extern struct look *look;

/* {{{ search replace dialog */

extern int replace_scanf;
extern int replace_regexp;
extern int replace_all;
extern int replace_prompt;
extern int replace_whole;
extern int replace_case;
extern int replace_backwards;
extern int search_create_bookmark;

static void look_next_search_replace_dialog (Window parent, int x, int y, char **search_text, char **replace_text, char **arg_order, const char *heading, int option)
{
    Window win;
    XEvent xev;
    CEvent cev;
    CState s;
    int xh, yh, h, xb, ys, yc, yb, yr;
    CWidget *m;
    int text_input_width ;

    CBackupState (&s);
    CDisable ("*");

    win = CDrawHeadedDialog ("replace", parent, x, y, heading);
    CGetHintPos (&xh, &h);
    xh += WINDOW_EXTRA_SPACING ;

/* NLS hotkey ? */
    CIdent ("replace")->position = WINDOW_ALWAYS_RAISED;
/* An input line comes after the ':' */
    (CDrawText ("replace.t1", win, xh, h, _(" Enter search text : ")))->hotkey = 'E';

    CGetHintPos (0, &yh);
    (m = CDrawTextInput ("replace.sinp", win, xh, yh, 10, AUTO_HEIGHT, 8192, *search_text))->hotkey = 'E';

    if (replace_text) {
	CGetHintPos (0, &yh);
	(CDrawText ("replace.t2", win, xh, yh, _(" Enter replace text : ")))->hotkey = 'n';
	CGetHintPos (0, &yh);
	(CDrawTextInput ("replace.rinp", win, xh, yh, 10, AUTO_HEIGHT, 8192, *replace_text))->hotkey = 'n';
	CSetToolHint ("replace.t2", _("You can enter regexp substrings with %s\n(not \\1, \\2 like sed) then use \"Enter...order\""));
	CSetToolHint ("replace.rinp", _("You can enter regexp substrings with %s\n(not \\1, \\2 like sed) then use \"Enter...order\""));
	CGetHintPos (0, &yh);
	(CDrawText ("replace.t3", win, xh, yh, _(" Enter argument (or substring) order : ")))->hotkey = 'o';
	CGetHintPos (0, &yh);
	(CDrawTextInput ("replace.ainp", win, xh, yh, 10, AUTO_HEIGHT, 256, *arg_order))->hotkey = 'o';
/* Tool hint */
	CSetToolHint ("replace.ainp", _("Enter the order of replacement of your scanf\nformat specifiers or regexp substrings, eg 3,1,2"));
	CSetToolHint ("replace.t3", _("Enter the order of replacement of your scanf\nformat specifiers or regexp substrings, eg 3,1,2"));
    }
    CGetHintPos (0, &yh);
    ys = yh;
/* The following are check boxes */
    CDrawSwitch ("replace.ww", win, xh, yh, replace_whole, _(" Whole words only "), 0);
    CGetHintPos (0, &yh);
    CDrawSwitch ("replace.case", win, xh, yh, replace_case, _(" Case sensitive "), 0);
    yc = yh;
    CGetHintPos (0, &yh);
    CDrawSwitch ("replace.reg", win, xh, yh, replace_regexp, _(" Regular expression "), 1);
    CSetToolHint ("replace.reg", _("See the regex man page for how\nto compose a regular expression"));
    CSetToolHint ("replace.reg.label", _("See the regex man page for how\nto compose a regular expression"));
    yb = yh;
    CGetHintPos (0, &yh);
    CGetHintPos (&xb, 0);
    xb += WINDOW_EXTRA_SPACING ;
    if (option & SEARCH_DIALOG_OPTION_BACKWARDS) {
	CDrawSwitch ("replace.bkwd", win, xh, yh, replace_backwards, _(" Backwards "), 0);
/* Tool hint */
	CSetToolHint ("replace.bkwd", _("Warning: Searching backward can be slow"));
	CSetToolHint ("replace.bkwd.label", _("Warning: Searching backward can be slow"));
    }
    if (replace_text) {
	yr = ys;
	if (option & SEARCH_DIALOG_OPTION_BACKWARDS)
	    yr = yc;
    } else {
	if (option & SEARCH_DIALOG_OPTION_BACKWARDS) {
	    if (option & SEARCH_DIALOG_OPTION_BOOKMARK)
		yr = yb;
	    else
		yr = yh;
	} else {
	    if (option & SEARCH_DIALOG_OPTION_BOOKMARK)
		yr = yc;
	    else
		yr = yb;
	}
    }

    if (replace_text) {
	CDrawSwitch ("replace.pr", win, xb, yr, replace_prompt, _(" Prompt on replace "), 0);
/* Tool hint */
	CSetToolHint ("replace.pr", _("Ask before making each replacement"));
	CGetHintPos (0, &yr);
	CDrawSwitch ("replace.all", win, xb, yr, replace_all, _(" Replace all "), 0);
/* Tool hint */
	CSetToolHint ("replace.all", _("Replace repeatedly"));
	CGetHintPos (0, &yr);
    }
    if (option & SEARCH_DIALOG_OPTION_BOOKMARK) {
	CDrawSwitch ("replace.bkmk", win, xb, yr, search_create_bookmark, _(" Bookmarks "), 0);
/* Tool hint */
	CSetToolHint ("replace.bkmk", _("Create bookmarks at all lines found"));
	CSetToolHint ("replace.bkmk.label", _("Create bookmarks at all lines found"));
	CGetHintPos (0, &yr);
    }
    CDrawSwitch ("replace.scanf", win, xb, yr, replace_scanf, _(" Scanf expression "), 1);
/* Tool hint */
    CSetToolHint ("replace.scanf", _("Allows entering of a C format string,\nsee the scanf man page"));

    get_hint_limits (&x, &y);
    {
      int btn_width, x, y ;
	CGetHintPos (&x, &y);
	
	y += WINDOW_EXTRA_SPACING * 2 ;
	x += WINDOW_EXTRA_SPACING * 2 ;
	CTextSize (&btn_width, 0, " Cancel ");
	btn_width += 4 + BUTTON_RELIEF * 2;
        x -= (btn_width + WINDOW_EXTRA_SPACING) * 2 + WINDOW_EXTRA_SPACING;

	CDrawButton ("replace.ok", win, x+btn_width + WINDOW_EXTRA_SPACING * 2, y, AUTO_WIDTH, AUTO_HEIGHT, "   Ok   ");
	CDrawButton ("replace.cancel", win, x, y, AUTO_WIDTH, AUTO_HEIGHT, " Cancel ");
	CGetHintPos (0, &y);
	x += (btn_width + WINDOW_EXTRA_SPACING) * 2 + WINDOW_EXTRA_SPACING;
	reset_hint_pos (x, y + WINDOW_EXTRA_SPACING*2);
    }
/* Tool hint */
    CSetToolHint ("replace.ok", _("Begin search, Enter"));
    CSetToolHint ("replace.cancel", _("Abort this dialog, Esc"));
    CSetSizeHintPos ("replace");
    CMapDialog ("replace");

    m = CIdent ("replace");
    text_input_width = m->width - WIDGET_SPACING * 3 - 4 - WINDOW_EXTRA_SPACING*2 ;
    CSetWidgetSize ("replace.sinp", text_input_width, (CIdent ("replace.sinp"))->height);
    if (replace_text) {
	CSetWidgetSize ("replace.rinp", text_input_width, (CIdent ("replace.rinp"))->height);
	CSetWidgetSize ("replace.ainp", text_input_width, (CIdent ("replace.ainp"))->height);
    }
    CFocus (CIdent ("replace.sinp"));

    for (;;) {
	CNextEvent (&xev, &cev);
	if (!CIdent ("replace")) {
	    *search_text = 0;
	    break;
	}
	if (!strcmp (cev.ident, "replace.cancel") || cev.command == CK_Cancel) {
	    *search_text = 0;
	    break;
	}
	if (!strcmp (cev.ident, "replace.reg") || !strcmp (cev.ident, "replace.scanf")) {
	    if (CIdent ("replace.reg")->keypressed || CIdent ("replace.scanf")->keypressed) {
		if (!(CIdent ("replace.case")->keypressed)) {
		    CIdent ("replace.case")->keypressed = 1;
		    CExpose ("replace.case");
		}
	    }
	}
	if (!strcmp (cev.ident, "replace.ok") || cev.command == CK_Enter) {
	    if (replace_text) {
		replace_all = CIdent ("replace.all")->keypressed;
		replace_prompt = CIdent ("replace.pr")->keypressed;
		*replace_text = (char *) strdup (CIdent ("replace.rinp")->text);
		*arg_order = (char *) strdup (CIdent ("replace.ainp")->text);
	    }
	    *search_text = (char *) strdup (CIdent ("replace.sinp")->text);
	    replace_whole = CIdent ("replace.ww")->keypressed;
	    replace_case = CIdent ("replace.case")->keypressed;
	    replace_scanf = CIdent ("replace.scanf")->keypressed;
	    replace_regexp = CIdent ("replace.reg")->keypressed;

	    if (option & SEARCH_DIALOG_OPTION_BACKWARDS) {
		replace_backwards = CIdent ("replace.bkwd")->keypressed;
	    } else {
		replace_backwards = 0;
	    }

	    if (option & SEARCH_DIALOG_OPTION_BOOKMARK) {
		search_create_bookmark = CIdent ("replace.bkmk")->keypressed;
	    } else {
		search_create_bookmark = 0;
	    }

	    break;
	}
    }
    CDestroyWidget ("replace");
    CRestoreState (&s);
}

/* }}} search replace dialog */


/* {{{ file list stuff */


#ifdef HAVE_STRFTIME
/* We want our own dates for NLS */
#undef HAVE_STRFTIME
#endif

#undef gettext_noop
#define gettext_noop(x) x

void get_file_time (char *timestr, time_t file_time, int l);

#if 0
static char **get_filelist_line (void *data, int line_number, int *num_fields, int *tagged)
{
    struct file_entry *directentry;
    static char *fields[10], size[24], mode[12], timestr[32];
    static char name[520], *n;
    mode_t m;

    *num_fields = 4;		/* name, size, date, mode only (for the mean time) */

    directentry = (struct file_entry *) data;
    if (directentry[line_number].options & FILELIST_LAST_ENTRY)
	return 0;

    n = name;
    strcpy (name, directentry[line_number].name);
    fields[0] = name;
    sprintf (size, "\t%u", (unsigned int) directentry[line_number].stat.st_size);
    fields[1] = size;

    get_file_time (timestr, directentry[line_number].stat.st_mtime, 0);
    fields[2] = timestr;

    memset (mode, ' ', 11);
    mode[11] = 0;
    mode[0] = '-';
    m = directentry[line_number].stat.st_mode;
    switch ((int) m & S_IFMT) {
    case S_IFLNK:
	mode[0] = 'l';
	break;
    case S_IFDIR:
	mode[0] = 'd';
	break;
    case S_IFCHR:
	mode[0] = 'c';
	break;
    case S_IFBLK:
	mode[0] = 'b';
	break;
    case S_IFIFO:
	mode[0] = 'f';
	break;
    case S_IFSOCK:
	mode[0] = 's';
	break;
    }

    mode[1] = m & S_IRUSR ? 'r' : '-';
    mode[2] = m & S_IWUSR ? 'w' : '-';
    mode[3] = m & S_IXUSR ? 'x' : '-';

    mode[4] = m & S_IRGRP ? 'r' : '-';
    mode[5] = m & S_IWGRP ? 'w' : '-';
    mode[6] = m & S_IXGRP ? 'x' : '-';

    mode[7] = m & S_IROTH ? 'r' : '-';
    mode[8] = m & S_IWOTH ? 'w' : '-';
    mode[9] = m & S_IXOTH ? 'x' : '-';

    if (S_ISLNK (m)) {
	int l, i;
	char *p;
	p = directentry[line_number].name;
	l = strlen (n);
	for (i = 0; i < l; i++) {
	    *n++ = '\b';
	    *n++ = *p++;
	}
	*n++ = '\0';
    } else if (m & (S_IXUSR | S_IXGRP | S_IXOTH)) {
	int l, i;
	char *p;
	p = directentry[line_number].name;
	l = strlen (n);
	for (i = 0; i < l; i++) {
	    *n++ = '\r';
	    *n++ = *p++;
	}
	*n++ = '\0';
    }
    fields[3] = mode;
    fields[*num_fields] = 0;
    if (directentry[line_number].options & FILELIST_TAGGED_ENTRY)
	*tagged = 1;
    return fields;
}
#endif

static char *get_filelist_line_short (void *data, int line_number, char buffer[512])
{
    struct file_entry *directentry;
    static char ctimestr[32], mtimestr[32];
    mode_t m;
    char *ptr;
    unsigned long size_bytes;

    directentry = (struct file_entry *) data;
    if (directentry[line_number].options & FILELIST_LAST_ENTRY)
	return 0;

    buffer[0] = 6;
    strncpy (&(buffer[1]), directentry[line_number].name, 256);
    ptr = &(buffer[strlen (buffer)]);

    sprintf (ptr, "%c\nMode: %c", 27, 9);
    ptr += strlen (ptr);

    m = directentry[line_number].stat.st_mode;
    switch ((int) m & S_IFMT) {
    case S_IFLNK:
	*ptr = 'l';
	break;
    case S_IFDIR:
	*ptr = 'd';
	break;
    case S_IFCHR:
	*ptr = 'c';
	break;
    case S_IFBLK:
	*ptr = 'b';
	break;
    case S_IFIFO:
	*ptr = 'f';
	break;
    case S_IFSOCK:
	*ptr = 's';
	break;
    default:
	*ptr = '-';
    }
    ptr++;
    *(ptr++) = m & S_IRUSR ? 'r' : '-';
    *(ptr++) = m & S_IWUSR ? 'w' : '-';
    *(ptr++) = m & S_IXUSR ? 'x' : '-';

    *(ptr++) = m & S_IRGRP ? 'r' : '-';
    *(ptr++) = m & S_IWGRP ? 'w' : '-';
    *(ptr++) = m & S_IXGRP ? 'x' : '-';

    *(ptr++) = m & S_IROTH ? 'r' : '-';
    *(ptr++) = m & S_IWOTH ? 'w' : '-';
    *(ptr++) = m & S_IXOTH ? 'x' : '-';


    size_bytes = directentry[line_number].stat.st_size;
    sprintf (ptr, "%c ; Size: [%c%9lu%c] bytes or [%c%6lu%c] KBytes", 27, 7, size_bytes, 27, 7, size_bytes >> 10, 27);
    ptr += strlen (ptr);

    get_file_time (ctimestr, directentry[line_number].stat.st_ctime, 0);
    get_file_time (mtimestr, directentry[line_number].stat.st_mtime, 0);

    sprintf (ptr, "\nCreated on: %c%s%c ; Modified on: %c%s%c", 1, ctimestr, 27, 1, mtimestr, 27);

    return buffer;
}

static Bool is_directory (struct file_entry * directentry, int line_number)
{
    if (directentry[line_number].options & FILELIST_LAST_ENTRY)
	return False;
    switch ((int) (directentry[line_number].stat.st_mode) & S_IFMT) {
    case S_IFLNK:
	/* fixme : add check if it is linked to dir or file */
	return True;
    case S_IFDIR:
	return True;
    }
    return False;
}

typedef struct FilelistCache {
    Pixmap cache;
    unsigned int width, height;
    unsigned int firstline, hilited;
    int current;
    unsigned int rowheight;
    unsigned long options;
} FilelistCache;

static void free_filelist_cache (void *vcache)
{
    if (vcache) {
	FilelistCache *cache = (FilelistCache *) vcache;
	if (cache->cache) {
	    XFreePixmap (CDisplay, cache->cache);
	    cache->cache = None;
	}
	free (vcache);
    }
}

#define NEXT_ARROW_SIZE  	8
#define NEXT_ARROW_FIELD 	(NEXT_ARROW_SIZE*2)

static void restrict_text_area (FilelistCache * cache)
{
    XRectangle clip_rec;
    clip_rec.x = 1;
    clip_rec.y = 1;
    clip_rec.width = cache->width - NEXT_ARROW_FIELD;
    clip_rec.height = cache->height - 2;
    XSetClipRectangles (CDisplay, CGC, 0, 0, &clip_rec, 1, YXSorted);
}

static void push_filelist_line (FilelistCache * cache, Bool pushed)
{
    int y1, y2;
    if (cache->current < 0)
	return;
    y1 = (cache->current - cache->firstline) * cache->rowheight;
    y2 = y1 + cache->rowheight;

    if (y1 < cache->height && y2 > 0) {
	if (pushed)
	    CSetColor (COLOR_BLACK);
	else
	    CSetColor ((cache->hilited == cache->current) ? color_widget (15) : COLOR_FLAT);

	CLine (cache->cache, 0, y2, 0, y1);
	CLine (cache->cache, 1, y1, cache->width - 2, y1);

	if (pushed)
	    CSetColor ((cache->hilited == cache->current) ? COLOR_FLAT : color_widget (15));

	CLine (cache->cache, 0, y2, cache->width - 2, y2);
	CLine (cache->cache, cache->width - 2, y2, cache->width - 2, y1);
    }
}

static void hilite_filelist_line (FilelistCache * cache, int color, struct file_entry *directentry)
{
    int y1, y2, x2;
    y1 = (cache->hilited - cache->firstline) * cache->rowheight;
    y2 = y1 + (cache->rowheight / 2 - 4);
    x2 = cache->width - NEXT_ARROW_FIELD + NEXT_ARROW_SIZE / 2;

    if (y1 >= 0 && y1 < cache->height - FONT_OVERHEAD) {
	CSetColor (color);
	CRectangle (cache->cache, 0, y1, cache->width - 1, cache->rowheight - 1);
	CSetColor (COLOR_BLACK);
	if (!(directentry[cache->hilited].options & FILELIST_LAST_ENTRY) &&
	    directentry[cache->hilited].name != NULL) {
	    restrict_text_area (cache);
	    CSetBackgroundColor (color);
	    CImageString (cache->cache, FONT_OFFSET_X + 1, FONT_OFFSET_Y + y1,
			  directentry[cache->hilited].name);
	    XSetClipMask (CDisplay, CGC, None);
	    if (is_directory (directentry, cache->hilited)) {
		CSetColor (COLOR_BLACK);
		CLine (cache->cache, x2, y2, x2 + 7, y2 + 4);
		CLine (cache->cache, x2, y2, x2, y2 + 8);
		CSetColor (COLOR_FLAT);
		CLine (cache->cache, x2, y2 + 8, x2 + 7, y2 + 4);
	    }
	}
    }
}

static void scroll_cache (FilelistCache * cache, unsigned int new_first, int *start, int *end)
{
    int good_start, good_end, scroll_to;

    if (cache->firstline > new_first) {		/* scrolling up */
	good_start = 0;
	scroll_to = (cache->firstline - new_first) * cache->rowheight;
	good_end = cache->height - scroll_to;
    } else {
	good_start = (new_first - cache->firstline) * cache->rowheight;
	good_end = cache->height;
	scroll_to = 0;
    }
    if (good_start < good_end) {
	XCopyArea (CDisplay, cache->cache, cache->cache, CGC, 0, good_start, cache->width, good_end, 0, scroll_to);
	if (cache->firstline > new_first) {	/* scrolling up */
	    *start = 0;
	    *end = scroll_to;
	} else {
	    *start = (cache->height - good_start);
	    *end = cache->height;
	}
    } else {
	*start = 0;
	*end = cache->height;
    }
    if (*start < *end) {
	CSetColor (COLOR_FLAT);
	CRectangle (cache->cache, 0, *start, cache->width, *end);
    }
    *start /= cache->rowheight;
    *end /= cache->rowheight;
}

static void check_filelist_cache (CWidget * wdt, unsigned int w, unsigned int h, FilelistCache * cache)
{
    struct file_entry *directentry = (struct file_entry *) wdt->hook;
    int y, x, start_y;
    int i, start = 0, end = 0;

    if (cache->cache) {
	if (cache->width == w && cache->height == h) {
	    if (cache->firstline == wdt->firstline &&
		cache->hilited == wdt->cursor &&
		cache->options == wdt->options)
		return;
	} else {
	    XFreePixmap (CDisplay, cache->cache);
	    cache->cache = None;
	}
    }
    if (cache->cache == None) {
	cache->cache = XCreatePixmap (CDisplay, wdt->winid, w, h, CDepth);
	cache->width = w;
	cache->height = h;
	CTextSize (NULL, (int *) &(cache->rowheight), "Ty~g$V|&^.,/_'I}[@");
	if (cache->rowheight == 0)
	    cache->rowheight = 1;
	cache->rowheight += FONT_OVERHEAD;
	CSetColor (COLOR_FLAT);
	CRectangle (cache->cache, 0, 0, w, h);
	end = h / cache->rowheight;
    } else {
	push_filelist_line (cache, False);
	if (cache->hilited != wdt->cursor)
	    hilite_filelist_line (cache, COLOR_FLAT, directentry);

	if (cache->firstline != wdt->firstline)
	    scroll_cache (cache, wdt->firstline, &start, &end);
    }

    cache->firstline = wdt->firstline;
    cache->hilited = wdt->cursor;
    hilite_filelist_line (cache, (wdt->options & BUTTON_HIGHLIGHT) ? COLOR_WHITE : color_widget (12), directentry);
    cache->current = wdt->current;
    push_filelist_line (cache, True);

    cache->options = wdt->options;

    start_y = start * cache->rowheight;
    y = start_y + FONT_OFFSET_Y;
    start += wdt->firstline;
    end += wdt->firstline;

    CSetColor (COLOR_BLACK);
    restrict_text_area (cache);
    for (i = start;
	 i <= end &&
	 i < wdt->numlines && y < cache->height &&
	 !(directentry[i].options & FILELIST_LAST_ENTRY); i++) {
	if (directentry[i].name && i != cache->hilited) {
	    CSetBackgroundColor (COLOR_FLAT);
	    CImageString (cache->cache, FONT_OFFSET_X + 1, y,
			 directentry[i].name);
	}

	y += cache->rowheight;
    }
    XSetClipMask (CDisplay, CGC, None);
    /* now drawing  arrows */
    y = start_y + (cache->rowheight / 2 - 4);
    x = cache->width - NEXT_ARROW_FIELD + NEXT_ARROW_SIZE / 2;
    for (i = start;
	 i <= end &&
	 i < wdt->numlines && y < cache->height &&
	 !(directentry[i].options & FILELIST_LAST_ENTRY); i++) {
	if (is_directory (directentry, i)) {
	    CSetColor (COLOR_BLACK);
	    CLine (cache->cache, x, y, x + 7, y + 4);
	    CLine (cache->cache, x, y, x, y + 8);
	    CSetColor ((i == cache->hilited) ? COLOR_FLAT : COLOR_WHITE);
	    CLine (cache->cache, x, y + 8, x + 7, y + 4);
	}
	y += cache->rowheight;
    }
}

static void render_NeXT_filelist (CWidget * wdt)
{
    FilelistCache *cache;

    if (wdt->hook == NULL) {
	render_bevel (wdt->winid, 0, 0, wdt->width - 1, wdt->height - 1, 1, 3);
	return;
    }
    if (wdt->user == NULL) {
	wdt->user = calloc (sizeof (FilelistCache), 1);
	wdt->free_user = free_filelist_cache;
    }
    cache = (FilelistCache *) (wdt->user);

    CPushFont ("widget", 0);
    check_filelist_cache (wdt, wdt->width - 3, wdt->height - 3, cache);
    CPopFont ();

    render_bevel (wdt->winid, 0, 0, wdt->width - 1, wdt->height - 1, 1, 1);

    XCopyArea (CDisplay, cache->cache, wdt->winid, CGC, 0, 0, cache->width, cache->height, 2, 2);
}

static void link_scrollbar_to_NeXT_filelist (CWidget * scrollbar, CWidget * w, XEvent * xevent, CEvent * cwevent, int whichscrbutton)
{
    /* fix me : add stuf */
    int redrawtext = 0;
    int new_first = w->firstline;
    FilelistCache *cache;
    int box_size;

    if (w->user == NULL)
	return;
    cache = (FilelistCache *) (w->user);
    box_size = cache->height / cache->rowheight;

    if ((xevent->type == ButtonRelease || xevent->type == MotionNotify) && whichscrbutton == 3) {
	new_first = (double) scrollbar->firstline * w->numlines / 65535.0;
    } else if (xevent->type == ButtonPress && (cwevent->button == Button1 || cwevent->button == Button2)) {
	new_first = w->firstline;
	switch (whichscrbutton) {
	case 1:
	    new_first -= box_size - 1;
	    break;
	case 2:
	    new_first--;
	    break;
	case 5:
	    new_first++;
	    break;
	case 4:
	    new_first += box_size - 1;
	    break;
	}
    }
    if (new_first < 0)
	new_first = 0;
    else if (new_first > w->numlines - box_size)
	new_first = w->numlines - box_size;
    redrawtext = (new_first != w->firstline);
    w->firstline = new_first;

    if (xevent->type == ButtonRelease ||
	(redrawtext && !CCheckWindowEvent (xevent->xany.window, ButtonReleaseMask | ButtonMotionMask, 0)))
	render_NeXT_filelist (w);

    scrollbar->firstline = (double) 65535.0 *w->firstline / (w->numlines ? w->numlines : 1);
    scrollbar->numlines = (double) 65535.0 *box_size / (w->numlines ? w->numlines : 1);

}

static int eh_NeXT_filelist (CWidget * w, XEvent * xevent, CEvent * cwevent);

static CWidget *look_next_draw_file_list (const char *identifier, Window parent, int x, int y,
			int width, int height, int line, int column,
			struct file_entry *directentry,
			long options)
{
    struct file_entry e;
    CWidget *w;
    int x_hint;
    int lines = 0;
    unsigned int prop;

    if (!directentry) {
	memset (&e, 0, sizeof (e));
	e.options = FILELIST_LAST_ENTRY;
	directentry = &e;
    } else {
	while (!(directentry[lines].options & FILELIST_LAST_ENTRY))
	    lines++;
    }

    if (height == AUTO_HEIGHT)
	height = max (1, lines) * FONT_PIX_PER_LINE + 6;

    if (width == AUTO_WIDTH)
	width = (FONT_MEAN_WIDTH * 24 + 15);

    w = CSetupWidget (identifier, parent, x, y,
	      width, height, C_TEXTBOX_WIDGET, INPUT_KEY, COLOR_FLAT, 1);

    x_hint = x + width + WIDGET_SPACING;
    prop = (double) 65535.0 *(height / FONT_PIX_PER_LINE) / (lines ? lines : 1);
    w->vert_scrollbar = CDrawVerticalScrollbar (catstrs (identifier, ".vsc", NULL), parent,
				 x_hint, y, height, AUTO_WIDTH, 0, prop);
    w->vert_scrollbar->position |= POSITION_HEIGHT;
    CSetScrollbarCallback (w->vert_scrollbar->ident, w->ident, link_scrollbar_to_NeXT_filelist);
    CGetHintPos (&x_hint, 0);

    set_hint_pos (x_hint, y + height + WIDGET_SPACING);
    w->eh = eh_NeXT_filelist;
    w->current = -1;
    w->cursor = -1;
    w->firstline = 0;
    w->numlines = lines;
    w->hook = directentry;
    return w;
}

CWidget *look_next_redraw_file_list (const char *identifier, struct file_entry * directentry, int preserve)
{
    CWidget *w = CIdent (identifier);

    if (w) {
	w->hook = directentry;
	if (!directentry)
	    w->numlines = 1;
	else {
	    register int i = 0;
	    while (!(directentry[i].options & FILELIST_LAST_ENTRY))
		i++;
	    w->numlines = i;
	}
	w->current = -1;
	w->cursor = -1;
	w->firstline = 0;

	free_filelist_cache (w->user);
	w->user = NULL;
	render_NeXT_filelist (w);
	if (w->user) {
	    FilelistCache *cache;
	    int box_size;
	    cache = (FilelistCache *) (w->user);
	    box_size = cache->height / cache->rowheight;
	    w->vert_scrollbar->firstline = (double) 65535.0 *w->firstline / (w->numlines ? w->numlines : 1);
	    w->vert_scrollbar->numlines = (double) 65535.0 *box_size / (w->numlines ? w->numlines : 1);
	    render_scrollbar (w->vert_scrollbar);
	}
    }
    return w;
}

void CSetFilelistPosition (const char *identifier, long current, long cursor, long firstline)
{
    CWidget *w = CIdent (identifier);

    if (w) {
	FilelistCache *cache;
	int redraw_scrollbar = 0;
	if (current >= w->numlines)
	    current = w->numlines - 1;
	if (cursor >= w->numlines)
	    cursor = w->numlines - 1;
	w->current = current;
	w->cursor = cursor;
	if (firstline >= 0 && (cache = (FilelistCache *) (w->user))) {
	    long max_fline = w->numlines - cache->height / cache->rowheight;
	    if (max_fline <= 0)
		if (firstline >= max_fline)
		    firstline = max_fline - 1;
	    /* if our window is larger then numlines : */
	    if (firstline < 0)
		firstline = 0;
	    redraw_scrollbar = (w->firstline != firstline);
	    w->firstline = firstline;
	}
	render_NeXT_filelist (w);
	if (redraw_scrollbar) {
	    w->vert_scrollbar->firstline = (double) 65535.0 *w->firstline / (w->numlines ? w->numlines : 1);
	    render_scrollbar (w->vert_scrollbar);
	}
    }
}

struct file_entry *look_next_get_file_list_line (CWidget * w, int line)
{
    struct file_entry *e;
    static struct file_entry r;

    memset (&r, 0, sizeof (r));
    e = (struct file_entry *) w->hook;
    if (e[line].options & FILELIST_LAST_ENTRY)
	r.options = FILELIST_LAST_ENTRY;
    else
	r = e[line];
    return &r;
}

int filelist_handle_mouse (CWidget * wdt, FilelistCache * cache, int y)
{
    int new_hilite;
    new_hilite = (int) (wdt->firstline) + y / (int) (cache->rowheight);
    if (y < 0)
	new_hilite--;
    /* we don't want to remove hilite even if user wants to go 
       beyound the beginning */
    if (new_hilite < 0)
	new_hilite = 0;

    return new_hilite;
}

int filelist_handle_keypress (CWidget * wdt, FilelistCache * cache, KeySym key)
{
    int new_cursor = wdt->cursor;

/* when text is highlighted, the cursor must be off */
    switch ((int) key) {
    case CK_Up:
	new_cursor--;
	break;
    case CK_Down:
	new_cursor++;
	break;
    case CK_Page_Up:
	new_cursor -= cache->height / cache->rowheight - 1;
	break;
    case CK_Page_Down:
	new_cursor += cache->height / cache->rowheight - 1;
	break;
    case CK_Home:
	new_cursor = 0;
	break;
    case CK_End:
	new_cursor = wdt->numlines;
	break;
    default:
	return -1;
    }
    /* we don't want to remove hilite even if user wants to go 
       beyound the beginning */
    if (new_cursor < 0)
	new_cursor = 0;
    return new_cursor;
}

int change_hilite (CWidget * w, int new_hilite)
{
    FilelistCache *cache = (FilelistCache *) (w->user);
    if (new_hilite >= w->numlines)
	new_hilite = w->numlines - 1;
    if (new_hilite != w->cursor) {
	int new_firstline = w->firstline;
#if 0
	struct file_entry *directentry = (struct file_entry *) w->hook;
#endif
	if (new_hilite < w->firstline && new_hilite >= 0)
	    new_firstline = new_hilite;
	if (new_hilite >= w->firstline + w->height / cache->rowheight)
	    new_firstline = new_hilite - w->height / cache->rowheight + 1;
	if (new_firstline != w->firstline) {
	    w->cursor = new_hilite;
	    w->firstline = new_firstline;
	    w->vert_scrollbar->firstline = (double) 65535.0 *w->firstline / (w->numlines ? w->numlines : 1);
	    render_scrollbar (w->vert_scrollbar);
	}
	w->cursor = new_hilite;
	return 1;
    }
    return 0;
}

void selection_send (XSelectionRequestEvent * rq);

int eh_NeXT_filelist (CWidget * w, XEvent * xevent, CEvent * cwevent)
{
#if 0
    int xevent_xbutton_y;
#endif
    int need_redraw = 0;
    FilelistCache *cache = (FilelistCache *) (w->user);
    int handled = 0;

    if ((w == NULL || w->hook == NULL || cache == NULL) &&
	xevent->type != Expose)
	return handled;

    switch (xevent->type) {
    case SelectionRequest:
/* fixme: later
   {
   int type;
   if (selection.text)  free (selection.text);
   selection.text = (unsigned char *) get_block (w, 0, 0, &type, &selection.len);
   selection_send (&(xevent->xselectionrequest));
   }
   return 1;
     */ case Expose:
	handled = 1;
	break;
    case ButtonPress:
	resolve_button (xevent, cwevent);
	need_redraw = change_hilite (w, filelist_handle_mouse (w, cache, cwevent->y));
	handled = 1;
	break;
    case ButtonRelease:
	resolve_button (xevent, cwevent);
	need_redraw = change_hilite (w, filelist_handle_mouse (w, cache, cwevent->y));
	handled = 1;
	break;
    case MotionNotify:
	resolve_button (xevent, cwevent);
	if (cwevent->state & (Button1Mask | Button2Mask)) {
	    need_redraw = change_hilite (w, filelist_handle_mouse (w, cache, cwevent->y));
	    handled = 1;
	}
	break;
    case FocusIn:
	w->options |= BUTTON_HIGHLIGHT;
	need_redraw = 1;
	handled = 1;
	break;
    case FocusOut:
	w->options &= ~BUTTON_HIGHLIGHT;
	need_redraw = 1;
	handled = 1;
	break;
    case EnterNotify:		/* we simply let our parent to process it */
    case LeaveNotify:
	cwevent->ident = w->ident;
	break;
    case KeyPress:
	{
	    int move_to = filelist_handle_keypress (w, cache, cwevent->command);

	    if (move_to >= 0) {
		need_redraw = change_hilite (w, move_to);
		handled = 1;
	    } else {
		switch (cwevent->command) {
		case CK_Left:
		case CK_Right:
		case CK_Home:
		case CK_End:
		case CK_Enter:
		case CK_BackSpace:
		    cwevent->ident = w->ident;
		    return 1;
		default:
		    if (cwevent->insert > 0) {
			cwevent->ident = w->ident;
			return 1;
		    }
		}
	    }
	}
	break;

    default:
	return handled;
    }

    if (need_redraw || cwevent->double_click)
	cwevent->ident = w->ident;

    if ((xevent->type == Expose && !xevent->xexpose.count) || need_redraw)
	render_NeXT_filelist (w);

    return handled;
}


/* }}} file list stuff */

/* {{{ file browser stuff */

/*****************************************************************/
/*     Miscellaneous read only text widget for dynamic text      */
/*****************************************************************/

static void render_dyn_text (CWidget * wdt)
{
    Window win = wdt->winid;
    char text[1024], *p, *q;
    int y, x = 0, w = wdt->width;
    int color = 0;
    int new_line = 1;

    CSetColor (COLOR_LIGHT);
    CRectangle (win, 1, 1, w - 2, wdt->height - 2);
    CSetColor (COLOR_BLACK);
    CSetBackgroundColor (COLOR_LIGHT);

    y = 1;			/* bevel */
    if ((q = wdt->text) == NULL)
	return;
    CPushFont ("widget", 0);
    for (;;) {
	long len;

	CSetColor (color_palette (color));
	/* looking for control characters */
	for (p = q; *p && *p >= ' '; p++);

	len = min ((unsigned long) p - (unsigned long) q, 1023);

	if (new_line) {
	    if (wdt->options & TEXT_CENTRED)
		x = (w - (TEXT_RELIEF + 1) * 2 - CImageTextWidth (q, len)) / 2;
	    else
		x = 0;
	    new_line = 0;
	}
	if (!*p) {		/* last line */
	    drawstring_xy (win, TEXT_RELIEF + 1 + x, TEXT_RELIEF + y, q);
	    break;
	} else {
	    memcpy (text, q, len);
	    text[len] = 0;
	    drawstring_xy (win, TEXT_RELIEF + 1 + x,
			   TEXT_RELIEF + y, text);
	    x += CImageTextWidth (q, len);
	}

	if (*p == '\n') {
	    new_line = 1;
	    y += FONT_PIX_PER_LINE;	/* next line */
	} else if (*p == 28)
	    color = (int) '\n';
	else
	    color = *p;

	color %= 27;
	q = p + 1;
    }
    CPopFont ();
    render_bevel (win, 0, 0, w - 1, wdt->height - 1, 1, 1);
}


/*-----------------------------------------------------------------------*/
static int eh_dyn_text (CWidget * w, XEvent * xevent, CEvent * cwevent)
{
    switch (xevent->type) {
    case Expose:
	if (!xevent->xexpose.count)
	    render_dyn_text (w);
	break;
    }
    return 0;
}

static CWidget *CDrawDynText (const char *identifier, Window parent,
		  int x, int y, int width, int rows, const char *fmt,...)
{
    va_list pa;
    char *str;
    int w, h;
    CWidget *wdt;

    va_start (pa, fmt);
    str = vsprintf_alloc (fmt, pa);
    va_end (pa);

    CPushFont ("widget", 0);
    CTextSize (&w, &h, str);
    if (width != AUTO_WIDTH && width)
	w = width;
    else
	w += TEXT_RELIEF * 2 + 2;

    if (rows != AUTO_HEIGHT && rows) {
	CTextSize (0, &h, "Ty~g$V|&^.,/_'I}[@");
	h = (h + FONT_OVERHEAD) * rows;
    }
    h += TEXT_RELIEF * 2 + 2;

    wdt = CSetupWidget (identifier, parent, x, y,
			w, h, C_TEXT_WIDGET, INPUT_EXPOSE, COLOR_FLAT, 0);
    wdt->text = (char *) strdup (str);
    wdt->eh = eh_dyn_text;
    free (str);
    set_hint_pos (x + w + WIDGET_SPACING, y + h + WIDGET_SPACING);
    CPopFont ();
    return wdt;
}

static CWidget *CRedrawDynText (const char *identifier, const char *fmt,...)
{
    va_list pa;
    char *str;
    CWidget *wdt;
#if 0
    int w, h;
#endif

    wdt = CIdent (identifier);
    if (!wdt)
	return 0;

    va_start (pa, fmt);
    str = vsprintf_alloc (fmt, pa);
    va_end (pa);

    if (wdt->text)
	free (wdt->text);
    wdt->text = (char *) strdup (str);

    render_dyn_text (wdt);
    free (str);
    return wdt;
}


/******************************************************************/
/*                   NeXT Directory Tree stuff                    */
/******************************************************************/
typedef struct NeXTDir {
    struct NeXTDir *next, *prev;

    char *name;
    char *filter;
    long cursor;
    long current;
    long firstline;
    struct file_entry *list;

} NeXTDir;

typedef struct NeXTDirTree {
    NeXTDir *first, *last, *selected;
    NeXTDir *discarded;		/* when we chdir - it will store previos tail */
    int pos;
    int numdirs;
    int numpanes;
    char *path;
    int path_length;

    int dirty;			/* can't change dirs - need to redraw widgets first */
} NeXTDirTree;

extern Bool is_directory (struct file_entry *directentry, int line_number);

static NeXTDir *create_dir_elem (char *name, int name_len)
{
    NeXTDir *dir;
    dir = (NeXTDir *) calloc (sizeof (NeXTDir), 1);
    dir->prev = dir->next = NULL;
    dir->filter = (char *) strdup ("*");
    dir->current = -1;
    dir->list = NULL;
    if (name == NULL || name_len == 0) {	/* root directory then */
	dir->name = (char *) strdup ("");
    } else {
	dir->name = malloc (name_len + 1);
	strncpy (dir->name, name, name_len);
	dir->name[name_len] = '\0';
    }
    return dir;
}

static void destroy_dir_elem (NeXTDir ** dir)
{
    if (dir) {
	if (*dir) {
	    if ((*dir)->filter)
		free ((*dir)->filter);
	    if ((*dir)->name)
		free ((*dir)->name);
	    if ((*dir)->list)
		free ((*dir)->list);
	    memset (*dir, 0x00, sizeof (NeXTDir));	/* just in case */
	    free (*dir);
	    *dir = NULL;
	}
    }
}

static NeXTDirTree *create_dir_tree (const char *path, int numpanes)
{
    NeXTDirTree *tree = NULL;
    char *ptr = 0;

    tree = (NeXTDirTree *) calloc (sizeof (NeXTDirTree), 1);

    if (path == NULL) {		/* defaulting to the root */
	tree->path_length = 1;
	tree->path = (char *) strdup ("/");
	tree->first = tree->last = tree->selected = create_dir_elem ("", 0);
        tree->numdirs++;
    } else {
	tree->path_length = strlen (path);
	tree->path = malloc (tree->path_length + 1);
	strcpy (tree->path, path);

	ptr = tree->path;

	if (*ptr == '/') {
	    tree->first = tree->last = tree->selected = create_dir_elem ("", 0);
	    tree->numdirs++;
	}
	while (*ptr) {
	    int len = 0;
	    while (*(ptr + len) && *(ptr + len) != '/')
		len++;
	    if (len) {
		tree->selected = create_dir_elem (ptr, len);
		if (tree->last == NULL)
		    tree->first = tree->last = tree->selected;
		else {
		    tree->selected->prev = tree->last;
		    tree->last->next = tree->selected;
		    tree->last = tree->selected;
		}
		tree->numdirs++;
	    }
	    ptr += len;
	    if (*ptr)
		ptr++;		/* skipping / */
	}
    }
    tree->numpanes = numpanes;
    tree->discarded = NULL;

    if (tree->selected == NULL)
	tree->selected = tree->last;
    tree->pos = tree->numdirs - tree->numpanes;
    if (tree->pos < 0)
	tree->pos = 0;
    return tree;
}

static void destroy_dir_chain (NeXTDir ** start)
{
    NeXTDir *dir;
    for (dir = *start; dir; dir = *start) {
	*start = dir->next;
	destroy_dir_elem (&dir);
    }
}

static void destroy_dir_tree (NeXTDirTree ** tree)
{
    if (tree) {
	destroy_dir_chain (&((*tree)->first));
	destroy_dir_chain (&((*tree)->discarded));
	free (*tree);
	*tree = NULL;
    }
}

static char *get_dir_path (NeXTDirTree * tree, NeXTDir * dir, char *buffer)
{
    register NeXTDir *d;
    register char *ptr1 = buffer, *ptr2;
    if (!ptr1 || !tree || !dir)
	return NULL;

    for (d = tree->first; d && d != dir; d = d->next) {
	*(ptr1++) = '/';
	ptr2 = d->name;
	if (*ptr2)
	    while (*ptr2)
		*(ptr1++) = *(ptr2++);
	else			/* we don't really want //, even thou it is not a violation */
	    ptr1--;
    }
    *ptr1 = '\0';
    return buffer;
}

static struct file_entry *read_dir_filelist (NeXTDir * dir, char *path)
{
    if (dir && path) {
	register int k;
	char *next_dir = NULL;
	CHourGlass (CFirstWindow);
	if (dir->list)
	    free (dir->list);
	dir->list = get_file_entry_list (path, FILELIST_ALL_FILES, (dir->filter) ? dir->filter : "*");
	if (dir->next)
	    next_dir = dir->next->name;
	dir->current = -1;
	if (dir->list && next_dir) {
	    for (k = 0; !(dir->list[k].options & FILELIST_LAST_ENTRY); k++)
		if (is_directory (dir->list, k))
		    if (strcmp (dir->list[k].name, next_dir) == 0) {
			dir->current = k;
			break;
		    }
	}
	dir->cursor = dir->firstline = (dir->current >= 0) ? dir->current : 0;
	CUnHourGlass (CFirstWindow);
    }
    return dir->list;
}

static void update_tree_integrity (NeXTDirTree * tree, NeXTDir * selected)
{
    NeXTDir *t;
    tree->selected = NULL;
    tree->numdirs = 0;
    tree->path_length = 0;

    for (t = tree->first; t; t = t->next) {
	tree->last = t;
	if (selected == t)
	    tree->selected = selected;
	tree->numdirs++;
	tree->path_length += 1 + strlen (t->name);
    }

    tree->path_length++;

    tree->pos = tree->numdirs - tree->numpanes;
    if (tree->pos < 0)
	tree->pos = 0;

    if (tree->selected == NULL)
	tree->selected = tree->last;
}

static void change_dir (NeXTDirTree * tree, NeXTDir * dir, long item_num)
{
    struct file_entry *item;
    NeXTDir *tmp;

    if (!tree || tree->dirty || !dir || item_num < 0)
	return;
    item = &(dir->list[item_num]);

    if (item->name[0] == '.') {
	if (item->name[1] == '\0')
	    return;
	else if (item->name[1] == '.' && item->name[2] == '\0') {	/* poping Up */
	    if (dir->prev == NULL)
		return;
	    dir = dir->prev;
	    if (tree->discarded)
		destroy_dir_chain (&(tree->discarded));
	    tree->discarded = dir->next;
	    dir->next = NULL;
	    tree->dirty = 1;
	    update_tree_integrity (tree, dir);
	    return;
	}
    }
    if (tree->discarded) {
	if (tree->discarded->prev == dir &&
	    strcmp (tree->discarded->name, item->name) == 0) {
	    tmp = dir->next;
	    dir->next = tree->discarded;
	    tree->discarded = tmp;
	} else {
	    destroy_dir_chain (&(tree->discarded));
	    tree->discarded = dir->next;
	    dir->next = NULL;
	}
    } else {
	tree->discarded = dir->next;
	dir->next = NULL;
    }

    if (dir->next == NULL) {
	dir->next = create_dir_elem (item->name, strlen (item->name));
	dir->next->prev = dir;
    }
    tree->dirty = 1;

    dir->current = item_num;
    /* taking some precautions so not to destroy tree integrity */
    update_tree_integrity (tree, dir);
}

static char *get_full_filename (NeXTDirTree * tree, NeXTDir * dir, long item)
{
    char *buffer, *path_tail;
    if (!tree || item < 0 || !dir || !(dir->list))
	return NULL;
    buffer = malloc (tree->path_length + 1 + strlen (dir->list[item].name) + 1);
    get_dir_path (tree, dir, buffer);
    path_tail = buffer + strlen (buffer);
    sprintf (path_tail, "/%s/%s", dir->name, dir->list[item].name);
    return buffer;
}

static char *commit (NeXTDirTree * tree, NeXTDir * dir)
{				/* when user presses Enter on selected item or dobleclicks */
    if (tree) {
	if (dir) {
	    if (dir->list && dir->cursor >= 0) {
		if (is_directory (dir->list, dir->cursor))
		    change_dir (tree, dir, dir->cursor);
		else
		    return get_full_filename (tree, dir, dir->cursor);
	    }
	}
    }
    return NULL;
}

/******************************************************************/
/*                   NeXT Browser widget stuff                    */
/******************************************************************/

#define FILEBROWSER_PANES 		2
#define STANDALONE_FILEBROWSER_PANES 	3

static char *mime_majors[3] =
{"url", "text", 0};

typedef struct NeXTFileBrowser {
    int numpanes;
    int focused;

#define MAX_PARTIAL_NAME 32
    char partial_name[MAX_PARTIAL_NAME];
    int partial_end;

    char **labels;
    char **lists;
    char *hscroll;
    char *fileattr;
    char *name;
    char *filter;
    NeXTDirTree *tree;
} NeXTFileBrowser;

static void clear_partial_buffer (NeXTFileBrowser * browser)
{
    if (browser) {
	browser->partial_end = 0;
    }
}


extern void CSetFilelistPosition (const char *identifier, long current, long cursor, long firstline);

static void dir_save_position (NeXTDir * dir, long cursor, long current, long firstline)
{
    dir->cursor = cursor;
    dir->current = current;
    dir->firstline = firstline;
}

static void destroy_next_filebrowser (CWidget * w)
{
    if (w->hook) {
	NeXTFileBrowser *browser = (NeXTFileBrowser *) w->hook;
	int i;
	destroy_dir_tree (&(browser->tree));
	if (browser->labels) {
	    for (i = 0; i < browser->numpanes; i++)
		if (browser->labels[i])
		    free (browser->labels[i]);
	    free (browser->labels);
	}
	if (browser->lists) {
	    for (i = 0; i < browser->numpanes; i++)
		if (browser->lists[i])
		    free (browser->lists[i]);
	    free (browser->lists);
	}
	if (browser->fileattr)
	    free (browser->fileattr);
	if (browser->name)
	    free (browser->name);
	if (browser->filter)
	    free (browser->filter);
	/* just in case */
	memset (browser, 0x00, sizeof (NeXTFileBrowser));

	free (browser);
	w->hook = 0;
    }
}

static NeXTDir *get_dir_by_num (NeXTDirTree * tree, int num)
{
    NeXTDir *dir = NULL;
    register int i;
    if (tree)
	dir = tree->first;
    for (i = 0; dir && i < num; i++)
	dir = dir->next;
    return dir;
}

static void update_filter (NeXTFileBrowser * browser, NeXTDir * dir);

static void link_browser_to_data (NeXTFileBrowser * browser, int start)
{
    NeXTDir *dir;
    char *path = NULL, *path_tail = 0;
    int i, todo = browser->numpanes;
    if (browser == NULL)
	return;
    if (start < 0 || start >= browser->numpanes)
	start = 0;

    if (start == 0)
	browser->tree->dirty = 0;

    if (browser->tree) {
	todo -= start;
	start += browser->tree->pos;
	dir = get_dir_by_num (browser->tree, start);
    } else
	dir = NULL;

    if (dir) {
	path = malloc (browser->tree->path_length + 1);
	get_dir_path (browser->tree, dir, path);
	path_tail = path + strlen (path);
    }
    for (i = 0; i < todo; i++) {
	if (!dir) {
	    CRedrawDynText (browser->labels[i], "");
	    CRedrawFilelist (browser->lists[i], 0, 0);
	} else {

	    if (dir->name) {
		*(path_tail++) = '/';
		strcpy (path_tail, dir->name);
		path_tail += strlen (dir->name);
		CRedrawDynText (browser->labels[i], "%s/%s", dir->name, dir->filter);
	    } else
		CRedrawDynText (browser->labels[i], "/%s", dir->filter);	/* root */

	    if (dir->list == NULL)
		read_dir_filelist (dir, path);
	    CRedrawFilelist (browser->lists[i], dir->list, 0);
	    CSetFilelistPosition (browser->lists[i], dir->current, dir->cursor, dir->firstline);
	    dir = dir->next;
	}
    }
}

static void update_scrollbar (CWidget * scrollbar, NeXTFileBrowser * browser, int redraw)
{
    scrollbar->firstline = (double) 65535.0 *browser->tree->pos / (browser->tree->numdirs ? browser->tree->numdirs : 1);
    scrollbar->numlines = (double) 65535.0 *browser->numpanes / (browser->tree->numdirs ? browser->tree->numdirs : 1);
    if (redraw)
	render_scrollbar (scrollbar);
}

static void link_scrollbar_to_NeXT_browser (CWidget * scrollbar, CWidget * w, XEvent * xevent, CEvent * cwevent, int whichscrbutton)
{
    /* fix me : add stuf */
    int redraw = 0;
    int new_first = w->firstline;
    NeXTFileBrowser *browser;
#if 0
    static int r = 0;
    NeXTDir *dir;
#endif

    if (w->hook == NULL)
	return;
    browser = (NeXTFileBrowser *) (w->hook);
    new_first = browser->tree->pos;

    if ((xevent->type == ButtonRelease || xevent->type == MotionNotify) && whichscrbutton == 3) {
	new_first = (double) scrollbar->firstline * browser->tree->numdirs / 65535.0;
    } else if (xevent->type == ButtonPress && (cwevent->button == Button1 || cwevent->button == Button2)) {
	switch (whichscrbutton) {
	case 1:
	    new_first -= browser->numpanes - 1;
	    break;
	case 2:
	    new_first--;
	    break;
	case 5:
	    new_first++;
	    break;
	case 4:
	    new_first += browser->numpanes - 1;
	    break;
	}
    }
    if (new_first > browser->tree->numdirs - browser->numpanes)
	new_first = browser->tree->numdirs - browser->numpanes;
    if (new_first < 0)
	new_first = 0;

    redraw = (new_first != browser->tree->pos);
    browser->tree->pos = new_first;

    if (xevent->type == ButtonRelease ||
	(redraw && !CCheckWindowEvent (xevent->xany.window, ButtonReleaseMask | ButtonMotionMask, 0)))
	link_browser_to_data (browser, 0);

    update_scrollbar (w->hori_scrollbar, browser, 0);
}

static void update_input (NeXTFileBrowser * browser, NeXTDir * dir)
{
    char *text = NULL;
    CWidget *inp_w = CIdent (browser->name);

    clear_partial_buffer (browser);

    text = get_full_filename (browser->tree, dir, dir->cursor);

    CDrawTextInput (browser->name, inp_w->parentid, inp_w->x, inp_w->y,
		    inp_w->width, inp_w->height, 256, text);
    if (text)
	free (text);

}

static void filter_changed (NeXTFileBrowser * browser, NeXTDir * dir)
{
    CWidget *inp_w = CIdent (browser->filter);
    char *new_filter = "*";
    if (inp_w->text)
	new_filter = inp_w->text;

    if (strcmp (dir->filter, new_filter)) {
	struct file_entry *list;
	if (dir->filter)
	    free (dir->filter);
	dir->filter = (char *) strdup (new_filter);
	list = dir->list;
	dir->list = NULL;
	link_browser_to_data (browser, browser->focused - browser->tree->pos);
	/* now after all filelists has updated its information 
	   we no longer need old data */
	if (list)
	    free (list);
    }
}

static void update_filter (NeXTFileBrowser * browser, NeXTDir * dir)
{
    CWidget *inp_w = CIdent (browser->filter);

    if (strcmp (dir->filter, inp_w->text) == 0)
	return;
    if (strlen (inp_w->text))
	CAddToTextInputHistory (browser->name, inp_w->text, strlen(inp_w->text));

    CDrawTextInput (browser->filter, inp_w->parentid, inp_w->x, inp_w->y,
		    inp_w->width, inp_w->height, 256, dir->filter);

}

/* returns 0 on fail */
static int goto_partial_file_name (NeXTFileBrowser * browser, NeXTDir * dir, int new_char)
{
    int success = 0;
    if (browser && dir && dir->list && browser->partial_end < MAX_PARTIAL_NAME - 1) {
	register int i;
	if (new_char != 0) {
	    browser->partial_name[browser->partial_end] = (char) new_char;
	    browser->partial_end++;
	} else if (browser->partial_end <= 0)
	    return success;
	else
	    browser->partial_end--;

	for (i = 0; !(dir->list[i].options & FILELIST_LAST_ENTRY); i++)
	    if (strncmp (dir->list[i].name, browser->partial_name, browser->partial_end) == 0) {
		dir->cursor = i;
		dir->firstline = dir->cursor;
		CSetFilelistPosition (browser->lists[browser->focused], dir->current, dir->cursor, dir->firstline);
		success++;
		break;
	    }
	if (!success)
	    browser->partial_end--;

    }
    return success;
}

static Window draw_file_browser (const char *identifier, Window parent, int x, int y,
	      const char *directory, const char *file, const char *label)
{
    CWidget *w, *tmp = 0;
    int y2, x2, x3, btn_width, btn_height;
    int text_w1, text_w2;
    Window win;
    NeXTFileBrowser *browser;
    int i;
    char *ident, *ctrl_name;
    int panes = FILEBROWSER_PANES;

    if (strcmp (identifier, "browser") == 0)
	panes = STANDALONE_FILEBROWSER_PANES;

    if (parent == CRoot)
	win = CDrawMainWindow (identifier, label);
    else
	win = CDrawHeadedDialog (identifier, parent, x, y, label);

    w = CIdent (identifier);
    w->options |= WINDOW_ALWAYS_RAISED;
    CHourGlass (CFirstWindow);
/* allocating, initializing and storing our data */
    browser = (NeXTFileBrowser *) calloc (sizeof (NeXTFileBrowser), 1);
    w->hook = browser;
    w->destroy = destroy_next_filebrowser;
    browser->tree = create_dir_tree (directory, panes);
    browser->numpanes = 0;
    CUnHourGlass (CFirstWindow);

    if (browser->tree->numdirs == 0) {
	CErrorDialog (parent, 20, 20, _ (" File browser "), _ (" Unable to read directory "));
	CDestroyWidget (identifier);
	return None;
    }
    browser->labels = (char **) calloc (sizeof (char *), panes);
    browser->lists = (char **) calloc (sizeof (char *), panes);
    browser->numpanes = panes;

    CGetHintPos (&x, &y);
    x += WINDOW_EXTRA_SPACING;
    y += WINDOW_EXTRA_SPACING;
#define FILE_BOX_WIDTH (FONT_MEAN_WIDTH * 32 + 7)
    /* we'll use it to construct our widget names */
    ident = malloc (strlen (identifier) + 20);
    strcpy (ident, identifier);
    ctrl_name = ident + strlen (identifier);
    y2 = 0;
    for (i = 0; i < browser->numpanes; i++) {
	CGetHintPos (&x2, 0);
	x2 += WINDOW_EXTRA_SPACING;
	sprintf (ctrl_name, ".label%3.3d", i);
	browser->labels[i] = (char *) strdup (ident);
	tmp = CDrawDynText (ident, win, x2, y, FILE_BOX_WIDTH, 1, "");
	tmp->options |= TEXT_CENTRED;

	/* only first time */
	if (y2 == 0) {
	    CGetHintPos (0, &y2);
	    /* y2+=WINDOW_EXTRA_SPACING ; */
	}
	sprintf (ctrl_name, ".flist%3.3d", i);
	browser->lists[i] = (char *) strdup (ident);
	tmp = CDrawFilelist (ident, win, x2, y2,
			     FILE_BOX_WIDTH, FONT_PIX_PER_LINE * 10, 0, 0, NULL, TEXTBOX_FILE_LIST);
	tmp->position |= POSITION_HEIGHT;
	xdnd_set_type_list (CDndClass, tmp->winid, xdnd_typelist_send[DndFiles]);
	/* Toolhint */
	CSetToolHint (ident, _ ("Double click to enter dir or open file."));
    }
    /* we want to automagically resize rightmost filelist */
    tmp->position |= POSITION_WIDTH;
    if (tmp->vert_scrollbar)
	tmp->vert_scrollbar->position |= POSITION_RIGHT;
    tmp = CIdent (browser->labels[browser->numpanes - 1]);
    tmp->position |= POSITION_WIDTH;

    /* the right end of dialog */
    CGetHintPos (&x3, &y2);

    strcpy (ctrl_name, ".hsc");
    i = (double) 65535.0 *browser->tree->numpanes / (browser->tree->numdirs ? browser->tree->numdirs : 1);
    w->hori_scrollbar = CDrawHorizontalScrollbar (ident, win,
			x, y2 + WINDOW_EXTRA_SPACING, x3 - x, AUTO_HEIGHT, i, i);
    CSetScrollbarCallback (ident, w->ident, link_scrollbar_to_NeXT_browser);
    w->hori_scrollbar->position = POSITION_BOTTOM | POSITION_WIDTH;
    update_scrollbar (w->hori_scrollbar, browser, 1);

    CPushFont ("widget", 0);

    CGetHintPos (0, &y2);
    strcpy (ctrl_name, ".fileattr");
    browser->fileattr = (char *) strdup (ident);
    tmp = CDrawDynText (ident, win, x, y2 + WINDOW_EXTRA_SPACING, x3 - x, 3, "");
    tmp->position |= POSITION_FILL | POSITION_BOTTOM;

    CTextSize (&text_w1, 0, "Name : ");
    CTextSize (&text_w2, 0, "Filter : ");
/* filename input stuff: */
    CGetHintPos (0, &y2);
    y2 += WINDOW_EXTRA_SPACING;
    /* label */
    strcpy (ctrl_name, ".namex");
    x2 = x;
    if (text_w1 < text_w2)
	x2 += text_w2 - text_w1;
    tmp = CDrawText (ident, win, x2, y2, _ ("Name : "));
    tmp->position |= POSITION_BOTTOM;
    /* we want filter and name inputs to be right underneath of each other */
    x2 += text_w1 + WINDOW_EXTRA_SPACING;
    /* input */
    strcpy (ctrl_name, ".finp");
    browser->name = (char *) strdup (ident);
    tmp = CDrawTextInput (ident, win, x2, y2, AUTO_WIDTH, AUTO_HEIGHT, 256, file);
    tmp->position |= POSITION_FILL | POSITION_BOTTOM;
    /* Toolhint */
    CSetToolHint (ident, _ ("Filename of the file to load."));
    /* DnD stuff */
    xdnd_set_type_list (CDndClass, tmp->winid, xdnd_typelist_send[DndFile]);
    tmp->funcs->types = DndFile;
    tmp->funcs->mime_majors = mime_majors;

/* file filter input stuff */
    CGetHintPos (0, &y2);
    y2 += WINDOW_EXTRA_SPACING;
    /* label */
    x2 = x;
    if (text_w2 < text_w1)
	x2 += text_w1 - text_w2;
    strcpy (ctrl_name, ".filtx");
    (CDrawText (ident, win, x2, y2, _ ("Filter :")))->position |= POSITION_BOTTOM;
    x2 += text_w2 + WINDOW_EXTRA_SPACING;
    /* input (we already know where to place it - right under filename input ) */
    strcpy (ctrl_name, ".filtinp");
    browser->filter = (char *) strdup (ident);
    tmp = CDrawTextInput (ident, win, x2, y2, AUTO_WIDTH, AUTO_HEIGHT, 256, TEXTINPUT_LAST_INPUT);
    tmp->position |= POSITION_FILL | POSITION_BOTTOM;
    /* Toolhint */
    CSetToolHint (ident, _ ("List only files matching this shell filter"));

/* buttons stuff */
    CGetHintPos (0, &y2);
    y2 += WINDOW_EXTRA_SPACING * 2;
    /* determining buttons size */
    CTextSize (&btn_width, &btn_height, " Cancel ");
    btn_height += 5 + BUTTON_RELIEF * 2;
    btn_width += 4 + BUTTON_RELIEF * 2;
    /* cancel button */
    x2 = x3 - (btn_width + WINDOW_EXTRA_SPACING) * 2 - WINDOW_EXTRA_SPACING;
    strcpy (ctrl_name, ".cancel");
    tmp = CDrawButton (ident, win, x2, y2, btn_width, btn_height, " Cancel ");
    tmp->position |= POSITION_RIGHT | POSITION_BOTTOM;
    CSetToolHint (ident, _ ("Abort this dialog, Escape"));
    /* ok button */
    x2 += btn_width + WINDOW_EXTRA_SPACING * 2;
    strcpy (ctrl_name, ".ok");
    tmp = CDrawButton (ident, win, x2, y2, btn_width, btn_height, "   Ok   ");
    tmp->position |= POSITION_RIGHT | POSITION_BOTTOM;
    CSetToolHint (ident, _ ("Accept, Enter"));

    CPopFont ();

/* all done - no longer need that : */
    free (ident);

/* make us real ! */
    link_browser_to_data (browser, 0);
    /* that will be our width/height : */
    y2 += btn_height + WINDOW_EXTRA_SPACING * 2;
    reset_hint_pos (x3, y2);
    CSetSizeHintPos (identifier);
    /* map us */
    CMapDialog (identifier);
    /* enable resizing */
    CSetWindowResizable (identifier, FONT_MEAN_WIDTH * 40, min (FONT_PIX_PER_LINE * 5 + 230, w->height), 1600, 1200);	/* minimum and maximum sizes */

    return win;
}

/* options */
#define GETFILE_GET_DIRECTORY		1
#define GETFILE_GET_EXISTING_FILE	2
#define GETFILE_BROWSER			4

void input_insert (CWidget * w, int c);

static void run_cmd (const char *fmt,...)
{
    signal (SIGCHLD, SIG_IGN);
    if (!fork ()) {		/* child process */
	va_list pa;
	char *str;

	va_start (pa, fmt);
	str = vsprintf_alloc (fmt, pa);
	va_end (pa);

	execlp ("/bin/sh", "/bin/sh", "-c", str, (char *) NULL);

	/* if all is fine then the thread will exit here */
	/* so displaying error if not                    */
	fprintf (stderr, "\nBad luck running command [%s].\n", str);
	exit (0);		/*thread completed */
    }
}

static char *empty_line = "";

static char *item_selected (const char *identifier, NeXTFileBrowser * browser, NeXTDir * dir)
{
    char *ptr = NULL;
    CWidget *w;

    if (!browser || !identifier)
	return 0;		/* something terrible has happen ! */
    if (dir == NULL) {
	if ((w = CIdent (browser->name)) && w->text)
	{
	    ptr = strdup (w->text);
	    if (strlen (w->text))
		CAddToTextInputHistory (browser->name, w->text, strlen(w->text));
	}
    } else {
	ptr = commit (browser->tree, dir);
	if (browser->tree->dirty)
	    link_browser_to_data (browser, 0);
    }

    if (ptr == NULL)
	ptr = empty_line;
    else if (strcmp (identifier, "browser") == 0) {
	struct file_entry *item = &(dir->list[dir->cursor]);

	if ((item->stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
	    run_cmd (ptr);
	} else {
	    static char *default_editor = "smalledit";
	    static char *editor = NULL;
	    if (editor == NULL) {
		editor = getenv ("EDITOR");
		if (editor == NULL)
		    editor = default_editor;
	    }
	    run_cmd ("%s %s", editor, ptr);
	}
	free (ptr);
	ptr = empty_line;
    }
    return ptr;
}

char *get_filelist_line_short (void *data, int line_number, char buffer[1024]);

static void cursor_moved (NeXTFileBrowser * browser, NeXTDir * dir, CWidget * list_w)
{
    if (browser && dir && list_w) {
	dir_save_position (dir, list_w->cursor, list_w->current, list_w->firstline);
	if (dir->cursor >= 0) {
	    char buffer[512];
#if 0
	    struct file_entry *item = &(dir->list[dir->cursor]);
	    char *tail;
#endif
	    if (get_filelist_line_short (dir->list, dir->cursor, buffer))
		CRedrawDynText (browser->fileattr, buffer);
	}
    }
}

/*
   Returns "" on no file entered and NULL on exit (i.e. Cancel button pushed)
   else returns the file or directory. Result must be immediately copied.
   Result must not be free'd.
 */
static char *handle_browser (const char *identifier, CEvent * cwevent, int options)
{
#if 0
    int i;
#endif
    CWidget *w = CIdent (identifier);
    NeXTFileBrowser *browser;
    int flist = -1;
    char *ptr;

    if (!w)
	return empty_line;

    if (cwevent->command == CK_Cancel)
	return 0;
    if ((browser = (NeXTFileBrowser *) w->hook) == NULL)
	return empty_line;

    /* that will be our subwidget name : */
    if ((ptr = strrchr (cwevent->ident, '.')) == NULL)
	/* nothing to do here - messege for self */
	return empty_line;

    ptr++;
    /* lets find out which one we have (starting from the shortest) */
    if (strcmp (ptr, "ok") == 0)
	/* we take text from text input and use it as filename */
	return item_selected (identifier, browser, NULL);
    else if (strncmp (ptr, "flist", 5) == 0)
	/* all filelists ends with flistNNN */
	flist = atoi (ptr + 5);	/* will process them later */
    else if (strcmp (ptr, "cancel") == 0)
	return 0;
    else if (strcmp (ptr, "finp") == 0) {
	if (cwevent->type == KeyPress && cwevent->command == CK_Enter)
	    return item_selected (identifier, browser, NULL);
	return empty_line;
    } else if (strcmp (ptr, "filtinp") == 0) {
	if (cwevent->type == KeyPress && cwevent->command == CK_Enter)
	    filter_changed (browser, browser->tree->selected);
	return empty_line;
    }
    if (flist >= 0 && flist < browser->numpanes) {	/* filelist event */
	NeXTDir *dir = get_dir_by_num (browser->tree, flist + browser->tree->pos);
	CWidget *list_w = CIdent (browser->lists[flist]);

	/* saving status */
	browser->focused = flist;
	browser->tree->selected = dir;

	switch (cwevent->type) {
	case EnterNotify:
	    CFocus (list_w);
	    update_filter (browser, dir);
	    break;
/*          case LeaveNotify : 
   fprintf(stderr, "LeaveNotify\n");
   browser->focused = -1 ; 
   CFocus( CIdent(browser->name));
   break ;
 */
	case ButtonPress:
	case ButtonRelease:
	case MotionNotify:
	    cursor_moved (browser, dir, list_w);
	    if (cwevent->type != ButtonRelease)
		update_input (browser, dir);
	    else if (cwevent->double_click)
		return item_selected (identifier, browser, dir);
	    break;
	case KeyPress:
	    {
		int scroll = 0;
		switch (cwevent->command) {
		case CK_Enter:
		    update_input (browser, dir);
		    return item_selected (identifier, browser, dir);
		case CK_Up:
		case CK_Down:
		case CK_Page_Up:
		case CK_Page_Down:
		    cursor_moved (browser, dir, list_w);
		    break;
		case CK_Left:
		    if (browser->tree->pos > 0)
			scroll = -1;
		    break;
		case CK_Right:
		    if (browser->tree->pos < browser->tree->numdirs - browser->numpanes)
			scroll = 1;
		    break;
		case CK_Home:
		    scroll = -(browser->tree->pos);
		    break;
		case CK_End:
		    scroll = (browser->tree->numdirs - browser->numpanes) - browser->tree->pos;
		    break;
		case CK_BackSpace:
		    /* remove last typed partial filename char */
		    goto_partial_file_name (browser, dir, 0);
		    break;
		default:
		    if (cwevent->insert > 0) {
			if (cwevent->key == ' ')
			    update_input (browser, dir);
			else if (isprint (cwevent->key)) {
			    goto_partial_file_name (browser, dir, cwevent->key);
			    cursor_moved (browser, dir, list_w);
			}
		    }
		}
		if (scroll != 0) {
		    browser->tree->pos += scroll;
		    link_browser_to_data (browser, 0);
		    dir = get_dir_by_num (browser->tree, browser->tree->pos + flist);
		    update_filter (browser, dir);
		    update_scrollbar (w->hori_scrollbar, browser, 1);
		}
	    }
	    break;
	default:
	    break;
	}
    }
    return empty_line;
}

Window find_mapped_window (Window w);

/* result must be free'd */
static char *look_next_get_file_or_dir (Window parent, int x, int y,
       const char *dir, const char *file, const char *label, int options)
{
    CEvent cwevent;
    XEvent xevent;
    CState s;
    CWidget *w;

    CBackupState (&s);
    CDisable ("*");
    CEnable ("_cfileBr*");

    parent = find_mapped_window (parent);
    if (!(x | y)) {
	x = 20;
	y = 20;
    }
    draw_file_browser ("CGetFile", parent, x, y, dir, file, label);

    CFocus (CIdent ("CGetFile.finp"));

    file = "";
    do {
	CNextEvent (&xevent, &cwevent);
	if (xevent.type == Expose || !xevent.type
	    || xevent.type == InternalExpose || xevent.type == TickEvent)
	    continue;
	if (!CIdent ("CGetFile")) {
	    file = 0;
	    break;
	}
	if (xevent.type == Expose || !xevent.type || xevent.type == AlarmEvent
	  || xevent.type == InternalExpose || xevent.type == TickEvent) {
	    file = "";
	    continue;
	}
	file = handle_browser ("CGetFile", &cwevent, options);
	if (!file)
	    break;
    } while (!(*file));

/* here we want to add the complete path to the text-input history: */
    w = CIdent ("CGetFile.finp");
    if (w) {
	if (w->text) {
	    free (w->text);
	    w->text = 0;
	}
	if (file)
	    w->text = (char *) strdup (file);
    }
    CDestroyWidget ("CGetFile");	/* text is added to history 
					   when text-input widget is destroyed */
    CRestoreState (&s);

    if (file)
	return (char *) ((*file) ? file : 0);
    else
	return 0;
}

/* 
   cb_browser():
   1) if filelist has decided that browser should be notifyed about event
   it sets CEvent.ident member, and we get into handle_browser.
   That includes events external for filelist (Left, Right, Home, 
   End, Space, Enter, Esc) even thou filelist does no processing of it.
   Note also that if user types in some characters while in filelist - 
   filelist will do search for filename, not the browser, as it was 
   happening in original version.
   2) handle_browser do all the neccessary processing of received events -
   updates edit boxes, updates NexTDirTree status, changes dir if needed,
   scrolls left, right, home and end.
   Sasha.
 */

static char *get_browser_name (char *ident, char *buffer)
{
    char *start = buffer;
    while (*ident && *ident != '.')
	*(buffer++) = *(ident++);
    *buffer = '\0';

    return start;
}

static int cb_browser (CWidget * w, XEvent * x, CEvent * c)
{
    char id[32];
    get_browser_name (w->ident, id);
    if (!handle_browser (id, c, GETFILE_BROWSER)) {
	w = CIdent (catstrs (id, ".finp", NULL));
	if (w)
	    if (w->text) {
		free (w->text);
		w->text = 0;
	    }
	CDestroyWidget (id);
    }
    return 0;
}

static void look_next_draw_browser (const char *ident, Window parent, int x, int y,
		   const char *dir, const char *file, const char *label)
{
    CWidget *w;

    if (!(parent | x | y)) {
	parent = CFirstWindow;
	x = 20;
	y = 20;
    }
    draw_file_browser (ident, parent, x, y, dir, file, label);
    if ((w = CIdent (ident))) {
	NeXTFileBrowser *browser = w->hook;
	int i;
	if (browser) {
	    for (i = 0; i < browser->numpanes; i++)
		CAddCallback (browser->lists[i], cb_browser);

	    CAddCallback (browser->name, cb_browser);
	    CAddCallback (browser->filter, cb_browser);
	    CAddCallback (catstrs (ident, ".ok", NULL), cb_browser);
	    CAddCallback (catstrs (ident, ".cancel", NULL), cb_browser);

	    CFocus (CIdent (catstrs (ident, ".finp", NULL)));
	}
    }
}


/* }}} file browser stuff */

/* {{{ scrollbar extras  */

#define PURENEXT
/*****************************************/
/* start NeXT scrollbar specific fetures */
/* could be anything from 13 to 19  but the true NeXT is 17 */
#define SB_WIDTH		17
/* this will define somemore parameters for shaping NeXTish scrollbars */
/* NEXT_SCROLL_CLEAN if defined removes shades of grey from buttons */
#undef NEXT_SCROLL_CLEAN
#define NEXT_SCROLL_SQUARE_ARROWS
#define SB_BORDER_WIDTH 1
/* this makes buttons thinner then scrollbar's base ( if more then 0 ) */
#define SIDE_STEP_WIDTH 1
/*  end NeXT scrollbar specific fetures  */
/*****************************************/

/*****************************************/
/* scrollbarr configuration stuff        */

#define Xdisplay CDisplay
#define Xroot    CRoot
#define Xdepth   CDepth

static char *SCROLLER_DIMPLE[] =
{
    ".%###.",
    "%#%%%%",
    "#%%...",
    "#%..  ",
    "#%.   ",
    ".%.  ."
};

#define SCROLLER_DIMPLE_WIDTH   6
#define SCROLLER_DIMPLE_HEIGHT  6

static char *SCROLLER_ARROW[4][13] =
{
    {".............",
     ".............",
     "......%......",
     "......#......",
     ".....%#%.....",
     ".....###.....",
     "....%###%....",
     "....#####....",
     "...%#####%...",
     "...#######...",
     "..%#######%..",
     ".............",
     "............."
    },
    {".............",
     ".............",
     "..%#######%..",
     "...#######...",
     "...%#####%...",
     "....#####....",
     "....%###%....",
     ".....###.....",
     ".....%#%.....",
     "......#......",
     "......%......",
     ".............",
     "............."
    },
    {"             ",
     "             ",
     "      %      ",
     "      %      ",
     "     %%%     ",
     "     %%%     ",
     "    %%%%%    ",
     "    %%%%%    ",
     "   %%%%%%%   ",
     "   %%%%%%%   ",
     "  %%%%%%%%%  ",
     "             ",
     "             "
    },
    {"             ",
     "             ",
     "  %%%%%%%%%  ",
     "   %%%%%%%   ",
     "   %%%%%%%   ",
     "    %%%%%    ",
     "    %%%%%    ",
     "     %%%     ",
     "     %%%     ",
     "      %      ",
     "      %      ",
     "             ",
     "             "
    }};

#define ARROW_SOURCE_WIDTH   13
#define ARROW_SOURCE_HEIGHT  13

typedef struct {
    Pixmap icon;
    Pixmap icon_mask;
    int origin_x, origin_y, width, height;
} Icon;

typedef struct {
    unsigned arrow_width, arrow_height;
    int bValid;
    Icon dimple;
    Icon Arrows[8];
    Pixmap stipple;
    /* this stuff is related to cache */
    XRectangle thumb;
    XRectangle buttons[2];
    unsigned int button_arrow[2];
    unsigned int width, height;
    int work_length;
    Pixmap cache;
} ScrollIcons;

#define UP_ARROW	(next_icons->Arrows[0])
#define UP_ARROW_HI	(next_icons->Arrows[1])
#define DOWN_ARROW	(next_icons->Arrows[2])
#define DOWN_ARROW_HI	(next_icons->Arrows[3])

#define ARROW_WIDTH   (next_icons->arrow_width)
#define ARROW_HEIGHT  (next_icons->arrow_height)

#define BEVEL_HI_WIDTH 1
#define BEVEL_LO_WIDTH 2
#define BEVEL_SIZE (BEVEL_HI_WIDTH+BEVEL_LO_WIDTH)

#define SB_BUTTON_HEIGHT (BEVEL_SIZE+ARROW_HEIGHT)
#define SB_BUTTONS_HEIGHT ((SB_BUTTON_HEIGHT<<1)-SIDE_STEP_WIDTH)

#ifndef SB_BORDER_WIDTH
#define SB_BORDER_WIDTH 1
#endif
#define SB_BORDER_SIZE  (SB_BORDER_WIDTH<<1)

#ifndef SIDE_STEP_WIDTH
#define SIDE_STEP_WIDTH 0
#endif

#define SB_MIN_THUMB_SIZE  (SCROLLER_DIMPLE_WIDTH+2+BEVEL_SIZE)

/* end NeXT unconfigurable stuff */
/*********************************/

#define stp_width 8
#define stp_height 8

#if 0
static unsigned char stp_bits[] =
{
    0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
#endif

#undef  TRANSPARENT
#define IS_TRANSP_SCROLL 0


typedef struct {
    GC blackGC;
    GC whiteGC;
    GC darkGC;
    GC maskGC;
    GC maskGC_0;
} IconGC;

static void init_scroll_size (ScrollIcons * next_icons)
{
    ARROW_WIDTH = (SB_WIDTH - BEVEL_SIZE - SB_BORDER_SIZE - SIDE_STEP_WIDTH);
    ARROW_WIDTH = min (ARROW_WIDTH, ARROW_SOURCE_WIDTH);

#ifdef NEXT_SCROLL_SQUARE_ARROWS
    ARROW_HEIGHT = ARROW_WIDTH;
#else
    ARROW_HEIGHT = ARROW_SOURCE_HEIGHT;
#endif
    next_icons->bValid = 1;
}

/* PROTO */
unsigned GetScrollArrowsHeight (ScrollIcons * next_icons)
{
    if (!next_icons->bValid)
	init_scroll_size (next_icons);
    return (SB_BUTTONS_HEIGHT);
}

static void CheckIconGC (IconGC * igc, Pixmap icon, Pixmap icon_mask)
{
    XGCValues values;
    unsigned long valuemask = GCForeground | GCGraphicsExposures;

    values.graphics_exposures = False;

    if (igc == NULL)
	return;
    if (igc->maskGC == None) {
	values.foreground = 1;
	igc->maskGC = XCreateGC (Xdisplay, icon_mask, valuemask, &values);
    }
    if (igc->maskGC_0 == None) {
	values.foreground = 0;
	igc->maskGC_0 = XCreateGC (Xdisplay, icon_mask, valuemask, &values);
    }
    if (igc->whiteGC == None) {
	values.foreground = COLOR_WHITE;
	igc->whiteGC = XCreateGC (Xdisplay, icon, valuemask, &values);
    }
    if (igc->darkGC == None) {
	values.foreground = COLOR_DARK;
	igc->darkGC = XCreateGC (Xdisplay, icon, valuemask, &values);
    }
    if (igc->blackGC == None) {
	values.foreground = COLOR_BLACK;
	igc->blackGC = XCreateGC (Xdisplay, icon, valuemask, &values);
    }
}

static void FreeIconGC (IconGC * igc)
{
    if (igc) {
	if (igc->maskGC != None) {
	    XFreeGC (Xdisplay, igc->maskGC);
	    igc->maskGC = None;
	}
	if (igc->maskGC_0 != None) {
	    XFreeGC (Xdisplay, igc->maskGC_0);
	    igc->maskGC_0 = None;
	}
	if (igc->whiteGC == None) {
	    XFreeGC (Xdisplay, igc->whiteGC);
	    igc->whiteGC = None;
	}
	if (igc->darkGC == None) {
	    XFreeGC (Xdisplay, igc->darkGC);
	    igc->darkGC = None;
	}
	if (igc->blackGC != None) {
	    XFreeGC (Xdisplay, igc->blackGC);
	    igc->blackGC = None;
	}
    }
}

static void renderIcon (Window win, char **data, Icon * pIcon, IconGC * igc, Bool rotate)
{
    Pixmap d, mask;
    register int i, k;
    int x, y, max_x, max_y;
    GC maskgc, paintgc;
    char pixel;

    d = XCreatePixmap (Xdisplay, win, pIcon->width, pIcon->height, Xdepth);
    mask = XCreatePixmap (Xdisplay, win, pIcon->width, pIcon->height, 1);

    if (rotate) {
	max_x = pIcon->height;
	max_y = pIcon->width;
    } else {
	max_x = pIcon->width;
	max_y = pIcon->height;
    }

    CheckIconGC (igc, d, mask);
    y = pIcon->origin_y;

    for (i = 0; i < max_y; y++, i++) {
	x = pIcon->origin_x;
	for (k = 0; k < max_x; k++, x++) {
	    maskgc = igc->maskGC;
	    if (rotate)
		pixel = data[x][y];
	    else
		pixel = data[y][x];
	    switch (pixel) {
	    case ' ':
	    case 'w':
		paintgc = igc->whiteGC;
		break;
	    case '%':
	    case 'd':
		paintgc = igc->darkGC;
		break;
	    case '#':
	    case 'b':
		paintgc = igc->blackGC;
		break;
	    case '.':
	    case 'l':
	    default:
		paintgc = CGC;
		maskgc = igc->maskGC_0;
		break;
	    }
	    XDrawPoint (Xdisplay, d, paintgc, k, i);
	    XDrawPoint (Xdisplay, mask, maskgc, k, i);
	}
    }

    pIcon->icon = d;
    pIcon->icon_mask = mask;
}

static ScrollIcons *
 init_next_icons (Window win)
{
    unsigned arrow_x_offset, arrow_y_offset;
    IconGC icongc =
    {None, None, None, None};
    int i;
    ScrollIcons *next_icons = calloc (1, sizeof (ScrollIcons));

    if (next_icons == NULL)
	return next_icons;

    CSetColor (COLOR_FLAT);

    next_icons->dimple.width = SCROLLER_DIMPLE_WIDTH;
    next_icons->dimple.height = SCROLLER_DIMPLE_WIDTH;
    renderIcon (win, SCROLLER_DIMPLE, &(next_icons->dimple), &icongc, False);

    init_scroll_size (next_icons);
    arrow_x_offset = (ARROW_SOURCE_WIDTH - ARROW_WIDTH) >> 1;
#ifdef NEXT_SCROLL_SQUARE_ARROWS
    arrow_y_offset = arrow_x_offset;
#else
    arrow_y_offset = 0;		/* not implemented yet */
#endif

    for (i = 0; i < 4; i++) {
	next_icons->Arrows[i].origin_x = arrow_x_offset;
	next_icons->Arrows[i].origin_y = arrow_y_offset;
	next_icons->Arrows[i].width = ARROW_WIDTH;
	next_icons->Arrows[i].height = ARROW_HEIGHT;
	renderIcon (win, SCROLLER_ARROW[i], &(next_icons->Arrows[i]), &icongc, False);
    }
    for (; i < 8; i++) {
	next_icons->Arrows[i].origin_x = arrow_y_offset;
	next_icons->Arrows[i].origin_y = arrow_x_offset;
	next_icons->Arrows[i].width = ARROW_HEIGHT;
	next_icons->Arrows[i].height = ARROW_WIDTH;
	renderIcon (win, SCROLLER_ARROW[i - 4], &(next_icons->Arrows[i]), &icongc, True);
    }

    FreeIconGC (&icongc);

    next_icons->stipple = XCreatePixmap (Xdisplay, win, 2, 2, Xdepth);
    XDrawPoint (Xdisplay, next_icons->stipple, CGC, 0, 0);
    XDrawPoint (Xdisplay, next_icons->stipple, CGC, 1, 1);

    CSetColor (COLOR_DARK);
    XDrawPoint (Xdisplay, next_icons->stipple, CGC, 0, 1);
    XDrawPoint (Xdisplay, next_icons->stipple, CGC, 1, 0);

    return next_icons;
}

void free_next_icons (void *ptr)
{
    ScrollIcons *next_icons = (ScrollIcons *) ptr;
    if (next_icons) {
	register int i;
	if (next_icons->dimple.icon) {
	    XFreePixmap (Xdisplay, next_icons->dimple.icon);
	    next_icons->dimple.icon = None;
	}
	if (next_icons->stipple) {
	    XFreePixmap (Xdisplay, next_icons->stipple);
	    next_icons->stipple = None;
	}
	for (i = 0; i < 4; i++)
	    if (next_icons->Arrows[i].icon) {
		XFreePixmap (Xdisplay, next_icons->Arrows[i].icon);
		next_icons->Arrows[i].icon = None;
	    }
	if (next_icons->cache) {
	    XFreePixmap (Xdisplay, next_icons->cache);
	    next_icons->cache = None;
	}
    }
}

static void render_next_icon (Drawable d, Icon * i, int x, int y)
{
#ifdef TRANSPARENT
    if (IS_TRANSP_SCROLL) {
	XSetClipMask (Xdisplay, CGC, i->icon_mask);
	XSetClipOrigin (Xdisplay, CGC, x, y);
    }
#endif
    XCopyArea (Xdisplay, i->icon, d, CGC, 0, 0,
	       i->width, i->height, x, y);

#ifdef TRANSPARENT
    if (IS_TRANSP_SCROLL)
	XSetClipMask (Xdisplay, CGC, None);
#endif
}


/* Draw bezel & arrows */
static void render_next_button (Drawable d, XRectangle * rec, Icon * icon, Bool pressed)
{
    int x2 = rec->x + rec->width - 1, y2 = rec->y + rec->height - 1;
    CSetColor ((pressed) ? COLOR_BLACK : COLOR_WHITE);

    CLine (d, rec->x, rec->y, rec->x, y2);
    CLine (d, rec->x, rec->y, x2, rec->y);

    CSetColor ((pressed) ? COLOR_WHITE : COLOR_BLACK);
    CLine (d, rec->x, y2, x2, y2);
    CLine (d, x2, y2, x2, rec->y);

    if (!pressed)
	CSetColor (COLOR_FLAT);

    CRectangle (d, rec->x + 1, rec->y + 1, rec->width - 3, rec->height - 3);

    render_next_icon (d, icon, rec->x + ((rec->width - icon->width) / 2),
		      rec->y + ((rec->height - icon->height) / 2));
}

Bool check_cache (ScrollIcons * next_icons, Window win, int width, int height, int pos, int prop, int flags)
{
    unsigned int l, wl, thumb_start, thumb_length;
    unsigned int button_arrow[2];
#if 0
    int cache_valid = 0;
#endif

    if (width > height) {
	if (height > SB_WIDTH + SB_BORDER_SIZE)		/* vertical */
	    height = SB_WIDTH + SB_BORDER_SIZE;
	l = width;
    } else {
	if (width > SB_WIDTH + SB_BORDER_SIZE)
	    width = SB_WIDTH + SB_BORDER_SIZE;
	l = height;
    }
    wl = l - SB_BORDER_SIZE - SB_BUTTONS_HEIGHT - SIDE_STEP_WIDTH;

    button_arrow[0] = 0;
    button_arrow[1] = 1;

    if (prop == 65535 || prop == 0) {
	thumb_length = 0;
	thumb_start = 0;
    } else {
	thumb_length = (double) (wl * prop) / 65535.0;
	if (thumb_length < SB_MIN_THUMB_SIZE) {
	    wl -= SB_MIN_THUMB_SIZE - thumb_length;
	    thumb_length = SB_MIN_THUMB_SIZE;
	} else if (thumb_length > wl)
	    thumb_length = wl;

	if (pos >= 65535 - prop - 1)
	    thumb_start = l - thumb_length - SB_BUTTONS_HEIGHT;
	else
	    thumb_start = ((double) (wl * pos) / 65535.0) + SB_BORDER_WIDTH;

	if (thumb_start - SB_BORDER_WIDTH > wl - thumb_length)
	    thumb_start = SB_BORDER_WIDTH + wl - thumb_length;
	if (thumb_start < SB_BORDER_WIDTH)
	    thumb_start = SB_BORDER_WIDTH;

	next_icons->work_length = wl;

	if (!(flags & 0x20)) {
	    if ((flags & 0x0f) == 2)
		button_arrow[0] = 2;
	    if ((flags & 0x0f) == 5)
		button_arrow[1] = 3;
	}
    }
    if (l != height) {
	button_arrow[0] += 4;
	button_arrow[1] += 4;
    }
    if (next_icons->cache) {
	if (width == next_icons->width && height == next_icons->height) {
	    if (button_arrow[0] == next_icons->button_arrow[0] &&
		button_arrow[1] == next_icons->button_arrow[1]) {
		if (l == height) {
		    if (next_icons->thumb.height == thumb_length &&
			next_icons->thumb.y == thumb_start)
			return True;
		} else if (next_icons->thumb.width == thumb_length &&
			   next_icons->thumb.x == thumb_start)
		    return True;
	    }
	} else {
	    XFreePixmap (Xdisplay, next_icons->cache);
	    next_icons->cache = None;
	}
    }
    next_icons->button_arrow[0] = button_arrow[0];
    next_icons->button_arrow[1] = button_arrow[1];

    if (l == height) {
	next_icons->thumb.height = thumb_length;
	next_icons->thumb.y = thumb_start;
	next_icons->thumb.width = SB_WIDTH;
	next_icons->thumb.x = SB_BORDER_WIDTH;
    } else {
	next_icons->thumb.width = thumb_length;
	next_icons->thumb.x = thumb_start;
	next_icons->thumb.height = SB_WIDTH;
	next_icons->thumb.y = SB_BORDER_WIDTH;
    }
    if (next_icons->cache == None) {	/* create double buffer */
	next_icons->cache = XCreatePixmap (Xdisplay, win, width, height, Xdepth);
	next_icons->width = width;
	next_icons->height = height;
	if (l == height) {
	    next_icons->buttons[0].x = next_icons->buttons[1].x = SB_BORDER_WIDTH;
	    next_icons->buttons[0].width = next_icons->buttons[1].width = SB_WIDTH;
	    next_icons->buttons[0].height = next_icons->buttons[1].height = SB_BUTTON_HEIGHT;
	    next_icons->buttons[1].y = l - next_icons->buttons[1].height - SIDE_STEP_WIDTH;
	    next_icons->buttons[0].y = next_icons->buttons[1].y - next_icons->buttons[0].height;
	} else {
	    next_icons->buttons[0].y = next_icons->buttons[1].y = SB_BORDER_WIDTH;
	    next_icons->buttons[0].height = next_icons->buttons[1].height = SB_WIDTH;
	    next_icons->buttons[0].width = next_icons->buttons[1].width = SB_BUTTON_HEIGHT;
	    next_icons->buttons[1].x = l - next_icons->buttons[1].width - SIDE_STEP_WIDTH;
	    next_icons->buttons[0].x = next_icons->buttons[1].x - next_icons->buttons[0].width;
	}
    }
    return False;
}

void scrollbar_fill_back (ScrollIcons * next_icons)
{
    int x2, y2;
    register int i;

    /* draw the background */
    XSetTile (Xdisplay, CGC, next_icons->stipple);
    XSetFillStyle (Xdisplay, CGC, FillTiled);
    XSetTSOrigin (Xdisplay, CGC, 0, 0);

    CSetColor (COLOR_WHITE);
    CRectangle (next_icons->cache,
		SB_BORDER_WIDTH, SB_BORDER_WIDTH,
		(next_icons->width) - SB_BORDER_SIZE,
		(next_icons->height) - SB_BORDER_SIZE);

    XSetTile (Xdisplay, CGC, None);
    XSetFillStyle (Xdisplay, CGC, FillSolid);

    x2 = next_icons->width - 1;
    y2 = next_icons->height - 1;

    CSetColor (COLOR_WHITE);
    for (i = 0; i < SB_BORDER_WIDTH; i++) {
	CLine (next_icons->cache, i, y2 - i, x2 - i, y2 - i);
	CLine (next_icons->cache, x2 - i, y2 - i, x2 - i, i);
    }
    CSetColor (COLOR_BLACK);
    for (i = 0; i < SB_BORDER_WIDTH; i++) {
	CLine (next_icons->cache, i, y2 - i, i, i);
	CLine (next_icons->cache, i, i, x2 - i, i);
    }


}

int render_next_scrollbar (ScrollIcons * next_icons, Window win, int x, int y, int width, int height, int pos, int prop, int flags)
{
    int cache_valid;

    cache_valid = check_cache (next_icons, win, width, height, pos, prop, flags);
/*fprintf( stderr, "%dx%d,pos: %d, prop: %d ->%s, whole: %dx%d, thumb: %dx%d%+d%+d\n",
   width, height, pos, prop, 
   (cache_valid)?"Cached":"Redrawing",
   next_icons->width, next_icons->height,
   next_icons->thumb.width, next_icons->thumb.height,
   next_icons->thumb.x, next_icons->thumb.y ); 
 */ if (!cache_valid) {
	int i;
	scrollbar_fill_back (next_icons);
	if (next_icons->thumb.width > 0 &&
	    next_icons->thumb.height > 0) {
	    render_next_button (next_icons->cache,
				&(next_icons->thumb),
				&(next_icons->dimple),
				False);
	    for (i = 0; i < 2; i++) {
		render_next_button (next_icons->cache,
				    &(next_icons->buttons[i]),
		      &(next_icons->Arrows[next_icons->button_arrow[i]]),
				    (next_icons->button_arrow[i] & 0x02));
	    }
	}
    }
    XCopyArea (Xdisplay, next_icons->cache, win, CGC, 0, 0, next_icons->width, next_icons->height, 0, 0);
    return 1;
}

/* }}} end scrollbar extras */

int find_menu_hotkey (struct menu_item m[], int this, int num);

/* outermost bevel */
#define BEVEL_MAIN	1
/* next-outermost bevel */
#define BEVEL_IN 	1
#define BEVEL_OUT	1
/* between items, and between items and next-outermost bevel */
#define SPACING		0

/* between items rectangle and text */
#define RELIEF		4

#define S		SPACING
/* between window border and items */
#define O		(BEVEL_OUT + SPACING)
/* total height of an item */

/* size of bar item */
#define BAR_HEIGHT	H
#define ITEM_BEVEL_TYPE 0
#define ITEM_BEVEL      1

#define H		(FONT_PIX_PER_LINE + RELIEF * 2 + 1)

#define B		BAR_HEIGHT

static char *look_next_get_default_widget_font (void)
{
    return "-*-fixed-medium-r-normal--%d-*-*-*-*-*-*";
}

static void look_next_get_menu_item_extents (int n, int j, struct menu_item m[], int *border, int *relief, int *y1, int *y2)
{
    int i, n_items = 0, n_bars = 0;

    *border = O;
    *relief = RELIEF;

    if (!n || j < 0) {
	*y1 = O;
	*y2 = *y1 + H;
    } else {
	int not_bar;
	not_bar = 1;
	for (i = 0; i < j; i++)
	    if (m[i].text[2])
		n_items++;
	    else
		n_bars++;
	*y1 = O + n_items * (H + S) + n_bars * (B + S) + (not_bar ? 0 : 2);
	*y2 = *y1 + (not_bar ? H : (B - 4));
    }
}

static void look_next_menu_draw (Window win, int w, int h, struct menu_item m[], int n, int light)
{
    int i, y1, y2, offset = 0;
    static int last_light = 0, last_n = 0;
    static Window last_win = 0;

    if (last_win == win && last_n != n) {
	XClearWindow (CDisplay, win);
	render_bevel (win, 0, 0, w - 1, h - 1, BEVEL_MAIN, 0);
	render_bevel (win, BEVEL_IN, BEVEL_IN, w - 1 - BEVEL_IN, h - 1 - BEVEL_IN, BEVEL_OUT - BEVEL_IN, 1);
    } else if (last_light >= 0 && last_light < n) {
	int border, relief;
	look_next_get_menu_item_extents (n, last_light, m, &border, &relief, &y1, &y2);
	CSetColor (COLOR_FLAT);
	CRectangle (win, O + 1, y1 - 1, w - O * 2 + 2, y2 - y1 + 2);
    }
    last_win = win;
    last_n = n;
    CPushFont ("widget", 0);
    for (i = 0; i < n; i++) {
	int border, relief;
	look_next_get_menu_item_extents (n, i, m, &border, &relief, &y1, &y2);
	render_bevel (win, O, y1, w - O - 1, y2 - 1, ITEM_BEVEL, ITEM_BEVEL_TYPE);
	if (i == light && m[i].text[2]) {
	    CSetColor (color_widget (14));
	    CRectangle (win, O + 1, y1 + 1, w - O * 2 - 4, y2 - y1 - 4);
	}
	if (m[i].text[2]) {
	    char *u;
	    u = strrchr (m[i].text, '\t');
	    if (u)
		*u = 0;
	    CSetColor (COLOR_BLACK);
	    if (m[i].hot_key == '~')
		m[i].hot_key = find_menu_hotkey (m, i, n);
	    if (i == light)
		CSetBackgroundColor (color_widget (14));
	    else
		CSetBackgroundColor (COLOR_FLAT);
	    drawstring_xy_hotkey (win, RELIEF + O - offset, RELIEF + y1 - offset, m[i].text, m[i].hot_key);
	    if (u) {
		drawstring_xy (win, RELIEF + O + (w - (O + RELIEF) * 2 - CImageStringWidth (u + 1)) - offset,
			       RELIEF + y1 - offset, u + 1);
		*u = '\t';
	    }
	}
    }
    last_light = light;
    CPopFont ();
}

static void look_next_render_menu_button (CWidget * wdt)
{
    int w = wdt->width, h = wdt->height;
    int x = 0, y = 0;

    Window win = wdt->winid;

    if (wdt->disabled || !((wdt->droppedmenu[0]) ||
		 (wdt->options & (BUTTON_PRESSED | BUTTON_HIGHLIGHT)))) {
	CSetColor (COLOR_FLAT);
	CRectangle (win, x, y, x + w, y + h);
	CSetColor (COLOR_BLACK);
	CSetBackgroundColor (COLOR_FLAT);
    } else {
	CSetColor (COLOR_DARK);
	CRectangle (win, x, y, x + w - 1, y + h - 1);
	if (wdt->options & BUTTON_PRESSED)
	    render_bevel (win, x, y, x + w - 1, y + h - 1, 2, 1);
	CSetColor (COLOR_WHITE);
	CSetBackgroundColor (COLOR_DARK);
    }
    if (!wdt->label)
	return;
    if (!(*(wdt->label)))
	return;
    CPushFont ("widget", 0);
    drawstring_xy_hotkey (win, x + 2 + BUTTON_RELIEF, y + 2 + BUTTON_RELIEF, wdt->label, wdt->hotkey);
    CPopFont ();
}

static void look_next_render_button (CWidget * wdt)
{
    int w = wdt->width, h = wdt->height;
    int x = 0, y = 0;

    Window win = wdt->winid;
#define BUTTON_BEVEL 1

    if((wdt->options & BUTTON_HIGHLIGHT) && !wdt->disabled)  
    {
	CSetColor (COLOR_BLACK);
	XDrawRectangle (CDisplay, win, CGC, x, y, w-1, h-1);
    } else
        render_bevel (win, x, y, x + w - 1, y + h - 1, BUTTON_BEVEL, ( wdt->options & BUTTON_PRESSED )?1:0);

    if (!wdt->label)
	return;
    if (!(*(wdt->label)))
	return;
    CSetColor (COLOR_BLACK);
    CSetBackgroundColor (COLOR_FLAT);
    CPushFont ("widget", 0);
    drawstring_xy_hotkey (win, x + 2 + BUTTON_RELIEF, y + 2 + BUTTON_RELIEF, wdt->label, wdt->hotkey);
    CPopFont ();
}

static void look_next_render_bar (CWidget * wdt)
{
    int w = wdt->width, h = wdt->height;

    Window win = wdt->winid;

    CSetColor (COLOR_FLAT);
    CLine (win, 1, 1, w - 2, 1);
    render_bevel (win, 0, 0, w - 1, h - 1, 1, 1);
}

static void look_next_render_sunken_bevel (Window win, int x1, int y1, int x2, int y2, int thick,
					   int sunken)
{
    int i;
    if ((sunken & 2) && (y2 - y1 - 2 * thick + 1 > 0) && (x2 - x1 - 2 * thick + 1 > 0)) {
	CSetColor (COLOR_FLAT);
	CRectangle (win, x1 + thick, y1 + thick, x2 - x1 - 2 * thick + 1, y2 - y1 - 2 * thick + 1);
    }
    CSetColor (COLOR_BLACK);
    CLine (win, x1, y1, x2, y1);
    CLine (win, x1, y1, x1, y2);

    CSetColor (COLOR_WHITE);
    for (i = 0; i < thick; i++) {
	CLine (win, x2 - i, y1 + i, x2 - i, y2 - i);
	CLine (win, x1 + i, y2 - i, x2 - i, y2 - i);
    }
    if (x2 - x1 - i > 0)
	XClearArea (CDisplay, win, x1 + i, y1 + i, x2 - x1 - i, 1, False);
    if (y2 - y1 - i > 0)
	XClearArea (CDisplay, win, x1 + i, y1 + i, 1, y2 - y1 - i, False);

    CSetColor (COLOR_DARK);
    XDrawPoint (CDisplay, win, CGC, x1, y2);
    XDrawPoint (CDisplay, win, CGC, x2, y1);
    x1++;
    x2--;
    y1++;
    y2--;
    for (i = 0; i < thick; i++) {
	CLine (win, x1 + i, y2 - i, x1 + i, y1 + i);
	CLine (win, x1 + i, y1 + i, x2 - i, y1 + i);
    }
}

static void look_next_render_raised_bevel (Window win, int x1, int y1, int x2, int y2, int thick,
					   int sunken)
{
    int i;
    if ((sunken & 2) && (y2 - y1 - 2 * thick + 1 > 0) && (x2 - x1 - 2 * thick + 1 > 0)) {
	CSetColor (COLOR_FLAT);
	CRectangle (win, x1 + thick, y1 + thick, x2 - x1 - 2 * thick + 1, y2 - y1 - 2 * thick + 1);
    }
    CSetColor (COLOR_WHITE);
    for (i = 0; i < thick; i++) {
	CLine (win, x1 + i, y1 + i, x2 - i, y1 + i);
	CLine (win, x1 + i, y1 + i, x1 + i, y2 - i);
    }

    if (x2 - x1 - i > 0)
	XClearArea (CDisplay, win, x1 + i, y1 + i, x2 - x1 - i, 1, False);
    if (y2 - y1 - i > 0)
	XClearArea (CDisplay, win, x1 + i, y1 + i, 1, y2 - y1 - i, False);

    CSetColor (COLOR_BLACK);
    CLine (win, x2, y1, x2, y2);
    CLine (win, x1, y2, x2, y2);
    CSetColor (COLOR_DARK);
    XDrawPoint (CDisplay, win, CGC, x1, y2);
    XDrawPoint (CDisplay, win, CGC, x2, y1);
    x1++;
    x2--;
    y1++;
    y2--;
    for (i = 0; i < thick; i++) {
	CLine (win, x1 + i, y2 - i, x2 - i, y2 - i);
	CLine (win, x2 - i, y1 + i, x2 - i, y2 - i);
    }
}

static void look_next_draw_hotkey_understroke (Window win, int x, int y, int hotkey)
{
    CLine (win, x+1, y , x + FONT_PER_CHAR (hotkey) - 2, y);
    y++;
    CLine (win, x+1, y, x + FONT_PER_CHAR (hotkey) - 2, y);
}

static void look_next_render_text (CWidget * wdt)
{
    Window win = wdt->winid;
    char text[1024], *p, *q;
    int hot, y, w = wdt->width, center = 0;
    int releif = 1 ;

    releif += TEXT_RELIEF ;
    CSetColor (COLOR_FLAT);
    CRectangle (win, 1, 1, w - 2, wdt->height - 2);
    CSetColor (COLOR_BLACK);

    hot = wdt->hotkey;		/* a letter that needs underlining */
    y = 1;			/* bevel */
    q = wdt->text;
    CPushFont ("widget", 0);
    CSetBackgroundColor (COLOR_FLAT);
    for (;;) {
	p = strchr (q, '\n');
	if (!p) {		/* last line */
	    if (wdt->options & TEXT_CENTRED)
		center = (wdt->width - releif * 2 - CImageTextWidth (q, strlen (q))) / 2;
	    drawstring_xy_hotkey (win, releif + center, releif + y - 1, q, hot);
	    break;
	} else {
	    int l;
	    l = min (1023, (unsigned long) p - (unsigned long) q);
	    memcpy (text, q, l);
	    text[l] = 0;
	    if (wdt->options & TEXT_CENTRED)
		center = (wdt->width - releif * 2 - CImageTextWidth (q, l)) / 2;
	    drawstring_xy_hotkey (win, releif + center, releif + y - 1, text, hot);
	}
	y += FONT_PIX_PER_LINE;
	hot = 0;		/* only for first line */
	q = p + 1;		/* next line */
    }
    CPopFont ();
}

#define NEXT_HANDLE	  25
#define NEXT_LOWBAR       7

static void look_next_render_window (CWidget * wdt)
{
    int w = wdt->width, h = wdt->height;

    Window win = wdt->winid;

    if (wdt->options & WINDOW_NO_BORDER)
	return;
    if ((wdt->position & WINDOW_RESIZABLE) && CRoot != wdt->parentid) {
	CSetColor (COLOR_FLAT);
	CRectangle (win, 0, 0, w - 1, h - 1);
	render_bevel (win, 1, h - NEXT_LOWBAR - 1, NEXT_HANDLE + 1, h - 1, 1, 0);
	render_bevel (win, NEXT_HANDLE + 2, h - NEXT_LOWBAR - 1, w - NEXT_HANDLE - 3, h - 1, 1, 0);
	render_bevel (win, w - NEXT_HANDLE - 2, h - NEXT_LOWBAR - 1, w - 1, h - 1, 1, 0);
    }
    render_bevel (win, 0, 0, w - 1, h - 1, 2, 0);
    if (CRoot != wdt->parentid)
	if (win == CGetFocus ())
	    render_bevel (win, 4, 4, w - 5, h - 5, 3, 1);
}

static void look_next_render_scrollbar (CWidget * wdt)
{
    int pos, prop;
    int flags = wdt->options;
    if (!wdt)
	return;

    pos = wdt->firstline;
    prop = wdt->numlines;

    if (prop < 0)
	prop = 0;
    if (pos < 0)
	pos = 0;
    if (pos > 65535)
	pos = 65535;
    if (pos + prop >= 65535)
	prop = 65535 - pos;

    render_next_scrollbar ((ScrollIcons *) wdt->user, wdt->winid,
			   wdt->x, wdt->y, wdt->width, wdt->height,
			   pos, prop, flags);

    if (wdt->scroll_bar_extra_render)
	(*wdt->scroll_bar_extra_render) (wdt);
}

/*
   Which scrollbar button was pressed: 3 is the middle button ?
 */
static int look_next_which_scrollbar_button (int bx, int by, CWidget * wdt)
{
#if 0
    int pos;
    int prop;
#endif
    ScrollIcons *next_icons = (ScrollIcons *) wdt->user;
/*    
   fprintf( stderr, "button: (%d,%d) thumb: (%d,%d)\n",bx, by, next_icons->thumb.x, next_icons->thumb.y  );    
 */
    if (wdt->kind == C_VERTSCROLL_WIDGET) {
	if (by > 0 && by < next_icons->thumb.y)
	    return 1;
	if (by >= next_icons->thumb.y &&
	    by <= next_icons->thumb.y + next_icons->thumb.height)
	    return 3;
	if (by > next_icons->thumb.y + next_icons->thumb.height &&
	    by < next_icons->buttons[0].y)
	    return 4;
	if (by >= next_icons->buttons[0].y &&
	  by <= next_icons->buttons[0].y + next_icons->buttons[0].height)
	    return 2;
	if (by >= next_icons->buttons[1].y &&
	  by <= next_icons->buttons[1].y + next_icons->buttons[1].height)
	    return 5;
    } else {
	if (bx > 0 && bx < next_icons->thumb.x)
	    return 1;
	if (bx >= next_icons->thumb.x &&
	    bx <= next_icons->thumb.x + next_icons->thumb.width)
	    return 3;
	if (bx > next_icons->thumb.x + next_icons->thumb.width &&
	    bx < next_icons->buttons[0].x)
	    return 4;
	if (bx >= next_icons->buttons[0].x &&
	    bx <= next_icons->buttons[0].x + next_icons->buttons[0].width)
	    return 2;
	if (bx >= next_icons->buttons[1].x &&
	    bx <= next_icons->buttons[1].x + next_icons->buttons[1].width)
	    return 5;

    }
    return 0;
}

static int look_next_scrollbar_handler (CWidget * w, XEvent * xevent, CEvent * cwevent)
{
    static int buttonypos, y, offset, whichscrbutton = 0;	/* which of the five scroll bar buttons was pressed */
    int xevent_xbutton_y, length, width, thumb_pos;
    ScrollIcons *next_icons = (ScrollIcons *) w->user;

    if (w->kind == C_VERTSCROLL_WIDGET) {
	xevent_xbutton_y = xevent->xbutton.y;
	length = w->height - SB_BUTTONS_HEIGHT - SB_BORDER_SIZE - next_icons->thumb.height;
	width = w->width;
	thumb_pos = next_icons->thumb.y;
    } else {
	xevent_xbutton_y = xevent->xbutton.x;
	length = w->width - SB_BUTTONS_HEIGHT - SB_BORDER_SIZE - next_icons->thumb.width;
	width = w->height;
	thumb_pos = next_icons->thumb.x;
    }

    if (next_icons->thumb.width == 0 || next_icons->thumb.height == 0) {
	if (xevent->type == Expose && !xevent->xexpose.count)
	    render_scrollbar (w);
	return 0;
    }
    switch (xevent->type) {
/*    case LeaveNotify:
 */
    case Expose:
	w->options = whichscrbutton = 0;
	break;
    case ButtonRepeat:
	resolve_button (xevent, cwevent);
	if (whichscrbutton != 3 &&
	    (cwevent->button == Button1 || cwevent->button == Button2)) {
	    int b;
	    b = look_next_which_scrollbar_button (cwevent->x, cwevent->y, w);
	    if (b != 3 && b > 0) {
		y = w->firstline;
		buttonypos = xevent_xbutton_y;
		w->options = whichscrbutton = b;
		cwevent->ident = w->ident;
		xevent->type = cwevent->type = ButtonPress;
	    }
	}
	break;
    case ButtonPress:
	resolve_button (xevent, cwevent);
	if (cwevent->button == Button1 || cwevent->button == Button2) {
	    buttonypos = xevent_xbutton_y;
	    y = w->firstline;
	    w->options = whichscrbutton = look_next_which_scrollbar_button (cwevent->x, cwevent->y, w);
	    if (whichscrbutton == 3) {
		offset = thumb_pos - buttonypos + SB_BORDER_WIDTH;
	    } else
		offset = 0;
	    cwevent->ident = w->ident;
	    w->search_start = w->firstline;
	    w->search_len = w->numlines;
	}
	break;
    case ButtonRelease:
	resolve_button (xevent, cwevent);
	if (whichscrbutton == 3) {
	    w->options = 0x20 + whichscrbutton;
	    y = (double) (xevent_xbutton_y + offset) * (double) 65535.0 / next_icons->work_length;
	    w->firstline = y;
	    buttonypos = xevent_xbutton_y;
	}
	break;
    case MotionNotify:
	if (whichscrbutton == 0)
	    return 0;
	resolve_button (xevent, cwevent);
	if (cwevent->state & (Button1Mask | Button2Mask)) {
	    w->options = whichscrbutton;
	    if (whichscrbutton == 3) {
		y = (double) (xevent_xbutton_y + offset) * (double) 65535.0 / (next_icons->work_length);
		w->firstline = y;
		buttonypos = xevent_xbutton_y;
	    }
	}
	break;
    default:
	return 0;
    }

    if (w->firstline > 65535)
	w->firstline = 65535;
    if (cwevent->state & (Button1Mask | Button2Mask) || cwevent->type == ButtonPress || cwevent->type == ButtonRelease)
	if (w->scroll_bar_link && w->vert_scrollbar)
	    (*w->scroll_bar_link) (w, w->vert_scrollbar, xevent, cwevent, whichscrbutton);

    if (cwevent->type == ButtonRelease)
	w->options = whichscrbutton = 0;
    if (xevent->type != Expose || !xevent->xexpose.count)
	look_next_render_scrollbar (w);

    return 0;
}

static void look_next_init_scrollbar_icons (CWidget * w)
{
    w->user = init_next_icons (w->winid);
    w->free_user = free_next_icons;
}

static int look_next_get_scrollbar_size (int type)
{
    return SB_WIDTH + SB_BORDER_SIZE;
}

static void look_next_get_button_color (XColor * color, int i)
{
    color->red = color->green = (i * 4) * 65535 / 63;
    color->blue = (i * 4) * 65535 / 63;
    color->flags = DoRed | DoBlue | DoGreen;
}

static int look_next_window_handler (CWidget * w, XEvent * xevent, CEvent * cwevent)
{
    static Window window_is_resizing = 0;
    static int windowx, windowy;
    static int wx = 0, wy = 0;
    static int wwidth = 0, wheight = 0;
    static int allowwindowmove = 0;
    static int allowwindowresize = 0;

    switch (xevent->type) {
    case ClientMessage:
	if (!w->disabled)
	    cwevent->ident = w->ident;
	break;
    case Expose:
	if (!xevent->xexpose.count)
	    render_window (w);
	break;
    case ButtonRelease:
	strcpy (cwevent->ident, w->ident);
	window_is_resizing = 0;
	resolve_button (xevent, cwevent);
	allowwindowmove = 0;
	allowwindowresize = 0;
	break;
    case ButtonPress:
	strcpy (cwevent->ident, w->ident);
	resolve_button (xevent, cwevent);
	if (cwevent->double_click == 1) {
	    CWidget *c = CChildFocus (w);
	    if (c)
		CFocus (c);
	}
	if (cwevent->button == Button1 && !(w->position & WINDOW_ALWAYS_LOWERED)) {
	    XRaiseWindow (CDisplay, w->winid);
	    CRaiseWindows ();
	} else if (cwevent->button == Button2 && !(w->position & WINDOW_ALWAYS_RAISED)) {
	    XLowerWindow (CDisplay, w->winid);
	    CLowerWindows ();
	}
	windowx = xevent->xbutton.x_root - w->x;
	windowy = xevent->xbutton.y_root - w->y;
	wx = xevent->xbutton.x;
	wy = xevent->xbutton.y;
	wwidth = w->width;
	wheight = w->height;
	if (wy > w->height - NEXT_LOWBAR && w->position & WINDOW_RESIZABLE) {
	    allowwindowresize = 1;
	    if (wx < NEXT_HANDLE) {
		allowwindowresize++;
		if (w->position & WINDOW_UNMOVEABLE)
		    allowwindowresize++;
	    } else if (wx < w->width - NEXT_HANDLE - 1)
		allowwindowresize = 3;
	} else
	    allowwindowmove = 1;
	break;
    case MotionNotify:
	resolve_button (xevent, cwevent);
	if ((w->position & WINDOW_RESIZABLE) && allowwindowresize
	    && (cwevent->state & (Button1Mask | Button2Mask))) {
	    int wi, he;
#if 0
	    int new_x, new_y;
#endif
	    int dx = xevent->xmotion.x_root - windowx - w->x;
	    int dy = xevent->xmotion.y_root - windowy - w->y;
	    window_is_resizing = w->winid;
	    he = wheight + dy;
	    wi = wwidth;

	    if (allowwindowresize == 1)
		wi += dx;
	    else if (allowwindowresize == 2)
		wi -= dx;

/* this is actually for the edit windows, and needs to be generalized */
	    if (wi < w->mark1)
		wi = w->mark1;
	    if (he < w->mark2)
		he = w->mark2;

	    wi -= w->firstcolumn;
	    wi -= wi % w->textlength;
	    wi += w->firstcolumn;
	    he -= w->firstline;
	    he -= he % w->numlines;
	    he += w->firstline;
	    w->position &= ~WINDOW_MAXIMISED;
	    dx = wi - w->width;
	    CSetSize (w, wi, he);
	    if (allowwindowresize == 2 && dx != 0) {
		w->x -= dx;
		wwidth = w->width;
		XMoveWindow (CDisplay, w->winid, w->x, w->y);
	    }
	}
	if (!(w->position & WINDOW_UNMOVEABLE) && allowwindowmove
	    && (cwevent->state & (Button1Mask | Button2Mask))) {
	    w->x = xevent->xmotion.x_root - windowx;
	    w->y = xevent->xmotion.y_root - windowy;
	    if (w->x + xevent->xmotion.x < 2)
		w->x = -wx + 2;
	    if (w->y + xevent->xmotion.y < 2)
		w->y = -wy + 2;
	    XMoveWindow (CDisplay, w->winid, w->x, w->y);
	}
	break;
    }
    return 0;
}

extern Pixmap Cswitchon;
extern Pixmap Cswitchoff;

static void look_next_render_switch (CWidget * wdt)
{
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    int x = 0, y = 0;

    CSetColor (COLOR_FLAT);
    CRectangle (win, x+2, y+2, w - 4, h - 4);

    CSetColor (wdt->fg);
    CSetBackgroundColor (wdt->bg);
    if (wdt->options & SWITCH_PICTURE_TYPE) {
	if (wdt->keypressed)
	    XCopyPlane (CDisplay, Cswitchon, win, CGC, 0, 0,
			w, h, x, y, 1);
	else
	    XCopyPlane (CDisplay, Cswitchoff, win, CGC, 0, 0,
			w, h, x, y, 1);
    } else {
	if (wdt->keypressed)
	{
	    render_bevel (win, x + 1, y + 1, x + w - 1, y + h - 1, 1, 0);
	    CSetColor (COLOR_DARK);
	    CLine( win, x+1+3, y+1+11, x+1+10, y+1+3 );
	    CSetColor( COLOR_BLACK );
	    CLine( win, x+1+4, y+1+5, x+1+4, y+1+8 );
	    CLine( win, x+1+4, y+1+11, x+1+10, y+1+4 );
	    CSetColor( COLOR_WHITE );
	    CLine(win, x+1+3, y+1+5, x+1+3, y+1+10 );
	    CLine(win, x+1+3, y+1+10, x+1+9, y+1+3 );	    
	}else
	    render_bevel (win, x + 1, y + 1, x + w - 2, y + h - 2, 1, 0);

    }
    if (wdt->options & (BUTTON_HIGHLIGHT | BUTTON_PRESSED))
    	CSetColor(COLOR_BLACK);
    else
    	CSetColor(COLOR_FLAT);
    XDrawRectangle (CDisplay, win, CGC, x, y, w-1, h-1);
}

static void look_next_edit_render_tidbits (CWidget * wdt)
{
    int isfocussed;
    int w = wdt->width, h = wdt->height;
    Window win;

    win = wdt->winid;
    isfocussed = (win == CGetFocus ());
    CSetColor (COLOR_FLAT);
    render_bevel (win, 0, 0, w - 1, h - 1, 1, 1);	/*most outer border bevel */
}

static CWidget *look_next_draw_cancel_button (char *ident, Window win, int x, int y)
{
    CWidget *wdt;
#if 0
    CGetHintPos (&x, 0);
#endif
    wdt = CDrawButton (ident, win, x, y, AUTO_WIDTH, AUTO_HEIGHT, _(" Cancel "));
#if 0
    CGetHintPos (0, &y);
    reset_hint_pos (x + WINDOW_EXTRA_SPACING, y + 2 * WINDOW_EXTRA_SPACING);
#endif
    return wdt;
}

static CWidget *look_next_draw_ok_button (char *ident, Window win, int x, int y)
{
    CWidget *wdt;
#if 0
    CGetHintPos (&x, 0);
#endif
    wdt = CDrawButton (ident, win, x, y, AUTO_WIDTH, AUTO_HEIGHT, _("   Ok   "));
#if 0
    CGetHintPos (0, &y);
    reset_hint_pos (x + WINDOW_EXTRA_SPACING, y + 2 * WINDOW_EXTRA_SPACING);
#endif
    return wdt;
}

static void look_next_render_fielded_textbox_tidbits (CWidget *w, int isfocussed)
{
    render_bevel (w->winid, 0, 0, w->width - 1, w->height - 1, 1, 1);	/*most outer border bevel */
}

static void look_next_render_textbox_tidbits (CWidget * w, int isfocussed)
{
    if (isfocussed) {
	render_bevel (w->winid, 0, 0, w->width - 1, w->height - 1, 1, 1);	/*most outer border bevel */
    } else {
	render_bevel (w->winid, 0, 0, w->width - 1, w->height - 1, 1, 0);	/*most outer border bevel */
    }
}

static void look_next_render_passwordinput_tidbits (CWidget * wdt, int isfocussed)
{
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    CSetColor (COLOR_WHITE);
    XDrawRectangle (CDisplay, win, CGC, 1, 1, w - 2, h - 2);
    XDrawRectangle (CDisplay, win, CGC, 2, 2, w - 4, h - 4);
    if (win == CGetFocus ()) {
	render_bevel (win, 0, 0, w - 1, h - 1, 1, 1);
    } else {
	render_bevel (win, 0, 0, w - 1, h - 1, 1, 1);
    }
}

static void look_next_render_textinput_tidbits (CWidget * wdt, int isfocussed)
{
    int w = wdt->width, h = wdt->height;
    Window win = wdt->winid;
    CSetColor (COLOR_WHITE);
    XDrawRectangle (CDisplay, win, CGC, 1, 1, w - h - 3, h - 3);
    XDrawRectangle (CDisplay, win, CGC, 2, 2, w - h - 5, h - 5);
    if (isfocussed) {
	render_bevel (win, 0, 0, w - h - 1, h - 1, 1, 1);	/*most outer border bevel */
    } else {
	render_bevel (win, 0, 0, w - h - 1, h - 1, 1, 1);	/*most outer border bevel */
    }
    /* history button to the right */
    if (wdt->options & BUTTON_PRESSED) {
	CRectangle (win, w - h + 2, 2, h - 4, h - 4);
	render_bevel (win, w - h, 0, w - 1, h - 1, 2, 3);
    } else if (wdt->options & BUTTON_HIGHLIGHT) {
	CRectangle (win, w - h + 1, 1, h - 2, h - 2);
	render_bevel (win, w - h, 0, w - 1, h - 1, 1, 2);
    } else {
	CRectangle (win, w - h + 2, 2, h - 4, h - 4);
	render_bevel (win, w - h, 0, w - 1, h - 1, 2, 2);
    }
}

extern struct focus_win focus_border;

static void render_focus_border_n (Window win, int i)
{
    int j;
    j = (i > 3) + 1;
    if (win == focus_border.top) {
	render_bevel (win, 0, 0, focus_border.width + 2 * WIDGET_FOCUS_RING - 1, focus_border.height + 2 * WIDGET_FOCUS_RING - 1, j, 0);
	render_bevel (win, i, i, focus_border.width + 2 * WIDGET_FOCUS_RING - 1 - i, focus_border.height + 2 * WIDGET_FOCUS_RING - 1 - i, 2, 1);
    } else if (win == focus_border.bottom) {
	render_bevel (win, 0, 0 - focus_border.height, focus_border.width + 2 * WIDGET_FOCUS_RING - 1, WIDGET_FOCUS_RING - 1, j, 0);
	render_bevel (win, i, i - focus_border.height, focus_border.width + 2 * WIDGET_FOCUS_RING - 1 - i, WIDGET_FOCUS_RING - 1 - i, 2, 1);
    } else if (win == focus_border.left) {
	render_bevel (win, 0, 0 - WIDGET_FOCUS_RING, focus_border.width + 2 * WIDGET_FOCUS_RING - 1, focus_border.height + WIDGET_FOCUS_RING - 1, j, 0);
	render_bevel (win, i, i - WIDGET_FOCUS_RING, focus_border.width + 2 * WIDGET_FOCUS_RING - 1 - i, focus_border.height + WIDGET_FOCUS_RING - 1 - i, 2, 1);
    } else if (win == focus_border.right) {
	render_bevel (win, 0 + WIDGET_FOCUS_RING - focus_border.width, 0 - WIDGET_FOCUS_RING, WIDGET_FOCUS_RING - 1, focus_border.height + WIDGET_FOCUS_RING - 1, j, 0);
	render_bevel (win, i + WIDGET_FOCUS_RING - focus_border.width, i - WIDGET_FOCUS_RING, WIDGET_FOCUS_RING - 1 - i, focus_border.height + WIDGET_FOCUS_RING - 1 - i, 2, 1);
    }
}

static void look_next_render_focus_border (Window win)
{
    render_focus_border_n (win, focus_border.border);
}

static int look_next_get_extra_window_spacing (void)
{
    return 3;
}

static int look_next_get_default_interwidget_spacing (void)
{
    return 1;
}

static int look_next_get_focus_ring_size (void)
{
    return 1;
}

static int look_next_get_window_resize_bar_thickness (void)
{
    return 7;
}

static unsigned long look_next_get_button_flat_color (void)
{
    return color_widget(10);
}

static int look_next_get_switch_size (void)
{
    return 16;
}

static int look_next_get_fielded_textbox_hscrollbar_width (void)
{
    return AUTO_WIDTH;
}

struct look look_next = {
    look_next_get_default_interwidget_spacing,
    look_next_menu_draw,
    look_next_get_menu_item_extents,
    look_next_render_menu_button,
    look_next_render_button,
    look_next_render_bar,
    look_next_render_raised_bevel,
    look_next_render_sunken_bevel,
    look_next_draw_hotkey_understroke,
    look_next_get_default_widget_font,
    look_next_render_text,
    look_next_render_window,
    look_next_render_scrollbar,
    look_next_get_scrollbar_size,
    look_next_init_scrollbar_icons,
    look_next_which_scrollbar_button,
    look_next_scrollbar_handler,
    look_next_get_button_color,
    look_next_get_extra_window_spacing,
    look_next_window_handler,
    look_next_get_focus_ring_size,
    look_next_get_button_flat_color,
    look_next_get_window_resize_bar_thickness,
    look_next_render_switch,
    look_next_get_switch_size,
    look_next_draw_browser,
    look_next_get_file_or_dir,
    look_next_draw_file_list,
    look_next_redraw_file_list,
    look_next_get_file_list_line,
    look_next_search_replace_dialog,
    look_next_edit_render_tidbits,
    look_next_draw_cancel_button,
    look_next_draw_cancel_button,
    look_next_draw_cancel_button,
    look_next_draw_ok_button,
    look_next_render_fielded_textbox_tidbits,
    look_next_render_textbox_tidbits,
    look_next_get_fielded_textbox_hscrollbar_width,
    look_next_render_textinput_tidbits,
    look_next_render_passwordinput_tidbits,
    look_next_render_focus_border,
};

#endif			/* NEXT_LOOK */


