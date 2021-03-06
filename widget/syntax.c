/* syntax.c
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <config.h>
#include <stdio.h>
#if defined(MIDNIGHT) || defined(GTK)
#include "edit.h"
#else
#include "coolwidget.h"
#include "stringtools.h"
#endif

/* bytes */
#define SYNTAX_MARKER_DENSITY 512

#define COUNT_BRACE(c, d) \
    do { \
        if ((c) == '(' || (c) == '[' || (c) == '{') \
            (d)++; \
        if ((c) == ')' || (c) == ']' || (c) == '}') \
            (d)--; \
    } while (0)


/*
   Mispelled words are flushed from the syntax highlighting rules
   when they have been around longer than
   TRANSIENT_WORD_TIME_OUT seconds. At a cursor rate of 30
   chars per second and say 3 chars + a space per word, we can
   accumulate 450 words absolute max with a value of 60. This is
   below this limit of 1024 words in a context.
 */
#define TRANSIENT_WORD_TIME_OUT 60

#define UNKNOWN_FORMAT "unknown"

#if !defined(MIDNIGHT) || defined(HAVE_SYNTAXH)

int option_syntax_highlighting = 1;
int option_auto_spellcheck = 1;

/* these three functions are called from the outside */
void edit_load_syntax (WEdit * edit, char **names, char *type);
void edit_free_syntax_rules (WEdit * edit);
void edit_get_syntax_color (WEdit * edit, long byte_index, int *fg, int *bg);

static void *syntax_malloc (size_t x)
{
    void *p;
    p = malloc (x);
    memset (p, 0, x);
    return p;
}

#define syntax_free(x) {if(x){free(x);(x)=0;}}

static long compare_word_to_right (WEdit * edit, long i, char *text, char *whole_left, char *whole_right, int line_start, int brace_match)
{
    int depth = 0;
    unsigned char *p, *q;
    int c, d, j;
    if (!*text)
	return -1;
    c = edit_get_byte (edit, i - 1);
    if (line_start)
	if (c != '\n')
	    return -1;
    if (whole_left)
	if (strchr (whole_left, c))
	    return -1;
    for (p = (unsigned char *) text, q = p + strlen ((char *) p); (unsigned long) p < (unsigned long) q; p++, i++) {
	switch (*p) {
	case '\001':    /* '*'  elastic wildcard */
	    p++;
	    for (;;) {
		c = edit_get_byte (edit, i);
                COUNT_BRACE(c, depth);
		if (!*p)
		    if (whole_right)
			if (!strchr (whole_right, c))
			    break;
                if (brace_match && depth > 0) {
                    /* refuse to check for end of elastic wildcard if brace is not closed, such as  'keyword ${*} brightgreen/16'  */
                } else {
		    if (c == *p)
		        break;
                }
		if (c == '\n')
		    return -1;
		i++;
	    }
	    break;
	case '\002':    /* '+' */
	    p++;
	    j = 0;
	    for (;;) {
		c = edit_get_byte (edit, i);
		if (c == *p) {
		    j = i;
		    if (*p == *text && !p[1])	/* handle eg '+' and @+@ keywords properly */
			break;
		}
		if (j && strchr ((char *) p + 1, c))	/* c exists further down, so it will get matched later */
		    break;
		if (c == '\n' || c == '\t' || c == ' ') {
		    if (!*p) {
			i--;
			break;
		    }
		    if (!j)
			return -1;
		    i = j;
		    break;
		}
		if (whole_right)
		    if (!strchr (whole_right, c)) {
			if (!*p) {
			    i--;
			    break;
			}
			if (!j)
			    return -1;
			i = j;
			break;
		    }
		i++;
	    }
	    break;
	case '\003':    /* '['  ']' */
	    p++;
	    c = -1;
	    for (;; i++) {
		d = c;
		c = edit_get_byte (edit, i);
		for (j = 0; p[j] != '\003'; j++)
		    if (c == p[j])
			goto found_char2;
		break;
	      found_char2:
		j = c;		/* dummy command */
                (void) j;
	    }
	    i--;
	    while (*p != '\003')
		p++;
	    if (p[1] == d)
		i--;
	    break;
	case '\004':    /* '{' '}' */
	    p++;
	    c = edit_get_byte (edit, i);
            COUNT_BRACE(c, depth);
	    for (; *p != '\004'; p++)
		if (c == *p)
		    goto found_char3;
	    return -1;
	  found_char3:
	    for (; *p != '\004'; p++);
	    break;
#if 0
/* especially for LaTeX */
	case '\005': {
                int b = 0;
	        p++;
	        for (;;) {
		    c = edit_get_byte (edit, i);
                    if (c == '\\') {
                        i += 2;
                        continue;
                    }
                    if (c == '{') {
                    }
		    if (c == *p)
		        break;
		    if (c == '\n')
		        return -1;
		    i++;
	        }
	        break;
            }
#endif
	default:
            c = edit_get_byte (edit, i);
            COUNT_BRACE(c, depth);
	    if (*p != (unsigned char) c)
		return -1;
	}
    }
    if (whole_right)
	if (strchr (whole_right, edit_get_byte (edit, i)))
	    return -1;
    return i;
}

#if 1
#define XXX							\
	    if (*s < '\005' || *s == (unsigned char) c)		\
		goto done;					\
	    s++;
static inline char *xx_strchr (const unsigned char *s, int c)
{
  repeat:
    XXX XXX XXX XXX XXX XXX XXX XXX;
    XXX XXX XXX XXX XXX XXX XXX XXX;
    goto repeat;
  done:
    return (char *) s;
}
#endif

#if 0
/* this optimized function gives no better overall perfomance */
static inline char *xx_strchr (const unsigned char *s, int c)
{
    int i;
    unsigned long *p, cl, one = 1UL, cx = 0xF8;
    p = (unsigned long *) (void *) s;

    cl = c;

    cl |= cl << 8;
    cl |= cl << 16;
    cl |= cl << 16;
    cl |= cl << 16;

    cx |= cx << 8;
    cx |= cx << 16;
    cx |= cx << 16;
    cx |= cx << 16;

    one |= one << 8;
    one |= one << 16;
    one |= one << 16;
    one |= one << 16;

    if (endian_little) {
	for (;;) {
	    unsigned long v, w1, w2;
	    v = *p;
	    w1 = v ^ cl;
	    w2 = v & cx;
	    if (((((w1 - one) & ~w1) & (one << 7)) | (((w2 - one) & ~w2) & (one << 7)))) {
		for (i = 0; i < sizeof (long); i++) {
		    unsigned int ss;
		    ss = (v >> (8 * i)) & 0xFF;
		    if (ss < 8 || ss == c)
			return ((char *) (void *) p) + i;
		}
	    }
	    p++;
	}
    } else {
	for (;;) {
	    unsigned long v, w1, w2;
	    v = *p;
	    w1 = v ^ cl;
	    w2 = v & cx;
	    if (((((w1 - one) & ~w1) & (one << 7)) | (((w2 - one) & ~w2) & (one << 7)))) {
		for (i = sizeof (long) - 1; i >= 0; i--) {
		    unsigned int ss;
		    ss = (v >> (8 * i)) & 0xFF;
		    if (ss < 8 || ss == c)
			return ((char *) (void *) p) + i;
		}
	    }
	    p++;
	}
    }
}
#endif



