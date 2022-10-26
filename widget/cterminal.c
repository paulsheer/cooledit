/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include <config.h>
#if defined(__sun) && defined(__SVR4)
#include <sys/strredir.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_UTIL_H
#include <util.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#if defined (HAVE_SYS_IOCTL_H) && !defined (__sun__)
#include <sys/ioctl.h>
#endif
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/wait.h>
#include <sys/stat.h>
#ifdef GREEK_SUPPORT
#include "grkelot.h"
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#else
#include <sgtty.h>
#endif                          /* HAVE_TERMIOS_H */
#ifndef NO_XSETLOCALE
#else
#ifndef NO_SETLOCALE
#include <locale.h>
#endif
#endif
#ifdef TTY_GID_SUPPORT
#include <grp.h>
#endif
#if defined(PTYS_ARE_PTMX) && !defined(__FreeBSD__) && !defined(__DragonFly__)
#include <sys/resource.h>       /* for struct rlimit */
#include <sys/stropts.h>        /* for I_PUSH */
#endif
#ifdef UTMP_SUPPORT
#if ! defined(HAVE_STRUCT_UTMPX) && ! defined(HAVE_STRUCT_UTMP)
#error cannot build with utmp support - no utmp or utmpx struct found
#endif
#ifdef RXVT_UTMP_AS_UTMPX
#include <utmpx.h>
#else
#include <utmp.h>
#endif
#ifdef HAVE_LASTLOG_H
#include <lastlog.h>
#endif
#include <pwd.h>
#ifdef WTMP_SUPPORT
#ifdef RXVT_UTMP_AS_UTMPX
#ifdef RXVT_WTMPX_FILE
#else
#error cannot build with wtmp support - no wtmpx file found
#endif
#else
#ifdef RXVT_WTMP_FILE
#else
#error cannot build with wtmp support - no wtmp file found
#endif
#endif
#endif
#endif
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_SYS_BYTEORDER_H
#include <sys/byteorder.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
#include <net/if_arp.h>
#endif
#ifndef NEXT_SCROLLBAR
#ifdef XTERM_SCROLLBAR          /* bitmap scrollbar */
#if (SB_WIDTH != 15)
#error Error, check scrollbar width (SB_WIDTH).It must be 15 for XTERM_SCROLLBAR
#endif
#else                           /* XTERM_SCROLLBAR */
#endif                          /* ! XTERM_SCROLLBAR */
#else                           /* ! NEXT_SCROLLBAR */
#endif                          /* ! NEXT_SCROLLBAR */



#include <stringtools.h>
#include "cterminal.h"



/* ways to deal with getting/setting termios structure */
#ifdef HAVE_TERMIOS_H
/* termios interface */

#ifdef TCSANOW                  /* POSIX */
#define GET_TERMIOS(fd,tios)	tcgetattr (fd, tios)
#define SET_TERMIOS(fd,tios)		\
	cfsetospeed (tios, BAUDRATE),	\
	cfsetispeed (tios, BAUDRATE),	\
	tcsetattr (fd, TCSANOW, tios)
#else
#ifdef TIOCSETA
#define GET_TERMIOS(fd,tios)	ioctl (fd, TIOCGETA, tios)
#define SET_TERMIOS(fd,tios)		\
	tios->c_cflag |= BAUDRATE,	\
	ioctl (fd, TIOCSETA, tios)
#else
#define GET_TERMIOS(fd,tios)	ioctl (fd, TCGETS, tios)
#define SET_TERMIOS(fd,tios)		\
	tios->c_cflag |= BAUDRATE,	\
	ioctl (fd, TCSETS, tios)
#endif
#endif
#define SET_TTYMODE(fd,tios)		SET_TERMIOS (fd, tios)
#else
/* sgtty interface */

struct _ttymode_t {
    struct sgttyb sg;
    struct tchars tc;
    struct ltchars lc;
    int line;
    int local;
};

