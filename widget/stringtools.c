/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* stringtools.c - convenient string utility functions
   Copyright (C) 1996-2022 Paul Sheer
 */



#include "inspect.h"
#include <config.h>
#include "global.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_string.h"
#include <stdarg.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include "stringtools.h"
#include "regex.h"
#include "time.h"
#include "remotefs.h"


/*
   This cats a whole lot of strings together.
   It has the advantage that the return result will
   be free'd automatically, and MUST NOT be free'd
   by the caller.
   It will hold the most recent NUM_STORED strings.
 */
#define NUM_STORED 256
#define STATIC_LIMIT 240

struct garbage {
    char *alloced;
    char *p;
    char data[STATIC_LIMIT];
};

static struct garbage stacked[NUM_STORED];

char *catstrs (const char *first,...)
{E_
    static int i = -1;
    va_list ap;
    int len;
    const char *data;
    char *s;
    struct garbage *t;

    if (!first)
	return 0;

    if (i == -1) {
	memset (stacked, '\0', sizeof (stacked));
	i = 0;
    }

    va_start (ap, first);

    len = 0;
    data = first;
    do {
	len += strlen(data);
    } while ((data = va_arg (ap, char *)) != 0);
    len++;

    i = (i + 1) % NUM_STORED;
    t = &stacked[i];
    if (t->alloced) {
	free (t->alloced);
	t->alloced = NULL;
    }

    if (len > STATIC_LIMIT) {
	t->alloced = (char *) malloc (len);
	t->p = t->alloced;
    } else {
	t->alloced = NULL;
	t->p = &t->data[0];
    }
    va_end (ap);

    va_start (ap, first);

    s = t->p;
    data = first;
    do {
	strcpy (s, data);
	s += strlen(s);
    } while ((data = va_arg (ap, char *)) != 0);

    va_end (ap);

    return t->p;
}

void catstrs_clean (void)
{E_
    int i;
    for (i = 0; i < NUM_STORED; i++)
	if (stacked[i].alloced) {
	    free (stacked[i].alloced);
	    stacked[i].alloced = NULL;
	}
}

int strlcpy (char *dst, const char *src, int siz)
{E_
    char *d = dst;
    const char *s = src;
    int n = siz;
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }
    if (n == 0) {
        if (siz != 0)
            *d = '\0';
        while (*s++);
    }
    return (s - src - 1);
}

char *space_string (const char *s)
{E_
    char *r, *p;
    int i;
    if (!s)
	return 0;
    p = r = malloc (strlen (s) + 3);
    while (*s == ' ')
	s++;
    *r++ = ' ';
    while (*s) {
	if (*s != '&')
	    *r++ = *s;
	s++;
    }
    *r = '\0';
    for (i = (r - p); i > 1; i--)
	if (p[i - 1] != ' ')
	    break;
    p[i++] = ' ';
    p[i] = '\0';
    return p;
}

#define ISSPACE(c)              ((c) <= ' ')
void string_chomp (char *_p)
{E_
    unsigned char *p, *q, *t;
    p = (unsigned char *) _p;
    assert (p);
    for (q = p; *q; q++)
        if (!ISSPACE (*q))
            break;
    for (t = p; *q;)
        if (!ISSPACE (*p++ = *q++))
            t = p;
    *t = '\0';
}

#define Ctolower(c)             (((c) >= 'A' && (c) <= 'Z') ? ((c) + 'a' - 'A') : (c))

int Cstrncasecmp (const char *p1, const char *p2, size_t n)
{E_
    unsigned char *s1 = (unsigned char *) p1, *s2 = (unsigned char *) p2;
    signed int c = 0;
    while (n--)
	if ((c = Ctolower ((int) *s1) - Ctolower ((int) *s2)) != 0 || !*s1++ || !*s2++)
	    break;
    return c;
}


void *Cmemmove (void *dest, const void *src, size_t n)
{E_
    char *t, *s;

    if (dest <= src) {
	t = (char *) dest;
	s = (char *) src;
	while (n--)
	    *t++ = *s++;
    } else {
	t = (char *) dest + n;
	s = (char *) src + n;
	while (n--)
	    *--t = *--s;
    }
    return dest;
}


int Cstrcasecmp (const char *p1, const char *p2)
{E_
    unsigned char *s1 = (unsigned char *) p1, *s2 = (unsigned char *) p2;
    signed int c;
    for (;;)
	if ((c = Ctolower ((int) *s1) - Ctolower ((int) *s2)) != 0 || !*s1++ || !*s2++)
	    break;
    return c;
}

