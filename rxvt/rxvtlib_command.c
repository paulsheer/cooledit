/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"
#include <coolwidget.h>
#include <stringtools.h>

/*--------------------------------*-C-*---------------------------------*
 * File:	command.c
 *----------------------------------------------------------------------*
 * $Id: command.c,v 1.85.2.23 1999/08/12 16:32:39 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1992      John Bovey, University of Kent at Canterbury <jdb@ukc.ac.uk>
 *				- original version
 * Copyright (C) 1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 * 				- extensive modifications
 * Copyright (C) 1995      Garrett D'Amore <garrett@netcom.com>
 *				- vt100 printing
 * Copyright (C) 1995      Steven Hirsch <hirsch@emba.uvm.edu>
 *				- X11 mouse report mode and support for
 *				  DEC "private mode" save/restore functions.
 * Copyright (C) 1995      Jakub Jelinek <jj@gnu.ai.mit.edu>
 *				- key-related changes to handle Shift+function
 *				  keys properly.
 * Copyright (C) 1997      MJ Olesen <olesen@me.queensu.ca>
 *				- extensive modifications
 * Copyright (C) 1997      Raul Garcia Garcia <rgg@tid.es>
 *				- modification and cleanups for Solaris 2.x
 *				  and Linux 1.2.x
 * Copyright (C) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
 * Copyright (C) 1998      Geoff Wing <gcw@pobox.com>
 * Copyright (C) 1998      Alfredo K. Kojima <kojima@windowmaker.org>
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

/*{{{ substitute system functions */
#if defined(__svr4__) && ! defined(_POSIX_VERSION)
/* INTPROTO */
int             getdtablesize (void)
{E_
    struct rlimit   rlim;

    getrlimit (RLIMIT_NOFILE, &rlim);
    return rlim.rlim_cur;
}
#endif
/*}}} */

/*{{{ take care of suid/sgid super-user (root) privileges */
/* EXTPROTO */
void            privileges (int mode)
{E_
#if ! defined(__CYGWIN32__)
# ifdef HAVE_SETEUID
    static uid_t    euid;
    static gid_t    egid;

    switch (mode) {
    case IGNORE:
	/*
	 * change effective uid/gid - not real uid/gid - so we can switch
	 * back to root later, as required
	 */
	seteuid (getuid ());
	setegid (getgid ());
	break;
    case SAVE:
	euid = geteuid ();
	egid = getegid ();
	break;
    case RESTORE:
	seteuid (euid);
	setegid (egid);
	break;
    }
# else
    switch (mode) {
    case IGNORE:
	setuid (getuid ());
	setgid (getgid ());
	/* FALLTHROUGH */
    case SAVE:
	/* FALLTHROUGH */
    case RESTORE:
	break;
    }
# endif
#endif
}
/*}}} */

/*{{{ signal handling, exit handler */
/*
 * Catch a SIGCHLD signal and exit if the direct child has died
 */
/* ARGSUSED */
/* INTPROTO */

#ifdef STANDALONE
rxvtlib *match_object_to_pid (rxvtlib * o, int pid) {
    static struct pid_list {
	rxvtlib *o;
	int pid;
	struct pid_list *next;
    } pid_list = {
    0, 0, 0};

    if (pid <= 0)
	return 0;
    if (o) {
	struct pid_list *p;
/* new list entry */
	for (p = &pid_list; p->next; p = p->next);
	p->next = malloc (sizeof (struct pid_list));
	p->next->next = 0;
	p->next->o = o;
	p->next->pid = pid;
	return o;
    } else {
	struct pid_list *p, *q = 0;
/* find the list entry */
	for (p = &pid_list; p && p->pid != pid; p = p->next)
	    q = p;
	if (!p)
	    return 0;
/* remove it */
	q->next = p->next;
	o = p->o;
	free (p);
	return o;
    }
    return 0;
}

RETSIGTYPE Child_signal (int unused)
{E_
    int pid, save_errno = errno;
    rxvtlib *o;

    do {
	errno = 0;
    } while ((-1 == (pid = waitpid (-1, NULL, WNOHANG)))
	     && (errno == EINTR));
    if ((o = match_object_to_pid (0, pid)))
	o->killed = EXIT_SUCCESS | DO_EXIT;
    errno = save_errno;
#error
    signal (SIGCHLD, Child_signal);
}
#endif

#ifdef STANDALONE
/*
 * Catch a fatal signal and tidy up before quitting
 */
/* INTPROTO */
RETSIGTYPE      Exit_signal (int sig)
{E_

#ifdef DEBUG_CMD
    print_error ("signal %d", sig);
#endif
    signal (sig, SIG_DFL);

#ifdef UTMP_SUPPORT
    if (!(Options & Opt_utmpInhibit)) {
	privileges (RESTORE);
	cleanutent ();
	privileges (IGNORE);
    }
#endif

    kill (getpid (), sig);
}
#endif

/*
 * Exit gracefully, clearing the utmp entry and restoring tty attributes
 * TODO: this should free up any known resources if we can
 */
/* INTPROTO */
void            clean_exit (void)
{E_

/*
    scr_release ();
#if defined(UTMP_SUPPORT) || ! defined(__CYGWIN32__)
    privileges (RESTORE);
# ifndef __CYGWIN32__
    if (changettyowner) {
#ifdef DEBUG_CMD
	fprintf (stderr, "Restoring \"%s\" to mode %03o, uid %d, gid %d\n",
		 ttydev, ttyfd_stat.st_mode, ttyfd_stat.st_uid,
		 ttyfd_stat.st_gid);
#endif
	chmod (ttydev, ttyfd_stat.st_mode);
	chown (ttydev, ttyfd_stat.st_uid, ttyfd_stat.st_gid);
    }
# endif
# ifdef UTMP_SUPPORT
    cleanutent ();
# endif
    privileges (IGNORE);
#endif
*/

}

/*}}} */

/*{{{ Acquire a pseudo-teletype from the system. */
/*
 * On failure, returns -1.
 * On success, returns the file descriptor.
 *
 * If successful, ttydev and ptydev point to the names of the
 * master and slave parts
 */
/* INTPROTO */
int             rxvtlib_get_pty (rxvtlib *o)
{E_
    int             fd;
    char           *ptydev;

    if (o->ttydev) {
	free (o->ttydev);
	o->ttydev = 0;
    }

#ifdef __FreeBSD__
    fd = posix_openpt(O_RDWR);
    if (fd >= 0) {
	if (grantpt (fd) == 0	/* change slave permissions */
	    && unlockpt (fd) == 0) {	/* slave now unlocked */
	    ptydev = o->ttydev = (char *) strdup (ptsname (fd));	/* get slave's name */
	    o->changettyowner = 0;
	    goto Found;
	}
	close (fd);
    }
#endif
#ifdef PTYS_ARE__GETPTY
    if ((ptydev = o->ttydev = (char *) strdup (_getpty (&fd, O_RDWR | O_NDELAY, 0622, 0))) != NULL)
	goto Found;
#endif
#ifdef PTYS_ARE_GETPTY
    while ((ptydev = getpty ()) != NULL)
	if ((fd = open (ptydev, O_RDWR)) >= 0) {
	    o->ttydev = strdup (ptydev);
	    goto Found;
	}
#endif
#if defined(HAVE_GRANTPT) && defined(HAVE_UNLOCKPT)
# if defined(PTYS_ARE_GETPT) || defined(PTYS_ARE_PTMX)
    {
	extern char    *ptsname ();

#  ifdef PTYS_ARE_GETPT
	if ((fd = getpt ()) >= 0)
#  else
	    if ((fd = open ("/dev/ptmx", O_RDWR)) >= 0)
#  endif
	    {
		if (grantpt (fd) == 0	/* change slave permissions */
		    && unlockpt (fd) == 0) {	/* slave now unlocked */
		    ptydev = o->ttydev = (char *) strdup (ptsname (fd));	/* get slave's name */
		    o->changettyowner = 0;
		    goto Found;
		}
		close (fd);
	    }
    }
# endif
#endif
#ifdef PTYS_ARE_PTC
    if ((fd = open ("/dev/ptc", O_RDWR)) >= 0) {
	ptydev = o->ttydev = (char *) strdup (ttyname (fd));
	goto Found;
    }
#endif
#ifdef PTYS_ARE_CLONE
    if ((fd = open ("/dev/ptym/clone", O_RDWR)) >= 0) {
	ptydev = o->ttydev = (char *) strdup (ptsname (fd));
	goto Found;
    }
#endif
#ifdef PTYS_ARE_NUMERIC
    {
	int             idx;
	char           *c1, *c2;
	char            pty_name[] = "/dev/ptyp???";
	char            tty_name[] = "/dev/ttyp???";

	ptydev = pty_name;
	o->ttydev = tty_name;

	c1 = &(pty_name[sizeof (pty_name) - 4]);
	c2 = &(tty_name[sizeof (tty_name) - 4]);
	for (idx = 0; idx < 256; idx++) {
	    sprintf (c1, "%d", idx);
	    sprintf (c2, "%d", idx);
	    if (access (o->ttydev, F_OK) < 0) {
		idx = 256;
		break;
	    }
	    if ((fd = open (ptydev, O_RDWR)) >= 0) {
		if (access (o->ttydev, R_OK | W_OK) == 0) {
		    o->ttydev = (char *) strdup (tty_name);
		    goto Found;
		}
		close (fd);
	    }
	}
    }
#endif
/* if PTYS_ARE_SEARCHED or no other method defined */
#if defined(PTYS_ARE_SEARCHED) || (!defined(PTYS_ARE_CLONE) && !defined(PTYS_ARE_GETPT) && !defined(PTYS_ARE_GETPTY) && !defined(PTYS_ARE_NUMERIC) && !defined(PTYS_ARE_PTC) && !defined(PTYS_ARE_PTMX) && !defined(PTYS_ARE_SEARCHED) && !defined(PTYS_ARE__GETPTY))
    {
	int             len;
	const char     *c1, *c2;
	char            pty_name[] = "/dev/pty??";
	char            tty_name[] = "/dev/tty??";

	len = sizeof (pty_name) - 3;
	ptydev = pty_name;
	o->ttydev = tty_name;

	for (c1 = PTYCHAR1; *c1; c1++) {
	    ptydev[len] = o->ttydev[len] = *c1;
	    for (c2 = PTYCHAR2; *c2; c2++) {
		ptydev[len + 1] = o->ttydev[len + 1] = *c2;
		if ((fd = open (ptydev, O_RDWR)) >= 0) {
		    if (access (o->ttydev, R_OK | W_OK) == 0) {
			o->ttydev = (char *) strdup (tty_name);
			goto Found;
		    }
		    close (fd);
		}
	    }
	}
    }
#endif

    print_error ("can't open pseudo-tty");
    return -1;

  Found:
    fcntl (fd, F_SETFL, O_NDELAY);
    return fd;
}
/*}}} */

/*{{{ establish a controlling teletype for new session */
/*
 * On some systems this can be done with ioctl() but on others we
 * need to re-open the slave tty.
 */
/* INTPROTO */
int             rxvtlib_get_tty (rxvtlib *o)
{E_
    int             fd, i;
    pid_t           pid;

/*
 * setsid() [or setpgrp] must be before open of the terminal,
 * otherwise there is no controlling terminal (Solaris 2.4, HP-UX 9)
 */
#ifndef ultrix
# ifdef NO_SETSID
    pid = setpgrp (0, 0);
# else
    pid = setsid ();
# endif
    if (pid < 0)
#ifdef STANDALONE
	perror (o->rs[Rs_name]);
#else
	perror ("rxvtlib_get_tty");
#endif
# ifdef DEBUG_TTYMODE
    print_error ("(%s: line %d): PID = %d\n", __FILE__, __LINE__, pid);
# endif
#endif				/* ultrix */

    if ((fd = open (o->ttydev, O_RDWR)) < 0
	&& (fd = open ("/dev/tty", O_RDWR)) < 0) {
	print_error ("can't open slave tty %s", o->ttydev);
	o->killed = EXIT_FAILURE | DO_EXIT;
	return -1;
    }
#if defined(PTYS_ARE_PTMX) && !defined(__FreeBSD__) && !defined(__DragonFly__)
/*
 * Push STREAMS modules:
 *    ptem: pseudo-terminal hardware emulation module.
 *    ldterm: standard terminal line discipline.
 *    ttcompat: V7, 4BSD and XENIX STREAMS compatibility module.
 */
    if (!o->changettyowner) {
	ioctl (fd, I_PUSH, "ptem");
	ioctl (fd, I_PUSH, "ldterm");
	ioctl (fd, I_PUSH, "ttcompat");
    }
#endif
    if (o->changettyowner) {
	/* change ownership of tty to real uid and real group */
	unsigned int    mode = 0622;
	gid_t           gid = getgid ();

#ifdef TTY_GID_SUPPORT
	{
	    struct group   *gr = getgrnam ("tty");

	    if (gr) {
		/* change ownership of tty to real uid, "tty" gid */
		gid = gr->gr_gid;
		mode = 0620;
	    }
	}
#endif				/* TTY_GID_SUPPORT */
#ifndef __CYGWIN32__
	privileges (RESTORE);
	fchown (fd, getuid (), gid);	/* fail silently */
	fchmod (fd, mode);
	privileges (IGNORE);
#endif
    }

/*
 * Close all file descriptors.  If only stdin/out/err are closed,
 * child processes remain alive upon deletion of the window.
 */
    for (i = 0; i < o->num_fds; i++)
	if (i != fd)
	    close (i);

/* Reopen stdin, stdout and stderr over the tty file descriptor */
    dup2 (fd, 0);		/* stdin */
    dup2 (fd, 1);		/* stdout */
    dup2 (fd, 2);		/* stderr */

    if (fd > 2)
	close (fd);

#ifdef ultrix
    if ((fd = open ("/dev/tty", O_RDONLY)) >= 0) {
	ioctl (fd, TIOCNOTTY, 0);
	close (fd);
    } else {
	pid = setpgrp (0, 0);
	if (pid < 0)
#ifdef STANDALONE
	    perror (o->rs[Rs_name]);
#else
	    perror ("rxvtlib_get_tty");
#endif
    }

/* no error, we could run with no tty to begin with */
#else				/* ultrix */
# ifdef TIOCSCTTY
    ioctl (0, TIOCSCTTY, 0);
# endif

/* set process group */
# if defined (_POSIX_VERSION) || defined (__svr4__)
    tcsetpgrp (0, pid);
# elif defined (TIOCSPGRP)
    ioctl (0, TIOCSPGRP, &pid);
# endif

/* svr4 problems: reports no tty, no job control */
/* # if !defined (__svr4__) && defined (TIOCSPGRP) */
    close (open (o->ttydev, O_RDWR, 0));
/* # endif */
#endif				/* ultrix */

    return fd;
}
/*}}} */

/*{{{ debug_ttymode() */
#ifdef DEBUG_TTYMODE
/* INTPROTO */
void            debug_ttymode (const ttymode_t * ttymode)
{E_
#ifdef HAVE_TERMIOS_H
/* c_iflag bits */
    fprintf (stderr, "Input flags\n");

/* cpp token stringize doesn't work on all machines <sigh> */

/* c_iflag bits */
    FOO (IGNBRK, "IGNBRK");
    FOO (BRKINT, "BRKINT");
    FOO (IGNPAR, "IGNPAR");
    FOO (PARMRK, "PARMRK");
    FOO (INPCK, "INPCK");
    FOO (ISTRIP, "ISTRIP");
    FOO (INLCR, "INLCR");
    FOO (IGNCR, "IGNCR");
    FOO (ICRNL, "ICRNL");
    FOO (IXON, "IXON");
    FOO (IXOFF, "IXOFF");
# ifdef IUCLC
    FOO (IUCLC, "IUCLC");
# endif
# ifdef IXANY
    FOO (IXANY, "IXANY");
# endif
# ifdef IMAXBEL
    FOO (IMAXBEL, "IMAXBEL");
# endif
    fprintf (stderr, "\n\n");

    FOO (VINTR, "VINTR");
    FOO (VQUIT, "VQUIT");
    FOO (VERASE, "VERASE");
    FOO (VKILL, "VKILL");
    FOO (VEOF, "VEOF");
    FOO (VEOL, "VEOL");
# ifdef VEOL2
    FOO (VEOL2, "VEOL2");
# endif
# ifdef VSWTC
    FOO (VSWTC, "VSWTC");
# endif
# ifdef VSWTCH
    FOO (VSWTCH, "VSWTCH");
# endif
    FOO (VSTART, "VSTART");
    FOO (VSTOP, "VSTOP");
    FOO (VSUSP, "VSUSP");
# ifdef VDSUSP
    FOO (VDSUSP, "VDSUSP");
# endif
# ifdef VREPRINT
    FOO (VREPRINT, "VREPRINT");
# endif
# ifdef VDISCRD
    FOO (VDISCRD, "VDISCRD");
# endif
# ifdef VWERSE
    FOO (VWERSE, "VWERSE");
# endif
# ifdef VLNEXT
    FOO (VLNEXT, "VLNEXT");
# endif
    fprintf (stderr, "\n\n");
#endif				/* HAVE_TERMIOS_H */
}

