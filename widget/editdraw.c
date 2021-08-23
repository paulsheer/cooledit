/* editdraw.c
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include "edit.h"

#define MAX_LINE_LEN 8192

#if ! defined (MIDNIGHT) && ! defined (GTK)
#include "app_glob.c"
#include "coollocal.h"
#endif

extern int column_highlighting;

extern struct look *look;

#if defined (MIDNIGHT) || defined (GTK)

void status_string (WEdit * edit, char *s, int w, int fill, int font_width)
{
#ifdef MIDNIGHT
    int i;
#endif
    char t[160];		/* 160 just to be sure */
/* The field lengths just prevents the status line from shortening to much */
    sprintf (t, "[%c%c%c%c] %2ld:%3ld+%2ld=%3ld/%3ld - *%-4ld/%4ldb=%3d",
	     edit->mark1 != edit->mark2 ? ( column_highlighting ? 'C' : 'B') : '-',
	     edit->modified ? 'M' : '-', edit->macro_i < 0 ? '-' : 'R',
	     edit->overwrite == 0 ? '-' : 'O',
	     edit->curs_col / font_width, edit->start_line + 1, edit->curs_row,
	     edit->curs_line + 1, edit->total_lines + 1, edit->curs1,
	     edit->last_byte, edit->curs1 < edit->last_byte
	     ? edit_get_byte (edit, edit->curs1) : -1);
#ifdef MIDNIGHT
    sprintf (s, "%.*s", w + 1, t);
    i = strlen (s);
    s[i] = ' ';
    i = w;
    do {
	if (strchr ("+-*=/:b", s[i]))	/* chop off the last word/number */
	    break;
	s[i] = fill;
    } while (i--);
    s[i] = fill;
    s[w] = 0;
#else
    strcpy (s, t);
#endif
}

#endif

#ifdef MIDNIGHT

/* how to get as much onto the status line as is numerically possible :) */
void edit_status (WEdit * edit)
{
    int w, i, t;
    char *s;
    w = edit->widget.cols - (edit->have_frame * 2);
    s = malloc (w + 15);
    if (w < 4)
	w = 4;
    memset (s, ' ', w);
    attrset (SELECTED_COLOR);
    if (w > 4) {
	widget_move (edit, edit->have_frame, edit->have_frame);
	i = w > 24 ? 18 : w - 6;
	i = i < 13 ? 13 : i;
	sprintf (s, "%s", (char *) name_trunc (edit->filename ? edit->filename : "", i));
	i += strlen (s);
	s[strlen (s)] = ' ';
	t = w - 20;
	if (t < 0)
	    t = 0;
	status_string (edit, s + 20, t, ' ', 1);
    } else {
	s[w] = 0;
    }
    printw ("%.*s", w, s);
    attrset (NORMAL_COLOR);
    free (s);
}

#else


void render_status (CWidget * wdt, int expose);

#ifdef GTK

void edit_status (WEdit *edit)
{
    GtkEntry *entry;
    int w, i, t;
    char s[160];
    w = edit->num_widget_columns - 1;
    if (w > 150)
	w = 150;
    if (w < 0)
	w = 0;
    memset (s, 0, w);
    if (w > 1) {
	i = w > 24 ? 18 : w - 6;
	i = i < 13 ? 13 : i;
	sprintf (s, "%s", (char *) name_trunc (edit->filename ? edit->filename : "", i));
	i = strlen (s);
	s[i] = ' ';
	s[i + 1] = ' ';
	t = w - i - 2;
	if (t < 0)
	    t = 0;
	status_string (edit, s + i + 2, t, 0, FONT_MEAN_WIDTH);
    }
    s[w] = 0;
    entry = GTK_ENTRY (edit->widget->status);
    if (strcmp (s, gtk_entry_get_text (entry)))
	gtk_entry_set_text (entry, s);
}

#else

