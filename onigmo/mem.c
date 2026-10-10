#include <stdlib.h>
#include <string.h>

#define size_t  long

#define FUDGE   64

/* Solaris SPARC with -m32 needs 64-bit alignment */
#define ALIGNMENT_HACK  2

/* Paul Sheer. This Onigma library has valgrind errors reading/writing past the end of the block */
void *dbg_malloc (size_t n)
{
    size_t *p, l;
    l = n + (sizeof(size_t) * ALIGNMENT_HACK) + FUDGE;    /* */
    p = (size_t *) malloc(l);
    memset((void *) p, '\0', l);
    *p = l;
    return p + ALIGNMENT_HACK;
}

void *dbg_realloc (void *v, size_t n)
{
    size_t *p, l_old, l_new;
    if (!v)
        return dbg_malloc (n);
    p = (size_t *) v;
    p -= ALIGNMENT_HACK;
    l_old = *p;
    l_new = n + (sizeof(size_t) * ALIGNMENT_HACK) + FUDGE;
    p = realloc(p, l_new);
    if (l_new > l_old)
        memset ((char *) p + l_old, '\0', l_new - l_old);
    *p = l_new;
    return p + ALIGNMENT_HACK;
}

void dbg_free (void *v)
{
    size_t *p;
    if (!v)
        return;
    p = (size_t *) v;
    p -= ALIGNMENT_HACK;
    free (p);
}