#endif				/* DEBUG_TTYMODE */
/*}}} */

/*{{{ get_ttymode() */
/* INTPROTO */
void            get_ttymode (ttymode_t * tio)
{E_
#ifdef HAVE_TERMIOS_H
/*
 * standard System V termios interface
 */
    if (GET_TERMIOS (0, tio) < 0) {
	/* return error - use system defaults */
	tio->c_cc[VINTR] = CINTR;
	tio->c_cc[VQUIT] = CQUIT;
	tio->c_cc[VERASE] = CERASE;
	tio->c_cc[VKILL] = CKILL;
	tio->c_cc[VSTART] = CSTART;
	tio->c_cc[VSTOP] = CSTOP;
	tio->c_cc[VSUSP] = CSUSP;
# ifdef VDSUSP
	tio->c_cc[VDSUSP] = CDSUSP;
# endif
# ifdef VREPRINT
	tio->c_cc[VREPRINT] = CRPRNT;
# endif
# ifdef VDISCRD
	tio->c_cc[VDISCRD] = CFLUSH;
# endif
# ifdef VWERSE
	tio->c_cc[VWERSE] = CWERASE;
# endif
# ifdef VLNEXT
	tio->c_cc[VLNEXT] = CLNEXT;
# endif
    }
    tio->c_cc[VEOF] = CEOF;
    tio->c_cc[VEOL] = VDISABLE;
# ifdef VEOL2
    tio->c_cc[VEOL2] = VDISABLE;
# endif
# ifdef VSWTC
    tio->c_cc[VSWTC] = VDISABLE;
# endif
# ifdef VSWTCH
    tio->c_cc[VSWTCH] = VDISABLE;
# endif
# if VMIN != VEOF
    tio->c_cc[VMIN] = 1;
# endif
# if VTIME != VEOL
    tio->c_cc[VTIME] = 0;
# endif

/* input modes */
    tio->c_iflag = (BRKINT | IGNPAR | ICRNL | IXON
# ifdef IMAXBEL
		    | IMAXBEL
# endif
	);

/* output modes */
    tio->c_oflag = (OPOST | ONLCR);

/* control modes */
    tio->c_cflag = (CS8 | CREAD);

/* line discipline modes */
    tio->c_lflag = (ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK
# if defined (ECHOCTL) && defined (ECHOKE)
		    | ECHOCTL | ECHOKE
# endif
	);
#else				/* HAVE_TERMIOS_H */

/*
 * sgtty interface
 */

/* get parameters -- gtty */
    if (ioctl (0, TIOCGETP, &(tio->sg)) < 0) {
	tio->sg.sg_erase = CERASE;	/* ^H */
	tio->sg.sg_kill = CKILL;	/* ^U */
    }
    tio->sg.sg_flags = (CRMOD | ECHO | EVENP | ODDP);

/* get special characters */
    if (ioctl (0, TIOCGETC, &(tio->tc)) < 0) {
	tio->tc.t_intrc = CINTR;	/* ^C */
	tio->tc.t_quitc = CQUIT;	/* ^\ */
	tio->tc.t_startc = CSTART;	/* ^Q */
	tio->tc.t_stopc = CSTOP;	/* ^S */
	tio->tc.t_eofc = CEOF;	/* ^D */
	tio->tc.t_brkc = -1;
    }
/* get local special chars */
    if (ioctl (0, TIOCGLTC, &(tio->lc)) < 0) {
	tio->lc.t_suspc = CSUSP;	/* ^Z */
	tio->lc.t_dsuspc = CDSUSP;	/* ^Y */
	tio->lc.t_rprntc = CRPRNT;	/* ^R */
	tio->lc.t_flushc = CFLUSH;	/* ^O */
	tio->lc.t_werasc = CWERASE;	/* ^W */
	tio->lc.t_lnextc = CLNEXT;	/* ^V */
    }
/* get line discipline */
    ioctl (0, TIOCGETD, &(tio->line));
# ifdef NTTYDISC
    tio->line = NTTYDISC;
# endif				/* NTTYDISC */
    tio->local = (LCRTBS | LCRTERA | LCTLECH | LPASS8 | LCRTKIL);
#endif				/* HAVE_TERMIOS_H */
}
/*}}} */


/*
 * Probe the modifier keymap to get the Meta (Alt) and Num_Lock settings
 * Use resource ``modifier'' to override the modifier
 */
/* INTPROTO */
void            rxvtlib_get_ourmods (rxvtlib *o)
{E_
    int             i, j, k, m;
    int             got_meta, got_numlock;
    XModifierKeymap *map;
    KeyCode        *kc;
    unsigned int    modmasks[] =
	{ Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask };

    got_meta = got_numlock = m = 0;
    if (o->rs[Rs_modifier]
	&& o->rs[Rs_modifier][0] == 'm'
	&& o->rs[Rs_modifier][1] == 'o'
	&& o->rs[Rs_modifier][2] == 'd'
	&& o->rs[Rs_modifier][3] >= '1' && o->rs[Rs_modifier][3] <= '5'
	&& !o->rs[Rs_modifier][4]) {
	o->ModMetaMask = modmasks[(o->rs[Rs_modifier][3] - '1')];
	got_meta = 1;
    }

    map = XGetModifierMapping (o->Xdisplay);
    kc = map->modifiermap;
    for (i = 3; i < 8; i++) {
	k = i * map->max_keypermod;
	for (j = 0; j < map->max_keypermod; j++, k++) {
	    if (kc[k] == 0)
		break;
	    switch (XKeycodeToKeysym (o->Xdisplay, kc[k], 0)) {
	    case XK_Num_Lock:
		if (!got_numlock) {
		    o->ModNumLockMask = modmasks[i - 3];
		    got_numlock = 1;
		}
		break;
	    case XK_Meta_L:
	    case XK_Meta_R:
		if (o->rs[Rs_modifier]
		    && !Cstrncasecmp (o->rs[Rs_modifier], "meta", 4)) {
		    o->ModMetaMask = modmasks[i - 3];
		    got_meta = 1;
		    break;
		}
		/* FALLTHROUGH */
	    case XK_Alt_L:
	    case XK_Alt_R:
		if (o->rs[Rs_modifier]
		    && !Cstrncasecmp (o->rs[Rs_modifier], "alt", 3)) {
		    o->ModMetaMask = modmasks[i - 3];
		    got_meta = 1;
		    break;
		}
		m = modmasks[i - 3];
		break;
	    case XK_Super_L:
	    case XK_Super_R:
		if (o->rs[Rs_modifier]
		    && !Cstrncasecmp (o->rs[Rs_modifier], "super", 5)) {
		    o->ModMetaMask = modmasks[i - 3];
		    got_meta = 1;
		}
		break;
	    case XK_Hyper_L:
	    case XK_Hyper_R:
		if (o->rs[Rs_modifier]
		    && !Cstrncasecmp (o->rs[Rs_modifier], "hyper", 5)) {
		    o->ModMetaMask = modmasks[i - 3];
		    got_meta = 1;
		}
		/* FALLTHROUGH */
	    default:
		break;
	    }
	}
	if (got_meta && got_numlock)
	    break;
    }
    XFreeModifiermap (map);
    if (!got_meta && m)
	o->ModMetaMask = m;
}

/*{{{ init_command() */
/* EXTPROTO */
void            rxvtlib_init_command (rxvtlib *o, const char *const *argv, int do_sleep)
{E_
/*
 * Initialize the command connection.
 * This should be called after the X server connection is established.
 */

/* Enable delete window protocol */
    o->wmDeleteWindow = XInternAtom (o->Xdisplay, "WM_DELETE_WINDOW", False);
    XSetWMProtocols (o->Xdisplay, o->TermWin.parent[0], &o->wmDeleteWindow, 1);

/* get number of available file descriptors */
#if defined(_POSIX_VERSION) || ! defined(__svr4__)
    o->num_fds = sysconf (_SC_OPEN_MAX);
#else
    o->num_fds = getdtablesize ();
#endif

#ifdef META8_OPTION
    o->meta_char = (o->Options & Opt_meta8 ? 0x80 : 033);
#endif
    rxvtlib_get_ourmods (o);
    if (!(o->Options & Opt_scrollTtyOutput))
	o->PrivateModes |= PrivMode_TtyOutputInh;
    if (o->Options & Opt_scrollKeypress)
	o->PrivateModes |= PrivMode_Keypress;
#ifndef NO_BACKSPACE_KEY
    if (strcmp (o->key_backspace, "DEC") == 0)
	o->PrivateModes |= PrivMode_HaveBackSpace;
#endif

#ifdef GREEK_SUPPORT
    greek_init ();
#endif

    o->Xfd = XConnectionNumber (o->Xdisplay);
    o->cmdbuf_ptr = o->cmdbuf_endp = o->cmdbuf_base;

    rxvtlib_run_command (o, argv, do_sleep);
    if (o->cmd_fd < 0) {
	print_error ("aborting");
	o->killed = EXIT_FAILURE | DO_EXIT;
    }
}
/*}}} */

#ifndef STANDALONE
extern int option_use_xim;
#endif

/*{{{ Xlocale */
/*
 * This is more or less stolen straight from XFree86 xterm.
 * This should support all European type languages.
 */
/* EXTPROTO */
void rxvtlib_init_xlocale (rxvtlib * o)
{E_
    char *locale = NULL;

    (void) locale;

#if !defined(NO_XSETLOCALE) || !defined(NO_SETLOCALE)
    locale = setlocale (LC_CTYPE, "");
#endif
#ifdef USE_XIM
    if (locale == NULL)
	print_error ("Setting locale failed.");
    else {
	/* To avoid Segmentation Fault in C locale */
#ifndef UTF8_FONT
	rxvtlib_setTermFontSet (o);
#endif
#ifndef STANDALONE
	if (option_use_xim)
#endif
#ifdef MULTICHAR_SET
	if (strcmp (locale, "C"))
#endif
	    XRegisterIMInstantiateCallback (o->Xdisplay, NULL, NULL, NULL,
					    rxvtlib_IMInstantiateCallback, (XPointer) o);
    }
#endif
}
/*}}} */

/*{{{ window resizing */
/*
 * Tell the teletype handler what size the window is.
 * Called after a window size change.
 */
/* INTPROTO */
void            rxvtlib_tt_winsize (rxvtlib *o, int fd)
{E_
    struct winsize  ws;

    if (fd < 0)
	return;

    ws.ws_col = (unsigned short)o->TermWin.ncol;
    ws.ws_row = (unsigned short)o->TermWin.nrow;
    ws.ws_xpixel = ws.ws_ypixel = 0;
    ioctl (fd, TIOCSWINSZ, &ws);
}

/* EXTPROTO */
void            rxvtlib_tt_resize (rxvtlib *o)
{E_
    rxvtlib_tt_winsize (o, o->cmd_fd);
}

/*}}} */

