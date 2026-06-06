/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* remotefs.c - remote fs access
   Copyright (C) 1996-2022 Paul Sheer
 */

#include "inspect.h"
#include "global.h"
#ifdef MSWIN
#include <config-mswin.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <error.h>
#else
#include <config.h>
#endif

#include <assert.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#include <my_string.h>
#include "stringtools.h"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_IOCTL_H
#include <ioctl.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifndef MSWIN
#include <sys/socket.h>
#include <sys/signal.h>
#ifndef __FreeBSD__
#include <sys/sysmacros.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
#endif

#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if defined(__sun) || defined(__sun__)
#include <sys/filio.h>
#endif

#define SHELL_SUPPORT
#ifndef MSWIN
#define SOUND_FWD
#include "soundfwd.h"
#endif

#include "remotefs.h"
#include "dirtools.h"
#include "aes.h"
#include "sha256.h"
#include "symauth.h"
#ifdef MSWIN
#include "mswinchild.h"
#else
#include "cterminal.h"
#endif
#include "childhandler.h"
#include "remotefs_local.h"



#ifdef SOUND_FWD


#define ERROR_EINTR()           (errno == EINTR)
#define ERROR_EAGAIN()          (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS)


#undef MIN
#undef MAX
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#define MAX(a,b)        ((a) > (b) ? (a) : (b))



struct soundclient_ {
    unsigned char *buf;
    int avail;
    int written;
    int alloced;
};

struct soundclient_data_item {
    unsigned long sound_client_id;
    SOCKET sock;
    struct remotefs *rfs;
    struct soundclient_data_item *next;
    struct soundclient_ rd;
    struct soundclient_ wr;
    int kill;

#define SOUNDFWDCLIENT_STATE_CONNECTING             1
#define SOUNDFWDCLIENT_STATE_RUNNING                2
    int state;
};

struct soundclient_data {
    remotefs_sockaddr_t sound_peer;
    struct soundclient_data_item *soundclient_list;
};

static add_watch_cb_f add_watch_cb = NULL;
static remove_watch_cb_f remove_watch_cb = NULL;

void soundclient_set_watch (add_watch_cb_f a, remove_watch_cb_f b)
{E_
    add_watch_cb = a;
    remove_watch_cb = b;
}

static void soundclient_free (struct soundclient_data_item *i)
{E_
    (*remove_watch_cb) (i->sock, NULL, 3);
    shutdown (i->sock, 2);
    close (i->sock);
    free (i->rd.buf);
    free (i->wr.buf);
    free (i);
}

void soundclient_kill (struct soundclient_data *x, unsigned long sound_client_id)
{E_
    struct soundclient_data_item **p;
    for (p = &x->soundclient_list; *p;) {
        struct soundclient_data_item *i;
        i = *p;
        if (i->sound_client_id == sound_client_id || i->kill) {
            *p = (*p)->next;
            soundclient_free (i);
            continue;
        } else {
            p = &(*p)->next;
        }
    }
}

void soundclient_freeall (struct soundclient_data *x)
{E_
    struct soundclient_data_item *i, *next;
    for (i = x->soundclient_list; i; i = next) {
        next = i->next;
        soundclient_free (i);
    }
    memset (x, '\0', sizeof (*x));
    free (x);
}

struct soundclient_data *soundclient_alloc (const char *sound_env_var)
{E_
    struct soundclient_data *r;
    char host[256];
    socklen_t l;
    const char *addr;
    int port;

    r = (struct soundclient_data *) malloc (sizeof (*r));
    memset (r, '\0', sizeof (*r));

    l = sizeof (r->sound_peer);
    memset (&r->sound_peer, '\0', sizeof (r->sound_peer));

    addr = NULL;
    port = 0;

