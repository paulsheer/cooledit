/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* widechar.c - handle multibyte and UTF-8 encoding
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include <edit.h>

#define MB_MARKER_DENSITY 64

enum font_encoding get_editor_encoding (void);

/*
     1 |    7 | 0vvvvvvv
     2 |   11 | 110vvvvv 10vvvvvv
     3 |   16 | 1110vvvv 10vvvvvv 10vvvvvv
     4 |   21 | 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv
     5 |   26 | 111110vv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv
     6 |   31 | 1111110v 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv 10vvvvvv
*/

unsigned char *wcrtomb_wchar_to_utf8 (C_wchar_t c)
{E_
    static unsigned char r[32];
    int i = 0;
#undef APPEND
#define APPEND(x) r[i++] = (unsigned char) (x)
    if (c < (1 << 7)) {
	APPEND (c);
    } else if (c < (1 << 11)) {
	APPEND ((c >> 6) | 0xC0);
	APPEND ((c & 0x3F) | 0x80);
    } else if (c < (1 << 16)) {
	APPEND ((c >> 12) | 0xE0);
	APPEND (((c >> 6) & 0x3F) | 0x80);
	APPEND ((c & 0x3F) | 0x80);
    } else if (c < (1 << 21)) {
	APPEND ((c >> 18) | 0xF0);
	APPEND (((c >> 12) & 0x3F) | 0x80);
	APPEND (((c >> 6) & 0x3F) | 0x80);
	APPEND ((c & 0x3F) | 0x80);
    } else if (c < (1 << 26)) {
	APPEND ((c >> 24) | 0xF8);
	APPEND (((c >> 18) & 0x3F) | 0x80);
	APPEND (((c >> 12) & 0x3F) | 0x80);
	APPEND (((c >> 6) & 0x3F) | 0x80);
	APPEND ((c & 0x3F) | 0x80);
    } else if (c < (1 << 31)) {
	APPEND ((c >> 30) | 0xFC);
	APPEND (((c >> 24) & 0x3F) | 0x80);
	APPEND (((c >> 18) & 0x3F) | 0x80);
	APPEND (((c >> 12) & 0x3F) | 0x80);
	APPEND (((c >> 6) & 0x3F) | 0x80);
	APPEND ((c & 0x3F) | 0x80);
    }
    APPEND ('\0');
    return r;
}


/* makes sense to me... (although only goes to 21 bits) */
int mbrtowc_utf8_to_wchar (C_wchar_t * c, const char *t, int n, void *x /* no shifting with utf8 */ )
{E_
    const unsigned char *s = (const unsigned char *) t;
    if (!*s) {
	*c = 0;
	return 0;
    }
    if (*s < 0x80) {
	*c = (C_wchar_t) * s;
	return 1;
    }
    if (*s < 0xC0)
	return -1;
    if (*s < 0xE0) {
	if (n < 2)
	    return -2;
	if ((s[1] & 0xC0) != 0x80)
	    return -1;
	*c = ((C_wchar_t) (s[0] & 0x1F) << 6) | (C_wchar_t) (s[1] & 0x3F);
	if (*c < (1 << 7))
	    return -1;
	return 2;
    }
    if (*s < 0xF0) {
	if (n < 3)
	    return -2;
	if ((s[1] & 0xC0) != 0x80)
	    return -1;
	if ((s[2] & 0xC0) != 0x80)
	    return -1;
	*c = ((C_wchar_t) (s[0] & 0x0F) << 12) | ((C_wchar_t) (s[1] & 0x3F) << 6) | (C_wchar_t) (s[2] & 0x3F);
	if (*c < (1 << 11))
	    return -1;
	return 3;
    }
    if (*s < 0xF8) {
	if (n < 4)
	    return -2;
	if ((s[1] & 0xC0) != 0x80)
	    return -1;
	if ((s[2] & 0xC0) != 0x80)
	    return -1;
	if ((s[3] & 0xC0) != 0x80)
	    return -1;
	*c =
	    ((C_wchar_t) (s[0] & 0x07) << 18) |
	    ((C_wchar_t) (s[1] & 0x3F) << 12) | ((C_wchar_t) (s[2] & 0x3F) << 6) | (C_wchar_t) (s[3] & 0x3F);
	if (*c < (1 << 16))
	    return -1;
	return 4;
    }
    if (*s < 0xFC) {
	if (n < 5)
	    return -2;
	if ((s[1] & 0xC0) != 0x80)
	    return -1;
	if ((s[2] & 0xC0) != 0x80)
	    return -1;
	if ((s[3] & 0xC0) != 0x80)
	    return -1;
	if ((s[4] & 0xC0) != 0x80)
	    return -1;
	*c =
	    ((C_wchar_t) (s[0] & 0x03) << 24) | ((C_wchar_t) (s[1] & 0x3F) << 18) |
	    ((C_wchar_t) (s[2] & 0x3F) << 12) | ((C_wchar_t) (s[3] & 0x3F) << 6) | (C_wchar_t) (s[4] & 0x3F);
	if (*c < (1 << 21))
	    return -1;
	return 5;
    }
    if (*s < 0xFE) {
	if (n < 6)
	    return -2;
	if ((s[1] & 0xC0) != 0x80)
	    return -1;
	if ((s[2] & 0xC0) != 0x80)
	    return -1;
	if ((s[3] & 0xC0) != 0x80)
	    return -1;
	if ((s[4] & 0xC0) != 0x80)
	    return -1;
	if ((s[5] & 0xC0) != 0x80)
	    return -1;
	*c =
	    ((C_wchar_t) (s[0] & 0x01) << 30) | ((C_wchar_t) (s[1] & 0x3F) << 24) | ((C_wchar_t) (s[2] & 0x3F) << 18) |
	    ((C_wchar_t) (s[3] & 0x3F) << 12) | ((C_wchar_t) (s[4] & 0x3F) << 6) | (C_wchar_t) (s[5] & 0x3F);
	if (*c < (1 << 26))
	    return -1;
	return 6;
    }
    return -1;
}

