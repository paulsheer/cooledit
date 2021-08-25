/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */


/* main widget: */
extern CWidget *man;

/* default window sizes */
#define START_WIDTH	110
#define START_HEIGHT	30

int open_man (char *def);

CWidget *CManpageDialog (Window in, int x, int y, int width, int height, const char *manpage);


