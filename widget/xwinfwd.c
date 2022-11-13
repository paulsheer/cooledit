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
#define XWIN_FWD
#include "xwinfwd.h"
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



#ifdef XWIN_FWD


#define ERROR_EINTR()           (errno == EINTR)
#define ERROR_EAGAIN()          (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS)


#undef MIN
#undef MAX
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#define MAX(a,b)        ((a) > (b) ? (a) : (b))



struct xwinclient_ {E_
    unsigned char *buf;
    int avail;
    int written;
    int alloced;
};

struct xwinclient_data_item {E_
    unsigned long xwin_client_id;
    SOCKET sock;
    struct remotefs *rfs;
    struct xwinclient_data_item *next;
    struct xwinclient_ rd;
    struct xwinclient_ wr;
    int kill;

#define XWINCLIENT_STATE_CONNECTING             1
#define XWINCLIENT_STATE_RUNNING                2
    int state;
};

struct xwinclient_data {E_
    remotefs_sockaddr_t xwin_peer;
    struct xwinclient_data_item *xwinclient_list;
};

static add_watch_cb_f add_watch_cb = NULL;
static remove_watch_cb_f remove_watch_cb = NULL;

void xwinclient_set_watch (add_watch_cb_f a, remove_watch_cb_f b)
{E_
    add_watch_cb = a;
    remove_watch_cb = b;
}

static void xwinclient_free (struct xwinclient_data_item *i)
{E_
    (*remove_watch_cb) (i->sock, NULL, 3);
    shutdown (i->sock, 2);
    close (i->sock);
    free (i->rd.buf);
    free (i->wr.buf);
    free (i);
}

void xwinclient_kill (struct xwinclient_data *x, unsigned long xwin_client_id)
{E_
    struct xwinclient_data_item **p;
    for (p = &x->xwinclient_list; *p;) {
        struct xwinclient_data_item *i;
        i = *p;
        if (i->xwin_client_id == xwin_client_id || i->kill) {
            *p = (*p)->next;
            xwinclient_free (i);
            continue;
        } else {
            p = &(*p)->next;
        }
    }
}

void xwinclient_freeall (struct xwinclient_data *x)
{E_
    struct xwinclient_data_item *i, *next;
    for (i = x->xwinclient_list; i; i = next) {
        next = i->next;
        xwinclient_free (i);
    }
    free (x);
}

struct xwinclient_data *xwinclient_alloc (int xwin_fd)
{E_
    struct xwinclient_data *r;
    socklen_t l;

    r = (struct xwinclient_data *) malloc (sizeof (*r));
    memset (r, '\0', sizeof (*r));

    l = sizeof (r->xwin_peer);
    memset (&r->xwin_peer, '\0', sizeof (r->xwin_peer));
    if (getpeername (xwin_fd, (struct sockaddr *) &r->xwin_peer, &l)) {
        free (r);
        return NULL;
    }
    return r;
}

void xwinclient_read_watch (SOCKET sock, fd_set *rd, fd_set *wr, fd_set *er, void *o)
{E_
    int c;
    struct xwinclient_data_item *n;
    n = (struct xwinclient_data_item *) o;

    assert (n->rd.alloced > n->rd.avail);
    c = recv (sock, n->rd.buf + n->rd.avail, n->rd.alloced - n->rd.avail, 0);
    if (c < 0 && (ERROR_EAGAIN () || ERROR_EINTR ())) {
        /* ok */
    } else if (c <= 0) {
        (*remove_watch_cb) (n->sock, NULL, 3);
        send_blind_message (remotefs_get_sock_data (n->rfs), REMOTEFS_ACTION_SHELLWRITE, n->xwin_client_id, XFWDSTATUS_SHUTDOWN, "", 0, NULL, 0);
        n->kill = 1;
    } else {
        int l;
        n->rd.avail += c;

        l = n->rd.avail - n->rd.written;
        if (send_blind_message (remotefs_get_sock_data (n->rfs), REMOTEFS_ACTION_SHELLWRITE, n->xwin_client_id, XFWDSTATUS_DATA, (char *) (n->rd.buf + n->rd.written), l, NULL, 0)) {
            perror ("send");
            n->kill = 1;
        }
        n->rd.written += l;
        if (n->rd.written == n->rd.avail)
            n->rd.written = n->rd.avail = 0;
        if (n->rd.avail == n->rd.alloced)       /* <== does not happen */
            (*remove_watch_cb) (n->sock, xwinclient_read_watch, 1);
    }
}