#if 0
void test_xx_strchr ()
{
    char *s = "0123456789abcdefghijklmnopq";
    int i;

    for (i = 0; i < 28; i++) {
        printf("\n");
	printf ("%c [%s]\n", (int) s[i], xx_strchr ((unsigned char *) s, s[i]));
    }
    exit(0);
}
#endif


static inline struct syntax_rule apply_rules_going_right (WEdit * edit, long i, struct syntax_rule rule)
{
    struct context_rule *r;
    int contextchanged = 0, c;
    int found_right = 0, found_left = 0, keyword_foundleft = 0, keyword_foundright = 0;
    int is_end;
    long end = 0;
    struct syntax_rule _rule = rule;
    if (!(c = edit_get_byte (edit, i)))
	return rule;
    COUNT_BRACE (c, rule.brace_depth);
    is_end = (rule.end == (unsigned char) i);
/* check to turn off a keyword */
    if (_rule.keyword) {
	if (edit_get_byte (edit, i - 1) == '\n')
	    _rule.keyword = 0;
	if (is_end) {
	    _rule.keyword = 0;
	    keyword_foundleft = 1;
	}
    }
/* check to turn off a context */
    if (_rule.context && !_rule.keyword) {
	long e;
	r = edit->rules[_rule.context];
	if (r->first_right == c && !(rule.border & RULE_ON_RIGHT_BORDER) && (e = compare_word_to_right (edit, i, r->right, r->whole_word_chars_left, r->whole_word_chars_right, r->line_start_right, 0)) > 0) {
	    _rule.end = e;
	    found_right = 1;
	    _rule.border = RULE_ON_RIGHT_BORDER;
	    if (r->between_delimiters)
		_rule.context = 0;
	} else if (is_end && rule.border & RULE_ON_RIGHT_BORDER) {
/* always turn off a context at 4 */
	    found_left = 1;
	    _rule.border = 0;
	    if (!keyword_foundleft)
		_rule.context = 0;
	} else if (is_end && rule.border & RULE_ON_LEFT_BORDER) {
/* never turn off a context at 2 */
	    found_left = 1;
	    _rule.border = 0;
	}
    }
/* check to turn on a keyword */
    if (!_rule.keyword) {
	char *p;
	p = (r = edit->rules[_rule.context])->keyword_first_chars;
	while (*(p = xx_strchr ((unsigned char *) p + 1, c))) {
	    struct key_word *k;
	    int count;
	    long e;
	    count = (unsigned long) p - (unsigned long) r->keyword_first_chars;
	    k = r->keyword[count];
	    e = compare_word_to_right (edit, i, k->keyword, k->whole_word_chars_left, k->whole_word_chars_right, k->line_start, k->brace_match);
	    if (e > 0) {
		end = e;
		_rule.end = e;
		_rule.keyword = count;
		keyword_foundright = 1;
		break;
	    }
	}
    }
/* check to turn on a context */
    if (!_rule.context) {
	if (!found_left && is_end) {
	    if (rule.border & RULE_ON_RIGHT_BORDER) {
		_rule.border = 0;
		_rule.context = 0;
		contextchanged = 1;
		_rule.keyword = 0;
	    } else if (rule.border & RULE_ON_LEFT_BORDER) {
		r = edit->rules[_rule._context];
		_rule.border = 0;
		if (r->between_delimiters) {
		    long e;
		    _rule.context = _rule._context;
		    contextchanged = 1;
		    _rule.keyword = 0;
		    if (r->first_right == c && (e = compare_word_to_right (edit, i, r->right, r->whole_word_chars_left, r->whole_word_chars_right, r->line_start_right, 0)) >= end) {
			_rule.end = e;
			found_right = 1;
			_rule.border = RULE_ON_RIGHT_BORDER;
			_rule.context = 0;
		    }
		}
	    }
	}
	if (!found_right) {
	    int count;
	    struct context_rule **rules = edit->rules;
	    for (count = 1; rules[count]; count++) {
		r = rules[count];
		if (r->first_left == c) {
		    long e;
		    e = compare_word_to_right (edit, i, r->left, r->whole_word_chars_left, r->whole_word_chars_right, r->line_start_left, 0);
		    if (e >= end && (!_rule.keyword || keyword_foundright)) {
			_rule.end = e;
			found_right = 1;
                        (void) found_right;
			_rule.border = RULE_ON_LEFT_BORDER;
			_rule._context = count;
			if (!r->between_delimiters)
			    if (!_rule.keyword) {
				_rule.context = count;
				contextchanged = 1;
			    }
			break;
		    }
		}
	    }
	}
    }
/* check again to turn on a keyword if the context switched */
    if (contextchanged && !_rule.keyword) {
	char *p;
	p = (r = edit->rules[_rule.context])->keyword_first_chars;
	while (*(p = xx_strchr ((unsigned char *) p + 1, c))) {
	    struct key_word *k;
	    int count;
	    long e;
	    count = (unsigned long) p - (unsigned long) r->keyword_first_chars;
	    k = r->keyword[count];
	    e = compare_word_to_right (edit, i, k->keyword, k->whole_word_chars_left, k->whole_word_chars_right, k->line_start, k->brace_match);
	    if (e > 0) {
		_rule.end = e;
		_rule.keyword = count;
		break;
	    }
	}
    }
    return _rule;
}

static struct syntax_rule edit_get_rule (WEdit * edit, long byte_index)
{
    long i;
    if (edit->syntax_invalidate) {
	struct _syntax_marker *s;
	while (edit->syntax_marker && edit->syntax_marker->offset >= edit->last_get_rule) {
	    s = edit->syntax_marker->next;
	    syntax_free (edit->syntax_marker);
	    edit->syntax_marker = s;
	}
	if (edit->syntax_marker) {
	    edit->last_get_rule = edit->syntax_marker->offset;
	    edit->rule = edit->syntax_marker->rule;
	} else {
	    edit->last_get_rule = -1;
	    memset (&edit->rule, 0, sizeof (edit->rule));
	}
	edit->syntax_invalidate = 0;
    }
    if (byte_index > edit->last_get_rule) {
	for (i = edit->last_get_rule + 1; i <= byte_index; i++) {
	    edit->rule = apply_rules_going_right (edit, i, edit->rule);
	    if (i > (edit->syntax_marker ? edit->syntax_marker->offset + SYNTAX_MARKER_DENSITY : SYNTAX_MARKER_DENSITY)) {
		struct _syntax_marker *s;
		s = edit->syntax_marker;
		edit->syntax_marker = syntax_malloc (sizeof (struct _syntax_marker));
		edit->syntax_marker->next = s;
		edit->syntax_marker->offset = i;
		edit->syntax_marker->rule = edit->rule;
	    }
	}
    } else if (byte_index < edit->last_get_rule) {
	struct _syntax_marker *s;
	for (;;) {
	    if (!edit->syntax_marker) {
		memset (&edit->rule, 0, sizeof (edit->rule));
		for (i = -1; i <= byte_index; i++)
		    edit->rule = apply_rules_going_right (edit, i, edit->rule);
		break;
	    }
	    if (byte_index >= edit->syntax_marker->offset) {
		edit->rule = edit->syntax_marker->rule;
		for (i = edit->syntax_marker->offset + 1; i <= byte_index; i++)
		    edit->rule = apply_rules_going_right (edit, i, edit->rule);
		break;
	    }
	    s = edit->syntax_marker->next;
	    syntax_free (edit->syntax_marker);
	    edit->syntax_marker = s;
	}
    }
    edit->last_get_rule = byte_index;
    return edit->rule;
}