    if (sound_env_var && *sound_env_var) {
        if (!strncmp (sound_env_var, "unix:", 5)) {
            addr = sound_env_var + 5;
        } else if (!strncmp (sound_env_var, "tcp:", 4)) {
            const char *colon;
            int n;
            sound_env_var += 4;
            colon = strchr (sound_env_var, ':');
            if (colon) {
                n = colon - sound_env_var;
                if (n > (int) sizeof (host) - 1)
                    n = sizeof (host) - 1;
                memcpy (host, sound_env_var, n);
                host[n] = '\0';
                addr = host;
                port = atoi (colon + 1);
            } else {
                addr = sound_env_var;
            }
        } else if (sound_env_var[0] == '/') {
            addr = sound_env_var;
        } else {
            const char *colon;
            int n;
            colon = strchr (sound_env_var, ':');
            if (colon) {
                n = colon - sound_env_var;
                if (n > (int) sizeof (host) - 1)
                    n = sizeof (host) - 1;
                memcpy (host, sound_env_var, n);
                host[n] = '\0';
                addr = host;
                port = atoi (colon + 1);
            } else {
                addr = sound_env_var;
            }
        }
    } else {
        snprintf (host, sizeof (host), "/run/user/%d/pulse/native", (int) getuid ());
        addr = host;
    }

    if (addr)
        if (!ipaddress_port_to_remotefs_sockaddr_t (&r->sound_peer, addr, port))
            return r;

    if (sound_env_var && *sound_env_var)
        fprintf (stderr, "Bad pulse audio setting '%s'. Could be environment\n\
                          variable PULSE_SERVER or .Xdefaults/ or .Xresources\n\
                          setting. BTW I won't resolve hostnames.\n", sound_env_var);

    free (r);
    return NULL;
}

void soundclient_read_watch (SOCKET sock, fd_set *rd, fd_set *wr, fd_set *er, void *o)
{E_
    int c;
    struct soundclient_data_item *n;
    n = (struct soundclient_data_item *) o;

    assert (n->rd.alloced > n->rd.avail);
    c = recv (sock, n->rd.buf + n->rd.avail, n->rd.alloced - n->rd.avail, 0);

    if (c < 0 && (ERROR_EAGAIN () || ERROR_EINTR ())) {
        /* ok */
    } else if (c <= 0) {
        (*remove_watch_cb) (n->sock, NULL, 3);
        send_blind_message (remotefs_get_sock_data (n->rfs), REMOTEFS_ACTION_SHELLWRITE, n->sound_client_id, SOUNDSTATUS_SHUTDOWN, "", 0, NULL, 0);
        n->kill = 1;
    } else {
        int l;
        n->rd.avail += c;

        l = n->rd.avail - n->rd.written;
        if (send_blind_message (remotefs_get_sock_data (n->rfs), REMOTEFS_ACTION_SHELLWRITE, n->sound_client_id, SOUNDSTATUS_DATA, (char *) (n->rd.buf + n->rd.written), l, NULL, 0)) {
            perror ("send");
            n->kill = 1;
        }
        n->rd.written += l;
        if (n->rd.written == n->rd.avail)
            n->rd.written = n->rd.avail = 0;
        if (n->rd.avail == n->rd.alloced)       /* <== does not happen */
            (*remove_watch_cb) (n->sock, soundclient_read_watch, 1);
    }
}


void soundclient_write_watch (SOCKET sock, fd_set *rd, fd_set *wr, fd_set *er, void *o)
{E_
    struct soundclient_data_item *n;
    n = (struct soundclient_data_item *) o;

    if (n->state == SOUNDFWDCLIENT_STATE_CONNECTING) {
        int r;
        r = remotefs_connection_check (n->sock, 1);
        if (r == CONNCHECK_SUCCESS) {
            n->state = SOUNDFWDCLIENT_STATE_RUNNING;
            (*add_watch_cb) (__FILE__, __LINE__, n->sock, soundclient_read_watch, 1, (void *) n);
            goto running;
        }
        if (r == CONNCHECK_WAITING) {
            /* ok */
        }
        if (r == CONNCHECK_ERROR) {
            (*remove_watch_cb) (n->sock, NULL, 3);
            n->kill = 1;
        }
    } else {
        int c;
      running:
        assert (n->wr.avail > n->wr.written);
        c = send (sock, n->wr.buf + n->wr.written, n->wr.avail - n->wr.written, 0);
        if (c < 0 && (ERROR_EAGAIN () || ERROR_EINTR ())) {
            /* ok */
        } else if (c <= 0) {
            (*remove_watch_cb) (n->sock, NULL, 3);
            send_blind_message (remotefs_get_sock_data (n->rfs), REMOTEFS_ACTION_SHELLWRITE, n->sound_client_id, SOUNDSTATUS_SHUTDOWN, "", 0, NULL, 0);
            n->kill = 1;
        } else {
            n->wr.written += c;
            if (n->wr.written == n->wr.avail) {
                n->wr.written = n->wr.avail = 0;
                (*remove_watch_cb) (n->sock, soundclient_write_watch, 2);
            }
        }
    }
}

