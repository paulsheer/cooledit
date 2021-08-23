#include <config.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#ifdef HAVE_STAT_H
#include <stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "stringtools.h"


static void zfree (unsigned char *s)
{
    memset (s, 0, strlen ((char *) s));
    free (s);
}

/*  Some configuration constants, meassured in points (1/72 inch) */
int postscript_option_page_top = 842;	/* A4 = 297mm x 210mm = 842pt x 595pt */
int postscript_option_page_right_edge = 595;

/*  Set to 1 if your printer doesn't have iso encoding builtin. */
int isoencoding_not_builtin = 0;

int postscript_option_landscape = 0;
int postscript_option_footline_in_landscape = 0;
int postscript_option_line_numbers = 0;
int postscript_option_font_size = 0;
int postscript_option_left_margin = 80;
int postscript_option_right_margin = 40;
int postscript_option_top_margin = 0;
int postscript_option_bottom_margin = 0;
char *postscript_option_font = "Courier";
char *postscript_option_header_font = "Helvetica-Bold";
int postscript_option_header_font_size = 12;
char *postscript_option_line_number_font = "Helvetica";
int postscript_option_line_number_size = 5;
int postscript_option_col_separation = 30;
int postscript_option_header_height = 30;
int postscript_option_show_header = 1;
int postscript_option_wrap_lines = 1;
int postscript_option_columns = 1;
int postscript_option_chars_per_line = 0;
int postscript_option_plain_text = 0;
unsigned char *postscript_option_title = 0;
unsigned char *postscript_option_file = 0;
unsigned char *postscript_option_pipe = 0;
unsigned char *option_continuation_string = 0;
int postscript_option_tab_size = 8;

void postscript_clean (void)
{
    if (postscript_option_title)
	free (postscript_option_title);
    if (postscript_option_file)
	free (postscript_option_file);
    if (postscript_option_pipe)
	free (postscript_option_pipe);
    if (option_continuation_string)
	free (option_continuation_string);
}


void (*postscript_dialog_cannot_open) (unsigned char *) = 0;
int (*postscript_dialog_exists) (unsigned char *) = 0;
unsigned char *(*postscript_get_next_line) (unsigned char *) = 0;

/* utility functions */

unsigned char *ps_string (unsigned char *text)
{
    unsigned char *r, *p, *q;
    int i = 0;
    for (i = 0, p = text; *p; p++, i++) {
	if ((char *) strchr ("\\()", (int) *p))
	    i++;
	else if (*p >= 0177 || *p <= 037)
	    i += 3;
    }
    r = (unsigned char *) malloc (i + 1);
    for (p = text, q = r; *p; p++, q++) {
	if (strchr ("\\()", *p)) {
	    *q++ = '\\';
	    *q = *p;
	} else if (*p >= 0177 || *p <= 037) {
	    sprintf ((char *) q, "\\%03o", (int) *p);
	    q += 3;
	} else {
	    *q = *p;
	}
    }
    *q = 0;
    return r;
}

static unsigned char *get_spaces (unsigned char *old, int l)
{
    if (old)
	zfree (old);
    if (l < 0)
	l = 0;
    old = (unsigned char *) malloc (l + 1);
    memset (old, ' ', l + 1 /* 1 extra to prevent warning */ );
    old[l] = '\0';
    return old;
}

static unsigned char *expand_tabs (unsigned char *old)
{
    int i;
    unsigned char *p, *r, *q, *spaces;
    spaces = get_spaces (0, postscript_option_tab_size);
    for (i = 0, p = old; *p; p++, i++)
	if (*p == '\t')
	    i += 7;
    r = (unsigned char *) malloc (i + 1);
    for (i = 0, q = r, p = old; *p; p++, i++)
	if (*p == '\t') {
	    int n = postscript_option_tab_size - (i % postscript_option_tab_size);
	    strncpy ((char *) q, (char *) spaces, n);
	    q += n;
	    i += n - 1;
	} else
	    *q++ = *p;
    *q = '\0';
    if (old)
	zfree (old);
    zfree (spaces);
    return r;
}

