/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#include "inspect.h"
#include <stdlib.h>

#if defined(NO_INSPECT) || !defined(__GNUC__) || !defined(__x86_64__)

struct inspect_st__;

void init_inspect (void)
{
}

void inspect_clean_exit (void)
{
}

void housekeeping_inspect (void)
{
}

struct inspect_st__ *inspect_data__ = NULL;

#else

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


struct inspect_st__ *inspect_data__ = NULL;
static char mmap_fname[1024] = "";

void inspect_clean_exit (void)
{
    if (mmap_fname[0])
        unlink (mmap_fname);
}

void housekeeping_inspect (void)
{
    time_t now;
    if (!inspect_data__)
        return;
    time (&now);
    snprintf (inspect_data__->timestamp, sizeof (inspect_data__->timestamp), "T%013lu\n", (unsigned long) now);
}

#define INSPECT_DIR     "/inspect"
#define CEDIT_DIR       "/.cedit"

void init_inspect (void)
{
    char *dir;
    const char *home;
    int fd;

    home = getenv ("HOME");
    if (!home)
        return;

    dir = alloca (strlen (home) + sizeof (CEDIT_DIR) + sizeof (INSPECT_DIR) + 1);

    strcpy (dir, home);
    strcat (dir, CEDIT_DIR);
    mkdir (dir, 0700);
    strcat (dir, INSPECT_DIR);
    mkdir (dir, 0700);
    snprintf (mmap_fname, sizeof (mmap_fname), "%s/inspect%ld", dir, (long) getpid ());

    if ((fd = open (mmap_fname, O_RDWR | O_CREAT, 0600)) >= 0) {
        int c;
        c = ftruncate (fd, sizeof (struct inspect_st__));
        if (c) {
            perror (mmap_fname);
        } else {
            inspect_data__ = mmap (NULL, sizeof (struct inspect_st__), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (!inspect_data__)
                perror (mmap_fname);
        }
        close (fd);
    } else {
        perror (mmap_fname);
    }

    if (!inspect_data__)
        inspect_data__ = (struct inspect_st__ *) malloc (sizeof (struct inspect_st__));

    if (!inspect_data__)
        exit (1);

    memset (inspect_data__, '\0', sizeof (struct inspect_st__));
    inspect_data__->dummy = ('#' << 24) | '#';
}

#endif


