
/* See ../onigmo/ */

#include <assert.h>

#include "../onigmo/onigmo.h"

static OnigSyntaxType cooledit_syntax;
int edit_get_max_numrepl (void);

/* 
  ┌─────────────────────────────┬─────┬───────┬──────┐
  │           Syntax            │ \b  │ \< \> │ (?i: │
  ├─────────────────────────────┼─────┼───────┼──────┤
  │ GNU Regex, Grep             │ yes │ yes   │ no   │
  ├─────────────────────────────┼─────┼───────┼──────┤
  │ Perl, Ruby, Java, Python    │ yes │ no    │ yes  │
  ├─────────────────────────────┼─────┼───────┼──────┤
  │ POSIX Basic/Extended, Emacs │ no  │ —     │ no   │
  └─────────────────────────────┴─────┴───────┴──────┘
*/

void onig_set_cooledit_mode (void)
{
//     onig_set_default_syntax (ONIG_SYNTAX_GNU_REGEX);

    assert (edit_get_max_numrepl() <= ONIG_NREGION);

    onig_copy_syntax (&cooledit_syntax, ONIG_SYNTAX_GNU_REGEX);
    onig_set_syntax_op (&cooledit_syntax, onig_get_syntax_op (&cooledit_syntax) | ONIG_SYN_OP_ESC_LTGT_WORD_BEGIN_END);
    onig_set_syntax_op2 (&cooledit_syntax, onig_get_syntax_op2 (&cooledit_syntax) | ONIG_SYN_OP2_QMARK_GROUP_EFFECT);
    onig_set_default_syntax (&cooledit_syntax);
}

