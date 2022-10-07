/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* international.c - routines for composing international characters
   Copyright (C) 1996-2022 Paul Sheer
 */



#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>

#include <pwd.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>

#include "coolwidget.h"
#include "edit.h"


enum font_encoding get_editor_encoding (void);


struct compose_international {
    int unicode;
    unsigned char ascii1, ascii2;
    const char *descr;
};

/* Unicode and Latin1 are identical in the code point range up to 0xFF */
static struct compose_international compose_nonascii_unicode[] = { \
    {0x00a0, ' ', ' ',          "NO-BREAK SPACE"},
    {0x00a1, '^', '!',          "INVERTED EXCLAMATION MARK"},
    {0x00a2, 'c', '/',          "CENT SIGN"},
    {0x00a3, 'l', '-',          "POUND SIGN"},
    {0x00a4, 'x', 'o',          "CURRENCY SIGN"},
    {0x00a5, 'y', '=',          "YEN SIGN"},
    {0x00a6, '|', '|',          "BROKEN BAR"},
    {0x00a7, '8', 'o',          "SECTION SIGN"},
    {0x00a7, 'o', 'S',          "SECTION SIGN"},
    {0x00a7, 'O', 'S',          "SECTION SIGN"},
    {0x00a8, ' ', '"',          "DIAERESIS"},
    {0x00a8, '\'', '\'',        "DIAERESIS"},
    {0x00a9, 'o', 'c',          "COPYRIGHT SIGN"},
    {0x00a9, 'O', 'c',          "COPYRIGHT SIGN"},
    {0x00aa, 'a', '_',          "FEMININE ORDINAL INDICATOR"},
    {0x00ab, '<', '<',          "LEFT-POINTING DOUBLE ANGLE QUOTATION MARK"},
    {0x00ac, '!', '!',          "NOT SIGN"},
    {0x00ad, ' ', '-',          "SOFT HYPHEN"},
    {0x00ad, '-', '-',          "SOFT HYPHEN"},
    {0x00ae, 'o', 'r',          "REGISTERED SIGN"},
    {0x00ae, 'O', 'r',          "REGISTERED SIGN"},
    {0x00af, '^', '-',          "MACRON"},
    {0x00b0, '^', '0',          "DEGREE SIGN"},
    {0x00b0, ' ', 'o',          "DEGREE SIGN"},
    {0x00b1, '+', '-',          "PLUS-MINUS SIGN"},
    {0x00b2, '^', '2',          "SUPERSCRIPT TWO"},
    {0x00b3, '^', '3',          "SUPERSCRIPT THREE"},
    {0x00b4, ' ', '\'',         "ACUTE ACCENT"},
    {0x00b4, '^', '\'',         "ACUTE ACCENT"},
    {0x00b5, 'u', '|',          "MICRO SIGN"},
    {0x00b6, 'q', '|',          "PILCROW SIGN"},
    {0x00b6, 'Q', '|',          "PILCROW SIGN"},
    {0x00b7, '^', '.',          "MIDDLE DOT"},
    {0x00b8, ' ', ',',          "CEDILLA"},
    {0x00b8, '_', ',',          "CEDILLA"},
    {0x00b9, '^', '1',          "SUPERSCRIPT ONE"},
    {0x00ba, 'o', '_',          "MASCULINE ORDINAL INDICATOR"},
    {0x00bb, '>', '>',          "RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK"},
    {0x00bc, '1', '4',          "VULGAR FRACTION ONE QUARTER"},
    {0x00bd, '1', '2',          "VULGAR FRACTION ONE HALF"},
    {0x00be, '3', '4',          "VULGAR FRACTION THREE QUARTERS"},
    {0x00bf, '^', '?',          "INVERTED QUESTION MARK"},
    {0x00c0, 'A', '`',          "LATIN CAPITAL LETTER A WITH GRAVE"},
    {0x00c1, 'A', '\'',         "LATIN CAPITAL LETTER A WITH ACUTE"},
    {0x00c2, 'A', '^',          "LATIN CAPITAL LETTER A WITH CIRCUMFLEX"},
    {0x00c3, 'A', '~',          "LATIN CAPITAL LETTER A WITH TILDE"},
    {0x00c4, 'A', '"',          "LATIN CAPITAL LETTER A WITH DIAERESIS"},
    {0x00c5, 'A', 'o',          "LATIN CAPITAL LETTER A WITH RING ABOVE"},
    {0x00c6, 'A', 'E',          "LATIN CAPITAL LETTER AE"},
    {0x00c7, 'C', ',',          "LATIN CAPITAL LETTER C WITH CEDILLA"},
    {0x00c8, 'E', '`',          "LATIN CAPITAL LETTER E WITH GRAVE"},
    {0x00c9, 'E', '\'',         "LATIN CAPITAL LETTER E WITH ACUTE"},
    {0x00ca, 'E', '^',          "LATIN CAPITAL LETTER E WITH CIRCUMFLEX"},
    {0x00cb, 'E', '"',          "LATIN CAPITAL LETTER E WITH DIAERESIS"},
    {0x00cc, 'I', '`',          "LATIN CAPITAL LETTER I WITH GRAVE"},
    {0x00cd, 'I', '\'',         "LATIN CAPITAL LETTER I WITH ACUTE"},
    {0x00ce, 'I', '^',          "LATIN CAPITAL LETTER I WITH CIRCUMFLEX"},
    {0x00cf, 'I', '"',          "LATIN CAPITAL LETTER I WITH DIAERESIS"},
    {0x00d0, 'D', '-',          "LATIN CAPITAL LETTER ETH"},
    {0x00d1, 'N', '~',          "LATIN CAPITAL LETTER N WITH TILDE"},
    {0x00d2, 'O', '`',          "LATIN CAPITAL LETTER O WITH GRAVE"},
    {0x00d3, 'O', '\'',         "LATIN CAPITAL LETTER O WITH ACUTE"},
    {0x00d4, 'O', '^',          "LATIN CAPITAL LETTER O WITH CIRCUMFLEX"},
    {0x00d5, 'O', '~',          "LATIN CAPITAL LETTER O WITH TILDE"},
    {0x00d6, 'O', '"',          "LATIN CAPITAL LETTER O WITH DIAERESIS"},
    {0x00d7, 'X', 'X',          "MULTIPLICATION SIGN"},
    {0x00d7, 'X', 'o',          "MULTIPLICATION SIGN"},
    {0x00d8, 'O', '/',          "LATIN CAPITAL LETTER O WITH STROKE"},
    {0x00d9, 'U', '`',          "LATIN CAPITAL LETTER U WITH GRAVE"},
    {0x00da, 'U', '\'',         "LATIN CAPITAL LETTER U WITH ACUTE"},
    {0x00db, 'U', '^',          "LATIN CAPITAL LETTER U WITH CIRCUMFLEX"},
    {0x00dc, 'U', '"',          "LATIN CAPITAL LETTER U WITH DIAERESIS"},
    {0x00dc, 'U', '^',          "LATIN CAPITAL LETTER U WITH DIAERESIS"},
    {0x00dd, 'Y', '\'',         "LATIN CAPITAL LETTER Y WITH ACUTE"},
    {0x00de, 'P', '|',          "LATIN CAPITAL LETTER THORN"},
    {0x00df, 's', 's',          "LATIN SMALL LETTER SHARP S"},
    {0x00e0, 'a', '`',          "LATIN SMALL LETTER A WITH GRAVE"},
    {0x00e1, 'a', '\'',         "LATIN SMALL LETTER A WITH ACUTE"},
    {0x00e2, 'a', '^',          "LATIN SMALL LETTER A WITH CIRCUMFLEX"},
    {0x00e3, 'a', '~',          "LATIN SMALL LETTER A WITH TILDE"},
    {0x00e4, 'a', '"',          "LATIN SMALL LETTER A WITH DIAERESIS"},
    {0x00e5, 'a', 'o',          "LATIN SMALL LETTER A WITH RING ABOVE"},
    {0x00e6, 'a', 'e',          "LATIN SMALL LETTER AE"},
    {0x00e7, 'c', ',',          "LATIN SMALL LETTER C WITH CEDILLA"},
    {0x00e8, 'e', '`',          "LATIN SMALL LETTER E WITH GRAVE"},
    {0x00e9, 'e', '\'',         "LATIN SMALL LETTER E WITH ACUTE"},
    {0x00ea, 'e', '^',          "LATIN SMALL LETTER E WITH CIRCUMFLEX"},
    {0x00eb, 'e', '"',          "LATIN SMALL LETTER E WITH DIAERESIS"},
    {0x00ec, 'i', '`',          "LATIN SMALL LETTER I WITH GRAVE"},
    {0x00ed, 'i', '\'',         "LATIN SMALL LETTER I WITH ACUTE"},
    {0x00ee, 'i', '^',          "LATIN SMALL LETTER I WITH CIRCUMFLEX"},
    {0x00ef, 'i', '"',          "LATIN SMALL LETTER I WITH DIAERESIS"},
    {0x00f0, 'd', '-',          "LATIN SMALL LETTER ETH"},
    {0x00f1, 'n', '~',          "LATIN SMALL LETTER N WITH TILDE"},
    {0x00f2, 'o', '`',          "LATIN SMALL LETTER O WITH GRAVE"},
    {0x00f3, 'o', '\'',         "LATIN SMALL LETTER O WITH ACUTE"},
    {0x00f4, 'o', '^',          "LATIN SMALL LETTER O WITH CIRCUMFLEX"},
    {0x00f5, 'o', '~',          "LATIN SMALL LETTER O WITH TILDE"},
    {0x00f6, 'o', '"',          "LATIN SMALL LETTER O WITH DIAERESIS"},
    {0x00f7, ':', '-',          "DIVISION SIGN"},
    {0x00f8, 'o', '/',          "LATIN SMALL LETTER O WITH STROKE"},
    {0x00f9, 'u', '`',          "LATIN SMALL LETTER U WITH GRAVE"},
    {0x00fa, 'u', '\'',         "LATIN SMALL LETTER U WITH ACUTE"},
    {0x00fb, 'u', '^',          "LATIN SMALL LETTER U WITH CIRCUMFLEX"},
    {0x00fc, 'u', '"',          "LATIN SMALL LETTER U WITH DIAERESIS"},
    {0x00fd, 'y', '\'',         "LATIN SMALL LETTER Y WITH ACUTE"},
    {0x00fe, 'p', '|',          "LATIN SMALL LETTER THORN"},
    {0x00ff, 'y', '"',          "LATIN SMALL LETTER Y WITH DIAERESIS"},
    {0x0100, 'A', '-',          "LATIN CAPITAL LETTER A WITH MACRON"},
    {0x0101, 'a', '-',          "LATIN SMALL LETTER A WITH MACRON"},
    {0x0102, 'A', '?',          "LATIN CAPITAL LETTER A WITH BREVE"},
    {0x0103, 'a', '?',          "LATIN SMALL LETTER A WITH BREVE"},
    {0x0104, 'A', ',',          "LATIN CAPITAL LETTER A WITH OGONEK"},
    {0x0105, 'a', ',',          "LATIN SMALL LETTER A WITH OGONEK"},
    {0x0106, 'C', '\'',         "LATIN CAPITAL LETTER C WITH ACUTE"},
    {0x0107, 'c', '\'',         "LATIN SMALL LETTER C WITH ACUTE"},
    {0x0108, 'C', '^',          "LATIN CAPITAL LETTER C WITH CIRCUMFLEX"},
    {0x0109, 'c', '^',          "LATIN SMALL LETTER C WITH CIRCUMFLEX"},
    {0x010a, 'C', '.',          "LATIN CAPITAL LETTER C WITH DOT ABOVE"},
    {0x010b, 'c', '.',          "LATIN SMALL LETTER C WITH DOT ABOVE"},
    {0x010c, 'C', '?',          "LATIN CAPITAL LETTER C WITH CARON"},
    {0x010d, 'c', '?',          "LATIN SMALL LETTER C WITH CARON"},
    {0x010e, 'D', '?',          "LATIN CAPITAL LETTER D WITH CARON"},
    {0x010f, 'd', '?',          "LATIN SMALL LETTER D WITH CARON"},
    {0x0110, 'D', '-',          "LATIN CAPITAL LETTER D WITH STROKE"},
    {0x0111, 'd', '-',          "LATIN SMALL LETTER D WITH STROKE"},
    {0x0112, 'E', '-',          "LATIN CAPITAL LETTER E WITH MACRON"},
    {0x0113, 'e', '-',          "LATIN SMALL LETTER E WITH MACRON"},
    {0x0114, 'E', 'u',          "LATIN CAPITAL LETTER E WITH BREVE"},
    {0x0115, 'e', 'u',          "LATIN SMALL LETTER E WITH BREVE"},
    {0x0116, 'E', '.',          "LATIN CAPITAL LETTER E WITH DOT ABOVE"},
    {0x0117, 'e', '.',          "LATIN SMALL LETTER E WITH DOT ABOVE"},
    {0x0118, 'E', ',',          "LATIN CAPITAL LETTER E WITH OGONEK"},
    {0x0119, 'e', ',',          "LATIN SMALL LETTER E WITH OGONEK"},
    {0x011a, 'E', '?',          "LATIN CAPITAL LETTER E WITH CARON"},
    {0x011b, 'e', '?',          "LATIN SMALL LETTER E WITH CARON"},
    {0x011c, 'G', '^',          "LATIN CAPITAL LETTER G WITH CIRCUMFLEX"},
    {0x011d, 'g', '^',          "LATIN SMALL LETTER G WITH CIRCUMFLEX"},
    {0x011e, 'G', 'u',          "LATIN CAPITAL LETTER G WITH BREVE"},
    {0x011f, 'g', 'u',          "LATIN SMALL LETTER G WITH BREVE"},
    {0x0120, 'G', '.',          "LATIN CAPITAL LETTER G WITH DOT ABOVE"},
    {0x0121, 'g', '.',          "LATIN SMALL LETTER G WITH DOT ABOVE"},
    {0x0122, 'G', ',',          "LATIN CAPITAL LETTER G WITH CEDILLA"},
    {0x0123, 'g', ',',          "LATIN SMALL LETTER G WITH CEDILLA"},
    {0x0124, 'H', '^',          "LATIN CAPITAL LETTER H WITH CIRCUMFLEX"},
    {0x0125, 'h', '^',          "LATIN SMALL LETTER H WITH CIRCUMFLEX"},
    {0x0126, 'H', '-',          "LATIN CAPITAL LETTER H WITH STROKE"},
    {0x0127, 'h', '-',          "LATIN SMALL LETTER H WITH STROKE"},
    {0x0128, 'I', '~',          "LATIN CAPITAL LETTER I WITH TILDE"},
    {0x0129, 'i', '~',          "LATIN SMALL LETTER I WITH TILDE"},
    {0x012A, 'I', '-',          "LATIN CAPITAL LETTER I WITH MACRON"},
    {0x012B, 'i', '-',          "LATIN SMALL LETTER I WITH MACRON"},
    {0x012C, 'I', 'u',          "LATIN CAPITAL LETTER I WITH BREVE"},
    {0x012D, 'i', 'u',          "LATIN SMALL LETTER I WITH BREVE"},
    {0x012E, 'I', ',',          "LATIN CAPITAL LETTER I WITH OGONEK"},
    {0x012F, 'i', ',',          "LATIN SMALL LETTER I WITH OGONEK"},