void xwinclient_write_watch (SOCKET sock, fd_set *rd, fd_set *wr, fd_set *er, void *o)
{E_
    struct xwinclient_data_item *n;
    n = (struct xwinclient_data_item *) o;

    if (n->state == XWINCLIENT_STATE_CONNECTING) {
        int r;
        r = remotefs_connection_check (n->sock, 1);
        if (r == CONNCHECK_SUCCESS) {
            n->state = XWINCLIENT_STATE_RUNNING;
            (*add_watch_cb) (__FILE__, __LINE__, n->sock, xwinclient_read_watch, 1, (void *) n);
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
            send_blind_message (remotefs_get_sock_data (n->rfs), REMOTEFS_ACTION_SHELLWRITE, n->xwin_client_id, XFWDSTATUS_SHUTDOWN, "", 0, NULL, 0);
            n->kill = 1;
        } else {
            n->wr.written += c;
            if (n->wr.written == n->wr.avail) {
                n->wr.written = n->wr.avail = 0;
                (*remove_watch_cb) (n->sock, xwinclient_write_watch, 2);
            }
        }
    }
}

void xwinclient_write (struct xwinclient_data *x, unsigned long xwin_client_id, const char *buf, int buflen)
{E_
    struct xwinclient_data_item *i;
    for (i = x->xwinclient_list; i; i = i->next) {
        if (i->xwin_client_id == xwin_client_id) {
            if (i->wr.avail + buflen > i->wr.alloced) {
                i->wr.buf = (unsigned char *) realloc (i->wr.buf, i->wr.avail + buflen);
                i->wr.alloced = i->wr.avail + buflen;
            }
            memcpy (i->wr.buf + i->wr.avail, buf, buflen);
            i->wr.avail += buflen;
            if (i->state == XWINCLIENT_STATE_RUNNING)
                xwinclient_write_watch (i->sock, NULL, NULL, NULL, (void *) i);
            if (i->wr.avail > i->wr.written)
                if (add_watch_cb)
                    (*add_watch_cb) (__FILE__, __LINE__, i->sock, xwinclient_write_watch, 2, (void *) i);
            return;
        }
    }
}

int xwinclient_new_client (struct remotefs *rfs, struct xwinclient_data *x, unsigned long xwin_client_id)
{E_
    SOCKET sock;
    struct xwinclient_data_item *n, *i;
    int r;
    int yes = 1;
    int connected = 0;

    for (i = x->xwinclient_list; i; i = i->next)
        if (i->xwin_client_id == xwin_client_id)
            return 0;

#warning if xwin s NULL we should not be allowed to get here
    assert (x);
    sock = socket (remotefs_sockaddr_t_addressfamily (&x->xwin_peer), SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        return -1;
    if (setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof (yes)))
        perror ("setsockopt(TCP_NODELAY)");
    if (ioctlsocket (sock, FIONBIO, &yes))
        perror ("ioctlsocket(FIONBIO)");
    r = connect (sock, (struct sockaddr *) &x->xwin_peer, sizeof (x->xwin_peer));
    if (r == SOCKET_ERROR && (ERROR_EINTR () || ERROR_EAGAIN ())) {
        /* ok */
    } else if (r == SOCKET_ERROR) {
        shutdown (sock, 2);
        close (sock);
        return -1;
    } else {
        connected = 1;
    }

    n = (struct xwinclient_data_item *) malloc (sizeof (*n));
    memset (n, '\0', sizeof (*n));
    n->sock = sock;
    n->rfs = rfs;
    n->xwin_client_id = xwin_client_id;
    n->rd.buf = (unsigned char *) malloc (TERMINAL_TCP_BUF_SIZE);
    n->rd.alloced = TERMINAL_TCP_BUF_SIZE;
    n->wr.buf = (unsigned char *) malloc (TERMINAL_TCP_BUF_SIZE);
    n->wr.alloced = TERMINAL_TCP_BUF_SIZE;

    if (connected) {
        if (add_watch_cb)
            (*add_watch_cb) (__FILE__, __LINE__, n->sock, xwinclient_read_watch, 1, (void *) n);
    } else {
        n->state = XWINCLIENT_STATE_CONNECTING;
        if (add_watch_cb)
            (*add_watch_cb) (__FILE__, __LINE__, n->sock, xwinclient_write_watch, 2, (void *) n);
    }

    n->next = x->xwinclient_list;
    x->xwinclient_list = n;

    return 0;
}


struct xwinfwd_ {
    unsigned char *buf;
    int avail;
    int written;
    int alloced;
};

static unsigned long xwin_client_id = 1;

struct xwinfwd_data_item {
    unsigned long xwin_client_id;
    SOCKET sock;
    struct xwinfwd_data_item *next;
    struct xwinfwd_ rd;
    struct timeval lastwrite;
    int didread;
    int kill;
    struct xwinfwd_ wr;
};

struct xwinfwd_data {
    SOCKET listen_sock;
    int listen_port;
    struct xwinfwd_data_item *xwinfwd_list;
};

