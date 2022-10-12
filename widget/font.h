/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* font.h - multiple font handling
   Copyright (C) 1996-2022 Paul Sheer
 */


#ifndef _FONT_H
#define _FONT_H

typedef struct {
    unsigned char width;
    unsigned char descent;
} fontdim_t;

struct freetype_cache {
    char *freetype_fname;
    int load_logged;
    int load_failed;
    int loaded_height;
    void *face;
};

struct freetype_face {
    struct freetype_cache faces[1024];
    int n_fonts;
    int desired_height;
    int measured_height;
    int measured_ascent;
};

struct aa_font {
    int load_id;
    int mean_font_width;
    int force_fixed_width;
    XFontSet font_set;
    XFontStruct *font_struct;
    struct freetype_face font_freetype;
};

enum font_encoding {
    FONT_ENCODING_UTF8 = 101,
    FONT_ENCODING_8BIT = 102,
    FONT_ENCODING_LOCALE = 103
};

struct font_object {
    char *name;
    int ref;
    enum font_encoding *encoding_interpretation;
    struct aa_font f;
    void *pad1;
    int pad2;
    GC gc;
    int mean_font_width;
    int fixed_font;
    int anti_aliasing;
    int font_height;
    int font_ascent;
    int font_descent;
    struct font_object *next;
    fontdim_t *per_char;
    int num_per_char;
};

#define CGC				current_font->gc
#define FONT_MEAN_WIDTH			current_font->mean_font_width
#define FIXED_FONT			current_font->fixed_font
#define FONT_ASCENT			current_font->font_ascent
#define FONT_HEIGHT			current_font->font_height
#define FONT_DESCENT			current_font->font_descent
#define FONT_PER_CHAR(x)		font_per_char(x)
#define FONT_USE_FONT_SET		(!current_font->f.font_struct && !current_font->f.font_freetype.n_fonts && current_font->f.font_set)
#define FONT_LAST_UNICHAR		0x10FFFFUL
#define FONT_ANTIALIASING		current_font->anti_aliasing
#define FONT_PER_CHAR_DESCENT(x)	font_per_char_descent(x)
#define FONT_OVERHEAD			option_text_line_spacing
#define FONT_PIX_PER_LINE		(FONT_OVERHEAD + FONT_HEIGHT)
#define FONT_BASE_LINE			(FONT_OVERHEAD + FONT_ASCENT)

int font_per_char (C_wchar_t c);
int font_per_char_descent (C_wchar_t c);
int mbrtowc_utf8_to_wchar (C_wchar_t * c, const char *t, int n, void *x /* no shifting with utf8 */ );
unsigned char *wcrtomb_wchar_to_utf8 (C_wchar_t c);

void CFreeAllFonts (void);
int CPushFont (const char *name, ...);
int CPushFontForceFixed (const char *name, ...);
void CPopFont (void);
int CIsFixedFont (void);


#endif