    {0x0139, 'L', '\'',         "LATIN CAPITAL LETTER L WITH ACUTE"},
    {0x013a, 'l', '\'',         "LATIN SMALL LETTER L WITH ACUTE"},
    {0x013b, 'L', ',',          "LATIN CAPITAL LETTER L WITH CEDILLA"},
    {0x013c, 'l', ',',          "LATIN SMALL LETTER L WITH CEDILLA"},
    {0x013d, 'L', '?',          "LATIN CAPITAL LETTER L WITH CARON"},
    {0x013e, 'l', '?',          "LATIN SMALL LETTER L WITH CARON"},
    {0x013f, 'L', '.',          "LATIN CAPITAL LETTER L WITH MIDDLE DOT"},
    {0x0140, 'l', '.',          "LATIN SMALL LETTER L WITH MIDDLE DOT"},
    {0x0141, 'L', '-',          "LATIN CAPITAL LETTER L WITH STROKE"},
    {0x0142, 'l', '-',          "LATIN SMALL LETTER L WITH STROKE"},
    {0x0143, 'N', '\'',         "LATIN CAPITAL LETTER N WITH ACUTE"},
    {0x0144, 'n', '\'',         "LATIN SMALL LETTER N WITH ACUTE"},
    {0x0145, 'N', ',',          "LATIN CAPITAL LETTER N WITH CEDILLA"},
    {0x0146, 'n', ',',          "LATIN SMALL LETTER N WITH CEDILLA"},
    {0x0147, 'N', '?',          "LATIN CAPITAL LETTER N WITH CARON"},
    {0x0148, 'n', '?',          "LATIN SMALL LETTER N WITH CARON"},
    {0x0149, 'n', '`',          "LATIN SMALL LETTER N PRECEDED BY APOSTROPHE"},

