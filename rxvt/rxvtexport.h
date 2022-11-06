/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#ifndef _RXVT_EXPORT_H
#define _RXVT_EXPORT_H

#ifndef rxvtlib_DEFINED
#define rxvtlib_DEFINED
typedef struct _rxvtlib rxvtlib;
#endif

void rxvtlib_shut (rxvtlib * o);

#define RXVT_OPTIONS_TERM8BIT           (1<<0)
#define RXVT_OPTIONS_BACKSPACE_CTRLH    (1<<1)
#define RXVT_OPTIONS_BACKSPACE_127      (1<<2)

void rxvtlib_init (rxvtlib *o, unsigned long rxvt_options);
void rxvt_process_x_event (rxvtlib * o);
void rxvtlib_update_screen (rxvtlib * o);
void rxvtlib_destroy_windows (rxvtlib * o);

#endif

