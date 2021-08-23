/* widechar.c - handle multibyte and UTF-8 encoding
   Copyright (C) 1996-2018 Paul Sheer
 */


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
{
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
{
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
{
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
{
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
{
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
{
    struct mb_rule r;
    r = get_mb_rule (edit, byte_index);
    return r.ch;
}

