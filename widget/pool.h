/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* pool.h
   Copyright (C) 1996-2022 Paul Sheer
 */


/*
   The memory file will begin at a few bytes and double in size whenener
   you try to write past the end. Thus you can use it to hold
   contiguous data whose size is not none a priori. Use instead of
   malloc().
 */

#ifndef _POOL_H
#define _POOL_H

typedef struct pool_type {
    unsigned char *start;
    unsigned char *current;
    unsigned char *end;
    unsigned long length;
} POOL;

#define START_SIZE 256

#define pool_freespace(p) ((unsigned long) p->end - (unsigned long) p->current)
#define pool_start(p) ((p)->start)
#define pool_current(p) ((p)->current)
#define pool_length(p) ((unsigned long) (p)->current - (unsigned long) (p)->start)

/* returns NULL on error */
POOL *pool_init (void);

/* free's a pool except for the actual data which is returned */
/* result must be free'd by the caller even if the pool_length is zero */
unsigned char *pool_break (POOL *p);

/* free's a pool and all its data */
void pool_free (POOL *p);

/* returns the number of bytes written into p */
unsigned long pool_write (POOL * p, const unsigned char *d, unsigned long l);

/* returns the number of bytes read into d */
unsigned long pool_read (POOL * p, unsigned char *d, unsigned long l);

/* sets the position in the pool */
unsigned long pool_seek (POOL * p, unsigned long l);

/* used like sprintf */
unsigned long pool_printf (POOL *p, const char *fmt,...);

/* make space for a forthcoming write of l bytes. leaves current untouched */
unsigned long pool_advance (POOL * p, unsigned long l);

/* removes the last line from the length, and null-terminates */
void pool_drop_last_line (POOL * p);

/* zero the char after the last char written/read without advancing current */
int pool_null (POOL *p);

#endif
