/* pool.c - create a file in memeory to write to
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "stringtools.h"
#include "pool.h"

#define min(x,y) ((x)<=(y)?(x):(y))

/* returns NULL on error */
POOL *pool_init (void)
{
    POOL *p;
    p = malloc (sizeof (POOL));
    if (!p)
	return 0;
    p->current = p->start = malloc (START_SIZE);
    if (!p->start) {
        free (p);
	return 0;
    }
    p->end = p->start + START_SIZE;
    p->length = START_SIZE;
    return p;
}

/* free's a pool except for the actual data which is returned */
/* result must be free'd by the caller even if the pool_length is zero */
unsigned char *pool_break (POOL * p)
{
    unsigned char *d;
    d = p->start;
    free (p);
    return d;
}

/* free's a pool and all its data */
void pool_free (POOL * p)
{
    if (!p)
	return;
    if (p->start)
	free (p->start);
    free (p);
}

/* make space for a forthcoming write of l bytes. leaves current untouched */
unsigned long pool_advance (POOL * p, unsigned long l)
{
    if ((unsigned long) p->current + l > (unsigned long) p->end) {
	unsigned char *t;
	unsigned long old_length;
	old_length = p->length;
	do {
	    p->length *= 2;
	    p->end = p->start + p->length;
	} while ((unsigned long) p->current + l > (unsigned long) p->end);
	t = malloc (p->length);
	if (!t)
	    return 0;
	memcpy (t, p->start, old_length);
	p->current = t + (unsigned long) p->current - (unsigned long) p->start;
	free (p->start);
	p->start = t;
	p->end = p->start + p->length;
    }
    return l;
}

/* returns the number of bytes written into p */
unsigned long pool_write (POOL * p, unsigned char *d, unsigned long l)
{
    unsigned long a;
    a = pool_advance (p, l);
    memcpy (p->current, d, a);
    p->current += a;
    return a;
}

/* returns the number of bytes read into d */
unsigned long pool_read (POOL * p, unsigned char *d, unsigned long l)
{
    unsigned long m;
    m = min (l, (unsigned long) p->end - (unsigned long) p->current);
    memcpy (d, p->current, m);
    p->current += m;
    return m;
}

/* sets the position in the pool */
unsigned long pool_seek (POOL * p, unsigned long l)
{
    unsigned long m;
    m = min (l, p->length);
    p->current = p->start + m;
    return m;
}

/* used like sprintf */
unsigned long pool_printf (POOL * p, const char *fmt,...)
{
    unsigned long l;
    va_list ap;
    va_start (ap, fmt);
    l = vfmtlen (fmt, ap) + 1;
    va_end (ap);
    if (pool_advance (p, l + 1) != l + 1)
	return 0;
    va_start (ap, fmt);
    vsprintf ((char *) p->current, fmt, ap);
    va_end (ap);
    l = strlen ((char *) p->current);
    p->current += l;
    return l;
}

/* zero the char after the last char written/read */
int pool_null (POOL * p)
{
    if (pool_advance (p, 1) != 1)
	return 0;
    p->current[0] = 0;
    return 1;
}

/* removes the last line from the length, and null-terminates */
void pool_drop_last_line (POOL * p)
{
    char *q;
    q = (char *) strrchr ((char *) pool_start (p), '\n');
    if (!q)
        pool_seek (p, 0);
    else
        pool_seek (p, (int) (q - (char *) pool_start (p)) + 1);
    pool_null (p);
}

