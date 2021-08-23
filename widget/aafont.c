/* aafont.c - generic library for drawing anti-aliased fonts
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include <assert.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "stringtools.h"
#include "font.h"
#include "aafont.h"


#define HAVE_FREETYPE


#ifdef HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif


/*
 * list management
 * ---------------
 *
 * Situation is a list of aa fonts:
 * Each font is uniquified by a fg colour, a bg colour and a fontid.
 * Each font has FONT_LAST_UNICHAR pixmap glyphs.
 * The FONT_LAST_UNICHAR pixmaps are divided into 256 blocks.
 * Each block is allocated on demand only.
 * We *ImageString* only i.e. no DrawString - since requires knowledge
 *                             of the background which is complicated.
 */

Display *aa_display;
int aa_depth;
Window aa_root;
Visual *aa_visual;

int option_rgb_order = RedFirst;
int option_interchar_spacing = 0;

void XAaInit (Display * display, Visual * visual, int depth, Window root)
{
    aa_display = display;
    aa_depth = depth;
    aa_root = root;
    aa_visual = visual;
}

struct aa_glyph_cache {
    Pixmap pixmap;
    short width;
    short descent;
};

#define NUM_GLYPH_BLOCKS        ((FONT_LAST_UNICHAR + 257) / 256)

struct aa_font_cache {
    const struct aa_font *f;
    GC gc;
    unsigned long fg;
    unsigned long bg;
    struct aa_glyph_cache *glyph[NUM_GLYPH_BLOCKS];
    int num_pixmaps;
    int scale;
    struct aa_font_cache *next;
} *font_cache_list = 0;

static void aa_insert (void)
{
    struct aa_font_cache *p;
    p = malloc (sizeof (*font_cache_list));
    memset (p, 0, sizeof (*font_cache_list));
    if (!font_cache_list) {
	font_cache_list = p;
    } else {
	p->next = font_cache_list;
	font_cache_list = p;
    }
}

static void aa_free (struct aa_font_cache *f)
{
    int i, j;
    if (f->f->font_struct)
        XFreeFontInfo (0, f->f->font_struct, 0);
    for (i = 0; i < NUM_GLYPH_BLOCKS; i++) {
	if (f->glyph[i]) {
	    for (j = 0; j < 256; j++)
		if (f->glyph[i][j].pixmap)
		    XFreePixmap (aa_display, f->glyph[i][j].pixmap);
	    memset (f->glyph[i], 0, 256 * sizeof (struct aa_glyph_cache));
	    free (f->glyph[i]);
	}
    }
    memset (f, 0, sizeof (*f));
    free (f);
}

/* passing fg == bg == 0 finds any load_id */
static struct aa_font_cache *aa_find (int load_id, unsigned long fg, unsigned long bg)
{
    struct aa_font_cache *p;
    for (p = font_cache_list; p; p = p->next)
	if (load_id && p->f->load_id == load_id && p->fg == fg && p->bg == bg)
	    return p;
    return 0;
}

/* passing fg == bg == 0 finds any load_id */
static struct aa_font_cache *aa_find_metrics_only (int load_id)
{
    struct aa_font_cache *p;
    for (p = font_cache_list; p; p = p->next)
	if (load_id && p->f->load_id == load_id)
	    return p;
    return 0;
}

/* returns zero on not found */
static int _aa_remove (int load_id)
{
    struct aa_font_cache *p, *q = 0;
    for (p = font_cache_list; p; p = p->next) {
	if (load_id && p->f->load_id == load_id) {
	    if (p == font_cache_list) {
		struct aa_font_cache *t;
		t = font_cache_list->next;
		aa_free (font_cache_list);
		font_cache_list = t;
		return 1;
	    } else {
		q->next = p->next;
		aa_free (p);
		return 1;
	    }
	}
	q = p;
    }
    return 0;
}

static void aa_remove (int load_id)
{
    while (_aa_remove (load_id));
}


/* fifth level */
/* 5 by 9/3 guassian convolution */
static unsigned long aa_convolve_3 (int i, int j, unsigned char *source, int source_bytes_per_line,
				  int byte_order, int bytes_per_pixel, int rgb_order, int red_shift,
				  int green_shift, int blue_shift, int red_mask, int green_mask,
				  int blue_mask)
{
    unsigned long red, green, blue;
#include "conv.c"
    red /= (256 * 3);
    green /= (256 * 3);
    blue /= (256 * 3);
    return (red << red_shift) | (green << green_shift) | (blue << blue_shift);
}

static unsigned long aa_convolve_1 (int i, int j, unsigned char *source,
				    int source_bytes_per_line,
                                    int byte_order, int bytes_per_pixel)
{
    if (byte_order == MSBFirst) {
	switch (bytes_per_pixel) {
	case 1:
	    return S1M (i, j);
	case 2:
	    return S2M (i, j);
	case 3:
	    return S3M (i, j);
	case 4:
	    return S4M (i, j);
	}
    } else {
	switch (bytes_per_pixel) {
	case 1:
	    return S1L (i, j);
	case 2:
	    return S2L (i, j);
	case 3:
	    return S3L (i, j);
	case 4:
	    return S4L (i, j);
	}
    }
    return 0;
}

