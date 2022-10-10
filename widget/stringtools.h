/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* stringtools.h - convenient string utility functions
   Copyright (C) 1996-2022 Paul Sheer
 */


#ifndef STRING_TOOLS_H
#define STRING_TOOLS_H

#include "global.h"
#include "my_string.h"

#define clear(x,type) memset((x), 0, sizeof(type))


#define C_ALNUM(c)  (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z') || ((c) >= '0' && (c) <= '9') || ((c) == '_'))


struct simple_string {
    char *data;
    int len;
};

typedef struct simple_string CStr;
typedef int C_wchar_t;

#undef strdup
#define strdup(s) Cstrdup(s)
char *Cstrdup (const char *p);
int Cstrncasecmp (const char *p1, const char *p2, size_t n);
int Cstrcasecmp (const char *p1, const char *p2);
void *Cmemmove (void *dest, const void *src, size_t n);

CStr CStr_dup (const char *s);
CStr CStr_const (const char *s, int l);
CStr CStr_dupstr (CStr s);
void CStr_free(CStr *s);
CStr CStr_cpy (const char *s, int l);

char *replace_str (const char *in, const char *a, const char *b);
int strendswith(const char *s, const char *p);

char **interpret_shell_cmdline (const char *s);
void free_shell_cmdline (char **r);

short *shortset (short *s, int c, size_t n);
short *integerset (short *s, int c, size_t n);

/*move to col character from beginning of line with i in the line somewhere. */
/*If col is past the end of the line, it returns position of end of line. */
/*Can be used as movetobeginning of line if col = 0. */
long strfrombeginline (const char *str, int i, int col);

/*move forward from i, where `lines' can be negative --- moving backward */
/*returns pos of begin of line moved to */
long strmovelines (const char *str, long i, long lines, int width);

/*returns a positive or negative count of lines
   from i to i + amount */
long strcountlines (const char *str, long i, long amount, int width);

char *str_strip_nroff (char *t, int *l);

/*returns a null terminated string. The string
   is a copy of the line beginning at p and ending at '\n' 
   in the string src.
   The result must be free'd. */
char *strline (const char *src, int p);

int strcolmove (unsigned char *str, int i, int col);

/*  cat many strings together. Result must not be free'd.
   Free's your result after 32 calls. */
char *catstrs (const char *first,...);
void catstrs_clean (void);

/* for regexp */
enum {
    match_file, match_normal
};
extern int easy_patterns;
char *convert_pattern (char *pattern, int match_type, int do_group);
int regexp_match (char *pattern, char *string, int match_type);
char *name_trunc (const char *txt, int trunc_len);
char *pathdup_debug (const char *cfile, int cline, const char *host, const char *p, char *errmsg);
#define pathdup(host,p,errmsg)         pathdup_debug(__FILE__, __LINE__,(host),(p),(errmsg))
char *vsprintf_alloc (const char *fmt, va_list ap);
char *sprintf_alloc (const char *fmt,...);
size_t vfmtlen (const char *fmt, va_list ap);
int readall (int fd, char *buf, int len);
int writeall (int fd, char *buf, int len);
char *Citoa (int i);
#define itoa(c)         Citoa(c)
size_t strnlen (const char *s, size_t count);
void destroy (void **p);
char *get_temp_file_name (void);
int change_directory (const char *path, char *errmsg);
char *get_current_wd (char *buffer, int size);
char *strcasechr (const char *p, int c);
char *space_string (const char *s);
int Cstrlcpy (char *dst, const char *src, int siz);
void string_chomp (char *_p);


#define my_lower_case(c) tolower(c & 0xFF)

#endif