/* globals */

unsigned char filedate[80];
int page_no = 0, line_no = 0, cur_col = 100, total_pages = 0;
double cur_pos = -1;
double top = 0, right_edge = 0, no_columns = 0, home_pos = 0, bottom_pos = 0;
double sep_bar_pos = 0;
unsigned char *box_format;
double font_size = 0.0, line_height = 0.0, char_width = 0.0;
double col_width = 0.0;
int chars_per_line = 0;
/*  The next few entries are from the AFM file for Adobe's postscript_option_font Courier */
double cour_char_width = 600;	/*  The width of each unsigned char in 1000x1000 square */
/* sub underline_position  { -82; }   # Where underline goes relative to baseline */
/* sub underline_thickness {  40; }   # and it's thickness */
static FILE *outfile;
unsigned char *top_box, *bot_box;
unsigned char *header = 0, *title = 0;
int lineNoOut = 0;

static void portrait_header (void)
{
    fprintf (outfile, "%s", top_box);
    fprintf (outfile, "%s", bot_box);
    /*  Then the banner or the filename */
    fprintf (outfile, "F3 SF\n");
    if (header) {
	fprintf (outfile, "%.1f %.1f M(%s)dup stringwidth pop neg 0 rmoveto show\n",
		 (double) right_edge - postscript_option_right_margin,
		 (double) top - postscript_option_top_margin - postscript_option_header_font_size, header);
    }
    /*  Then print date and page number */
    fprintf (outfile, "(%s)%.1f %.1f S\n", filedate, (double) postscript_option_left_margin, (double) postscript_option_bottom_margin + postscript_option_header_font_size * 0.3);
    fprintf (outfile, "%.1f %.1f M(%d)dup stringwidth pop neg 0 rmoveto show\n",
	     (double) right_edge - postscript_option_right_margin,
	     (double) postscript_option_bottom_margin + postscript_option_header_font_size * 0.3,
	     page_no);
}

static void landscape_header (void)
{
    double y;
    if (postscript_option_footline_in_landscape) {
	fprintf (outfile, "%s", bot_box);
	y = (double) postscript_option_bottom_margin + postscript_option_header_font_size * 0.3;
    } else {
	fprintf (outfile, "%s", top_box);
	y = (double) top - postscript_option_top_margin - postscript_option_header_font_size;
    }
    fprintf (outfile, "F3 SF\n");
    if (header) {
	fprintf (outfile, "%.1f %.1f M(%s)dup stringwidth pop 2 div neg 0 rmoveto show\n",
		 (double) (postscript_option_left_margin + right_edge - postscript_option_right_margin) / 2, y, header);
    }
    fprintf (outfile, "(%s)%.1f %.1f S\n", filedate, (double) postscript_option_left_margin, (double) y);
    fprintf (outfile, "%.1f %.1f M(%d)dup stringwidth pop neg 0 rmoveto show\n", (double) right_edge - (double) postscript_option_right_margin, (double) y, page_no);
}

static void end_page (void)
{
    if (total_pages) {
	fprintf (outfile, "page_save restore\n");
	fprintf (outfile, "showpage\n");
    }
}

static void new_page (void)
{
    end_page ();
    page_no++;
    total_pages++;
    fprintf (outfile, "%%%%Page: %d %d\n", page_no, page_no);
    fprintf (outfile, "%%%%BeginPageSetup\n");
    fprintf (outfile, "/page_save save def\n");
    if (postscript_option_landscape)
	fprintf (outfile, "90 rotate 0 -%d translate %% landscape mode\n", postscript_option_page_right_edge);
    if (postscript_option_show_header)
	fprintf (outfile, "0.15 setlinewidth\n");
    fprintf (outfile, "%%%%EndPageSetup\n");
    if (postscript_option_show_header) {
	if (postscript_option_landscape)
	    landscape_header ();
	else
	    portrait_header ();
	fprintf (outfile, "F1 SF\n");
    }
    if (no_columns > 1) {
	int c;
	for (c = 1; c < no_columns; c++) {
	    double x;
	    x = (double) postscript_option_left_margin + (double) c *sep_bar_pos;
	    fprintf (outfile, "%.1f %.1f M %.1f %.1f L stroke\n", (double) x, (double) bottom_pos - 10, (double) x, (double) home_pos + 10);
	}
    }
}