/* fourth level */
static Pixmap aa_shrink_pixmap (struct aa_font_cache *f, Pixmap pixmap, int width, int height)
{
    XImage *image, *shrunk;
    int i, j, w, h, bytes_per_pixel;
    int imw, imh;

    imw = f->scale == 1 ? width : (width + 4 + X_ENLARGEMENT + option_interchar_spacing * f->scale);
    imh = f->scale == 1 ? height : (height + 8 + Y_ENLARGEMENT);
/* create an image to put the enlarged glyph into - make it slightly large to hold
   the diameter of the 5x9 guassian as well as a one pixel enlargement and rounding error */
    image = XCreateImage (aa_display, aa_visual, aa_depth, ZPixmap, 0, 0, imw, imh, 8, 0);
    bytes_per_pixel = image->bytes_per_line / image->width;
    image->data = (char *) malloc (image->bytes_per_line * image->height);
    memset(image->data, 0, image->bytes_per_line * image->height);

    for (i = 0; i < imw; i++)
	XPutPixel (image, i, 0, f->bg);
    for (j = 1; j < imh; j++)
	memcpy (image->data + image->bytes_per_line * j, image->data, image->bytes_per_line);

/* create an image to put the reduced glyph into. round w and h up */
    w = SHRINK_WIDTH (width, f->scale);
    h = SHRINK_HEIGHT (height, f->scale);
    shrunk = XCreateImage (aa_display, aa_visual, aa_depth, ZPixmap, 0, 0, w, h, 8, 0);
    shrunk->data = (char *) malloc (shrunk->bytes_per_line * h);
    memset(shrunk->data, 0, shrunk->bytes_per_line * h);

    if (f->scale == 3) {
        int red_shift, green_shift, blue_shift;
        unsigned long red_mask, green_mask, blue_mask;

        for (red_mask = image->red_mask, red_shift = 0; red_shift < 32 && !(red_mask & 1);
	    red_shift++, red_mask >>= 1);
        for (green_mask = image->green_mask, green_shift = 0; green_shift < 32 && !(green_mask & 1);
	    green_shift++, green_mask >>= 1);
        for (blue_mask = image->blue_mask, blue_shift = 0; blue_shift < 32 && !(blue_mask & 1);
	    blue_shift++, blue_mask >>= 1);

        XGetSubImage (aa_display, pixmap, 0, 0, width, height,
		      image->red_mask | image->green_mask | image->blue_mask, ZPixmap, image, 2, 4);

        for (i = 0; i < w; i++) {
	    for (j = 0; j < h; j++) {
	        unsigned long pixel;
	        pixel =
		    aa_convolve_3 (i * 3, j * 3,
			        (unsigned char *) image->data + bytes_per_pixel * 2 +
			        image->bytes_per_line * 4, image->bytes_per_line, image->byte_order,
			        bytes_per_pixel, option_rgb_order, red_shift, green_shift, blue_shift,
			        red_mask, green_mask, blue_mask);
	        XPutPixel (shrunk, i, j, pixel);
	    }
        }
    } else {
        XGetSubImage (aa_display, pixmap, 0, 0, width, height,
		      image->red_mask | image->green_mask | image->blue_mask, ZPixmap, image, 0, 0);

        for (i = 0; i < w; i++) {
	    for (j = 0; j < h; j++) {
	        unsigned long pixel;
	        pixel = aa_convolve_1 (i, j, (unsigned char *) image->data, image->bytes_per_line, image->byte_order, bytes_per_pixel);
	        XPutPixel (shrunk, i, j, pixel);
	    }
        }
    }

    pixmap = XCreatePixmap (aa_display, aa_root, w, h, aa_depth);
    XPutImage (aa_display, pixmap, f->gc, shrunk, 0, 0, 0, 0, w, h);
    free (image->data);
    image->data = 0;
    XDestroyImage (image);
    free (shrunk->data);
    shrunk->data = 0;
    XDestroyImage (shrunk);
    return pixmap;
}

/* third level */
static void aa_create_pixmap (struct aa_font_cache *f, int j, int i, struct aa_glyph_cache *glyph, int metrics_only)
{
    Pixmap w;
    int direction, ascent, descent, height;
    XCharStruct ch;
    XChar2b c;
    c.byte1 = j;
    c.byte2 = i;
    XTextExtents16 (f->f->font_struct, &c, 1, &direction, &ascent, &descent, &ch);
    glyph->width = SHRINK_WIDTH (ch.width, f->scale);
    glyph->descent = ch.descent / f->scale;

    if (metrics_only)
        return;
    height = f->f->font_struct->ascent + f->f->font_struct->descent;
    w = XCreatePixmap (aa_display, aa_root, ch.width, height, aa_depth);
/* cheapest way to clear the background */
    XDrawImageString (aa_display, w, f->gc, 0, f->f->font_struct->ascent, "     ", 5);
/* needed to clear the background if the function fails on non-existing chars */
    XDrawImageString16 (aa_display, w, f->gc, 0, f->f->font_struct->ascent, &c, 1);
    glyph->pixmap = aa_shrink_pixmap (f, w, ch.width, height);
    XFreePixmap (aa_display, w);
}

