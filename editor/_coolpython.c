/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#include "inspect.h"
#include <config.h>

int dummy_avoid_warn_coolpython_c;

#ifdef HAVE_PYTHON
#include "coolpython.c"
#endif

