/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#ifndef __GTK_EDIT_INTERNAL_H__
#define __GTK_EDIT_INTERNAL_H__

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

#ifdef __cplusplus
extern "C" {
#endif				/* __cplusplus */

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <my_string.h>		/* FIXME: gtk won't have my string */
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include "regex.h"		/* FIXME: will gtk have regexp? */
#include <signal.h>
#include "lkeysym.h"		/* FIXME: gtk won't have lkeysym.h */
#include "stringtools.h"	/* FIXME: gtk won't have my stringtools.h */

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif				/* __GTK_EDIT_INTERNAL_H__ */