    {0x0150, 'O', '~',          "LATIN CAPITAL LETTER O WITH DOUBLE ACUTE"},
    {0x0151, 'o', '~',          "LATIN SMALL LETTER O WITH DOUBLE ACUTE"},

    {0x0154, 'R', '\'',         "LATIN CAPITAL LETTER R WITH ACUTE"},
    {0x0155, 'r', '\'',         "LATIN SMALL LETTER R WITH ACUTE"},

    {0x0158, 'R', '?',          "LATIN CAPITAL LETTER R WITH CARON"},
    {0x0159, 'r', '?',          "LATIN SMALL LETTER R WITH CARON"},
    {0x015a, 'S', '\'',         "LATIN CAPITAL LETTER S WITH ACUTE"},
    {0x015b, 's', '\'',         "LATIN SMALL LETTER S WITH ACUTE"},
    {0x015c, 'S', '^',          "LATIN CAPITAL LETTER S WITH CIRCUMFLEX"},
    {0x015d, 's', '^',          "LATIN SMALL LETTER S WITH CIRCUMFLEX"},
    {0x015e, 'S', ',',          "LATIN CAPITAL LETTER S WITH CEDILLA"},
    {0x015f, 's', ',',          "LATIN SMALL LETTER S WITH CEDILLA"},
    {0x0160, 'S', '?',          "LATIN CAPITAL LETTER S WITH CARON"},
    {0x0161, 's', '?',          "LATIN SMALL LETTER S WITH CARON"},
    {0x0162, 'T', ',',          "LATIN CAPITAL LETTER T WITH CEDILLA"},
    {0x0163, 't', ',',          "LATIN SMALL LETTER T WITH CEDILLA"},
    {0x0164, 'T', '?',          "LATIN CAPITAL LETTER T WITH CARON"},
    {0x0165, 't', '?',          "LATIN SMALL LETTER T WITH CARON"},