/*{{{ Convert the keypress event into a string */
/* INTPROTO */
void            rxvtlib_lookup_key (rxvtlib *o, XEvent * ev)
{E_
    int             ctrl, meta, shft, len;
    KeySym          keysym;
    static XComposeStatus compose = { NULL, 0 };
    static unsigned char kbuf[KBUFSZ];
    static int      numlock_state = 0;

#ifdef DEBUG_CMD
    static int      debug_key = 1;	/* accessible by a debugger only */
#endif
#ifdef GREEK_SUPPORT
    static short    greek_mode = 0;
#endif
#ifdef USE_XIM
    int             valid_keysym = 0;
#endif

/*
 * use Num_Lock to toggle Keypad on/off.  If Num_Lock is off, allow an
 * escape sequence to toggle the Keypad.
 *
 * Always permit `shift' to override the current setting
 */
    shft = (ev->xkey.state & ShiftMask);
    ctrl = (ev->xkey.state & ControlMask);
    meta = (ev->xkey.state & o->ModMetaMask);
    if (numlock_state || (ev->xkey.state & o->ModNumLockMask)) {
	numlock_state = (ev->xkey.state & o->ModNumLockMask);
	PrivMode ((!numlock_state), PrivMode_aplKP);
    }
#ifdef USE_XIM
    len = 0;
    if (o->Input_Context != NULL) {
	Status          status_return;

	kbuf[0] = '\0';
	len = XmbLookupString (o->Input_Context, &ev->xkey, (char *)kbuf,
			       sizeof (kbuf), &keysym, &status_return);
	valid_keysym = ((status_return == XLookupKeySym)
			|| (status_return == XLookupBoth));
    } else
	len = XLookupString (&ev->xkey, (char *)kbuf, sizeof (kbuf), &keysym,
			     &compose);

#else				/* USE_XIM */
    len = XLookupString (&ev->xkey, (char *)kbuf, sizeof (kbuf), &keysym,
			 &compose);



/*
 * map unmapped Latin[2-4]/Katakana/Arabic/Cyrillic/Greek entries -> Latin1
 * good for installations with correct fonts, but without XLOCALE
 */
    if (!len && (keysym >= 0x0100) && (keysym < 0x0800)) {
	len = 1;
	kbuf[0] = (keysym & 0xFF);
    }
#endif				/* USE_XIM */

    if (len && (o->Options & Opt_scrollKeypress))
	o->TermWin.view_start = 0;

#ifdef USE_XIM
    if (valid_keysym) {
#endif

/* for some backwards compatibility */
#if defined(HOTKEY_CTRL) || defined(HOTKEY_META)
# ifdef HOTKEY_CTRL
	if (ctrl)
# else
	    if (meta)
# endif
	    {
		if (keysym == o->ks_bigfont) {
		    rxvtlib_change_font (o, 0, FONT_UP);
		    return;
		} else if (keysym == o->ks_smallfont) {
		    rxvtlib_change_font (o, 0, FONT_DN);
		    return;
		}
	    }
#endif

	if (o->TermWin.saveLines) {
#ifdef UNSHIFTED_SCROLLKEYS
	    if (!ctrl && !meta)
#else
	    if (IS_SCROLL_MOD)
#endif
	    {
		int             lnsppg;

#ifdef PAGING_CONTEXT_LINES
		lnsppg = o->TermWin.nrow - PAGING_CONTEXT_LINES;
#else
		lnsppg = o->TermWin.nrow * 4 / 5;
#endif
		if (keysym == XK_Prior) {
		    rxvtlib_scr_page (o, UP, lnsppg);
		    return;
		} else if (keysym == XK_Next) {
		    rxvtlib_scr_page (o, DN, lnsppg);
		    return;
		}
	    }
	    if (IS_SCROLL_MOD) {
		if (keysym == XK_Up) {
		    rxvtlib_scr_page (o, UP, 1);
		    return;
		} else if (keysym == XK_Down) {
		    rxvtlib_scr_page (o, DN, 1);
		    return;
		}
	    }
	}

	if (shft) {
	    /* Shift + F1 - F10 generates F11 - F20 */
	    if (keysym >= XK_F1 && keysym <= XK_F10) {
		keysym += (XK_F11 - XK_F1);
		shft = 0;	/* turn off Shift */
	    } else if (!ctrl && !meta && (o->PrivateModes & PrivMode_ShiftKeys)) {
		switch (keysym) {
		    /* normal XTerm key bindings */
		case XK_Insert:	/* Shift+Insert = paste mouse selection */
		    rxvtlib_selection_request (o, ev->xkey.time, 0, 0);
		    return;
		    /* rxvt extras */
		case XK_KP_Add:	/* Shift+KP_Add = bigger font */
		    rxvtlib_change_font (o, 0, FONT_UP);
		    return;
		case XK_KP_Subtract:	/* Shift+KP_Subtract = smaller font */
		    rxvtlib_change_font (o, 0, FONT_DN);
		    return;
		}
	    }
	}
#ifdef PRINTPIPE
	if (keysym == XK_Print) {
	    rxvtlib_scr_printscreen (o, ctrl | shft);
	    return;
	}
#endif
#ifdef GREEK_SUPPORT
	if (keysym == XK_Mode_switch) {
	    greek_mode = !greek_mode;
	    if (greek_mode) {
		rxvtlib_xterm_seq (o, XTerm_title,
			   (greek_getmode () == GREEK_ELOT928 ? "[Greek: iso]"
			    : "[Greek: ibm]"));
		greek_reset ();
	    } else
		rxvtlib_xterm_seq (o, XTerm_title, APL_NAME "-" VERSION);
	    return;
	}
#endif

	if (keysym >= 0xFF00 && keysym <= 0xFFFF) {
#ifdef KEYSYM_RESOURCE
	    if (!(shft | ctrl) && o->KeySym_map[keysym - 0xFF00] != NULL) {
		unsigned int    len;
		const unsigned char *kbuf;
		const unsigned char ch = '\033';

		kbuf = (o->KeySym_map[keysym - 0xFF00]);
		len = *kbuf++;

		/* escape prefix */
		if (meta)
# ifdef META8_OPTION
		    if (o->meta_char == 033)
# endif
			rxvtlib_tt_write (o, &ch, 1);
		rxvtlib_tt_write (o, kbuf, len);
		return;
	    } else
#endif
		switch (keysym) {
#ifndef NO_BACKSPACE_KEY
		case XK_BackSpace:
		    if (o->PrivateModes & PrivMode_HaveBackSpace) {
			len = 1;
			kbuf[0] = ((!!(o->PrivateModes & PrivMode_BackSpace)
				    ^ !!(shft | ctrl)) ? '\b' : '\177');
		    } else
			len = strlen (STRCPY (kbuf, o->key_backspace));
		    break;
#endif
#ifndef NO_DELETE_KEY
		case XK_Delete:
		    len = strlen (STRCPY (kbuf, o->key_delete));
		    break;
#endif
		case XK_Tab:
		    if (shft) {
			len = 3;
			STRCPY (kbuf, "\033[Z");
		    }
		    break;

#ifdef XK_KP_Home
		case XK_KP_Home:
		    /* allow shift to override */
		    if ((o->PrivateModes & PrivMode_aplKP) ? !shft : shft) {
			len = 3;
			STRCPY (kbuf, "\033Ow");
			break;
		    }
		    /* FALLTHROUGH */
#endif
		case XK_Home:
		    len = strlen (STRCPY (kbuf, KS_HOME));
		    break;

#ifdef XK_KP_Left
		case XK_KP_Up:	/* \033Ox or standard */
		case XK_KP_Down:	/* \033Ow or standard */
		case XK_KP_Right:	/* \033Ov or standard */
		case XK_KP_Left:	/* \033Ot or standard */
		    if ((o->PrivateModes & PrivMode_aplKP) ? !shft : shft) {
			len = 3;
			STRCPY (kbuf, "\033OZ");
			kbuf[2] = ("txvw"[keysym - XK_KP_Left]);
			break;
		    } else
			/* translate to std. cursor key */
			keysym = XK_Left + (keysym - XK_KP_Left);
		    /* FALLTHROUGH */
#endif
		case XK_Up:	/* "\033[A" */
		case XK_Down:	/* "\033[B" */
		case XK_Right:	/* "\033[C" */
		case XK_Left:	/* "\033[D" */
		    len = 3;
		    STRCPY (kbuf, "\033[@");
		    kbuf[2] = ("DACB"[keysym - XK_Left]);
		    /* do Shift first */
		    if (shft)
			kbuf[2] = ("dacb"[keysym - XK_Left]);
		    else if (ctrl) {
			kbuf[1] = 'O';
			kbuf[2] = ("dacb"[keysym - XK_Left]);
		    } else if (o->PrivateModes & PrivMode_aplCUR)
			kbuf[1] = 'O';
		    break;

#ifndef UNSHIFTED_SCROLLKEYS
# ifdef XK_KP_Prior
		case XK_KP_Prior:
		    /* allow shift to override */
		    if ((o->PrivateModes & PrivMode_aplKP) ? !shft : shft) {
			len = 3;
			STRCPY (kbuf, "\033Oy");
			break;
		    }
		    /* FALLTHROUGH */
# endif
		case XK_Prior:
		    len = 4;
		    STRCPY (kbuf, "\033[5~");
		    break;
# ifdef XK_KP_Next
		case XK_KP_Next:
		    /* allow shift to override */
		    if ((o->PrivateModes & PrivMode_aplKP) ? !shft : shft) {
			len = 3;
			STRCPY (kbuf, "\033Os");
			break;
		    }
		    /* FALLTHROUGH */
# endif
		case XK_Next:
		    len = 4;
		    STRCPY (kbuf, "\033[6~");
		    break;
#endif
#ifdef XK_KP_End
		case XK_KP_End:
		    /* allow shift to override */
		    if ((o->PrivateModes & PrivMode_aplKP) ? !shft : shft) {
			len = 3;
			STRCPY (kbuf, "\033Oq");
			break;
		    }
		    /* FALLTHROUGH */
#endif
		case XK_End:
		    len = strlen (STRCPY (kbuf, KS_END));
		    break;

		case XK_Select:
		    len = 4;
		    STRCPY (kbuf, "\033[4~");
		    break;
#ifdef DXK_Remove		/* support for DEC remove like key */
		case DXK_Remove:
		    /* FALLTHROUGH */
#endif
		case XK_Execute:
		    len = 4;
		    STRCPY (kbuf, "\033[3~");
		    break;
		case XK_Insert:
		    len = 4;
		    STRCPY (kbuf, "\033[2~");
		    break;

		case XK_Menu:
		    len = 5;
		    STRCPY (kbuf, "\033[29~");
		    break;
		case XK_Find:
		    len = 4;
		    STRCPY (kbuf, "\033[1~");
		    break;
		case XK_Help:
		    len = 5;
		    STRCPY (kbuf, "\033[28~");
		    break;

		case XK_KP_Enter:
		    /* allow shift to override */
		    if ((o->PrivateModes & PrivMode_aplKP) ? !shft : shft) {
			len = 3;
			STRCPY (kbuf, "\033OM");
		    } else {
			len = 1;
			kbuf[0] = '\r';
		    }
		    break;

#ifdef XK_KP_Begin
		case XK_KP_Begin:
		    len = 3;
		    STRCPY (kbuf, "\033Ou");
		    break;

		case XK_KP_Insert:
		    len = 3;
		    STRCPY (kbuf, "\033Op");
		    break;

		case XK_KP_Delete:
		    len = 3;
		    STRCPY (kbuf, "\033On");
		    break;
#endif

		case XK_KP_F1:	/* "\033OP" */
		case XK_KP_F2:	/* "\033OQ" */
		case XK_KP_F3:	/* "\033OR" */
		case XK_KP_F4:	/* "\033OS" */
		    len = 3;
		    STRCPY (kbuf, "\033OP");
		    kbuf[2] += (keysym - XK_KP_F1);
		    break;

		case XK_KP_Multiply:	/* "\033Oj" : "*" */
		case XK_KP_Add:	/* "\033Ok" : "+" */
		case XK_KP_Separator:	/* "\033Ol" : "," */
		case XK_KP_Subtract:	/* "\033Om" : "-" */
		case XK_KP_Decimal:	/* "\033On" : "." */
		case XK_KP_Divide:	/* "\033Oo" : "/" */
		case XK_KP_0:	/* "\033Op" : "0" */
		case XK_KP_1:	/* "\033Oq" : "1" */
		case XK_KP_2:	/* "\033Or" : "2" */
		case XK_KP_3:	/* "\033Os" : "3" */
		case XK_KP_4:	/* "\033Ot" : "4" */
		case XK_KP_5:	/* "\033Ou" : "5" */
		case XK_KP_6:	/* "\033Ov" : "6" */
		case XK_KP_7:	/* "\033Ow" : "7" */
		case XK_KP_8:	/* "\033Ox" : "8" */
		case XK_KP_9:	/* "\033Oy" : "9" */
		    /* allow shift to override */
		    if ((o->PrivateModes & PrivMode_aplKP) ? !shft : shft) {
			len = 3;
			STRCPY (kbuf, "\033Oj");
			kbuf[2] += (keysym - XK_KP_Multiply);
		    } else {
			len = 1;
			kbuf[0] = ('*' + (keysym - XK_KP_Multiply));
		    }
		    break;

		case XK_F1:	/* "\033[11~" */
		case XK_F2:	/* "\033[12~" */
		case XK_F3:	/* "\033[13~" */
		case XK_F4:	/* "\033[14~" */
		case XK_F5:	/* "\033[15~" */
		    FKEY (11, XK_F1);
		    break;

		case XK_F6:	/* "\033[17~" */
		case XK_F7:	/* "\033[18~" */
		case XK_F8:	/* "\033[19~" */
		case XK_F9:	/* "\033[20~" */
		case XK_F10:	/* "\033[21~" */
		    FKEY (17, XK_F6);
		    break;

		case XK_F11:	/* "\033[23~" */
		case XK_F12:	/* "\033[24~" */
		case XK_F13:	/* "\033[25~" */
		case XK_F14:	/* "\033[26~" */
		    FKEY (23, XK_F11);
		    break;

		case XK_F15:	/* "\033[28~" */
		case XK_F16:	/* "\033[29~" */
		    FKEY (28, XK_F15);
		    break;

		case XK_F17:	/* "\033[31~" */
		case XK_F18:	/* "\033[32~" */
		case XK_F19:	/* "\033[33~" */
		case XK_F20:	/* "\033[34~" */
		case XK_F21:	/* "\033[35~" */
		case XK_F22:	/* "\033[36~" */
		case XK_F23:	/* "\033[37~" */
		case XK_F24:	/* "\033[38~" */
		case XK_F25:	/* "\033[39~" */
		case XK_F26:	/* "\033[40~" */
		case XK_F27:	/* "\033[41~" */
		case XK_F28:	/* "\033[42~" */
		case XK_F29:	/* "\033[43~" */
		case XK_F30:	/* "\033[44~" */
		case XK_F31:	/* "\033[45~" */
		case XK_F32:	/* "\033[46~" */
		case XK_F33:	/* "\033[47~" */
		case XK_F34:	/* "\033[48~" */
		case XK_F35:	/* "\033[49~" */
		    FKEY (31, XK_F17);
		    break;
		}
	    /*
	     * Pass meta for all function keys, if 'meta' option set
	     */
#ifdef META8_OPTION
	    if (meta && (o->meta_char == 0x80) && len > 0)
		kbuf[len - 1] |= 0x80;
#endif
	} else if (ctrl && keysym == XK_minus) {
	    len = 1;
	    kbuf[0] = '\037';	/* Ctrl-Minus generates ^_ (31) */
	} else {
#ifdef META8_OPTION
	    /* set 8-bit on */
	    if (meta && (o->meta_char == 0x80)) {
		unsigned char  *ch;

		for (ch = kbuf; ch < kbuf + len; ch++)
		    *ch |= 0x80;
		meta = 0;
	    }
#endif
#ifdef GREEK_SUPPORT
	    if (greek_mode)
		len = greek_xlat (kbuf, len);
#endif
	    /* nil */ ;
	}
#ifdef USE_XIM
    }
#endif

    if (len <= 0)
	return;			/* not mapped */

/*
 * these modifications only affect the static keybuffer
 * pass Shift/Control indicators for function keys ending with `~'
 *
 * eg,
 *   Prior = "ESC[5~"
 *   Shift+Prior = "ESC[5~"
 *   Ctrl+Prior = "ESC[5^"
 *   Ctrl+Shift+Prior = "ESC[5@"
 * Meta adds an Escape prefix (with META8_OPTION, if meta == <escape>).
 */
    if (kbuf[0] == '\033' && kbuf[1] == '[' && kbuf[len - 1] == '~')
	kbuf[len - 1] = (shft ? (ctrl ? '@' : '$') : (ctrl ? '^' : '~'));

/* escape prefix */
    if (meta
#ifdef META8_OPTION
	&& (o->meta_char == 033)
#endif
	) {
	const unsigned char ch = '\033';

	rxvtlib_tt_write (o, &ch, 1);
    }
#ifdef DEBUG_CMD
    if (debug_key) {		/* Display keyboard buffer contents */
	char           *p;
	int             i;

	fprintf (stderr, "key 0x%04X [%d]: `", (unsigned int)keysym, len);
	for (i = 0, p = kbuf; i < len; i++, p++)
	    fprintf (stderr, (*p >= ' ' && *p < '\177' ? "%c" : "\\%03o"), *p);
	fprintf (stderr, "'\n");
    }
#endif				/* DEBUG_CMD */
    rxvtlib_tt_write (o, kbuf, len);
}
/*}}} */

#if (MENUBAR_MAX)
/*{{{ cmd_write(), cmd_getc() */
/* attempt to `write' COUNT to the input buffer */
/* EXTPROTO */
unsigned int    rxvtlib_cmd_write (rxvtlib *o, const unsigned char *str, unsigned int count)
{E_
    int             n;

    n = (count - (o->cmdbuf_ptr - o->cmdbuf_base));
/* need to insert more chars that space available in the front */
    if (n > 0) {
	/* try and get more space from the end */
	unsigned char  *src, *dst;

	dst = (o->cmdbuf_base + sizeof (o->cmdbuf_base) - 1);	/* max pointer */

	if ((o->cmdbuf_ptr + n) > dst)
	    n = (dst - o->cmdbuf_ptr);	/* max # chars to insert */

	if ((o->cmdbuf_endp + n) > dst)
	    o->cmdbuf_endp = (dst - n);	/* truncate end if needed */

	/* equiv: memmove ((cmdbuf_ptr+n), cmdbuf_ptr, n); */
	src = o->cmdbuf_endp;
	dst = src + n;
	/* FIXME: anything special to avoid possible pointer wrap? */
	while (src >= o->cmdbuf_ptr)
	    *dst-- = *src--;

	/* done */
	o->cmdbuf_ptr += n;
	o->cmdbuf_endp += n;
    }
    while (count-- && o->cmdbuf_ptr > o->cmdbuf_base) {
	/* sneak one in */
	o->cmdbuf_ptr--;
	*o->cmdbuf_ptr = str[count];
    }

    return 0;
}
#endif				/* MENUBAR_MAX */

#ifdef STANDALONE
/* cmd_getc() - Return next input character */
/*
 * Return the next input character after first passing any keyboard input
 * to the command.
 */
/* INTPROTO */
unsigned char rxvtlib_cmd_getc (rxvtlib * o)
{E_
    fd_set readfds;
    struct timeval value;
    int quick_timeout;
    int retval;

/* If there have been a lot of new lines, then update the screen
 * What the heck I'll cheat and only refresh less than every page-full.
 * the number of pages between refreshes is refresh_limit, which
 * is incremented here because we must be doing flat-out scrolling.
 *
 * refreshing should be correct for small scrolls, because of the
 * time-out */
    if (o->refresh_count >= (o->refresh_limit * (o->TermWin.nrow - 1))) {
	if (o->refresh_limit < REFRESH_PERIOD)
	    o->refresh_limit++;
	o->refresh_count = 0;
	rxvtlib_scr_refresh (o, o->refresh_type);
    }

/* characters already read in */
    if (o->cmdbuf_ptr < o->cmdbuf_endp)
	return (*o->cmdbuf_ptr++);

    while (!o->killed) {
	if (o->v_bufstr < o->v_bufptr)	/* output any pending chars */
	    rxvtlib_tt_write (o, NULL, 0);

	while (XPending (o->Xdisplay)) {			/* process pending X events */
	    rxvtlib_XProcessEvent (o, o->Xdisplay);
	    if (o->killed)
		return 0;
	    /* in case button actions pushed chars to cmdbuf */
	    if (o->cmdbuf_ptr < o->cmdbuf_endp)
		return (*o->cmdbuf_ptr++);
	}
#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
	if (scrollbar_isUp ()) {
	    if (!o->scroll_arrow_delay-- && rxvtlib_scr_page (o, UP, 1)) {
		o->scroll_arrow_delay = SCROLLBAR_CONTINUOUS_DELAY;
		o->refresh_type |= SMOOTH_REFRESH;
	    }
	} else if (scrollbar_isDn ()) {
	    if (!o->scroll_arrow_delay-- && rxvtlib_scr_page (o, DN, 1)) {
		o->scroll_arrow_delay = SCROLLBAR_CONTINUOUS_DELAY;
		o->refresh_type |= SMOOTH_REFRESH;
	    }
	}
#endif				/* NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING */

	/* Nothing to do! */
	FD_ZERO (&readfds);
	FD_SET (o->cmd_fd, &readfds);
	FD_SET (o->Xfd, &readfds);
	value.tv_usec = TIMEOUT_USEC;
	value.tv_sec = 0;

#ifdef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
	quick_timeout = o->want_refresh;
#else
	quick_timeout = o->want_refresh || scrollbar_isUpDn ();
#endif
	retval = select (o->num_fds, &readfds, NULL, NULL,
			 (quick_timeout ? &value : NULL));
	/* See if we can read from the application */
	if (FD_ISSET (o->cmd_fd, &readfds)) {
	    int count, n;
	    if (o->cmdbuf_ptr != o->cmdbuf_endp)
		return (*o->cmdbuf_ptr++);
	    o->cmdbuf_ptr = o->cmdbuf_endp = o->cmdbuf_base;
	    for (count = BUFSIZ; count; count -= n, o->cmdbuf_endp += n)
		if ((n = read (o->cmd_fd, o->cmdbuf_endp, count)) <= 0)
		    break;
	    if (count != BUFSIZ)	/* some characters read in */
		return (*o->cmdbuf_ptr++);
	}
	/* select statement timed out - we're not hard and fast scrolling */
	if (retval == 0) {
	    o->refresh_count = 0;
	    o->refresh_limit = 1;
	}
	if (o->want_refresh) {
	    rxvtlib_scr_refresh (o, o->refresh_type);
	    rxvtlib_scrollbar_show (o, 1);
	    if (o->killed)
		return 0;
#ifdef USE_XIM
	    rxvtlib_IMSendSpot (o);
#endif
	}
    }
    return 0;
}
/*}}} */