#define SET_TTYMODE(fd,tt)				\
	tt->sg.sg_ispeed = tt->sg.sg_ospeed = BAUDRATE,	\
	ioctl (fd, TIOCSETP, &(tt->sg)),		\
	ioctl (fd, TIOCSETC, &(tt->tc)),		\
	ioctl (fd, TIOCSLTC, &(tt->lc)),		\
	ioctl (fd, TIOCSETD, &(tt->line)),		\
	ioctl (fd, TIOCLSET, &(tt->local))
#endif                          /* HAVE_TERMIOS_H */



#ifdef PTYS_ARE_PTMX
#define _NEW_TTY_CTRL           /* to get proper defines in <termios.h> */
#endif

/* use the fastest baud-rate */
#ifdef B38400
#define BAUDRATE	B38400
#else
#ifdef B19200
#define BAUDRATE	B19200
#else
#define BAUDRATE	B9600
#endif
#endif

/* Disable special character functions */
#ifdef _POSIX_VDISABLE
#define VDISABLE	_POSIX_VDISABLE
#else
#define VDISABLE	255
#endif

#define PTYCHAR1	"pqrstuvwxyz"
#define PTYCHAR2	"0123456789abcdef"

#define PUTENV(e)       do { if (o->n_envvar < RXVTLIB_MAX_ENVVAR) \
                                 o->envvar[o->n_envvar++] = (char *) strdup (e); } while (0)

#define UNSETENV(e)       do { if (o->n_envvar < RXVTLIB_MAX_ENVVAR) \
                                   o->envvar[o->n_envvar++] = (char *) strdup (e); } while (0)

#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))



/*----------------------------------------------------------------------*/
/* EXTPROTO */
static const char *my_basename (const char *str)
{E_
    const char *base = strrchr (str, '/');

    return (base ? base + 1 : str);
}



void cterminal_cleanup (struct cterminal *o)
{E_
    int i;
    for (i = 0; i < o->n_envvar; i++)
        free (o->envvar[i]);

    if (o->ttydev) {
        free (o->ttydev);
        o->ttydev = 0;
    }
}


/*{{{ take care of suid/sgid super-user (root) privileges */
/* EXTPROTO */
static void cterminal_privileges (int mode)
{E_
#warning comment out
#ifdef HAVE_SETEUID
    static uid_t euid;
    static gid_t egid;

    switch (mode) {
    case CTERMINAL_IGNORE:
        /*
         * change effective uid/gid - not real uid/gid - so we can switch
         * back to root later, as required
         */
        seteuid (getuid ());
        setegid (getgid ());
        break;
    case CTERMINAL_SAVE:
        euid = geteuid ();
        egid = getegid ();
        break;
    case CTERMINAL_RESTORE:
        seteuid (euid);
        setegid (egid);
        break;
    }
#else
        switch (mode) {
    case CTERMINAL_IGNORE:
        setuid (getuid ());
        setgid (getgid ());
        /* FALLTHROUGH */
    case CTERMINAL_SAVE:
        /* FALLTHROUGH */
    case CTERMINAL_RESTORE:
        break;
    }
#endif
}

/*}}} */


/*{{{ establish a controlling teletype for new session */
/*
 * On some systems this can be done with ioctl() but on others we
 * need to re-open the slave tty.
 */
