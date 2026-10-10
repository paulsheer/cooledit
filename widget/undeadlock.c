/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* undeadlock.c - undead terminal lock-file management
   Copyright (C) 1996-2022 Paul Sheer
 */

#include "inspect.h"
#include <config.h>

#include <stdio.h>
#include <my_string.h>
#include <string.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef MSWIN
#include "mswinchild.h"
#else
#include "cterminal.h"
#endif

#include "undeadlock.h"

#ifndef MSWIN

struct undead_lock {
    struct undead_lock *prev;
    struct undead_lock *next;
    char *undead_path;
    int undead_lock_fd;
};

static struct undead_lock *undead_lock_list = NULL;

int undead_lock_fcntl (int fd, short l_type, int cmd)
{
    struct flock fl;
    memset (&fl, 0, sizeof (fl));
    fl.l_type = l_type;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    return fcntl (fd, cmd, &fl);
}

struct undead_lock *undead_lock_alloc (const char *undead_path, struct cterminal_config *config)
{
    struct undead_lock *udl;
    FILE *f;
    int fd;

    udl = (struct undead_lock *) malloc (sizeof (struct undead_lock));
    memset (udl, '\0', sizeof (*udl));
    udl->undead_path = (char *) malloc (strlen (undead_path) + 1);
    strcpy (udl->undead_path, undead_path);
    f = fopen (udl->undead_path, "w");
    if (f) {
        fprintf (f, "%lu-%llu\n", config->host_pid, config->start_time);
        fclose (f);
    }
    udl->undead_lock_fd = -1;
    fd = open (udl->undead_path, O_RDWR);
    if (fd >= 0) {
        if (undead_lock_fcntl (fd, F_WRLCK, F_SETLKW) == 0)
            udl->undead_lock_fd = fd;
        else
            close (fd);
    }

    udl->next = undead_lock_list;
    udl->prev = NULL;
    if (undead_lock_list)
        undead_lock_list->prev = udl;
    undead_lock_list = udl;

    return udl;
}

struct undead_lock *undead_lock_find (const char *undead_path)
{
    struct undead_lock *i;
    for (i = undead_lock_list; i; i = i->next)
        if (!strcmp (undead_path, i->undead_path))
            return i;
    return NULL;
}

void undead_lock_free (struct undead_lock *udl)
{
    if (udl->prev)
        udl->prev->next = udl->next;
    else
        undead_lock_list = udl->next;
    if (udl->next)
        udl->next->prev = udl->prev;

    if (udl->undead_lock_fd >= 0) {
        undead_lock_fcntl (udl->undead_lock_fd, F_UNLCK, F_SETLK);
        close (udl->undead_lock_fd);
    }
    unlink (udl->undead_path);

    free (udl->undead_path);
    free (udl);
}

#endif