    {0x016e, 'U', 'o',          "LATIN CAPITAL LETTER U WITH RING ABOVE"},
    {0x016f, 'u', 'o',          "LATIN SMALL LETTER U WITH RING ABOVE"},
    {0x0170, 'U', '~',          "LATIN CAPITAL LETTER U WITH DOUBLE ACUTE"},
    {0x0171, 'u', '~',          "LATIN SMALL LETTER U WITH DOUBLE ACUTE"},
    {0x0172, 'U', ',',          "LATIN CAPITAL LETTER U WITH OGONEK"},
    {0x0173, 'u', ',',          "LATIN SMALL LETTER U WITH OGONEK"},
    {0x0174, 'W', '^',          "LATIN CAPITAL LETTER W WITH CIRCUMFLEX"},
    {0x0175, 'w', '^',          "LATIN SMALL LETTER W WITH CIRCUMFLEX"},
    {0x0176, 'Y', '^',          "LATIN CAPITAL LETTER Y WITH CIRCUMFLEX"},
    {0x0177, 'y', '^',          "LATIN SMALL LETTER Y WITH CIRCUMFLEX"},
    {0x0178, 'Y', '"',          "LATIN CAPITAL LETTER Y WITH DIAERESIS"},
    {0x0179, 'Z', '\'',         "LATIN CAPITAL LETTER Z WITH ACUTE"},
    {0x017a, 'z', '\'',         "LATIN SMALL LETTER Z WITH ACUTE"},
    {0x017b, 'Z', 'o',          "LATIN CAPITAL LETTER Z WITH DOT ABOVE"},
    {0x017c, 'z', 'o',          "LATIN SMALL LETTER Z WITH DOT ABOVE"},
    {0x017d, 'Z', '?',          "LATIN CAPITAL LETTER Z WITH CARON"},
    {0x017e, 'z', '?',          "LATIN SMALL LETTER Z WITH CARON"},
    {0x017F, 'f', '|',          "LATIN SMALL LETTER LONG S"},