/* INTPROTO */
static int cterminal_get_tty (struct cterminal *o, char *errmsg)
{E_
    int fd, i;
    pid_t pid;

/*
 * setsid() [or setpgrp] must be before open of the terminal,
 * otherwise there is no controlling terminal (Solaris 2.4, HP-UX 9)
 */
#ifdef NO_SETSID
    pid = setpgrp (0, 0);
#else
    pid = setsid ();
#endif
    if (pid < 0)
#ifdef STANDALONE
        perror (o->rs[Rs_name]);
#else
        perror ("cterminal_get_tty");
#endif

    if ((fd = open (o->ttydev, O_RDWR)) < 0
        && (fd = open ("/dev/tty", O_RDWR)) < 0) {
        snprintf (errmsg, CTERMINAL_ERR_MSG_LEN, "can't open slave tty %s [%s]", o->ttydev, strerror (errno));
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
        unsigned int mode = 0622;
        gid_t gid = getgid ();

#ifdef TTY_GID_SUPPORT
        {
            struct group *gr = getgrnam ("tty");

            if (gr) {
                /* change ownership of tty to real uid, "tty" gid */
                gid = gr->gr_gid;
                mode = 0620;
            }
        }
#endif                          /* TTY_GID_SUPPORT */
        cterminal_privileges (CTERMINAL_RESTORE);
        fchown (fd, getuid (), gid);    /* fail silently */
        fchmod (fd, mode);
        cterminal_privileges (CTERMINAL_IGNORE);
    }

    /*
     * Close all file descriptors.  If only stdin/out/err are closed,
     * child processes remain alive upon deletion of the window.
     */
    {
        int max_fds;
        max_fds = sysconf (_SC_OPEN_MAX);
        for (i = 0; i < max_fds; i++)
            if (i != fd)
                close (i);
    }

/* Reopen stdin, stdout and stderr over the tty file descriptor */
    dup2 (fd, 0);               /* stdin */
    dup2 (fd, 1);               /* stdout */
    dup2 (fd, 2);               /* stderr */

    if (fd > 2)
        close (fd);

#ifdef TIOCSCTTY
    ioctl (0, TIOCSCTTY, 0);
#endif

/* set process group */
#if defined (_POSIX_VERSION) || defined (__svr4__)
    tcsetpgrp (0, pid);
#elif defined (TIOCSPGRP)
    ioctl (0, TIOCSPGRP, &pid);
#endif

/* svr4 problems: reports no tty, no job control */
/* # if !defined (__svr4__) && defined (TIOCSPGRP) */
    close (open (o->ttydev, O_RDWR, 0));
/* # endif */

    return fd;
}

/*}}} */



pid_t open_under_pty (int *in, int *out, char *line, const char *file, char *const argv[], char *errmsg)
{E_
    int cmd_fd = -1;
    struct cterminal tty_values;
    struct cterminal *o = &tty_values;
#if 0
    struct winsize ws;
#endif
    ttymode_t tios;
    memset (&tios, 0, sizeof (tios));
    memset (o, 0, sizeof (struct cterminal));
    cmd_fd = cterminal_get_pty (o, errmsg);
    if (cmd_fd < 0)
        return -1;
    strcpy (line, o->ttydev);

    lstat (o->ttydev, &o->ttyfd_stat);
#ifdef HAVE_TERMIOS_H
    GET_TERMIOS (cmd_fd, &tios);
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
#endif                          /* HAVE_TCGETATTR */
    o->cmd_parentpid = getpid ();
    o->cmd_pid = fork ();
    if (o->cmd_pid < 0) {
        snprintf (errmsg, CTERMINAL_ERR_MSG_LEN, "can't fork: [%s]", strerror (errno));
        return -1;
    }
    if (o->cmd_pid == 0) {      /* child */
        char *envvar[32];
        int n_envvar = 0;

        memset (envvar, '\0', sizeof (envvar));

        envvar[n_envvar++] = "LINES";
        envvar[n_envvar++] = "COLUMNS";
        envvar[n_envvar++] = "TERMCAP";
        envvar[n_envvar++] = "TERM=dumb";

        if (cterminal_get_tty (o, errmsg) < 0)
            exit (1);
        SET_TTYMODE (0, &tios);
#if 0
        ws.ws_col = (unsigned short) 80;
        ws.ws_row = (unsigned short) 25;
        ws.ws_xpixel = ws.ws_ypixel = 0;
        ioctl (0, TIOCSWINSZ, &ws);
        signal (SIGINT, SIG_DFL);       /* FIXME: what should we do about these signals? */
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
        execve_path_search (file, argv, set_env_var (envvar, n_envvar));
        exit (1);
    }
    *in = *out = cmd_fd;
    if (o->ttydev) {
        o->ttydev = NULL;
        free (o->ttydev);
    }
    return o->cmd_pid;
}


/*{{{ get_ttymode() */
/* INTPROTO */
static void get_ttymode (ttymode_t * tio, int *erase_char)
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
#ifdef VDSUSP
        tio->c_cc[VDSUSP] = CDSUSP;
