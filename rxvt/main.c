/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include <stdlib.h>
#include <stdio.h>

int main (int argc, const char *const *argv)
{E_
    void *o;

/* rxvtlib main struct is actually only about 10k */
    o = (void *) malloc (65536);
    rxvtlib_init (o, 0);
    rxvtlib_main (o, argc, argv, 0);
    return 0;
}