static Pixmap aa_render_glyph (GC fgc, long font_fg, long font_bg, int dx, int dy, FT_Bitmap *bitmap, FT_Glyph_Metrics *metrics, int u, int U, int w, int h, int W, int H, int blank);


#ifdef MAP_WINDOWS
static int windows_mapping[32] = {
/* 0x80 */    0x0000, /*  */
/* 0x81 */    0x0000, /*  */
/* 0x82 */    0x201A, /* #SINGLE LOW-9 QUOTATION MARK */
/* 0x83 */    0x0192, /* #LATIN SMALL LETTER F WITH HOOK */

/* 0x84 */    0x201E, /* #DOUBLE LOW-9 QUOTATION MARK */
/* 0x85 */    0x2026, /* #HORIZONTAL ELLIPSIS */
/* 0x86 */    0x2020, /* #DAGGER */
/* 0x87 */    0x2021, /* #DOUBLE DAGGER */

/* 0x88 */    0x02C6, /* #MODIFIER LETTER CIRCUMFLEX ACCENT */
/* 0x89 */    0x2030, /* #PER MILLE SIGN */
/* 0x8A */    0x0160, /* #LATIN CAPITAL LETTER S WITH CARON */
/* 0x8B */    0x2039, /* #SINGLE LEFT-POINTING ANGLE QUOTATION MARK */

/* 0x8C */    0x0152, /* #LATIN CAPITAL LIGATURE OE */
/* 0x8D */    0x0000, /*  */
/* 0x8E */    0x0000, /*  */
/* 0x8F */    0x0000, /*  */

/* 0x90 */    0x0000, /*  */
/* 0x91 */    0x2018, /* #LEFT SINGLE QUOTATION MARK */
/* 0x92 */    0x2019, /* #RIGHT SINGLE QUOTATION MARK */
/* 0x93 */    0x201C, /* #LEFT DOUBLE QUOTATION MARK */

/* 0x94 */    0x201D, /* #RIGHT DOUBLE QUOTATION MARK */
/* 0x95 */    0x2022, /* #BULLET */
/* 0x96 */    0x2013, /* #EN DASH */
/* 0x97 */    0x2014, /* #EM DASH */

/* 0x98 */    0x02DC, /* #SMALL TILDE */
/* 0x99 */    0x2122, /* #TRADE MARK SIGN */
/* 0x9A */    0x0161, /* #LATIN SMALL LETTER S WITH CARON */
/* 0x9B */    0x203A, /* #SINGLE RIGHT-POINTING ANGLE QUOTATION MARK */

/* 0x9C */    0x0153, /* #LATIN SMALL LIGATURE OE */
/* 0x8D */    0x0000, /*  */
/* 0x8E */    0x0000, /*  */
/* 0x9F */    0x0178, /* #LATIN CAPITAL LETTER Y WITH DIAERESIS */
};
#endif


const char *hex_chars[16][7] = {
{
   " xx ",
   "x  x",
   "x  x",
   "x  x",
   "x  x",
   "x  x",
   " xx ",
},                
{
   "  x ",
   " xx ",
   "  x ",
   "  x ",
   "  x ",
   "  x ",
   "  x ",
},
{
   " xx ",
   "x  x",
   "   x",
   "  x ",
   " x  ",
   "x   ",
   "xxxx",
},
{
   " xx ",
   "x  x",
   "   x",
   " xx ",
   "   x",
   "x  x",
   " xx ",
},
{
   " xx ",
   "x x ",
   "x x ",
   "x x ",
   "xxxx",
   "  x ",
   "  x ",
},
{
   "xxxx",
   "x   ",
   "xxx ",
   "   x",
   "   x",
   "x  x",
   " xx ",
},
{
   " xx ",
   "x  x",
   "x   ",
   "xxx ",
   "x  x",
   "x  x",
   " xx ",
},
{
   "xxxx",
   "x  x",
   "   x",
   "  x ",
   " x  ",
   " x  ",
   " x  ",
},
{
   " xx ",
   "x  x",
   "x  x",
   " xx ",
   "x  x",
   "x  x",
   " xx ",
},
{
   " xx ",
   "x  x",
   "x  x",
   " xxx",
   "   x",
   "   x",
   " xx ",
},
{
   " xx ",
   "x  x",
   "x  x",
   "x  x",
   "xxxx",
   "x  x",
   "x  x",
},
{
   "xxx ",
   "x  x",
   "x  x",
   "xxx ",
   "x  x",
   "x  x",
   "xxx ",
},
{
   " xx ",
   "x  x",
   "x   ",
   "x   ",
   "x   ",
   "x  x",
   " xx ",
},
{
   "xxx ",
   "x  x",
   "x  x",
   "x  x",
   "x  x",
   "x  x",
   "xxx ",
},
{
   "xxxx",
   "x   ",
   "x   ",
   "xxx ",
   "x   ",
   "x   ",
   "xxxx",
},
{
   "xxxx",
   "x   ",
   "x   ",
   "xxx ",
   "x   ",
   "x   ",
   "x   ",
}};