#endif
#ifdef VREPRINT
        tio->c_cc[VREPRINT] = CRPRNT;
#endif
#ifdef VDISCRD
        tio->c_cc[VDISCRD] = CFLUSH;
#endif
#ifdef VWERSE
        tio->c_cc[VWERSE] = CWERASE;
#endif
#ifdef VLNEXT
        tio->c_cc[VLNEXT] = CLNEXT;
#endif
    }
    *erase_char = tio->c_cc[VERASE];
    tio->c_cc[VEOF] = CEOF;
    tio->c_cc[VEOL] = VDISABLE;
#ifdef VEOL2
    tio->c_cc[VEOL2] = VDISABLE;
#endif
#ifdef VSWTC
    tio->c_cc[VSWTC] = VDISABLE;
#endif
#ifdef VSWTCH
    tio->c_cc[VSWTCH] = VDISABLE;
#endif
#if VMIN != VEOF
    tio->c_cc[VMIN] = 1;
#endif
#if VTIME != VEOL
    tio->c_cc[VTIME] = 0;
#endif

/* input modes */
    tio->c_iflag = (BRKINT | IGNPAR | ICRNL | IXON
#ifdef IMAXBEL
                    | IMAXBEL
#endif
        );

/* output modes */
    tio->c_oflag = (OPOST | ONLCR);

/* control modes */
    tio->c_cflag = (CS8 | CREAD);

/* line discipline modes */
    tio->c_lflag = (ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK
#if defined (ECHOCTL) && defined (ECHOKE)
                    | ECHOCTL | ECHOKE
#endif
        );
#else                           /* HAVE_TERMIOS_H */
/*
 * sgtty interface
 */
/* get parameters -- gtty */
        if (ioctl (0, TIOCGETP, &(tio->sg)) < 0) {
        tio->sg.sg_erase = CERASE;      /* ^H */
        tio->sg.sg_kill = CKILL;        /* ^U */
    }
    *erase_char = tio->sg.sg_erase;
    tio->sg.sg_flags = (CRMOD | ECHO | EVENP | ODDP);

/* get special characters */
    if (ioctl (0, TIOCGETC, &(tio->tc)) < 0) {
        tio->tc.t_intrc = CINTR;        /* ^C */
        tio->tc.t_quitc = CQUIT;        /* ^\ */
        tio->tc.t_startc = CSTART;      /* ^Q */
        tio->tc.t_stopc = CSTOP;        /* ^S */
        tio->tc.t_eofc = CEOF;  /* ^D */
        tio->tc.t_brkc = -1;
    }
/* get local special chars */
    if (ioctl (0, TIOCGLTC, &(tio->lc)) < 0) {
        tio->lc.t_suspc = CSUSP;        /* ^Z */
        tio->lc.t_dsuspc = CDSUSP;      /* ^Y */
        tio->lc.t_rprntc = CRPRNT;      /* ^R */
        tio->lc.t_flushc = CFLUSH;      /* ^O */
        tio->lc.t_werasc = CWERASE;     /* ^W */
        tio->lc.t_lnextc = CLNEXT;      /* ^V */
    }
/* get line discipline */
    ioctl (0, TIOCGETD, &(tio->line));
#ifdef NTTYDISC
    tio->line = NTTYDISC;
#endif                          /* NTTYDISC */
    tio->local = (LCRTBS | LCRTERA | LCTLECH | LPASS8 | LCRTKIL);
