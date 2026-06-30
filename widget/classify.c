/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* classify.c - Unicode 17.0 character classification
   Copyright (C) 1996-2022 Paul Sheer
 */

#include "inspect.h"
#include <config.h>
#include "classify.h"

#include "gen_uc_table.inc"

int uc_general_category (C_wchar_t c)
{E_
    int lower = 0;
    int upper = UC_TABLE_SIZE - 1;
    int i = UC_TABLE_SIZE / 2;

    if (c < uc_table[0].start)
        return UC_Cn;
    if (c >= uc_table[upper].start)
        return uc_table[upper].category;

    do {
        if (c >= uc_table[i].start && c < uc_table[i + 1].start)
            return uc_table[i].category;
        if (c == uc_table[i + 1].start)
            return uc_table[i + 1].category;
        if (c < uc_table[i].start)
            upper = i - 1;
        else if (c > uc_table[i].start)
            lower = i + 1;
        i = (lower + upper) / 2;
    }
    while (lower <= upper);

    return UC_Cn;
}

int uc_is_word_char (C_wchar_t c)
{E_
    switch (uc_general_category (c)) {
    case UC_Lu:
    case UC_Ll:
    case UC_Lt:
    case UC_Lm:
    case UC_Lo:
    case UC_Nd:
    case UC_Nl:
    case UC_No:
    case UC_Mn:
    case UC_Mc:
    case UC_Me:
    case UC_Pc:
        return 1;
    default:
        return 0;
    }
}
