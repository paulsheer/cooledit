/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* font.c
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include "coolwidget.h"
#include "coollocal.h"
#include "aafont.h"
#include "stringtools.h"
#include "font.h"



#ifndef NO_TTF
#define HAVE_FREETYPE
#else
#undef HAVE_FREETYPE
#endif


const char *font_error_string = "Use <x-font-name>/3 or <x-font-name>/1 or <font-file>:NN. For example 12x24/3 or Helvetica.ttf:12 -- for 12 pixels height\n";

#ifdef HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H

static int last_font_load_id = 1;
static void utf8_to_wchar_t (const unsigned char *s, int l, C_wchar_t ** r_ret, int *l_ret, enum font_encoding e);


int load_one_freetype_font (FT_Face *face, const char *filename, int *desired_height, int *loaded_height)
{E_
    static FT_Library library;
    static int initialized = 0;
    int i, nominal_height, size_index = -1;
    int closest;

    if (!filename) {
        if (initialized) {
            FT_Done_FreeType(library);
            initialized = 0;
        }
        return 0;
    }

    if (!initialized) {
	initialized = 1;
	if (FT_Init_FreeType (&library)) {
	    fprintf (stderr, "Error initializing fretype library\n");
	    return 1;
        }
    }

    if (FT_New_Face (library, filename, 0, face)) {
	fprintf (stderr, "Font %s could not be loaded.\n%s", filename, font_error_string);
	return 1;
    }

    if (*desired_height) {
        nominal_height = *desired_height;
    } else {     /* no nominal_height was requested on the command line, so try work out viable heights from the font: */
        int preferred_heights[] = {13, 14, 12, 15, 11, 16, 10, 17, 9};
        nominal_height = 0;
        if (!(*face)->num_fixed_sizes) {
            nominal_height = preferred_heights[0];        /* any size is allowed */
        } else if ((*face)->num_fixed_sizes == 1) {
            nominal_height = (*face)->available_sizes[0].height;
            size_index = 0;
        } else {
            int j;
            for (j = 0; j < sizeof (preferred_heights) / sizeof (preferred_heights[0]); j++) {
                for (i = 0; i < (*face)->num_fixed_sizes; i++) {
                    if (preferred_heights[j] == (*face)->available_sizes[i].height) {
                        nominal_height = preferred_heights[j];
                        size_index = j;
                        goto out;
                    }
                }
            }
        }
      out:
        if (!nominal_height) {
	    fprintf (stderr, "Font %s has no obvious height I could guess you should use.\n%s", filename, font_error_string);
	    FT_Done_Face(*face);
            *face = 0;
            return 1;
        }
    }

    /* find the closest size larger than height: */
    if (size_index == -1) {
        closest = 1024000;
        for (i = 0; i < (*face)->num_fixed_sizes; i++) {
            if ((*face)->available_sizes[i].height >= nominal_height) {
                if (closest > (*face)->available_sizes[i].height) {
                    closest = (*face)->available_sizes[i].height;
                    size_index = i;
                }
            }
        }
    }

    /* if not found, find the closest size smaller than height: */
    if (size_index == -1) {
        closest = 0;
        for (i = 0; i < (*face)->num_fixed_sizes; i++) {
            if ((*face)->available_sizes[i].height < nominal_height) {
                if (closest < (*face)->available_sizes[i].height) {
                    closest = (*face)->available_sizes[i].height;
                    size_index = i;
                }
            }
        }
    }

    if (size_index != -1 && !FT_Select_Size((*face), size_index)) {
        nominal_height = (*face)->available_sizes[size_index].height;
    } else if (!FT_Set_Pixel_Sizes((*face), nominal_height, nominal_height)) {
        nominal_height = (*face)->size->metrics.y_ppem;
    } else {
	fprintf (stderr, "Font %s, fail setting size to %d.\n%s", filename, nominal_height, font_error_string);
        if ((*face)->num_fixed_sizes) {
	    fprintf (stderr, "Available sizes:\n");
	    for (i = 0; i < (*face)->num_fixed_sizes; i++) {
	        fprintf (stderr, "  %d\n", (int) (*face)->available_sizes[i].height);
	    }
        }
        FT_Done_Face(*face);
        *face = 0;
        return 1;
    }

    if (!*desired_height)  /* wildcard */
        *desired_height = nominal_height;
    *loaded_height = nominal_height;

    return 0;
}


