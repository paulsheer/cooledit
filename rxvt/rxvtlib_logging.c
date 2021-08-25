/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"

/*--------------------------------*-C-*---------------------------------*
 * File:	logging.c
 *----------------------------------------------------------------------*
 * $Id: logging.c,v 1.7.2.1 1999/07/10 04:20:01 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1992      John Bovey <jdb@ukc.ac.uk>
 *				- original version
 * Copyright (C) 1993      lipka
 * Copyright (C) 1993      Brian Stempien <stempien@cs.wmich.edu>
 * Copyright (C) 1995      Raul Garcia Garcia <rgg@tid.es>
 * Copyright (C) 1995      Piet W. Plomp <piet@idefix.icce.rug.nl>
 * Copyright (C) 1997      Raul Garcia Garcia <rgg@tid.es>
 * Copyright (C) 1997,1998 Geoff Wing <gcw@pobox.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*
 * Public:
 *	extern void cleanutent (void);
 *	extern void makeutent (const char * pty, const char * hostname);
 *
 * Private:
 *	write_utmp ();
 *	update_wtmp ();
 *----------------------------------------------------------------------*/

/*
 * HAVE_SETUTENT corresponds to SYSV-style utmp support.
 * Without it corresponds to using BSD utmp support.
 * SYSV-style utmp support is further divided in normal utmp support
 * and utmpx support (Solaris 2.x) by RXVT_UTMP_AS_UTMPX
 */

/*
 * update wtmp entries - only for SYSV style UTMP systems
 */
#ifdef UTMP_SUPPORT
	/* remember if entry to utmp made */

# ifndef USE_SYSV_UTMP
	/* BSD position of utmp-stamp */

# endif
#endif

/* ------------------------------------------------------------------------- */
#ifndef RXVT_UTMP_AS_UTMPX	/* supposedly we have updwtmpx ? */
#ifdef WTMP_SUPPORT
/* INTPROTO */
void            rxvt_update_wtmp (const char *fname, const struct utmp *putmp)
{E_
    int             fd, retry = 10;	/* 10 attempts at locking */
    struct flock    lck;	/* fcntl locking scheme */

    if ((fd = open (fname, O_WRONLY | O_APPEND, 0)) < 0)
	return;

    lck.l_whence = SEEK_END;	/* start lock at current eof */
    lck.l_len = 0;		/* end at ``largest possible eof'' */
    lck.l_start = 0;
    lck.l_type = F_WRLCK;	/* we want a write lock */

    while (retry--)
	/* attempt lock with F_SETLK - F_SETLKW would cause a deadlock! */
	if ((fcntl (fd, F_SETLK, &lck) < 0) && errno != EACCESS) {
	    close (fd);
	    return;		/* failed for unknown reason: give up */
	}
    write (fd, putmp, sizeof (struct utmp));

/* unlocking the file */
    lck.l_type = F_UNLCK;
    fcntl (fd, F_SETLK, &lck);

    close (fd);
}

#endif				/* WTMP_SUPPORT */
#endif				/* !HAVE_UTMPX_H */
/* ------------------------------------------------------------------------- */
#ifdef UTMP_SUPPORT
/*
 * make a utmp entry
 */
