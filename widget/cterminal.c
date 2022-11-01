/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#ifndef MSWIN

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
#include <grp.h>
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
#ifdef HAVE_LASTLOG_H
#include <lastlog.h>
#endif
#include <pwd.h>

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


#define UTMP_SUPPORT

#ifdef __FreeBSD__
#define HAVE_STRUCT_UTMPX
#define RXVT_UTMP_AS_UTMPX
#define HAVE_UTMPX_HOST
#else
#define HAVE_STRUCT_UTMP
#define HAVE_UTMP_HOST
#endif



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

#ifdef STANDALONE
#undef strdup
#endif


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


static void cterminal_makeutent (struct cterminal *o, const char *hostname);
static void cterminal_cleanutent (struct cterminal *o);
static void cterminal_setttyowner (struct cterminal *o);



extern char **environ;

char **set_env_var (char *new_var[], int nn)
{E_
    int exists, n, i, j, k, *l;
    char **r;
    l = (int *) alloca (nn * sizeof (int));
    for (n = 0; environ[n]; n++);
    for (k = 0; k < nn; k++) {
        assert (new_var[k]);
        assert (new_var[k][0]);
        l[k] = strcspn (new_var[k], "=");
    }
    r = (char **) malloc (sizeof (const char *) * (n + nn + 2));
    for (i = j = 0; j < n; j++) {
        assert (environ[j]);
        for (exists = k = 0; k < nn && !exists; k++)
            exists = !strncmp (environ[j], new_var[k], l[k]);
        if (!exists)
            r[i++] = environ[j];
    }
    for (k = 0; k < nn; k++)
        if (new_var[k][l[k]] == '=')       /* a missing equals sign means to delete environment variable */
            r[i++] = new_var[k];
    r[i] = NULL;
    return r;
}


int execve_path_search (const char *file, char *const argv[], char *const envp[])
{E_
    char *path, *p, *q;
    int done = 0;

    path = getenv ("PATH");
    if (!path || !*path || *file == '/')
        return execve (file, argv, envp);

    p = q = path;

    while (!done) {
        int fd;
        char *v;
        while (*q && *q != ':')
            q++;
        if (!*q)
            done = 1;
        if (!strlen (p))
            continue;
        v = (char *) malloc ((q - p) + strlen (file) + 2);
        strncpy (v, p, q - p);
        strcpy (v + (q - p), "/");
        strcat (v, file);
        if ((fd = open (v, O_RDONLY)) >= 0) {
            close (fd);
            return execve (v, argv, envp);
        }
        free (v);
        p = q + 1;
        q = p;
    }

    errno = ENOENT;
    return -1;
}



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

    cterminal_cleanutent (o);

    if (o->ttydev) {
        free (o->ttydev);
        o->ttydev = 0;
    }
}



/* establish a controlling teletype for new session */
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
#ifdef OLD_RXVT_STANDALONE_CODE
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

#warning on older unix boxes this may have a problem grabbing lots of terminals in the users name:
    if (o->changettyowner)
        cterminal_setttyowner (o);

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

static void get_ttymode_dumb_terminal (ttymode_t * tio)
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

#ifdef OCRNL
    tio->c_oflag &= ~(ONLCR | OCRNL);
#else
    tio->c_oflag &= ~ONLCR;
#endif
    tio->c_lflag &= ~(ECHO | ICANON | ISIG);
    tio->c_iflag &= ~(ICRNL);
#ifdef VTIME
    tio->c_cc[VTIME] = 1;
#endif
#ifdef VMIN
    tio->c_cc[VMIN] = 1;
#endif
    tio->c_iflag &= ~(ISTRIP);
#if defined(TABDLY) && defined(TAB3)
    if ((tio->c_oflag & TABDLY) == TAB3)
        tio->c_oflag &= ~TAB3;
#endif
/* disable interpretation of ^S: */
    tio->c_iflag &= ~IXON;
#ifdef VDISCARD
    tio->c_cc[VDISCARD] = 255;
#endif
#ifdef VEOL2
    tio->c_cc[VEOL2] = 255;
#endif
#ifdef VEOL
    tio->c_cc[VEOL] = 255;