static int load_font_from_file (const char *fname, struct aa_font *r, int desired_height)
{E_
    const char *arg = fname;
    const char *end_of_name, *next = fname;

    r->font_freetype.desired_height = desired_height;

    for (;;) {
        int l;
        char t[MAX_PATH_LEN] = "";
        char v[MAX_PATH_LEN / 2] = "";
        FT_Face face = 0;
        FILE *fontfile = NULL;
        int loaded_height = 0;

        fname = next;
        if (!fname) {
            if (!r->font_freetype.n_fonts) {
                fprintf(stderr, "%s: No fonts could be loaded.\n%s", arg, font_error_string);
                return 1;
            }
            break;
        }

        if ((end_of_name = strchr(fname, ','))) {
            next = end_of_name + 1;
        }
        else {
            end_of_name = fname + strlen(fname);
            next = NULL;
        }

        l = (int) (end_of_name - fname);
        if (l < 1 || l >= sizeof(v) - 1)
            continue;

        Cstrlcpy (v, fname, l + 1);

        if (!fontfile) {
            snprintf (t, MAX_PATH_LEN, "%s", v);
            fontfile = fopen (t, "r");
        }

        if (!fontfile) {
            snprintf (t, MAX_PATH_LEN, "notosans/%s", v);
            fontfile = fopen (t, "r");
        }

        if (!fontfile) {
            snprintf (t, MAX_PATH_LEN, "%s/%s/%s", LIBDIR, "fonts", v);
            fontfile = fopen (t, "r");
        }

        if (!fontfile) {
            snprintf (t, MAX_PATH_LEN, "%s/%s", "/usr/local/share/fonts/noto", v);
            fontfile = fopen (t, "r");
        }

        if (!fontfile) {
            snprintf (t, MAX_PATH_LEN, "%s/%s", "/usr/local/share/fonts/misc", v);
            fontfile = fopen (t, "r");
        }

        if (!(fontfile)) { /* test if it is a file */
            if (r->font_freetype.n_fonts == 0) {  /* if the first one is not a loadable file, probably the user specified a X Font, so don't print an error */
                return l;
            }
	    fprintf (stderr, "No such file %s in ./, notosans/, %s/%s/, %s/, or %s/.\n", v, LIBDIR, "/fonts", "/usr/local/share/fonts/noto", "/usr/local/share/fonts/misc");
            continue;
        } else {
            fclose(fontfile);
        }

        if (r->font_freetype.n_fonts > sizeof(r->font_freetype.faces) / sizeof(r->font_freetype.faces[0])) {
            fprintf(stderr, "Font %s cannot be loaded -- too many fonts.\n%s\n", t, font_error_string);
            continue;
        }

        if (r->font_freetype.n_fonts >= 1) {    /* just load the name */
            r->font_freetype.faces[r->font_freetype.n_fonts++].freetype_fname = (char *) strdup (t);
            continue;
        }

/* a.0: dnh=24 drh=136 afh=100   notosans/NotoSans-Regular.ttf */

        if (load_one_freetype_font (&face, t, &r->font_freetype.desired_height, &loaded_height)) {
            continue;
        }

        if (!r->font_freetype.measured_height) {

/* To work out the ascent and descent we cannot go on the font general metrics. The reason is that
these metrics assume a huge typographic space above and below the glyph consistent with good
typography for rendering general text. Here we want a minimum space for locating the text within
small buttons and menus. We also want a condensed text editing style. */

            const char *p = "AÄÁÂÅËQZXifhtl1695ypqjg";
            int a_ascent = 0, y_descent = 0;
            C_wchar_t *wc = 0;
            int wc_l = 0, v = 0;
            utf8_to_wchar_t ((const unsigned char *) p, strlen(p), &wc, &wc_l, FONT_ENCODING_UTF8);
            for (v = 0; v < wc_l; v++) {
                if (FT_Get_Char_Index(face, wc[v]) && !FT_Load_Char(face, wc[v], FT_LOAD_RENDER) && face->glyph && face->glyph->bitmap.width) {
                    FT_Glyph_Metrics *metrics;
                    metrics = &face->glyph->metrics;
                    if (a_ascent < metrics->horiBearingY)
                        a_ascent = metrics->horiBearingY;
                    if (y_descent < metrics->height - metrics->horiBearingY)
                        y_descent = metrics->height - metrics->horiBearingY;
                }
            }
            r->font_freetype.measured_height = (a_ascent + y_descent + 31) * r->font_freetype.desired_height / loaded_height / 64;
            r->font_freetype.measured_ascent = a_ascent * r->font_freetype.desired_height / loaded_height / 64;

/* printf("a.2 nom=%d real=%d  asc+desc=%d\n", r->font_freetype.desired_height, loaded_height, r->font_freetype.measured_height); */

        }

        if (!r->font_freetype.measured_height) {
            FT_Done_Face(face);
	    fprintf (stderr, "Font %s does not have a defined height.\n%s", t, font_error_string);
            return 1;
        }

        r->font_freetype.faces[r->font_freetype.n_fonts].freetype_fname = (char *) strdup (t);
        r->font_freetype.faces[r->font_freetype.n_fonts].face = (void *) face;
        r->font_freetype.faces[r->font_freetype.n_fonts].loaded_height = loaded_height;
        r->font_freetype.n_fonts++;
    }

    return 0;
}
#endif

#define TEST_FONT_STRING "The Quick Brown Fox Jumps Over The Lazy Dog"

int option_font_set = 0;

static struct font_stack {
    struct font_object *f;
    struct font_stack *next;
} *font_stack = 0;

static C_wchar_t *wchar_tmp_buf = 0;
static int wchar_tmp_buf_len = 0;

void utf_tmp_buf_free(void)
{E_
    if (wchar_tmp_buf)
        free(wchar_tmp_buf);
}

