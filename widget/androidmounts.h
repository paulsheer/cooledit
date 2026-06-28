/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#ifndef _ANDROIDMOUNTS_H
#define _ANDROIDMOUNTS_H

#if defined(ANDROID) || defined(ANDROID_TEST)

struct portable_stat;

void strreplaceall (char *r, const char *s, char c1, char c2);
void load_mount (int refresh);
const char *mount_resolve (const char *orig);
int mount_stat (const char *path, struct portable_stat *st, char *link_target, int link_target_sz);
typedef void (*mount_cb)(const char *real, const char *encoded, void *userdata);
void mount_lambda (mount_cb cb, void *userdata);
void mount_cleanup (void);

#else

#define mount_resolve(buf, path, sz) (path)

#endif /* ANDROID */

#endif /* _ANDROIDMOUNTS_H */