/* EXTPROTO */
void            rxvtlib_makeutent (rxvtlib *o, const char *pty, const char *hostname)
{E_
    struct passwd  *pwent = getpwuid (getuid ());
    UTMP            ut;

#ifndef USE_SYSV_UTMP
/*
 * BSD style utmp entry
 *      ut_line, ut_name, ut_host, ut_time
 */
    int             i;
    FILE           *fd0, *fd1;
    char            buf[256], name[256];

#else
/*
 * SYSV style utmp entry
 *      ut_user, ut_id, ut_line, ut_pid, ut_type, ut_exit, ut_time
 */
    char           *colon;

#endif				/* !USE_SYSV_UTMP */

/* BSD naming is of the form /dev/tty?? or /dev/pty?? */

    MEMSET (&ut, 0, sizeof (UTMP));
    if (!strncmp (pty, "/dev/", 5))
	pty += 5;		/* skip /dev/ prefix */
    if (!strncmp (pty, "pty", 3) || !strncmp (pty, "tty", 3))
	STRNCPY (o->ut_id, (pty + 3), sizeof (o->ut_id));
    else
#ifndef USE_SYSV_UTMP
    {
	print_error ("can't parse tty name \"%s\"", pty);
	o->ut_id[0] = '\0';	/* entry not made */
	return;
    }

    STRNCPY (ut.ut_line, pty, sizeof (ut.ut_line));
    STRNCPY (ut.ut_name, (pwent && pwent->pw_name) ? pwent->pw_name : "?",
	     sizeof (ut.ut_name));
    STRNCPY (ut.ut_host, hostname, sizeof (ut.ut_host));
    ut.ut_time = time (NULL);

    if ((fd0 = fopen (RXVT_REAL_UTMP_FILE, "r+")) == NULL)
	o->ut_id[0] = '\0';	/* entry not made */
    else {
	o->utmp_pos = -1;
	if ((fd1 = fopen (TTYTAB_FILENAME, "r")) != NULL) {
	    for (i = 1; (fgets (buf, sizeof (buf), fd1) != NULL);) {
		if (*buf == '#' || sscanf (buf, "%s", name) != 1)
		    continue;
		if (!strcmp (ut.ut_line, name)) {
		    fclose (fd1);
		    o->utmp_pos = i * sizeof (struct utmp);

		    break;
		}
		i++;
	    }
	    fclose (fd1);
	}
	if (o->utmp_pos < 0)
	    o->ut_id[0] = '\0';	/* entry not made */
	else {
	    fseek (fd0, o->utmp_pos, 0);
	    fwrite (&ut, sizeof (UTMP), 1, fd0);
	}
	fclose (fd0);
    }

#else				/* USE_SYSV_UTMP */
    {
	int             n;

	if (sscanf (pty, "pts/%d", &n) == 1)
	    sprintf (o->ut_id, "vt%02x", (n % 256));	/* sysv naming */
	else {
	    print_error ("can't parse tty name \"%s\"", pty);
	    o->ut_id[0] = '\0';	/* entry not made */
	    return;
	}
    }

#if 0
    /* XXX: most likely unnecessary.  could be harmful */
    utmpname (RXVT_REAL_UTMP_FILE);
#endif

    setutent ();		/* XXX: should be unnecessaray */

    STRNCPY (ut.o->ut_id, o->ut_id, sizeof (ut.o->ut_id));
    ut.ut_type = DEAD_PROCESS;
    (void)getutid (&ut);	/* position to entry in utmp file */

/* set up the new entry */
    ut.ut_type = USER_PROCESS;
#ifndef linux
    ut.ut_exit.e_exit = 2;
#endif
    STRNCPY (ut.ut_user, (pwent && pwent->pw_name) ? pwent->pw_name : "?",
	     sizeof (ut.ut_user));
    STRNCPY (ut.o->ut_id, o->ut_id, sizeof (ut.o->ut_id));
    STRNCPY (ut.ut_line, pty, sizeof (ut.ut_line));

#if (defined(HAVE_UTMP_HOST) && ! defined(RXVT_UTMP_AS_UTMPX)) || (defined(HAVE_UTMPX_HOST) && defined(RXVT_UTMP_AS_UTMPX))
    STRNCPY (ut.ut_host, hostname, sizeof (ut.ut_host));
# ifndef linux
    if ((colon = strrchr (ut.ut_host, ':')) != NULL)
	*colon = '\0';
# endif
#endif

/* ut_name is normally the same as ut_user, but .... */
    STRNCPY (ut.ut_name, (pwent && pwent->pw_name) ? pwent->pw_name : "?",
	     sizeof (ut.ut_name));

    ut.ut_pid = getpid ();

#ifdef RXVT_UTMP_AS_UTMPX
    ut.ut_session = getsid (0);
    ut.ut_tv.tv_sec = time (NULL);
    ut.ut_tv.tv_usec = 0;
#else
    ut.ut_time = time (NULL);
#endif				/* HAVE_UTMPX_H */

    pututline (&ut);

#ifdef WTMP_SUPPORT
    update_wtmp (RXVT_WTMP_FILE, &ut);
#endif

    endutent ();		/* close the file */
#endif				/* !USE_SYSV_UTMP */
}
#endif				/* UTMP_SUPPORT */

/* ------------------------------------------------------------------------- */
#ifdef UTMP_SUPPORT
/*
 * remove a utmp entry
 */
/* EXTPROTO */
void            rxvtlib_cleanutent (rxvtlib *o)
{E_
    UTMP            ut;

#ifndef USE_SYSV_UTMP
    FILE           *fd;

    if (o->ut_id[0] && ((fd = fopen (RXVT_REAL_UTMP_FILE, "r+")) != NULL)) {
	MEMSET (&ut, 0, sizeof (struct utmp));

	fseek (fd, o->utmp_pos, 0);
	fwrite (&ut, sizeof (struct utmp), 1, fd);

	fclose (fd);
    }
#else				/* USE_SYSV_UTMP */
    UTMP           *putmp;

    if (!o->ut_id[0])
	return;			/* entry not made */

#if 0
    /* XXX: most likely unnecessary.  could be harmful */
    utmpname (RXVT_REAL_UTMP_FILE);
#endif
    MEMSET (&ut, 0, sizeof (UTMP));
    STRNCPY (ut.o->ut_id, o->ut_id, sizeof (ut.o->ut_id));
    ut.ut_type = USER_PROCESS;

    setutent ();		/* XXX: should be unnecessaray */

    putmp = getutid (&ut);
    if (!putmp || putmp->ut_pid != getpid ())
	return;

    putmp->ut_type = DEAD_PROCESS;

#ifdef RXVT_UTMP_AS_UTMPX
    putmp->ut_session = getsid (0);
    putmp->ut_tv.tv_sec = time (NULL);
    putmp->ut_tv.tv_usec = 0;
#else				/* HAVE_UTMPX_H */
    putmp->ut_time = time (NULL);
#endif				/* HAVE_UTMPX_H */
    pututline (putmp);

#ifdef WTMP_SUPPORT
    update_wtmp (RXVT_WTMP_FILE, putmp);
#endif

    endutent ();
#endif				/* !USE_SYSV_UTMP */
}
#endif