void soundclient_write (struct soundclient_data *x, unsigned long sound_client_id, const char *buf, int buflen)
{E_
    struct soundclient_data_item *i;
    for (i = x->soundclient_list; i; i = i->next) {
        if (i->sound_client_id == sound_client_id) {
            if (i->wr.avail + buflen > i->wr.alloced) {
                i->wr.buf = (unsigned char *) realloc (i->wr.buf, i->wr.avail + buflen);
                i->wr.alloced = i->wr.avail + buflen;
            }
            memcpy (i->wr.buf + i->wr.avail, buf, buflen);
            i->wr.avail += buflen;
            if (i->state == SOUNDFWDCLIENT_STATE_RUNNING)
                soundclient_write_watch (i->sock, NULL, NULL, NULL, (void *) i);
            if (i->wr.avail > i->wr.written)
                if (add_watch_cb)
                    (*add_watch_cb) (__FILE__, __LINE__, i->sock, soundclient_write_watch, 2, (void *) i);
            return;
        }
    }
}

int soundclient_new_client (struct remotefs *rfs, struct soundclient_data *x, unsigned long sound_client_id)
{E_
    SOCKET sock;
    struct soundclient_data_item *n, *i;
    int r;
    int yes = 1;
    int connected = 0;

    for (i = x->soundclient_list; i; i = i->next)
        if (i->sound_client_id == sound_client_id)
            return 0;

#warning if xwin s NULL we should not be allowed to get here
    assert (x);
    sock = socket (remotefs_sockaddr_t_addressfamily (&x->sound_peer), SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        return -1;
    if (setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof (yes)))
        perror ("setsockopt(TCP_NODELAY)");
    if (ioctlsocket (sock, FIONBIO, &yes))
        perror ("ioctlsocket(FIONBIO)");

    r = connect (sock, (struct sockaddr *) &x->sound_peer, remotefs_sockaddr_t_socksz (&x->sound_peer));
    if (r == SOCKET_ERROR && (ERROR_EINTR () || ERROR_EAGAIN ())) {
        /* ok */
    } else if (r == SOCKET_ERROR) {
        perror ("connect()");
        shutdown (sock, 2); close (sock);
        return -1;
    } else {
        connected = 1;
    }

    n = (struct soundclient_data_item *) malloc (sizeof (*n));
    memset (n, '\0', sizeof (*n));
    n->sock = sock;
    n->rfs = rfs;
    n->sound_client_id = sound_client_id;
    n->rd.buf = (unsigned char *) malloc (TERMINAL_TCP_BUF_SIZE);
    n->rd.alloced = TERMINAL_TCP_BUF_SIZE;
    n->wr.buf = (unsigned char *) malloc (TERMINAL_TCP_BUF_SIZE);
    n->wr.alloced = TERMINAL_TCP_BUF_SIZE;

    if (connected) {
        if (add_watch_cb)
            (*add_watch_cb) (__FILE__, __LINE__, n->sock, soundclient_read_watch, 1, (void *) n);
    } else {
        n->state = SOUNDFWDCLIENT_STATE_CONNECTING;
        if (add_watch_cb)
            (*add_watch_cb) (__FILE__, __LINE__, n->sock, soundclient_write_watch, 2, (void *) n);
    }

    n->next = x->soundclient_list;
    x->soundclient_list = n;

    return 0;
}


/* for security we don't want a TCP socket */
#define PULSE_UNIX_SOCKET

struct soundfwd_ {
    unsigned char *buf;
    int avail;
    int written;
    int alloced;
};

static unsigned long sound_client_id = 1;

struct soundfwd_data_item {
    unsigned long sound_client_id;
    SOCKET sock;
    struct soundfwd_data_item *next;
    struct soundfwd_ rd;
    struct timeval lastwrite;
    int didread;
    int kill;
    struct soundfwd_ wr;
};