static Pixmap aa_render_glyph (GC fgc, long font_fg, long font_bg, int dx, int dy, FT_Bitmap *bitmap, FT_Glyph_Metrics *metrics, int u, int U, int w, int h, int W, int H, int blank);
int load_one_freetype_font (FT_Face *face, const char *filename, int *desired_height, int *loaded_height);

/* third level */
static void aa_create_pixmap_freetype (struct aa_font_cache *f, unsigned long the_chr, struct aa_glyph_cache *glyph, int metrics_only)
{
    int found = 0, font_i;
    int h, H, w, W;
    int U, u;

    FT_Face face;
    FT_Glyph_Metrics *metrics;

    int error, dx = 0, dy = 0;

#ifdef MAP_WINDOWS
  retry_with_windows_mapping:
#endif

    for (font_i = 0; font_i < f->f->font_freetype.n_fonts; font_i++) {
        unsigned long t;
        int g;
        struct freetype_cache *cache;
        const struct freetype_cache *const_cache;
        t = the_chr == ' ' ? 'f' : the_chr;
        const_cache = &f->f->font_freetype.faces[font_i];
        memcpy(&cache, &const_cache, sizeof(cache));    /* get around const warning for this special case */
        if (cache->load_failed)
            continue;

        if (!cache->face) {
            int desired_height;
            desired_height = f->f->font_freetype.desired_height;
            if (load_one_freetype_font ((FT_Face *) &cache->face, cache->freetype_fname, &desired_height, &cache->loaded_height)) {
                cache->load_failed = 1;
                continue;
            }


#if 0
/* #warning remove debug code */
if (t == 0x82) {
volatile unsigned long *u;
u = 1000000000;
*u = -1;
}
#endif

#if 0
            {
                FT_UInt agindex = 0;
                long start = -1, prev = -1, next = -1;
                long max_block_from = 0;
                long max_block_to = 0;
    
                next = (long) FT_Get_First_Char ((FT_Face) cache->face, &agindex);
                while (agindex) {
                    agindex = 0;
                    next = FT_Get_Next_Char ((FT_Face) cache->face, next, &agindex);
                    if (agindex && (next == prev + 1 || prev == -1)) {
                        prev = next;
                        continue;
                    }
                    if (start == -1 || start == prev) {
                        start = next;
                    } else {
                        if (max_block_to - max_block_from < prev - start) {
                            max_block_to = prev;
                            max_block_from = start;
                        }
                        start = -1;
                    }
                    prev = next;
                }
                printf("Loaded font %s (nom=%d real=%d) in attempt to find unicode code point 0x%X. (0x%lX-0x%lX etc.)\n", cache->freetype_fname, desired_height, cache->loaded_height, (unsigned int) t, max_block_from, max_block_to);
            }
#endif

            printf("Loaded font %s (nom=%d real=%d) in attempt to find unicode code point 0x%X.\n", cache->freetype_fname, desired_height, cache->loaded_height, (unsigned int) t);

        }

        face = (FT_Face) cache->face;
        if ((g = FT_Get_Char_Index(face, t))) {
#ifdef FT_LOAD_COLOR
            error = FT_Load_Glyph(face, g, FT_LOAD_COLOR);
#else
#error Color fonts not supported in your installation of the FreeType library.
#error Please update your FreeType library libfreetype.so to the latest stable version.
#error As an alternative just remove these lines and continue compiling -- it is fine.
            error = FT_Load_Glyph(face, g, FT_LOAD_DEFAULT);
#endif
            if (!error && face->glyph) {
                error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
                if (!error && face->glyph->bitmap.width) {
                    found = 1;
                    U = cache->loaded_height;
                    break;
                }
            }
        }
    }

    if (!found) {
#ifdef MAP_WINDOWS
        if (the_chr >= 0x80 && the_chr <= 0x9F) {
            the_chr = windows_mapping[the_chr - 0x80];
            if (the_chr)
                goto retry_with_windows_mapping;
        }
#endif
        glyph->width = 0;
        glyph->descent = 0;
        return;
    }

    metrics = &face->glyph->metrics;
    if (metrics->horiAdvance < metrics->width) { /* for example characters 000f96 000faa 00135d have metrics->horiAdvance less than zero */
        W = metrics->width / 64;
        dx = 0;
    } else {
        W = metrics->horiAdvance / 64;
        dx = metrics->horiBearingX / 64;
    }

    u = f->f->font_freetype.desired_height;

    if (U <= u)
        U = u;

    h = f->f->font_freetype.measured_height;
    H = h * U / u;

    w = glyph->width = W * u / U;
    glyph->descent = (metrics->height - metrics->horiBearingY) * u / U / 64;
    dy = f->f->font_freetype.measured_ascent * U / u - metrics->horiBearingY / 64;

    if (!metrics_only)
        glyph->pixmap = aa_render_glyph (f->gc, f->fg, f->bg, dx, dy, &face->glyph->bitmap, metrics, u, U, w, h, W, H, the_chr == ' ');
}