#endif
#ifdef VLNEXT
    tio->c_cc[VLNEXT] = 255;
#endif
#ifdef VREPRINT
    tio->c_cc[VREPRINT] = 255;
#endif
#ifdef VSUSP
    tio->c_cc[VSUSP] = 255;
#endif
#ifdef VWERASE
    tio->c_cc[VWERASE] = 255;
#endif
#endif                          /* HAVE_TCGETATTR */
}


/* get_ttymode() */
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





/* Acquire a pseudo-teletype from the system. */
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



/* window resizing */
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
    if (ioctl (fd, TIOCSWINSZ, &ws))
        perror ("TIOCSWINSZ");
}

/* run_command() */
/*
 * Run the command in a subprocess and return a file descriptor for the
 * master end of the pseudo-teletype pair with the command talking to
 * the slave.
 */
/* INTPROTO */
int cterminal_run_command (struct cterminal *o, struct cterminal_config *config, int dumb_terminal, const char *log_origin_host, char *const argv[], char *errmsg)
{E_
    int cmd_fd = -1;
    ttymode_t tio;

    o->cmd_fd = -1;

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

#if 0
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
    memset (&tio, '\0', sizeof (tio));
    if (config) {
        get_ttymode (&tio, &o->erase_char);
        config->erase_char = o->erase_char;
    } else {
        get_ttymode_dumb_terminal (&tio);
    }

#ifdef OLD_RXVT_STANDALONE_CODE
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
        close (cmd_fd);
        snprintf (errmsg, CTERMINAL_ERR_MSG_LEN, "can't fork: [%s]", strerror (errno));
        return -1;
    }
    if (o->cmd_pid == 0) {      /* child */
        char envstr[100];

        /* signal (SIGHUP, Exit_signal); */
        /* signal (SIGINT, Exit_signal); */
        /* avoid passing old settings and confusing term size */

        if (config) {

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
            if (config->env_fg >= 0)
                snprintf (envstr + strlen (envstr), 32, "%d;", config->env_fg);
            else
                snprintf (envstr + strlen (envstr), 32, "default;");
            if (config->env_bg >= 0)
                snprintf (envstr + strlen (envstr), 32, "%d", config->env_bg);
            else
                snprintf (envstr + strlen (envstr), 32, "default");
            PUTENV (envstr);
            PUTENV ("CLICOLOR=1");  /* Enable colorize ls output on FreeBSD. See the FreeBSD man page for ls */
            if (config->charset_8bit)
                PUTENV ("LANG");
#ifdef RXVT_TERMINFO
            PUTENV ("TERMINFO=" RXVT_TERMINFO);
#endif
        } else {

            UNSETENV ("LINES");
            UNSETENV ("COLUMNS");
            UNSETENV ("TERMCAP");
            UNSETENV ("TERM=dumb");

        }

        if (dumb_terminal == 2) {
            PUTENV ("LC_ALL=C");
            PUTENV ("LC_CTYPE=C");
            PUTENV ("LC_MESSAGE=C");
            UNSETENV ("LANG");
            UNSETENV ("LANGUAGE");
        }

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
        if (config)
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
        if (argv != NULL && argv[0] != NULL) {
            if (config && config->do_sleep)
                while (1)
                    sleep (60);
            execve_path_search (argv[0], argv, set_env_var (o->envvar, o->n_envvar));
        } else {
            const char *argv0, *shell;
            char *args[2];

            if ((shell = getenv ("SHELL")) == NULL || *shell == '\0')
                shell = "/bin/sh";

            argv0 = my_basename (shell);
            if (config && config->login_shell) {
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

#ifdef OLD_RXVT_STANDALONE_CODE
    match_object_to_pid (o, o->cmd_pid);
#endif

#ifdef UTMP_SUPPORT
    if (log_origin_host) {
        o->clean_utmp = 1;
        cterminal_makeutent (o, log_origin_host);       /* stamp /etc/utmp */
    }
#endif

    o->cmd_fd = cmd_fd;
    return 0;
}





#ifdef UTMP_SUPPORT
#if ! defined(HAVE_STRUCT_UTMPX) && ! defined(HAVE_STRUCT_UTMP)
#error cannot build with utmp support - no utmp or utmpx struct found
#endif
#ifdef RXVT_UTMP_AS_UTMPX
#include <utmpx.h>
#else
#include <utmp.h>
#endif



#ifdef UTMP_SUPPORT
# if ! defined(HAVE_STRUCT_UTMPX) && ! defined(HAVE_STRUCT_UTMP)
#  error cannot build with utmp support - no utmp or utmpx struct found
# endif

# if defined(RXVT_UTMPX_FILE) && defined(HAVE_STRUCT_UTMPX)
#   define RXVT_UTMP_AS_UTMPX
# else
#  if defined(RXVT_UTMP_FILE) && defined(HAVE_STRUCT_UTMP)
#   undef RXVT_UTMP_AS_UTMPX
#  endif
# endif
/* if you have both utmp and utmpx files lying around and are really
 * using utmp not utmpx, then uncomment the following line */
/* #undef RXVT_UTMP_AS_UTMPX */

# ifdef RXVT_UTMP_AS_UTMPX
#  define RXVT_REAL_UTMP_FILE	RXVT_UTMPX_FILE
# else
#  define RXVT_REAL_UTMP_FILE	RXVT_UTMP_FILE
# endif

# ifdef RXVT_UTMP_AS_UTMPX
#  define USE_SYSV_UTMP
# else
#  ifdef HAVE_SETUTENT
#   define USE_SYSV_UTMP
#  else
#   undef USE_SYSV_UTMP
#  endif
# endif

# undef UTMP
# ifdef USE_SYSV_UTMP
#  ifndef USER_PROCESS
#   define USER_PROCESS		7
#  endif
#  ifndef DEAD_PROCESS
#   define DEAD_PROCESS		8
#  endif
#  ifdef RXVT_UTMP_AS_UTMPX
#   define UTMP			struct utmpx
#   define setutent		setutxent
#   define getutent		getutxent
#   define getutid		getutxid
#   define endutent		endutxent
#   define pututline		pututxline
#  endif
# endif
# ifndef UTMP
#  define UTMP			struct utmp
# endif

# ifdef WTMP_SUPPORT
#  ifdef RXVT_UTMP_AS_UTMPX
#   define update_wtmp		updwtmpx
#   ifdef RXVT_WTMPX_FILE
#    define RXVT_REAL_WTMP_FILE	RXVT_WTMPX_FILE
#   else
#    error cannot build with wtmp support - no wtmpx file found
#   endif
#  else
#   define update_wtmp		rxvt_update_wtmp
#   ifdef RXVT_WTMP_FILE
#    define RXVT_REAL_WTMP_FILE	RXVT_WTMP_FILE
#   else
#    error cannot build with wtmp support - no wtmp file found
#   endif
#  endif
# endif

#endif



#ifdef UTMP_SUPPORT

static int utmp_pipe[2] = {-1, -1};

struct wtmp_command {
    struct cterminal ct;
#define UTMP_CMD_MAKE           1
#define UTMP_CMD_CLEAN          2
#define UTMP_CMD_SETTTYOWNER    3
    int cmd;
    char hostname[256];
    char ttydev[64];
    pid_t sid;
    pid_t pid;
    time_t now;
};

#endif

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

static int NOWARN_BOUNDS(int v)
{
    volatile int r;
    r = v;
    return r;
}

/*
 * make a utmp entry
 */
/* EXTPROTO */
static void cterminal_makeutent_ (struct wtmp_command *tc, int dead)
{E_
    struct cterminal *o;
    char ut_id[8];
    char *pty;
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
#if (defined(HAVE_UTMP_HOST) && ! defined(RXVT_UTMP_AS_UTMPX)) || (defined(HAVE_UTMPX_HOST) && defined(RXVT_UTMP_AS_UTMPX))
# ifndef linux
    char           *colon;
# endif
#endif

#endif				/* !USE_SYSV_UTMP */

    o = &tc->ct;
    pty = tc->ttydev;
    (void) o;

/* BSD naming is of the form /dev/tty?? or /dev/pty?? */

    memset (&ut, 0, sizeof (UTMP));
    if (!strncmp (pty, "/dev/", 5))
	pty += 5;		/* skip /dev/ prefix */
    if (!strncmp (pty, "pty", 3) || !strncmp (pty, "tty", 3))
	strncpy (ut_id, (pty + 3), NOWARN_BOUNDS (sizeof (ut_id)));
    else
#ifndef USE_SYSV_UTMP
    {
	print_error ("can't parse tty name \"%s\"", pty);
	ut_id[0] = '\0';	/* entry not made */
	return;
    }

    strncpy (ut.ut_line, pty, sizeof (ut.ut_line));
    strncpy (ut.ut_name, (pwent && pwent->pw_name) ? pwent->pw_name : "?",
	     sizeof (ut.ut_name));
    strncpy (ut.ut_host, tc->hostname, sizeof (ut.ut_host));
    ut.ut_time = time (NULL);

    if ((fd0 = fopen (RXVT_REAL_UTMP_FILE, "r+")) == NULL)
	ut_id[0] = '\0';	/* entry not made */
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
	    ut_id[0] = '\0';	/* entry not made */
	else {
	    fseek (fd0, o->utmp_pos, 0);
            if (dead)
                memset (&ut, '\0', sizeof (UTMP));
	    fwrite (&ut, sizeof (UTMP), 1, fd0);
	}
	fclose (fd0);
    }

#else				/* USE_SYSV_UTMP */
    {
	int             n;

	if (sscanf (pty, "pts/%d", &n) == 1)
	    snprintf (ut_id, sizeof (ut_id), "vt%02x", (n % 256));	/* sysv naming */
	else {
	    printf ("can't parse tty name \"%s\"\n", pty);
	    ut_id[0] = '\0';	/* entry not made */
	    return;
	}
    }

#if 0
    /* XXX: most likely unnecessary.  could be harmful */
    utmpname (RXVT_REAL_UTMP_FILE);
#endif

    setutent ();		/* XXX: should be unnecessaray */

    ut_id[sizeof (ut_id) - 1] = '\0';
    strncpy (ut.ut_id, ut_id, sizeof (ut.ut_id));
    ut.ut_type = dead ? USER_PROCESS : DEAD_PROCESS;
    (void)getutid (&ut);	/* position to entry in utmp file */

/* set up the new entry */
    ut.ut_type = dead ? DEAD_PROCESS : USER_PROCESS;
#if !defined(linux) && !defined(__FreeBSD__)
    ut.ut_exit.e_exit = 2;
#endif
    strncpy (ut.ut_user, (pwent && pwent->pw_name) ? pwent->pw_name : "?",
	     sizeof (ut.ut_user));
    strncpy (ut.ut_id, ut_id, sizeof (ut.ut_id));
    strncpy (ut.ut_line, pty, sizeof (ut.ut_line));

#if (defined(HAVE_UTMP_HOST) && ! defined(RXVT_UTMP_AS_UTMPX)) || (defined(HAVE_UTMPX_HOST) && defined(RXVT_UTMP_AS_UTMPX))
    strncpy (ut.ut_host, tc->hostname, sizeof (ut.ut_host));
# ifndef linux
    if ((colon = strrchr (ut.ut_host, ':')) != NULL)
	*colon = '\0';
# endif
#endif

#ifndef __FreeBSD__
/* ut_name is normally the same as ut_user, but .... */
    strncpy (ut.ut_name, (pwent && pwent->pw_name) ? pwent->pw_name : "?",
	     sizeof (ut.ut_name));
#endif

    ut.ut_pid = tc->pid;

#ifdef RXVT_UTMP_AS_UTMPX
#ifndef __FreeBSD__
    ut.ut_session = tc->sid;
#endif
    ut.ut_tv.tv_sec = tc->now;
    ut.ut_tv.tv_usec = 0;
#else
    ut.ut_time = tc->now;
#endif				/* HAVE_UTMPX_H */

    pututline (&ut);

#ifdef WTMP_SUPPORT
    update_wtmp (RXVT_WTMP_FILE, &ut);
#endif

    endutent ();		/* close the file */
#endif				/* !USE_SYSV_UTMP */
}

#endif				/* UTMP_SUPPORT */

static void cterminal_to_utmpcommand (int cmd, struct wtmp_command *c, struct cterminal *o, const char *hostname)
{E_
    assert (o->ttydev);
    assert (o->ttydev[0]);
    memset (c, '\0', sizeof (*c));
    c->ct = *o;
    c->ct.ttydev = NULL;        /* cannot pass pointers through a pipe */
    c->cmd = cmd;
    strncpy (c->hostname, hostname, sizeof (c->hostname));
    c->hostname[sizeof (c->hostname) - 1] = '\0';
    strncpy (c->ttydev, o->ttydev, sizeof (c->ttydev));
    c->ttydev[sizeof (c->ttydev) - 1] = '\0';
    c->sid = getsid (0);
    c->pid = getpid ();
    time (&c->now);
}

static void cterminal_makeutent (struct cterminal *o, const char *hostname)
{E_
    struct wtmp_command c;
    if (utmp_pipe[1] == -1)
        return;
    cterminal_to_utmpcommand (UTMP_CMD_MAKE, &c, o, hostname);
    if (write (utmp_pipe[1], &c, sizeof (c)) != sizeof (c))
        exit (1);
}

static void cterminal_cleanutent (struct cterminal *o)
{E_
    struct wtmp_command c;
    if (utmp_pipe[1] == -1)
        return;
    cterminal_to_utmpcommand (UTMP_CMD_CLEAN, &c, o, "");
    if (write (utmp_pipe[1], &c, sizeof (c)) != sizeof (c))
        exit (1);
}

static void cterminal_setttyowner (struct cterminal *o)
{E_
    struct wtmp_command c;
    if (utmp_pipe[1] == -1)
        return;
    cterminal_to_utmpcommand (UTMP_CMD_SETTTYOWNER, &c, o, "");
    if (write (utmp_pipe[1], &c, sizeof (c)) != sizeof (c))
        exit (1);
}

void cterminal_fork_utmp_manager (void)
{
    int res = 0;
    pid_t p;
    if (pipe (utmp_pipe)) {
        perror ("pipe");
        utmp_pipe[0] = -1;
        utmp_pipe[1] = -1;
        return;
    }
    p = fork ();
    if (p < 0) {
        perror ("fork");
        return;
    }
    if (p) {
        if (getuid () != geteuid () || getgid () != getegid ())
            printf ("dropping setuid and setgid privileges\n");
        if (seteuid (getuid ())) {
            perror ("seteuid");
            exit (1);
        }
        if (setegid (getgid ())) {
            perror ("setegid");
            exit (1);
        }
        return;
    }
    /* retain privileges */
    if (!p) {
        struct group *gr;       /* change ownership of tty to real uid, "tty" gid */
        gid_t gid;
        uid_t uid;
        unsigned int mode = 0622;
        gid = getgid ();
        uid = getuid ();
        if ((gr = getgrnam ("tty"))) {
            gid = gr->gr_gid;
            mode = 0620;
        }
        for (;;) {
            struct wtmp_command c;
            if (read (utmp_pipe[0], &c, sizeof (c)) != sizeof (c)) {
                perror ("utmp commander, read");
                exit (1);
            }
            switch (c.cmd) {
            case UTMP_CMD_MAKE:
                cterminal_makeutent_ (&c, 0);
                break;
            case UTMP_CMD_CLEAN:
                if (c.ct.changettyowner) {
                    res += chmod (c.ttydev, c.ct.ttyfd_stat.st_mode & 0x777);                   /* fail silently, prevent warning */
                    res += chown (c.ttydev, c.ct.ttyfd_stat.st_uid, c.ct.ttyfd_stat.st_gid);    /* fail silently, prevent warning */
                }
                if (c.ct.clean_utmp)
                    cterminal_makeutent_ (&c, 1);
                break;
            case UTMP_CMD_SETTTYOWNER:
                /* change ownership of tty to real uid and real group */
                res += chown (c.ttydev, uid, gid);      /* fail silently, prevent warning */
                res += chmod (c.ttydev, mode);          /* fail silently, prevent warning */
                break;
            default:
                fprintf (stderr, "utmp invalid command %d\n", c.cmd);
                exit (res);
            }
        }
    }
}

#endif          /* UTMP_SUPPORT */
#endif          /* !MSWIN */