#else

void rxvtlib_update_screen (rxvtlib * o)
{E_
    if (o->refresh_count >= (o->refresh_limit * (o->TermWin.nrow - 1))) {
	if (o->refresh_limit < REFRESH_PERIOD)
	    o->refresh_limit++;
	o->refresh_count = 0;
	rxvtlib_scr_refresh (o, o->refresh_type);
    }
#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
    if (scrollbar_isUp ()) {
	if (!o->scroll_arrow_delay-- && rxvtlib_scr_page (o, UP, 1)) {
	    o->scroll_arrow_delay = SCROLLBAR_CONTINUOUS_DELAY;
	    o->refresh_type |= SMOOTH_REFRESH;
	}
    } else if (scrollbar_isDn ()) {
	if (!o->scroll_arrow_delay-- && rxvtlib_scr_page (o, DN, 1)) {
	    o->scroll_arrow_delay = SCROLLBAR_CONTINUOUS_DELAY;
	    o->refresh_type |= SMOOTH_REFRESH;
	}
    }
#endif				/* NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING */
    if (CIsIdle ()) {
	o->refresh_count = 0;
	o->refresh_limit = 1;
    }
    if (o->want_refresh) {
	rxvtlib_scr_refresh (o, o->refresh_type);
	rxvtlib_scrollbar_show (o, 1);
#ifdef USE_XIM
	rxvtlib_IMSendSpot (o);
#endif
    }
}

static int rxvt_fd_read (rxvtlib * o)
{E_
    int n, count = 0;
    if (o->cmdbuf_ptr >= o->cmdbuf_endp)
	o->cmdbuf_ptr = o->cmdbuf_endp = o->cmdbuf_base;
    for (;;) {
	n = read (o->cmd_fd, o->cmdbuf_endp, BUFSIZ - (o->cmdbuf_endp - o->cmdbuf_base));
	if (n <= 0)
	    break;
	count += n;
	o->cmdbuf_endp += n;
    }
    return count;
}

void rxvt_fd_read_watch (int fd, fd_set * reading, fd_set * writing,
				fd_set * error, void *data)
{E_
    rxvtlib *o = (rxvtlib *) data;
    if (!rxvt_fd_read (o))
	return;
    rxvtlib_main_loop (o);
    rxvtlib_update_screen (o);
}

void rxvt_process_x_event (rxvtlib * o)
{E_
    o->refresh_count = 0;
    o->refresh_limit = 1;
    if (o->x_events_pending)
	rxvtlib_XProcessEvent (o, o->Xdisplay);
    if (o->killed) {
	close (o->cmd_fd);
	kill (o->cmd_pid, SIGTERM);
    }
#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
    if (scrollbar_isUp ()) {
	if (!o->scroll_arrow_delay-- && rxvtlib_scr_page (o, UP, 1)) {
	    o->scroll_arrow_delay = SCROLLBAR_CONTINUOUS_DELAY;
	    o->refresh_type |= SMOOTH_REFRESH;
	}
    } else if (scrollbar_isDn ()) {
	if (!o->scroll_arrow_delay-- && rxvtlib_scr_page (o, DN, 1)) {
	    o->scroll_arrow_delay = SCROLLBAR_CONTINUOUS_DELAY;
	    o->refresh_type |= SMOOTH_REFRESH;
	}
    }
#endif
}

/* cmd_getc() - Return next input character */
/*
 * Return the next input character after first passing any keyboard input
 * to the command.
 */
/* INTPROTO */
unsigned char rxvtlib_cmd_getc (rxvtlib * o)
{E_
    int ch = -1;
    if (o->v_bufstr < o->v_bufptr)	/* output any pending chars */
	rxvtlib_tt_write (o, NULL, 0);
    while (ch == -1) {
	if (o->cmdbuf_ptr < o->cmdbuf_endp)
	    ch = (unsigned char) (*o->cmdbuf_ptr++);
	else
	    rxvt_fd_read (o);
    }
    if (o->cmdbuf_endp == o->cmdbuf_base + BUFSIZ && o->cmdbuf_ptr < o->cmdbuf_endp) {
/* can't read past the end of the buffer */
	CRemoveWatch (o->cmd_fd, rxvt_fd_read_watch, 1);
    } else {
	CAddWatch (o->cmd_fd, rxvt_fd_read_watch, 1, (void *) o);
    }
    return ch;
}

/*}}} */

#endif

/*
 * the 'essential' information for reporting Mouse Events
 * pared down from XButtonEvent
 */
/* milliseconds *//* key or button mask *//* detail */

/* INTPROTO */
void            rxvtlib_mouse_report (rxvtlib *o, const XButtonEvent * ev)
{E_
    int             button_number, key_state = 0;
    int             x, y;

    x = ev->x;
    y = ev->y;
    pixel_position (o, &x, &y);

    button_number = ((o->MEvent.button == AnyButton) ? 3 :
		     (o->MEvent.button - Button1));

    if (o->PrivateModes & PrivMode_MouseX10) {
	/*
	 * do not report ButtonRelease
	 * no state info allowed
	 */
	key_state = 0;
	if (button_number == 3)
	    return;
    } else {
	/* XTerm mouse reporting needs these values:
	 *   4 = Shift
	 *   8 = Meta
	 *  16 = Control
	 * plus will add in our own Double-Click reporting
	 *  32 = Double Click
	 */
	key_state = ((o->MEvent.state & ShiftMask) ? 4 : 0)
	    + ((o->MEvent.state & o->ModMetaMask) ? 8 : 0)
	    + ((o->MEvent.state & ControlMask) ? 16 : 0);
#ifdef MOUSE_REPORT_DOUBLECLICK
	key_state += ((o->MEvent.clicks > 1) ? 32 : 0);
#endif
    }

#ifdef DEBUG_MOUSEREPORT
    fprintf (stderr, "Mouse [");
    if (key_state & 16)
	fputc ('C', stderr);
    if (key_state & 4)
	fputc ('S', stderr);
    if (key_state & 8)
	fputc ('A', stderr);
    if (key_state & 32)
	fputc ('2', stderr);
    fprintf (stderr, "]: <%d>, %d/%d\n", button_number, x + 1, y + 1);
#else
    rxvtlib_tt_printf (o, "\033[M%c%c%c",
	       (32 + button_number + key_state), (32 + x + 1), (32 + y + 1));
#endif
}

/*{{{ process an X event */
/* INTPROTO */
void rxvtlib_process_x_event (rxvtlib * o, XEvent * ev)
{E_
    int reportmode;
    Window unused_root, unused_child;
    int unused_root_x, unused_root_y;
    unsigned int unused_mask;
    static int bypass_keystate = 0;
    static int csrO = 0;	/* Hops - csr offset in thumb/slider      */

    /*        to give proper Scroll behaviour */
#ifdef DEBUG_X
    char *eventnames[] = {	/* mason - this matches my system */
	"",
	"",
	"KeyPress",
	"KeyRelease",
	"ButtonPress",
	"ButtonRelease",
	"MotionNotify",
	"EnterNotify",
	"LeaveNotify",
	"FocusIn",
	"FocusOut",
	"KeymapNotify",
	"Expose",
	"GraphicsExpose",
	"NoExpose",
	"VisibilityNotify",
	"CreateNotify",
	"DestroyNotify",
	"UnmapNotify",
	"MapNotify",
	"MapRequest",
	"ReparentNotify",
	"ConfigureNotify",
	"ConfigureRequest",
	"GravityNotify",
	"ResizeRequest",
	"CirculateNotify",
	"CirculateRequest",
	"PropertyNotify",
	"SelectionClear",
	"SelectionRequest",
	"SelectionNotify",
	"ColormapNotify",
	"ClientMessage",
	"MappingNotify"
    };

    fprintf (stderr, "Event type: %-16s, Window: %lx (p:%lx,vt:%lx,sb:%lx)\n",
	     eventnames[ev->type], ev->xany.window, o->TermWin.parent[0],
	     o->TermWin.vt, o->scrollBar.win);
    fflush (stderr);
#endif

    switch (ev->type) {
    case KeyPress:
	rxvtlib_lookup_key (o, ev);
	break;

    case ClientMessage:
	if (ev->xclient.format == 32 && ev->xclient.data.l[0] == o->wmDeleteWindow) {
#ifndef STANDALONE
	    XDestroyWindow (o->Xdisplay, o->TermWin.parent[0]);
	    XDestroyWindow (o->Xdisplay, o->TermWin.vt);
	    XDestroyWindow (o->Xdisplay, o->scrollBar.win);
	    XFlush (o->Xdisplay);
#endif
	    o->killed = EXIT_SUCCESS | DO_EXIT;
	    return;
	}
	break;

    case MappingNotify:
	XRefreshKeyboardMapping (&(ev->xmapping));
	break;

	/*
	 * XXX: this is not the _current_ arrangement
	 * Here's my conclusion:
	 * If the window is completely unobscured, use bitblt's
	 * to scroll. Even then, they're only used when doing partial
	 * screen scrolling. When partially obscured, we have to fill
	 * in the GraphicsExpose parts, which means that after each refresh,
	 * we need to wait for the graphics expose or Noexpose events,
	 * which ought to make things real slow!
	 */
    case VisibilityNotify:
	switch (ev->xvisibility.state) {
	case VisibilityUnobscured:
	    o->refresh_type = FAST_REFRESH;
	    break;
	case VisibilityPartiallyObscured:
	    o->refresh_type = SLOW_REFRESH;
	    break;
	default:
	    o->refresh_type = NO_REFRESH;
	    break;
	}
	break;

    case FocusIn:
	if (!o->TermWin.focus) {
	    o->TermWin.focus = 1;
	    o->want_refresh = 1;
#ifdef USE_XIM
	    if (o->Input_Context != NULL)
		XSetICFocus (o->Input_Context);
#endif
	}
	break;

    case FocusOut:
	if (o->TermWin.focus) {
	    o->TermWin.focus = 0;
	    o->want_refresh = 1;
#ifdef USE_XIM
	    if (o->Input_Context != NULL)
		XUnsetICFocus (o->Input_Context);
#endif
	}
	break;

    case ConfigureNotify:
	if (ev->xconfigure.window != o->TermWin.parent[0])
	    break;
#ifdef TRANSPARENT		/* XXX: maybe not needed - leave in for now */
	if (o->Options & Opt_transparent)
	    rxvtlib_check_our_parents (o);
#endif
	if (!rxvtlib_RemoveFromCNQueue (o, ev->xconfigure.width, ev->xconfigure.height)) {
	    rxvtlib_resize_window (o, ev->xconfigure.width, ev->xconfigure.height);
#ifdef USE_XIM
	    rxvtlib_IMSetStatusPosition (o);
#endif
	}
#ifdef TRANSPARENT
	if (o->Options & Opt_transparent) {
	    rxvtlib_scr_clear (o);
	    rxvtlib_scr_touch (o);
	}
#endif
	break;

    case SelectionClear:
	rxvtlib_selection_clear (o);
	break;

    case SelectionNotify:
	rxvtlib_selection_paste (o, ev->xselection.requestor, ev->xselection.property, True);
	break;

    case SelectionRequest:
	rxvtlib_selection_send (o, &(ev->xselectionrequest));
	break;

    case UnmapNotify:
	o->TermWin.mapped = 0;
	break;

    case MapNotify:
	o->TermWin.mapped = 1;
	break;

#ifdef TRANSPARENT
    case PropertyNotify:
	{
	    /*
	     * if user used some Esetroot compatible prog to set the root
	     * bg, use the property to determine that. We don't use it's
	     * value, yet
	     */
	    static Atom atom = 0;

	    if (!atom)
		atom = XInternAtom (o->Xdisplay, "_XROOTPMAP_ID", False);
	    if (ev->xproperty.atom != atom)
		break;
	}
	/* FALLTHROUGH */

    case ReparentNotify:
	if ((o->Options & Opt_transparent)
	    && rxvtlib_check_our_parents (o)) {	/* parents change then clear screen */
	    rxvtlib_scr_clear (o);
	    rxvtlib_scr_touch (o);
	}
	break;
#endif				/* TRANSPARENT */

    case GraphicsExpose:
    case Expose:
	if (ev->xany.window == o->TermWin.vt) {
	    rxvtlib_scr_expose (o, ev->xexpose.x, ev->xexpose.y,
				ev->xexpose.width, ev->xexpose.height);
	} else {
	    XEvent unused_xevent;

	    while (XCheckTypedWindowEvent (o->Xdisplay, ev->xany.window, Expose, &unused_xevent));
	    while (XCheckTypedWindowEvent (o->Xdisplay, ev->xany.window,
					   GraphicsExpose, &unused_xevent));
	    if (isScrollbarWindow (ev->xany.window)) {
		scrollbar_setNone ();
		rxvtlib_scrollbar_show (o, 0);
	    }
	    if (menubar_visible () && isMenuBarWindow (ev->xany.window))
		rxvtlib_menubar_expose (o);
	    rxvtlib_Gr_expose (o, ev->xany.window);
	}
	break;

    case ButtonPress:
	bypass_keystate = (ev->xbutton.state & (o->ModMetaMask | ShiftMask));
	reportmode = (bypass_keystate ? 0 : (o->PrivateModes & PrivMode_mouse_report));

	if (ev->xany.window == o->TermWin.vt) {
	    if (ev->xbutton.subwindow != None)
		Gr_ButtonPress (ev->xbutton.x, ev->xbutton.y);
	    else {
		if (reportmode) {
		    /* mouse report from vt window */
		    /* save the xbutton state (for ButtonRelease) */
		    o->MEvent.state = ev->xbutton.state;
#ifdef MOUSE_REPORT_DOUBLECLICK
		    if (ev->xbutton.button == o->MEvent.button
			&& (ev->xbutton.time - o->MEvent.time < MULTICLICK_TIME)) {
			/* same button, within alloted time */
			o->MEvent.clicks++;
			if (o->MEvent.clicks > 1) {
			    /* only report double clicks */
			    o->MEvent.clicks = 2;
			    rxvtlib_mouse_report (o, &(ev->xbutton));

			    /* don't report the release */
			    o->MEvent.clicks = 0;
			    o->MEvent.button = AnyButton;
			}
		    } else {
			/* different button, or time expired */
			o->MEvent.clicks = 1;
			o->MEvent.button = ev->xbutton.button;
			rxvtlib_mouse_report (o, &(ev->xbutton));
		    }
#else
		    o->MEvent.button = ev->xbutton.button;
		    rxvtlib_mouse_report (o, &(ev->xbutton));
#endif				/* MOUSE_REPORT_DOUBLECLICK */
		} else {
		    if (ev->xbutton.button != o->MEvent.button)
			o->MEvent.clicks = 0;
		    switch (ev->xbutton.button) {
		    case Button1:
			if (o->MEvent.button == Button1
			    && (ev->xbutton.time - o->MEvent.time <
				MULTICLICK_TIME)) o->MEvent.clicks++;
			else
			    o->MEvent.clicks = 1;
			rxvtlib_selection_click (o, o->MEvent.clicks, ev->xbutton.x, ev->xbutton.y);
			o->MEvent.button = Button1;
			break;

		    case Button3:
			if (o->MEvent.button == Button3
			    && (ev->xbutton.time - o->MEvent.time <
				MULTICLICK_TIME))
				rxvtlib_selection_rotate (o, ev->xbutton.x, ev->xbutton.y);
			else
			    rxvtlib_selection_extend (o, ev->xbutton.x, ev->xbutton.y, 1);
			o->MEvent.button = Button3;
			break;
		    }
		}
		o->MEvent.time = ev->xbutton.time;
		return;
	    }
	}
	if (isScrollbarWindow (ev->xany.window)) {
	    scrollbar_setNone ();
	    /*
	     * Rxvt-style scrollbar:
	     * move up if mouse is above slider
	     * move dn if mouse is below slider
	     *
	     * XTerm-style scrollbar:
	     * Move display proportional to pointer location
	     * pointer near top -> scroll one line
	     * pointer near bot -> scroll full page
	     */
#ifndef NO_SCROLLBAR_REPORT
	    if (reportmode) {
		/*
		 * Mouse report disabled scrollbar:
		 * arrow buttons - send up/down
		 * click on scrollbar - send pageup/down
		 */
		if (scrollbar_upButton (ev->xbutton.y))
		    rxvtlib_tt_printf (o, "\033[A");
		else if (scrollbar_dnButton (ev->xbutton.y))
		    rxvtlib_tt_printf (o, "\033[B");
		else
		    switch (ev->xbutton.button) {
		    case Button2:
			rxvtlib_tt_printf (o, "\014");
			break;
		    case Button1:
			rxvtlib_tt_printf (o, "\033[6~");
			break;
		    case Button3:
			rxvtlib_tt_printf (o, "\033[5~");
			break;
		    }
	    } else
#endif				/* NO_SCROLLBAR_REPORT */
	    {
		if (scrollbar_upButton (ev->xbutton.y)) {
#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
		    o->scroll_arrow_delay = SCROLLBAR_INITIAL_DELAY;
#endif
		    if (rxvtlib_scr_page (o, UP, 1))
			scrollbar_setUp ();
		} else if (scrollbar_dnButton (ev->xbutton.y)) {
#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
		    o->scroll_arrow_delay = SCROLLBAR_INITIAL_DELAY;
#endif
		    if (rxvtlib_scr_page (o, DN, 1))
			scrollbar_setDn ();
		} else
		    switch (ev->xbutton.button) {
		    case Button2:
#if ! defined(FUNKY_SCROLL_BEHAVIOUR)
			/* align to thumb centre */
			csrO = (o->scrollBar.bot - o->scrollBar.top) / 2;
#elif ! defined(XTERM_SCROLLBAR)
			if (scrollbar_above_slider (ev->xbutton.y)
			    || scrollbar_below_slider (ev->xbutton.y))
#endif				/* FUNKY_SCROLL_BEHAVIOUR */
			    rxvtlib_scr_move_to (o, scrollbar_position (ev->xbutton.y) -
						 csrO, scrollbar_size ());
			scrollbar_setMotion ();
			break;

		    case Button1:
#ifndef FUNKY_SCROLL_BEHAVIOUR
			/* ptr offset in thumb */
			csrO = ev->xbutton.y - o->scrollBar.top;
#endif
			/* FALLTHROUGH */

		    case Button3:
#ifndef XTERM_SCROLLBAR
			if (scrollbar_above_slider (ev->xbutton.y))
# ifdef RXVT_SCROLL_FULL
			    rxvtlib_scr_page (o, UP, o->TermWin.nrow - 1);
# else
			rxvtlib_scr_page (o, UP, o->TermWin.nrow / 4);
# endif
			else
		    if (scrollbar_below_slider (ev->xbutton.y))
# ifdef RXVT_SCROLL_FULL
			rxvtlib_scr_page (o, DN, o->TermWin.nrow - 1);
# else
			rxvtlib_scr_page (o, DN, o->TermWin.nrow / 4);
# endif
			else
			scrollbar_setMotion ();
#else				/* XTERM_SCROLLBAR */
			rxvtlib_scr_page (o (o, ev->xbutton.button == Button1 ? DN : UP),
					  (o->TermWin.nrow *
					   scrollbar_position (ev->xbutton.y) / scrollbar_size ())
			    );
#endif				/* XTERM_SCROLLBAR */
			break;
		    }
	    }
	    return;
	}
	if (isMenuBarWindow (ev->xany.window)) {
	    rxvtlib_menubar_control (o, &(ev->xbutton));
	    return;
	}
	break;

    case ButtonRelease:
	csrO = 0;		/* reset csr Offset */
	reportmode = bypass_keystate ? 0 : (o->PrivateModes & PrivMode_mouse_report);

	if (scrollbar_isUpDn ()) {
	    scrollbar_setNone ();
	    rxvtlib_scrollbar_show (o, 0);
#ifndef NO_SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
	    o->refresh_type &= ~SMOOTH_REFRESH;
#endif
	}
	if (ev->xany.window == o->TermWin.vt) {
	    if (ev->xbutton.subwindow != None)
		Gr_ButtonRelease (ev->xbutton.x, ev->xbutton.y);
	    else {
		if (reportmode) {
		    /* mouse report from vt window */
#ifdef MOUSE_REPORT_DOUBLECLICK
		    /* only report the release of 'slow' single clicks */
		    if (o->MEvent.button != AnyButton
			&& (ev->xbutton.button != o->MEvent.button
			    || (ev->xbutton.time - o->MEvent.time > MULTICLICK_TIME / 2))
			) {
			o->MEvent.clicks = 0;
			o->MEvent.button = AnyButton;
			rxvtlib_mouse_report (o, &(ev->xbutton));
		    }
#else				/* MOUSE_REPORT_DOUBLECLICK */
		    o->MEvent.button = AnyButton;
		    rxvtlib_mouse_report (o, &(ev->xbutton));
#endif				/* MOUSE_REPORT_DOUBLECLICK */
		    return;
		}
		/*
		 * dumb hack to compensate for the failure of click-and-drag
		 * when overriding mouse reporting
		 */
		if (o->PrivateModes & PrivMode_mouse_report
		    && bypass_keystate && ev->xbutton.button == Button1 && o->MEvent.clicks <= 1)
		    rxvtlib_selection_extend (o, ev->xbutton.x, ev->xbutton.y, 0);

		switch (ev->xbutton.button) {
		case Button1:
		case Button3:
		    rxvtlib_selection_make (o, ev->xbutton.time);
		    break;
		case Button2:
		    rxvtlib_selection_request (o, ev->xbutton.time, ev->xbutton.x, ev->xbutton.y);
		    break;
#ifndef NO_MOUSE_WHEEL
		case Button4:
		case Button5:
		    {
			int i, v;

			i = (ev->xbutton.state & ShiftMask) ? 1 : 5;
			v = (ev->xbutton.button == Button4) ? UP : DN;
# ifdef JUMP_MOUSE_WHEEL
			rxvtlib_scr_page (o, v, i);
			rxvtlib_scr_refresh (o, SMOOTH_REFRESH);
			rxvtlib_scrollbar_show (o, 1);
# else
			for (; i--;) {
			    rxvtlib_scr_page (o, v, 1);
			    rxvtlib_scr_refresh (o, SMOOTH_REFRESH);
			    rxvtlib_scrollbar_show (o, 1);
			}
# endif
		    }
		    break;
#endif
		}
	    }
	} else if (isMenuBarWindow (ev->xany.window)) {
	    rxvtlib_menubar_control (o, &(ev->xbutton));
	}
	break;

    case MotionNotify:
	if (isMenuBarWindow (ev->xany.window)) {
	    rxvtlib_menubar_control (o, &(ev->xbutton));
	    break;
	}
	if ((o->PrivateModes & PrivMode_mouse_report) && !(bypass_keystate))
	    break;

	if (ev->xany.window == o->TermWin.vt) {
	    if ((ev->xbutton.state & (Button1Mask | Button3Mask))) {
		while (XCheckTypedWindowEvent (o->Xdisplay, o->TermWin.vt, MotionNotify, ev));
		XQueryPointer (o->Xdisplay, o->TermWin.vt,
			       &unused_root, &unused_child,
			       &unused_root_x, &unused_root_y,
			       &(ev->xbutton.x), &(ev->xbutton.y), &unused_mask);
#ifdef MOUSE_THRESHOLD
		/* deal with a `jumpy' mouse */
		if ((ev->xmotion.time - o->MEvent.time) > MOUSE_THRESHOLD)
#endif
		    rxvtlib_selection_extend (o, (ev->xbutton.x), (ev->xbutton.y),
					      (ev->xbutton.state & Button3Mask) ? 2 : 0);
	    }
	} else if (isScrollbarWindow (ev->xany.window)
		   && scrollbar_isMotion ()) {
	    while (XCheckTypedWindowEvent (o->Xdisplay, o->scrollBar.win, MotionNotify, ev));
	    XQueryPointer (o->Xdisplay, o->scrollBar.win, &unused_root,
			   &unused_child, &unused_root_x, &unused_root_y,
			   &(ev->xbutton.x), &(ev->xbutton.y), &unused_mask);
	    rxvtlib_scr_move_to (o, scrollbar_position (ev->xbutton.y) - csrO, scrollbar_size ());
	    rxvtlib_scr_refresh (o, o->refresh_type);
	    o->refresh_count = o->refresh_limit = 0;
	    rxvtlib_scrollbar_show (o, 1);
#ifdef USE_XIM
	    rxvtlib_IMSendSpot (o);
#endif
	}
	break;
    }
}