static void utf8_to_wchar_t (const unsigned char *s, int l, C_wchar_t ** r_ret, int *l_ret, enum font_encoding e)
{E_
    C_wchar_t *c;
    if (wchar_tmp_buf_len < l + 1) {
        wchar_tmp_buf_len = (l + 1) * 2;
        if (wchar_tmp_buf)
            free(wchar_tmp_buf);
        wchar_tmp_buf = malloc (wchar_tmp_buf_len * sizeof (C_wchar_t));
    }
    *r_ret = c = wchar_tmp_buf;
/* FIXME: we don't deal with locale encoding */
    if (e == FONT_ENCODING_8BIT) {
        while (l) {
            *c = *s;
            c++;
            s++;
            l--;
        }
        *l_ret = (c - *r_ret);
        return;
    }
    while (l) {
        if (*s < 0xC0) {
            *c = *s;
            c++;
            s++;
            l--;
            continue;
        }
        if (*s < 0xE0) {
            if (l < 2)
                break;
	    *c = ((C_wchar_t) (s[0] & 0x1F) << 6) | (C_wchar_t) (s[1] & 0x3F);
            c++;
            s += 2;
            l -= 2;
            continue;
        }
        if (*s < 0xF0) {
            if (l < 3)
                break;
	    *c = ((C_wchar_t) (s[0] & 0x0F) << 12) | ((C_wchar_t) (s[1] & 0x3F) << 6) | (C_wchar_t) (s[2] & 0x3F);
            c++;
            s += 3;
            l -= 3;
            continue;
        }
        if (*s < 0xF8) {
            if (l < 4)
                break;
	    *c = ((C_wchar_t) (s[0] & 0x07) << 18) | ((C_wchar_t) (s[1] & 0x3F) << 12) | ((C_wchar_t) (s[2] & 0x3F) << 6) | (C_wchar_t) (s[3] & 0x3F);
            c++;
            s += 4;
            l -= 4;
            continue;
        }
        if (*s < 0xFC) {
            if (l < 5)
                break;
	    *c = ((C_wchar_t) (s[0] & 0x03) << 24) | ((C_wchar_t) (s[1] & 0x3F) << 18) | ((C_wchar_t) (s[2] & 0x3F) << 12) | ((C_wchar_t) (s[3] & 0x3F) << 6) | (C_wchar_t) (s[4] & 0x3F);
            c++;
            s += 5;
            l -= 5;
            continue;
        }
        if (*s < 0xFE) {
            if (l < 6)
                break;
	    *c = ((C_wchar_t) (s[0] & 0x01) << 30) | ((C_wchar_t) (s[1] & 0x3F) << 24) | ((C_wchar_t) (s[2] & 0x3F) << 18) | ((C_wchar_t) (s[3] & 0x3F) << 12) | ((C_wchar_t) (s[4] & 0x3F) << 6) | (C_wchar_t) (s[5] & 0x3F);
            c++;
            s += 6;
            l -= 6;
            continue;
        }
        *c = *s;
        c++;
        s++;
        l--;
    }
    *l_ret = (c - *r_ret);
}

int count_one_utf8_char (const char *s_)
{E_
    int n = 0, r = 0;
    const unsigned char *s = (const unsigned char *) s_;
    for (;;) {
        if (!*s)
            return 0;
        if (*current_font->encoding_interpretation == FONT_ENCODING_8BIT) {
	    n = 1;
	    break;
        }
        if ((*s & 0xC0) == 0x80)
            return -1;
	if (*s < 0xC0) {
	    n = 1;
	    break;
	}
	if (*s < 0xE0) {
	    n = 2;
	    break;
	}
	if (*s < 0xF0) {
	    n = 3;
	    break;
	}
	if (*s < 0xF8) {
	    n = 4;
	    break;
	}
	if (*s < 0xFC) {
	    n = 5;
	    break;
	}
	if (*s < 0xFE) {
	    n = 6;
	    break;
	}
	n = 1;
	break;
    }
    while (n--) {
	r++;
        s++;
	if (!*s)
	    break;
    }
    return r;
}

int count_one_utf8_char_sloppy (const char *s)
{E_
    int r;
    r = count_one_utf8_char (s);
    return r > 0 ? r : 1;
}

unsigned char *font_wchar_to_charenc (C_wchar_t c, int *l)
{E_
    static char wr[32];
    static unsigned char ch;
    wchar_t wc = 0;
    unsigned char *r;
    mbstate_t ps;
    if (!c) {
	ch = (unsigned char) c;
	*l = 1;
	return &ch;
    }
    switch (*current_font->encoding_interpretation) {
    case FONT_ENCODING_UTF8:
        r = wcrtomb_wchar_to_utf8 (c);
        *l = strlen ((const char *) r);
	return r;

    case FONT_ENCODING_8BIT:
	ch = (unsigned char) c;
	*l = 1;
	return &ch;

    case FONT_ENCODING_LOCALE:
        memset (&ps, '\0', sizeof (ps));
        wc = (wchar_t) c;
        *l = wcrtomb (wr, wc, &ps);
        if (*l == -1)
            *l = 0;
	return (unsigned char *) wr;
    }

    return 0;
}

