/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* initapp.c - initialise X Server connection, X application, and widget library
   Copyright (C) 1996-2022 Paul Sheer
 */


/* setup application */

/*
   Colormap allocation:

   A colormap is allocated as follows into the array color_pixels:
   ('i' refers to color_pixels[i])

   These are allocated in sequential palette cells, hence
   (for pseudocolor only) color_pixels[j + i] = color_pixels[j] + i,
   for j = 0,16,43. Obviously in TrueColor this is not true.

   ,----------+------------------------------------------.
   |    i     |   colors                                 |
   +----------+------------------------------------------+
   |  0-15    | 16 levels of the widget colors that      |
   |          | make up button bevels etc. Starting from |
   |          | (i=0) black (for shadowed bevels), up    |
   |          | to (i=15) bright highlighted bevels      |
   |          | those facing up-to-the-left. These       |
   |          | are sequential (see next block).         |
   +----------+------------------------------------------+
   |  16-42   | 3^3 combinations of RGB, vis. (0,0,0),   |
   |          | (0,0,127), (0,0,255), (0,127,0), ...     |
   |          | ... (255,255,255).                       |
   +----------+------------------------------------------+
   |  43-106  | 64 levels of grey. (optional)            |
   +----------+------------------------------------------+
   |  107->   | For other colors. (Not used at present)  |
   `----------+------------------------------------------'
 */

/* Thence macros are defined in coolwidgets.h for color lookup */


#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>

#define  DEF_APP_GLOB		/* so that globals get defined not externed */

#include "coolwidget.h"
#include "xim.h"
#include "stringtools.h"
#include "remotefs.h"
#include "remotefspassword.h"
#include "aafont.h"
#include "childhandler.h"
#include "remotefs.h"

#include <signal.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif


static int verbose_operation = 0;
CInitData *given = 0;


/* defaults */
#define DEFAULT_DISPLAY			NULL
#define DEFAULT_GEOM			NULL
#define DEFAULT_BG_COLOR		0 /* not used *************/ 
#define DEFAULT_WIDGET_COLOR_R		"0.9"
#define DEFAULT_WIDGET_COLOR_G		"1.1"
#define DEFAULT_WIDGET_COLOR_B		"1.4"

#define DEFAULT_BDWIDTH         1

struct look *look = 0;
#ifdef NEXT_LOOK
extern struct look look_next;
#endif
extern struct look look_cool;
extern struct look look_gtk;

struct resource_param {
    char *name;
    char **value;
};

static char *init_display = DEFAULT_DISPLAY;
char *init_geometry = DEFAULT_GEOM;
char *init_font = 0;
char *init_widget_font = 0;
char *init_8bit_term_font = 0;
char *init_bg_color = DEFAULT_BG_COLOR; /* not used *************/ 
char *init_fg_color_red = DEFAULT_WIDGET_COLOR_R;
char *init_fg_color_green = DEFAULT_WIDGET_COLOR_G;
char *init_fg_color_blue = DEFAULT_WIDGET_COLOR_B;
#ifndef NEXT_LOOK
char *init_look = "gtk";
#else
char *init_look = "next";
#endif

Atom ATOM_ICCCM_P2P_CLIPBOARD;
Atom ATOM_UTF8_STRING;
Atom ATOM_WM_PROTOCOLS, ATOM_WM_DELETE_WINDOW;
Atom ATOM_WM_NAME, ATOM_WM_NORMAL_HINTS, ATOM_WM_TAKE_FOCUS;

/* Resources */

struct resource_param resources[] =
{
    {"display", &init_display},
    {"geometry", &init_geometry},
    {"font", &init_font},
    {"widget_font", &init_widget_font},
    {"fg_red", &init_fg_color_red},
    {"fg_blue", &init_fg_color_blue},
    {"fg_green", &init_fg_color_green},
    {0, 0}
};

static void alloccolorerror (void)
{E_
/* Translations in initapp.c are of a lower priority than other files, since they only output when cooledit is run in verbose mode */
    fprintf (stderr, _ ("Cannot allocate colors. Could be to many applications\ntrying to use the colormap. If closing other apps doesn't\nhelp, then your graphics hardware may be inadequite.\n"));
    exit (1);
}

static void init_widgets (void)
{E_
    int i;
    last_widget = 1;		/*widget[0] is never used since index 0 is used
				   to indicate an error message */
    for (i = 0; i < MAX_NUMBER_OF_WIDGETS; i++)
	CIndex (i) = NULL;	/*initialise */
}

static void open_display (char *app_name, int wait_for_display)
{E_
    if (wait_for_display) {
	CDisplay = 0;
	while (!(CDisplay = XOpenDisplay (init_display)))
	    sleep (1);
    } else {
	if ((CDisplay = XOpenDisplay (init_display)) == NULL) {
	    fprintf (stderr, _ ("%s: can't open display named \"%s\"\n"),
		     app_name, XDisplayName (init_display));
	    exit (1);
	}
    }
    remotefs_set_display_log_for_wtmp (XDisplayName (init_display));
    CRoot = DefaultRootWindow (CDisplay);
    if (verbose_operation)
	printf (_ ("Opened display \"%s\"\n"), XDisplayName (init_display));
}

static void get_resources (void)
{E_
    int i;
    char *type;
    XrmValue value;
    XrmDatabase rdb;
    XrmInitialize ();
    rdb = XrmGetFileDatabase (catstrs (getenv ("HOME"), "/.Xdefaults", NULL));
    if (rdb != NULL) {
	for (i = 0; resources[i].name; i++) {
	    char *rname = catstrs (CAppName, "*", resources[i].name, NULL);
	    if (XrmGetResource (rdb, rname, rname,
				&type, &value)) {
		*resources[i].value = value.addr;
	    }
	}
    }
}

static enum font_encoding editor_encoding = FONT_ENCODING_UTF8;

/* returns non-zero on changed */
int set_editor_encoding (int utf_encoding, int locale_encoding)
{E_
    enum font_encoding last_encoding;

    last_encoding = editor_encoding;

    if (utf_encoding)
        editor_encoding = FONT_ENCODING_UTF8;
    else if (locale_encoding)
        editor_encoding = FONT_ENCODING_LOCALE;
    else
        editor_encoding = FONT_ENCODING_8BIT;

    xdnd_set_dnd_mime_types (editor_encoding == FONT_ENCODING_UTF8);
    return last_encoding != editor_encoding;
}

enum font_encoding get_editor_encoding (void)
{E_
    return editor_encoding;
}

