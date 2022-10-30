/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#include "inspect.h"
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <my_string.h>
#include <sys/types.h>

#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <signal.h>
#include <sys/types.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif



#define CHILD_EXITED_MAX	256

static struct {
    pid_t pid;
    int status;
} children_exitted[CHILD_EXITED_MAX];

static unsigned char children_exitted_leader = 0;
static unsigned char children_exitted_trailer = 0;

#if (RETSIGTYPE==void)
#define handler_return return
#else
#define handler_return return 0
#endif

static RETSIGTYPE childhandler (int x)
{E_
    int save_errno = errno;
    pid_t pid;
    pid = waitpid (-1, &children_exitted[children_exitted_leader].status, WNOHANG);
    if (pid > 0) {
	if ((unsigned char) (children_exitted_leader - children_exitted_trailer) <
	    (unsigned char) ((int) CHILD_EXITED_MAX - 2)) {
	    children_exitted[children_exitted_leader].pid = pid;
	    children_exitted_leader++;
	}
    }
    errno = save_errno;
    signal (SIGCHLD, childhandler);
    handler_return;
}

struct child_exitted_item {
    struct child_exitted_item *next;
    pid_t pid;
    int status;
};

struct child_exitted_list {
    struct child_exitted_item *next;
};

static struct child_exitted_list child_list = {NULL};
void childhandler_ (void)
{E_
    while (children_exitted_trailer != children_exitted_leader) {
        struct child_exitted_item *c;
        c = malloc (sizeof (struct child_exitted_item));
        memset (c, '\0', sizeof (*c));
        c->pid = children_exitted[children_exitted_trailer].pid;
        c->status = children_exitted[children_exitted_trailer].status;
        c->next = child_list.next;
        child_list.next = c;
        children_exitted_trailer++;
    }
}

/* returns non-zero on child exit */
int CChildExitted (pid_t p, int *status)
{E_
    struct child_exitted_item *c;
    if (!p)
        return 0;
    for (c = (struct child_exitted_item *) &child_list; c->next;) {
        if (c->next->pid == p) {
            struct child_exitted_item *t;
            t = c->next;
            c->next = c->next->next;
	    if (status)
        	*status = t->status;
            free (t);
            return 1;
        } else {
            c = c->next;
        }
    }
    return 0;
}

void CChildWait (pid_t p)
{E_
    while (!CChildExitted (p, NULL)) {
        struct timeval tv;
        childhandler_ ();
        tv.tv_sec = 0;
        tv.tv_usec = 50000;
        select (0, NULL, NULL, NULL, &tv);
    }
}

void set_child_handler (void)
{E_
    memset (children_exitted, 0, sizeof (children_exitted));
    signal (SIGCHLD, childhandler);
}

