#include <stdlib.h>
#include <string.h>

#define size_t  long

#define FUDGE   32

void *dbg_malloc (size_t n)
{
    size_t *p, l;
    l = n + sizeof(size_t) + FUDGE;
    p = (size_t *) malloc(l);
    memset((void *) p, '\0', l);
    *p = l;
    return p + 1;
}

void *dbg_realloc (void *v, size_t n)
{
    size_t *p, l_old, l_new;
    p = (size_t *) v;
    p--;
    l_old = *p;
    l_new = n + sizeof(size_t) + FUDGE;
    p = realloc(p, l_new);
    if (l_new > l_old)
        memset ((char *) p + l_old, '\0', l_new - l_old);
    return p + 1;
}

void dbg_free (void *v)
{
    size_t *p;
    p = (size_t *) v;
    p--;
    free (p);
}