#ifndef NO_TTF
#define ALL_TTF_RESIZABLE_FONTS \
        "NotoSans-Regular.ttf," \
        "NotoSansSymbols-Regular.ttf," \
        "NotoSansSymbols2-Regular.ttf," \
        "NotoSansMath-Regular.ttf," \
        "NotoMusic-Regular.ttf," \
        "NotoColorEmoji.ttf," \
        "NotoLoopedLao-Regular.ttf," \
        "NotoNastaliqUrdu-Regular.ttf," \
        "NotoSansAdlam-Regular.ttf," \
        "NotoSansAnatolianHieroglyphs-Regular.ttf," \
        "NotoSansArabic-Regular.ttf," \
        "NotoSansArmenian-Regular.ttf," \
        "NotoSansAvestan-Regular.ttf," \
        "NotoSansBalinese-Regular.ttf," \
        "NotoSansBamum-Regular.ttf," \
        "NotoSansBassaVah-Regular.ttf," \
        "NotoSansBatak-Regular.ttf," \
        "NotoSansBengali-Regular.ttf," \
        "NotoSansBhaiksuki-Regular.ttf," \
        "NotoSansBrahmi-Regular.ttf," \
        "NotoSansBuginese-Regular.ttf," \
        "NotoSansBuhid-Regular.ttf," \
        "NotoSansCanadianAboriginal-Regular.ttf," \
        "NotoSansCarian-Regular.ttf," \
        "NotoSansCaucasianAlbanian-Regular.ttf," \
        "NotoSansChakma-Regular.ttf," \
        "NotoSansCham-Regular.ttf," \
        "NotoSansCherokee-Regular.ttf," \
        "NotoSansChorasmian-Regular.ttf," \
        "NotoSansCoptic-Regular.ttf," \
        "NotoSansCuneiform-Regular.ttf," \
        "NotoSansCypriot-Regular.ttf," \
        "NotoSansCyproMinoan-Regular.ttf," \
        "NotoSansDeseret-Regular.ttf," \
        "NotoSansDevanagari-Regular.ttf," \
        "NotoSansDuployan-Regular.ttf," \
        "NotoSansEgyptianHieroglyphs-Regular.ttf," \
        "NotoSansElbasan-Regular.ttf," \
        "NotoSansElymaic-Regular.ttf," \
        "NotoSansEthiopic-Regular.ttf," \
        "NotoSansGeorgian-Regular.ttf," \
        "NotoSansGlagolitic-Regular.ttf," \
        "NotoSansGothic-Regular.ttf," \
        "NotoSansGrantha-Regular.ttf," \
        "NotoSansGujarati-Regular.ttf," \
        "NotoSansGunjalaGondi-Regular.ttf," \
        "NotoSansGurmukhi-Regular.ttf," \
        "NotoSansHanifiRohingya-Regular.ttf," \
        "NotoSansHanunoo-Regular.ttf," \
        "NotoSansHatran-Regular.ttf," \
        "NotoSansHebrew-Regular.ttf," \
        "NotoSansImperialAramaic-Regular.ttf," \
        "NotoSansIndicSiyaqNumbers-Regular.ttf," \
        "NotoSansInscriptionalPahlavi-Regular.ttf," \
        "NotoSansInscriptionalParthian-Regular.ttf," \
        "NotoSansJavanese-Regular.ttf," \
        "NotoSansKaithi-Regular.ttf," \
        "NotoSansKannada-Regular.ttf," \
        "NotoSansKayahLi-Regular.ttf," \
        "NotoSansKharoshthi-Regular.ttf," \
        "NotoSansKhmer-Regular.ttf," \
        "NotoSansKhudawadi-Regular.ttf," \
        "NotoSansLepcha-Regular.ttf," \
        "NotoSansLimbu-Regular.ttf," \
        "NotoSansLinearA-Regular.ttf," \
        "NotoSansLinearB-Regular.ttf," \
        "NotoSansLisu-Regular.ttf," \
        "NotoSansLycian-Regular.ttf," \
        "NotoSansLydian-Regular.ttf," \
        "NotoSansMahajani-Regular.ttf," \
        "NotoSansMalayalam-Regular.ttf," \
        "NotoSansMandaic-Regular.ttf," \
        "NotoSansManichaean-Regular.ttf," \
        "NotoSansMarchen-Regular.ttf," \
        "NotoSansMasaramGondi-Regular.ttf," \
        "NotoSansMedefaidrin-Regular.ttf," \
        "NotoSansMeeteiMayek-Regular.ttf," \
        "NotoSansMendeKikakui-Regular.ttf," \
        "NotoSansMeroitic-Regular.ttf," \
        "NotoSansMiao-Regular.ttf," \
        "NotoSansModi-Regular.ttf," \
        "NotoSansMongolian-Regular.ttf," \
        "NotoSansMono-Regular.ttf," \
        "NotoSansMro-Regular.ttf," \
        "NotoSansMultani-Regular.ttf," \
        "NotoSansMyanmar-Regular.ttf," \
        "NotoSansNKo-Regular.ttf," \
        "NotoSansNabataean-Regular.ttf," \
        "NotoSansNandinagari-Regular.ttf," \
        "NotoSansNewTaiLue-Regular.ttf," \
        "NotoSansNewa-Regular.ttf," \
        "NotoSansOgham-Regular.ttf," \
        "NotoSansOlChiki-Regular.ttf," \
        "NotoSansOldHungarian-Regular.ttf," \
        "NotoSansOldItalic-Regular.ttf," \
        "NotoSansOldNorthArabian-Regular.ttf," \
        "NotoSansOldPermic-Regular.ttf," \
        "NotoSansOldPersian-Regular.ttf," \
        "NotoSansOldSogdian-Regular.ttf," \
        "NotoSansOldSouthArabian-Regular.ttf," \
        "NotoSansOldTurkic-Regular.ttf," \
        "NotoSansOriya-Regular.ttf," \
        "NotoSansOsage-Regular.ttf," \
        "NotoSansOsmanya-Regular.ttf," \
        "NotoSansPahawhHmong-Regular.ttf," \
        "NotoSansPalmyrene-Regular.ttf," \
        "NotoSansPauCinHau-Regular.ttf," \
        "NotoSansPhagsPa-Regular.ttf," \
        "NotoSansPhoenician-Regular.ttf," \
        "NotoSansPsalterPahlavi-Regular.ttf," \
        "NotoSansRejang-Regular.ttf," \
        "NotoSansRunic-Regular.ttf," \
        "NotoSansSamaritan-Regular.ttf," \
        "NotoSansSaurashtra-Regular.ttf," \
        "NotoSansSharada-Regular.ttf," \
        "NotoSansShavian-Regular.ttf," \
        "NotoSansSiddham-Regular.ttf," \
        "NotoSansSignWriting-Regular.ttf," \
        "NotoSansSinhala-Regular.ttf," \
        "NotoSansSogdian-Regular.ttf," \
        "NotoSansSoraSompeng-Regular.ttf," \
        "NotoSansSoyombo-Regular.ttf," \
        "NotoSansSundanese-Regular.ttf," \
        "NotoSansSylotiNagri-Regular.ttf," \
        "NotoSansSyriac-Regular.ttf," \
        "NotoSansTagalog-Regular.ttf," \
        "NotoSansTagbanwa-Regular.ttf," \
        "NotoSansTaiLe-Regular.ttf," \
        "NotoSansTaiTham-Regular.ttf," \
        "NotoSansTaiViet-Regular.ttf," \
        "NotoSansTakri-Regular.ttf," \
        "NotoSansTamil-Regular.ttf," \
        "NotoSansTamilSupplement-Regular.ttf," \
        "NotoSansTangsa-Regular.ttf," \
        "NotoSansTelugu-Regular.ttf," \
        "NotoSansThaana-Regular.ttf," \
        "NotoSansTifinagh-Regular.ttf," \
        "NotoSansTirhuta-Regular.ttf," \
        "NotoSansUgaritic-Regular.ttf," \
        "NotoSansVai-Regular.ttf," \
        "NotoSansVithkuqi-Regular.ttf," \
        "NotoSansWancho-Regular.ttf," \
        "NotoSansWarangCiti-Regular.ttf," \
        "NotoSansYi-Regular.ttf," \
        "NotoSansZanabazarSquare-Regular.ttf," \
        "NotoSerifAhom-Regular.ttf," \
        "NotoSerifDivesAkuru-Regular.ttf," \
        "NotoSerifDogra-Regular.ttf," \
        "NotoSerifKhojki-Regular.ttf," \
        "NotoSerifMakasar-Regular.ttf," \
        "NotoSerifNyiakengPuachueHmong-Regular.ttf," \
        "NotoSerifOldUyghur-Regular.ttf," \
        "NotoSerifTangut-Regular.ttf," \
        "NotoSerifTibetan-Regular.ttf," \
        "NotoSerifToto-Regular.ttf," \
        "NotoSerifYezidi-Regular.ttf," \
        "NotoTraditionalNushu-Regular.ttf," \
        "NotoSansHK-Regular.otf," \
        "NotoSansJP-Regular.otf," \
        "NotoSansKR-Regular.otf," \
        "NotoSansTC-Regular.otf," \
        "NotoSansSC-Regular.otf"
