#include <stdlib.h>
#include <stdio.h>

int main (int argc, const char *const *argv)
{
    void *o;

/* rxvtlib main struct is actually only about 10k */
    o = (void *) malloc (65536);
    rxvtlib_init (o);
    rxvtlib_main (o, argc, argv, 0);
    return 0;
}