char *Cstrdup (const char *p)
{E_
    char *s;
    int l;
    l = strlen ((char *) p);
    s = (char *) malloc (l + 1);
    memcpy (s, p, l);
    s[l] = '\0';
    return s;
}

CStr CStr_dup (const char *s)
{E_
    CStr r;
    r.len = strlen (s);
    r.data = (char *) malloc (r.len + 1);
    memcpy (r.data, s, r.len);
    r.data[r.len] = '\0';
    return r;
}

CStr CStr_const (const char *s, int l)
{E_
    CStr r;
    r.data = (char *) s;
    r.len = l;
    return r;
}

CStr CStr_cpy (const char *s, int l)
{E_
    CStr r;
    r.len = l;
    r.data = (char *) malloc (r.len + 1);
    memcpy (r.data, s, r.len);
    r.data[r.len] = '\0';
    return r;
}

CStr CStr_dupstr (CStr s)
{E_
    return CStr_cpy(s.data, s.len);
}

void CStr_free(CStr *s)
{E_
    if (s->data)
        free(s->data);
    s->data = 0;
    s->len = 0;
}

char *replace_str (const char *in, const char *a, const char *b)
{E_
    char *out;
    int found = 0;
    const char *p;
    char *q;
    int al, bl, outl;
    al = strlen (a);
    bl = strlen (b);
    for (p = in; *p;) {
        if (!memcmp (p, a, al)) {
            p += al;
            found++;
        } else {
            p++;
        }
    }
    outl = strlen (in) + found * (bl - al) + 1;
    out = (char *) malloc (outl);
    for (p = in, q = out; *p;) {
        if (!memcmp (p, a, al)) {
            memcpy (q, b, bl);
            p += al;
            q += bl;
        } else {
            *q++ = *p++;
        }
    }
    *q++ = '\0';
    assert (q == out + outl);
    return out;
}

int strendswith(const char *s, const char *p)
{E_
    int sl, pl;
    sl = strlen(s);
    pl = strlen(p);
    if (pl > sl)
        return 0;
    return !strcmp(s + sl - pl, p);
}

/* alternative to free() */
void destroy (void **p)
{E_
    if (*p) {
	free (*p);
	*p = 0;
    }
}

char *strcasechr (const char *p, int c)
{E_
    unsigned char *s = (unsigned char *) p;
    for (; my_lower_case ((int) *s) != my_lower_case ((int) c); ++s)
	if (*s == '\0')
	    return 0;
    return (char *) s;
}

char *Citoa (int i)
{E_
    static char t[20];
    char *s = t + 19;
    int j = i;
    i = abs (i);
    *s-- = 0;
    do {
	*s-- = i % 10 + '0';
    } while ((i = i / 10));
    if (j < 0)
	*s-- = '-';
    return ++s;
}

/*
   cd's to path and sets current_dir variable if getcwd works, else set
   current_dir to "".
 */
extern char current_dir[];
int change_directory (const char *path, char *errmsg)
{E_
    struct remotefs *u;
    u = the_remotefs_local;
    return (*u->remotefs_chdir) (u, path, current_dir, MAX_PATH_LEN, errmsg);
}


short *shortset (short *s, int c, size_t n)
{E_
    short *r = s;
    while (n--)
	*s++ = c;
    return r;
}

char *name_trunc (const char *txt, int trunc_len)
{E_
    static char x[1024];
    int txt_len, y;
    int mid;
    mid = trunc_len / 2;
    txt_len = strlen (txt);
    if (txt_len <= trunc_len || trunc_len < 3) {
	strcpy (x, txt);
	return x;
    }
    y = trunc_len % 2;
    while (((const unsigned char *) txt)[mid + y] >= 0xC0)        /* for unicode */
        mid++;
    strncpy (x, txt, mid + y);
    strcpy (x + mid + y, txt + txt_len - mid - y);
    x[mid + y] = '~';
    return x;
}

int prop_font_strcolmove (unsigned char *str, int i, int column);

int strcolmove (unsigned char *str, int i, int column)
{E_
    return prop_font_strcolmove (str, i, column);
}

/*move to col character from beginning of line with i in the line somewhere. */
/*If col is past the end of the line, it returns position of end of line */
long strfrombeginline (const char *s, int i, int col)
{E_
    unsigned char *str = (unsigned char *) s;
    if (i < 0) {
/* NLS ? */
	fprintf (stderr, "strfrombeginline called with negative index.\n");
	exit (1);
    }
    while (i--)
	if (str[i] == '\n') {
	    i++;
	    break;
	}
    if (i < 0)
	i = 0;
    if (!col)
	return i;
    return strcolmove (str, i, col);
}