static void translate_rule_to_color (WEdit * edit, struct syntax_rule rule, int *fg, int *bg)
{
    struct key_word *k;
    k = edit->rules[rule.context]->keyword[rule.keyword];
    *bg = k->bg;
    *fg = k->fg;
}

void edit_get_syntax_color (WEdit * edit, long byte_index, int *fg, int *bg)
{
    if (edit->rules && byte_index < edit->last_byte && option_syntax_highlighting) {
	translate_rule_to_color (edit, edit_get_rule (edit, byte_index), fg, bg);
    } else {
#ifdef MIDNIGHT
	*fg = EDITOR_NORMAL_COLOR;
#else
	*fg = NO_COLOR;
	*bg = NO_COLOR;
#endif
    }
}


/*
   Returns 0 on error/eof or a count of the number of bytes read
   including the newline. Result must be free'd.
 */
static int read_one_line (char **line, FILE * f)
{
    char *p;
    int len = 256, c, r = 0, i = 0;
    p = syntax_malloc (len);
    for (;;) {
	c = fgetc (f);
	if (c <= 0) {
	    if (!feof(f) && (c == -1 && errno == EINTR))
		continue;
	    r = 0;
	    break;
	} else if (c == '\n') {
	    r = i + 1;		/* extra 1 for the newline just read */
	    break;
	} else {
	    if (i >= len - 1) {
		char *q;
		q = syntax_malloc (len * 2);
		memcpy (q, p, len);
		syntax_free (p);
		p = q;
		len *= 2;
	    }
	    p[i++] = c;
	}
    }
    p[i] = 0;
    *line = p;
    return r;
}

static char *strdup_convert (char *s)
{
    char *r, *p;
    p = r = (char *) strdup (s);
    while (*s) {
	switch (*s) {
	case '\\':
	    s++;
	    switch (*s) {
	    case ' ':
		*p = ' ';
		s--;
		break;
	    case 'n':
		*p = '\n';
		break;
	    case 'r':
		*p = '\r';
		break;
	    case 't':
		*p = '\t';
		break;
	    case 's':
		*p = ' ';
		break;
	    case '*':
		*p = '*';
		break;
	    case '\\':
		*p = '\\';
		break;
	    case '[':
	    case ']':
		*p = '\003';
		break;
	    case '{':
	    case '}':
		*p = '\004';
		break;
	    default:
		*p = *s;
		break;
	    }
	    break;
	case '*':
	    *p = '\001';
	    break;
	case '+':
	    *p = '\002';
	    break;
	default:
	    *p = *s;
	    break;
	}
	s++;
	p++;
    }
    *p = '\0';
    return r;
}

#define whiteness(x) ((x) == '\t' || (x) == '\n' || (x) == ' ')

static void get_args (char *l, char **args, int *argc)
{
    *argc = 0;
    l--;
    for (;;) {
	char *p;
	for (p = l + 1; *p && whiteness (*p); p++);
	if (!*p)
	    break;
	for (l = p + 1; *l && !whiteness (*l); l++);
	*l = '\0';
	*args = strdup_convert (p);
	(*argc)++;
	args++;
    }
    *args = 0;
}

static void free_args (char **args)
{
    while (*args) {
	syntax_free (*args);
	*args = 0;
	args++;
    }
}

#define check_a {if(!*a){result=line;break;}}
#define check_not_a {if(*a){result=line;break;}}

#ifdef MIDNIGHT

int try_alloc_color_pair (char *fg, char *bg);

int this_try_alloc_color_pair (char *fg, char *bg)
{
    char f[80], b[80], *p;
    if (bg)
	if (!*bg)
	    bg = 0;
    if (fg)
	if (!*fg)
	    fg = 0;
    if (fg) {
	strcpy (f, fg);
	p = strchr (f, '/');
	if (p)
	    *p = '\0';
	fg = f;
    }
    if (bg) {
	strcpy (b, bg);
	p = strchr (b, '/');
	if (p)
	    *p = '\0';
	bg = b;
    }
    return try_alloc_color_pair (fg, bg);
}
#else
#ifdef GTK
int allocate_color (WEdit *edit, gchar *color);

int this_allocate_color (WEdit *edit, char *fg)
{
    char *p;
    if (fg)
	if (!*fg)
	    fg = 0;
    if (!fg)
	return allocate_color (edit, 0);
    p = strchr (fg, '/');
    if (!p)
	return allocate_color (edit, fg);
    return allocate_color (edit, p + 1);
}
#else
int this_allocate_color (WEdit *edit, char *fg)
{
    char *p;
    if (fg)
	if (!*fg)
	    fg = 0;
    if (!fg)
	return allocate_color (0);
    p = strchr (fg, '/');
    if (!p)
	return allocate_color (fg);
    return allocate_color (p + 1);
}
#endif
#endif

static char *error_file_name = 0;

static FILE *open_include_file (char *filename)
{
    FILE *f;
    char p[MAX_PATH_LEN];
    syntax_free (error_file_name);
    error_file_name = (char *) strdup (filename);
    if (*filename == '/')
	return fopen (filename, "r");
    strcpy (p, local_home_dir);
    strcat (p, EDIT_DIR "/");
    strcat (p, filename);
    syntax_free (error_file_name);
    error_file_name = (char *) strdup (p);
    f = fopen (p, "r");
    if (f)
	return f;
    strcpy (p, LIBDIR "/syntax/");
    strcat (p, filename);
    syntax_free (error_file_name);
    error_file_name = (char *) strdup (p);
    return fopen (p, "r");
}

