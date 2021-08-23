/* pathdup.c - reproduces a path stripping /../ /./ and resolving symlinks
   Copyright (C) 1996-2018 Paul Sheer
 */

#include "global.h"
#ifdef MSWIN
#include <config-mswin.h>
#else
#include <config.h>
#endif
#include "pipe-headers.h"
#include <my_string.h>

#include "remotefs.h"


char *get_current_wd (char *buffer, int size);


struct comp {
    struct comp *prev;
    struct comp *next;
    char name[2];
};

static struct comp *comp_last (struct comp *p)
{
    while (p->next)
	p = p->next;
    return p;
}

static struct comp *comp_first (struct comp *p)
{
    while (p->prev)
	p = p->prev;
    return p;
}

static inline struct comp *comp_cat (struct comp *s, struct comp *t)
{
    s = comp_last (s);
    t = comp_first (t);
    s->next = t;
    t->prev = s;
    return comp_first (s);
}

static inline struct comp *comp_insert (struct comp *p, struct comp *s)
{
    struct comp *t;
    t = comp_last (s);
    s = comp_first (s);
    if (p->prev)
	p->prev->next = s;
    if (p->next)
	p->next->prev = t;
    t->next = p->next;
    s->prev = p->prev;
    memset (p, 0, sizeof (*p));
    free (p);
    return t;
}

static inline struct comp *comp_replace (struct comp *p, struct comp *s)
{
    struct comp *t, *prev, *r;
    t = comp_last (s);
    if (p->next)
	p->next->prev = t;
    t->next = p->next;
    for (r = p; r; r = prev) {
	prev = r->prev;
	memset (r, 0, sizeof (*r));
	free (r);
    }
    return t;
}

static inline void comp_free (struct comp *p)
{
    struct comp *next;
    p = comp_first (p);
    for (; p; p = next) {
	next = p->next;
	memset (p, 0, sizeof (*p));
	free (p);
    }
}

#define COMP_DUMP(p)					\
	    if (u == p)					\
		u = p->next;				\
	    if (p->next)				\
		p->next->prev = p->prev;		\
	    if (p->prev)				\
		p->prev->next = p->next;		\
	    memset (p, 0, sizeof (*p));			\
	    free (p);


/* dump  ..  .  and nothings, but remember the place in the list of p */
static struct comp *comp_strip (struct comp *p)
{
    struct comp *u, *next;
    u = comp_first (p);
    for (p = u; p; p = next) {
	next = p->next;
	if (!*p->name || !strcmp (p->name, ".")) {
	    COMP_DUMP (p);
	} else if (!strcmp (p->name, "..")) {
	    struct comp *t;
	    if ((t = p->prev)) {
		COMP_DUMP (t);
	    }
	    COMP_DUMP (p);
	}
    }
    if (!u) {
/* mustn't strip everything */
	u = malloc (sizeof (struct comp));
	memset (u, 0, sizeof (struct comp));
    }
    return u;
}

/* split into a list along / */
static char *comp_combine (struct comp *s)
{
    int n;
    struct comp *t, *f;
    char *p, *r;
    f = comp_first (s);
    for (n = 0, t = f; t != s->next; t = t->next)
	n += strlen (t->name) + 1;
    r = malloc (n + 2);
    for (p = r, t = f; t != s->next; t = t->next) {
	*p++ = '/';
	strcpy (p, t->name);
	p += strlen (p);
    }
    return r;
}

/* split into a list along / */
static struct comp *comp_tize (const char *s)
{
    struct comp *u, *p = 0;
    const char *t;
    int done = 0;
    while (!done) {
	int l;
	t = (char *) strchr (s, '/');
	if (!t) {
	    t = s + strlen (s);
	    done = 1;
	}
	l = (t - s);
	u = malloc (sizeof (struct comp) + l);
	u->prev = p;
	u->next = 0;
	if (p)
	    p->next = u;
	p = u;
	memcpy (u->name, s, l);
	u->name[l] = '\0';
	s = t + 1;
    }
    return p;
}

static inline char *comp_readlink (struct comp *p)
{
#ifdef MSWIN
    return "";
#else
    char *s;
    int r;
    static char buf[2048];
    s = comp_combine (p);
    r = readlink (s, buf, 2047);
    if (r == -1 && errno == EINVAL) {
	free (s);
	return "";
    }
    if (r == -1) {
	free (s);
	return 0;
    }
    buf[r] = '\0';
    free (s);
    return buf;
#endif
}

/* if there is an error, this just returns as far as it got */
static inline struct comp *resolve_symlink (struct comp *path)
{
    int i;
    struct comp *t;
    path = comp_strip (comp_first (path));
    path = comp_last (path);
    for (i = 0;; i++) {
	char *l;
	if (i >= 1000)
	    break;
	l = comp_readlink (path);
	if (!l)
	    break;
	if (l[0] == '/') {
/* absolute symlink */
	    t = comp_tize (l);
	    path = comp_replace (path, t);
	    path = comp_strip (path);
	    path = comp_last (path);
	    continue;
	} else if (*l) {
/* relative symlink */
	    t = comp_tize (l);
	    path = comp_insert (path, t);
	    path = comp_strip (path);
	    path = comp_last (path);
	    continue;
	} else if (path->prev) {
/* not a symlink */
	    path = path->prev;
	    continue;
	}
	break;
    }
    return path;
}

static char *strdupextra (const char *s)
{
    int l;
    char *p;
    l = strlen (s);
    p = (char *) malloc (l + MAX_PATH_LEN + 2);
    memcpy (p, s, l + 1);
    return p;
}

char *pathdup_debug (const char *cfile, int cline, const char *host, const char *p)
{
    static char out[MAX_PATH_LEN] = "";
    static char in[MAX_PATH_LEN] = "";
    static char host_[MAX_PATH_LEN] = "";
    char errmsg[REMOTEFS_ERR_MSG_LEN];
    struct remotefs *u;

    if (!host || !*host)
        host = REMOTEFS_LOCAL;

    if ((!strcmp (in, p) || !strcmp (out, p)) && !strcmp (host_, host))
        return strdupextra (out);

    strncpy (in, p, MAX_PATH_LEN);
    in[MAX_PATH_LEN - 1] = '\0';

    strcpy (host_, host);

    u = remotefs_lookup (host);

    if ((*u->remotefs_realpathize) (u, p, remotefs_home_dir (u), out, MAX_PATH_LEN, errmsg)) {
        strncpy (out, p, MAX_PATH_LEN);
        out[MAX_PATH_LEN - 1] = '\0';
        return strdupextra (p);
    }
    return strdupextra (out);
}

char *pathdup_ (const char *p, const char *home_dir)
{
    char *r;
    struct comp *s;
    s = comp_tize (p);

    if (!strcmp (comp_first (s)->name, "~")) {
	s = comp_replace (comp_first (s), comp_tize (home_dir));
    } else if (*p != '/') {
	char cwd[MAX_PATH_LEN];
        get_current_wd (cwd, MAX_PATH_LEN);
	s = comp_cat (comp_tize (cwd), comp_tize (p));
    }
    s = resolve_symlink (s);
    r = comp_combine (comp_last (s));
    comp_free (s);
    return r;
}

#if 0
char *home_dir = "/root";

int main (int argc, char **argv)
{
    printf ("%s\n", pathdup (argv[1], home_dir));
    return 0;
}
#endif