#endif

const char *get_default_editor_font (void)
{E_
#ifndef NO_TTF
    return "8x13B.pcf.gz," ALL_TTF_RESIZABLE_FONTS;
#else
    return "-*-fixed-bold-r-*--13-120-*-*-*-80-*";
#endif
}

const char *get_default_editor_font_large (void)
{E_
#ifndef NO_TTF
    return "9x15B.pcf.gz," ALL_TTF_RESIZABLE_FONTS;
#else
    return "-*-fixed-bold-r-*--15-140-*-*-*-*-*";
#endif
}

const char *get_default_widget_font (void)
{E_
#ifndef NO_TTF
    return ALL_TTF_RESIZABLE_FONTS ":14";
#else
    return "-*-helvetica-bold-r-*--14-*-*-*-*-*-*";
#endif
}

const char *get_default_8bit_term_font (void)
{E_
#ifndef NO_TTF
    return "8x13B-ISO8859-1.pcf.gz";
#else
    return "-*-fixed-bold-r-*--13-120-*-*-*-80-*";
#endif
}

const char *get_default_8bit_term_font_large (void)
{E_
#ifndef NO_TTF
    return "9x15B-ISO8859-1.pcf.gz";
#else
    return "-*-fixed-bold-r-*--15-120-*-*-*-*-*";
#endif
}

const char *get_default_bookmark_font (void)
{E_
#ifndef NO_TTF
    return ALL_TTF_RESIZABLE_FONTS ":13";
#else
    return "-*-helvetica-bold-r-*--13-*-*-*-*-*-*";
#endif
}

static void init_load_font (void)
{E_
    static enum font_encoding widget_encoding = FONT_ENCODING_UTF8;
    static enum font_encoding bookmark_encoding = FONT_ENCODING_UTF8;
    static enum font_encoding rxvt_8bit_encoding = FONT_ENCODING_8BIT;
    static enum font_encoding rxvt_encoding = FONT_ENCODING_UTF8;
    char *f;

    CFontLazy ("editor", init_font, "-*-fixed-bold-r-*--13-120-*-*-*-80-*", &editor_encoding);
    CFontLazy ("rxvt", init_font, "-*-fixed-bold-r-*--13-120-*-*-*-80-*", &rxvt_encoding);
    CFontLazy ("rxvt8bit", init_8bit_term_font, "-*-fixed-bold-r-*--13-120-*-*-*-80-*", &rxvt_8bit_encoding);

    f = CMalloc (strlen (init_widget_font) + 256);
    sprintf (f, init_widget_font, 14);
    CFontLazy ("widget", f, "-bitstream-*-medium-r-normal--14-*-*-*-*-*-*-*", &widget_encoding);
    free(f);

    CFontLazy ("bookmark", get_default_bookmark_font(), NULL, &bookmark_encoding);
}

static void visual_comments (int class)
{E_
    switch (class) {
    case PseudoColor:
	printf ("PseudoColor");
	if (CDepth >= 7)
/* 'Depth' is the number of color bits per graphics pixel */
	    printf (_ (" - depth ok, this will work.\n"));
	else
/* 'Depth' is the number of color bits per graphics pixel */
	    printf (_ (" - depth low, this may not work.\n"));
	break;
    case GrayScale:
	printf ("Grayscale -\n");
/* 'Visual' is the hardware method of displaying graphics */
	printf (_ ("Mmmmh, haven't tried this visual class, let's see what happens.\n"));
	break;
    case DirectColor:
	printf ("DirectColor -\n");
/* 'Visual' is the hardware method of displaying graphics */
	printf (_ ("Mmmmh, haven't tried this visual class, let's see what happens.\n"));
	break;
    case StaticColor:
	printf ("StaticColor - ");
/* "Let us attempt to use this Visual even though it may not work" */
	printf (_ ("lets give it a try.\n"));
	break;
    case StaticGray:
	printf ("StaticGray - ");
/* "Let us attempt to use this Visual even though it may not work" */
	printf (_ ("lets give it a try.\n"));
	break;
    case TrueColor:
	printf ("TrueColor - ");
/* "Adequite" (with sarcasm) : i.e. it is actually the best kind of Visual " */
	printf (_ ("fine.\n"));
	break;
    default:
/* 'Visual' is the method hardware method of displaying graphics */
	CError (_ ("?\nVisual class unknown.\n"));
	break;
    }
}

/* must be free'd */
XColor *get_cells (Colormap cmap, int *size)
{E_
    int i;
    XColor *c;
    *size = DisplayCells (CDisplay, DefaultScreen (CDisplay));
    c = CMalloc (*size * sizeof (XColor));
    for (i = 0; i < *size; i++)
	c[i].pixel = i;
    XQueryColors (CDisplay, cmap, c, *size);
    return c;
}

#define BitsPerRGBofVisual(v) (v->bits_per_rgb)