int utf8_to_wchar_t_one_char_safe (C_wchar_t * c, const char *t, int n)
{E_
    int r;
    if (!*t) {
	*c = 0;
	return 0;
    }
    if (*current_font->encoding_interpretation == FONT_ENCODING_8BIT) {
        *c = *((const unsigned char *) t);
        return 1;
    }
    r = mbrtowc_utf8_to_wchar (c, t, n, 0);
    if (r < 0) {
        *c = '?';
        return 1;
    }
    return r;
}

/* only ImageStrings can bee anti-aliased */
int CImageTextWidth (const char *s, int l)
{E_
    C_wchar_t *t = 0;
    int n = 0;
    utf8_to_wchar_t((const unsigned char *) s, l, &t, &n, *current_font->encoding_interpretation);
    return CImageTextWidthWC (0, t, n);
}

static XChar2b *XChar2b_tmp_buf = 0;
static int XChar2b_tmp_buf_len = 0;

void XChar2b_tmp_buf_free(void)
{E_
    if (XChar2b_tmp_buf)
        free(XChar2b_tmp_buf);
}

static XChar2b *wchar_t_to_XChar2b (const C_wchar_t * s, int l)
{E_
    XChar2b *p, *ret;
    if (XChar2b_tmp_buf_len < l + 1) {
	XChar2b_tmp_buf_len = (l + 1) * 2;
	if (XChar2b_tmp_buf)
	    free (XChar2b_tmp_buf);
	XChar2b_tmp_buf = malloc (XChar2b_tmp_buf_len * sizeof (XChar2b));
    }
    ret = p = XChar2b_tmp_buf;
    while (l-- > 0) {
	p->byte1 = (unsigned char) (*s >> 8);
	p->byte2 = (unsigned char) (*s & 0xFF);
	p++;
	s++;
    }
    p->byte1 = 0;
    p->byte2 = 0;
    return ret;
}

static wchar_t *wchar_t_tmp_buf = 0;
static int wchar_t_tmp_buf_len = 0;

void wchar_t_tmp_buf_free(void)
{E_
    if (wchar_t_tmp_buf)
        free(wchar_t_tmp_buf);
}

static wchar_t *Cwchar_to_wchar (const C_wchar_t * s, int l)
{E_
    wchar_t *p, *ret;
    if (wchar_t_tmp_buf_len < l + 1) {
	wchar_t_tmp_buf_len = (l + 1) * 2;
	if (wchar_t_tmp_buf)
	    free (wchar_t_tmp_buf);
	wchar_t_tmp_buf = malloc (wchar_t_tmp_buf_len * sizeof (wchar_t));
    }
    ret = p = wchar_t_tmp_buf;
    while (l-- > 0) {
	*p = (wchar_t) *s;
	p++;
	s++;
    }
    *p = 0;
    return ret;
}

int CImageTextWidthWC (XChar2b * s, C_wchar_t * swc, int l)
{E_
    if (FONT_USE_FONT_SET)
	return XwcTextEscapement (current_font->f.font_set, Cwchar_to_wchar (swc, l), l);
#ifndef NO_TTF
    if (FONT_ANTIALIASING)
        return XAaTextWidth16 (&current_font->f, s, swc, l, 0, FONT_ANTIALIASING);
#endif
    if (!s)
	return XTextWidth16 (current_font->f.font_struct, wchar_t_to_XChar2b (swc, l), l);
    return XTextWidth16 (current_font->f.font_struct, s, l);
}

int CImageStringWidth (const char *s)
{E_
    return CImageTextWidth (s, strlen (s));
}

int CImageText (Window w, int x, int y, const char *s, int l)
{E_
    C_wchar_t *t = 0;
    int n = 0;
    utf8_to_wchar_t((const unsigned char *) s, l, &t, &n, *current_font->encoding_interpretation);
    return CImageTextWC (w, x, y, 0, t, n);
}

int CImageTextWC (Window w, int x, int y, XChar2b * s, C_wchar_t * swc, int l)
{E_
    if (FONT_USE_FONT_SET) {
        wchar_t *sw;
        sw = Cwchar_to_wchar (swc, l);
	XwcDrawImageString (CDisplay, w, current_font->f.font_set, CGC, x, y, sw, l);
	return XwcTextEscapement (current_font->f.font_set, sw, l);
    }
#ifndef NO_TTF
    if (FONT_ANTIALIASING)
	return XAaDrawImageString16 (CDisplay, w, CGC, &current_font->f, x, y, s, swc, l, FONT_ANTIALIASING);
#endif
    if (!s)
	return XDrawImageString16 (CDisplay, w, CGC, x, y, wchar_t_to_XChar2b (swc, l), l);
    return XDrawImageString16 (CDisplay, w, CGC, x, y, s, l);
}

int CImageString (Window w, int x, int y, const char *s)
{E_
    return CImageText (w, x, y, s, strlen (s));
}

