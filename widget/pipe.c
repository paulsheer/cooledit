/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* pipe.c - for opening a process as a pipe and reading both stderr and stdout together
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include <config.h>
#include "global.h"
#include "pipe-headers.h"
#include <my_string.h>

#include "pool.h"


#ifndef _PATH_DEV
#define _PATH_DEV "/dev/"
#endif

#define _PATH_DEV_PTYXX _PATH_DEV "ptyXX"

int data_read_ready (int f);
int data_read_wait (int f);
int data_write_ready (int f);
int data_write_wait (int f);
pid_t triple_pipe_open (int *in, int *out, int *err, int mix, const char *file, char *const argv[]);
char *read_pipe (int fd, int *len, pid_t *child_pid);

#undef min
#define min(x,y)     (((x) < (y)) ? (x) : (y))
#undef max
#define max(x,y)     (((x) > (y)) ? (x) : (y))

void set_signal_handlers_to_default (void)
{E_
    signal (SIGHUP, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGINT, SIG_DFL);
    signal (SIGTERM, SIG_DFL);
    signal (SIGABRT, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);
    signal (SIGALRM, SIG_IGN);
}

/* returns non-zero if the file exists in the PATH. mimicks the behavior
of execvp */
int PATH_search (const char *file)
{E_
    int fd;
    if (strchr (file, '/')) {
	if ((fd = open (file, O_RDONLY)) >= 0) {
	    close (fd);
	    return 1;
	}
    } else {
	char *path, *p, *name;
	int len;
	if (!(path = getenv ("PATH")))
	    return 0;
	len = strlen ((char *) file) + 1;
	name = (char *) malloc (strlen (path) + len + 1);
	p = path;
	do {
	    path = p;
	    if (!(p = strchr (path, ':')))
		p = path + strlen (path);
	    if (p == path) {	/* double colons */
		strcpy (name, (char *) file);
	    } else {
		memcpy (name, path, (int) (p - path));
		strcpy (&name[(int) (p - path)], "/");
		strcat (name, (char *) file);
	    }
	    if ((fd = open (name, O_RDONLY)) >= 0) {
		free (name);
		close (fd);
		return 1;
	    }
	} while (*p++);
	free (name);
    }
    return 0;
}


/*
   This opens a process as a pipe. 'in', 'out' and 'err' are pointers to file handles
   which are filled in by popen. 'in' refers to stdin of the process, to
   which you can write. 'out' and 'err' refer to stdout and stderr of the process
   from which you can read. 'in', 'out' and 'err' can be passed
   as NULL if you want to ignore output or input of those pipes.
   If 'mix' is non-zero, then both stderr and stdout of the process
   can be read from 'out'. If mix is non-zero, then 'err' must be passed as NULL.
   Popen forks and then calls execvp (see execvp(3)) --- which must also take argv[0]
   and args must terminate with a NULL.
   Returns -1 if the fork failed, and -2 if pipe() failed.
   Otherwise returns the pid of the child.
 */
pid_t triple_pipe_open_env (int *in, int *out, int *err, int mix, const char *file, char *const argv[], char *const envp[])
{E_
    pid_t p;
    int e;
    int f0[2], f1[2], f2[2];

/* we must at least check if the file is in the PATH and is readable.
catching this basic condition helps a lot even though an exec may still
fail for other reasons: */
    if (!PATH_search (file)) {
	errno = ENOENT;
	return -1;
    }

    e = (pipe (f0) | pipe (f1) | pipe (f2));

    if (e) {
	close (f0[0]);
	close (f0[1]);
	close (f1[0]);
	close (f1[1]);
	close (f2[0]);
	close (f2[1]);
	return -2;
    }

    p = fork ();

    if (p == -1) {
	close (f0[0]);
	close (f0[1]);
	close (f1[0]);
	close (f1[1]);
	close (f2[0]);
	close (f2[1]);
	return -1;
    }
    if (p) {
	if (in) {
	    *in = f0[1];
	} else {
	    close (f0[1]);
	}
	if (out) {
	    *out = f1[0];
	} else {
	    close (f1[0]);
	}
	if (err) {
	    *err = f2[0];
	} else {
	    close (f2[0]);
	}
	close (f0[0]);
	close (f1[1]);
	close (f2[1]);
	return p;
    } else {
	int nulldevice_wr, nulldevice_rd;

	nulldevice_wr = open ("/dev/null", O_WRONLY);
	nulldevice_rd = open ("/dev/null", O_RDONLY);

	close (0);
	if (in) {
	    if (dup (f0[0]) == -1)
                exit (1);
	} else {
	    if (dup (nulldevice_rd) == -1)
                exit (1);
        }
	close (1);
	if (out) {
	    if (dup (f1[1]) == -1)
                exit (1);
	} else {
	    if (dup (nulldevice_wr) == -1)
                exit (1);
        }
	close (2);
	if (err) {
	    if (dup (f2[1]) == -1)
                exit (1);
	} else {
	    if (mix) {
		if (dup (f1[1]) == -1)
                    exit (1);
	    } else {
		if (dup (nulldevice_wr) == -1)
                    exit (1);
            }
	}
	close (f0[0]);
	close (f0[1]);
	close (f1[0]);
	close (f1[1]);
	close (f2[0]);
	close (f2[1]);

	close (nulldevice_rd);
	close (nulldevice_wr);
	set_signal_handlers_to_default ();
        if (envp)
	    execvpe (file, argv, envp);
        else
	    execvp (file, argv);
	exit (1);
    }
    return 0; /* prevents warning */
}

