/* international.c - routines for composing international characters
   Copyright (C) 1996-2018 Paul Sheer
 */



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


enum font_encoding get_editor_encoding (void);


int option_latin2 = 0;

struct compose_international {
    int extended_ascii;
    int unicode;
    unsigned char ascii1, ascii2;
};

/* Unicode and Latin1 are identical in the code point range up to 0xFF */
static struct compose_international compose_latin1[] = { \
    {0xA0, 0xA0, ' ', 0},		/* NO-BREAK SPACE */
    {0xA1, 0xA1, '^', '!'},		/* INVERTED EXCLAMATION MARK */
    {0xA2, 0xA2, 'c', '/'},		/* CENT SIGN */
    {0xA3, 0xA3, 'l', '-'},		/* POUND SIGN */
    {0xA4, 0xA4, 'x', 'o'},		/* CURRENCY SIGN */
    {0xA5, 0xA5, 'y', '='},		/* YEN SIGN */
    {0xA6, 0xA6, '|', '|'},		/* BROKEN BAR */
    {0xA7, 0xA7, 'O', 'S'},		/* SECTION SIGN */
    {0xA7, 0xA7, 'o', 'S'},		/* SECTION SIGN */
    {0xA8, 0xA8, '\'', '\''},		/* DIAERESIS */
    {0xA9, 0xA9, 'O', 'c'},		/* COPYRIGHT SIGN */
    {0xA9, 0xA9, 'o', 'c'},		/* COPYRIGHT SIGN */
    {0xAA, 0xAA, 'a', '_'},		/* FEMININE ORDINAL INDICATOR */
    {0xAB, 0xAB, '<', '<'},		/* LEFT-POINTING DOUBLE ANGLE QUOTATION MARK */
    {0xAC, 0xAC, '!', '!'},		/* NOT SIGN */
    {0xAD, 0xAD, '-', '-'},		/* SOFT HYPHEN */
    {0xAE, 0xAE, 'O', 'r'},		/* REGISTERED SIGN */
    {0xAE, 0xAE, 'o', 'r'},		/* REGISTERED SIGN */
    {0xAF, 0xAF, '^', '-'},		/* MACRON */
    {0xB0, 0xB0, '^', '0'},		/* DEGREE SIGN */
    {0xB1, 0xB1, '+', '-'},		/* PLUS-MINUS SIGN */
    {0xB2, 0xB2, '^', '2'},		/* SUPERSCRIPT TWO */
    {0xB3, 0xB3, '^', '3'},		/* SUPERSCRIPT THREE */
    {0xB4, 0xB4, '^', '\''},		/* ACUTE ACCENT */
    {0xB5, 0xB5, 'u', '|'},		/* MICRO SIGN */
    {0xB6, 0xB6, 'Q', '|'},		/* PILCROW SIGN */
    {0xB6, 0xB6, 'q', '|'},		/* PILCROW SIGN */
    {0xB7, 0xB7, '^', '.'},		/* MIDDLE DOT */
    {0xB8, 0xB8, '_', ','},		/* CEDILLA */
    {0xB9, 0xB9, '^', '1'},		/* SUPERSCRIPT ONE */
    {0xBA, 0xBA, 'o', '_'},		/* MASCULINE ORDINAL INDICATOR */
    {0xBB, 0xBB, '>', '>'},		/* RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK */
    {0xBC, 0xBC, '1', '4'},		/* VULGAR FRACTION ONE QUARTER */
    {0xBD, 0xBD, '1', '2'},		/* VULGAR FRACTION ONE HALF */
    {0xBE, 0xBE, '3', '4'},		/* VULGAR FRACTION THREE QUARTERS */
    {0xBF, 0xBF, '^', '?'},		/* INVERTED QUESTION MARK */
    {0xC0, 0xC0, 'A', '`'},		/* LATIN CAPITAL LETTER A WITH GRAVE ACCENT */
    {0xC1, 0xC1, 'A', '\''},		/* LATIN CAPITAL LETTER A WITH ACUTE ACCENT */
    {0xC2, 0xC2, 'A', '^'},		/* LATIN CAPITAL LETTER A WITH CIRCUMFLEX ACCENT */
    {0xC3, 0xC3, 'A', '~'},		/* LATIN CAPITAL LETTER A WITH TILDE */
    {0xC4, 0xC4, 'A', '"'},		/* LATIN CAPITAL LETTER A WITH DIAERESIS */
    {0xC5, 0xC5, 'A', 'o'},		/* LATIN CAPITAL LETTER A WITH RING ABOVE */
    {0xC6, 0xC6, 'A', 'E'},		/* LATIN CAPITAL LIGATURE AE */
    {0xC7, 0xC7, 'C', ','},		/* LATIN CAPITAL LETTER C WITH CEDILLA */
    {0xC8, 0xC8, 'E', '`'},		/* LATIN CAPITAL LETTER E WITH GRAVE ACCENT */
    {0xC9, 0xC9, 'E', '\''},		/* LATIN CAPITAL LETTER E WITH ACUTE ACCENT */
    {0xCA, 0xCA, 'E', '^'},		/* LATIN CAPITAL LETTER E WITH CIRCUMFLEX ACCENT */
    {0xCB, 0xCB, 'E', '"'},		/* LATIN CAPITAL LETTER E WITH DIAERESIS */
    {0xCC, 0xCC, 'I', '`'},		/* LATIN CAPITAL LETTER I WITH GRAVE ACCENT */
    {0xCD, 0xCD, 'I', '\''},		/* LATIN CAPITAL LETTER I WITH ACUTE ACCENT */
    {0xCE, 0xCE, 'I', '^'},		/* LATIN CAPITAL LETTER I WITH CIRCUMFLEX ACCENT */
    {0xCF, 0xCF, 'I', '"'},		/* LATIN CAPITAL LETTER I WITH DIAERESIS */
    {0xD0, 0xD0, 'D', '-'},		/* LATIN CAPITAL LETTER ETH */
    {0xD1, 0xD1, 'N', '~'},		/* LATIN CAPITAL LETTER N WITH TILDE */
    {0xD2, 0xD2, 'O', '`'},		/* LATIN CAPITAL LETTER O WITH GRAVE ACCENT */
    {0xD3, 0xD3, 'O', '\''},		/* LATIN CAPITAL LETTER O WITH ACUTE ACCENT */
    {0xD4, 0xD4, 'O', '^'},		/* LATIN CAPITAL LETTER O WITH CIRCUMFLEX ACCENT */
    {0xD5, 0xD5, 'O', '~'},		/* LATIN CAPITAL LETTER O WITH TILDE */
    {0xD6, 0xD6, 'O', '"'},		/* LATIN CAPITAL LETTER O WITH DIAERESIS */
    {0xD7, 0xD7, 'X', 0},		/* MULTIPLICATION SIGN */
    {0xD8, 0xD8, 'O', '/'},		/* LATIN CAPITAL LETTER O WITH STROKE */
    {0xD9, 0xD9, 'U', '`'},		/* LATIN CAPITAL LETTER U WITH GRAVE ACCENT */
    {0xDA, 0xDA, 'U', '\''},		/* LATIN CAPITAL LETTER U WITH ACUTE ACCENT */
    {0xDB, 0xDB, 'U', '^'},		/* LATIN CAPITAL LETTER U WITH CIRCUMFLEX ACCENT */
    {0xDC, 0xDC, 'U', '"'},		/* LATIN CAPITAL LETTER U WITH DIAERESIS */
    {0xDD, 0xDD, 'Y', '\''},		/* LATIN CAPITAL LETTER Y WITH ACUTE ACCENT */
    {0xDE, 0xDE, 'P', '|'},		/* LATIN CAPITAL LETTER THORN */
    {0xDF, 0xDF, 's', 0},		/* LATIN SMALL LETTER SHARP S */
    {0xE0, 0xE0, 'a', '`'},		/* LATIN SMALL LETTER A WITH GRAVE ACCENT */
    {0xE1, 0xE1, 'a', '\''},		/* LATIN SMALL LETTER A WITH ACUTE ACCENT */
    {0xE2, 0xE2, 'a', '^'},		/* LATIN SMALL LETTER A WITH CIRCUMFLEX ACCENT */
    {0xE3, 0xE3, 'a', '~'},		/* LATIN SMALL LETTER A WITH TILDE */
    {0xE4, 0xE4, 'a', '"'},		/* LATIN SMALL LETTER A WITH DIAERESIS */
    {0xE5, 0xE5, 'a', 'o'},		/* LATIN SMALL LETTER A WITH RING ABOVE */
    {0xE6, 0xE6, 'a', 'e'},		/* LATIN SMALL LIGATURE AE */
    {0xE7, 0xE7, 'c', ','},		/* LATIN SMALL LETTER C WITH CEDILLA */
    {0xE8, 0xE8, 'e', '`'},		/* LATIN SMALL LETTER E WITH GRAVE ACCENT */
    {0xE9, 0xE9, 'e', '\''},		/* LATIN SMALL LETTER E WITH ACUTE ACCENT */
    {0xEA, 0xEA, 'e', '^'},		/* LATIN SMALL LETTER E WITH CIRCUMFLEX ACCENT */
    {0xEB, 0xEB, 'e', '"'},		/* LATIN SMALL LETTER E WITH DIAERESIS */
    {0xEC, 0xEC, 'i', '`'},		/* LATIN SMALL LETTER I WITH GRAVE ACCENT */
    {0xED, 0xED, 'i', '\''},		/* LATIN SMALL LETTER I WITH ACUTE ACCENT */
    {0xEE, 0xEE, 'i', '^'},		/* LATIN SMALL LETTER I WITH CIRCUMFLEX ACCENT */
    {0xEF, 0xEF, 'i', '"'},		/* LATIN SMALL LETTER I WITH DIAERESIS */
    {0xF0, 0xF0, 'd', '-'},		/* LATIN SMALL LETTER ETH */
    {0xF1, 0xF1, 'n', '~'},		/* LATIN SMALL LETTER N WITH TILDE */
    {0xF2, 0xF2, 'o', '`'},		/* LATIN SMALL LETTER O WITH GRAVE ACCENT */
    {0xF3, 0xF3, 'o', '\''},		/* LATIN SMALL LETTER O WITH ACUTE ACCENT */
    {0xF4, 0xF4, 'o', '^'},		/* LATIN SMALL LETTER O WITH CIRCUMFLEX ACCENT */
    {0xF5, 0xF5, 'o', '~'},		/* LATIN SMALL LETTER O WITH TILDE */
    {0xF6, 0xF6, 'o', '"'},		/* LATIN SMALL LETTER O WITH DIAERESIS */
    {0xF7, 0xF7, ':', '-'},		/* DIVISION SIGN */
    {0xF8, 0xF8, 'o', '/'},		/* LATIN SMALL LETTER O WITH OBLIQUE BAR */
    {0xF9, 0xF9, 'u', '`'},		/* LATIN SMALL LETTER U WITH GRAVE ACCENT */
    {0xFA, 0xFA, 'u', '\''},		/* LATIN SMALL LETTER U WITH ACUTE ACCENT */
    {0xFB, 0xFB, 'u', '^'},		/* LATIN SMALL LETTER U WITH CIRCUMFLEX ACCENT */
    {0xFC, 0xFC, 'u', '"'},		/* LATIN SMALL LETTER U WITH DIAERESIS */
    {0xFD, 0xFD, 'y', '\''},		/* LATIN SMALL LETTER Y WITH ACUTE ACCENT */
    {0xFE, 0xFE, 'p', '|'},		/* LATIN SMALL LETTER THORN */
    {0xFF, 0xFF, 'y', '"'},		/* LATIN SMALL LETTER Y WITH DIAERESIS */
    {0, 0, 0}
};