struct soundfwd_data {
    SOCKET listen_sock;
#ifdef PULSE_UNIX_SOCKET
    char unix_path[256];
    char pulseaudio_config_path[256];
#else
    int listen_port;
#endif
    struct soundfwd_data_item *soundfwd_list;
};

void soundfwd_construct_envvar (struct soundfwd_data *x, char *e1, int l1, char *e2, int l2)
{E_
#ifdef PULSE_UNIX_SOCKET
    snprintf (e1, l1, "unix:%s", x->unix_path);
    snprintf (e2, l2, "%s", x->pulseaudio_config_path);
#else
    snprintf (e1, l1, "tcp:127.0.0.1:%d", x->listen_port);
    snprintf (e2, l2, "");
#endif
}

int soundfwd_listen_socket (struct soundfwd_data *x)
{E_
    return x->listen_sock;
}

struct soundfwd_data *soundfwd_alloc (void)
{E_
    struct soundfwd_data *x;
    const char *home;
    mode_t old_umask;
    int i;

    x = (struct soundfwd_data *) malloc (sizeof (*x));
    memset (x, '\0', sizeof (*x));

    home = getenv ("HOME");
    if (!home)
        home = "/tmp";

#ifdef PULSE_UNIX_SOCKET
    old_umask = umask (0077);
    snprintf (x->pulseaudio_config_path, sizeof (x->pulseaudio_config_path), "%s/.cooledit-pulseaudio.conf", home);
    if (access (x->pulseaudio_config_path, R_OK)) {
        FILE *f;
        f = fopen(x->pulseaudio_config_path, "w");
        if (f) {
            fprintf (f, "enable-shm = no\nenable-memfd = no\n");
            fclose (f);
        }
    }
    for (i = 0; i < 50; i++) {
        snprintf (x->unix_path, sizeof (x->unix_path), "%s/.cooledit-%d-pulseaudio-fwd", home, i);
        x->listen_sock = remotefs_listen_socket (x->unix_path, 0);
        if (x->listen_sock != INVALID_SOCKET) {
            umask (old_umask);
            return x;
        }
    }
    umask (old_umask);
#else
    for (i = 0; i < 50; i++) {
        x->listen_sock = remotefs_listen_socket ("127.0.0.1", 4725 + i);
        if (x->listen_sock != INVALID_SOCKET) {
            umask (old_umask);
            x->listen_port = 4725 + i;
            return x;
        }
    }
#endif
    free (x);
    return NULL;
}

int soundfwd_new_client (struct soundfwd_data *x)
{E_
    SOCKET sock;
    struct soundfwd_data_item *n;
    socklen_t l;
    remotefs_sockaddr_t client_address;
    int yes = 1;

    l = sizeof (client_address);
    sock = accept (x->listen_sock, (struct sockaddr *) &client_address, &l);

    if (sock == INVALID_SOCKET) {
        perror ("accept fail");
        return -1;
    }

    if (setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof (yes)))
        perror ("setsockopt(TCP_NODELAY)");
    if (ioctlsocket (sock, FIONBIO, &yes))
        perror ("ioctlsocket(FIONBIO)");

    n = (struct soundfwd_data_item *) malloc (sizeof (*n));
    memset (n, '\0', sizeof (*n));
    n->sock = sock;
    n->sound_client_id = sound_client_id++;
    n->rd.buf = (unsigned char *) malloc (TERMINAL_TCP_BUF_SIZE);
    n->rd.alloced = TERMINAL_TCP_BUF_SIZE;
    n->wr.buf = (unsigned char *) malloc (TERMINAL_TCP_BUF_SIZE);
    n->wr.alloced = TERMINAL_TCP_BUF_SIZE;
    n->didread = 1; /* startup has not set lastwrite */

    n->next = x->soundfwd_list;
    x->soundfwd_list = n;

    printf ("Pulseaudio connection established\n");

    return 0;
}

void soundfwd_prep_sockets (struct soundfwd_data *x, fd_set * rd, fd_set * wr, int *n)
{E_
    struct soundfwd_data_item *i;
    for (i = x->soundfwd_list; i; i = i->next) {
        if (i->sock != INVALID_SOCKET && i->rd.avail < i->rd.alloced) {
            FD_SET (i->sock, rd);
            *n = MAX (*n, i->sock);
        }
        if (i->sock != INVALID_SOCKET && i->wr.written < i->wr.avail) {
            FD_SET (i->sock, wr);
            *n = MAX (*n, i->sock);
        }
    }
}