static inline struct mb_rule apply_mb_rules_going_right_utf8_to_wchar (WEdit * edit, long byte_index,
								      struct mb_rule mb_rule)
{E_
    C_wchar_t wc;
    unsigned char p[16];
    int n;
    if (mb_rule.end) {
	mb_rule.end--;
	mb_rule.ch = -1;
	return mb_rule;
    }
    for (n = 0; n < 6; n++) {
	int r;
	p[n] = edit_get_byte (edit, byte_index + n);
	r = mbrtowc_utf8_to_wchar (&wc, (char *) p, n + 1, &mb_rule.shift_state);
	if (r >= 0) {
	    mb_rule.end = n;
	    mb_rule.ch = wc;
	    return mb_rule;
	}
	if (r == -1) {
	    mb_rule.end = 0;
	    mb_rule.ch = (unsigned long) *p | 0x80000000;
	    return mb_rule;
	}
    }
    mb_rule.end = 0;
    mb_rule.ch = -1;
    return mb_rule;
}

static inline struct mb_rule apply_mb_rules_going_right (WEdit * edit, long byte_index, struct mb_rule mb_rule)
{E_
#ifdef HAVE_WCHAR_H
    C_wchar_t wc;
    unsigned char p[16];
    int n;
    if (mb_rule.end) {
	mb_rule.end--;
	mb_rule.ch = -1;
	return mb_rule;
    }
    for (n = 0; n < MB_CUR_MAX && n < sizeof(p) - 1; n++) {
	int r;
        wchar_t cwc = 0;
        mbstate_t s;
	p[n] = edit_get_byte (edit, byte_index + n);
        s = mb_rule.shift_state;
	r = mbrtowc (&cwc, (char *) p, n + 1, &s);
        wc = (wchar_t) cwc;
	if (r >= 0) {
	    mb_rule.end = n;
	    mb_rule.ch = wc;
            mb_rule.shift_state = s;
	    return mb_rule;
	}
	if (r == -1) {
            memset (&mb_rule.shift_state, '\0', sizeof(mb_rule.shift_state));
	    mb_rule.end = 0;
	    mb_rule.ch = *p;
	    return mb_rule;
	}
    }
    mb_rule.end = 0;
    mb_rule.ch = -1;
#endif
    return mb_rule;
}

struct mb_rule get_mb_rule (WEdit * edit, long byte_index)
{E_
    long i;