pid_t triple_pipe_open (int *in, int *out, int *err, int mix, const char *file, char *const argv[])
{E_
    return triple_pipe_open_env (in, out, err, mix, file, argv, NULL);
}

#if 0
#ifndef HAVE_FORKPTY

/* This is stolen from libc-5.4.46 with the intention of having
   this all still work on systems that do not have the forkpty()
   function (like non-BSD systems like solaris). No idea if it will
   actually work on these systems. */

static int my_openpty (int *amaster, int *aslave, char *name)
{E_
    char line[] = _PATH_DEV_PTYXX;
    const char *p, *q;
    int master, slave, ttygid;
    struct group *gr;
    if ((gr = getgrnam ("tty")) != NULL)
	ttygid = gr->gr_gid;
    else
	ttygid = -1;
    for (p = "pqrstuvwxyzabcde"; *p; p++) {
	line[sizeof (_PATH_DEV_PTYXX) - 3] = *p;
	for (q = "0123456789abcdef"; *q; q++) {
	    line[sizeof (_PATH_DEV_PTYXX) - 2] = *q;
	    if ((master = open (line, O_RDWR, 0)) == -1) {
		if (errno == ENOENT)
		    return -1;
	    } else {
		line[sizeof (_PATH_DEV) - 1] = 't';
		chown (line, getuid (), ttygid);
		chmod (line, S_IRUSR | S_IWUSR | S_IWGRP);
		if ((slave = open (line, O_RDWR, 0)) != -1) {
		    *amaster = master;
		    *aslave = slave;
		    strcpy (name, line);
		    return 0;
		}
		close (master);
		line[sizeof (_PATH_DEV) - 1] = 'p';
	    }
	}
    }
    errno = ENOENT;
    return -1;
}

static int forkpty (int *amaster, char *name,...)
{E_
    int master, slave;
    pid_t pid;
    if (my_openpty (&master, &slave, name) == -1)
	return -1;
    switch (pid = fork ()) {
    case -1:
	return -1;
    case 0:
	close (master);
#ifdef HAVE_SETSID
	pid = setsid ();
#define HAVE_PID_THIS_TTY
#elif defined (HAVE_SETPGRP)
	pid = setpgrp (0, 0);
#define HAVE_PID_THIS_TTY
#endif
#ifdef TIOCSCTTY
	ioctl (slave, TIOCSCTTY, 0);
#ifdef HAVE_PID_THIS_TTY
#elif defined (HAVE_TCSETPGRP)
	tcsetpgrp (slave, pid);
#elif defined (TIOCSPGRP)
	ioctl (slave, TIOCSPGRP, &pid);
#endif
#endif
	close (0);
	dup (slave);
	close (1);
	dup (slave);
	close (2);
	dup (slave);
	if (slave > 2)
	    close (slave);
	return 0;
    }
    *amaster = master;
    close (slave);
    return pid;
}

#endif				/* ! HAVE_FORKPTY */

