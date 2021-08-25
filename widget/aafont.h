/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* aafont.h
   Copyright (C) 1996-2022 Paul Sheer
 */


struct aa_font;

void XAaFree (int load_id);

void XAaInit (Display * display, Visual * visual, int depth, Window root);
void XAaCacheClean (void);

int XAaTextWidth (const struct aa_font *f, const char *s, int length, int *descent, int scale);

int XAaTextWidth16 (const struct aa_font *f, XChar2b * s, C_wchar_t * swc, int length, int *descent, int scale);

int XAaDrawImageString (Display * display, Drawable d, GC gc, const struct aa_font *, int x, int y, char *s, int length, int scale);

int XAaDrawImageString16 (Display * display, Drawable d, GC gc, const struct aa_font *, int x, int y, XChar2b * wc, C_wchar_t * swc, int length, int scale);

#define RedFirst 0
#define BlueFirst 1

extern int option_interchar_spacing;
extern int option_rgb_order;


#define X_ENLARGEMENT	3
#define Y_ENLARGEMENT	5

#define SHRINK_WIDTH(x,s) ((s) <= 1 ? (x) : (((x) + X_ENLARGEMENT) / (s) + option_interchar_spacing))
#define SHRINK_HEIGHT(x,s) ((s) <= 1 ? (x) : (((x) + Y_ENLARGEMENT) / (s)))