int xwinfwd_display_port (struct xwinfwd_data *x)
{E_
    return x->listen_port;
}

int xwinfwd_listen_socket (struct xwinfwd_data *x)
{E_
    return x->listen_sock;
}

struct xwinfwd_data *xwinfwd_alloc (void)
{E_
    int i;
    struct xwinfwd_data *x;
    x = (struct xwinfwd_data *) malloc (sizeof (*x));
    memset (x, '\0', sizeof (*x));
    for (i = 10; i >= 0; i--) {
        x->listen_sock = remotefs_listen_socket ("127.0.0.1", 6000 + i);
        if (x->listen_sock != INVALID_SOCKET) {
            x->listen_port = 6000 + i;
            return x;
        }
    }
    free (x);
    return NULL;
}

int xwinfwd_new_client (struct xwinfwd_data *x)
{E_
    SOCKET sock;
    struct xwinfwd_data_item *n;
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

    n = (struct xwinfwd_data_item *) malloc (sizeof (*n));
    memset (n, '\0', sizeof (*n));
    n->sock = sock;
    n->xwin_client_id = xwin_client_id++;
    n->rd.buf = (unsigned char *) malloc (TERMINAL_TCP_BUF_SIZE);
    n->rd.alloced = TERMINAL_TCP_BUF_SIZE;
    n->wr.buf = (unsigned char *) malloc (TERMINAL_TCP_BUF_SIZE);
    n->wr.alloced = TERMINAL_TCP_BUF_SIZE;
    n->didread = 1; /* startup has not set lastwrite */

    n->next = x->xwinfwd_list;
    x->xwinfwd_list = n;

    return 0;
}

void xwinfwd_prep_sockets (struct xwinfwd_data *x, fd_set * rd, fd_set * wr, int *n)
{E_
    struct xwinfwd_data_item *i;
    for (i = x->xwinfwd_list; i; i = i->next) {
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

void xwinfwd_write (struct xwinfwd_data *x, unsigned long xwin_client_id, const char *buf, int buflen)
{E_
    struct xwinfwd_data_item *i;
    for (i = x->xwinfwd_list; i; i = i->next) {
        if (i->xwin_client_id == xwin_client_id) {
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

static void xwinfwd_free (struct xwinfwd_data_item *i)
{E_
    shutdown (i->sock, 2);
    close (i->sock);
    free (i->rd.buf);
    free (i->wr.buf);
    free (i);
}

int xwinfwd_process_sockets (struct sock_data *sock_data, struct xwinfwd_data *x, fd_set * rd, fd_set * wr)
{E_
    struct xwinfwd_data_item **p;
    for (p = &x->xwinfwd_list; *p;) {
        struct xwinfwd_data_item *i;
        int c;
        i = *p;
        if ((*p)->kill) {
            *p = (*p)->next;
            xwinfwd_free (i);
            continue;
        } else {
            p = &(*p)->next;
        }
        if (i->sock != INVALID_SOCKET && FD_ISSET (i->sock, rd)) {
            c = recv (i->sock, i->rd.buf + i->rd.avail, i->rd.alloced - i->rd.avail, 0);
            if (c < 0 && (ERROR_EAGAIN () || ERROR_EINTR ())) {
                /* ok */
            } else if (c <= 0) {
                if (send_blind_message (sock_data, REMOTEFS_ACTION_SHELLREAD, i->xwin_client_id, XFWDSTATUS_SHUTDOWN, "", 0, NULL, 0))
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
                if (send_blind_message (sock_data, REMOTEFS_ACTION_SHELLREAD, i->xwin_client_id, XFWDSTATUS_SHUTDOWN, "", 0, NULL, 0))
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
            if (send_blind_message (sock_data, REMOTEFS_ACTION_SHELLREAD, i->xwin_client_id, XFWDSTATUS_DATA, (char *) (i->rd.buf + i->rd.written), i->rd.avail - i->rd.written, NULL, 0))
                return -1;
            i->rd.written += l;
            if (i->rd.written == i->rd.avail)
                i->rd.written = i->rd.avail = 0;
        }
    }
    return 0;
}

void xwinfwd_kill (struct xwinfwd_data *x, unsigned long xwin_client_id)
{E_
    struct xwinfwd_data_item *i;
    for (i = x->xwinfwd_list; i; i = i->next) {
        if (i->xwin_client_id == xwin_client_id) {
            i->kill = 1;
        }
    }
}

void xwinfwd_freeall (struct xwinfwd_data *x)
{E_
    struct xwinfwd_data_item *i, *next;
    for (i = x->xwinfwd_list; i; i = next) {
        next = i->next;
        xwinfwd_free (i);
    }
    free (x);
}


#endif