static struct font_object *find_font (const char *name)
{E_
    struct font_object *n;
    if (current_font)
	if (!strcmp (current_font->name, name))
	    return current_font;
    for (n = all_fonts; n; n = n->next)
	if (!strcmp (n->name, name))
	    return n;
    return 0;
}

/*
 *     ●    If the min_byte1 and max_byte1 members are both zero,
 *           min_char_or_byte2 specifies the linear character
 *          index corresponding to the first element of the
 *           per_char array, and max_char_or_byte2 specifies the
 *          linear character index of the last element.
 *
 *           If either min_byte1 or max_byte1 are nonzero, both
 *          min_char_or_byte2 and max_char_or_byte2 are less than
 *           256, and the 2-byte character index values corre­
 *          sponding to the per_char array element N (counting
 *           from 0) are:
 *
 *               byte1 = N/D + min_byte1
 *               byte2 = N%D + min_char_or_byte2
 *
 *          where:
 *
 *                   D = max_char_or_byte2 - min_char_or_byte2 + 1
 *                   / = integer division
 *                   % = integer modulus
 */


/* returns width. the descent is only ever used for drawing an underbar.
   the ascent is only ever used in calculating FONT_PIX_PER_LINE */
static int get_wchar_dimension (C_wchar_t ch, int *height, int *ascent, int *ink_descent)
{E_
    int width, direction;
    XRectangle ink;
    XRectangle logical;

    if (FONT_USE_FONT_SET) {
        wchar_t cwc;
        cwc = (wchar_t) ch;
	width = XwcTextExtents (current_font->f.font_set, &cwc, 1, &ink, &logical);
	if (height)
	    *height = logical.height;
	if (ascent)
	    *ascent = (-logical.y);
	if (ink_descent)
	    *ink_descent = ink.height + ink.y;
#ifndef NO_TTF
    } else if (FONT_ANTIALIASING) {
        if (ascent) {
            if (current_font->f.font_struct)
                *ascent = current_font->f.font_struct->max_bounds.ascent / FONT_ANTIALIASING;
            else
                *ascent = current_font->f.font_freetype.measured_ascent / FONT_ANTIALIASING;
        }
        if (height) {
            if (current_font->f.font_struct)
                *height = (current_font->f.font_struct->max_bounds.ascent + current_font->f.font_struct->max_bounds.descent) / FONT_ANTIALIASING;
            else
                *height = current_font->f.font_freetype.measured_height / FONT_ANTIALIASING;
        }
        width = XAaTextWidth16 (&current_font->f, 0, &ch, 1, ink_descent, FONT_ANTIALIASING);
        return width;
#endif
    } else {
	XCharStruct c;
	XChar2b s;
	int ascent_, descent, w;
	s.byte2 = ch & 0xFF;
	s.byte1 = (ch >> 8) & 0xFF;
	XTextExtents16 (current_font->f.font_struct, &s, 1, &direction, &ascent_, &descent, &c);
	width = c.width;
        if (ascent)
            *ascent = ascent_;
        if (height)
            *height = ascent_ + descent;
	w =
	    current_font->f.font_struct->max_char_or_byte2 -
	    current_font->f.font_struct->min_char_or_byte2 + 1;
	if (w == 1)
	    w = 0;
	if (ink_descent) {
	    if (s.byte2 >= current_font->f.font_struct->min_char_or_byte2
		&& s.byte2 <= current_font->f.font_struct->max_char_or_byte2
		&& s.byte1 >= current_font->f.font_struct->min_byte1
		&& s.byte1 <= current_font->f.font_struct->max_byte1) {
		if (current_font->f.font_struct->per_char) {
		    *ink_descent =
			current_font->f.font_struct->per_char[
							    (s.byte2 -
							     current_font->f.font_struct->
							     min_char_or_byte2) + w * (s.byte1 -
										       current_font->
										       f.font_struct->
										       min_byte1)].
			descent;
		} else {
/* this happens when you try to scale a non-scalable font */
		    *ink_descent = current_font->f.font_struct->max_bounds.descent;
		}
	    } else {
		*ink_descent = 0;
	    }
	}
    }
    return width;
}

/* returns width. the descent is only ever used for drawing an underbar.
   the ascent is only ever used in calculating FONT_PIX_PER_LINE */