/*
   strip backspaces from the nroff file to produce normal text.
   returns strlen(result) if l is non null
 */
char *str_strip_nroff (char *t, int *l)
{E_
    unsigned char *s = (unsigned char *) t;
    unsigned char *r, *q;
    int p;

    q = r = malloc (strlen (t) + 2);
    if (!r)
	return 0;

    for (p = 0; s[p]; p++) {
	while (s[p + 1] == '\b' && isprint (s[p + 2]) && isprint (s[p]))
	    p += 2;
	*q++ = s[p];
    }
    *q = 0;
    if (l)
	*l = ((unsigned long) q - (unsigned long) r);
    return (char *) r;
}

long countlinesforward (const char *text, long from, long amount, long lines, int width)
{E_
    if (amount) {
	int i = 0;
	amount += from;
	for (;;) {
	    from = strcolmove ((unsigned char *) text, from, width);
	    if (from >= amount || !text[from])
		return i;
	    i++;
	    from++;
	}
    } else if (lines) {
	int i;
	for (i = 0; i < lines; i++) {
	    int q;
	    q = strcolmove ((unsigned char *) text, from, width);
	    if (!text[q])
		break;
	    from = q + 1;
	}
	return from;
    }
    return 0;
}

/* returns pos of begin of line moved to */
/* move forward from i, `lines' can be negative --- moveing backward */
long strmovelines (const char *str, long from, long lines, int width)
{E_
    int p, q;
    if (lines > 0)
	return countlinesforward (str, from, 0, lines, width);
    if (lines == 0)
	return from;
    else {
	int line = 0;
	p = from;
	for (; p > 0;) {
	    q = p;
	    p = strfrombeginline (str, q - 1, 0);
	    line += countlinesforward (str, p, q - p, 0, width);
	    if (line > -lines)
		return countlinesforward (str, p, 0, line + lines, width);
	    if (line == -lines)
		return p;
	}
	return 0;
    }
}



/*returns a positive or negative count of lines */
long strcountlines (const char *str, long i, long amount, int width)
{E_
    int lines, p;
    if (amount > 0) {
	return countlinesforward (str, i, amount, 0, width);
    }
    if (amount == 0)
	return 0;
    if (i + amount < 0)
	amount = -i;
    p = strfrombeginline (str, i + amount, 0);
    lines = countlinesforward (str, p, i + amount - p, 0, width);
    return -countlinesforward (str, p, i - p, 0, width) + lines;
}

/*
   returns a null terminated string. The string
   is a copy of the line beginning at p and ending at '\n' 
   in the string src.
   The result must not be free'd. This routine caches the last
   four results.
 */
char *strline (const char *src, int p)
{E_
    static char line[4][1024];
    static int last = 0;
    int i = 0;
    char *r;
    while (src[p] != '\n' && src[p] && i < 1000) {
	i++;
	p++;
    }
    r = line[last & 3];
    memcpy (r, src + p - i, i);
    r[i] = 0;
    last++;
    return r;
}

size_t strnlen (const char *s, size_t count)
{E_
    const char *sc;

    for (sc = s; count-- && *sc != '\0'; ++sc)
	/* nothing */ ;
    return sc - s;
}

static void fmt_tool (char **cpy, int *len, const char *fmt, va_list ap)
{E_
    static char *tmp = NULL;
    static int tmp_alloc = 0;
    int l;
    if (!tmp) {
	tmp_alloc = 8192;
	tmp = (char *) malloc (tmp_alloc);
    }
    vsnprintf (tmp, tmp_alloc, fmt, ap);
    tmp[tmp_alloc - 1] = '\0';
    l = strlen (tmp);
    if (cpy) {
	*cpy = (char *) malloc (l + 1);
	memcpy (*cpy, tmp, l + 1);
    }
    if (len)
	*len = l;
}

size_t vfmtlen (const char *fmt, va_list ap)
{E_
    int l;
    fmt_tool (NULL, &l, fmt, ap);
    return (size_t) l;
}

char *vsprintf_alloc (const char *fmt, va_list ap)
{E_
    char *s;
    fmt_tool (&s, NULL, fmt, ap);
    return s;
}

char *sprintf_alloc (const char *fmt,...)
{E_
    char *s;
    va_list ap;
    va_start (ap, fmt);
    s = vsprintf_alloc (fmt, ap);
    va_end (ap);
    return s;
}