#ifdef TRANSPARENT
/*
 * Check our parents are still who we think they are.
 */
/* INTPROTO */
int             rxvtlib_check_our_parents (rxvtlib *o)
{E_
    int             i, pchanged;
    unsigned int    n;
    Window          root, oldp, *list;

/* Get all X ops out of the queue so that our information is up-to-date. */
    XSync (o->Xdisplay, False);

/*
 * Make the frame window set by the window manager have
 * the root background. Some window managers put multiple nested frame
 * windows for each client, so we have to take care about that.
 */
    pchanged = 0;
    for (i = 1; i < KNOW_PARENTS; i++) {
	oldp = o->TermWin.parent[i];
	XQueryTree (o->Xdisplay, o->TermWin.parent[i - 1], &root,
		    &o->TermWin.parent[i], &list, &n);
	XFree (list);
	if (o->TermWin.parent[i] == Xroot) {
	    if (oldp != None)
		pchanged = 1;
	    break;
	}
	if (oldp != o->TermWin.parent[i])
	    pchanged = 1;
    }
    n = 0;
    if (pchanged) {
	XWindowAttributes wattr;
	int             d;

	XGetWindowAttributes (o->Xdisplay, Xroot, &wattr);
	d = wattr.depth;
	for (n = 0; n < i; n++) {
	    XGetWindowAttributes (o->Xdisplay, o->TermWin.parent[n], &wattr);
	    if (wattr.depth != d || wattr.class == InputOnly) {
		n = KNOW_PARENTS + 1;
		break;
	    }
	}
    }
    if (n > KNOW_PARENTS) {
	XSetWindowBackground (o->Xdisplay, o->TermWin.parent[0],
			      o->PixColors[Color_fg]);
	XSetWindowBackground (o->Xdisplay, o->TermWin.vt, o->PixColors[Color_bg]);
	/* XXX: also turn off Opt_transparent? */
    } else
	for (n = 0; n < i; n++)
	    XSetWindowBackgroundPixmap (o->Xdisplay, o->TermWin.parent[n],
					ParentRelative);

    for (; i < KNOW_PARENTS; i++)
	o->TermWin.parent[i] = None;
    return pchanged;
}
#endif

/*}}} */

/*
 * Send printf() formatted output to the command.
 * Only use for small ammounts of data.
 */
/* EXTPROTO */
void            rxvtlib_tt_printf (rxvtlib *o, const char *fmt, ...)
{E_
    va_list         arg_ptr;
    unsigned char   buf[256];

    va_start (arg_ptr, fmt);
    vsprintf ((char *) buf, fmt, arg_ptr);
    va_end (arg_ptr);
    rxvtlib_tt_write (o, buf, strlen ((char *) buf));
}

/*{{{ print pipe */
/*----------------------------------------------------------------------*/
#ifdef PRINTPIPE
/* EXTPROTO */
FILE           *rxvtlib_popen_printer (rxvtlib *o)
{E_
    FILE           *stream = (FILE *) popen (o->rs[Rs_print_pipe], "w");

    if (stream == NULL)
	print_error ("can't open printer pipe");
    return stream;
}

/* EXTPROTO */
int             pclose_printer (FILE * stream)
{E_
    fflush (stream);
/* pclose() reported not to work on SunOS 4.1.3 */
# if defined (__sun__)		/* TODO: RESOLVE THIS */
/* pclose works provided SIGCHLD handler uses waitpid */
    return pclose (stream);	/* return fclose (stream); */
# else
    return pclose (stream);
# endif
}

/*
 * simulate attached vt100 printer
 */
/* INTPROTO */
void            rxvtlib_process_print_pipe (rxvtlib *o)
{E_
    int             done;
    FILE           *fd;

    if ((fd = rxvtlib_popen_printer (o)) == NULL)
	return;

/* 
 * Send all input to the printer until either ESC[4i or ESC[?4i 
 * is received. 
 */
    for (done = 0; !done && !o->killed;) {
	unsigned char   buf[8];
	unsigned char   ch;
	unsigned int    i, len;

	if ((ch = rxvtlib_cmd_getc (o)) != '\033') {
	    if (putc (ch, fd) == EOF)
		break;		/* done = 1 */
	} else {
	    len = 0;
	    buf[len++] = ch;

	    if ((buf[len++] = rxvtlib_cmd_getc (o)) == '[') {
		if ((ch = rxvtlib_cmd_getc (o)) == '?') {
		    buf[len++] = '?';
		    ch = rxvtlib_cmd_getc (o);
		}
		if ((buf[len++] = ch) == '4') {
		    if ((buf[len++] = rxvtlib_cmd_getc (o)) == 'i')
			break;	/* done = 1 */
		}
	    }
	    for (i = 0; i < len; i++)
		if (putc (buf[i], fd) == EOF) {
		    done = 1;
		    break;
		}
	}
    }
    pclose_printer (fd);
}
#endif				/* PRINTPIPE */
/*}}} */

/*{{{ process escape sequences */
/* INTPROTO */
void            rxvtlib_process_escape_seq (rxvtlib *o)
{E_
    unsigned char   ch = rxvtlib_cmd_getc (o);

    switch (ch) {
	/* case 1:        do_tek_mode (); break; */
    case '#':
	if (rxvtlib_cmd_getc (o) == '8')
	    rxvtlib_scr_E (o);
	break;
    case '(':
	rxvtlib_scr_charset_set (o, 0, rxvtlib_cmd_getc (o));
	break;
    case ')':
	rxvtlib_scr_charset_set (o, 1, rxvtlib_cmd_getc (o));
	break;
    case '*':
	rxvtlib_scr_charset_set (o, 2, rxvtlib_cmd_getc (o));
	break;
    case '+':
	rxvtlib_scr_charset_set (o, 3, rxvtlib_cmd_getc (o));
	break;
#ifdef MULTICHAR_SET
    case '$':
	rxvtlib_scr_charset_set (o, -2, rxvtlib_cmd_getc (o));
	break;
#endif
    case '6':
#ifndef NO_FRILLS
	rxvtlib_scr_backindex (o);
#endif
	break;
    case '7':
	rxvtlib_scr_cursor (o, SAVE);
	break;
    case '8':
	rxvtlib_scr_cursor (o, RESTORE);
	break;
    case '9':
#ifndef NO_FRILLS
	rxvtlib_scr_forwardindex (o);
#endif
	break;
    case '=':
    case '>':
	PrivMode ((ch == '='), PrivMode_aplKP);
	break;
    case '@':
	(void)rxvtlib_cmd_getc (o);
	break;
    case 'D':
	rxvtlib_scr_index (o, UP);
	break;
    case 'E':
	rxvtlib_scr_add_lines (o, (const unsigned char *)"\n\r", 1, 2);
	break;
    case 'G':
	rxvtlib_process_graphics (o);
	break;
    case 'H':
	rxvtlib_scr_set_tab (o, 1);
	break;
    case 'M':
	rxvtlib_scr_index (o, DN);
	break;
	/*case 'N': scr_single_shift (2);   break; */
	/*case 'O': scr_single_shift (3);   break; */
    case 'Z':
	rxvtlib_tt_printf (o, ESCZ_ANSWER);
	break;			/* steal obsolete ESC [ c */
    case '[':
	rxvtlib_process_csi_seq (o);
	break;
    case ']':
	rxvtlib_process_xterm_seq (o);
	break;
    case 'c':
	rxvtlib_scr_poweron (o);
	break;
    case 'n':
	rxvtlib_scr_charset_choose (o, 2);
	break;
    case 'o':
	rxvtlib_scr_charset_choose (o, 3);
	break;
    }
}
/*}}} */

