/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include <stdio.h>
#ifdef HAVE_STDLIB_H
# ifdef HAVE_GETPT
#  define _GNU_SOURCE
# endif
# include <stdlib.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#include <sys/stat.h>
#include <errno.h>
main ()
{
    int pty, checkerror;
    FILE *a, *f = fopen ("conftestval", "w");
    struct stat statbuf;
    extern int errno;
    if (!f)
	exit (1);
/* presume that S_IXOTH is required for people to access devices */
    if (stat ("/dev", &statbuf) < 0)
	checkerror = 0;
    else
	checkerror = ((statbuf.st_mode & S_IXOTH) == S_IXOTH) ? 1 : 0;
#if defined(__sgi) || defined(sgi) || defined(__sgi__)
    if (stat ("/dev/ptc", &statbuf) >= 0)
# ifdef HAVE__GETPTY
	fprintf (f, "SGI4");
# else
	fprintf (f, "SGI3");
# endif
    else
	fprintf (f, "SGI4");
    exit (0);
#endif
#ifdef _SCO_DS
    if (stat ("/dev/ttyp20", &statbuf) == 0) {
	fprintf (f, "SCO");
	exit (0);
    }
#endif
/* HPUX: before ptmx */
    pty = open ("/dev/ptym/clone", O_RDWR);
    if (pty >= 0 || (checkerror && errno == EACCES)) {
	fprintf (f, "HPUX");
	exit (0);
    }
#if defined(HAVE_GRANTPT) && defined(HAVE_UNLOCKPT)
# ifdef HAVE_GETPT
    pty = getpt ();
    if (pty >= 0 || errno == EACCES) {
	fprintf (f, "GLIBC");
	exit (0);
    }
# endif
#ifndef S_IFCHR
#define S_IFCHR  0020000
#endif
    if (stat ("/dev/ptmx", &statbuf) >= 0 && ((statbuf.st_mode & S_IFCHR) == S_IFCHR))
	if (stat ("/dev/pts/0", &statbuf) >= 0 && ((statbuf.st_mode & S_IFCHR) == S_IFCHR)
	    && ((pty = open ("/dev/ptmx", O_RDWR)) >= 0 || (checkerror && errno == EACCES))) {
	    fprintf (f, "USG");
	    exit (0);
	}
#endif
    if (stat ("/dev/ttyp20", &statbuf) == 0) {
	fprintf (f, "SCO");
	exit (0);
    }
    fprintf (f, "BSD");
    exit (0);
}