void soundfwd_write (struct soundfwd_data *x, unsigned long sound_client_id, const char *buf, int buflen)
{E_
    struct soundfwd_data_item *i;
    for (i = x->soundfwd_list; i; i = i->next) {
        if (i->sound_client_id == sound_client_id) {
            if (i->wr.avail + buflen > i->wr.alloced) {
                i->wr.buf = (unsigned char *) realloc (i->wr.buf, i->wr.avail + buflen);
                i->wr.alloced = i->wr.avail + buflen;
            }
            memcpy (i->wr.buf + i->wr.avail, buf, buflen);
            i->wr.avail += buflen;
            return;
        }
    }
}

static void soundfwd_free (struct soundfwd_data_item *i)
{E_
    shutdown (i->sock, 2);
    close (i->sock);
    free (i->rd.buf);
    free (i->wr.buf);
    free (i);
}

int soundfwd_process_sockets (struct sock_data *sock_data, struct soundfwd_data *x, fd_set * rd, fd_set * wr)
{E_
    struct soundfwd_data_item **p;
    for (p = &x->soundfwd_list; *p;) {
        struct soundfwd_data_item *i;
        int c;
        i = *p;
        if ((*p)->kill) {
            *p = (*p)->next;
            soundfwd_free (i);
            continue;
        } else {
            p = &(*p)->next;
        }
        if (i->sock != INVALID_SOCKET && FD_ISSET (i->sock, rd)) {
            c = recv (i->sock, i->rd.buf + i->rd.avail, i->rd.alloced - i->rd.avail, 0);
            if (c < 0 && (ERROR_EAGAIN () || ERROR_EINTR ())) {
                /* ok */
            } else if (c <= 0) {
                if (send_blind_message (sock_data, REMOTEFS_ACTION_SHELLREAD, i->sound_client_id, SOUNDSTATUS_SHUTDOWN, "", 0, NULL, 0))
                    return -1;
                i->kill = 1;
            } else {
                i->rd.avail += c;
            }
        }
        if (i->sock != INVALID_SOCKET && FD_ISSET (i->sock, wr)) {
            c = send (i->sock, i->wr.buf + i->wr.written, i->wr.avail - i->wr.written, 0);
            if (c < 0 && (ERROR_EAGAIN () || ERROR_EINTR ())) {
                /* ok */
            } else if (c <= 0) {
                if (send_blind_message (sock_data, REMOTEFS_ACTION_SHELLREAD, i->sound_client_id, SOUNDSTATUS_SHUTDOWN, "", 0, NULL, 0))
                    return -1;
                i->kill = 1;
            } else {
                i->wr.written += c;
                if (i->wr.written == i->wr.avail)
                    i->wr.written = i->wr.avail = 0;
            }
        }
        if (i->rd.avail > i->rd.written) {
            int l = i->rd.avail - i->rd.written;
// printf("send_blind_message %d\n", (int) (i->rd.avail - i->rd.written));
            if (send_blind_message (sock_data, REMOTEFS_ACTION_SHELLREAD, i->sound_client_id, SOUNDSTATUS_DATA, (char *) (i->rd.buf + i->rd.written), i->rd.avail - i->rd.written, NULL, 0))
                return -1;
            i->rd.written += l;
            if (i->rd.written == i->rd.avail)
                i->rd.written = i->rd.avail = 0;
        }
    }
    return 0;
}

void soundfwd_kill (struct soundfwd_data *x, unsigned long sound_client_id)
{E_
    struct soundfwd_data_item *i;
    for (i = x->soundfwd_list; i; i = i->next) {
        if (i->sound_client_id == sound_client_id) {
            i->kill = 1;
        }
    }
}

void soundfwd_freeall (struct soundfwd_data *x)
{E_
    struct soundfwd_data_item *i, *next;
    for (i = x->soundfwd_list; i; i = next) {
        next = i->next;
        soundfwd_free (i);
    }
#ifdef PULSE_UNIX_SOCKET
    unlink (x->unix_path);
#endif
    shutdown (x->listen_sock, 2);
    close (x->listen_sock);
    free (x);
}


#endif


