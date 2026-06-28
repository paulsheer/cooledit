/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#include "androidtest.h"

#if defined(ANDROID) || defined(ANDROID_TEST)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "androidmounts.h"
#include "dirtools.h"
#include "my_string.h"

struct mount_map {
    struct mount_map *next;
    char *encoded;
    char *real;
};

struct mount_extras {
    char *encoded;
    char *real;
};

static struct mount_map *mounts;

void strreplaceall (char *r, const char *s, char c1, char c2)
{
    while (*s) {
        *r = *s == c1 ? c2 : *s;
        r++, s++;
    }
    *r = '\0';
}

void load_mount (int refresh)
{
    FILE *f;
    char line[1024];
    struct mount_map *tail;

    if (refresh) {
        while (mounts) {
            struct mount_map *m = mounts;
            mounts = mounts->next;
            free (m->encoded);
            free (m->real);
            free (m);
        }
    }
    if (mounts)
        return;
    tail = NULL;
    f = fopen ("/proc/mounts", "r");
    if (!f)
        return;
    while (fgets (line, sizeof (line), f)) {
        DIR *mount_dir;
        char *p, *mount_path;
        if (!(mount_path = strchr (line, ' ')))
            continue;
        mount_path++;
        if (!(p = strchr (mount_path, ' ')))
            continue;
        *p = '\0';
        if (*mount_path != '/' || !strcmp (mount_path, "/"))
            continue;
        if (!(mount_dir = opendir (mount_path)))
            continue;
        closedir (mount_dir);
        {
            struct mount_map *m = (struct mount_map *) malloc (sizeof (*m));
            char t[MAX_PATH_LEN];
            m->real = strdup (mount_path);
            if (*mount_path == '/')
                mount_path++;
            strreplaceall (t, mount_path, '/', '@');
            m->encoded = strdup (t);
            m->next = NULL;
            if (tail)
                tail->next = m;
            else
                mounts = m;
            tail = m;
        }
    }
    fclose (f);
    {
        struct mount_extras extras[3] = {
            {"sdcard", "sdcard"},
#ifdef ANDROID_TEST
            {"usr@src", "/usr/src"},
#endif
            {NULL, NULL}
        };
        struct mount_extras *q;
        for (q = extras; q->encoded; q++) {
            struct mount_map *m = (struct mount_map *) malloc (sizeof (*m));
            m->encoded = strdup (q->encoded);
            m->real = strdup (q->real);
            m->next = NULL;
            if (tail)
                tail->next = m;
            else
                mounts = m;
            tail = m;
        }
    }
}

int mount_stat (const char *path, struct portable_stat *st, char *link_target, int link_target_sz)
{
    struct mount_map *m;
    const char *slash, *e;
    int len;

    load_mount (0);
    while (*path == '/')  /* /////mnt@sdcard */
        path++;
    slash = path;
    while (*slash && *slash != '/')
        slash++;
    if (!(len = (int)(slash - path)))
        return -1;
    e = slash;
    while (*e == '/')   /* mnt@sdcard////// */
        e++;
    if (*e)
        return -1;      /* mnt@sdcard/Downloads/IMG_4EA860F9.JPG */
    for (m = mounts; m; m = m->next) {
        if (strlen (m->encoded) == (size_t)len && !memcmp (m->encoded, path, len)) {
            if (st) {
                memset (st, 0, sizeof (*st));
                st->ustat.st_mode = S_IFDIR | 00777;
            }
            if (link_target && link_target_sz > 0) {
                strncpy (link_target, m->real, link_target_sz - 1);
                link_target[link_target_sz - 1] = '\0';
            }
            return 0;
        }
    }
    return -1;
}

const char *mount_resolve (const char *orig)
{
    static char buf[MAX_PATH_LEN + MAX_PATH_LEN];
    struct mount_map *m;
    const char *slash, *leading_slash = "";
    const char *path = orig;
    int len;

    load_mount (0);

    while (*path == '/') {
        path++;
        leading_slash = "/";
    }
    slash = strchr (path, '/');
    len = slash ? (int) (slash - path) : (int) strlen (path);
    if (len == 0)
        return orig;
    for (m = mounts; m; m = m->next) {
        if (strlen (m->encoded) == (size_t) len && !memcmp (m->encoded, path, len)) {
            if (m->real[0] == '/')
                leading_slash = "";
            if (slash)
                snprintf (buf, sizeof (buf), "%s%s%s", leading_slash, m->real, slash);
            else
                snprintf (buf, sizeof (buf), "%s%s", leading_slash, m->real);
            return buf;
        }
    }
    return orig;
}

void mount_lambda (mount_cb cb, void *userdata)
{
    struct mount_map *m;

    load_mount (0);
    for (m = mounts; m; m = m->next)
        cb (m->real, m->encoded, userdata);
}

void mount_cleanup (void)
{
    while (mounts) {
        struct mount_map *m = mounts;
        mounts = mounts->next;
        free (m->encoded);
        free (m->real);
        free (m);
    }
}

#endif /* ANDROID */