/* find the closest color without allocating it */
int CGetCloseColor (XColor * cells, int ncells, XColor color, long *error)
{E_
    unsigned long merror = (unsigned long) -1;
    unsigned long e;
    int min = 0, i;
    unsigned long mask = 0xFFFF0000UL;

    mask >>= min (BitsPerRGBofVisual (CVisual), 5);
    for (i = 0; i < ncells; i++) {
	e = 8 * abs ((int) (color.red & mask) - (int) (cells[i].red & mask)) + 10 * abs ((int) (color.green & mask) - (int) (cells[i].green & mask)) + 5 * abs ((int) (color.blue & mask) - (int) (cells[i].blue & mask));
	if (e < merror) {
	    merror = e;
	    min = i;
	}
    }
    merror = 8 * abs ((int) (color.red & mask) - (int) (cells[min].red & mask)) + 10 * abs ((int) (color.green & mask) - (int) (cells[min].green & mask)) + 5 * abs ((int) (color.blue & mask) - (int) (cells[min].blue & mask));
    if (error)
	*error = (long) merror;
    return min;
}

#define grey_intense(i) (i * 65535 / 63)

/* return -1 if not found. Meaning that another coolwidget app is not running */
int find_coolwidget_grey_scale (XColor * c, int ncells)
{E_
    unsigned long mask = 0xFFFF0000UL;
    int i, j;
    mask >>= BitsPerRGBofVisual (CVisual);

    for (i = 0; i < ncells; i++) {
	for (j = 0; j < 64; j++)
	    if (!((c[i + j].green & mask) == (grey_intense (j) & mask)
		  && c[i + j].red == c[i + j].green && c[i + j].green == c[i + j].blue))
		goto cont;
	return i;
      cont:;
    }
    return -1;
}


void CAllocColorCells (Colormap colormap, Bool contig,
		       unsigned long plane_masks[], unsigned int nplanes,
		       unsigned long pixels[], unsigned int npixels)
{E_
    if (!XAllocColorCells (CDisplay, colormap, contig,
			   plane_masks, nplanes, pixels, npixels))
	alloccolorerror ();
}

void CAllocColor (Colormap cmap, XColor * c)
{E_
    if (!XAllocColor (CDisplay, cmap, c))
	alloccolorerror ();
}

static void get_grey_colors (XColor * color, int i)
{E_
    color->red = color->green = grey_intense (i);
    color->blue = grey_intense (i);
    color->flags = DoRed | DoBlue | DoGreen;
}

static void get_button_color (XColor * color, int i)
{E_
    (*look->get_button_color) (color, i);
}

int option_invert_colors = 0;
int option_invert_crome = 0;
int option_invert_red_green = 0;
int option_invert_green_blue = 0;
int option_invert_red_blue = 0;

#define clip(x,a,b) ((x) >= (b) ? (b) : ((x) <= (a) ? (a) : (x)))
#define cswap(a,b) {float t; t = (a); (a) = (b); (b) = t;}

/* inverts the cromiance - this is for editor background colors that are very light */
static int transform (int color)
{E_
    float r, g, b, y, y_max, c1, c1_max, c2, c2_max;
    r = (float) ((color >> 16) & 0xFF);
    g = (float) ((color >> 8) & 0xFF);
    b = (float) ((color >> 0) & 0xFF);
    y_max = 0.3 * 240.0 + 0.6 * 240.0 + 0.1 * 240.0;
    if (option_invert_red_green)
	cswap (r, g)
	    if (option_invert_green_blue)
	    cswap (g, b)
		if (option_invert_red_blue)
		cswap (r, b)
		    y = 0.3000 * r + 0.6000 * g + 0.1000 * b;
    c1 = -0.1500 * r - 0.3000 * g + 0.4500 * b;
    c2 = 0.4375 * r - 0.3750 * g - 0.0625 * b;
    c1_max = -0.1500 * 255.0 - 0.3000 * 255.0 + 0.4500 * 255.0;
    c2_max = 0.4375 * 255.0 - 0.3750 * 255.0 - 0.0625 * 255.0;
    if (option_invert_crome) {
	c1 = c1_max - c1;
	c2 = c2_max - c2;
    }
    if (option_invert_colors)
	y = y_max - y;
    r = 1.0 * y + 0.0000 * c1 + 1.6 * c2;
    g = 1.0 * y - 0.3333 * c1 - 0.8 * c2;
    b = 1.0 * y + 2.0000 * c1 + 0.0 * c2;
    r = clip (r, 0.0, 255.0);
    g = clip (g, 0.0, 255.0);
    b = clip (b, 0.0, 255.0);
    return (int) (((int) r) << 16) | (((int) g) << 8) | (((int) b) << 0);
}

/* colours */
int option_color_0 = 0x080808;
int option_color_1 = 0x000065;
int option_color_2 = 0x0000FF;
int option_color_3 = 0x008B00;
int option_color_4 = 0x008B8B;
int option_color_5 = 0x009ACD;
int option_color_6 = 0x00FF00;
int option_color_7 = 0x00FA9A;
int option_color_8 = 0x00FFFF;
int option_color_9 = 0x8B2500;
int option_color_10 = 0x8B008B;
int option_color_11 = 0x7D26CD;
int option_color_12 = 0x8B7500;
int option_color_13 = 0x7F7F7F;
int option_color_14 = 0x7B68EE;
int option_color_15 = 0x7FFF00;
int option_color_16 = 0x87CEEB;
int option_color_17 = 0x7FFFD4;
int option_color_18 = 0xEE0000;
int option_color_19 = 0xEE1289;
int option_color_20 = 0xEE00EE;
int option_color_21 = 0xCD6600;
int option_color_22 = 0xF8B7B7;
int option_color_23 = 0xE066FF;
int option_color_24 = 0xEEEE00;
int option_color_25 = 0xEEE685;
int option_color_26 = 0xF8F8FF;

/* takes 0-26 and converts it to RGB */
static void get_general_colors (XColor * color, int i)
{E_
    unsigned long c = 0;
    switch (i) {
    case 0:
	c = transform (option_color_0);
	break;
    case 1:
	c = transform (option_color_1);
	break;
    case 2:
	c = transform (option_color_2);
	break;
    case 3:
	c = transform (option_color_3);
	break;
    case 4:
	c = transform (option_color_4);
	break;
    case 5:
	c = transform (option_color_5);
	break;
    case 6:
	c = transform (option_color_6);
	break;
    case 7:
	c = transform (option_color_7);
	break;
    case 8:
	c = transform (option_color_8);
	break;
    case 9:
	c = transform (option_color_9);
	break;
    case 10:
	c = transform (option_color_10);
	break;
    case 11:
	c = transform (option_color_11);
	break;
    case 12:
	c = transform (option_color_12);
	break;
    case 13:
	c = transform (option_color_13);
	break;
    case 14:
	c = transform (option_color_14);
	break;
    case 15:
	c = transform (option_color_15);
	break;
    case 16:
	c = transform (option_color_16);
	break;
    case 17:
	c = transform (option_color_17);
	break;
    case 18:
	c = transform (option_color_18);
	break;
    case 19:
	c = transform (option_color_19);
	break;
    case 20:
	c = transform (option_color_20);
	break;
    case 21:
	c = transform (option_color_21);
	break;
    case 22:
	c = transform (option_color_22);
	break;
    case 23:
	c = transform (option_color_23);
	break;
    case 24:
	c = transform (option_color_24);
	break;
    case 25:
	c = transform (option_color_25);
	break;
    case 26:
	c = transform (option_color_26);
	break;
    }
    color->red = ((c >> 16) & 0xFF) << 8;
    color->green = ((c >> 8) & 0xFF) << 8;
    color->blue = ((c >> 0) & 0xFF) << 8;
    color->flags = DoRed | DoBlue | DoGreen;
}