void edit_status (WEdit * edit)
{
    int status_text_width;
    int name_trunc_len = 3;
    long start_mark, end_mark;
    CWidget *wdt;
    char *p;
    char id[33];
    char s[4096];
    char block[32] = "";
    char b[64];
    char mode[65];
    char b_widthcheck[64];
    if (eval_marks (edit, &start_mark, &end_mark))
	end_mark = start_mark = 0;
    if ((COptionsOf (edit->widget) & EDITOR_NO_TEXT))
	return;
    CPushFont ("editor", 0);
    p = edit->filename ? edit->filename : "";
    if (edit->curs1 < edit->last_byte) {
	C_wchar_t c;
	c = edit_get_wide_byte (edit, edit->curs1) & 0x7FFFFFFFUL;
	sprintf (b, "%3lu/\011%lX\033", (unsigned long) c, (unsigned long) c);
	sprintf (b_widthcheck, "%3lu/%lX", (unsigned long) c, (unsigned long) c);
    } else {
	sprintf (b, "\022EOF\033");
	sprintf (b_widthcheck, "EOF");
    }
    CPushFont ("widget", 0);
    sprintf (s, "%s  -rwxrwxrwx  MMMM  %02ld  %-4ld+%2ld=%4ld/%3ld  *%-5ld/%5ldb=%s%c %ld",
                *p ? "" : _("<unnamed>"),
                edit->curs_col / FONT_MEAN_WIDTH, edit->start_line + 1, edit->curs_row,
                edit->curs_line + 1, edit->total_lines + 1, edit->curs1, edit->last_byte, b_widthcheck,
                end_mark - start_mark && !column_highlighting ? ' ' : '\0', end_mark - start_mark);
    status_text_width = CImageTextWidth (s, strlen (s));
    name_trunc_len = (CWidthOf (edit->widget) - 13 - status_text_width) / FONT_MEAN_WIDTH;
    if (name_trunc_len < 8)
        name_trunc_len = 8;
    CPopFont ();
    if (end_mark - start_mark && !column_highlighting)
        sprintf(block, "  \034\001%ld\033\035", end_mark - start_mark);
    pstat_to_mode_string (&edit->stat, mode);
    sprintf (s,
	     "\034%c%s\033\035  \034%s\035  \034%s%s%s%c\035  \034\014%02ld\033\035  \034%-4ld+%2ld=\014%4ld\033/%3ld\035  \034*%-5ld/%5ldb=%s\035%s  \034%s\035",
	     *p ? '\033' : '\003', *p ? (char *) name_trunc (p, name_trunc_len) : _("<unnamed>"),
             mode,
	     end_mark - start_mark || (edit->mark2 == -1
				       && !edit->highlight) ? (column_highlighting ? "\032C\033" :
							       "\001B\033") : "-",
	     edit->modified ? "\012M\033" : "-", edit->macro.macro_i < 0 ? "-" : "\023R\033",
	     edit->overwrite == 0 ? '-' : 'O', edit->curs_col / FONT_MEAN_WIDTH,
	     edit->start_line + 1, edit->curs_row, edit->curs_line + 1, edit->total_lines + 1,
	     edit->curs1, edit->last_byte, b, block, edit->dir);
    strcpy (id, CIdentOf (edit->widget));
    strcat (id, ".text");
    wdt = CIdent (id);
    CStr_free (&wdt->text);
    wdt->text = CStr_dup(s);
    CSetWidgetSize (id, CWidthOf (edit->widget), CHeightOf (wdt));
    render_status (wdt, 0);
    CPopFont ();
}

#endif

#endif


/* boolean */
int cursor_in_screen (WEdit * edit, long row)
{
    if (row < 0 || row >= edit->num_widget_lines)
	return 0;
    else
	return 1;
}

/* returns rows from the first displayed line to the cursor */
int cursor_from_display_top (WEdit * edit)
{
    if (edit->curs1 < edit->start_display)
	return -edit_move_forward (edit, edit->curs1, 0, edit->start_display);
    else
	return edit_move_forward (edit, edit->start_display, 0, edit->curs1);
}

/* returns how far the cursor is out of the screen */
int cursor_out_of_screen (WEdit * edit)
{
    int row = cursor_from_display_top (edit);
    if (row >= edit->num_widget_lines)
	return row - edit->num_widget_lines + 1;
    if (row < 0)
	return row;
    return 0;
}