static void printLine (unsigned char *p)
{
    double x = 0;
    if (cur_pos < bottom_pos) {
	cur_pos = home_pos;
	if (cur_col < no_columns) {
	    cur_col++;
	} else {
	    cur_col = 1;
	    new_page ();
	}
    }
    cur_pos -= line_height;
    if (*p) {			/*  no work for empty lines */
	double indent = 0;
	while (*p == ' ') {
	    p++;
	    indent++;
	}
	indent *= char_width;
	p = ps_string (p);
	x = (double) postscript_option_left_margin + (double) (cur_col - 1) * ((double) col_width + postscript_option_col_separation);
	fprintf (outfile, "(%s)%.1f %.1f S\n", p, (double) x + indent, (double) cur_pos);
	zfree (p);
    }
    if (lineNoOut) {
	fprintf (outfile, "F2 SF(%d)%.1f %.1f S F1 SF\n", line_no, (double) x + col_width + 5, (double) cur_pos);
	lineNoOut = 0;
    }
}

static void printLongLines (unsigned char *p)
{
    int maxLength;
    int lineLength = 0;
    int rightMargin = 0;
    unsigned char *spaces = 0;
    unsigned char *temp = 0;
    unsigned char *c;
    int right_space = 0;

    c = option_continuation_string;
    if (!c)
	c = (unsigned char *) "~";
    maxLength = chars_per_line - strlen ((char *) c);
    while (*p) {
	unsigned char *q;
	if (strlen ((char *) p) > maxLength && postscript_option_wrap_lines) {
	    char *t;
	    for (lineLength = maxLength - 1, q = p + maxLength - 1; q >= p && *q != ' '; q--, lineLength--);
	    if (lineLength < maxLength / 2)
		lineLength = maxLength;
	    temp = p + lineLength;
	    t = (char *) malloc (maxLength + right_space + strlen ((char *) c) + 1);
	    strncpy ((char *) t, (char *) p, lineLength);
	    if (!right_space)
		right_space = maxLength - lineLength;
	    spaces = get_spaces (spaces, right_space);
	    strcpy ((char *) t + lineLength, (char *) spaces);
	    strcat ((char *) t, (char *) option_continuation_string);
	    p = (unsigned char *) t;
	} else {
	    temp = (unsigned char *) "";
	    if (strlen ((char *) p) > maxLength)
		p[maxLength] = '\0';
	    p = (unsigned char *) strdup ((char *) p);
	    lineLength = strlen ((char *) p);
	}
	if (rightMargin == 0) {
	    spaces = get_spaces (spaces, 0);
	    rightMargin = lineLength;
	} else {
	    spaces = get_spaces (spaces, rightMargin - lineLength);
	}
	q = (unsigned char *) malloc (strlen ((char *) spaces) + strlen ((char *) p) + 1);
	strcpy ((char *) q, (char *) spaces);
	strcat ((char *) q, (char *) p);
	zfree (p);
	printLine (q);
	zfree (q);
	p = temp;
	maxLength = chars_per_line * 3 / 4;
    }
    zfree (spaces);
}

