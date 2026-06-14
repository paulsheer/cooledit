
/* See ../onigmo/ */

#include "../onigmo/onigmo.h"

void onig_set_gnu_mode (void)
{
    onig_set_default_syntax (ONIG_SYNTAX_GNU_REGEX);
}