    {0x02d8, ' ', '?',          "BREVE"},
    {0x02c7, '^', 'v',          "CARON"},
    {0x02d9, ' ', '`',          "DOT ABOVE"},
    {0x02db, 'v', '`',          "OGONEK"},
    {0x02dd, ' ', '~',          "DOUBLE ACUTE ACCENT"},
    {0x03a0, 'T', 'T',          "GREEK CAPITAL LETTER PI"},
    {0x03c0, 't', 't',          "GREEK SMALL LETTER PI"},
    {0x2014, '-', '=',          "EM DASH"},
    {0x20ac, 'C', '=',          "EURO SIGN"},
    {0x2122, 'T', 'M',          "TRADE MARK SIGN"},
    {0x2245, '=', '~',          "APPROXIMATELY EQUAL TO"},
    {0x2260, '=', '/',          "NOT EQUAL TO"},
    {0x2264, '=', '<',          "LESS-THAN OR EQUAL TO"},
    {0x2265, '=', '>',          "GREATER-THAN OR EQUAL TO"},
    {0, 0, 0}
};

#define HELP_COLUMNS    5

void free_compose_help (char ***r, int n)
{E_
    int i;
    assert (r);
    for (i = 0; r[i]; i++) {
        int j;
        assert (r[i]);
        for (j = 0; j < HELP_COLUMNS; j++) {
            assert (r[i][j]);
            free (r[i][j]);
        }
        assert (!r[i][HELP_COLUMNS]);
        free (r[i]);
    }
    assert (i == n);
    free (r);
}