/*{{{ process CSI (code sequence introducer) sequences `ESC[' */
/* INTPROTO */
void            rxvtlib_process_csi_seq (rxvtlib *o)
{E_
    unsigned char   ch, priv;
    unsigned int    nargs;
    int             arg[ESC_ARGS];

    for (nargs = ESC_ARGS; nargs > 0;)
	arg[--nargs] = 0;

    priv = 0;
    ch = rxvtlib_cmd_getc (o);
    if (ch >= '<' && ch <= '?') {
	priv = ch;
	ch = rxvtlib_cmd_getc (o);
    }
/* read any numerical arguments */
    do {
	int             n = 0;

	if (isdigit (ch)) {
	    for (; isdigit (ch); ch = rxvtlib_cmd_getc (o))
		n = n * 10 + (ch - '0');
	    if (nargs < ESC_ARGS)
		arg[nargs++] = n;
	}
	if (ch == '\b') {
	    rxvtlib_scr_backspace (o);
	} else if (ch == 033) {
	    rxvtlib_process_escape_seq (o);
	    return;
	} else if (ch < ' ') {
	    rxvtlib_scr_add_lines (o, &ch, 0, 1);
	    return;
	}
	if (ch < '@')
	    ch = rxvtlib_cmd_getc (o);
    } while (ch >= ' ' && ch < '@');
    if (ch == 033) {
	rxvtlib_process_escape_seq (o);
	return;
    } else if (ch < ' ')
	return;

    switch (ch) {
#ifdef PRINTPIPE
    case 'i':			/* printing */
	switch (arg[0]) {
	case 0:
	    rxvtlib_scr_printscreen (o, 0);
	    break;
	case 5:
	    rxvtlib_process_print_pipe (o);
	    break;
	}
	break;
#endif
    case 'A':
    case 'e':			/* up <n> */
	rxvtlib_scr_gotorc (o, arg[0] ? -arg[0] : -1, 0, RELATIVE);
	break;
    case 'B':			/* down <n> */
	rxvtlib_scr_gotorc (o, arg[0] ? arg[0] : 1, 0, RELATIVE);
	break;
    case 'C':
    case 'a':			/* right <n> */
	rxvtlib_scr_gotorc (o, 0, arg[0] ? arg[0] : 1, RELATIVE);
	break;
    case 'D':			/* left <n> */
	rxvtlib_scr_gotorc (o, 0, arg[0] ? -arg[0] : -1, RELATIVE);
	break;
    case 'E':			/* down <n> & to first column */
	rxvtlib_scr_gotorc (o, arg[0] ? arg[0] : 1, 0, R_RELATIVE);
	break;
    case 'F':			/* up <n> & to first column */
	rxvtlib_scr_gotorc (o, arg[0] ? -arg[0] : -1, 0, R_RELATIVE);
	break;
    case 'G':
    case '`':			/* move to col <n> */
	rxvtlib_scr_gotorc (o, 0, arg[0] ? arg[0] - 1 : 1, R_RELATIVE);
	break;
    case 'd':			/* move to row <n> */
	rxvtlib_scr_gotorc (o, arg[0] ? arg[0] - 1 : 1, 0, C_RELATIVE);
	break;
    case 'H':
    case 'f':			/* position cursor */
	switch (nargs) {
	case 0:
	    rxvtlib_scr_gotorc (o, 0, 0, 0);
	    break;
	case 1:
	    rxvtlib_scr_gotorc (o, arg[0] ? arg[0] - 1 : 0, 0, 0);
	    break;
	default:
	    rxvtlib_scr_gotorc (o, arg[0] - 1, arg[1] - 1, 0);
	    break;
	}
	break;
    case 'I':
	rxvtlib_scr_tab (o, arg[0] ? arg[0] : 1);
	break;
    case 'Z':
	rxvtlib_scr_tab (o, arg[0] ? -arg[0] : -1);
	break;
    case 'J':
	rxvtlib_scr_erase_screen (o, arg[0]);
	break;
    case 'K':
	rxvtlib_scr_erase_line (o, arg[0]);
	break;
    case '@':
	rxvtlib_scr_insdel_chars (o, arg[0] ? arg[0] : 1, INSERT);
	break;
    case 'L':
	rxvtlib_scr_insdel_lines (o, arg[0] ? arg[0] : 1, INSERT);
	break;
    case 'M':
	rxvtlib_scr_insdel_lines (o, arg[0] ? arg[0] : 1, DELETE);
	break;
    case 'X':
	rxvtlib_scr_insdel_chars (o, arg[0] ? arg[0] : 1, ERASE);
	break;
    case 'P':
	rxvtlib_scr_insdel_chars (o, arg[0] ? arg[0] : 1, DELETE);
	break;
    case 'T':
    case '^':
	rxvtlib_scr_scroll_text (o, arg[0] ? -arg[0] : -1);
	break;
    case 'S':
	rxvtlib_scr_scroll_text (o, arg[0] ? arg[0] : 1);
	break;
    case 'c':
	rxvtlib_tt_printf (o, VT100_ANS);
	break;
    case 'm':
	rxvtlib_process_sgr_mode (o, nargs, arg);
	break;
    case 'n':			/* request for information */
	switch (arg[0]) {
	case 5:
	    rxvtlib_tt_printf (o, "\033[0n");
	    break;		/* ready */
	case 6:
	    rxvtlib_scr_report_position (o);
	    break;
#if defined (ENABLE_DISPLAY_ANSWER)
	case 7:
	    rxvtlib_tt_printf (o, "%s\n", o->rs[Rs_display_name]);
	    break;
#endif
	case 8:
	    rxvtlib_xterm_seq (o, XTerm_title, APL_NAME "-" VERSION);
	    break;
	}
	break;
    case 'r':			/* set top and bottom margins */
	if (priv != '?') {
	    if (nargs < 2 || arg[0] >= arg[1])
		rxvtlib_scr_scroll_region (o, 0, 10000);
	    else
		rxvtlib_scr_scroll_region (o, arg[0] - 1, arg[1] - 1);
	    break;
	}
	/* FALLTHROUGH */
    case 's':
    case 'h':
    case 'l':
	rxvtlib_process_terminal_mode (o, ch, priv, nargs, arg);
	break;
    case 't':
	if (priv)
	    rxvtlib_process_terminal_mode (o, ch, priv, nargs, arg);
#ifndef NO_FRILLS
	else
	    rxvtlib_process_window_ops (o, arg, nargs);
#endif
	break;
    case 'g':
	switch (arg[0]) {
	case 0:
	    rxvtlib_scr_set_tab (o, 0);
	    break;		/* delete tab */
	case 3:
	    rxvtlib_scr_set_tab (o, -1);
	    break;		/* clear all tabs */
	}
	break;
    case 'W':
	switch (arg[0]) {
	case 0:
	    rxvtlib_scr_set_tab (o, 1);
	    break;		/* = ESC H */
	case 2:
	    rxvtlib_scr_set_tab (o, 0);
	    break;		/* = ESC [ 0 g */
	case 5:
	    rxvtlib_scr_set_tab (o, -1);
	    break;		/* = ESC [ 3 g */
	}
	break;
    }
}
/*}}} */

#ifndef NO_FRILLS
/* ARGSUSED */
/* INTPROTO */
void            rxvtlib_process_window_ops (rxvtlib *o, const int *args, int nargs)
{E_
    int             x, y;
    char           *s;
    XWindowAttributes wattr;
    Window          wdummy;

    if (nargs == 0)
	return;
    switch (args[0]) {
	/*
	 * commands
	 */
    case 1:			/* deiconify window */
	XMapWindow (o->Xdisplay, o->TermWin.parent[0]);
	break;
    case 2:			/* iconify window */
	XIconifyWindow (o->Xdisplay, o->TermWin.parent[0], DefaultScreen (o->Xdisplay));
	break;
    case 3:			/* set position (pixels) */
	rxvtlib_AddToCNQueue (o, o->szHint.width, o->szHint.height);
	XMoveWindow (o->Xdisplay, o->TermWin.parent[0], args[1], args[2]);
	break;
    case 4:			/* set size (pixels) */
	rxvtlib_set_widthheight (o, args[2], args[1]);
	break;
    case 5:			/* raise window */
	XRaiseWindow (o->Xdisplay, o->TermWin.parent[0]);
	break;
    case 6:			/* lower window */
	XLowerWindow (o->Xdisplay, o->TermWin.parent[0]);
	break;
    case 7:			/* refresh window */
	rxvtlib_scr_touch (o);
	break;
    case 8:			/* set size (chars) */
	rxvtlib_set_widthheight (o, args[2] * o->TermWin.fwidth, args[1] * o->TermWin.fheight);
	break;
    default:
	if (args[0] >= 24)	/* set height (chars) */
	    rxvtlib_set_widthheight (o, o->TermWin.width, args[1] * o->TermWin.fheight);
	break;
	/*
	 * reports - some output format copied from XTerm
	 */
    case 11:			/* report window state */
	XGetWindowAttributes (o->Xdisplay, o->TermWin.parent[0], &wattr);
	rxvtlib_tt_printf (o, "\033[%dt", wattr.map_state == IsViewable ? 1 : 2);
	break;
    case 13:			/* report window position */
	XGetWindowAttributes (o->Xdisplay, o->TermWin.parent[0], &wattr);
	XTranslateCoordinates (o->Xdisplay, o->TermWin.parent[0], wattr.root,
			       -wattr.border_width, -wattr.border_width,
			       &x, &y, &wdummy);
	rxvtlib_tt_printf (o, "\033[3;%d;%dt", x, y);
	break;
    case 14:			/* report window size (pixels) */
	XGetWindowAttributes (o->Xdisplay, o->TermWin.parent[0], &wattr);
	rxvtlib_tt_printf (o, "\033[4;%d;%dt", wattr.height, wattr.width);
	break;
    case 18:			/* report window size (chars) */
	rxvtlib_tt_printf (o, "\033[8;%d;%dt", o->TermWin.nrow, o->TermWin.ncol);
	break;
    case 20:			/* report icon label */
	XGetIconName (o->Xdisplay, o->TermWin.parent[0], &s);
	rxvtlib_tt_printf (o, "\033]L%s\033\\", s ? s : "");
	break;
    case 21:			/* report window title */
	XFetchName (o->Xdisplay, o->TermWin.parent[0], &s);
	rxvtlib_tt_printf (o, "\033]l%s\033\\", s ? s : "");
	break;
    }
}
#endif

/*{{{ process xterm text parameters sequences `ESC ] Ps ; Pt BEL' */
/* INTPROTO */
void            rxvtlib_process_xterm_seq (rxvtlib *o)
{E_
    unsigned char   ch, string[STRING_MAX];
    int             arg;

    ch = rxvtlib_cmd_getc (o);
    for (arg = 0; isdigit (ch); ch = rxvtlib_cmd_getc (o))
	arg = arg * 10 + (ch - '0');

    if (ch == ';') {
	int             n = 0;

	while ((ch = rxvtlib_cmd_getc (o)) != 007) {
	    if (ch) {
		if (ch == '\t')
		    ch = ' ';	/* translate '\t' to space */
		else if (ch < ' ')
		    return;	/* control character - exit */

		if (n < sizeof (string) - 1)
		    string[n++] = ch;
	    }
	}
	string[n] = '\0';
	/*
	 * menubar_dispatch() violates the constness of the string,
	 * so do it here
	 */
	if (arg == XTerm_Menu)
	    rxvtlib_menubar_dispatch (o, (char *) string);
	else
	    rxvtlib_xterm_seq (o, arg, (char *)string);
    }
}

/*}}} */

/*{{{ process DEC private mode sequences `ESC [ ? Ps mode' */
/*
 * mode can only have the following values:
 *      'l' = low
 *      'h' = high
 *      's' = save
 *      'r' = restore
 *      't' = toggle
 * so no need for fancy checking
 */
/* INTPROTO */
void            rxvtlib_process_terminal_mode (rxvtlib *o, int mode, int priv, unsigned int nargs,
				       const int *arg)
{E_
    unsigned int    i;
    int             state;

    if (nargs == 0)
	return;

/* make lo/hi boolean */
    if (mode == 'l')
	mode = 0;
    else if (mode == 'h')
	mode = 1;

    switch (priv) {
    case 0:
	if (mode && mode != 1)
	    return;		/* only do high/low */
	for (i = 0; i < nargs; i++)
	    switch (arg[i]) {
	    case 4:
		rxvtlib_scr_insert_mode (o, mode);
		break;
		/* case 38:  TEK mode */
	    }
	break;

    case '?':
	for (i = 0; i < nargs; i++)
	    switch (arg[i]) {
	    case 1:		/* application cursor keys */
		PrivCases (PrivMode_aplCUR);
		break;

		/* case 2:   - reset charsets to USASCII */

	    case 3:		/* 80/132 */
		PrivCases (PrivMode_132);
		if (o->PrivateModes & PrivMode_132OK)
		    rxvtlib_set_widthheight (o, (state ? 132 : 80) * o->TermWin.fwidth,
				     o->TermWin.height);
		break;

		/* case 4:   - smooth scrolling */

	    case 5:		/* reverse video */
		PrivCases (PrivMode_rVideo);
		rxvtlib_scr_rvideo_mode (o, state);
		break;

	    case 6:		/* relative/absolute origins  */
		PrivCases (PrivMode_relOrigin);
		rxvtlib_scr_relative_origin (o, state);
		break;

	    case 7:		/* autowrap */
		PrivCases (PrivMode_Autowrap);
		rxvtlib_scr_autowrap (o, state);
		break;

		/* case 8:   - auto repeat, can't do on a per window basis */

	    case 9:		/* X10 mouse reporting */
		PrivCases (PrivMode_MouseX10);
		/* orthogonal */
		if (o->PrivateModes & PrivMode_MouseX10)
		    o->PrivateModes &= ~(PrivMode_MouseX11);
		break;
#ifdef menuBar_esc
	    case menuBar_esc:
		PrivCases (PrivMode_menuBar);
		rxvtlib_map_menuBar (o, state);
		break;
#endif
#ifdef scrollBar_esc
	    case scrollBar_esc:
		PrivCases (PrivMode_scrollBar);
		rxvtlib_map_scrollBar (o, state);
		break;
#endif
	    case 25:		/* visible/invisible cursor */
		PrivCases (PrivMode_VisibleCursor);
		rxvtlib_scr_cursor_visible (o, state);
		break;

	    case 35:
		PrivCases (PrivMode_ShiftKeys);
		break;

	    case 40:		/* 80 <--> 132 mode */
		PrivCases (PrivMode_132OK);
		break;

	    case 47:		/* secondary screen */
		PrivCases (PrivMode_Screen);
		rxvtlib_scr_change_screen (o, state);
		break;

	    case 66:		/* application key pad */
		PrivCases (PrivMode_aplKP);
		break;

	    case 67:
#ifndef NO_BACKSPACE_KEY
		if (o->PrivateModes & PrivMode_HaveBackSpace) {
		    PrivCases (PrivMode_BackSpace);
		}
#endif
		break;

	    case 1000:		/* X11 mouse reporting */
		PrivCases (PrivMode_MouseX11);
		/* orthogonal */
		if (o->PrivateModes & PrivMode_MouseX11)
		    o->PrivateModes &= ~(PrivMode_MouseX10);
		break;
#if 0
	    case 1001:
		break;		/* X11 mouse highlighting */
#endif
	    case 1010:		/* scroll to bottom on TTY output inhibit */
		PrivCases (PrivMode_TtyOutputInh);
		if (o->PrivateModes & PrivMode_TtyOutputInh)
		    o->Options &= ~Opt_scrollTtyOutput;
		else
		    o->Options |= Opt_scrollTtyOutput;
		break;

	    case 1011:		/* scroll to bottom on key press */
		PrivCases (PrivMode_Keypress);
		if (o->PrivateModes & PrivMode_Keypress)
		    o->Options |= Opt_scrollKeypress;
		else
		    o->Options &= ~Opt_scrollKeypress;
		break;

	    default:
		break;
	    }
	break;
    }
}
/*}}} */

/*{{{ process sgr sequences */
/* INTPROTO */
void            rxvtlib_process_sgr_mode (rxvtlib *o, unsigned int nargs, const int *arg)
{E_
    unsigned int    i;

    if (nargs == 0) {
	rxvtlib_scr_rendition (o, 0, ~RS_None);
	return;
    }
    for (i = 0; i < nargs; i++)
	switch (arg[i]) {
	case 0:
	    rxvtlib_scr_rendition (o, 0, ~RS_None);
	    break;
	case 1:
	    rxvtlib_scr_rendition (o, 1, RS_Bold);
	    break;
	case 4:
	    rxvtlib_scr_rendition (o, 1, RS_Uline);
	    break;
	case 5:
	    rxvtlib_scr_rendition (o, 1, RS_Blink);
	    break;
	case 7:
	    rxvtlib_scr_rendition (o, 1, RS_RVid);
	    break;
	case 22:
	    rxvtlib_scr_rendition (o, 0, RS_Bold);
	    break;
	case 24:
	    rxvtlib_scr_rendition (o, 0, RS_Uline);
	    break;
	case 25:
	    rxvtlib_scr_rendition (o, 0, RS_Blink);
	    break;
	case 27:
	    rxvtlib_scr_rendition (o, 0, RS_RVid);
	    break;

	case 30:
	case 31:		/* set fg color */
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	    rxvtlib_scr_color (o, minCOLOR + (arg[i] - 30), RS_Bold);
	    break;
	case 39:		/* default fg */
	    rxvtlib_scr_color (o, restoreFG, RS_Bold);
	    break;

	case 40:
	case 41:		/* set bg color */
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	    rxvtlib_scr_color (o, minCOLOR + (arg[i] - 40), RS_Blink);
	    break;
	case 49:		/* default bg */
	    rxvtlib_scr_color (o, restoreBG, RS_Blink);
	    break;
	}
}
/*}}} */

