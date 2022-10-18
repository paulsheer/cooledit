#!/usr/bin/python3

defs = """
#define edit_buffer_get_byte(e, i)              edit_get_byte (edit, i)
#define xx_tolower(e,c)                         (c)
#define whitespace(c)                           ((c) == ' ' || (c) == '\\t')
#define whiteness(c)                            (whitespace (c) || (c) == '\\n')
#define off_t                                   long
#define CONTEXT_RULE(x)                         ((struct context_rule *) (x))
#define SYNTAX_KEYWORD(x)                       ((syntax_keyword_t *) (x))
#define g_ptr_array_index(a,i)                  (a[i])
typedef struct key_word syntax_keyword_t;
typedef struct context_rule context_rule_t;

#define TRUE            1
#define FALSE           0

#define SYNTAX_TOKEN_STAR       '\\001'
#define SYNTAX_TOKEN_PLUS       '\\002'
#define SYNTAX_TOKEN_BRACKET    '\\003'
#define SYNTAX_TOKEN_BRACE      '\\004'

#define gboolean                int

"""


r = open('replace-syntax.txt').readlines()
s = open('syntax-orig.c').read()

for i in range(0, len(r), 2):
    p = r[i + 0].rstrip()
    q = r[i + 1].rstrip()
    q = q.replace('$', '\n')
    if not p:
        continue
    s = s.replace(p, q)

s = s.replace('/* (@) */', defs)

print(s)