int edit_get_line_height (WEdit * edit, int row)
{
    int n, i, h = 0;
    struct _book_mark *book_marks[10];
    n = book_mark_query_all (edit, row, book_marks);
    for (i = 0; i < n; i++)
	h += book_marks[i]->height;
    h += FONT_PIX_PER_LINE;
    return h;
}

int edit_get_line_height_and_bookmarks (WEdit * edit, int row, struct _book_mark **book_marks, int *n)
{
    int i, h = 0;
    *n = book_mark_query_all (edit, row, book_marks);
    for (i = 0; i < *n; i++)
	h += book_marks[i]->height;
    h += FONT_PIX_PER_LINE;
    return h;
}

#ifndef MIDNIGHT
int edit_width_of_long_printable (int c);
#endif

/* this scrolls the text so that cursor is on the screen */
void edit_scroll_screen_over_cursor (WEdit * edit)
{
    int p;
    int outby;
    int b_extreme, t_extreme, l_extreme, r_extreme;
    int ch, chw;
    CPushFont("editor", 0);
    r_extreme = EDIT_RIGHT_EXTREME;
    l_extreme = EDIT_LEFT_EXTREME;
    b_extreme = EDIT_BOTTOM_EXTREME;
    t_extreme = EDIT_TOP_EXTREME;
    if (edit->found_len) {
	b_extreme = max (edit->num_widget_lines / 4, b_extreme);
	t_extreme = max (edit->num_widget_lines / 4, t_extreme);
    }
    if (b_extreme + t_extreme + 1 > edit->num_widget_lines) {
	int n;
	n = b_extreme + t_extreme;
	b_extreme = (b_extreme * (edit->num_widget_lines - 1)) / n;
	t_extreme = (t_extreme * (edit->num_widget_lines - 1)) / n;
    }
    if (l_extreme + r_extreme + 1 > edit->num_widget_columns) {
	int n;
	n = l_extreme + t_extreme;
	l_extreme = (l_extreme * (edit->num_widget_columns - 1)) / n;
	r_extreme = (r_extreme * (edit->num_widget_columns - 1)) / n;
    }
    p = edit_get_col (edit);
    edit_update_curs_row (edit);
#ifdef MIDNIGHT
    outby = p + edit->start_col - edit->num_widget_columns + 1 + (r_extreme + edit->found_len);
#else
    ch = edit_get_byte (edit, edit->curs1);
    if (ch == '\t' || ch == '\n')
        chw = FONT_PER_CHAR(' ');
    else
        chw = edit_width_of_long_printable (ch);
    outby = p + edit->start_col - CWidthOf (edit->widget) + 7 + (r_extreme + edit->found_len) * FONT_MEAN_WIDTH + chw;
#endif
    if (outby > 0)
	edit_scroll_right (edit, outby);
#ifdef MIDNIGHT
    outby = l_extreme - p - edit->start_col;
#else
    outby = l_extreme * FONT_MEAN_WIDTH - p - edit->start_col;
#endif
    if (outby > 0)
	edit_scroll_left (edit, outby);

    outby = t_extreme - edit->curs_row;
    if (outby > 0) {
	edit_scroll_upward (edit, outby);
    } else {
        int row, y = 0, max_height;
        max_height = CHeightOf (edit->widget) - EDIT_FRAME_H - b_extreme * FONT_PIX_PER_LINE;
        for (row = edit->curs_line; row >= edit->start_line; row--) {
            y += edit_get_line_height (edit, row);
            if (y > max_height) {
                row++;
                break;
            }
        }
        if (row > edit->start_line)
	    edit_scroll_downward (edit, row - edit->start_line);
    }

    edit_update_curs_row (edit);

    CPopFont();
    return;
}


#ifndef MIDNIGHT

#define CACHE_WIDTH 256
#define CACHE_HEIGHT 128

int EditExposeRedraw = 0;
int EditClear = 0;

/* background colors: marked is refers to mouse highlighting, highlighted refers to a found string. */
unsigned long edit_abnormal_color, edit_marked_abnormal_color;
unsigned long edit_highlighted_color, edit_marked_color;
unsigned long edit_normal_background_color;

/* foreground colors */
unsigned long edit_normal_foreground_color, edit_bold_color;
unsigned long edit_italic_color;

/* cursor color */
unsigned long edit_cursor_color;