int readall (int fd, char *buf, int len)
{E_
    int count;
    int total = 0;
    if (len <= 0)
	return 0;
    for (;;) {
	count = read (fd, buf, len);
	if (count == -1) {
	    if (errno == EINTR || errno == EAGAIN)
		continue;
	    return -1;
	}
	if (!count)
	    return -1;
	buf += count;
	len -= count;
	total += count;
	if (len <= 0)
	    break;
    }
    return total;
}

int writeall (int fd, char *buf, int len)
{E_
    int count;
    int total = 0;
    if (len <= 0)
	return 0;
    for (;;) {
	count = write (fd, buf, len);
	if (count == -1) {
printf("count=%d fd=%d len=%d\n", count, fd, len);
	    if (errno == EINTR || errno == EAGAIN)
		continue;
	    return -1;
	}
	if (!count)
	    return -1;
	buf += count;
	len -= count;
	total += count;
	if (len <= 0)
	    break;
    }
    return total;
}


void free_shell_cmdline (char **r)
{E_
    int i;
    for (i = 0; r[i]; i++)
        free (r[i]);
    free (r);
}

static char translate_escape (char c)
{E_
    char *p;
    static const char *out = "\a\b\t\n\v\f\r\\\"\000";
    static const char *in = "abtnvfr\\\"0";
    p = strchr (in, c);
    if (!p)
	return c;
    return out[(int) (p - in)];
}

/* returns NULL terminated list of char *.
  free each element */
char **interpret_shell_cmdline (const char *s)
{E_
    char **r;
    char *t;
    int n = 0;
    int l = 0;
    t = malloc (strlen (s) + 1);
    r = malloc ((strlen (s) + 1) * sizeof (char *));

#define GETC    ((unsigned char) *s++)

#define APPEND(x)  \
    do { \
        t[l++] = x; \
    } while (0)

#define NEXT \
    do { \
        APPEND ('\0'); \
        r[n++] = Cstrdup (t); \
        l = 0; \
    } while (0)

    for (;;) {
	unsigned char c;
	c = GETC;
	if (!c) {
	    break;
	} else if ((unsigned char) c <= ' ') {
	    continue;
	} else if (c == '\"') {
	    for (;;) {
		c = GETC;
		if (!c) {
		    NEXT;
		    goto out;
		} else if (c == '\"') {
		    NEXT;
		    break;
		} else if (c == '\\') {
		    c = GETC;
		    if (!c) {
			APPEND ('\\');
			NEXT;
			goto out;
		    }
		    c = translate_escape (c);
		    APPEND (c);
		} else {
		    APPEND (c);
		}
	    }
	} else if (c == '\'') {
	    for (;;) {
		c = GETC;
		if (!c) {
		    NEXT;
		    goto out;
		} else if (c == '\'') {
		    NEXT;
		    break;
		} else  {
		    APPEND (c);
		}
	    }
	} else {
	    for (;;) {
                if (!c) {
		    NEXT;
		    goto out;
		} else if ((unsigned char) c <= ' ') {
		    NEXT;
		    break;
		} else if (c == '\\') {
		    c = GETC;
		    if (!c) {
			APPEND ('\\');
			NEXT;
			goto out;
		    }
		    c = translate_escape (c);
		    APPEND (c);
		} else {
		    APPEND (c);
		}
		c = GETC;
	    }
	}
    }

  out:
    r[n++] = 0;
    return r;
}

#if 0
void check(const char *in)
{E_
    int i;
    char **r;
    r = interpret_shell_cmdline (in);
    printf ("\n[%s] =>\n", in);
    for (i = 0; r[i]; i++) {
        printf ("[%s] ", r[i]);
    }
    printf ("\n");
}

int main()
{E_
    check("       ");
    check("   a    ");
    check("   a  b  ");
    check("a  b  ");
    check("   a  b");
    check("a");


    check("   \"\"    ");
    check("   \"a\"    ");
    check("   \"a\"  \"b\" ");
    check("\"a\"  \"b\"  ");
    check("   \"a\"  \"b\"");
    check("\"a\"");

    check("   ''    ");
    check("   'a'    ");
    check("   'a'  'b' ");
    check("'a'  'b'  ");
    check("   'a'  'b'");
    check("'a'");


    check(" \"a\\\"\"");
    check(" \"\\\\\\\"\"");


    check(" \"\\g\"");
    check(" \\   ");
    check(" \\ ");
    check(" \\");

    check("'\\'");
    check("\"\\\"");

    check("-DLOCALEDIR=\\\"/opt/cooledit/share/locale\\\" -DLIBDIR=\\\"/opt/cooledit/share/cooledit\\\" -DHAVE_CONFIG_H  -I. -I. -I.. -I../widget -I.. -I../intl   -I/usr/include/freetype2");
}
#endif


