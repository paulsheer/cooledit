/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */


#define PASSWORD_FILE           "/.cedit/.password"

void password_load (void);
int password_load_ (void);
int password_save (const char *host, int crypto_enabled_, const char *pass);
int password_find (const char *host, int *crypto_enabled_, char *pass, int pass_len);
int password_forget (const char *host);
void password_init (void);

