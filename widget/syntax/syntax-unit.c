
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "regex.h"
#include "stringtools.h"


#define E_ 

struct syntax_rule {
    unsigned short keyword;
    unsigned char brace_depth;
    unsigned char end;
    unsigned char context;
    unsigned char _context;
#define RULE_ON_LEFT_BORDER 1
#define RULE_ON_RIGHT_BORDER 2
    unsigned char border;
};

#define MAX_WORDS_PER_CONTEXT	4096
#define MAX_CONTEXTS		128
#define MAX_PATH_LEN            1024
#define EDIT_DIR                "./"
#define LIBDIR                  "./"
#define SYNTAX_FILE             "unittest.syntax-start"

#define REDRAW_PAGE             1

int triple_pipe_open(int *in, ...)
{
    return -1;
}

int CErrorDialog(int x, ...)
{
    printf ("CErrorDialog\n");
    exit (1);
    return -1;
}

int CMessageDialog(int x, ...)
{
    printf ("CMessageDialog\n");
    exit (1);
    return -1;
}

void edit_error_dialog(const char *a, const char *b)
{
    printf ("edit_error_dialog [%s] [%s]\n", a, b);
    exit (1);
}

void CDisableAlarm(void)
{
}

void CEnableAlarm(void)
{
}

char *Cstrdup(const char *s);

struct inspect_st__;
struct inspect_st__ *inspect_data__ = NULL;

void inspect___(const char *s,...)
{
}

int PATH_search (const char *file)
{
    exit(1);
}

struct remotefs;
struct remotefs *remotefs_lookup (const char *host_, char *last_directory)
{
    exit (1);
}

int prop_font_strcolmove (unsigned char *str, int i, int column)
{
    exit (1);
}

const char local_home_dir[] = "./";
const char current_dir[] = "./";

struct CWidget_ {
};

typedef struct CWidget_ CWidget;

struct key_word {
    char *keyword;
    unsigned char first;
    char *whole_word_chars_left;
    char *whole_word_chars_right;
    time_t time;
#define NO_COLOR 0x7FFFFFFF
#define SPELLING_ERROR 0x7EFEFEFE
    char line_start;
    char brace_match;
    int bg;
    int fg;
};

struct context_rule {
    char *left;
    unsigned char first_left;
    char *right;
    unsigned char first_right;
    char line_start_left;
    char line_start_right;
    int between_delimiters;
    char *whole_word_chars_left;
    char *whole_word_chars_right;
    char *keyword_first_chars;
    int spelling;
/* first word is word[1] */
    struct key_word **keyword;
};

struct _syntax_marker {
    long offset;
    struct syntax_rule rule;
    struct _syntax_marker *next;
};

struct defin;

struct WEdit_ {
    CWidget *widget;

    unsigned char *text;
    long last_byte;
    long curs1;

    char *filename;

    int force;

    struct _syntax_marker *syntax_marker;
    struct defin *defin;
    struct context_rule **rules;
    int is_case_insensitive;
    long last_get_rule;
    struct syntax_rule rule;
    int syntax_invalidate;
    char *syntax_type;		/* description of syntax highlighting type being used */
    int explicit_syntax;	/* have we forced the syntax hi. type in spite of the filename? */
};

typedef struct WEdit_ WEdit;

int edit_get_byte(WEdit *e, int i)
{
    if (i == -1)
        return '\n';
    if (i == e->last_byte)
        return '\n';
    assert (i >= 0);
    assert (i < e->last_byte);
    return e->text[i];
}



#define NO_COLOR 0x7FFFFFFF

int allocate_color (char *color)
{
    if (!color)
	return NO_COLOR;
    return atoi (color);
}