static int get_string_dimensions (const char *s, int n, int *height, int *ascent, int *ink_descent)
{E_
    int width = 0, direction = 0;
    XRectangle ink;
    XRectangle logical;
    if (FONT_USE_FONT_SET) {
	width = XmbTextExtents (current_font->f.font_set, s, n, &ink, &logical);
	if (height)
	    *height = logical.height;
	if (ascent)
	    *ascent = (-logical.y);
	if (ink_descent)
	    *ink_descent = ink.height + ink.y;
#ifndef NO_TTF
    } else if (FONT_ANTIALIASING) {
        if (ascent) {
            if (current_font->f.font_struct)
                *ascent = current_font->f.font_struct->max_bounds.ascent / FONT_ANTIALIASING;
            else
                *ascent = current_font->f.font_freetype.measured_ascent / FONT_ANTIALIASING;
        }
        if (height) {
            if (current_font->f.font_struct)
                *height = (current_font->f.font_struct->max_bounds.ascent + current_font->f.font_struct->max_bounds.descent) / FONT_ANTIALIASING;
            else
                *height = current_font->f.font_freetype.measured_height / FONT_ANTIALIASING;
        }
        width = XAaTextWidth (&current_font->f, s, n, ink_descent, FONT_ANTIALIASING);
        return width;
#endif
    } else {
	XCharStruct c;
	int ascent_, descent;
	XTextExtents (current_font->f.font_struct, s, n, &direction, &ascent_, &descent, &c);
        width = c.width;
        if (ascent)
            *ascent = ascent_;
        if (height)
            *height = ascent_ + descent;
	if (ink_descent) {
	    if (n == 1) {
		int i;
		i = (unsigned char) *s;
		if (i >= current_font->f.font_struct->min_char_or_byte2
		    && i <= current_font->f.font_struct->max_char_or_byte2)
			*ink_descent =
			current_font->f.font_struct->per_char[i -
							    current_font->
							    f.font_struct->min_char_or_byte2].descent;
		else
		    *ink_descent = 0;
	    } else
		*ink_descent = descent;
	}
    }
    return width;
}

static int check_font_fixed (void)
{E_
    int m;
    char *p;
    m = get_string_dimensions ("M", 1, 0, 0, 0);
    for (p = "!MI i.1@~W"; *p; p++)
	if (m != get_string_dimensions (p, 1, 0, 0, 0))
	    return 0;
    return m;
}

static void get_font_dimensions (void)
{E_
    const char *p = TEST_FONT_STRING;
    FONT_MEAN_WIDTH = get_string_dimensions (p, strlen(p), &FONT_HEIGHT, &FONT_ASCENT, &FONT_DESCENT) / strlen(p);
}

/* this tries to keep the array of cached of font widths as small ever
   needed - i.e. only enlarges on lookups */
static void _font_per_char (C_wchar_t c)
{E_
    if (!current_font->per_char) {
	current_font->num_per_char = c + 1;
	current_font->per_char = CMalloc (current_font->num_per_char * sizeof (fontdim_t));
	memset (current_font->per_char, 0xFF, current_font->num_per_char * sizeof (fontdim_t));
    } else if (c >= current_font->num_per_char) {
	long l;
	fontdim_t *t;
	l = c + 1024;
	t = CMalloc (l * sizeof (fontdim_t));
	memcpy (t, current_font->per_char, current_font->num_per_char * sizeof (fontdim_t));
	memset (t + current_font->num_per_char, 0xFF,
		(l - current_font->num_per_char) * sizeof (fontdim_t));
	free (current_font->per_char);
	current_font->per_char = t;
	current_font->num_per_char = l;
    }
    if (current_font->per_char[c].width == 0xFF) {
	int d;
        if (c == '\n' || c == '\t') {
            current_font->per_char[c].width = 0;
	    current_font->per_char[c].descent = 0;
        } else {
	    current_font->per_char[c].width = get_wchar_dimension (c, 0, 0, &d);
	    current_font->per_char[c].descent = (unsigned char) d;
	    if (FIXED_FONT)
                if (current_font->per_char[c].width)
	            if ((current_font->per_char[c].width != FIXED_FONT))
		        FIXED_FONT = 0;
        }
    }
}

int font_per_char (C_wchar_t c)
{E_
    if ((unsigned long) c > FONT_LAST_UNICHAR)  /* catches c < 0 */
	return 0;
    _font_per_char (c);
    return current_font->per_char[c].width;
}

int font_per_char_descent (C_wchar_t c)
{E_
    if ((unsigned long) c > FONT_LAST_UNICHAR)
	return 0;
    _font_per_char (c);
    return current_font->per_char[c].descent;
}

/* seems you cannot draw different fonts in the same GC. this is
   strange???, so we create a dummy GC for each font */
static Window get_dummy_gc (void)
{E_
    static Window dummy_window = 0;
    XGCValues gcv;
    memset (&gcv, '\0', sizeof (gcv));
    if (!dummy_window) {
	XSetWindowAttributes xswa;
        memset (&xswa, '\0', sizeof (xswa));
	xswa.override_redirect = 1;
	dummy_window =
	    XCreateWindow (CDisplay, CRoot, 0, 0, 1, 1, 0, CDepth, InputOutput, CVisual,
			   CWOverrideRedirect, &xswa);
    }
    gcv.foreground = COLOR_BLACK;
    if (current_font->f.font_struct) {
	gcv.font = current_font->f.font_struct->fid;
	CGC = XCreateGC (CDisplay, dummy_window, GCForeground | GCFont, &gcv);
    } else {
	CGC = XCreateGC (CDisplay, dummy_window, GCForeground, &gcv);
    }
    return dummy_window;
}

static XFontSet get_font_set (char *name)
{E_
    XFontSet fontset;
    char **a = 0;
    int b;
    if (!XSupportsLocale ())
#ifdef LC_CTYPE
	fprintf (stderr, "X does not support the locale: %s\n", setlocale (LC_CTYPE, 0));
#else
	fprintf (stderr, "X does not support locale's\n");
#endif
    fontset = XCreateFontSet (CDisplay, name, &a, &b, 0);
    if (!fontset)
	return 0;
    XFreeStringList (a);
    return fontset;
}