void edit_set_foreground_colors (unsigned long normal, unsigned long bold, unsigned long italic)
{
    edit_normal_foreground_color = normal;
    edit_bold_color = bold;
    edit_italic_color = italic;
}

void edit_set_background_colors (unsigned long normal, unsigned long abnormal, unsigned long marked, unsigned long marked_abnormal, unsigned long highlighted)
{
    edit_abnormal_color = abnormal;
    edit_marked_abnormal_color = marked_abnormal;
    edit_marked_color = marked;
    edit_highlighted_color = highlighted;
    edit_normal_background_color = normal;
}

void edit_set_cursor_color (unsigned long c)
{
    edit_cursor_color = c;
}

#else

static void set_color (int font)
{
    attrset (font);
}

#define edit_move(x,y) widget_move(edit, y, x);

static void print_to_widget (WEdit * edit, long row, int start_col, float start_col_real, long end_col, unsigned int line[])
{
    int x = (float) start_col_real + EDIT_TEXT_HORIZONTAL_OFFSET;
    int x1 = start_col + EDIT_TEXT_HORIZONTAL_OFFSET;
    int y = row + EDIT_TEXT_VERTICAL_OFFSET;

    set_color (EDITOR_NORMAL_COLOR);
    edit_move (x1, y);
    hline (' ', end_col + 1 - EDIT_TEXT_HORIZONTAL_OFFSET - x1);

    edit_move (x + FONT_OFFSET_X, y + FONT_OFFSET_Y);
    {
	unsigned int *p = line;
	int textchar = ' ';
	long style;

	while (*p) {
	    style = *p >> 8;
	    textchar = *p & 0xFF;
#ifdef HAVE_SYNTAXH
	    if (!(style & (0xFF - MOD_ABNORMAL - MOD_CURSOR)))
		SLsmg_set_color ((*p & 0x007F0000) >> 16);
#endif
	    if (style & MOD_ABNORMAL)
		textchar = '.';
	    if (style & MOD_HIGHLIGHTED) {
		set_color (EDITOR_BOLD_COLOR);
	    } else if (style & MOD_MARKED) {
		set_color (EDITOR_MARKED_COLOR);
	    }
	    if (style & MOD_UNDERLINED) {
		set_color (EDITOR_UNDERLINED_COLOR);
	    }
	    if (style & MOD_BOLD) {
		set_color (EDITOR_BOLD_COLOR);
	    }
	    addch (textchar);
	    p++;
	}
    }
}

/* b pointer to begining of line */
static void edit_draw_this_line_proportional (WEdit * edit, long b, long row, long start_col, long end_col)
{
    static unsigned int line[MAX_LINE_LEN];
    unsigned int *p = line;
    unsigned int *eol = &line[MAX_LINE_LEN - 1];
    long m1 = 0, m2 = 0, q, c1, c2;
    int col, start_col_real;
    unsigned int c;
    int fg, bg;
    int i, book_mark = -1;

#if 0
    if (!book_mark_query (edit, edit->start_line + row, &book_mark))
	book_mark = -1;
#endif

    edit_get_syntax_color (edit, b - 1, &fg, &bg);
    q = edit_move_forward3 (edit, b, start_col - edit->start_col, 0);
    start_col_real = (col = (int) edit_move_forward3 (edit, b, 0, q)) + edit->start_col;
    c1 = min (edit->column1, edit->column2);
    c2 = max (edit->column1, edit->column2);

    if (col + 16 > -edit->start_col) {
	eval_marks (edit, &m1, &m2);

	if (row <= edit->total_lines - edit->start_line) {
	    while (col <= end_col - edit->start_col) {
		*p = 0;
		if (q == edit->curs1)
		    *p |= MOD_CURSOR * 256;
		if (q >= m1 && q < m2) {
		    if (column_highlighting) {
			int x;
			x = edit_move_forward3 (edit, b, 0, q);
			if (x >= c1 && x < c2)
			    *p |= MOD_MARKED * 256;
		    } else
			*p |= MOD_MARKED * 256;
		}
		if (q == edit->bracket)
		    *p |= MOD_BOLD * 256;
		if (q >= edit->found_start && q < edit->found_start + edit->found_len)
		    *p |= MOD_HIGHLIGHTED * 256;
		c = edit_get_byte (edit, q);
/* we don't use bg for mc - fg contains both */
		if (book_mark == -1) {
		    edit_get_syntax_color (edit, q, &fg, &bg);
		    *p |= fg << 16;
		} else {
		    *p |= book_mark << 16;
		}
		q++;
		switch (c) {
		case '\n':
		    col = end_col - edit->start_col + 1;	/* quit */
		    *(p++) |= ' ';
		    if (p == eol)
			goto done_loop;
		    break;
		case '\t':
		    i = TAB_SIZE - ((int) col % TAB_SIZE);
		    *p |= ' ';
		    c = *(p++) & (0xFFFFFFFF - MOD_CURSOR * 256);
		    if (p == eol)
			goto done_loop;
		    col += i;
		    while (--i) {
			*(p++) = c;
			if (p == eol)
			    goto done_loop;
		    }
		    break;
		case '\r':
		    break;
		default:
		    if (is_printable (c)) {
			*(p++) |= c;
			if (p == eol)
			    goto done_loop;
		    } else {
			*(p++) = '.';
			if (p == eol)
			    goto done_loop;
			*p |= (256 * MOD_ABNORMAL);
		    }
		    col++;
		    break;
		}
	    }
	}
    } else {
	start_col_real = start_col = 0;
    }
  done_loop:
    *p = 0;

    print_to_widget (edit, row, start_col, start_col_real, end_col, line);
}