void alloc_grey_scale (Colormap cmap)
{E_
    XColor color;
    int i;

    if (option_using_grey_scale) {
	for (i = 0; i < 64; i++) {
	    get_grey_colors (&color, i);
	    CAllocColor (cmap, &color);
	    color_pixels[i + N_WIDGET_COLORS + N_FAUX_COLORS].raw = color.pixel;
	}
    }
}

/*
   This sets up static color, but tries to be more intelligent about the
   way it handles grey scales. This allows resonable color display on 16
   color VGA servers.
 */
static void setup_staticcolor (void)
{E_
    XColor *c;
    unsigned short *grey_levels;
    XColor color;
    int size, i, j, k, n, m = 0, num_greys, grey;

    c = get_cells (CColormap, &size);
    grey_levels = CMalloc ((size + 2) * sizeof (unsigned short));

/* we are probably not going to find our coolwwidget colors here,
   so use greyscale for the buttons. first count how many greys,
   and sort them: */

    grey = 0;
    for (i = 0; i < size; i++) {
	if (c[i].red == c[i].green && c[i].green == c[i].blue) {
	    if (grey) {
		for (n = 0; n < grey; n++)
		    if (c[i].green == grey_levels[n])
			goto cont;
		for (n = grey - 1; n >= 0; n--)
		    if (grey_levels[n] > c[i].green) {
			Cmemmove (&(grey_levels[n + 1]), &(grey_levels[n]), (grey - n) * sizeof (unsigned short));
			grey_levels[n] = c[i].green;
			grey++;
			goto cont;
		    }
		grey_levels[grey] = c[i].green;
	    } else
		grey_levels[grey] = c[i].green;
	    grey++;
	  cont:;
	}
    }
    num_greys = grey;

    if (num_greys <= 2) {	/* there's just no hope  :(   */
	if (verbose_operation)
	    printf (_ ("This will work, but it may look terrible.\n"));
	for (grey = 0; grey < 16; grey++) {
	    color.flags = DoRed | DoGreen | DoBlue;
	    color.red = grey * 65535 / 15;
	    color.green = grey * 65535 / 15;
	    color.blue = grey * 65535 / 15;
	    if (!XAllocColor (CDisplay, CColormap, &color))
		alloccolorerror ();
	    color_pixels[grey].raw = color.pixel;
	}
	alloc_grey_scale (CColormap);
    } else {
	j = 0;
	k = 0;
	for (grey = 0; grey < num_greys; grey++) {
/* button colors */
	    color.red = color.green = grey_levels[grey];
	    color.blue = grey_levels[grey];
	    color.flags = DoRed | DoGreen | DoBlue;

	    for (; j < (grey + 1) * 16 / num_greys; j++) {
		CAllocColor (CColormap, &color);
		color_pixels[j].raw = color.pixel;
	    }
/* grey scale */
	    if (option_using_grey_scale) {
		for (; k < (grey + 1) * 64 / num_greys; k++) {
		    CAllocColor (CColormap, &color);
		    color_pixels[k + N_WIDGET_COLORS + N_FAUX_COLORS].raw = color.pixel;
		}
	    }
	}
    }

    for (i = 0; i < N_FAUX_COLORS; i++) {
	get_general_colors (&color, i);
	m = CGetCloseColor (c, size, color, 0);
	CAllocColor (CColormap, &(c[m]));
	color_pixels[N_WIDGET_COLORS + i].raw = c[m].pixel;
    }

    free (grey_levels);
    free (c);
}

static void make_grey (XColor * color)
{E_
    long g;

    g = ((long) color->red) * 8L + ((long) color->green) * 10L + ((long) color->blue) * 5L;
    g /= (8l + 10L + 5L);
    color->red = color->green = g;
    color->blue = g;
}

/*
   For TrueColor displays, colors can always be found. Hence we
   need not find the "closest" matching color.
 */
static void setup_alloc_colors (int force_grey)
{E_
    int i;
    XColor color;

    color.flags = DoRed | DoGreen | DoBlue;

    for (i = 0; i < N_WIDGET_COLORS; i++) {
	get_button_color (&color, i);
	if (force_grey)
	    make_grey (&color);
	CAllocColor (CColormap, &color);
	color_pixels[i].raw = color.pixel;
    }

    for (i = 0; i < N_FAUX_COLORS; i++) {
	get_general_colors (&color, i);
	if (force_grey)
	    make_grey (&color);
	CAllocColor (CColormap, &color);
	color_pixels[N_WIDGET_COLORS + i].raw = color.pixel;
    }

    alloc_grey_scale (CColormap);
}

void store_grey_scale (Colormap cmap)
{E_
    XColor color;
    int i;
    if (verbose_operation)
/* "Grey scale" is a list of increasingly bright grey levels of color */
	printf (_ ("Storing grey scale.\n"));
    if (!XAllocColorCells (CDisplay, cmap, 1, color_planes, 6, &color_pixels[N_WIDGET_COLORS + N_FAUX_COLORS].raw, 1))
	alloccolorerror ();
    for (i = 0; i < 64; i++) {
	color_pixels[N_WIDGET_COLORS + N_FAUX_COLORS + i].raw = color_pixels[N_WIDGET_COLORS + N_FAUX_COLORS].raw + i;
	color.pixel = color_pixels[N_WIDGET_COLORS + N_FAUX_COLORS + i].raw;
	get_grey_colors (&color, i);
	XStoreColor (CDisplay, cmap, &color);
    }
}

void try_color (Colormap cmap, XColor * c, int size, XColor color, int i)
{E_
    int x;
    long error;
    XColor closest;

    x = CGetCloseColor (c, size, color, &error);
    closest = c[x];

    if (error) {
	if (XAllocColorCells (CDisplay, cmap, 0, color_planes, 0, &color_pixels[i].raw, 1)) {
	    color.pixel = color_pixels[i].raw;
	    XStoreColor (CDisplay, cmap, &color);
	    if (verbose_operation)
/* "Assign color" */
		printf (_ ("Store,"));
	    return;
	}
    }
    if (!XAllocColor (CDisplay, cmap, &closest))
	if (verbose_operation)
/* "Ignoring" means that the program will continue regardless of this error */
	    printf (_ ("\nerror allocating this color - ignoring;"));	/* this should never happen since closest comes from the colormap */
    color_pixels[i].raw = closest.pixel;
    if (verbose_operation)
	printf ("%ld,", ((error == 0) ? 0 : 1) + ((error / (8 + 10 + 5)) >> (16 - BitsPerRGBofVisual (CVisual))));
}