/* loads a font and sets the current font to it */
static struct font_object *load_font (const char *name, const char *xname_, enum font_encoding *e, enum force_fixed_width_enum force_fixed_width)
{E_
    char compactname[80];
    char *xname;
    int aa = 0;
    struct font_object *n;
    Window w;
    int l;
    int desired_height = 0;
    char *colon;

    xname = (char *) strdup (xname_);

    l = strlen(xname);
    if (l > 3 && xname[l - 1] >= '0' && xname[l - 1] <= '9' && xname[l - 2] == '/') {
        int t;
        t = xname[l - 1] - '0';
	if (!(t == 1 || t == 3)) {
            fprintf (stderr, "%s: cannot load font\n\t%s\n%s", CAppName, xname, font_error_string);
	    free (xname);
	    return 0;
	}
        aa = t;
        xname[l - 2] = '\0';
    }

    colon = strrchr(xname, ':');
    if (colon && strlen(colon) <= 4) {
        desired_height = atoi(colon + 1);
        if (desired_height < 2 || desired_height > 255) {
            fprintf (stderr, "%s: cannot load font\n\t%s\n%s", CAppName, xname, font_error_string);
	    free (xname);
	    return 0;
        }
        aa = 1;
        *colon = '\0';
    }

    if (aa && CVisual->class != TrueColor) {
	fprintf (stderr, "%s: %s: can't do anti-aliasing without TrueColor visual.\nTry setting your X server to non-8-bits-per-pixel display.\n", CAppName, xname);
        free (xname);
        return 0;
    }

    n = CMalloc (sizeof (struct font_object));
    memset (n, 0, sizeof (struct font_object));

    n->encoding_interpretation = e;

#ifndef NO_TTF
    if (!load_font_from_file(xname, &n->f, desired_height)) {
        if (aa > 1) {
            fprintf(stderr, "Loaded fonts cannot be scaled with /3\n");
	    free (xname);
	    return 0;
        }
        aa = 1;
    } else
#endif
    if (option_font_set && !aa) {
	n->f.font_set = get_font_set (xname);
	if (!n->f.font_set)
	    fprintf (stderr,
		     _("%s: display %s cannot load font\n\t%s\nas a font set - trying raw load.\n"),
		     CAppName, DisplayString (CDisplay), xname);
/* font set may have failed only because of an invalid locale, but
   we try load the font anyway */
    }

    if (!n->f.font_freetype.n_fonts && !n->f.font_set && !strchr (xname, ','))
	n->f.font_struct = XLoadQueryFont (CDisplay, xname);

    if (!n->f.font_freetype.n_fonts && !n->f.font_struct && !n->f.font_set) {
	fprintf (stderr, _("%s: display %s cannot load font\n\t%s\n"), CAppName,
		 DisplayString (CDisplay), xname);
	free (n);
	free (xname);
	return 0;
    }

#ifndef NO_TTF
    n->f.load_id = last_font_load_id++;
#endif

    n->next = all_fonts;
    current_font = all_fonts = n;
    n->name = (char *) strdup (name);

/* font set may have worked, but if there is only one font, we
   might as well try use a font struct (which draws faster)
   unless the user ask asked otherwise with option_font_set = 1 */
    if (!option_font_set && current_font->f.font_set && !current_font->f.font_struct) {
	int i, num_fonts, single_font = 1;
	XFontStruct **font_struct_list_return = 0;
	char **font_name_list_return = 0;
	num_fonts =
	    XFontsOfFontSet (current_font->f.font_set, &font_struct_list_return,
			     &font_name_list_return);
/* check if there is really only one font */
	for (i = 1; i < num_fonts; i++) {
	    if (strcmp (font_name_list_return[0], font_name_list_return[i])) {
		single_font = 0;
		break;
	    }
	}
	if (single_font)
	    current_font->f.font_struct = XLoadQueryFont(CDisplay, xname);
    }
#ifndef NO_TTF
    if (current_font->f.font_struct || current_font->f.font_freetype.n_fonts)
	FONT_ANTIALIASING = aa;
#endif
    w = get_dummy_gc ();
    if (current_font->f.font_freetype.n_fonts)
        ;
    else if (FONT_USE_FONT_SET)
	XmbDrawImageString (CDisplay, w, current_font->f.font_set, CGC, 0, 0, " AZ~", 4);
    else
	XDrawImageString (CDisplay, w, CGC, 0, 0, " AZ~", 4);
    FIXED_FONT = check_font_fixed ();
    get_font_dimensions ();
    n->f.mean_font_width = FONT_MEAN_WIDTH;
    n->f.force_fixed_width = force_fixed_width;
    if (FONT_MEAN_WIDTH <= 2) {
	fprintf (stderr, _("%s: display %s cannot load font\n\t%s\n"), CAppName, DisplayString (CDisplay), xname);
        free (xname);
        return 0;
    }
/* -----> FIXME: yes, you actually HAVE to draw with the GC for
   the font to stick. can someone tell me what I am doing wrong? */
    if (!current_font->f.font_set) {
/* font list like rxvt. FIXME: make rxvt and this see same font list */
	current_font->f.font_set = get_font_set ("7x14,6x10,6x13,8x13,9x15");   /* needed for XIM */
    }
    if (strlen (xname) > 60)
        sprintf (compactname, "%.40s...%s", xname, xname + strlen (xname) - 7);
    else
        strcpy (compactname, xname);
    printf("loaded %s as %s\n", compactname, current_font->f.font_struct ? "font struct" : (current_font->f.font_freetype.n_fonts ? "freetype font" : ((current_font->f.font_set ? "font set" : "(error)"))));
    free (xname);
    return current_font;
}