static void set_termios (int fd)
{E_
#ifdef HAVE_TCGETATTR
    struct termios tios;
    memset (&tios, 0, sizeof (tios));
    if (tcgetattr (fd, &tios) != 0)
	return;
#ifdef B19200
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
    tcsetattr (fd, TCSADRAIN, &tios);
#endif
#endif				/* HAVE_TCGETATTR */
}

pid_t open_under_pty (int *in, int *out, char *line, const char *file, char *const argv[])
{E_
    int master = 0;
    char l[80];
    pid_t p;
#ifdef HAVE_FORKPTY
#ifdef NEED_WINSIZE
    struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
    } win;
#else
    struct winsize win;
#endif
    memset (&win, 0, sizeof (win));
    p = (pid_t) forkpty (&master, l, NULL, &win);
#else
    p = (pid_t) forkpty (&master, l);
#endif
    if (p == -1)
	return -1;
#if 0
    ioctl (master, FIONBIO, &yes);
    ioctl (master, FIONBIO, &yes);
#endif
    strcpy (line, l);
    if (p) {
	*in = dup (master);
	*out = dup (master);
	close (master);
	return p;
    }
    set_termios (0);
    execvp (file, argv);
    exit (1);
    return 0;
}
#endif

#define CHUNK 8192

int CChildExitted (pid_t p, int *status);

#define ERROR_EAGAIN()          (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS)

/*
   Reads all available data and mallocs space for it plus
   one byte, and sets that byte to zero. If len is non-NULL
   bytes read is placed in len. If *len is passed non-zero then
   reads that amount max.
 */
char *read_pipe (int fd, int *len, pid_t *child_pid)
{E_
    POOL *p;
    int c, count = 0;
    int l = CHUNK;
    p = pool_init ();
    if (len)
	if (*len)
	    if (l > *len)
		l = *len;
    for (;;) {
	if (pool_freespace (p) < l + 1)
	    pool_advance (p, l + 1);
	for (;;) {
	    c = read (fd, pool_current (p), l);
            if (c < 0 && ERROR_EAGAIN()) {
                fd_set rd;
                if (child_pid && CChildExitted (*child_pid, 0))
                    break;
                FD_ZERO (&rd);
                FD_SET (fd, &rd);
                select (fd + 1, &rd, NULL, NULL, NULL);
                continue;
            }
            break;
	}
	if (c <= 0)
	    break;
	count += c;
	pool_current (p) += c;
	if (len)
	    if (*len)
		if (pool_length (p) >= l)
		    break;
    }
    pool_null (p);
    if (len)
	*len = pool_length (p);
    return (char *) pool_break (p);
}

int read_two_pipes (int fd1, int fd2, char **r1, int *len1, char **r2, int *len2, pid_t *child_pid)
{E_
    POOL *p1, *p2;

    p1 = pool_init ();
    p2 = pool_init ();

    for (;;) {
        fd_set rd;
        int n, m = -1;

        FD_ZERO (&rd);
        if (fd1 != -1) {
            FD_SET (fd1, &rd);
            m = max (m, fd1);
        }
        if (fd2 != -1) {
            FD_SET (fd2, &rd);
            m = max (m, fd2);
        }

        if (m == -1)
            break;

        n = select (m + 1, &rd, 0, 0, 0);

        if (n < 0 && errno == EINTR)
            continue;

        if (n < 0)
            break;

        if (fd1 != -1 && FD_ISSET (fd1, &rd)) {
            int c;
	    if (pool_freespace (p1) < CHUNK + 1)
		pool_advance (p1, CHUNK + 1);
	    c = read (fd1, pool_current (p1), CHUNK);
            if (c <= 0)
                fd1 = -1;
            else
	        pool_current (p1) += c;
        }

        if (fd2 != -1 && FD_ISSET (fd2, &rd)) {
            int c;
	    if (pool_freespace (p2) < CHUNK + 1)
		pool_advance (p2, CHUNK + 1);
	    c = read (fd2, pool_current (p2), CHUNK);
            if (c <= 0)
                fd2 = -1;
            else
	        pool_current (p2) += c;
        }
    }

    pool_null (p1);
    if (len1)
	*len1 = pool_length (p1);
    *r1 = (char *) pool_break (p1);

    pool_null (p2);
    if (len2)
	*len2 = pool_length (p2);
    *r2 = (char *) pool_break (p2);

    return 0;
}