/*
   for PseudoColor displays. This tries to be conservative in the number
   of entries its going to store, while still keeping the colors exact.
   first it looks for an entry in the default colormap and only stores
   in the map if no match is found. Multiple coolwidget applications can
   therefore share the same map. At worst 16 + 27 of the palette are used
   plus another 64 if you are using greyscale.
 */
static void setup_store_colors (void)
{E_
    int i, size;
    XColor *c;
    XColor color;

    c = get_cells (CColormap, &size);

    color.flags = DoRed | DoGreen | DoBlue;

/* grey scale has to be contigous to be fast so store a 64 colors */
    if (option_using_grey_scale) {
#if 0
	i = find_coolwidget_grey_scale (c, size);
	if (i >= 0) {
	    if (verbose_operation)
/* Not essential to translate */
		printf (_ ("found grey scale\n"));
	    alloc_grey_scale (CColormap);
	} else {
#endif
	    store_grey_scale (CColormap);
#if 0
	}
#endif
    }
    if (verbose_operation)
/* This isn't very important, run cooledit -verbose to see how this works */
	printf (_ ("Trying colors...\n( 'Store'=store my own color, Number=integer error with existing color )\n"));

    for (i = 0; i < N_WIDGET_COLORS; i++) {
	get_button_color (&color, i);
	try_color (CColormap, c, size, color, i);
    }

    for (i = 0; i < N_FAUX_COLORS; i++) {
	get_general_colors (&color, i);
	try_color (CColormap, c, size, color, i + N_WIDGET_COLORS);
    }
    if (verbose_operation)
	printf ("\n");
    free (c);
}


static void setup_colormap (int class)
{E_
    memset (color_pixels, '\0', sizeof (color_pixels));
    color_last_pixel = N_FAUX_COLORS;

    switch (class) {
    case StaticColor:
    case StaticGray:
	setup_staticcolor ();
	break;
    case GrayScale:
	setup_alloc_colors (1);
	return;
    case DirectColor:
    case TrueColor:
	setup_alloc_colors (0);
	return;
    case PseudoColor:
	setup_store_colors ();
	break;
    }
}

int CSendEvent (XEvent * e);
static XEvent xevent;

static int cursor_blink_rate;

/*
   Aim1: Get the cursor to flash all the time:

   Aim2: Have coolwidgets send an alarm event, just like
   any other event. For the application to use.

   Requirements: XNextEvent must still be the blocker
   so that the process doesn't hog when idle.

   Problems: If the alarm handler sends an event using
   XSendEvent it may hang the program.

   To solve, we put a pause() before XNextEvent so that it waits for 
   an alarm, and also define our own CSendEvent routine with
   its own queue. So that things don't slow down, we pause() only
   if no events are pending. Also make the alarm rate high (100 X per sec).
   (I hope this is the easiest way to do this  :|   )
 */

void CSetCursorBlinkRate (int b)
{E_
    if (b < 1)
	b = 1;
    cursor_blink_rate = b;
}

int CGetCursorBlinkRate (void)
{E_
    return cursor_blink_rate;
}

/* does nothing and calls nothing for t seconds, resolution is ALRM_PER_SECOND */
void CSleep (double t)
{E_
    float i;
    for (i = 0; i < t; i += 1.0 / ALRM_PER_SECOND)
	pause ();
}


static struct itimerval alarm_every =
{
    {
	0, 0
    },
    {
	0, 1000000 / ALRM_PER_SECOND
    }
};

static struct itimerval alarm_off =
{
    {
	0, 0
    },
    {
	0, 0
    }
};

/*
   This flag goes non-zero during the CSendEvent procedure. This
   prevents the small chance that an alarm event might occur during
   a CSendEvent.
 */
int block_push_event = 0;
int got_alarm = 0;

void _alarmhandler (void)
{E_
    static int count = ALRM_PER_SECOND;
    got_alarm = 0;
    if (count) {
	count--;
	if (CQueueSize () < 16 && !block_push_event) {
	    CSendEvent (&xevent);
	}
    } else {
        housekeeping_inspect ();
	xevent.type = AlarmEvent;
	if (CQueueSize () < 128 && !block_push_event) {	/* say */
	    CSendEvent (&xevent);
	}
	xevent.type = TickEvent;
	count = ALRM_PER_SECOND / cursor_blink_rate;
    }
}

static RETSIGTYPE alarmhandler (int x)
{E_
    if (!got_alarm)
	got_alarm = 1;
    signal (SIGALRM, alarmhandler);
    setitimer (ITIMER_REAL, &alarm_every, NULL);
#if (RETSIGTYPE==void)
    return;
#else
    return 1;			/* :guess --- I don't know what to return here */
#endif
}

static void set_alarm (void)
{E_
    memset (&xevent, 0, sizeof (XEvent));
    xevent.type = 0;
    xevent.xany.display = CDisplay;
    xevent.xany.send_event = 1;

    CSetCursorBlinkRate (7);	/* theta rhythms ? */

    signal (SIGALRM, alarmhandler);
    setitimer (ITIMER_REAL, &alarm_every, NULL);
}

void CEnableAlarm (void)
{E_
    set_alarm ();
}

void CDisableAlarm (void)
{E_
    setitimer (ITIMER_REAL, &alarm_off, NULL);
    signal (SIGALRM, SIG_IGN);
}

static void get_endian (void)
{E_
    char o[sizeof (long)];
    unsigned long *p;
    p = (unsigned long *) (void *) &o[0];
    *p = 1UL;
    if (o[0])
        endian_little = 1;
    else
        endian_little = 0;
}

void get_temp_dir (void)
{E_
    if (temp_dir)
	return;
    temp_dir = getenv ("TEMP");
    if (temp_dir)
	if (*temp_dir) {
	    temp_dir = (char *) strdup (temp_dir);
	    return;
	}
    temp_dir = getenv ("TMP");
    if (temp_dir)
	if (*temp_dir) {
	    temp_dir = (char *) strdup (temp_dir);
	    return;
	}
    temp_dir = (char *) strdup ("/tmp");
}

void get_home_dir (void)
{E_
    char homedir_[MAX_PATH_LEN] = "";
    char errmsg[REMOTEFS_ERR_MSG_LEN] = "";
    struct remotefs *u;
    if (local_home_dir)		/* already been set */
	return;
    u = the_remotefs_local;
    if ((*u->remotefs_gethomedir) (u, homedir_, sizeof (homedir_), errmsg) || !*homedir_) {
        fprintf (stderr, "could not get home directory: %s\n", errmsg);
        local_home_dir = strdup ("/");
        return;
    }
    local_home_dir = strdup (homedir_);
}

