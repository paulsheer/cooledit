/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* complete.c - brings up a possible word list
   Copyright (C) 1996-2022 Paul Sheer
 */


#define COMPLETION_FILE "/cooledit.completion"

/* email address with spaces */
#define ALLOW_CHARS_TYPE_A "_#.@-\\/! <>()"
/* email address without spaces */
#define ALLOW_CHARS_TYPE_B "_#.@-\\/!"
/* latex functions */
#define ALLOW_CHARS_TYPE_C "\\"
/* C functions, and #'s */
#define ALLOW_CHARS_TYPE_D "_#"

#include "inspect.h"
#include "coolwidget.h"

extern Window main_window;
char *loadfile (const char *filename, long *filelen);

static char *word_list = 0;
static char **words;
static long num_completion_words = 0;

static int compare_completion (const char **a, const char **b)
{E_
    return strcmp (*a, *b);
}

static void load_competion_file (void)
{E_
    char *f, *n;
    long l, i;
    f = loadfile (n = catstrs (local_home_dir, EDIT_DIR COMPLETION_FILE, 0), &l);
    if (!f) {
	CErrorDialog (main_window, 20, 20, _(" Complete Word "),
		_(" You have not yet created a `completion' word list \n" \
		      " See the section COMPLETION in the man page for \n creating a personalised word list. \n" \
	  " The completion word list should go into the file \n %s "), n);
	return;
    }
    word_list = f;
    i = 0;
    for (; *f; f++)
	if (*f == '\n')
	    i++;
    while (*f == '\n' || !*f)	/* remove trailing blank lines */
	*(f--) = '\0';
    words = CMalloc ((i + 4) * sizeof (char *));
    i = 1;
    f = word_list;
    while (*f == '\n')		/* remove leading blank lines */
	f++;
    words[0] = f;
    for (;; f++) {
	if (*f == '\n') {
	    *f = '\0';
	    words[i++] = f + 1;
	} else if (*f == '\0') {
	    words[i] = 0;
	    break;
	}
    }
    num_completion_words = i;
    qsort (words, num_completion_words, sizeof (char *),
	    (int (*)(const void *, const void *)) compare_completion);
}

/* w = 0 causes return of the last word. Result must not be free'd */
static char *get_current_word (CWidget * w, char *allow_chars)
{E_
    static char t[1024];
    int i;
    static char *p;
    if (!w)
	return p;
    p = t + 1023;
    *p = '\0';
    for (i = 1; i < 1021; i++) {
	int c;
	c = edit_get_byte (w->editor, w->editor->curs1 - i);
	if (strchr (allow_chars, c) || isalnum (c)) {
	    *(--p) = c;
	    continue;
	}
	break;
    }
    return p;
}

/* result must be free'd, returns 0 on not found */
static char **get_possible_words (CWidget * w, char *allow_chars)
{E_
    char *p, **r;
    int i, l, o;
    p = get_current_word (w, allow_chars);
    l = strlen (p);
    if (!l)
	return 0;
    o = i = num_completion_words / 2;
    for (;;) {			/* binary search */
	int comp;
	i = (i + 1) >> 1;
	if (o >= num_completion_words)	/* is this possible ? */
	    o = num_completion_words - 1;
	if (o < 0)		/* is this possible ? */
	    o = 0;
	comp = strncmp (words[o], p, l);
	if (!comp)
	    break;
	if (comp < 0)
	    o += i;
	else
	    o -= i;
	if (i == 1) {
	    if (o >= num_completion_words)
		o = num_completion_words - 1;
	    if (o < 0)
		o = 0;
	    if (strncmp (words[o], p, l))
		return 0;
	}
    }
    while (o) {			/* find the first word that matches */
	if (strncmp (words[o - 1], p, l))
	    break;
	o--;
    }
    i = o;
    while (words[i]) {		/* find the last word that matches */
	if (strncmp (words[i], p, l))
	    break;
	i++;
    }
    i -= o;
    r = CMalloc ((i + 1) * sizeof (char *));
    memcpy (r, words + o, i * sizeof (char *));
    r[i] = 0;
    return r;
}

static char *complete_selection_get_line (void *data, int line)
{E_
    char **s;
    s = (char **) data;
    if (s[line])
	return s[line];
    return "";
}

static int get_selection_complete (char **s)
{E_
    int i, c = 0;
    for (i = 0; s[i]; i++) {
	int l;
	l = strlen (s[i]);
	if (c < l)
	    c = l;
    }
    return CListboxDialog (main_window, 20, 20, c, min (i, 10), 0, 0, 0, i,
			   complete_selection_get_line, (void *) s);
}

static void complete_with_word (CWidget * w, char *s)
{E_
    s += strlen (get_current_word (0, 0));
    while (*s) {
	edit_insert (w->editor, *s++);
	w->editor->force |= REDRAW_COMPLETELY;
    }
}

void complete_command (CWidget * edit)
{E_
    char **s;
    if (!edit) {
	if (word_list) {
	    free (word_list);
	    word_list = 0;
	}
	if (words) {
	    free (words);
	    words = 0;
	}
	return;
    } 
    if (!word_list)
	load_competion_file ();
    if (!word_list)
	return;
    s = get_possible_words (edit, ALLOW_CHARS_TYPE_A);
    if (!s)
	s = get_possible_words (edit, ALLOW_CHARS_TYPE_B);
    if (!s)
	s = get_possible_words (edit, ALLOW_CHARS_TYPE_C);
    if (!s)
	s = get_possible_words (edit, ALLOW_CHARS_TYPE_D);
    if (!s)
	return;
    if (!s[1]) {
	complete_with_word (edit, s[0]);
    } else {
	int i;
	i = get_selection_complete (s);
	if (i >= 0)
	    complete_with_word (edit, s[i]);
    }
    free (s);
}


