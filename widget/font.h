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
#ifndef NO_TTF
    struct freetype_cache faces[1024];
#endif
    int n_fonts;
#ifndef NO_TTF
    int desired_height;
    int measured_height;
    int measured_ascent;
#endif
};

enum force_fixed_width_enum {
    FORCE_FIXED_WIDTH__DISABLE = 0,
    FORCE_FIXED_WIDTH__SINGLEWIDTH = 1,
    FORCE_FIXED_WIDTH__UNICODETERMINALMODE = 2         /* double-wide chars are 2X the width */
};

struct aa_font {
#ifndef NO_TTF
    int load_id;
#endif
    int mean_font_width;
    int monochrome;
    enum force_fixed_width_enum force_fixed_width;
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
#ifndef NO_TTF
    int anti_aliasing;
#endif
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
#define ZERO_WIDTH_EMPTY_CHAR           (FONT_LAST_UNICHAR + 1)
#ifndef NO_TTF
#define FONT_ANTIALIASING		current_font->anti_aliasing
#endif
#define FONT_PER_CHAR_DESCENT(x)	font_per_char_descent(x)
#define FONT_OVERHEAD			option_text_line_spacing
#define FONT_PIX_PER_LINE		(FONT_OVERHEAD + FONT_HEIGHT)
#define FONT_BASE_LINE			(FONT_OVERHEAD + FONT_ASCENT)

int font_per_char (C_wchar_t c);
int font_per_char_descent (C_wchar_t c);
int mbrtowc_utf8_to_wchar (C_wchar_t * c, const char *t, int n, void *x /* no shifting with utf8 */ );
unsigned char *wcrtomb_wchar_to_utf8 (C_wchar_t c);
int is_unicode_doublewidth_char (C_wchar_t c);
void font_lazy_cleanup (void);

void CFreeAllFonts (void);
int CPushFont (const char *name, const char *xname, ...);
int CPushFontForceFixed (const char *name, const char *xname, ...);
int CPushFontHonorFixedDoubleWidth (const char *name, const char *xname, ...);
void CPopFont (void);
int CIsFixedFont (void);
void CFontLazy (const char *name, const char *pref1, const char *pref2, enum font_encoding *enc);
void CFontLazyForceFixed (const char *name, const char *pref1, const char *pref2, enum font_encoding *enc);
void CFontLazyHonorFixedDoubleWidth (const char *name, const char *pref1, const char *pref2, enum font_encoding *enc);



#endif