static void get_dir (void)
{E_
    if (!get_current_wd (current_dir, MAX_PATH_LEN))
	*current_dir = 0;
    get_temp_dir ();
    get_home_dir ();
}

void wm_interaction_init (void)
{E_
    ATOM_ICCCM_P2P_CLIPBOARD = XInternAtom (CDisplay, "CLIPBOARD", False);
    ATOM_UTF8_STRING = XInternAtom (CDisplay, "UTF8_STRING", False);
    ATOM_WM_PROTOCOLS = XInternAtom (CDisplay, "WM_PROTOCOLS", False);
    ATOM_WM_DELETE_WINDOW = XInternAtom (CDisplay, "WM_DELETE_WINDOW", False);
    ATOM_WM_NAME = XInternAtom (CDisplay, "WM_NAME", False);
    ATOM_WM_TAKE_FOCUS = XInternAtom (CDisplay, "WM_TAKE_FOCUS", False);
}

/* look up the modifier mask for the Alt or Meta key */
static void get_alt_key (void)
{E_
    int i, j, k, v;
    unsigned int found_mask = 0;
    XModifierKeymap *map;
    unsigned int modmasks[] = { 0, 0, 0, Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask };
    unsigned int masks[] = {XK_Alt_L, XK_Alt_R, XK_Meta_L, XK_Meta_R};
    map = XGetModifierMapping (CDisplay);
    for (v = 0; v < 4; v++) {
	for (i = 3; i < 8; i++) {
	    k = i * map->max_keypermod;
	    for (j = 0; j < map->max_keypermod; j++) {
		if (map->modifiermap[k + j] == 0)
		    break;
                if (CKeycodeToKeysym (map->modifiermap[k + j]) == masks[v]) {
                    found_mask = modmasks[i];
		    goto done;
		}
	    }
	}
    }
  done:
    alt_modifier_mask = found_mask ? found_mask : Mod1Mask;
    XFreeModifiermap (map);
}


#ifdef GUESS_VISUAL

char visual_name[16][16];

void make_visual_list (void)
{E_
    memset (visual_name, 0, sizeof (visual_name));
    strcpy (visual_name[StaticGray], "StaticGray");
    strcpy (visual_name[GrayScale], "GrayScale");
    strcpy (visual_name[StaticColor], "StaticColor");
    strcpy (visual_name[PseudoColor], "PseudoColor");
    strcpy (visual_name[TrueColor], "TrueColor");
    strcpy (visual_name[DirectColor], "DirectColor");
}

struct visual_priority {
    int class;
    int depth_low;
    int depth_high;
} visual_priority[] = {

    {
	TrueColor, 15, 16
    },
    {
	TrueColor, 12, 14
    },
    {
	PseudoColor, 6, 999
    },
    {
	DirectColor, 12, 999
    },
    {
	TrueColor, 17, 999
    },

    {
	DirectColor, 8, 11
    },
    {
	TrueColor, 8, 11
    },
    {
	StaticColor, 8, 999
    },

    {
	PseudoColor, 4, 5
    },
    {
	DirectColor, 6, 7
    },
    {
	TrueColor, 6, 7
    },
    {
	StaticColor, 6, 7
    },

    {
	DirectColor, 4, 5
    },
    {
	TrueColor, 4, 5
    },
    {
	StaticColor, 4, 5
    },

    {
	GrayScale, 6, 999
    },
    {
	StaticGray, 6, 999
    },
    {
	GrayScale, 4, 5
    },
    {
	StaticGray, 4, 5
    },
};

#endif

char *option_preferred_visual = 0;
int option_force_own_colormap = 0;
int option_force_default_colormap = 0;

void get_preferred (XVisualInfo * v, int n, Visual ** vis, int *depth)
{E_
#ifndef GUESS_VISUAL
    *vis = DefaultVisual (CDisplay, DefaultScreen (CDisplay));
    *depth = DefaultDepth (CDisplay, DefaultScreen (CDisplay));
#else
    int i, j;
    Visual *def = 0;
    int def_depth = 0;

    if (option_preferred_visual)
	if (!*option_preferred_visual)
	    option_preferred_visual = 0;

/* NLS ? */
    if (option_preferred_visual)
	if (!Cstrcasecmp (option_preferred_visual, "help")
	    || !Cstrcasecmp (option_preferred_visual, "h")) {
	    printf (_ ("%s:\n  The <visual-class> is the hardware technique used by the\n" \
		       "computer to draw pixels to the screen. _Usually_ only one\n" \
		 "visual is truly supported. This option is provided\n" \
	     "if you would rather use a TrueColor than a PseudoColor\n" \
		 "visual where you are short of color palette space.\n" \
		       "The depth is the number of bits per pixel used by the hardware.\n" \
		   "It is automatically selected using heuristics.\n"), \
		    CAppName);
	    printf (_ ("Available visuals on this system are:\n"));
	    for (i = 0; i < n; i++)
		printf ("        class %s, depth %d\n", visual_name[v[i].class], v[i].depth);
	    exit (1);
	}
    if (option_preferred_visual) {	/* first check if the user wants the default visual */
	int default_name;
	default_name = ClassOfVisual (DefaultVisual (CDisplay, DefaultScreen (CDisplay)));
	if (visual_name[default_name])
	    if (!Cstrcasecmp (visual_name[default_name], option_preferred_visual)) {
		*vis = DefaultVisual (CDisplay, DefaultScreen (CDisplay));
		*depth = DefaultDepth (CDisplay, DefaultScreen (CDisplay));
		return;
	    }
    }
    for (j = 0; j < sizeof (visual_priority) / sizeof (struct visual_priority); j++)
	for (i = 0; i < n; i++)
	    if (v[i].class == visual_priority[j].class)
		if (v[i].depth >= visual_priority[j].depth_low && v[i].depth <= visual_priority[j].depth_high) {
		    if (option_preferred_visual) {
			if (!Cstrcasecmp (visual_name[v[i].class], option_preferred_visual)) {
			    *vis = v[i].visual;
			    *depth = v[i].depth;
			    return;
			}
		    }
		    if (!def) {
			def = v[i].visual;
			def_depth = v[i].depth;
		    }
		}
    if (option_preferred_visual)
/* We will select a visual in place of the one you specified, since yours is unavailable */
	fprintf (stderr, _ ("%s: preferred visual not found, selecting...\n"), CAppName);

    if (def) {
	*vis = def;
	*depth = def_depth;
    } else {
/* We will select the default visual, since the list didn't have a matching visual */
	fprintf (stderr, _ ("%s: no known visuals found, using default...\n"), CAppName);
	*vis = DefaultVisual (CDisplay, DefaultScreen (CDisplay));
	*depth = DefaultDepth (CDisplay, DefaultScreen (CDisplay));
    }
    option_preferred_visual = 0;
#endif
}