/*
 * Latin 2 ascii table
 * :: I am not absolutely sure with it :-(
 * :: M.K.
 */

static struct compose_international \
compose_latin2[] = { \
    {0xA0, 0x00a0, ' ', 0},		/* NO-BREAK SPACE */
    {0xA1, 0x0104, 'A', ','},		/* A with tilde */
    {0xA2, 0x02d8, ' ', '?'},		/* Caron */
    {0xA3, 0x0141, 'l', '-'},		/* POUND SIGN */
    {0xA4, 0x00a4, 'x', 'o'},		/* CURRENCY SIGN */
    {0xA5, 0x013d, 'L', '?'},		/* L with Caron */
    {0xA6, 0x015a, 'S', '\''},		/* S Acute */
    {0xA7, 0x00a7, '8', 'o'},		/* Paragraph */
    {0xA8, 0x00a8, ' ', '"'},		/* DIAERESIS */
    {0xA9, 0x0160, 'S', '?'},		/* S with Caron */
    {0xAA, 0x015e, 'S', ','},		/* S with cedilla */
    {0xAB, 0x0164, 'T', '?'},		/* T with Caron */
    {0xAC, 0x0179, 'Z', '\''},		/* Z acute */
    {0xAD, 0x00ad, ' ', '-'},		/* SOFT HYPHEN */
    {0xAE, 0x017d, 'Z', '?'},		/* Z with Caron */
    {0xAF, 0x017b, 'Z', 'o'},		/* Z with dot above */
    {0xB0, 0x00b0, ' ', 'o'},		/* DEGREE SIGN */
    {0xB1, 0x0105, 'a', ','},		/* a with cedilla */
    {0xB2, 0x02db, ' ', '`'},		/* tilde !?*/
    {0xB3, 0x0142, 'l', '-'},		/* -l- */
    {0xB4, 0x00b4, ' ', '\''},		/* acute */
    {0xB5, 0x013e, 'l', '?'},		/* l with Caron */
    {0xB6, 0x015b, 's', '\''},		/* s acute */
    {0xB7, 0x02c7, ' ', '?'},		/* Caron */
    {0xB8, 0x00b8, ' ', ','},		/* cedilla */ 
    {0xB9, 0x0161, 's', '?'},		/* s with Caron */
    {0xBA, 0x015f, 's', '\''},		/* s with tilde */
    {0xBB, 0x0165, 't', '?'},		/* t with Caron */
    {0xBC, 0x017a, 'z', '\''},		/* z acute */
    {0xBD, 0x02dd, ' ', '~'},		/* tilde */
    {0xBE, 0x017e, 'z', '?'},		/* z with Caron */
    {0xBF, 0x017c, 'z', 'o'},		/* z with dot above */
    {0xC0, 0x0154, 'R', '\''},		/* R acute */
    {0xC1, 0x00c1, 'A', '\''},		/* LATIN CAPITAL LETTER A WITH ACUTE ACCENT */
    {0xC2, 0x00c2, 'A', '^'},		/* LATIN CAPITAL LETTER A WITH CIRCUMFLEX ACCENT */
    {0xC3, 0x0102, 'A', '?'},		/* A with Caron */
    {0xC4, 0x00c4, 'A', '"'},		/* LATIN CAPITAL LETTER A WITH DIAERESIS */
    {0xC5, 0x0139, 'L', '\''},		/* L acute */
    {0xC6, 0x0106, 'C', '\''},		/* C acute */
    {0xC7, 0x00c7, 'C', ','},		/* LATIN CAPITAL LETTER C WITH CEDILLA */
    {0xC8, 0x010c, 'C', '?'},		/* C with Caron */
    {0xC9, 0x00c9, 'E', '\''},		/* LATIN CAPITAL LETTER E WITH ACUTE ACCENT */
    {0xCA, 0x0118, 'E', ','},		/* E with cedilla */
    {0xCB, 0x00cb, 'E', '"'},		/* LATIN CAPITAL LETTER E WITH DIAERESIS */
    {0xCC, 0x011a, 'E', '?'},		/* E with Caron */
    {0xCD, 0x00cd, 'I', '\''},		/* LATIN CAPITAL LETTER I WITH ACUTE ACCENT */
    {0xCE, 0x00ce, 'I', '^'},		/* LATIN CAPITAL LETTER I WITH CIRCUMFLEX ACCENT */
    {0xCF, 0x010e, 'D', '?'},		/* D with Caron */
    {0xD0, 0x0110, 'D', '-'},		/* LATIN CAPITAL LETTER ETH */
    {0xD1, 0x0143, 'N', '\''},		/* N acute */
    {0xD2, 0x0147, 'N', '?'},		/* N with Caron */
    {0xD3, 0x00d3, 'O', '\''},		/* LATIN CAPITAL LETTER O WITH ACUTE ACCENT */
    {0xD4, 0x00d4, 'O', '^'},		/* LATIN CAPITAL LETTER O WITH CIRCUMFLEX ACCENT */
    {0xD5, 0x0150, 'O', '~'},		/* LATIN CAPITAL LETTER O WITH TILDE */
    {0xD6, 0x00d6, 'O', '"'},		/* LATIN CAPITAL LETTER O WITH DIAERESIS */
    {0xD7, 0x00d7, 'X', 'o'},		/* MULTIPLICATION SIGN */
    {0xD8, 0x0158, 'R', '?'},		/* R with Caron */
    {0xD9, 0x016e, 'U', 'o'},		/* LATIN CAPITAL LETTER U WITH GRAVE ACCENT */
    {0xDA, 0x00da, 'U', '\''},		/* LATIN CAPITAL LETTER U WITH ACUTE ACCENT */
    {0xDB, 0x0170, 'U', '~'},		/* U with cedilla */
    {0xDC, 0x00dc, 'U', '^'},		/* U with DIAERESIS */
    {0xDD, 0x00dd, 'Y', '\''},		/* LATIN CAPITAL LETTER Y WITH ACUTE ACCENT */
    {0xDE, 0x0162, 'T', ','},		/* T with cedilla */
    {0xDF, 0x00df, 'S', '^'},		/* ß */
    {0xE0, 0x0155, 'r', '\''},		/* r acute */
    {0xE1, 0x00e1, 'a', '\''},		/* LATIN SMALL LETTER A WITH ACUTE ACCENT */
    {0xE2, 0x00e2, 'a', '^'},		/* LATIN SMALL LETTER A WITH CIRCUMFLEX ACCENT */
    {0xE3, 0x0103, 'a', '?'},		/* a with Caron */
    {0xE4, 0x00e4, 'a', '"'},		/* LATIN SMALL LETTER A WITH DIAERESIS */
    {0xE5, 0x013a, 'l', '\''},		/* l acute */
    {0xE6, 0x0107, 'c', '\''},		/* c acute */
    {0xE7, 0x00e7, 'c', ','},		/* LATIN SMALL LETTER C WITH CEDILLA */
    {0xE8, 0x010d, 'c', '?'},		/* c with Caret */
    {0xE9, 0x00e9, 'e', '\''},		/* LATIN SMALL LETTER E WITH ACUTE ACCENT */
    {0xEA, 0x0119, 'e', ','},		/* e with cedilla */
    {0xEB, 0x00eb, 'e', '"'},		/* LATIN SMALL LETTER E WITH DIAERESIS */
    {0xEC, 0x011b, 'e', '?'},		/* e with Caret */
    {0xED, 0x00ed, 'i', '\''},		/* LATIN SMALL LETTER I WITH ACUTE ACCENT */
    {0xEE, 0x00ee, 'i', '^'},		/* LATIN SMALL LETTER I WITH CIRCUMFLEX ACCENT */
    {0xEF, 0x010f, 'd', '?'},		/* d with Caret */
    {0xF0, 0x0111, 'd', '-'},		/* LATIN SMALL LETTER ETH */
    {0xF1, 0x0144, 'n', '\''},		/* n acute */
    {0xF2, 0x0148, 'n', '?'},		/* n with Caret */
    {0xF3, 0x00f3, 'o', '\''},		/* LATIN SMALL LETTER O WITH ACUTE ACCENT */
    {0xF4, 0x00f4, 'o', '^'},		/* LATIN SMALL LETTER O WITH CIRCUMFLEX ACCENT */
    {0xF5, 0x0151, 'o', '~'},		/* LATIN SMALL LETTER O WITH TILDE */
    {0xF6, 0x00f6, 'o', '"'},		/* LATIN SMALL LETTER O WITH DIAERESIS */
    {0xF7, 0x00f7, ':', '-'},		/* DIVISION SIGN */
    {0xF8, 0x0159, 'r', '?'},		/* r with Caret */
    {0xF9, 0x016f, 'u', 'o'},		/* u with ring above */
    {0xFA, 0x00fa, 'u', '\''},		/* LATIN SMALL LETTER U WITH ACUTE ACCENT */
    {0xFB, 0x0171, 'u', '~'},		/* u with tilde */
    {0xFC, 0x00fc, 'u', '"'},		/* LATIN SMALL LETTER U WITH DIAERESIS */
    {0xFD, 0x00fd, 'y', '\''},		/* LATIN SMALL LETTER Y WITH ACUTE ACCENT */
    {0xFE, 0x0163, 't', ','},		/* t with cedilla */
    {0xFF, 0x02d9, ' ', '`'},		/* dot above */
    {0, 0, 0}
};


int get_international_character (unsigned char key_press)
{
    int i;
    static int last_press = 0;
    struct compose_international *compose;
    if (!key_press) {
	last_press = 0;
	return 0;
    }
    if (option_latin2)
        compose = compose_latin2;
    else
        compose = compose_latin1;
    if (last_press) {
	for (i = 0; compose[i].ascii1; i++) {
	    if ((compose[i].ascii2 == key_press && compose[i].ascii1 == last_press)
		||
		(compose[i].ascii1 == key_press && compose[i].ascii2 == last_press)) {
		last_press = 0;
		return get_editor_encoding () == FONT_ENCODING_UTF8 ? compose[i].unicode : compose[i].extended_ascii;
	    }
	}
	last_press = 0;
	return 0;
    }
    for (i = 0; compose[i].ascii1; i++) {
	if (compose[i].ascii1 == key_press || compose[i].ascii2 == key_press) {
	    if (!compose[i].ascii2)
		return get_editor_encoding () == FONT_ENCODING_UTF8 ? compose[i].unicode : compose[i].extended_ascii;
	    else {
		last_press = key_press;
		return 1;
	    }
	}
    }
    return 0;
}