#define IS_DIGIT(a)     ((a) >= '0' && (a) <= '9')
#define IS_UPPER(a)     ((a) >= 'A' && (a) <= 'Z')
#define IS_LOWER(a)     ((a) >= 'a' && (a) <= 'z')
#define CHAR_FLAVOR(a) ( \
            IS_DIGIT(a) ? -1 : ( \
                IS_UPPER(a) ? -2 : ( \
                    IS_LOWER(a) ? -3 : ( \
                        (a) > 0 ? (a) : -4 \
                    ) \
                ) \
            ) \
        )

/* Condense a string from, say, "!.01234ABCD" to "!.0-4A-D" for display in the status line */
static char *possible_char_ (int first_char)
{E_
    static char out[256];
    int i, n = 0, wrote_dash = 0, min_char = 255, max_char = 0;
    char c[256];
    int last_flavor = 0, last_char;
    int flavor = 0;
    struct compose_international *p;
    memset (c, '\0', sizeof (c));
    for (p = compose_nonascii_unicode; p->unicode; p++) {
        if (first_char > 0) {
            if (p->ascii1 == first_char) {
                if (min_char > p->ascii2)
                    min_char = p->ascii2;
                if (max_char < p->ascii2)
                    max_char = p->ascii2;
                c[p->ascii2] = 1;
            }
            if (p->ascii2 == first_char) {
                if (min_char > p->ascii1)
                    min_char = p->ascii1;
                if (max_char < p->ascii1)
                    max_char = p->ascii1;
                c[p->ascii1] = 1;
            }
        } else {
            if (min_char > p->ascii1)
                min_char = p->ascii1;
            if (max_char < p->ascii1)
                max_char = p->ascii1;
            if (min_char > p->ascii2)
                min_char = p->ascii2;
            if (max_char < p->ascii2)
                max_char = p->ascii2;
            c[p->ascii1] = 1;
            c[p->ascii2] = 1;
        }
    }
    for (i = min_char; i <= max_char; i++) {
        if (!c[i]) {
            last_flavor = 0;
            continue;
        }
        flavor = CHAR_FLAVOR (i);
        if (flavor != last_flavor) {
            if (wrote_dash) {
                out[n] = last_char;
                if (n >= 2 && out[n - 2] == last_char - 2)
                    out[n - 1] = last_char - 1; /* change F-H into FGH */
                n++;
            }
            out[n++] = i;
            wrote_dash = 0;
        } else if (!wrote_dash) {
            out[n++] = '-';
            wrote_dash = 1;
        }
        last_char = i;
        last_flavor = flavor;
    }
    if (wrote_dash) {
        out[n] = last_char;
        if (n >= 2 && out[n - 2] == last_char - 2)
            out[n - 1] = last_char - 1; /* change F-H into FGH */
        n++;
    }
    out[n] = '\0';
    return out;
}