int font_depth = 0;

static int CPushFont_ (enum force_fixed_width_enum force_fixed_width, const char *name, const char *xname, enum font_encoding *e);

int CPushFont (const char *name, ...)
{E_
    va_list ap;
    enum font_encoding *e;
    const char *xname;

    va_start (ap, name);
    xname = va_arg (ap, const char *);
    e = va_arg (ap, enum font_encoding *);
    va_end (ap);
    return CPushFont_ (FORCE_FIXED_WIDTH__DISABLE, name, xname, e);
}

int CPushFontForceFixed (const char *name, ...)
{E_
    va_list ap;
    enum font_encoding *e;
    const char *xname;

    va_start (ap, name);
    xname = va_arg (ap, const char *);
    e = va_arg (ap, enum font_encoding *);
    va_end (ap);
    return CPushFont_ (FORCE_FIXED_WIDTH__SINGLEWIDTH, name, xname, e);
}

int CPushFontHonorFixedDoubleWidth (const char *name, ...)
{E_
    va_list ap;
    enum font_encoding *e;
    const char *xname;

    va_start (ap, name);
    xname = va_arg (ap, const char *);
    e = va_arg (ap, enum font_encoding *);
    va_end (ap);
    return CPushFont_ (FORCE_FIXED_WIDTH__UNICODETERMINALMODE, name, xname, e);
}

static int CPushFont_ (enum force_fixed_width_enum force_fixed_width, const char *name, const char *xname, enum font_encoding *e)
{E_
    struct font_stack *p;
    struct font_object *f;
    f = find_font (name);
    if (f) {
	f->ref++;
    } else {
        if (!e || !*e) {
	    fprintf (stderr, "CPushFont passed NULL encoding\n");
	    fflush (stderr);
	    abort ();
        }
        if (!xname) {
	    fprintf (stderr, "CPushFont passed NULL font name\n");
	    fflush (stderr);
	    abort ();
        }
	switch (*e) {
	case FONT_ENCODING_UTF8:
	case FONT_ENCODING_8BIT:
	case FONT_ENCODING_LOCALE:
	    break;
	default:
	    fprintf (stderr, "invalid encoding\n");
	    fflush (stderr);
	    abort ();
	}
	f = load_font (name, xname, e, force_fixed_width);
	if (!f)
	    return 1;
	f->ref = 1;
    }
    p = CMalloc (sizeof (struct font_stack));
    p->f = f;
    p->next = font_stack;
    font_stack = p;
    current_font = font_stack->f;
    font_depth++;
    return 0;
}

void CPopFont (void)
{E_
    struct font_stack *p;
    if (!font_stack) {
	fprintf (stderr, "Huh\n?");
	abort ();
    }
    if (!--font_stack->f->ref) {
#ifndef NO_TTF
	int i;
#endif
	if (font_stack->f->gc)
	    XFreeGC (CDisplay, font_stack->f->gc);
	if (font_stack->f->f.font_set)
	    XFreeFontSet (CDisplay, font_stack->f->f.font_set);
#ifndef NO_TTF
	XAaFree (font_stack->f->f.load_id);
#endif
	if (font_stack->f->f.font_struct)
	    XFreeFont (CDisplay, font_stack->f->f.font_struct);
#ifndef NO_TTF
	for (i = 0; i < font_stack->f->f.font_freetype.n_fonts; i++) {
	    struct freetype_cache *cache;
	    cache = &font_stack->f->f.font_freetype.faces[i];
	    if (cache->freetype_fname)
		free (cache->freetype_fname);
	    if (cache->face)
		FT_Done_Face(cache->face);
	}
#endif
	if (font_stack->f->per_char)
	    free (font_stack->f->per_char);
	free (font_stack->f->name);
	free (font_stack->f);
    }
    p = font_stack->next;
    if (p) {
	current_font = p->f;
    } else {
	current_font = 0;
    }
    font_depth--;
    free (font_stack);
    font_stack = p;
}

void CFreeAllFonts (void)
{E_
    int i = 0;
    while (font_stack) {
	CPopFont ();
	i++;
    }
#ifndef NO_TTF
    load_one_freetype_font(0, 0, 0, 0);
#endif
}

int CIsFixedFont (void)
{E_
    return FIXED_FONT;
}