static Pixmap aa_render_glyph (GC fgc, long font_fg, long font_bg, int dx, int dy, FT_Bitmap *bitmap, FT_Glyph_Metrics *metrics, int u, int U, int w, int h, int W, int H, int blank)
{
    XImage *shrunk;
    Pixmap pixmap;
    int i, j;
    int inverse = 0;
    int red_shift, green_shift, blue_shift;
    unsigned long red_mask, green_mask, blue_mask;
    unsigned long r_bg, g_bg, b_bg;
    unsigned long r_fg, g_fg, b_fg;

/* create an image to put the reduced glyph into. round w and h up */
    shrunk = XCreateImage (aa_display, aa_visual, aa_depth, ZPixmap, 0, 0, w, h, 8, 0);
    shrunk->data = (char *) malloc (shrunk->bytes_per_line * h);
    memset(shrunk->data, 0, shrunk->bytes_per_line * h);

    for (red_mask = shrunk->red_mask, red_shift = 0; red_shift < 32 && !(red_mask & 1);
        red_shift++, red_mask >>= 1);
    for (green_mask = shrunk->green_mask, green_shift = 0; green_shift < 32 && !(green_mask & 1);
        green_shift++, green_mask >>= 1);
    for (blue_mask = shrunk->blue_mask, blue_shift = 0; blue_shift < 32 && !(blue_mask & 1);
        blue_shift++, blue_mask >>= 1);

    if (inverse) {
        long sw;
        sw = font_fg;
        font_fg = font_bg;
        font_bg = sw;
    }

    r_fg = ((font_fg >> red_shift) & red_mask);
    g_fg = ((font_fg >> green_shift) & green_mask);
    b_fg = ((font_fg >> blue_shift) & blue_mask);

    r_bg = ((font_bg >> red_shift) & red_mask);;
    g_bg = ((font_bg >> green_shift) & green_mask);
    b_bg = ((font_bg >> blue_shift) & blue_mask);

/* if the glyph is trying to draw itself outside the bounds of the XImage then we constrain it to the furthest edge */
    if (dy > h * U / u - bitmap->rows)
        dy = h * U / u - bitmap->rows;
    if (dy < 0)
        dy = 0;
    if (dx > w * U / u - bitmap->width)
        dx = w * U / u - bitmap->width;
    if (dx < 0)
        dx = 0;

#define DECLM \
    unsigned long grey = 0; \
    unsigned long r, g, b; \
    unsigned long pixel; \
    int ii, jj

#define RGB(s) \
    r = (r_fg * grey / (s)) + (r_bg * ((s) - grey) / (s)); \
    g = (g_fg * grey / (s)) + (g_bg * ((s) - grey) / (s)); \
    b = (b_fg * grey / (s)) + (b_bg * ((s) - grey) / (s)); \
    pixel = (r << red_shift) | (g << green_shift) | (b << blue_shift); \
    XPutPixel (shrunk, i, j, pixel)

#define RGBC \
    r = (r * red_mask   + (255 - grey) * r_bg) / 255; \
    g = (g * green_mask + (255 - grey) * g_bg) / 255; \
    b = (b * blue_mask  + (255 - grey) * b_bg) / 255; \
    pixel = (r << red_shift) | (g << green_shift) | (b << blue_shift); \
    XPutPixel (shrunk, i, j, pixel)

#define BITM(s1,s7,s8) \
    do { \
        if (bitmap->pitch >= 0) { \
            DECLM; \
            for (i = 0; i < w; i++) { \
                jj = j - dy; \
                ii = i - dx; \
                if (jj >= 0 && ii >= 0 && jj < bitmap->rows && ii < bitmap->width && !blank) { \
                    grey = ((bitmap->buffer[jj * bitmap->pitch + (ii / s8)] >> (s7 - (ii % s8))) & s1); \
                    RGB(s1); \
                } else { \
                    grey = 0; \
                    RGB(s1); \
                } \
            } \
        } else { \
            for (i = 0; i < w; i++) { \
                DECLM; \
                jj = j - dy; \
                ii = i - dx; \
                if (jj >= 0 && ii >= 0 && jj < bitmap->rows && ii < bitmap->width && !blank) { \
                    grey = ((bitmap->buffer[(h - 1 - jj) * (-bitmap->pitch) + (ii / s8)] >> (s7 - (ii % s8))) & s1); \
                    RGB(s1); \
                } else { \
                    grey = 0; \
                    RGB(s1); \
                } \
            } \
        } \
    } while (0)

#define BIT255 \
    do { \
        if (bitmap->pitch >= 0) { \
            DECLM; \
            for (i = 0; i < w; i++) { \
                jj = j - dy; \
                ii = i - dx; \
                if (jj >= 0 && ii >= 0 && jj < bitmap->rows && ii < bitmap->width && !blank) { \
                    grey = bitmap->buffer[jj * bitmap->pitch + ii]; \
                    RGB(255); \
                } else { \
                    grey = 0; \
                    RGB(255); \
                } \
            } \
        } else { \
            DECLM; \
            for (i = 0; i < w; i++) { \
                jj = j - dy; \
                ii = i - dx; \
                if (jj >= 0 && ii >= 0 && jj < bitmap->rows && ii < bitmap->width && !blank) { \
                    grey = bitmap->buffer[(h - 1 - jj) * (-bitmap->pitch) + ii]; \
                    RGB(255); \
                } else { \
                    grey = 0; \
                    RGB(255); \
                } \
            } \
        } \
    } while (0)

/*


W=7
w=3
i=1

        0              1              2              3              4              5              6
+--------------+--------------+--------------+--------------+--------------+--------------+--------------+
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
+--------------+--------------+--------------+--------------+--------------+--------------+--------------+
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
|              |              |     . . . . .|. . . . . . . |. . . . .     |              |              |
|              |              |     .        |              |        .     |              |              |
+--------------+--------------+-----.--------+--------------+--------.-----+--------------+--------------+
|              |              |     .        |              |        .     |              |              |
|              |              |     .        |              |        .     |              |              |
|              |              |     .        |    W / w     |        .     |              |              |
|              |              |     .<-----------------------------> .     |              |              |
|              |              |     .        |              |        .     |              |              |
|              |              |     .        |              |        .     |              |              |
+--------------+--------------+-----.--------+--------------+--------.-----+--------------+--------------+
|              |              |     .        |              |        .     |              |              |
|              |              |     .        |              |        .     |              |              |
|              |              |     .        |              |        .     |              |              |
|              |              |     .        |              |        .     |              |              |
|              |              |     .        |              |        .     |              |              |
|              |              |     .        |              |        .     |              |              |
+--------------+--------------+-----.--------+--------------+--------.-----+--------------+--------------+
|              |              |     .        |              |        .     |              |              |
|              |              |     . . . . . . . . . . . . |. . . . .     |              |              |
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
|              |              |              |              |              |              |              |
+--------------+--------------+--------------+--------------+--------------+--------------+--------------+

                               ^^^^^^^^^^^^^^                ^^^^^^^^^^^^^^
                                i * W / w                  i * W / w + W / w


test1:

// x1 = 1 * 7 / 3
// x1 = 2
// x2 = (1 + 1) * 7 / 3
// x2 = 4
// 
// A = (256 * i * W / w)
// A = 597
// B = (256 * (i * W / w))
// B = 512
// (A - B) = 85
// 255 - (A - B) = 170


test2:

// A = (256 * (i + 1) * W / w)
// A = 1194
// B = (256 * ((i + 1) * W / w))
// B = 1024
// (A - B) = 170


algorithm:

for (j = 0; j < w; j++) {
    for (i = 0; i < w; i++) {
        int c3 = 0;
        for (y = j * H / h; y <= (j + 1) * H / h; y++) 
        {
            int c2 = 0;
            for (x = i * W / w; x <= (i + 1) * W / w; x++) 
            {
                int c1 = color[y][x];
                if (x == i * W / w) {
                    c2 += c1 * (255 - ((256 * (i * W / w)) - (256 * i * W / w))) / 255;
                } else if (x == (i + 1) * W / w) {
                    c2 += c1 * ((256 * (i + 1) * W / w) - (256 * ((i + 1) * W / w))) / 255;
                } else {
                    c2 += c1;
                }
            }
            c2 = c2 * w / W;
            if (y == j * H / h) {
                c3 += c2 * (255 - ((256 * (j * H / h)) - (256 * j * H / h))) / 255;
            } else if (y == (j + 1) * H / h) {
                c3 += c2 * ((256 * (j + 1) * H / h) - (256 * ((j + 1) * H / h))) / 255;
            } else {
                c3 += c2;
            }
        }
        c3 = c3 * h / H;
        new_color[j][i] = c3;
    }
}

*/


#define SUM_COLOR(result, color_y_x) \
    do { \
        int x, y; \
        int c3 = 0, C3 = 0; \
        int Y1, Y2, X1, X2; \
        X1 = ii * U / u - dx; \
        X2 = (ii + 1) * U / u - dx; \
        Y1 = jj * U / u - dy; \
        Y2 = (jj + 1) * U / u - dy; \
        for (y = Y1; y <= Y2; y++)  \
        { \
            int c2 = 0, C2 = 0; \
            for (x = X1; x <= X2; x++)  \
            { \
                int c1, C1; \
                if (!(x >= 0 && y >= 0 && x < bitmap->width && y < bitmap->rows)) \
                    continue; \
                c1 = color_y_x; \
                C1 = 255; \
                if (x == X1) { \
                    c2 += c1 * (255 - ((256 * ii * U / u) - (256 * (ii * U / u)))) / 255; \
                    C2 += C1 * (255 - ((256 * ii * U / u) - (256 * (ii * U / u)))) / 255; \
                } else if (x == X2) { \
                    c2 += c1 * ((256 * (ii + 1) * U / u) - (256 * ((ii + 1) * U / u))) / 255; \
                    C2 += C1 * ((256 * (ii + 1) * U / u) - (256 * ((ii + 1) * U / u))) / 255; \
                } else { \
                    c2 += c1; \
                    C2 += C1; \
                } \
            } \
            if (y == Y1) { \
                c3 += c2 * (255 - ((256 * jj * U / u) - (256 * (jj * U / u)))) / 255; \
                C3 += C2 * (255 - ((256 * jj * U / u) - (256 * (jj * U / u)))) / 255; \
            } else if (y == Y2) { \
                c3 += c2 * ((256 * (jj + 1) * U / u) - (256 * ((jj + 1) * U / u))) / 255; \
                C3 += C2 * ((256 * (jj + 1) * U / u) - (256 * ((jj + 1) * U / u))) / 255; \
            } else { \
                c3 += c2; \
                C3 += C2; \
            } \
        } \
        if (C3 > 0) \
            result = c3 * 255 / C3; \
        else \
            result = 0; \
    } while (0)


#define BITC \
    do { \
        if (bitmap->pitch >= 0) { \
            DECLM; \
            for (i = 0; i < w; i++) { \
                jj = j; \
                ii = i; \
                if (!blank) { \
                    SUM_COLOR(b, bitmap->buffer[y * bitmap->pitch + x * 4 + 0]); \
                    SUM_COLOR(g, bitmap->buffer[y * bitmap->pitch + x * 4 + 1]); \
                    SUM_COLOR(r, bitmap->buffer[y * bitmap->pitch + x * 4 + 2]); \
                    SUM_COLOR(grey, bitmap->buffer[y * bitmap->pitch + x * 4 + 3]); \
                    RGBC; \
                } else { \
                    r = g = b = grey = 0; \
                    RGBC; \
                } \
            } \
        } else { \
            DECLM; \
            for (i = 0; i < w; i++) { \
                jj = j; \
                ii = i; \
                if (!blank) { \
                    SUM_COLOR(b, bitmap->buffer[(h - 1 - y) * (-bitmap->pitch) + x * 4 + 0]); \
                    SUM_COLOR(g, bitmap->buffer[(h - 1 - y) * (-bitmap->pitch) + x * 4 + 1]); \
                    SUM_COLOR(r, bitmap->buffer[(h - 1 - y) * (-bitmap->pitch) + x * 4 + 2]); \
                    SUM_COLOR(grey, bitmap->buffer[(h - 1 - y) * (-bitmap->pitch) + x * 4 + 3]); \
                    RGBC; \
                } else { \
                    r = g = b = grey = 0; \
                    RGBC; \
                } \
            } \
        } \
    } while (0)

#define BIT255S \
    do { \
        if (bitmap->pitch >= 0) { \
            DECLM; \
            for (i = 0; i < w; i++) { \
                jj = j; \
                ii = i; \
                if (!blank) { \
                    SUM_COLOR(grey, bitmap->buffer[y * bitmap->pitch + x]); \
                    RGB(255); \
                } else { \
                    grey = 0; \
                    RGB(255); \
                } \
            } \
        } else { \
            DECLM; \
            for (i = 0; i < w; i++) { \
                jj = j; \
                ii = i; \
                if (!blank) { \
                    SUM_COLOR(grey, bitmap->buffer[(h - 1 - y) * (-bitmap->pitch) + x]); \
                    RGB(255); \
                } else { \
                    grey = 0; \
                    RGB(255); \
                } \
            } \
        } \
    } while (0)

#define FGZERO \
    do { \
        for (i = 0; i < w; i++) { \
            DECLM; \
            (void) ii; \
            (void) jj; \
            grey = 0; \
            RGB(1); \
        } \
    } while (0)

    if (U == u) {
        for (j = 0; j < h; j++) {
            switch (bitmap->pixel_mode) {
            case FT_PIXEL_MODE_MONO:
                BITM(1,7,8);
                break;
            case FT_PIXEL_MODE_GRAY2:
                BITM(3,3,4);
                break;
            case FT_PIXEL_MODE_GRAY4:
                BITM(15,1,2);
                break;
            case FT_PIXEL_MODE_GRAY:
                BIT255;
                break;
#ifdef FT_LOAD_COLOR
            case FT_PIXEL_MODE_BGRA:
                BITC;
                break;
#endif
            default:
                FGZERO;
                break;
            }
        }
    } else {
        for (j = 0; j < h; j++) {
            switch ((int) bitmap->pixel_mode) {
            case FT_PIXEL_MODE_MONO:
                BITM(1,7,8);
                break;
            case FT_PIXEL_MODE_GRAY2:
                BITM(3,3,4);
                break;
            case FT_PIXEL_MODE_GRAY4:
                BITM(15,1,2);
                break;
            case FT_PIXEL_MODE_GRAY:
                BIT255S;
                break;
#ifdef FT_LOAD_COLOR
            case FT_PIXEL_MODE_BGRA:
                BITC;
                break;
#endif
            default:
                FGZERO;
                break;
            }
        }
    }

    pixmap = XCreatePixmap (aa_display, aa_root, w, h, aa_depth);
    XPutImage (aa_display, pixmap, fgc, shrunk, 0, 0, 0, 0, w, h);

    free (shrunk->data);
    shrunk->data = 0;
    XDestroyImage (shrunk);

    return pixmap;
}