/*{{{ process Rob Nation's own graphics mode sequences */
/* INTPROTO */
void            rxvtlib_process_graphics (rxvtlib *o)
{E_
    unsigned char   ch, cmd = rxvtlib_cmd_getc (o);

#ifndef RXVT_GRAPHICS
    if (cmd == 'Q') {		/* query graphics */
	rxvtlib_tt_printf (o, "\033G0\n");	/* no graphics */
	return;
    }
/* swallow other graphics sequences until terminating ':' */
    do
	ch = rxvtlib_cmd_getc (o);
    while (ch != ':');
#else
    int             nargs;
    int             args[NGRX_PTS];
    unsigned char  *text = NULL;

    if (cmd == 'Q') {		/* query graphics */
	rxvtlib_tt_printf (o, "\033G1\n");	/* yes, graphics (color) */
	return;
    }
    for (nargs = 0; nargs < (sizeof (args) / sizeof (args[0])) - 1;) {
	int             neg;

	ch = rxvtlib_cmd_getc (o);
	neg = (ch == '-');
	if (neg || ch == '+')
	    ch = rxvtlib_cmd_getc (o);

	for (args[nargs] = 0; isdigit (ch); ch = rxvtlib_cmd_getc (o))
	    args[nargs] = args[nargs] * 10 + (ch - '0');
	if (neg)
	    args[nargs] = -args[nargs];

	nargs++;
	args[nargs] = 0;
	if (ch != ';')
	    break;
    }

    if ((cmd == 'T') && (nargs >= 5)) {
	int             i, len = args[4];

	text = MALLOC ((len + 1) * sizeof (char));

	if (text != NULL) {
	    for (i = 0; i < len; i++)
		text[i] = rxvtlib_cmd_getc (o);
	    text[len] = '\0';
	}
    }
    rxvtlib_Gr_do_graphics (o, cmd, nargs, args, text);
# ifdef USE_XIM
    rxvtlib_IMSendSpot (o);
# endif
#endif
}
/*}}} */

/* ------------------------------------------------------------------------- */
/*
 * A simple queue to hold ConfigureNotify events we've generated so we can
 * bypass them when they come in.  Don't bother keeping a pointer to the tail
 * since we don't expect masses of CNs at any one time.
 */
/* EXTPROTO */
void            rxvtlib_AddToCNQueue (rxvtlib *o, int width, int height)
{E_
    XCNQueue_t     *rq, *nrq;

    nrq = (XCNQueue_t *) MALLOC (sizeof (XCNQueue_t));
    assert (nrq);
    nrq->next = NULL;
    nrq->width = width;
    nrq->height = height;
    if (o->XCNQueue == NULL)
	o->XCNQueue = nrq;
    else {
	for (rq = o->XCNQueue; rq->next; rq = rq->next)
	    /* nothing */ ;
	rq->next = nrq;
    }
}

/* INTPROTO */
int             rxvtlib_RemoveFromCNQueue (rxvtlib *o, int width, int height)
{E_
    XCNQueue_t     *rq, *prq;
    int		    new_ncol, new_nrow;

/*
 * If things are working properly we should only need to check the first one
 */
    for (rq = o->XCNQueue, prq = NULL; rq; rq = rq->next) {
	if (rq->width == width && rq->height == height) {
#if 0
	    /* unlink rq */
	    if (prq)
		prq->next = rq->next;
	    else
		o->XCNQueue = rq->next;
	    FREE (rq);
	    return 1;
#else
	    new_ncol = (width - o->szHint.base_width) / o->TermWin.fwidth;
	    new_nrow = (height - o->szHint.base_height) / o->TermWin.fheight;
	    if (new_ncol == o->TermWin.ncol && new_nrow == o->TermWin.nrow) {
	    /* unlink rq */
		if (prq)
		    prq->next = rq->next;
		else
		    o->XCNQueue = rq->next;
		FREE(rq);
		return 1;
	    }
#endif
	}
	prq = rq;
    }
    return 0;
}
/* ------------------------------------------------------------------------- */

/*{{{ Read and process output from the application */
/* EXTPROTO */
void rxvtlib_main_loop (rxvtlib * o)
{E_
    int nlines;
    unsigned char ch, *str;

    while (!o->killed) {
#ifdef STANDALONE
	while ((ch = rxvtlib_cmd_getc (o)) == 0 && !o->killed);	/* wait for something */
	if (o->killed)
	    return;
#else
	if (o->cmdbuf_ptr >= o->cmdbuf_endp) {
	    CAddWatch (o->cmd_fd, rxvt_fd_read_watch, 1, (void *) o);
	    return;
	}
	ch = rxvtlib_cmd_getc (o);
#endif

	if (ch >= ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
	    /* Read a text string from the input buffer */
	    /*
	     * point `str' to the start of the string,
	     * decrement first since it was post incremented in cmd_getc()
	     */
	    for (str = --o->cmdbuf_ptr, nlines = 0; o->cmdbuf_ptr < o->cmdbuf_endp;) {
		ch = *o->cmdbuf_ptr++;
		if (ch == '\n') {
		    nlines++;
		    if (++o->refresh_count >= (o->refresh_limit * (o->TermWin.nrow - 1)))
			break;
		} else if (ch < ' ' && ch != '\t' && ch != '\r') {
		    /* unprintable */
		    o->cmdbuf_ptr--;
		    break;
		}
	    }
	    rxvtlib_scr_add_lines (o, str, nlines, (o->cmdbuf_ptr - str));
	} else
	    switch (ch) {
	    case 005:		/* terminal Status */
		rxvtlib_tt_printf (o, VT100_ANS);
		break;
	    case 007:		/* bell */
		rxvtlib_scr_bell (o);
		break;
	    case '\b':		/* backspace */
		rxvtlib_scr_backspace (o);
		break;
	    case 013:		/* vertical tab, form feed */
	    case 014:
		rxvtlib_scr_index (o, UP);
		break;
	    case 016:		/* shift out - acs */
		rxvtlib_scr_charset_choose (o, 1);
		break;
	    case 017:		/* shift in - acs */
		rxvtlib_scr_charset_choose (o, 0);
		break;
	    case 033:		/* escape char */
		rxvtlib_process_escape_seq (o);
		break;
	    }
    }
/* NOTREACHED */
}

#ifndef STANDALONE
void rxvt_fd_write_watch (int fd, fd_set * reading,
				 fd_set * writing, fd_set * error, void *data)
{E_
    int riten, p;
    rxvtlib *o = (rxvtlib *) data;
    p = o->v_bufptr - o->v_bufstr;
    riten = write (o->cmd_fd, o->v_bufstr, p < MAX_PTY_WRITE ? p : MAX_PTY_WRITE);
    if (riten < 0)
	riten = 0;
    o->v_bufstr += riten;
    if (o->v_bufstr >= o->v_bufptr) {	/* we wrote it all */
	o->v_bufstr = o->v_bufptr = o->v_buffer;
	CRemoveWatch (o->cmd_fd, rxvt_fd_write_watch, 2);
    }
    rxvtlib_main_loop (o);
    rxvtlib_update_screen (o);
}
#endif

/* ---------------------------------------------------------------------- */
/* Addresses pasting large amounts of data and rxvt hang
 * code pinched from xterm (v_write()) and applied originally to
 * rxvt-2.18 - Hops
 * Write data to the pty as typed by the user, pasted with the mouse,
 * or generated by us in response to a query ESC sequence.
 */
/* EXTPROTO */
void            rxvtlib_tt_write (rxvtlib *o, const unsigned char *d, int len)
{E_
    int             p;
#ifdef STANDALONE
    int             riten;
#endif

    if (o->v_bufstr == NULL && len > 0) {
	o->v_buffer = o->v_bufstr = o->v_bufptr = MALLOC (len);
	o->v_bufend = o->v_buffer + len;
    }
/*
 * Append to the block we already have.  Always doing this simplifies the
 * code, and isn't too bad, either.  If this is a short block, it isn't
 * too expensive, and if this is a long block, we won't be able to write
 * it all anyway.
 */
    if (len > 0) {
	if (o->v_bufend < o->v_bufptr + len) {	/* we've run out of room */
	    if (o->v_bufstr != o->v_buffer) {
		/* there is unused space, move everything down */
		/* possibly overlapping bcopy here */
		/* bcopy(v_bufstr, v_buffer, v_bufptr - v_bufstr); */
		memcpy (o->v_buffer, o->v_bufstr, o->v_bufptr - o->v_bufstr);
		o->v_bufptr -= o->v_bufstr - o->v_buffer;
		o->v_bufstr = o->v_buffer;
	    }
	    if (o->v_bufend < o->v_bufptr + len) {
		/* still won't fit: get more space */
		/* Don't use XtRealloc because an error is not fatal. */
		int             size = o->v_bufptr - o->v_buffer;

		/* save across realloc */
		o->v_buffer = REALLOC (o->v_buffer, size + len);
		if (o->v_buffer) {
		    o->v_bufstr = o->v_buffer;
		    o->v_bufptr = o->v_buffer + size;
		    o->v_bufend = o->v_bufptr + len;
		} else {
		    /* no memory: ignore entire write request */
		    print_error ("cannot allocate buffer space");
		    o->v_buffer = o->v_bufstr;	/* restore clobbered pointer */
		}
	    }
	}
	if (o->v_bufend >= o->v_bufptr + len) {	/* new stuff will fit */
	    memcpy (o->v_bufptr, d, len);	/* bcopy(d, v_bufptr, len); */
	    o->v_bufptr += len;
	}
    }
/*
 * Write out as much of the buffer as we can.
 * Be careful not to overflow the pty's input silo.
 * We are conservative here and only write a small amount at a time.
 *
 * If we can't push all the data into the pty yet, we expect write
 * to return a non-negative number less than the length requested
 * (if some data written) or -1 and set errno to EAGAIN,
 * EWOULDBLOCK, or EINTR (if no data written).
 *
 * (Not all systems do this, sigh, so the code is actually
 * a little more forgiving.)
 */

    if ((p = o->v_bufptr - o->v_bufstr) > 0) {
#ifdef STANDALONE
	riten =
	    write (o->cmd_fd, o->v_bufstr, p < MAX_PTY_WRITE ? p : MAX_PTY_WRITE);
	if (riten < 0)
	    riten = 0;
	o->v_bufstr += riten;
	if (o->v_bufstr >= o->v_bufptr)	/* we wrote it all */
	    o->v_bufstr = o->v_bufptr = o->v_buffer;
#else
	CAddWatch (o->cmd_fd, rxvt_fd_write_watch, 2, (void *) o);
#endif
    }
#ifndef STANDALONE
    else {
	CRemoveWatch (o->cmd_fd, rxvt_fd_write_watch, 2);
    }
#endif

/*
 * If we have lots of unused memory allocated, return it
 */
    if (o->v_bufend - o->v_bufptr > 1024) {	/* arbitrary hysteresis */
	/* save pointers across realloc */
	int             start = o->v_bufstr - o->v_buffer;
	int             size = o->v_bufptr - o->v_buffer;
	int             allocsize = size ? size : 1;

	o->v_buffer = REALLOC (o->v_buffer, allocsize);
	if (o->v_buffer) {
	    o->v_bufstr = o->v_buffer + start;
	    o->v_bufptr = o->v_buffer + size;
	    o->v_bufend = o->v_buffer + allocsize;
	} else {
	    /* should we print a warning if couldn't return memory? */
	    o->v_buffer = o->v_bufstr - start;	/* restore clobbered pointer */
	}
    }
}

#ifdef USE_XIM
/* INTPROTO */
void            rxvtlib_setSize (rxvtlib *o, XRectangle * size)
{E_
    size->x = TermWin_internalBorder;
    size->y = TermWin_internalBorder;
    size->width = Width2Pixel (o->TermWin.ncol);
    size->height = Height2Pixel (o->TermWin.nrow);
}

/* INTPROTO */
void            rxvtlib_setColor (rxvtlib *o, unsigned int *fg, unsigned int *bg)
{E_
    *fg = o->PixColors[Color_fg];
    *bg = o->PixColors[Color_bg];
}

/* INTPROTO */
void            rxvtlib_IMSendSpot (rxvtlib *o)
{E_
    XPoint          spot;
    XVaNestedList   preedit_attr;
    XIMStyle        input_style;

#warning backport this fix
    memset(&input_style, '\0', sizeof (input_style));

    if (o->Input_Context == NULL || !o->TermWin.focus)
	return;
    else {
	XGetICValues (o->Input_Context, XNInputStyle, &input_style, NULL);
	if (!(input_style & XIMPreeditPosition))
	    return;
    }
    rxvtlib_setPosition (o, &spot);

    preedit_attr = XVaCreateNestedList (0, XNSpotLocation, &spot, NULL);
    XSetICValues (o->Input_Context, XNPreeditAttributes, preedit_attr, NULL);
    XFree (preedit_attr);
}

/* INTPROTO */
#ifndef UTF8_FONT
void            rxvtlib_setTermFontSet (rxvtlib *o)
{E_
    char           *string;
    long            length, i;

#ifdef DEBUG_CMD
    fprintf (stderr, "setTermFontSet()\n");
#endif
    if (o->TermWin.fontset != NULL) {
	XFreeFontSet (o->Xdisplay, o->TermWin.fontset);
	o->TermWin.fontset = NULL;
    }
    length = 0;
    for (i = 0; i < NFONTS; i++) {
	if (o->rs[Rs_font + i])
	    length += strlen (o->rs[Rs_font + i]) + 1;
# ifdef MULTICHAR_SET
	if (o->rs[Rs_mfont + i])
	    length += strlen (o->rs[Rs_mfont + i]) + 1;
# endif
    }
    if (length == 0 || (string = MALLOC (length + 1)) == NULL)
	o->TermWin.fontset = NULL;
    else {
	int             missing_charsetcount;
	char          **missing_charsetlist, *def_string;

	string[0] = '\0';
	for (i = 0; i < NFONTS; i++) {
	    if (o->rs[Rs_font + i]) {
		strcat (string, o->rs[Rs_font + i]);
		strcat (string, ",");
	    }
# ifdef MULTICHAR_SET
	    if (o->rs[Rs_mfont + i]) {
		strcat (string, o->rs[Rs_mfont + i]);
		strcat (string, ",");
	    }
# endif
	}
	string[strlen (string) - 1] = '\0';
	o->TermWin.fontset = XCreateFontSet (o->Xdisplay, string,
					  &missing_charsetlist,
					  &missing_charsetcount, &def_string);
	FREE (string);
    }
}
#endif

/* INTPROTO */
void            rxvtlib_setPreeditArea (rxvtlib *o, XRectangle * preedit_rect,
				XRectangle * status_rect,
				XRectangle * needed_rect)
{E_
    preedit_rect->x = needed_rect->width
	+ (scrollbar_visible () && !(o->Options & Opt_scrollBar_right)
	   ? (SB_WIDTH + o->sb_shadow * 2) : 0);
    preedit_rect->y = Height2Pixel (o->TermWin.nrow - 1)
	+ ((o->menuBar.state == 1) ? menuBar_TotalHeight () : 0);

    preedit_rect->width = Width2Pixel (o->TermWin.ncol + 1) - needed_rect->width
	+ (!(o->Options & Opt_scrollBar_right)
	   ? (SB_WIDTH + o->sb_shadow * 2) : 0);
    preedit_rect->height = Height2Pixel (1);

    status_rect->x = (scrollbar_visible () && !(o->Options & Opt_scrollBar_right))
	? (SB_WIDTH + o->sb_shadow * 2) : 0;
    status_rect->y = Height2Pixel (o->TermWin.nrow - 1)
	+ ((o->menuBar.state == 1) ? menuBar_TotalHeight () : 0);

    status_rect->width = needed_rect->width ? needed_rect->width
	: Width2Pixel (o->TermWin.ncol + 1);
    status_rect->height = Height2Pixel (1);
}

/* INTPROTO */
void            rxvtlib_IMDestroyCallback (XIM xim, XPointer client_data,
				   XPointer call_data)
{E_
    rxvtlib *o;
    o = (rxvtlib *) client_data;
    o->Input_Context = NULL;
    XRegisterIMInstantiateCallback (o->Xdisplay, NULL, NULL, NULL,
				    rxvtlib_IMInstantiateCallback, (XPointer) o);
}