static int last_press = 0;

char *possible_char (void)
{E_
    return possible_char_ (last_press);
}

void compose_help (char ****r_, int *n)
{E_
    char ***r;
    char s[256];
    struct compose_international *p;
    int i;
    *n = 0;
    for (p = compose_nonascii_unicode; p->unicode; p++)
        (*n)++;
    r = (char ***) CMalloc (sizeof (char **) * (*n + 1));
    for (i = 0, p = compose_nonascii_unicode; i < *n; i++, p++) {
        r[i] = (char **) CMalloc (sizeof (char *) * (HELP_COLUMNS + 1));
        snprintf (s, sizeof (s), "  %s  ", wcrtomb_wchar_to_utf8 (p->unicode));
        r[i][0] = (char *) strdup (s);
        snprintf (s, sizeof (s), "  %c  ", p->ascii1);
        r[i][1] = (char *) strdup (s);
        snprintf (s, sizeof (s), "  %c  ", p->ascii2);
        r[i][2] = (char *) strdup (s);
        snprintf (s, sizeof (s), " %04lX ", (unsigned long) p->unicode);
        r[i][3] = (char *) strdup (s);
        snprintf (s, sizeof (s), " %s ", p->descr);
        r[i][4] = (char *) strdup (s);
        r[i][5] = NULL;
    }
    r[i] = NULL;
    *r_ = r;
}



int get_international_character (int key_press)
{E_
    int i;
    struct compose_international *compose;
    if (key_press == GET_INTL_CHAR_STATUS)
        return last_press;
    if (key_press == GET_INTL_CHAR_RESET_COMPOSE) {
	last_press = 0;
	return GET_INTL_CHAR_ERROR;
    }
    compose = compose_nonascii_unicode;
    if (last_press) {
	for (i = 0; compose[i].ascii1; i++) {
	    if ((compose[i].ascii2 == key_press && compose[i].ascii1 == last_press)
		||
		(compose[i].ascii1 == key_press && compose[i].ascii2 == last_press)) {
		last_press = 0;
		return compose[i].unicode;
	    }
	}
	last_press = 0;
	return GET_INTL_CHAR_ERROR;
    }
    for (i = 0; compose[i].ascii1; i++) {
	if (compose[i].ascii1 == key_press || compose[i].ascii2 == key_press) {
	    if (!compose[i].ascii2) {
		return compose[i].unicode;
	    } else {
		last_press = key_press;
		return GET_INTL_CHAR_BUSY;
	    }
	}
    }
    last_press = 0;
    return GET_INTL_CHAR_ERROR;
}