    if (
#ifdef HAVE_WCHAR_H
            (MB_CUR_MAX == 1 && get_editor_encoding () == FONT_ENCODING_LOCALE) ||
#endif
            get_editor_encoding () == FONT_ENCODING_8BIT) {
	struct mb_rule r;
	r.end = 0;
	r.ch = edit_get_byte (edit, byte_index);
	return r;
    }
    if (edit->mb_invalidate) {
	struct _mb_marker *s;
	while (edit->mb_marker && edit->mb_marker->offset >= edit->last_get_mb_rule) {
	    s = edit->mb_marker->next;
	    free (edit->mb_marker);
	    edit->mb_marker = s;
	}
	if (edit->mb_marker) {
	    edit->last_get_mb_rule = edit->mb_marker->offset;
	    edit->mb_rule = edit->mb_marker->rule;
	} else {
	    edit->last_get_mb_rule = -1;
	    memset (&edit->mb_rule, 0, sizeof (edit->mb_rule));
	}
	edit->mb_invalidate = 0;
    }
    if (byte_index > edit->last_get_mb_rule) {
	if (get_editor_encoding () == FONT_ENCODING_UTF8) {
	    for (i = edit->last_get_mb_rule + 1; i <= byte_index; i++) {
		edit->mb_rule = apply_mb_rules_going_right_utf8_to_wchar (edit, i, edit->mb_rule);
		if (i >
		    (edit->mb_marker ? edit->mb_marker->offset +
		     MB_MARKER_DENSITY : MB_MARKER_DENSITY)) {
		    struct _mb_marker *s;
		    s = edit->mb_marker;
		    edit->mb_marker = malloc (sizeof (struct _mb_marker));
		    edit->mb_marker->next = s;
		    edit->mb_marker->offset = i;
		    edit->mb_marker->rule = edit->mb_rule;
		}
	    }
	} else {
	    for (i = edit->last_get_mb_rule + 1; i <= byte_index; i++) {
		edit->mb_rule = apply_mb_rules_going_right (edit, i, edit->mb_rule);
		if (i >
		    (edit->mb_marker ? edit->mb_marker->offset +
		     MB_MARKER_DENSITY : MB_MARKER_DENSITY)) {
		    struct _mb_marker *s;
		    s = edit->mb_marker;
		    edit->mb_marker = malloc (sizeof (struct _mb_marker));
		    edit->mb_marker->next = s;
		    edit->mb_marker->offset = i;
		    edit->mb_marker->rule = edit->mb_rule;
		}
	    }
	}
    } else if (byte_index < edit->last_get_mb_rule) {
	struct _mb_marker *s;
	for (;;) {
	    if (!edit->mb_marker) {
		memset (&edit->mb_rule, 0, sizeof (edit->mb_rule));
		if (get_editor_encoding () == FONT_ENCODING_UTF8) {
		    for (i = -1; i <= byte_index; i++)
			edit->mb_rule =
			    apply_mb_rules_going_right_utf8_to_wchar (edit, i, edit->mb_rule);
		} else {
		    for (i = -1; i <= byte_index; i++)
			edit->mb_rule = apply_mb_rules_going_right (edit, i, edit->mb_rule);
		}
		break;
	    }
	    if (byte_index >= edit->mb_marker->offset) {
		edit->mb_rule = edit->mb_marker->rule;
		if (get_editor_encoding () == FONT_ENCODING_UTF8) {
		    for (i = edit->mb_marker->offset + 1; i <= byte_index; i++)
			edit->mb_rule =
			    apply_mb_rules_going_right_utf8_to_wchar (edit, i, edit->mb_rule);
		} else {
		    for (i = edit->mb_marker->offset + 1; i <= byte_index; i++)
			edit->mb_rule = apply_mb_rules_going_right (edit, i, edit->mb_rule);
		}
		break;
	    }
	    s = edit->mb_marker->next;
	    free (edit->mb_marker);
	    edit->mb_marker = s;
	}
    }
    edit->last_get_mb_rule = byte_index;
    return edit->mb_rule;
}

long edit_get_wide_byte (WEdit * edit, long byte_index)
{E_
    struct mb_rule r;
    r = get_mb_rule (edit, byte_index);
    return r.ch;
}



struct widetable {
    C_wchar_t c;
    int wide;
};

