/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#ifndef _RXVT_EXPORT_H
#define _RXVT_EXPORT_H

#ifndef rxvtlib_DEFINED
#define rxvtlib_DEFINED
typedef struct _rxvtlib rxvtlib;
#endif

void rxvtlib_shut (rxvtlib * o);
void rxvtlib_init (rxvtlib *o, int charset_8bit);
void rxvt_process_x_event (rxvtlib * o);
void rxvtlib_update_screen (rxvtlib * o);
void rxvtlib_destroy_windows (rxvtlib * o);
void rxvt_get_tty_name (rxvtlib * rxvt, char *p);
void rxvt_get_pid (rxvtlib * rxvt, pid_t *pid, const char *host);
int rxvt_have_pid (const char *host, pid_t pid);
void rxvt_kill (pid_t p);

rxvtlib *rxvt_start (const char *host, Window win, char **argv, int do_sleep, int charset_8bit);
void rxvtlib_shutall (void);

#endif