static void get_preferred_visual_and_depth (void)
{E_
    XVisualInfo *v, t;
    int n;

#ifdef GUESS_VISUAL
    make_visual_list ();
#endif
    t.screen = DefaultScreen (CDisplay);

    v = XGetVisualInfo (CDisplay, VisualScreenMask, &t, &n);

    get_preferred (v, n, &CVisual, &CDepth);

    XFree(v);
}

static void assign_default_cmap (void)
{E_
    if (verbose_operation)
	printf (_ ("Using DefaultColormap()\n"));
    CColormap = DefaultColormap (CDisplay, DefaultScreen (CDisplay));
}

static void assign_own_cmap (void)
{E_
    if (verbose_operation)
	printf (_ ("Creating own colormap\n"));
    CColormap = XCreateColormap (CDisplay,
			 RootWindow (CDisplay, DefaultScreen (CDisplay)),
				 CVisual, AllocNone);
}

static void assign_check_colormap (void)
{E_
#if 0				/* What do I do here ? */
    switch (ClassOfVisual (CVisual)) {
    case PseudoColor:
    case GrayScale:
    case DirectColor:
	assign_default_cmap ();
	return;
    }
#endif
    assign_own_cmap ();
}

/* #define TRY_WM_COLORMAP 1 */

#define COLORMAP_PROPERTY "RGB_DEFAULT_MAP"

static void get_colormap (void)
{E_
#ifdef TRY_WM_COLORMAP
    Atom DEFAULT_CMAPS;
#endif

    if (option_force_default_colormap) {
	assign_default_cmap ();
	return;
    }
    if (option_force_own_colormap) {
	assign_own_cmap ();
	return;
    }
    if (XVisualIDFromVisual (CVisual)
	== XVisualIDFromVisual (DefaultVisual (CDisplay, DefaultScreen (CDisplay)))) {
	if (verbose_operation)
	    printf (_ ("Default visual ID found\n"));
	assign_default_cmap ();
	return;
    }
#ifdef TRY_WM_COLORMAP
/* NLS ? */
    if (verbose_operation)
	printf ("I don't really know what I'm doing here, so feel free to help - paul\n");

    DEFAULT_CMAPS = XInternAtom (CDisplay, COLORMAP_PROPERTY, True);

    if (DEFAULT_CMAPS == None) {
	if (verbose_operation)
/* "An Atom of name %s could not be found". 'Atom' is X terminology */
	    printf (_ ("No Atom %s \n"), COLORMAP_PROPERTY);
	assign_check_colormap ();
	return;
    } else {
	int i, n;
	XStandardColormap *cmap;
	if (!XGetRGBColormaps (CDisplay, CRoot, &cmap, &n,
			       DEFAULT_CMAPS)) {
	    if (verbose_operation)
		printf (_ ("XGetRGBColormaps(%s) failed\n"), COLORMAP_PROPERTY);
	    assign_check_colormap ();
	    return;
	}
	if (verbose_operation)
	    printf (_ ("Choosing from %d 'XGetRGBColormaps' colormaps\n"), n);
	for (i = 0; i < n; i++) {
	    if (XVisualIDFromVisual (CVisual) == cmap[i].visualid) {
		if (verbose_operation)
		    printf (_ ("Colormap %d matches visual ID\n"), i);
		CColormap = cmap[i].colormap;
		return;
	    }
	}
	if (verbose_operation)
	    printf (_ ("No colormap found matching our visual ID\n"));
    }
#endif

    assign_check_colormap ();
}

int ignore_handler (Display * c, XErrorEvent * e)
{E_
    return 0;
}

void init_cursors (void);
void get_dummy_gc (void);

/*-------------------------------------------------------------*/
void CInitialise (CInitData * config_start)
{E_
    get_endian ();

/*test_xx_strchr ();
*/

    if (!config_start->look)
	config_start->look = init_look;

    if (!strncmp (config_start->look, "gtk", 3)) {
	look = &look_gtk;
    } else if (!strncmp (config_start->look, "next", 4)) {
#ifdef NEXT_LOOK
	look = &look_next;
#else
	fprintf (stderr, _ ("%s: NeXT look was not compiled into this binary\n"), config_start->name);
	exit (1);
#endif
    } else if (!strncmp (config_start->look, "cool", 4)) {
	look = &look_cool;
    } else {
	look = &look_gtk;
    }

    option_interwidget_spacing = (*look->get_default_interwidget_spacing) ();
    init_widget_font = (char *) (*look->get_default_widget_font) ();
    init_8bit_term_font = (char *) get_default_8bit_term_font ();
    init_font = (char *) get_default_editor_font ();

    given = config_start;
    verbose_operation = (given->options & CINIT_OPTION_VERBOSE);

    if (verbose_operation)
	printf ("sizeof(CWidget) = %d\n", (int) sizeof (CWidget));

    CAppName = given->name;

    option_using_grey_scale = (given->options & CINIT_OPTION_USE_GREY);

/* zero out */
    init_cursor_state ();

/* Initialise the widget library */
    init_widgets ();

/* get home dir directory into home_dir and current directory into current_dir */
    get_dir ();

/* Get resources from the resource file */
    get_resources ();
    if (given->display)
	init_display = given->display;
    if (given->geometry)
	init_geometry = given->geometry;
    if (given->font)
	init_font = given->font;
    if (given->widget_font)
	init_widget_font = given->widget_font;
    if (given->_8bit_term_font)
	init_8bit_term_font = given->_8bit_term_font;
    if (given->bg)
	init_bg_color = given->bg;
    if (given->fg_red)
	init_fg_color_red = given->fg_red;
    if (given->fg_green)
	init_fg_color_green = given->fg_green;
    if (given->fg_blue)
	init_fg_color_blue = given->fg_blue;

/*  Open connection to display selected by user */
    open_display (CAppName, given->options & CINIT_OPTION_WAIT_FOR_DISPLAY);
    XSetErrorHandler (ignore_handler);

/*  Initialise window manager atoms to detect a user close */
    wm_interaction_init ();

/* Now set up the visual and colors */
    get_preferred_visual_and_depth ();

    if (verbose_operation) {
	printf (_ ("Found a visual, depth = %d,\n       visual class = "), CDepth);
	visual_comments (ClassOfVisual (CVisual));
    }
    get_colormap ();

/* Now setup that color map discribed above */
    setup_colormap (ClassOfVisual (CVisual));

#ifndef NO_TTF
    XAaInit (CDisplay, CVisual, CDepth, CRoot);
#endif

/* Set up font */
    init_load_font ();

#ifdef USE_XIM
/* set the XIM locale */
    init_xlocale ();
#endif

    get_alt_key ();

/* some special cursors */
    init_cursors ();

/* Initialise drag and drop capabilities xdnd protocol */
    xdnd_init (CDndClass, CDisplay);
    mouse_init ();

/* set child handler */
    set_child_handler ();

/* an alarm handler generates xevent of tyoe AlarmEvent every 1/4 second to flash the cursor */
    set_alarm ();
}
