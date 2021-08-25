/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */



int password_load (void);
int password_save (const char *host, int crypto_enabled_, const char *pass);
int password_find (const char *host, int *crypto_enabled_, char *pass, int pass_len);
void password_init (void);