static struct widetable is_wide_table[] = {
    { 0x1100, 1 },
    { 0x1160, 0 },
    { 0x231a, 1 },
    { 0x231c, 0 },
    { 0x2329, 1 },
    { 0x232b, 0 },
    { 0x23e9, 1 },
    { 0x23ed, 0 },
    { 0x23f0, 1 },
    { 0x23f1, 0 },
    { 0x23f3, 1 },
    { 0x23f4, 0 },
    { 0x25fd, 1 },
    { 0x25ff, 0 },
    { 0x2614, 1 },
    { 0x2616, 0 },
    { 0x2648, 1 },
    { 0x2654, 0 },
    { 0x267f, 1 },
    { 0x2680, 0 },
    { 0x2693, 1 },
    { 0x2694, 0 },
    { 0x26a1, 1 },
    { 0x26a2, 0 },
    { 0x26aa, 1 },
    { 0x26ac, 0 },
    { 0x26bd, 1 },
    { 0x26bf, 0 },
    { 0x26c4, 1 },
    { 0x26c6, 0 },
    { 0x26ce, 1 },
    { 0x26cf, 0 },
    { 0x26d4, 1 },
    { 0x26d5, 0 },
    { 0x26ea, 1 },
    { 0x26eb, 0 },
    { 0x26f2, 1 },
    { 0x26f4, 0 },
    { 0x26f5, 1 },
    { 0x26f6, 0 },
    { 0x26fa, 1 },
    { 0x26fb, 0 },
    { 0x26fd, 1 },
    { 0x26fe, 0 },
    { 0x2705, 1 },
    { 0x2706, 0 },
    { 0x270a, 1 },
    { 0x270c, 0 },
    { 0x2728, 1 },
    { 0x2729, 0 },
    { 0x274c, 1 },
    { 0x274d, 0 },
    { 0x274e, 1 },
    { 0x274f, 0 },
    { 0x2753, 1 },
    { 0x2756, 0 },
    { 0x2757, 1 },
    { 0x2758, 0 },
    { 0x2795, 1 },
    { 0x2798, 0 },
    { 0x27b0, 1 },
    { 0x27b1, 0 },
    { 0x27bf, 1 },
    { 0x27c0, 0 },
    { 0x2b1b, 1 },
    { 0x2b1d, 0 },
    { 0x2b50, 1 },
    { 0x2b51, 0 },
    { 0x2b55, 1 },
    { 0x2b56, 0 },
    { 0x2e80, 1 },
    { 0x2e9a, 0 },
    { 0x2e9b, 1 },
    { 0x2ef4, 0 },
    { 0x2f00, 1 },
    { 0x2fd6, 0 },
    { 0x2ff0, 1 },
    { 0x2ffc, 0 },
    { 0x3000, 1 },
    { 0x303f, 0 },
    { 0x3041, 1 },
    { 0x3097, 0 },
    { 0x3099, 1 },
    { 0x3100, 0 },
    { 0x3105, 1 },
    { 0x3130, 0 },
    { 0x3131, 1 },
    { 0x318f, 0 },
    { 0x3190, 1 },
    { 0x31e4, 0 },
    { 0x31f0, 1 },
    { 0x321f, 0 },
    { 0x3220, 1 },
    { 0x3248, 0 },
    { 0x3250, 1 },
    { 0x4dc0, 0 },
    { 0x4e00, 1 },
    { 0xa48d, 0 },
    { 0xa490, 1 },
    { 0xa4c7, 0 },
    { 0xa960, 1 },
    { 0xa97d, 0 },
    { 0xac00, 1 },
    { 0xd7a4, 0 },
    { 0xf900, 1 },
    { 0xfb00, 0 },
    { 0xfe10, 1 },
    { 0xfe1a, 0 },
    { 0xfe30, 1 },
    { 0xfe53, 0 },
    { 0xfe54, 1 },
    { 0xfe67, 0 },
    { 0xfe68, 1 },
    { 0xfe6c, 0 },
    { 0xff01, 1 },
    { 0xff61, 0 },
    { 0xffe0, 1 },
    { 0xffe7, 0 },
    { 0x16fe0, 1 },
    { 0x16fe5, 0 },
    { 0x16ff0, 1 },
    { 0x16ff2, 0 },
    { 0x17000, 1 },
    { 0x187f8, 0 },
    { 0x18800, 1 },
    { 0x18cd6, 0 },
    { 0x18d00, 1 },
    { 0x18d09, 0 },
    { 0x1aff0, 1 },
    { 0x1aff4, 0 },
    { 0x1aff5, 1 },
    { 0x1affc, 0 },
    { 0x1affd, 1 },
    { 0x1afff, 0 },
    { 0x1b000, 1 },
    { 0x1b123, 0 },
    { 0x1b132, 1 },
    { 0x1b133, 0 },
    { 0x1b150, 1 },
    { 0x1b153, 0 },
    { 0x1b155, 1 },
    { 0x1b156, 0 },
    { 0x1b164, 1 },
    { 0x1b168, 0 },
    { 0x1b170, 1 },
    { 0x1b2fc, 0 },
    { 0x1f004, 1 },
    { 0x1f005, 0 },
    { 0x1f0cf, 1 },
    { 0x1f0d0, 0 },
    { 0x1f18e, 1 },
    { 0x1f18f, 0 },
    { 0x1f191, 1 },
    { 0x1f19b, 0 },
    { 0x1f200, 1 },
    { 0x1f203, 0 },
    { 0x1f210, 1 },
    { 0x1f23c, 0 },
    { 0x1f240, 1 },
    { 0x1f249, 0 },
    { 0x1f250, 1 },
    { 0x1f252, 0 },
    { 0x1f260, 1 },
    { 0x1f266, 0 },
    { 0x1f300, 1 },
    { 0x1f321, 0 },
    { 0x1f32d, 1 },
    { 0x1f336, 0 },
    { 0x1f337, 1 },
    { 0x1f37d, 0 },
    { 0x1f37e, 1 },
    { 0x1f394, 0 },
    { 0x1f3a0, 1 },
    { 0x1f3cb, 0 },
    { 0x1f3cf, 1 },
    { 0x1f3d4, 0 },
    { 0x1f3e0, 1 },
    { 0x1f3f1, 0 },
    { 0x1f3f4, 1 },
    { 0x1f3f5, 0 },
    { 0x1f3f8, 1 },
    { 0x1f43f, 0 },
    { 0x1f440, 1 },
    { 0x1f441, 0 },
    { 0x1f442, 1 },
    { 0x1f4fd, 0 },
    { 0x1f4ff, 1 },
    { 0x1f53e, 0 },
    { 0x1f54b, 1 },
    { 0x1f54f, 0 },
    { 0x1f550, 1 },
    { 0x1f568, 0 },
    { 0x1f57a, 1 },
    { 0x1f57b, 0 },
    { 0x1f595, 1 },
    { 0x1f597, 0 },
    { 0x1f5a4, 1 },
    { 0x1f5a5, 0 },
    { 0x1f5fb, 1 },
    { 0x1f650, 0 },
    { 0x1f680, 1 },
    { 0x1f6c6, 0 },
    { 0x1f6cc, 1 },
    { 0x1f6cd, 0 },
    { 0x1f6d0, 1 },
    { 0x1f6d3, 0 },
    { 0x1f6d5, 1 },
    { 0x1f6d8, 0 },
    { 0x1f6dc, 1 },
    { 0x1f6e0, 0 },
    { 0x1f6eb, 1 },
    { 0x1f6ed, 0 },
    { 0x1f6f4, 1 },
    { 0x1f6fd, 0 },
    { 0x1f7e0, 1 },
    { 0x1f7ec, 0 },
    { 0x1f7f0, 1 },
    { 0x1f7f1, 0 },
    { 0x1f90c, 1 },
    { 0x1f93b, 0 },
    { 0x1f93c, 1 },
    { 0x1f946, 0 },
    { 0x1f947, 1 },
    { 0x1fa00, 0 },
    { 0x1fa70, 1 },
    { 0x1fa7d, 0 },
    { 0x1fa80, 1 },
    { 0x1fa89, 0 },
    { 0x1fa90, 1 },
    { 0x1fabe, 0 },
    { 0x1fabf, 1 },
    { 0x1fac6, 0 },
    { 0x1face, 1 },
    { 0x1fadc, 0 },
    { 0x1fae0, 1 },
    { 0x1fae9, 0 },
    { 0x1faf0, 1 },
    { 0x1faf9, 0 },
    { 0x20000, 1 },
    { 0x2fffe, 0 },
    { 0x30000, 1 },
    { 0x3fffe, 0 },
};


int is_unicode_doublewidth_char (C_wchar_t c)
{
    int lower = 0;
    int upper = sizeof (is_wide_table) / sizeof (is_wide_table[0]) - 1;
    int i = (sizeof (is_wide_table) / sizeof (is_wide_table[0])) / 2;

    if (c < is_wide_table[0].c)
        return 0;
    if (c >= is_wide_table[upper].c)
        return is_wide_table[upper].wide;

    do {
        if (c >= is_wide_table[i].c && c < is_wide_table[i + 1].c)
            return is_wide_table[i].wide;
        if (c == is_wide_table[i + 1].c)
            return is_wide_table[i + 1].wide;
        if (c < is_wide_table[i].c)
            upper = i - 1;
        else if (c > is_wide_table[i].c)
            lower = i + 1;
        i = (lower + upper) / 2;
    }
    while (lower <= upper);

    return 0;
}