/* second level */
static void aa_create_pixmap_ (struct aa_font_cache *f, int j, int i, int metrics_only)
{
    if (!f->glyph[j]) {
        int i;
	f->glyph[j] = malloc (sizeof (struct aa_glyph_cache) * 256);
        for (i = 0; i < 256; i++) {
            memset(&f->glyph[j][i], 0, sizeof(f->glyph[j][i]));
            f->glyph[j][i].width = -1;
        }
    }
    if (metrics_only) {
        if (f->glyph[j][i].width == -1) {
            if (f->f->font_freetype.n_fonts)
                aa_create_pixmap_freetype (f, (j << 8) | i, &f->glyph[j][i], 1);
            else
                aa_create_pixmap (f, j, i, &f->glyph[j][i], 1);
        }
    } else {
        if (!f->glyph[j][i].pixmap || f->glyph[j][i].width == -1) {
            if (f->f->font_freetype.n_fonts)
                aa_create_pixmap_freetype (f, (j << 8) | i, &f->glyph[j][i], 0);
            else
                aa_create_pixmap (f, j, i, &f->glyph[j][i], 0);
        }
    }
}

int _XAaDrawImageStringWC (Display * display, Drawable d, GC gc, const struct aa_font *aa_font, int x, int y, const char *s,
			   XChar2b * wc, C_wchar_t * swc, int length, int *descent_r, int scale, int metrics_only)
{
    int i, x_start = x;
    int descent = -10240;
    struct aa_font_cache *f;
    XGCValues values_return;

    if (metrics_only) {
        f = aa_find_metrics_only(aa_font->load_id);
    } else {
        XGetGCValues (display, gc, GCForeground | GCBackground, &values_return);
        f = aa_find (aa_font->load_id, values_return.foreground, values_return.background);
    }

    if (!f) {
	aa_insert ();
	f = font_cache_list;
	f->f = aa_font;
        f->scale = scale;
        if (!metrics_only)
	    aa_display = display;
    }

    if (!metrics_only) {
        assert(!f->gc || f->gc == gc);
        assert(!f->fg || f->fg == values_return.foreground);
        assert(!f->bg || f->bg == values_return.background);
        f->gc = gc;
        f->fg = values_return.foreground;
        f->bg = values_return.background;
    }

    if (swc) {

#define XCOPYAREA_GLYPH(X,Y) \
        do { \
            int page, width, height; \
            unsigned char chr; \
            struct aa_glyph_cache *gl; \
            page = X; \
            chr = Y; \
	    aa_create_pixmap_ (f, page, chr, metrics_only); \
            gl = &f->glyph[page][chr]; \
            width = gl->width; \
            if (descent < (int) gl->descent) \
                descent = (int) gl->descent; \
            if (f->f->font_struct) \
                height = SHRINK_HEIGHT (f->f->font_struct->ascent + f->f->font_struct->descent, scale); \
            else \
                height = f->f->font_freetype.measured_height; \
            if (width && !metrics_only) { \
                if (f->f->font_struct) \
	            XCopyArea (display, gl->pixmap, d, gc, 0, 0, width, height, x, y - f->f->font_struct->ascent / scale); \
                else \
	            XCopyArea (display, gl->pixmap, d, gc, 0, 0, width, height, x, y - f->f->font_freetype.measured_ascent); \
            } \
	    x += width; \
        } while (0)

	for (i = 0; i < length; i++) {
            unsigned int c = swc[i];
            if (c <= FONT_LAST_UNICHAR) {
                XCOPYAREA_GLYPH(c >> 8, c & 0xFF);
            }
	}
    } else if (wc) {
	for (i = 0; i < length; i++) {
            XCOPYAREA_GLYPH(wc[i].byte1, wc[i].byte2);
	}
    } else {
	for (i = 0; i < length; i++) {
            XCOPYAREA_GLYPH(0, (unsigned char) s[i]);
	}
    }
    if (descent_r)
        *descent_r = descent;
    return x - x_start;
}

int XAaTextWidth (const struct aa_font *f, const char *s, int length, int *descent, int scale)
{
    return _XAaDrawImageStringWC (0, 0, 0, f, 0, 0, s, 0, 0, length, descent, scale, 1);
}

int XAaTextWidth16 (const struct aa_font *f, XChar2b * s, C_wchar_t * swc, int length, int *descent, int scale)
{
    return _XAaDrawImageStringWC (0, 0, 0, f, 0, 0, 0, s, swc, length, descent, scale, 1);
}

int XAaDrawImageString16 (Display * display, Drawable d, GC gc, const struct aa_font *f, int x, int y, XChar2b * wc, C_wchar_t * swc,
			  int length, int scale)
{
    return _XAaDrawImageStringWC (display, d, gc, f, x, y, 0, wc, swc, length, 0, scale, 0);
}

int XAaDrawImageString (Display * display, Drawable d, GC gc, const struct aa_font *f, int x, int y, char *s, int length, int scale)
{
    return _XAaDrawImageStringWC (display, d, gc, f, x, y, s, 0, 0, length, 0, scale, 0);
}

void XAaFree (int load_id)
{
    aa_remove (load_id);
}

