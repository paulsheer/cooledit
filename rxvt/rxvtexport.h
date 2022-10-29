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
void rxvt_get_tty_name (rxvtlib * rxvt, char *p);
void rxvt_get_pid_host (rxvtlib * rxvt, pid_t *pid, char *host, int host_len);
int rxvt_have_pid (pid_t pid);

rxvtlib *rxvt_start (const char *host, Window win, char **argv, int do_sleep, int charset_8bit);
void rxvtlib_shutall (void);

#endif