#endif

#ifdef MIDNIGHT

#define key_pending(x) (!is_idle())

#else

/* #define edit_draw_this_line edit_draw_this_line_proportional */

int option_smooth_scrolling = 0;

static int key_pending (WEdit * edit)
{
    static int flush = 0, line = 0;
#ifdef GTK
    /* ******* */
#else
    if (!edit) {
	flush = line = 0;
    } else if (!(edit->force & REDRAW_COMPLETELY) && !EditExposeRedraw && !option_smooth_scrolling) {
/* this flushes the display in logarithmic intervals - so both fast and
   slow machines will get good performance vs nice-refreshing */
	if ((1 << flush) == ++line) {
	    flush++;
	    return CKeyPending ();
	}
    }
#endif
    return 0;
}

#endif

static int edit_height_delta (WEdit * edit, int row1, int row2)
{
    int y;
    for (y = 0; row1 < row2; row1++)
        y += edit_get_line_height (edit, row1);
    return y;
}

static int edit_row_to_ypixel (WEdit * edit, int row_search)
{
    int row, y;
    for (y = 0, row = 0; row < row_search; row++)
        y += edit_get_line_height (edit, edit->start_line + row);
    return y + EDIT_TEXT_VERTICAL_OFFSET;
}

/* b for pointer to begining of line */
static int edit_draw_this_char (WEdit * edit, long curs, long row)
{
    int b = edit_bol (edit, curs);
#ifdef MIDNIGHT
    edit_draw_this_line_proportional (edit, b, row, 0, edit->num_widget_columns - 1);
#else
    struct _book_mark *book_marks[10];
    int n;
    n = book_mark_query_all (edit, edit->start_line + row, book_marks);
    return edit_draw_this_line_proportional (edit, b, row, edit_row_to_ypixel (edit, row), 0, CWidthOf (edit->widget), book_marks, n);
#endif
}

void edit_draw_proportional_invalidate (int row_start, int row_end, int x_max);