static void prolog (void)
{
    struct tm *tm;
    time_t t;
    time (&t);
    tm = localtime (&t);
    fprintf (outfile, "%%!PS-Adobe-2.0\n");
    if (title)
	fprintf (outfile, "%%%%Title: %s\n", title);
    fprintf (outfile, "%%%%Creator: Cooledit, text editor and IDE.\n");
    fprintf (outfile, "%%%%CreationDate: (%d %d, %d) (%2d:%02d)\n", tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900, tm->tm_hour, tm->tm_min);
    fprintf (outfile, "%%%%For: %s\n", /* username ******* */ "(unknown)");
    fprintf (outfile, "%%%%Pages: (atend)\n");
    fprintf (outfile, "%%%%DocumentFonts: %s", postscript_option_font);
    if (postscript_option_line_numbers)
	fprintf (outfile, " %s", postscript_option_line_number_font);
    if (postscript_option_show_header)
	fprintf (outfile, " %s", postscript_option_header_font);
    fprintf (outfile, "\n");
    fprintf (outfile, "\
%%%%EndComments\n\
/S{moveto show}bind def\n\
/M/moveto load def\n\
/L/lineto load def\n\
/SF/setfont load def\n\
");
    if (isoencoding_not_builtin) {
	fprintf (outfile, "\
ISOLatin1Encoding where { pop } { ISOLatin1Encoding\n\
[/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n\
/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n\
/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n\
/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/space\n\
/exclam/quotedbl/numbersign/dollar/percent/ampersand/quoteright\n\
/parenleft/parenright/asterisk/plus/comma/minus/period/slash/zero/one\n\
/two/three/four/five/six/seven/eight/nine/colon/semicolon/less/equal\n\
/greater/question/at/A/B/C/D/E/F/G/H/I/J/K/L/M/N/O/P/Q/R/S\n\
/T/U/V/W/X/Y/Z/bracketleft/backslash/bracketright/asciicircum\n\
/underscore/quoteleft/a/b/c/d/e/outfile/g/h/i/j/k/l/m/n/o/p/q/r/s\n\
/t/u/v/w/x/y/z/braceleft/bar/braceright/asciitilde/.notdef/.notdef\n\
/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n\
/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/dotlessi/grave\n\
/acute/circumflex/tilde/macron/breve/dotaccent/dieresis/.notdef/ring\n\
/cedilla/.notdef/hungarumlaut/ogonek/caron/space/exclamdown/cent\n\
/sterling/currency/yen/brokenbar/section/dieresis/copyright/ordfeminine\n\
/guillemotleft/logicalnot/hyphen/registered/macron/degree/plusminus\n\
/twosuperior/threesuperior/acute/mu/paragraph/periodcentered/cedilla\n\
/onesuperior/ordmasculine/guillemotright/onequarter/onehalf/threequarters\n\
/questiondown/Agrave/Aacute/Acircumflex/Atilde/Adieresis/Aring/AE\n\
/Ccedilla/Egrave/Eacute/Ecircumflex/Edieresis/Igrave/Iacute/Icircumflex\n\
/Idieresis/Eth/Ntilde/Ograve/Oacute/Ocircumflex/Otilde/Odieresis\n\
/multiply/Oslash/Ugrave/Uacute/Ucircumflex/Udieresis/Yacute/Thorn\n\
/germandbls/agrave/aacute/acircumflex/atilde/adieresis/aring/ae\n\
/ccedilla/egrave/eacute/ecircumflex/edieresis/igrave/iacute/icircumflex\n\
/idieresis/eth/ntilde/ograve/oacute/ocircumflex/otilde/odieresis/divide\n\
/oslash/ugrave/uacute/ucircumflex/udieresis/yacute/thorn/ydieresis]\n\
def %%ISOLatin1Encoding\n\
} ifelse\n\
");
    }
    fprintf (outfile, "%%%%BeginProcSet: newencode 1.0 0\n\
/NE { %%def\n\
   findfont begin\n\
      currentdict dup length dict begin\n\
         { %%forall\n\
            1 index/FID ne {def} {pop pop} ifelse\n\
         } forall\n\
         /FontName exch def\n\
         /Encoding exch def\n\
         currentdict dup\n\
      end\n\
   end\n\
   /FontName get exch definefont pop\n\
} bind def\n\
%%%%EndProcSet: newencode 1.0 0\n\
%%%%EndProlog\n\
%%%%BeginSetup\n\
");
    fprintf (outfile, "ISOLatin1Encoding /%s-ISO/%s NE\n", postscript_option_font, postscript_option_font);
    if (postscript_option_show_header)
	fprintf (outfile, "ISOLatin1Encoding /%s-ISO/%s NE\n", postscript_option_header_font, postscript_option_header_font);
    fprintf (outfile, "/F1/%s-ISO findfont %.2g scalefont def\n", postscript_option_font, font_size);
    if (postscript_option_line_numbers)
	fprintf (outfile, "/F2/%s findfont %d scalefont def\n", postscript_option_line_number_font, postscript_option_line_number_size);
    if (postscript_option_show_header)
	fprintf (outfile, "/F3/%s-ISO findfont %d scalefont def\n", postscript_option_header_font, postscript_option_header_font_size);
    fprintf (outfile, "F1 SF\n");
    fprintf (outfile, "%%%%EndSetup\n");
}


