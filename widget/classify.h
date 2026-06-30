/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* classify.h - Unicode 17.0 character classification
   Copyright (C) 1996-2022 Paul Sheer
 */

#ifndef __CLASSIFY_H
#define __CLASSIFY_H

#include "stringtools.h"

enum uc_general_category {
    UC_Cn,                          /* Unassigned */
    UC_Lu,                          /* Uppercase_Letter */
    UC_Ll,                          /* Lowercase_Letter */
    UC_Lt,                          /* Titlecase_Letter */
    UC_Lm,                          /* Modifier_Letter */
    UC_Lo,                          /* Other_Letter */
    UC_Mn,                          /* Nonspacing_Mark */
    UC_Mc,                          /* Spacing_Mark */
    UC_Me,                          /* Enclosing_Mark */
    UC_Nd,                          /* Decimal_Number */
    UC_Nl,                          /* Letter_Number */
    UC_No,                          /* Other_Number */
    UC_Pc,                          /* Connector_Punctuation */
    UC_Pd,                          /* Dash_Punctuation */
    UC_Ps,                          /* Open_Punctuation */
    UC_Pe,                          /* Close_Punctuation */
    UC_Pi,                          /* Initial_Punctuation */
    UC_Pf,                          /* Final_Punctuation */
    UC_Po,                          /* Other_Punctuation */
    UC_Sm,                          /* Math_Symbol */
    UC_Sc,                          /* Currency_Symbol */
    UC_Sk,                          /* Modifier_Symbol */
    UC_So,                          /* Other_Symbol */
    UC_Zs,                          /* Space_Separator */
    UC_Zl,                          /* Line_Separator */
    UC_Zp,                          /* Paragraph_Separator */
    UC_Cc,                          /* Control */
    UC_Cf,                          /* Format */
    UC_Cs,                          /* Surrogate */
    UC_Co,                          /* Private_Use */
};

int uc_general_category (C_wchar_t c);

int uc_is_word_char (C_wchar_t c);

#endif  /* __CLASSIFY_H */