#endif                          /* HAVE_TERMIOS_H */
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
int cterminal_get_pty (struct cterminal *o, char *errmsg)
{E_
    int fd;
    char *ptydev;

    o->changettyowner = 1;

    if (o->ttydev) {
        free (o->ttydev);
        o->ttydev = 0;
    }

#ifdef __FreeBSD__
    fd = posix_openpt (O_RDWR);
    if (fd >= 0) {
        if (grantpt (fd) == 0   /* change slave permissions */
            && unlockpt (fd) == 0) {    /* slave now unlocked */
            ptydev = o->ttydev = (char *) strdup (ptsname (fd));        /* get slave's name */
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
#if defined(PTYS_ARE_GETPT) || defined(PTYS_ARE_PTMX)
    {
        extern char *ptsname ();

#ifdef PTYS_ARE_GETPT
        if ((fd = getpt ()) >= 0)
#else
        if ((fd = open ("/dev/ptmx", O_RDWR)) >= 0)
#endif
        {
            if (grantpt (fd) == 0       /* change slave permissions */
                && unlockpt (fd) == 0) {        /* slave now unlocked */
                ptydev = o->ttydev = (char *) strdup (ptsname (fd));    /* get slave's name */
                o->changettyowner = 0;
                goto Found;
            }
            close (fd);
        }
    }
#endif
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
        int idx;
        char *c1, *c2;
        char pty_name[] = "/dev/ptyp???";
        char tty_name[] = "/dev/ttyp???";

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
        int len;
        const char *c1, *c2;
        char pty_name[] = "/dev/pty??";
        char tty_name[] = "/dev/tty??";

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

    snprintf (errmsg, CTERMINAL_ERR_MSG_LEN, "can't open pseudo-tty");
    return -1;

  Found:
    fcntl (fd, F_SETFL, O_NDELAY);
    return fd;
}

/*}}} */


/*{{{ window resizing */
/*
 * Tell the teletype handler what size the window is.
 * Called after a window size change.
 */
/* INTPROTO */
void cterminal_tt_winsize (struct cterminal *o, int fd, int col, int row)
{E_
    struct winsize ws;

    if (fd < 0)
        return;

    ws.ws_col = (unsigned short) col;
    ws.ws_row = (unsigned short) row;
    ws.ws_xpixel = ws.ws_ypixel = 0;
    ioctl (fd, TIOCSWINSZ, &ws);
}

/*{{{ run_command() */
/*
 * Run the command in a subprocess and return a file descriptor for the
 * master end of the pseudo-teletype pair with the command talking to
 * the slave.
 */
/* INTPROTO */
int cterminal_run_command (struct cterminal *o, struct cterminal_config *config, char *const argv[], char *errmsg)
{E_
    int cmd_fd = -1;
    ttymode_t tio;

/*
 * Save and then give up any super-user privileges
 * If we need privileges in any area then we must specifically request it.
 * We should only need to be root in these cases:
 *  1.  write utmp entries on some systems
 *  2.  chown tty on some systems
 */
    cterminal_privileges (CTERMINAL_SAVE);
    cterminal_privileges (CTERMINAL_IGNORE);

#if 0
    printf ("display_env_var = %s\n", config->display_env_var);
    printf ("term_win_id = %lu\n", config->term_win_id);
    printf ("term_name = %s\n", config->term_name);
    printf ("colorterm_name = %s\n", config->colorterm_name);
    printf ("col = %d\n", config->col);
    printf ("row = %d\n", config->row);
    printf ("env_fg = %d\n", config->env_fg);
    printf ("env_bg = %d\n", config->env_bg);
    printf ("login_shell = %d\n", config->login_shell);
    printf ("do_sleep = %d\n", config->do_sleep);
    printf ("charset_8bit = %d\n", config->charset_8bit);
#endif

    if ((cmd_fd = cterminal_get_pty (o, errmsg)) < 0)
        return -1;

/* store original tty status for restoration clean_exit() -- rgg 04/12/95 */
    lstat (o->ttydev, &o->ttyfd_stat);

#if 1
#warning handle cleanup ownerships
#else
/* install exit handler for cleanup */
#ifdef HAVE_ATEXIT
    atexit (clean_exit);
#else
#if defined (__sun__)
    on_exit (clean_exit, NULL); /* non-ANSI exit handler */
#else
#ifdef UTMP_SUPPORT
    print_error ("no atexit(), UTMP entries can't be cleaned");
#endif
#endif
#endif

#endif

/*
 * get tty settings before fork()
 * and make a reasonable guess at the value for BackSpace
 */
    get_ttymode (&tio, &o->erase_char);
    config->erase_char = o->erase_char;

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

    o->cmd_parentpid = getpid ();
    o->cmd_pid = fork ();
    if (o->cmd_pid < 0) {
        snprintf (errmsg, CTERMINAL_ERR_MSG_LEN, "can't fork: [%s]", strerror (errno));
        return -1;
    }
    if (o->cmd_pid == 0) {      /* child */
        char envstr[100];

        /* signal (SIGHUP, Exit_signal); */
        /* signal (SIGINT, Exit_signal); */
        /* avoid passing old settings and confusing term size */

        UNSETENV ("LINES");
        UNSETENV ("COLUMNS");
        /* avoid passing termcap since terminfo should be okay */
        UNSETENV ("TERMCAP");

/* add entries to the environment:
 * @ DISPLAY:   in case we started with -display
 * @ WINDOWID:  X window id number of the window
 * @ COLORTERM: terminal sub-name and also indicates its color
 * @ TERM:      terminal name
 * @ TERMINFO:	path to terminfo directory
 */

#define PUTENVF(a, b) \
    do {  snprintf (envstr, sizeof(envstr), a, b); \
          PUTENV(envstr); } while (0)

        PUTENVF ("DISPLAY=%.90s", config->display_env_var);
        PUTENVF ("WINDOWID=%lu", config->term_win_id);
        PUTENVF ("TERM=%s", config->term_name);
        PUTENVF ("COLORTERM=%s", config->colorterm_name);
        strcpy (envstr, "COLORFGBG=");
        strcat (envstr, config->env_fg >= 0 ? Citoa (config->env_fg) : "default");
        strcat (envstr, ";");
        strcat (envstr, config->env_bg >= 0 ? Citoa (config->env_bg) : "default");
        PUTENV (envstr);
        PUTENV ("CLICOLOR=1");  /* Enable colorize ls output on FreeBSD. See the FreeBSD man page for ls */
        if (config->charset_8bit)
            PUTENV ("LANG");

#ifdef RXVT_TERMINFO
        PUTENV ("TERMINFO=" RXVT_TERMINFO);
#endif

        /* establish a controlling teletype for the new session */
        if (cterminal_get_tty (o, errmsg) < 0)
            return -1;

        /* initialize terminal attributes */
        SET_TTYMODE (0, &tio);

#if 0                           /* we don't want this for cooledit/coolwidgets */
        /* become virtual console, fail silently */
        if (o->Options & Opt_console) {
#ifdef TIOCCONS
            unsigned int on = 1;

            ioctl (0, TIOCCONS, &on);
#elif defined (SRIOCSREDIR)
            int fd = open (CONSOLE, O_WRONLY);

            if (fd < 0 || ioctl (fd, SRIOCSREDIR, 0) < 0) {
                if (fd >= 0)
                    close (fd);
            }
#endif                          /* SRIOCSREDIR */
        }
#endif
        cterminal_tt_winsize (o, 0, config->col, config->row);  /* set window size */

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
            if (config->do_sleep)
                while (1)
                    sleep (60);
            execve_path_search (argv[0], argv, set_env_var (o->envvar, o->n_envvar));
        } else {
            const char *argv0, *shell;
            char *args[2];

            if ((shell = getenv ("SHELL")) == NULL || *shell == '\0')
                shell = "/bin/sh";

            argv0 = my_basename (shell);
            if (config->login_shell) {
                char *p = (char *) malloc ((strlen (argv0) + 2) * sizeof (char));

                p[0] = '-';
                strcpy (&p[1], argv0);
                argv0 = p;
            }
            args[0] = (char *) strdup (argv0);
            args[1] = NULL;
            execve_path_search (shell, args, set_env_var (o->envvar, o->n_envvar));
        }
        exit (EXIT_FAILURE);
        return -1;
    }

#ifdef STANDALONE
    match_object_to_pid (o, o->cmd_pid);
#endif

#ifdef UTMP_SUPPORT
    if (!(o->Options & Opt_utmpInhibit)) {
        cterminal_privileges (CTERMINAL_RESTORE);
        rxvtlib_makeutent (o, o->ttydev, o->rs[Rs_display_name]);       /* stamp /etc/utmp */
        cterminal_privileges (CTERMINAL_IGNORE);
    }
#endif

    return cmd_fd;
}

/*}}} */