/* INTPROTO */
void            rxvtlib_IMInstantiateCallback (Display * display, XPointer client_data,
				       XPointer call_data)
{E_
    rxvtlib *o;
    char           *p, *s, buf[64], tmp[1024];
    char           *end, *next_s;
    XIM             xim = NULL;
    XIMStyle        input_style = 0;
    XIMStyles      *xim_styles = NULL;
    int             found;
    XPoint          spot;
    XRectangle      rect, status_rect, needed_rect;
    unsigned int    fg, bg;
    XVaNestedList   preedit_attr = NULL;
    XVaNestedList   status_attr = NULL;
    XIMCallback     ximcallback;
    XFontSet        fontset;

    o = (rxvtlib *) client_data;

    if (o->Input_Context)
	return;

    ximcallback.callback = rxvtlib_IMDestroyCallback;
    ximcallback.client_data = (XPointer) o;

    if (o->rs[Rs_inputMethod] && *o->rs[Rs_inputMethod]) {
	STRNCPY (tmp, o->rs[Rs_inputMethod], sizeof (tmp) - 1);
	for (s = tmp; *s; s = next_s + 1) {
	    for (; *s && isspace (*s); s++) ;
	    if (!*s)
		break;
	    for (end = s; (*end && (*end != ',')); end++) ;
	    for (next_s = end--; ((end >= s) && isspace (*end)); end--) ;
	    *(end + 1) = '\0';

	    if (*s) {
		STRCPY (buf, "@im=");
		strncat (buf, s, sizeof (buf) - 4 - 1);
		if ((p = XSetLocaleModifiers (buf)) != NULL && *p
		    && (xim = XOpenIM (o->Xdisplay, NULL, NULL, NULL)) != NULL)
		    break;
	    }
	    if (!*next_s)
		break;
	}
    }

    /* try with XMODIFIERS env. var. */
    if (xim == NULL && (p = XSetLocaleModifiers ("")) != NULL && *p)
	xim = XOpenIM (o->Xdisplay, NULL, NULL, NULL);

    /* try with no modifiers base */
    if (xim == NULL && (p = XSetLocaleModifiers ("@im=none")) != NULL && *p)
	xim = XOpenIM (o->Xdisplay, NULL, NULL, NULL);

    if (xim == NULL)
	return;
    XSetIMValues (xim, XNDestroyCallback, &ximcallback, NULL);

    if (XGetIMValues (xim, XNQueryInputStyle, &xim_styles, NULL)
	|| !xim_styles) {
	print_error ("input method doesn't support any style");
	XCloseIM (xim);
	return;
    }
    STRNCPY (tmp, (o->rs[Rs_preeditType] ? o->rs[Rs_preeditType]
		   : "OverTheSpot,OffTheSpot,Root"), sizeof (tmp) - 1);
    for (found = 0, s = tmp; *s && !found; s = next_s + 1) {
	unsigned short  i;

	for (; *s && isspace (*s); s++) ;
	if (!*s)
	    break;
	for (end = s; (*end && (*end != ',')); end++) ;
	for (next_s = end--; ((end >= s) && isspace (*end)); end--) ;
	*(end + 1) = '\0';

	if (!strcmp (s, "OverTheSpot"))
	    input_style = (XIMPreeditPosition | XIMStatusNothing);
	else if (!strcmp (s, "OffTheSpot"))
	    input_style = (XIMPreeditArea | XIMStatusArea);
	else if (!strcmp (s, "Root"))
	    input_style = (XIMPreeditNothing | XIMStatusNothing);

	for (i = 0; i < xim_styles->count_styles; i++)
	    if (input_style == xim_styles->supported_styles[i]) {
		found = 1;
		break;
	    }
    }
    XFree (xim_styles);

    if (found == 0) {
	print_error ("input method doesn't support my preedit type");
	XCloseIM (xim);
	return;
    }
    if ((input_style != (XIMPreeditNothing | XIMStatusNothing))
	&& (input_style != (XIMPreeditArea | XIMStatusArea))
	&& (input_style != (XIMPreeditPosition | XIMStatusNothing))) {
	print_error ("This program does not support the preedit type");
	XCloseIM (xim);
	return;
    }

#ifdef UTF8_FONT
#warning finish
    fontset = 0;
#else
    fontset = o->TermWin.fontset;
#endif

    if (input_style & XIMPreeditPosition) {
	rxvtlib_setSize (o, &rect);
	rxvtlib_setPosition (o, &spot);
	rxvtlib_setColor (o, &fg, &bg);

	preedit_attr = XVaCreateNestedList (0, XNArea, &rect,
					    XNSpotLocation, &spot,
					    XNForeground, fg,
					    XNBackground, bg,
					    XNFontSet, fontset, NULL);
    } else if (input_style & XIMPreeditArea) {
	rxvtlib_setColor (o, &fg, &bg);

	/* 
	 * The necessary width of preedit area is unknown
	 * until create input context.
	 */
	needed_rect.width = 0;

	rxvtlib_setPreeditArea (o, &rect, &status_rect, &needed_rect);

	preedit_attr = XVaCreateNestedList (0, XNArea, &rect,
					    XNForeground, fg,
					    XNBackground, bg,
					    XNFontSet, fontset, NULL);
	status_attr = XVaCreateNestedList (0, XNArea, &status_rect,
					   XNForeground, fg,
					   XNBackground, bg,
					   XNFontSet, fontset, NULL);
    }
    o->Input_Context = XCreateIC (xim, XNInputStyle, input_style,
			       XNClientWindow, o->TermWin.parent[0],
			       XNFocusWindow, o->TermWin.parent[0],
			       XNDestroyCallback, &ximcallback,
			       preedit_attr ? XNPreeditAttributes : NULL,
			       preedit_attr,
			       status_attr ? XNStatusAttributes : NULL,
			       status_attr, NULL);
    XFree (preedit_attr);
    XFree (status_attr);
    if (o->Input_Context == NULL) {
	print_error ("Failed to create input context");
	XCloseIM (xim);
    }
    if (input_style & XIMPreeditArea)
	rxvtlib_IMSetStatusPosition (o);
}

/* EXTPROTO */
void            rxvtlib_IMSetStatusPosition (rxvtlib *o)
{E_
    XIMStyle        input_style;
    XRectangle      preedit_rect, status_rect, *needed_rect;
    XVaNestedList   preedit_attr, status_attr;

#warning backport this fix
    memset(&input_style, '\0', sizeof (input_style));

    if (o->Input_Context == NULL)
	return;

    XGetICValues (o->Input_Context, XNInputStyle, &input_style, NULL);

    if (input_style & XIMPreeditArea) {
	/* Getting the necessary width of preedit area */
	status_attr =
	    XVaCreateNestedList (0, XNAreaNeeded, &needed_rect, NULL);
	XGetICValues (o->Input_Context, XNStatusAttributes, status_attr, NULL);
	XFree (status_attr);

	rxvtlib_setPreeditArea (o, &preedit_rect, &status_rect, needed_rect);

	preedit_attr = XVaCreateNestedList (0, XNArea, &preedit_rect, NULL);
	status_attr = XVaCreateNestedList (0, XNArea, &status_rect, NULL);

	XSetICValues (o->Input_Context,
		      XNPreeditAttributes, preedit_attr,
		      XNStatusAttributes, status_attr, NULL);

	XFree (preedit_attr);
	XFree (status_attr);
    }
}
#endif				/* USE_XIM */

/* INTPROTO */
void            rxvtlib_XProcessEvent (rxvtlib *o, Display * display)
{E_
    XEvent          xev;

#ifdef STANDALONE
    XNextEvent (display, &xev);
#else
    memcpy (&xev, &o->xevent, sizeof (xev));
#endif
#ifdef USE_XIM
#ifdef STANDALONE
    if (!XFilterEvent (&xev, xev.xany.window))
#endif
	rxvtlib_process_x_event (o, &xev);
#else
    rxvtlib_process_x_event (o, &xev);
#endif
    return;
}



/*{{{ run_command() */
/*
 * Run the command in a subprocess and return a file descriptor for the
 * master end of the pseudo-teletype pair with the command talking to
 * the slave.
 */
/* INTPROTO */
void            rxvtlib_run_command (rxvtlib *o, const char *const *argv, int do_sleep)
{E_
    ttymode_t       tio;
#if defined (DEBUG_CMD) || defined(STANDALONE)
    int             i;
#endif

    o->cmd_fd = rxvtlib_get_pty (o);
    if (o->cmd_fd < 0)
	return;

/* store original tty status for restoration clean_exit() -- rgg 04/12/95 */
    lstat (o->ttydev, &o->ttyfd_stat);
#ifdef DEBUG_CMD
    fprintf (stderr, "Original settings of %s are mode %o, uid %d, gid %d\n",
	     o->ttydev, o->ttyfd_stat.st_mode, o->ttyfd_stat.st_uid, o->ttyfd_stat.st_gid);
#endif

/* install exit handler for cleanup */
#ifdef HAVE_ATEXIT
    atexit (clean_exit);
#else
# if defined (__sun__)
    on_exit (clean_exit, NULL);	/* non-ANSI exit handler */
# else
#  ifdef UTMP_SUPPORT
    print_error ("no atexit(), UTMP entries can't be cleaned");
#  endif
# endif
#endif

/*
 * get tty settings before fork()
 * and make a reasonable guess at the value for BackSpace
 */
    get_ttymode (&tio);

/* add value for scrollBar */
    if (scrollbar_visible ()) {
	o->PrivateModes |= PrivMode_scrollBar;
	o->SavedModes |= PrivMode_scrollBar;
    }
    if (menubar_visible ()) {
	o->PrivateModes |= PrivMode_menuBar;
	o->SavedModes |= PrivMode_menuBar;
    }
#ifdef DEBUG_TTYMODE
    debug_ttymode (&tio);
#endif

#ifdef STANDALONE
#ifdef UTMP_SUPPORT
/* spin off the command interpreter */
    signal (SIGHUP, Exit_signal);
#ifndef __svr4__
    signal (SIGINT, Exit_signal);
#endif
    signal (SIGQUIT, Exit_signal);
    signal (SIGTERM, Exit_signal);
#endif
#error
    signal (SIGCHLD, Child_signal);
#endif

/* need to trap SIGURG for SVR4 (Unixware) rlogin */
/* signal (SIGURG, SIG_DFL); */

    o->cmd_pid = fork ();
    if (o->cmd_pid < 0) {
	print_error ("can't fork");
	o->cmd_fd = -1;
	return;
    }
    if (o->cmd_pid == 0) {		/* child */
	/* signal (SIGHUP, Exit_signal); */
	/* signal (SIGINT, Exit_signal); */
#ifdef HAVE_UNSETENV
	/* avoid passing old settings and confusing term size */
	unsetenv ("LINES");
	unsetenv ("COLUMNS");
	/* avoid passing termcap since terminfo should be okay */
	unsetenv ("TERMCAP");
#endif				/* HAVE_UNSETENV */
	/* establish a controlling teletype for the new session */
	rxvtlib_get_tty (o);
	if (o->killed)
	    return;

	/* initialize terminal attributes */
	SET_TTYMODE (0, &tio);

	/* become virtual console, fail silently */
	if (o->Options & Opt_console) {
#ifdef TIOCCONS
	    unsigned int    on = 1;

	    ioctl (0, TIOCCONS, &on);
#elif defined (SRIOCSREDIR)
	    int             fd = open (CONSOLE, O_WRONLY);

	    if (fd < 0 || ioctl (fd, SRIOCSREDIR, 0) < 0) {
		if (fd >= 0)
		    close (fd);
	    }
#endif				/* SRIOCSREDIR */
	}
	rxvtlib_tt_winsize (o, 0);		/* set window size */

	/* reset signals and spin off the command interpreter */
	signal (SIGINT, SIG_DFL);
	signal (SIGQUIT, SIG_DFL);
	signal (SIGCHLD, SIG_DFL);
	/*
	 * mimick login's behavior by disabling the job control signals
	 * a shell that wants them can turn them back on
	 */
#ifdef SIGTSTP
	signal (SIGTSTP, SIG_IGN);
#endif
#ifdef SIGTTIN
	signal (SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTTOU
	signal (SIGTTOU, SIG_IGN);
#endif

	/* command interpreter path */
	if (argv != NULL) {
#ifdef DEBUG_CMD
	    int             i;

	    for (i = 0; argv[i]; i++)
		fprintf (stderr, "argv [%d] = \"%s\"\n", i, argv[i]);
#endif
            if (do_sleep)
		while (1)
		    sleep (60);
	    execvp (argv[0], (char *const *)argv);
	} else {
	    const char     *argv0, *shell;

	    if ((shell = getenv ("SHELL")) == NULL || *shell == '\0')
		shell = "/bin/sh";

	    argv0 = my_basename (shell);
	    if (o->Options & Opt_loginShell) {
		char           *p =

		    MALLOC ((strlen (argv0) + 2) * sizeof (char));

		p[0] = '-';
		STRCPY (&p[1], argv0);
		argv0 = p;
	    }
	    execlp (shell, argv0, NULL);
	}
	exit (EXIT_FAILURE);
	return;
    }

#ifdef STANDALONE
    match_object_to_pid (o, o->cmd_pid);
#endif

/*
 * Close all unused file descriptors. 
 * We don't want them, we don't need them.
 * XXX: is not having 0, 1, 2 for PRINTPIPE a security prob?  Must check. 
 */
#ifdef STANDALONE
    for (i = 0; i < o->num_fds; i++) {
	if (i == 2 || i == o->cmd_fd || i == o->Xfd)
	    continue;
	close (i);
    }
#endif
/*
 * Reduce num_fds to what we use, so select() is more efficient
 */
    o->num_fds = max (2, o->cmd_fd);
    MAX_IT (o->num_fds, o->Xfd);
    o->num_fds++;			/* counts from 0 */

#ifdef UTMP_SUPPORT
    if (!(o->Options & Opt_utmpInhibit)) {
	privileges (RESTORE);
	rxvtlib_makeutent (o, o->ttydev, o->rs[Rs_display_name]);	/* stamp /etc/utmp */
	privileges (IGNORE);
    }
#endif
}
/*}}} */




#ifndef STANDALONE
pid_t open_under_pty (int *in, int *out, char *line, const char *file, char *const argv[])
{E_
    struct tty_values {
	char *ttydev;
	short changettyowner;
	unsigned int num_fds;
	int cmd_fd;
	struct stat ttyfd_stat;
	pid_t cmd_pid;
    } tty_values;
    struct tty_values *o = &tty_values;
#if 0
    struct winsize ws;
#endif
    ttymode_t tios;
    memset (o, 0, sizeof (struct tty_values));
    o->cmd_fd = rxvtlib_get_pty ((rxvtlib *) o);
    o->num_fds = 3;
    if (o->cmd_fd < 0)
	return -1;
    strcpy (line, o->ttydev);

    lstat (o->ttydev, &o->ttyfd_stat);
#ifdef HAVE_TERMIOS_H
    GET_TERMIOS (o->cmd_fd, &tios);
#ifdef OCRNL
    tios.c_oflag &= ~(ONLCR | OCRNL);
#else
    tios.c_oflag &= ~ONLCR;
#endif
    tios.c_lflag &= ~(ECHO | ICANON | ISIG);
    tios.c_iflag &= ~(ICRNL);
#ifdef VTIME
    tios.c_cc[VTIME] = 1;
#endif
#ifdef VMIN
    tios.c_cc[VMIN] = 1;
#endif
    tios.c_iflag &= ~(ISTRIP);
#if defined(TABDLY) && defined(TAB3)
    if ((tios.c_oflag & TABDLY) == TAB3)
	tios.c_oflag &= ~TAB3;
#endif
/* disable interpretation of ^S: */
    tios.c_iflag &= ~IXON;
#ifdef VDISCARD
    tios.c_cc[VDISCARD] = 255;
#endif
#ifdef VEOL2
    tios.c_cc[VEOL2] = 255;
#endif
#ifdef VEOL
    tios.c_cc[VEOL] = 255;
#endif
#ifdef VLNEXT
    tios.c_cc[VLNEXT] = 255;
#endif
#ifdef VREPRINT
    tios.c_cc[VREPRINT] = 255;
#endif
#ifdef VSUSP
    tios.c_cc[VSUSP] = 255;
#endif
#ifdef VWERASE
    tios.c_cc[VWERASE] = 255;
#endif
#endif				/* HAVE_TCGETATTR */
    o->cmd_pid = fork ();
    if (o->cmd_pid < 0) {
	print_error ("can't fork");
	o->cmd_fd = -1;
	return -1;
    }
    if (o->cmd_pid == 0) {	/* child */
#ifdef HAVE_UNSETENV
	unsetenv ("LINES");
	unsetenv ("COLUMNS");
	unsetenv ("TERMCAP");
#endif				/* HAVE_UNSETENV */
	putenv ("TERM=dumb");
	rxvtlib_get_tty ((rxvtlib *) o);
	SET_TTYMODE (o->cmd_fd, &tios);
#if 0
	ws.ws_col = (unsigned short) 80;
	ws.ws_row = (unsigned short) 25;
	ws.ws_xpixel = ws.ws_ypixel = 0;
	ioctl (o->cmd_fd, TIOCSWINSZ, &ws);
	signal (SIGINT, SIG_DFL);	/* FIXME: what should we do about these signals? */
	signal (SIGQUIT, SIG_DFL);
	signal (SIGCHLD, SIG_DFL);
#endif
#ifdef SIGTSTP
	signal (SIGTSTP, SIG_IGN);
#endif
#ifdef SIGTTIN
	signal (SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTTOU
	signal (SIGTTOU, SIG_IGN);
#endif
	execvp (file, (char *const *) argv);
	exit (1);
    }
    *in = *out = o->cmd_fd;
    if (o->ttydev) {
	o->ttydev = NULL;
	free (o->ttydev);
    }
    return o->cmd_pid;
}
#endif


/*}}} */
/*----------------------- end-of-file (C source) -----------------------*/