/* cursor must be in screen for other than REDRAW_PAGE passed in force */
static int render_edit_text (WEdit * edit, long start_y, long start_x, long end_y,
		       long end_x)
{
    int drawn_extents_y = 0;
    static int prev_curs_row = 0;
    static long prev_curs = 0;
    static long prev_start = -1;

#ifndef MIDNIGHT
    static unsigned long prev_win = 0;
#endif

    int force = edit->force;

    CPushFont ("editor", 0);

#ifndef MIDNIGHT
    key_pending (0);
#endif

/*
   if the position of the page has not moved then we can draw the cursor character only.
   This will prevent line flicker when using arrow keys.
 */
    if ((!(force & REDRAW_CHAR_ONLY)) || (force & REDRAW_PAGE)
#ifndef MIDNIGHT
#ifdef GTK
	|| prev_win != ((GdkWindowPrivate *) CWindowOf (edit->widget)->text_area)->xwindow
#else
	|| prev_win != CWindowOf (edit->widget)
#endif
#endif
	) {
	int time_division = 5;
        int threshold;
        threshold = CHeightOf(edit->widget) / 2;
#if 0
	if (CPending ())
	    time_division--;
#endif
#ifndef MIDNIGHT
	if (prev_start < 0)
	    prev_start = edit->start_line;
	if (option_smooth_scrolling && time_division > 0 && prev_win == CWindowOf (edit->widget) && !edit->screen_modified) {
	    int t;
	    int pos1, pos2, h;
	    if (edit->start_line > prev_start) {
                h = edit_height_delta (edit, prev_start, edit->start_line);
                if (h <= threshold) {
		    edit_draw_proportional (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		    for (pos2 = 0, t = 0; t < time_division; t++) {
		        int move, finish_up = 0;
		        pos1 = h * (t + 1) / time_division;
		        move = pos1 - pos2;
		        if (CCheckWindowEvent (0, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | KeyPressMask, 0)) {
                            if (t == time_division - 1) {
                                finish_up = 1;
                            } else {
                                continue;
                            }
                        }
		        pos2 += move;
		        XCopyArea (CDisplay, edit->widget->winid, edit->widget->winid, CGC,
			        EDIT_TEXT_HORIZONTAL_OFFSET,
			        EDIT_TEXT_VERTICAL_OFFSET + move,
			        edit->widget->width - EDIT_FRAME_W,
			        edit->widget->height - (EDIT_FRAME_H - 1) - move,
			        EDIT_TEXT_HORIZONTAL_OFFSET, EDIT_TEXT_VERTICAL_OFFSET);
		        XClearArea (CDisplay, edit->widget->winid, EDIT_TEXT_HORIZONTAL_OFFSET,
				    edit->widget->height - (EDIT_FRAME_H - 1) +
				    EDIT_TEXT_VERTICAL_OFFSET - move,
				    edit->widget->width - EDIT_FRAME_W, move, 0);
		        XFlush (CDisplay);
                        if (finish_up)
                            break;
		        pause ();
		    }
	        }
	    } else if (edit->start_line < prev_start) {
                h = edit_height_delta (edit, edit->start_line, prev_start);
                if (h <= threshold) {
		    edit_draw_proportional (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		    for (pos2 = 0, t = 0; t < time_division; t++) {
		        int move, finish_up = 0;
		        pos1 = h * (t + 1) / time_division;
		        move = pos1 - pos2;
		        if (CCheckWindowEvent (0, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | KeyPressMask, 0)) {
                            if (t == time_division - 1) {
                                finish_up = 1;
                            } else {
                                continue;
                            }
                        }
		        pos2 += move;
		        XCopyArea (CDisplay, edit->widget->winid, edit->widget->winid, CGC,
			        EDIT_TEXT_HORIZONTAL_OFFSET, EDIT_TEXT_VERTICAL_OFFSET,
			        edit->widget->width - EDIT_FRAME_W,
			        edit->widget->height - (EDIT_FRAME_H - 1) -
			        move, EDIT_TEXT_HORIZONTAL_OFFSET, EDIT_TEXT_VERTICAL_OFFSET + move);
		        XClearArea (CDisplay, edit->widget->winid, EDIT_TEXT_HORIZONTAL_OFFSET,
				    EDIT_TEXT_VERTICAL_OFFSET,
				    edit->widget->width - EDIT_FRAME_W, move, 0);
		        XFlush (CDisplay);
                        if (finish_up)
                            break;
		        pause ();
		    }
	        }
	    }
	}
#endif
	if (!(force & REDRAW_IN_BOUNDS)) {	/* !REDRAW_IN_BOUNDS means to ignore bounds and redraw whole rows */
	    start_y = 0;
	    end_y = CHeightOf (edit->widget);
	    start_x = 0;
#ifdef MIDNIGHT
#error
#else
	    end_x = CWidthOf (edit->widget);
#endif
	}

        {
            long y = 0, curs_row, row;
            long b = -1;
            curs_row = edit->curs_row;
            for (y = EDIT_TEXT_VERTICAL_OFFSET, row = 0;;) {
                int h, n = 0;
                struct _book_mark *book_marks[10];
                h = edit_get_line_height_and_bookmarks (edit, edit->start_line + row, book_marks, &n);

                if (
                        ((force & REDRAW_PAGE)           && (y + h >= start_y))         ||
                        ((force & REDRAW_BEFORE_CURSOR)  && row < curs_row)             ||
                        ((force & REDRAW_LINE_ABOVE)     && row == curs_row - 1)        ||
                        (!(force & REDRAW_PAGE)          && row == curs_row)            ||
                        ((force & REDRAW_LINE_BELOW)     && row == curs_row + 1)        ||
                        ((force & REDRAW_AFTER_CURSOR)   && row > curs_row)) {
                    if (b == -1)
                        b = edit_move_forward (edit, edit->start_display, row, 0);
                }

                if (b != -1) {
                    int ye;
                    if (key_pending (edit))
                        goto exit_render;
                    ye = edit_draw_this_line_proportional (edit, b, row, y, start_x, end_x, book_marks, n);
                    if (drawn_extents_y < ye)
                        drawn_extents_y = ye;
                    b = edit_move_forward (edit, b, 1, 0);
                }

                y += h;
                row++;

                if (y >= CHeightOf (edit->widget) - EDIT_FRAME_H + EDIT_TEXT_VERTICAL_OFFSET) {
                    edit_draw_proportional_invalidate (row, edit->num_widget_lines, CWidthOf (edit->widget));
                    break;
                }

                if (
                        ((force & REDRAW_PAGE)           && y <= end_y)                              ||
                        ((force & REDRAW_BEFORE_CURSOR)  && row < curs_row)                          ||
                        ((force & REDRAW_LINE_ABOVE)     && row <= curs_row - 1)                     ||
                        (!(force & REDRAW_PAGE)          && row <= curs_row)                         ||
                        ((force & REDRAW_LINE_BELOW)     && row <= curs_row + 1)                     ||
                        ((force & REDRAW_AFTER_CURSOR))) {
                    continue;
                }

                break;
	    }
	}
    } else {
        int y;
	if (prev_curs_row < edit->curs_row) {	/* with the new text highlighting, we must draw from the top down */
	    y = edit_draw_this_char (edit, prev_curs, prev_curs_row);
            if (drawn_extents_y < y)
                drawn_extents_y = y;
	    y = edit_draw_this_char (edit, edit->curs1, edit->curs_row);
            if (drawn_extents_y < y)
                drawn_extents_y = y;
	} else {
	    y = edit_draw_this_char (edit, edit->curs1, edit->curs_row);
            if (drawn_extents_y < y)
                drawn_extents_y = y;
	    y = edit_draw_this_char (edit, prev_curs, prev_curs_row);
            if (drawn_extents_y < y)
                drawn_extents_y = y;
	}
    }

    edit->force = 0;

    prev_curs_row = edit->curs_row;
    prev_curs = edit->curs1;
#ifndef MIDNIGHT
#ifdef GTK
    prev_win = ((GdkWindowPrivate *) CWindowOf (edit->widget)->text_area)->xwindow;
#else
    prev_win = CWindowOf (edit->widget);
#endif
#endif
  exit_render:
    edit->screen_modified = 0;
    prev_start = edit->start_line;
    CPopFont ();

    return drawn_extents_y;
}



#ifndef MIDNIGHT

void edit_convert_expose_to_area (XExposeEvent * xexpose, int *y1, int *x1, int *y2, int *x2)
{
    *x1 = xexpose->x - EDIT_TEXT_HORIZONTAL_OFFSET;
    *y1 = xexpose->y - EDIT_TEXT_VERTICAL_OFFSET;
    *x2 = xexpose->x + xexpose->width + EDIT_TEXT_HORIZONTAL_OFFSET + 3;
    *y2 = xexpose->y + xexpose->height + EDIT_TEXT_VERTICAL_OFFSET + 3;
}

#ifdef GTK

void edit_render_tidbits (GtkEdit * edit)
{
    gtk_widget_draw_focus (GTK_WIDGET (edit));
}

#else

void edit_render_tidbits (CWidget * wdt)
{
    (*look->edit_render_tidbits) (wdt);
}

#endif

void edit_set_space_width (int s);
extern int option_long_whitespace;

#endif

void edit_render (WEdit * edit, int page, int y_start, int x_start, int y_end, int x_end)
{
    int f = 0, drawn_extents_y;
    if (y_start < 0)
	y_start = 0;
    if (y_end < y_start)
	return;
    if (x_start < 0)
	x_start = 0;
    if (x_end < x_start)
	return;
    if (y_end > edit->widget->height)
	y_end = edit->widget->height;
    if (x_end > edit->widget->width)
	x_end = edit->widget->width;
#ifdef GTK
    GtkEdit *win;
#endif
    if (page)			/* if it was an expose event, 'page' would be set */
	edit->force |= REDRAW_PAGE | REDRAW_IN_BOUNDS;
    f = edit->force & (REDRAW_PAGE | REDRAW_COMPLETELY);

#ifdef MIDNIGHT
    if (edit->force & REDRAW_COMPLETELY)
	redraw_labels (edit->widget.parent, (Widget *) edit);
#else
    if (option_long_whitespace)
	edit_set_space_width (FONT_PER_CHAR(' ') * 2);
    else
	edit_set_space_width (FONT_PER_CHAR(' '));
#ifdef GTK
    win = (GtkEdit *) edit->widget;
#endif
    edit_set_foreground_colors (
				 color_palette (option_editor_fg_normal),
				   color_palette (option_editor_fg_bold),
				   color_palette (option_editor_fg_italic)
	);
    edit_set_background_colors (
				 color_palette (option_editor_bg_normal),
			       color_palette ((option_editor_bg_normal == option_editor_bg_abnormal2) ? (option_editor_bg_normal ? 0 : 1) : option_editor_bg_abnormal2),
				 color_palette (option_editor_bg_marked),
			color_palette (option_editor_bg_marked_abnormal),
			     color_palette (option_editor_bg_highlighted)
	);
    edit_set_cursor_color (
			      color_palette (option_editor_fg_cursor)
	);

#ifdef GTK
    /* *********** */
#else
    if (!EditExposeRedraw)
	set_cursor_position (0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif
#endif
    drawn_extents_y = render_edit_text (edit, y_start, x_start, y_end, x_end);
    if (edit->force)		/* edit->force != 0 means a key was pending and the redraw 
				   was halted, so next time we must redraw everything in case stuff
				   was left undrawn from a previous key press */
	edit->force |= REDRAW_PAGE | f;
#ifndef MIDNIGHT
    if (f || drawn_extents_y >= CHeightOf (edit->widget) - (EDIT_FRAME_H - EDIT_TEXT_VERTICAL_OFFSET - 1))
	edit_render_tidbits (edit->widget);
#endif
}

#ifndef MIDNIGHT
void edit_render_expose (WEdit * edit, XExposeEvent * xexpose)
{
    CPushFont ("editor", 0);
    EditExposeRedraw = 1;
    edit->num_widget_lines = (CHeightOf (edit->widget) - EDIT_FRAME_H) / FONT_PIX_PER_LINE;
    edit->num_widget_columns = (CWidthOf (edit->widget) - EDIT_FRAME_W) / FONT_MEAN_WIDTH;
    if (edit->force & (REDRAW_PAGE | REDRAW_COMPLETELY)) {
	edit->force |= REDRAW_PAGE | REDRAW_COMPLETELY;
	edit_render_keypress (edit);
    } else {
        int y_start, x_start, y_end, x_end;
	edit_convert_expose_to_area (xexpose, &y_start, &x_start, &y_end, &x_end);
	edit_render (edit, 1, y_start, x_start, y_end, x_end);
    }
    CPopFont ();
    EditExposeRedraw = 0;
}

void edit_render_keypress (WEdit * edit)
{
    CPushFont ("editor", 0);
    edit_render (edit, 0, 0, 0, 0, 0);
    CPopFont ();
}

#else

void edit_render_keypress (WEdit * edit)
{
    edit_render (edit, 0, 0, 0, 0, 0);
}

#endif