void postscript_print (void)
{
    unsigned char *p = 0;
    double llx = 0.0, lly = 0.0, urx = 0.0, ury = 0.0;
    struct stat st;
    struct tm *tm = 0;

    outfile = 0;

    page_no = 0;
    line_no = 0;
    cur_col = 100;
    total_pages = 0;
    cur_pos = -1;
    top = 0;
    right_edge = 0;
    no_columns = 0;
    home_pos = 0;
    bottom_pos = 0;
    sep_bar_pos = 0;
    font_size = 0.0;
    line_height = 0.0;
    char_width = 0.0;
    col_width = 0.0;
    chars_per_line = 0;
    cour_char_width = 600;
    lineNoOut = 0;

    if (!option_continuation_string)
	option_continuation_string = (unsigned char *) strdup ("~");

    if (title)
	free (title);
    title = 0;
    if (postscript_option_title)
	title = ps_string (postscript_option_title);

    if (postscript_option_landscape) {
	top = postscript_option_page_right_edge;
	right_edge = postscript_option_page_top;
	postscript_option_left_margin = postscript_option_right_margin;		/*  this is a dirty one */
	no_columns = postscript_option_columns;
	font_size = postscript_option_font_size ? postscript_option_font_size : 7;
	if (postscript_option_footline_in_landscape) {
	    postscript_option_top_margin = 30;
	    postscript_option_bottom_margin = 30;
	    home_pos = top - postscript_option_top_margin - postscript_option_header_height;
	    bottom_pos = postscript_option_bottom_margin + postscript_option_header_height;
	} else {
	    postscript_option_top_margin = 45;
	    postscript_option_bottom_margin = 30;
	    home_pos = top - postscript_option_top_margin - postscript_option_header_height;
	    bottom_pos = postscript_option_bottom_margin;
	}
    } else {
	top = postscript_option_page_top;
	right_edge = postscript_option_page_right_edge;
	no_columns = postscript_option_columns;
	font_size = postscript_option_font_size ? postscript_option_font_size : 10;
	postscript_option_top_margin = 45;
	postscript_option_bottom_margin = 45;
	home_pos = top - postscript_option_top_margin - (postscript_option_show_header ? postscript_option_header_height : 0);
	bottom_pos = postscript_option_bottom_margin + postscript_option_header_height;
    }
    col_width = (double) (right_edge - postscript_option_left_margin - postscript_option_right_margin
     - (no_columns - 1) * postscript_option_col_separation) / no_columns;
    sep_bar_pos = (right_edge - postscript_option_left_margin - postscript_option_right_margin) / no_columns;
    if (!font_size)
	font_size = (double) ((double) col_width / postscript_option_chars_per_line) / (cour_char_width / 1000);
    if (postscript_option_chars_per_line)
	font_size = (double) ((double) col_width / postscript_option_chars_per_line) / (cour_char_width / 1000);
    line_height = font_size * 1.08;
    char_width = cour_char_width * font_size / 1000;
    chars_per_line = (double) col_width / char_width + 1;

/* Compute the box for the page headers. */
    box_format = (unsigned char *) "%.1f %.1f M %.1f %.1f L %.1f %.1f L %.1f %.1f L closepath \n";
    llx = postscript_option_left_margin - 10;
    lly = top - postscript_option_top_margin - postscript_option_header_font_size * 1.3;
    urx = right_edge - postscript_option_right_margin + 10;
    ury = top - postscript_option_top_margin;

    top_box = (unsigned char *) malloc (1024);
    sprintf ((char *) top_box, (char *) box_format, llx, lly, urx, lly, urx, ury, llx, ury);
    strcat ((char *) top_box, "gsave .95 setgray fill grestore stroke\n");

    lly = postscript_option_bottom_margin;
    ury = lly + postscript_option_header_font_size * 1.3;
    bot_box = (unsigned char *) malloc (1024);
    sprintf ((char *) bot_box, (char *) box_format, llx, lly, urx, lly, urx, ury, llx, ury);
    strcat ((char *) bot_box, "gsave .95 setgray fill grestore stroke\n");

/* Open the output pipe or file. */
    if (postscript_option_file) {
	int r;
	r = stat ((char *) postscript_option_file, &st);
	if (!r)
	    if (postscript_dialog_exists)
		if (!(*postscript_dialog_exists) (postscript_option_file))
		    goto postscript_done;
	outfile = fopen ((char *) postscript_option_file, "w+");
	if (!outfile) {
	    if (postscript_dialog_cannot_open)
		(*postscript_dialog_cannot_open) (postscript_option_file);
	    goto postscript_done;
	}
    } else if (postscript_option_pipe) {
	outfile = (FILE *) popen ((char *) postscript_option_pipe, "w");
	if (!outfile) {
	    if (postscript_dialog_cannot_open)
		(*postscript_dialog_cannot_open) (postscript_option_pipe);
	    goto postscript_done;
	}
    }
    if (postscript_option_plain_text) {
	while ((p = (*postscript_get_next_line) (p)))
	    fprintf (outfile, "%s\n", p);
	goto postscript_done;
    }
    prolog ();

    filedate[0] = '\0';
    time (&st.st_mtime);
    tm = localtime (&st.st_mtime);
    get_file_time (filedate, st.st_mtime, 1);

    if (title)
	header = title;
    else
	header = 0;

    page_no = 0;
    line_no = 0;
    while ((p = (*postscript_get_next_line) (p))) {
	line_no++;
	if (postscript_option_line_numbers && ((line_no % 5) == 0))
	    lineNoOut = line_no;
	if (p[0] == 014) {	/*  form feed */
	    strcpy ((char *) p, (char *) p + 1);	/*  chop off first unsigned char */
	    cur_pos = -1;
	    if (!*p)
		p = postscript_get_next_line (p);
	    if (!p)
		break;
	}
	p = expand_tabs (p);	/*  expand tabs */
	if (strlen ((char *) p) <= chars_per_line) {
	    printLine (p);
	} else {
	    printLongLines (p);
	}
    }				/*  while (each line) */
    cur_pos = -1;		/*  this will force a new column next time */
    cur_col = 100;		/*  this will force a new page next time */

    end_page ();
    fprintf (outfile, "%%%%Trailer\n");
    fprintf (outfile, "%%%%Pages: %d\n", total_pages);
  postscript_done:
    free (top_box);
    free (bot_box);
    if (p)
	free (p);
    if (outfile) {
	if (postscript_option_file) {
	    fflush (outfile);
	    fclose (outfile);
	} else {
	    fflush (outfile);
	    pclose (outfile);
	}
    }
    outfile = 0;
    if (title)
	free (title);
    title = 0;
}