/* returns line number on error */
static int edit_read_syntax_rules (WEdit * edit, FILE * f)
{
    FILE *g = 0;
    char *fg, *bg;
    char last_fg[32] = "", last_bg[32] = "";
    char whole_right[512];
    char whole_left[512];
    char *args[1024], *l = 0;
    int save_line = 0, line = 0;
    struct context_rule **r, *c = 0;
    int num_words = -1, num_contexts = -1;
    int argc, result = 0;
    int i, j;

    args[0] = 0;

    strcpy (whole_left, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_01234567890");
    strcpy (whole_right, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_01234567890");

    r = edit->rules = syntax_malloc (MAX_CONTEXTS * sizeof (struct context_rule *));

    for (;;) {
	char **a;
	line++;
	l = 0;
	if (!read_one_line (&l, f)) {
	    if (g) {
		fclose (f);
		f = g;
		g = 0;
		line = save_line + 1;
		syntax_free (error_file_name);
		if (l)
		    syntax_free (l);
		if (!read_one_line (&l, f))
		    break;
	    } else {
		break;
	    }
	}
	get_args (l, args, &argc);
	a = args + 1;
	if (!args[0]) {
	    /* do nothing */
	} else if (!strcmp (args[0], "include")) {
	    if (g || argc != 2) {
		result = line;
		break;
	    }
	    g = f;
	    f = open_include_file (args[1]);
	    if (!f) {
		syntax_free (error_file_name);
		result = line;
		break;
	    }
	    save_line = line;
	    line = 0;
	} else if (!strcmp (args[0], "wholechars")) {
	    check_a;
	    if (!strcmp (*a, "left")) {
		a++;
		strcpy (whole_left, *a);
	    } else if (!strcmp (*a, "right")) {
		a++;
		strcpy (whole_right, *a);
	    } else {
		strcpy (whole_left, *a);
		strcpy (whole_right, *a);
	    }
	    a++;
	    check_not_a;
	} else if (!strcmp (args[0], "context")) {
	    check_a;
	    if (num_contexts == -1) {
		if (strcmp (*a, "default")) {	/* first context is the default */
		    *a = 0;
		    check_a;
		}
		a++;
		c = r[0] = syntax_malloc (sizeof (struct context_rule));
		c->left = (char *) strdup (" ");
		c->right = (char *) strdup (" ");
		num_contexts = 0;
	    } else {
		c = r[num_contexts] = syntax_malloc (sizeof (struct context_rule));
		if (!strcmp (*a, "exclusive")) {
		    a++;
		    c->between_delimiters = 1;
		}
		check_a;
		if (!strcmp (*a, "whole")) {
		    a++;
		    c->whole_word_chars_left = (char *) strdup (whole_left);
		    c->whole_word_chars_right = (char *) strdup (whole_right);
		} else if (!strcmp (*a, "wholeleft")) {
		    a++;
		    c->whole_word_chars_left = (char *) strdup (whole_left);
		} else if (!strcmp (*a, "wholeright")) {
		    a++;
		    c->whole_word_chars_right = (char *) strdup (whole_right);
		}
		check_a;
		if (!strcmp (*a, "linestart")) {
		    a++;
		    c->line_start_left = 1;
		}
		check_a;
		c->left = (char *) strdup (*a++);
		check_a;
		if (!strcmp (*a, "linestart")) {
		    a++;
		    c->line_start_right = 1;
		}
		check_a;
		c->right = (char *) strdup (*a++);
		c->first_left = *c->left;
		c->first_right = *c->right;
		c->single_char = (strlen (c->right) == 1);
	    }
	    c->keyword = syntax_malloc (MAX_WORDS_PER_CONTEXT * sizeof (struct key_word *));
#if 0
	    c->max_words = MAX_WORDS_PER_CONTEXT;
#endif
	    num_words = 1;
	    c->keyword[0] = syntax_malloc (sizeof (struct key_word));
	    fg = *a;
	    if (*a)
		a++;
	    bg = *a;
	    if (*a)
		a++;
	    strcpy (last_fg, fg ? fg : "");
	    strcpy (last_bg, bg ? bg : "");
#ifdef MIDNIGHT
	    c->keyword[0]->fg = this_try_alloc_color_pair (fg, bg);
#else
	    c->keyword[0]->fg = this_allocate_color (edit, fg);
	    c->keyword[0]->bg = this_allocate_color (edit, bg);
#endif
	    c->keyword[0]->keyword = (char *) strdup (" ");
	    check_not_a;
	    num_contexts++;
	} else if (!strcmp (args[0], "spellcheck")) {
	    if (!c) {
		result = line;
		break;
	    }
	    c->spelling = 1;
	} else if (!strcmp (args[0], "keyword")) {
	    struct key_word *k;
	    if (num_words == -1)
		*a = 0;
	    check_a;
	    k = r[num_contexts - 1]->keyword[num_words] = syntax_malloc (sizeof (struct key_word));
	    if (!strcmp (*a, "whole")) {
		a++;
		k->whole_word_chars_left = (char *) strdup (whole_left);
		k->whole_word_chars_right = (char *) strdup (whole_right);
	    } else if (!strcmp (*a, "wholeleft")) {
		a++;
		k->whole_word_chars_left = (char *) strdup (whole_left);
	    } else if (!strcmp (*a, "wholeright")) {
		a++;
		k->whole_word_chars_right = (char *) strdup (whole_right);
	    }
	    check_a;
	    if (!strcmp (*a, "linestart")) {
		a++;
		k->line_start = 1;
	    }
	    check_a;
	    if (!strcmp (*a, "bracematch")) {
		a++;
		k->brace_match = 1;
	    }
	    check_a;
	    if (!strcmp (*a, "whole")) {
		*a = 0;
		check_a;
	    }
	    k->keyword = (char *) strdup (*a++);
	    k->first = *k->keyword;
	    fg = *a;
	    if (*a)
		a++;
	    bg = *a;
	    if (*a)
		a++;
	    if (!fg)
		fg = last_fg;
	    if (!bg)
		bg = last_bg;
#ifdef MIDNIGHT
	    k->fg = this_try_alloc_color_pair (fg, bg);
#else
	    k->fg = this_allocate_color (edit, fg);
	    k->bg = this_allocate_color (edit, bg);
#endif
	    check_not_a;
	    num_words++;
	} else if (!strncmp (args[0], "#", 1)) {
	    /* do nothing for comment */
	} else if (!strcmp (args[0], "file")) {
	    break;
	} else {		/* anything else is an error */
	    *a = 0;
	    check_a;
	}
	free_args (args);
	syntax_free (l);
    }
    free_args (args);
    syntax_free (l);

    if (!edit->rules[0])
	syntax_free (edit->rules);

    if (result)
	return result;

    if (num_contexts == -1) {
	result = line;
	return result;
    }

    {
	char first_chars[MAX_WORDS_PER_CONTEXT + 2], *p;
	for (i = 0; edit->rules[i]; i++) {
	    c = edit->rules[i];
	    p = first_chars;
	    *p++ = (char) 1;
	    for (j = 1; c->keyword[j]; j++)
		*p++ = c->keyword[j]->first;
	    *p = '\0';
	    c->keyword_first_chars = syntax_malloc (strlen (first_chars) + 7);
	    strcpy (c->keyword_first_chars, first_chars);
	}
    }

    return result;
}

#if !defined (GTK) && !defined (MIDNIGHT)

/* strdup and append c */
static char *strdupc (char *s, int c)
{
    char *t;
    int l;
    l = strlen (s);
    strcpy (t = syntax_malloc (l + 1 + 7), s);
    t[l] = c;
    t[l + 1] = '\0';
    return t;
}

static void edit_syntax_clear_keyword (WEdit * edit, int context, int j)
{
    int k;
    char *q;
    struct context_rule *c;
    struct _syntax_marker *s;
    c = edit->rules[context];
/* first we clear any instances of this keyword in our cache chain (we used to just clear the cache chain, but this slows things down) */
    for (s = edit->syntax_marker; s; s = s->next)
	if (s->rule.keyword == j)
	    s->rule.keyword = 0;
	else if (s->rule.keyword > j)
	    s->rule.keyword--;
    syntax_free (c->keyword[j]->keyword);
    syntax_free (c->keyword[j]->whole_word_chars_left);
    syntax_free (c->keyword[j]->whole_word_chars_right);
    syntax_free (c->keyword[j]);
    for (k = j; k < MAX_WORDS_PER_CONTEXT - 1; k++)
        c->keyword[k] = c->keyword[k + 1];
    for (q = &c->keyword_first_chars[j]; *q; q++)
        *q = *(q + 1);
}


FILE *spelling_pipe_in = 0;
FILE *spelling_pipe_out = 0;
pid_t ispell_pid = 0;


/* adds a keyword for underlining into the keyword list for this context, returns 1 if too many words */
static int edit_syntax_add_keyword (WEdit * edit, char *keyword, int context, time_t t)
{
    int j;
    char *s;
    struct context_rule *c;
    c = edit->rules[context];
    for (j = 1; c->keyword[j]; j++) {
/* if a keyword has been around for more than TRANSIENT_WORD_TIME_OUT 
   seconds, then remove it - we don't want to run out of space or makes syntax highlighting to slow */
	if (c->keyword[j]->time) {
	    if (c->keyword[j]->time + TRANSIENT_WORD_TIME_OUT < t) {
		edit->force |= REDRAW_PAGE;
		edit_syntax_clear_keyword (edit, context, j);
		j--;
	    }
	}
    }
/* are we out of space? */
    if (j >= MAX_WORDS_PER_CONTEXT - 2)
	return 1;
/* add the new keyword and date it */
    c->keyword[j + 1] = 0;
    c->keyword[j] = syntax_malloc (sizeof (struct key_word));
#ifdef MIDNIGHT
    c->keyword[j]->fg = SPELLING_ERROR;
#else
    c->keyword[j]->fg = c->keyword[0]->fg;
    c->keyword[j]->bg = SPELLING_ERROR;
#endif
    c->keyword[j]->keyword = (char *) strdup (keyword);
    c->keyword[j]->first = *c->keyword[j]->keyword;
    c->keyword[j]->whole_word_chars_left = (char *) strdup ("--abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ????????????????????????????????????????????????????????????????????????????????????????????????????");
    c->keyword[j]->whole_word_chars_right = (char *) strdup ("--abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ????????????????????????????????????????????????????????????????????????????????????????????????????");
    c->keyword[j]->whole_word_chars_left[0] = (c->first_left == '\'' ? '-' : '\'');
    c->keyword[j]->whole_word_chars_right[0] = (c->first_right == '\'' ? '-' : '\'');
    c->keyword[j]->time = t;
    s = strdupc (c->keyword_first_chars, c->keyword[j]->first);
    syntax_free (c->keyword_first_chars);
    c->keyword_first_chars = s;
    return 0;
}

/* checks spelling of the word at offset */
static int edit_check_spelling_at (WEdit * edit, long byte_index)
{
    int context;
    long p1, p2;
    unsigned char *p, *q;
    int r, c1, c2, j;
    int ch;
    time_t t;
    struct context_rule *c;
/* sanity check */
    if (!edit->rules || byte_index > edit->last_byte)
	return 0;
/* in what context are we */
    context = edit_get_rule (edit, byte_index).context;
    c = edit->rules[context];
/* does this context have `spellcheck' */
    if (!edit->rules[context]->spelling)
	return 0;
/* find word start */
    for (p1 = byte_index - 1;; p1--) {
	ch = edit_get_byte (edit, p1);
	if (isalpha (ch))
	    continue;
        if (ch == c->first_left)
	    break;
	if (ch == '-' || ch == '\'')
	    continue;
	break;
    }
    p1++;
/* find word end */
    for (p2 = byte_index;; p2++) {
	ch = edit_get_byte (edit, p2);
	if (isalpha (ch))
	    continue;
        if (ch == c->first_right)
	    break;
	if (ch == '-' || ch == '\'')
	    continue;
	break;
    }
    if (p2 <= p1)
	return 0;
/* create string */
    q = p = syntax_malloc (p2 - p1 + 2);
    for (; p1 < p2; p1++)
	*p++ = edit_get_byte (edit, p1);
    *p = '\0';
    if (q[0] == '-' || strlen ((char *) q) > 40) {	/* if you are using words over 40 characters, you are on your own */
	syntax_free (q);
	return 0;
    }
    time (&t);
    for (j = 1; c->keyword[j]; j++) {
/* if the keyword is present, then update its time only. if it is a fixed keyword from the rules file, then just return */
	if (!strcmp (c->keyword[j]->keyword, (char *) q)) {
	    if (c->keyword[j]->time)
		c->keyword[j]->time = t;
	    syntax_free (q);
	    return 0;
	}
    }
/* feed it to ispell */
    fprintf (spelling_pipe_out, "%s\n", (char *) q);
    fflush (spelling_pipe_out);
/* what does ispell say? */
    do {
	r = fgetc (spelling_pipe_in);
    } while (r == -1 && errno == EINTR);
    if (r == -1) {
	syntax_free (q);
	return 1;
    }
    if (r == '\n') {		/* ispell sometimes returns just blank line if it is given bad characters */
	syntax_free (q);
	return 0;
    }
/* now read ispell output untill we get two blanks lines - we are not intersted in this part */
    do {
	c1 = fgetc (spelling_pipe_in);
    } while (c1 == -1 && errno == EINTR);
    for (;;) {
	if (c1 == -1) {
	    syntax_free (q);
	    return 1;
	}
	do {
	    c2 = fgetc (spelling_pipe_in);
	} while (c2 == -1 && errno == EINTR);
	if (c1 == '\n' && c2 == '\n')
	    break;
	c1 = c2;
    }
/* spelled ok */
    if (r == '*' || r == '+' || r == '-') {
	syntax_free (q);
	return 0;
    }
/* not spelled ok - so add a syntax keyword for this word */
    edit_syntax_add_keyword (edit, (char *) q, context, t);
    syntax_free (q);
    return 0;
}

char *option_alternate_dictionary = "";

int PATH_search (const char *file);

#if defined(__GNUC__) && defined(__STRICT_ANSI__)
FILE *fdopen(int fd, const char *mode);
#endif

int edit_check_spelling (WEdit * edit)
{
    if (!option_auto_spellcheck)
	return 0;
/* magic arg to close up shop */
    if (!edit) {
	option_auto_spellcheck = 0;
	goto close_spelling;
    }
/* do we at least have a syntax rule struct to put new wrongly spelled keyword in for highlighting? */
    if (!edit->rules && !edit->explicit_syntax)
	edit_load_syntax (edit, 0, UNKNOWN_FORMAT);
    if (!edit->rules) {
	option_auto_spellcheck = 0;
	return 0;
    }
/* is ispell running? */
    if (!spelling_pipe_in) {
	int in, out, a = 0;
	char *arg[20];
	if (PATH_search ("aspell")) {
	    arg[a++] = "aspell";
	    arg[a++] = "--sug-mode=ultra";
	    if (option_alternate_dictionary && *option_alternate_dictionary) {
		arg[a++] = "-d";
		arg[a++] = option_alternate_dictionary;
	    }
	    arg[a++] = "-a";
	    arg[a++] = 0;
	} else {
	    arg[a++] = "ispell";
	    arg[a++] = "-a";
	    if (option_alternate_dictionary && *option_alternate_dictionary) {
		arg[a++] = "-d";
		arg[a++] = option_alternate_dictionary;
	    }
	    arg[a++] = "-a";
	    arg[a++] = 0;
	}
/* start ispell process */
	ispell_pid = triple_pipe_open (&in, &out, 0, 1, arg[0], arg);
	if (ispell_pid < 1 && errno == ENOENT) {	/* ispell not present in the path */
	    static int tries = 0;
	    option_auto_spellcheck = 0;
	    if (++tries >= 2)
		CErrorDialog (0, 0, 0, _(" Spelling Message "), "%s",
			      _
			      (" The ispell (or aspell) program could not be found in your path. \n Check that it is in your path and works with the -a option. "));
	    return 1;
	}
	if (ispell_pid < 1) {
	    option_auto_spellcheck = 0;
	    CErrorDialog (0, 0, 0, _(" Spelling Message "), "%s",
			  _
			  (" Fail trying to open ispell (or aspell) program. \n Check that it is in your path and works with the -a option. \n Alternatively, disable spell checking from the Options menu. "));
	    return 1;
	}
/* prepare pipes */
	spelling_pipe_in = (FILE *) fdopen (out, "r");
	spelling_pipe_out = (FILE *) fdopen (in, "w");
	if (!spelling_pipe_in || !spelling_pipe_out) {
	    option_auto_spellcheck = 0;
	    CErrorDialog (0, 0, 0, _(" Spelling Message "), "%s",
			  _
			  (" Fail trying to open ispell (or aspell) pipes. \n Check that it is in your path and works with the -a option. \n Alternatively, disable spell checking from the Options menu. "));
	    return 1;
	}
/* read the banner message */
        CDisableAlarm ();
	for (;;) {
	    int c1;
	    if ((c1 = fgetc (spelling_pipe_in)) == -1) {
		option_auto_spellcheck = 0;
		CErrorDialog (0, 0, 0, _(" Spelling Message "), "%s",
			      _
			      (" Fail trying to read ispell (or aspell) pipes. \n Check that it is in your path and works with the -a option. \n Alternatively, disable spell checking from the Options menu. "));
                CEnableAlarm ();
		return 1;
	    }
	    if (c1 == '\n')
		break;
	}
        CEnableAlarm ();
    }
/* spellcheck the word under the cursor */
    if (edit_check_spelling_at (edit, edit->curs1)) {
	CMessageDialog (0, 0, 0, 0, _(" Spelling Message "), "%s",
			_(" Error reading from ispell (or aspell). \n Ispell is being restarted. "));
      close_spelling:
	fclose (spelling_pipe_in);
	spelling_pipe_in = 0;
	fclose (spelling_pipe_out);
	spelling_pipe_out = 0;
	kill (ispell_pid, SIGKILL);
    }
    return 0;
}

#else				/* ! GTK && ! MIDNIGHT*/

int edit_check_spelling (WEdit * edit)
{
    return 0;
}

#endif

void (*syntax_change_callback) (CWidget *) = 0;

void edit_set_syntax_change_callback (void (*callback) (CWidget *))
{
    syntax_change_callback = callback;
}

void edit_free_syntax_rules (WEdit * edit)
{
    int i, j;
    if (!edit)
	return;
    if (!edit->rules)
	return;
    edit_get_rule (edit, -1);
    syntax_free (edit->syntax_type);
    edit->syntax_type = 0;
    if (syntax_change_callback)
#ifdef MIDNIGHT
	(*syntax_change_callback) (&edit->widget);
#else
	(*syntax_change_callback) (edit->widget);
#endif
    for (i = 0; edit->rules[i]; i++) {
	if (edit->rules[i]->keyword) {
	    for (j = 0; edit->rules[i]->keyword[j]; j++) {
		syntax_free (edit->rules[i]->keyword[j]->keyword);
		syntax_free (edit->rules[i]->keyword[j]->whole_word_chars_left);
		syntax_free (edit->rules[i]->keyword[j]->whole_word_chars_right);
		syntax_free (edit->rules[i]->keyword[j]);
	    }
	}
	syntax_free (edit->rules[i]->left);
	syntax_free (edit->rules[i]->right);
	syntax_free (edit->rules[i]->whole_word_chars_left);
	syntax_free (edit->rules[i]->whole_word_chars_right);
	syntax_free (edit->rules[i]->keyword);
	syntax_free (edit->rules[i]->keyword_first_chars);
	syntax_free (edit->rules[i]);
    }
    for (;;) {
	struct _syntax_marker *s;
	if (!edit->syntax_marker)
	    break;
	s = edit->syntax_marker->next;
	syntax_free (edit->syntax_marker);
	edit->syntax_marker = s;
    }
    syntax_free (edit->rules);
}

#define CURRENT_SYNTAX_RULES_VERSION "73"

char *syntax_text[] = {
"# syntax rules version " CURRENT_SYNTAX_RULES_VERSION,
"# (after the slash is a Cooledit color, 0-26 or any of the X colors in rgb.txt)",
"# black",
"# red",
"# green",
"# brown",
"# blue",
"# magenta",
"# cyan",
"# lightgray",
"# gray",
"# brightred",
"# brightgreen",
"# yellow",
"# brightblue",
"# brightmagenta",
"# brightcyan",
"# white",
"",
"file gobledy_gook #\\sHelp\\ssupport\\sother\\sfile\\stypes",
"context default",
"file gobledy_gook #\\sby\\scoding\\srules\\sin\\s~/.cedit/syntax.",
"context default",
"file gobledy_gook #\\sSee\\sman/syntax\\sin\\sthe\\ssource\\sdistribution",
"context default",
"file gobledy_gook #\\sand\\sconsult\\sthe\\sman\\spage.",
"context default",
"",
"",
"file ..\\*\\\\.(diff|rej|patch)$ Diff\\sOutput ^diff",
"include diff.syntax",
"",
"file ..\\*\\\\.lsm$ LSM\\sFile\\sVersion\\s4 ^Begin4",
"include lsm4.syntax",
"",
"file ..\\*\\\\.lsm$ LSM\\sFile\\sVersion\\s3 ^Begin3",
"include lsm.syntax",
"",
"file ..\\*\\\\.sh$ Shell\\sScript ^#!(\\s|)\\*/.\\*/(ksh|bash|sh|pdkzsh)",
"include sh.syntax",
"",
"file ..\\*\\\\.(pl|PL)$ Perl\\sProgram ^#!(\\s|)\\*/.\\*/perl",
"include perl.syntax",
"",
"file ..\\*\\\\.(py|PY)$ Python\\sProgram ^#!(\\s|)\\*/.\\*/python",
"include python.syntax",
"",
"file ..\\*\\\\.(man|[0-9n]|[0-9]x)$ NROFF\\sMan\\sPage ^(.\\\"\\s|.de\\s|.ds\\s|.\"Generated\\sby\\sdb2man.xsl|...\\\"\\s$Header|.so\\sman|.TH\\s)",
"include nroff.syntax",
"",
"file ..\\*\\\\.(htm|html|HTM|HTML)$ HTML\\sFile",
"include html.syntax",
"",
"file ..\\*\\\\.(pp|PP|pas|PAS)$ Pascal\\sProgram",
"include pascal.syntax",
"",
"file ..\\*\\\\.(ada|adb|ADA|ADB)$ Ada\\sProgram",
"include ada95.syntax",
"",
"file ..\\*\\\\.tex$ LaTeX\\s2e\\sDocument",
"include latex.syntax",
"",
"file ..\\*\\.(texi|texinfo|TEXI|TEXINFO)$ Texinfo\\sDocument",
"include texinfo.syntax",
"",
"file ..\\*\\\\.([chC]|CC|cxx|cc|cpp|CPP|CXX)$ C/C\\+\\+\\sProgram",
"include c.syntax",
"",
"file ..\\*\\\\.[fF]$ Fortran\\sProgram",
"include fortran.syntax",
"",
"file ..\\*\\\\.i$ SWIG\\sSource",
"include swig.syntax",
"",
"file ..\\*\\\\.php[0-9]$ PHP\\sProgram",
"include php.syntax",
"",
"file ..\\*\\\\.(java|JAVA|Java|jav)$ Java\\sProgram",
"include java.syntax",
"",
"file ..\\*\\\\.js$ Java\\sScript ^^#!(\\s|)\\*/.\\*/node$",
"include js.syntax",
"",
"file ..\\*\\\\.(st)$ SmallTalk\\sProgram",
"include smalltalk.syntax",
"",
"file ..\\*\\\\.(ml|mli|mly|mll|mlp)$ ML\\sProgram",
"include ml.syntax",
"",
"file ..\\*\\\\.(ly|fly|sly)$ Lilypond\\sSource",
"include mudela.syntax",
"",
"file .\\*ChangeLog$ GNU\\sDistribution\\sChangeLog\\sFile",
"include changelog.syntax",
"",
"file .\\*Makefile[\\\\\\.a-z]\\*$ Makefile",
"include makefile.syntax",
"",
"file ..\\*\\\\.(bap|bp2|bp3|bpp|bpe|lib|l32)$ BAssPasC\\sProgram",
"include bapc.syntax",
"",
"file ..\\*\\\\.(mhtml|mac)$ Macro-HTML\\sSource",
"include mhtml.syntax",
"",
"file ..\\*\\\\.(jasm|JASM)$ Java\\sAssembly",
"include jasm.syntax",
"",
"file ..\\*\\\\.(lua|LUA)$ LUA\\sSource",
"include lua.syntax",
"",
"file ..\\*\\\\.(p|P)$ Prolog\\sProgram",
"include prolog.syntax",
"",
"file ..\\*\\\\.(xml|XML)$ XML\\sFile <?xml",
"include xml.syntax",
"",
"file ..\\*\\\\.(css|CSS)$ CSS\\sFile <?xml",
"include css.syntax",
"",
"file ..\\*\\\\.(sql|SQL)$ SQL\\sFile",
"include sql.syntax",
"",
"file ..\\*\\\\.(scm|SCM)$ Gimp\\sScript-Fu",
"include scm.syntax",
"",
"file Don_t_match_me Mail\\sfolder ^From\\s",
"include mail.syntax",
"",
"file .\\*syntax$ Syntax\\sHighlighting\\sdefinitions",
"",
"context default",
"    keyword whole spellch\\eck yellow/24",
"    keyword whole keyw\\ord\\[\\t\\s\\]brac\\ematch yellow/24",
"    keyword whole keyw\\ord yellow/24",
"    keyword whole whole\\[\\t\\s\\]l\\inestart brightcyan/17",
"    keyword whole whole\\[\\t\\s\\]l\\inestart brightcyan/17",
"    keyword whole wh\\oleleft\\[\\t\\s\\]l\\inestart brightcyan/17",
"    keyword whole wh\\oleright\\[\\t\\s\\]l\\inestart brightcyan/17",
"    keyword whole l\\inestart\\[\\t\\s\\]wh\\ole",
"    keyword whole l\\inestart\\[\\t\\s\\]wh\\ole",
"    keyword whole l\\inestart\\[\\t\\s\\]wh\\oleleft",
"    keyword whole l\\inestart\\[\\t\\s\\]wh\\oleright",
"    keyword wholeleft whole\\s brightcyan/17",
"    keyword wholeleft whole\\t brightcyan/17",
"    keyword whole wh\\oleleft brightcyan/17",
"    keyword whole wh\\oleright brightcyan/17",
"    keyword whole lin\\[e\\]start brightcyan/17",
"    keyword whole c\\ontext\\[\\t\\s\\]exclusive brightred/18",
"    keyword whole c\\ontext\\[\\t\\s\\]default brightred/18",
"    keyword whole c\\ontext brightred/18",
"    keyword whole wh\\olechars\\[\\t\\s\\]left brightcyan/17",
"    keyword whole wh\\olechars\\[\\t\\s\\]right brightcyan/17",
"    keyword whole wh\\olechars brightcyan/17",
"    keyword whole f\\ile brightgreen/6",
"    keyword whole in\\clude brightred/18",
"",
"    keyword whole 0 lightgray/0	blue/26",
"    keyword whole 1 lightgray/1	blue/26",
"    keyword whole 2 lightgray/2	blue/26",
"    keyword whole 3 lightgray/3	blue/26",
"    keyword whole 4 lightgray/4	blue/26",
"    keyword whole 5 lightgray/5	blue/26",
"    keyword whole 6 lightgray/6",
"    keyword whole 7 lightgray/7",
"    keyword whole 8 lightgray/8",
"    keyword whole 9 lightgray/9",
"    keyword whole 10 lightgray/10",
"    keyword whole 11 lightgray/11",
"    keyword whole 12 lightgray/12",
"    keyword whole 13 lightgray/13",
"    keyword whole 14 lightgray/14",
"    keyword whole 15 lightgray/15",
"    keyword whole 16 lightgray/16",
"    keyword whole 17 lightgray/17",
"    keyword whole 18 lightgray/18",
"    keyword whole 19 lightgray/19",
"    keyword whole 20 lightgray/20",
"    keyword whole 21 lightgray/21",
"    keyword whole 22 lightgray/22",
"    keyword whole 23 lightgray/23",
"    keyword whole 24 lightgray/24",
"    keyword whole 25 lightgray/25",
"    keyword whole 26 lightgray/26",
"",
"    keyword wholeleft black\\/ black/0",
"    keyword wholeleft red\\/ red/DarkRed",
"    keyword wholeleft green\\/ green/green3",
"    keyword wholeleft brown\\/ brown/saddlebrown",
"    keyword wholeleft blue\\/ blue/blue3",
"    keyword wholeleft magenta\\/ magenta/magenta3",
"    keyword wholeleft cyan\\/ cyan/cyan3",
"    keyword wholeleft lightgray\\/ lightgray/lightgray",
"    keyword wholeleft gray\\/ gray/gray",
"    keyword wholeleft brightred\\/ brightred/red",
"    keyword wholeleft brightgreen\\/ brightgreen/green1",
"    keyword wholeleft yellow\\/ yellow/yellow",
"    keyword wholeleft brightblue\\/ brightblue/blue1",
"    keyword wholeleft brightmagenta\\/ brightmagenta/magenta",
"    keyword wholeleft brightcyan\\/ brightcyan/cyan1",
"    keyword wholeleft white\\/ white/26",
"",
"context linestart # \\n brown/22",
"",
"file .\\* " UNKNOWN_FORMAT,
"include unknown.syntax",
"",
0};


FILE *upgrade_syntax_file (char *syntax_file)
{
    FILE *f;
    char *p;
    char line[80];
    f = fopen (syntax_file, "r");
    if (!f) {
	char **syntax_line;
	f = fopen (syntax_file, "w");
	if (!f)
	    return 0;
	for (syntax_line = syntax_text; *syntax_line; syntax_line++)
	    fprintf (f, "%s\n", *syntax_line);
	fclose (f);
	return fopen (syntax_file, "r");
    }
    memset (line, 0, 79);
    (void) fread (line, 80, 1, f);
    if (!strstr (line, "syntax rules version"))
	goto rename_rule_file;
    p = strstr (line, "version") + strlen ("version") + 1;
    if (atoi (p) < atoi (CURRENT_SYNTAX_RULES_VERSION)) {
	char s[1024];
      rename_rule_file:
	strcpy (s, syntax_file);
	strcat (s, ".OLD");
	unlink (s);
	rename (syntax_file, s);
	unlink (syntax_file);	/* might rename() fail ? */
#if defined(MIDNIGHT) || defined(GTK)
	edit_message_dialog (" Load Syntax Rules ", " Your syntax rule file is outdated \n A new rule file is being installed. \n Your old rule file has been saved with a .OLD extension. ");
#else
	CMessageDialog (0, 20, 20, 0, " Load Syntax Rules ", " Your syntax rule file is outdated \n A new rule file is being installed. \n Your old rule file has been saved with a .OLD extension. ");
#endif
	return upgrade_syntax_file (syntax_file);
    }
    rewind (f);
    return f;
}

static int apply_syntax_rules (WEdit * edit, FILE * f, int line, char *syntax_type)
{
    int line_error, result = 0;
    line_error = edit_read_syntax_rules (edit, f);
    if (line_error) {
	if (!error_file_name)	/* an included file */
	    result = line + line_error;
	else
	    result = line_error;
    } else {
	syntax_free (edit->syntax_type);
	edit->syntax_type = (char *) strdup (syntax_type);
/* if there are no rules then turn off syntax highlighting for speed */
	if (!edit->rules[1])
	    if (!edit->rules[0]->keyword[1] && !edit->rules[0]->spelling) {
		edit_free_syntax_rules (edit);
		return result;
	    }
/* notify the callback of a change in rule set */
	if (syntax_change_callback)
#ifdef MIDNIGHT
	    (*syntax_change_callback) (&edit->widget);
#else
	    (*syntax_change_callback) (edit->widget);
#endif
    }
    return result;
}

/* returns -1 on file error, line number on error in file syntax */
static int edit_read_syntax_file (WEdit * edit, char **names, char *syntax_file, char *editor_file,
				  char *first_line, char *type)
{
    FILE *f;
    regex_t r;
    regmatch_t pmatch[1];
    char *args[1024], *l = 0;
    int line = 0;
    int argc;
    int result = 0;
    int count = 0;
    int max_score = 0;
    long offset_at_max_score = 0;
    char *syntax_type = NULL;
    f = upgrade_syntax_file (syntax_file);
    if (!f)
	return -1;
    args[0] = 0;
    for (;;) {
	line++;
	syntax_free (l);
	if (!read_one_line (&l, f))
	    break;
	get_args (l, args, &argc);
	if (!args[0])
	    continue;
/* looking for `file ...' lines only */
	if (strcmp (args[0], "file")) {
	    free_args (args);
	    continue;
	}
/* must have two args or report error */
	if (!args[1] || !args[2]) {
	    result = line;
	    break;
	}
	if (names) {
/* 1: just collecting a list of names of rule sets */
	    names[count++] = (char *) strdup (args[2]);
	    names[count] = 0;
	} else if (type) {
/* 2: rule set was explicitly specified by the caller */
	    if (!strcmp (type, args[2])) {
		result = apply_syntax_rules (edit, f, line, args[2]);
		break;
	    }
	} else if (editor_file && edit) {
/* 3: auto-detect rule set from regular expressions */
	    int q = 0;
	    memset (&r, '\0', sizeof (r));
	    if (regcomp (&r, args[1], REG_EXTENDED)) {
		result = line;
		break;
	    }
/* does filename match arg 1 ? */
	    q = !regexec (&r, editor_file, 1, pmatch, 0);
	    regfree (&r);
	    if (args[3]) {
		memset (&r, '\0', sizeof (r));
		if (regcomp (&r, args[3], REG_EXTENDED)) {
		    result = line;
		    break;
		}
/* does first line match arg 3 ? */
		q += (!regexec (&r, first_line, 1, pmatch, 0));
		regfree (&r);
	    }
	    if (q > max_score) {
		max_score = q;
		offset_at_max_score = ftell (f);
		syntax_type = (char *) strdup ((char *) args[2]);
	    }
	}
	free_args (args);
    }
    if (editor_file && edit)
	if (!result && max_score) {
	    fseek (f, offset_at_max_score, 0);
	    result = apply_syntax_rules (edit, f, line, syntax_type);
	}
    if (syntax_type)
	free (syntax_type);
    free_args (args);
    syntax_free (l);
    fclose (f);
    return result;
}

static char *get_first_editor_line (WEdit * edit)
{
    int i;
    static char s[256];
    s[0] = '\0';
    if (!edit)
	return s;
    for (i = 0; i < 255; i++) {
	s[i] = edit_get_byte (edit, i);
	if (s[i] == '\n') {
	    s[i] = '\0';
	    break;
	}
    }
    s[255] = '\0';
    return s;
}

/* loads rules into edit struct. one of edit or names must be zero. if
   edit is zero, a list of types will be stored into name. type may be zero
   in which case the type will be selected according to the filename. */
void edit_load_syntax (WEdit * edit, char **names, char *type)
{
    int r;
    char *f;

    edit_free_syntax_rules (edit);

#ifdef MIDNIGHT
    if (!SLtt_Use_Ansi_Colors || !use_colors)
	return;
#endif

    if (edit) {
	if (!edit->filename)
	    return;
	if (!*edit->filename && !type)
	    return;
    }
    f = catstrs (local_home_dir, SYNTAX_FILE, NULL);
    CDisableAlarm();
    r = edit_read_syntax_file (edit, names, f, edit ? edit->filename : 0, get_first_editor_line (edit), type);
    CEnableAlarm();
    if (r == -1) {
	edit_free_syntax_rules (edit);
	edit_error_dialog (_ (" Load syntax file "), _ (" File access error "));
	return;
    }
    if (r) {
	char s[80];
	edit_free_syntax_rules (edit);
	sprintf (s, _ (" Error in file %s on line %d "), error_file_name ? error_file_name : f, r);
	edit_error_dialog (_ (" Load syntax file "), s);
	syntax_free (error_file_name);
	return;
    }
}

#else

int option_syntax_highlighting = 0;

void edit_load_syntax (WEdit * edit, char **names, char *type)
{
    return;
}

void edit_free_syntax_rules (WEdit * edit)
{
    return;
}

void edit_get_syntax_color (WEdit * edit, long byte_index, int *fg, int *bg)
{
    *fg = NORMAL_COLOR;
}

#endif		/* !defined(MIDNIGHT) || defined(HAVE_SYNTAXH) */



