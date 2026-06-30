/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* Unit test for classify.c — Unicode 17.0 character classification */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "classify.h"

static int tests_run = 0;
static int tests_failed = 0;

static const char *category_names[] = {
    "Cn", "Lu", "Ll", "Lt", "Lm", "Lo",
    "Mn", "Mc", "Me",
    "Nd", "Nl", "No",
    "Pc", "Pd", "Ps", "Pe", "Pi", "Pf", "Po",
    "Sm", "Sc", "Sk", "So",
    "Zs", "Zl", "Zp",
    "Cc", "Cf", "Cs", "Co",
};

struct test_case {
    unsigned long codepoint;
    int expected_cat;
    int expected_word;      /* 1 = word char, 0 = not word char */
    const char *description;
};

static struct test_case tests[] = {
    /* === ASCII controls (Cc) === */
    { 0x0000, UC_Cc, 0, "NULL" },
    { 0x0009, UC_Cc, 0, "TAB" },
    { 0x001F, UC_Cc, 0, "UNIT SEPARATOR" },

    /* === ASCII space (Zs) === */
    { 0x0020, UC_Zs, 0, "SPACE" },

    /* === ASCII punctuation (Po) === */
    { 0x0021, UC_Po, 0, "EXCLAMATION MARK" },
    { 0x002E, UC_Po, 0, "FULL STOP" },
    { 0x003F, UC_Po, 0, "QUESTION MARK" },

    /* === ASCII currency symbol (Sc) === */
    { 0x0024, UC_Sc, 0, "DOLLAR SIGN" },

    /* === ASCII open/close punctuation (Ps, Pe) === */
    { 0x0028, UC_Ps, 0, "LEFT PARENTHESIS" },
    { 0x0029, UC_Pe, 0, "RIGHT PARENTHESIS" },
    { 0x005B, UC_Ps, 0, "LEFT SQUARE BRACKET" },
    { 0x005D, UC_Pe, 0, "RIGHT SQUARE BRACKET" },

    /* === ASCII math symbols (Sm) === */
    { 0x002B, UC_Sm, 0, "PLUS SIGN" },
    { 0x003D, UC_Sm, 0, "EQUALS SIGN" },

    /* === ASCII dash punctuation (Pd) === */
    { 0x002D, UC_Pd, 0, "HYPHEN-MINUS" },

    /* === ASCII digits (Nd) === */
    { 0x0030, UC_Nd, 1, "DIGIT ZERO" },
    { 0x0039, UC_Nd, 1, "DIGIT NINE" },

    /* === ASCII uppercase letters (Lu) === */
    { 0x0041, UC_Lu, 1, "LATIN CAPITAL A" },
    { 0x005A, UC_Lu, 1, "LATIN CAPITAL Z" },

    /* === ASCII lowercase letters (Ll) === */
    { 0x0061, UC_Ll, 1, "LATIN SMALL A" },
    { 0x007A, UC_Ll, 1, "LATIN SMALL Z" },

    /* === ASCII connector punctuation (Pc) — underscore is a word char === */
    { 0x005F, UC_Pc, 1, "LOW LINE (underscore)" },

    /* === ASCII delete (Cc) === */
    { 0x007F, UC_Cc, 0, "DELETE" },

    /* === Latin-1 Supplement: letters (Ll, Lu) === */
    { 0x00E0, UC_Ll, 1, "LATIN SMALL A WITH GRAVE" },
    { 0x00C0, UC_Lu, 1, "LATIN CAPITAL A WITH GRAVE" },

    /* === Latin-1: punctuation and symbols === */
    { 0x00A1, UC_Po, 0, "INVERTED EXCLAMATION MARK" },
    { 0x00A9, UC_So, 0, "COPYRIGHT SIGN" },
    { 0x00AB, UC_Pi, 0, "LEFT-POINTING DOUBLE ANGLE QUOTATION MARK" },
    { 0x00BB, UC_Pf, 0, "RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK" },
    { 0x00BF, UC_Po, 0, "INVERTED QUESTION MARK" },

    /* === Latin-1: non-breaking space (Zs) === */
    { 0x00A0, UC_Zs, 0, "NO-BREAK SPACE" },

    /* === Latin-1: soft hyphen (Cf — format char, not a word char) === */
    { 0x00AD, UC_Cf, 0, "SOFT HYPHEN" },

    /* === General punctuation — THE ORIGINAL BUG: curly quotes === */
    { 0x2018, UC_Pi, 0, "LEFT SINGLE QUOTATION MARK" },
    { 0x2019, UC_Pf, 0, "RIGHT SINGLE QUOTATION MARK" },
    { 0x201C, UC_Pi, 0, "LEFT DOUBLE QUOTATION MARK" },
    { 0x201D, UC_Pf, 0, "RIGHT DOUBLE QUOTATION MARK" },
    { 0x201E, UC_Ps, 0, "DOUBLE LOW-9 QUOTATION MARK" },

    /* === Dashes and hyphens (Pd) === */
    { 0x2013, UC_Pd, 0, "EN DASH" },
    { 0x2014, UC_Pd, 0, "EM DASH" },

    /* === Various punctuation types === */
    { 0x2026, UC_Po, 0, "HORIZONTAL ELLIPSIS" },
    { 0x2022, UC_Po, 0, "BULLET" },
    { 0x2039, UC_Pi, 0, "SINGLE LEFT-POINTING ANGLE QUOTATION MARK" },
    { 0x203A, UC_Pf, 0, "SINGLE RIGHT-POINTING ANGLE QUOTATION MARK" },

    /* === Greek letters === */
    { 0x0391, UC_Lu, 1, "GREEK CAPITAL ALPHA" },
    { 0x03B1, UC_Ll, 1, "GREEK SMALL ALPHA" },
    { 0x03A9, UC_Lu, 1, "GREEK CAPITAL OMEGA" },
    { 0x03C9, UC_Ll, 1, "GREEK SMALL OMEGA" },

    /* === Cyrillic letters === */
    { 0x0410, UC_Lu, 1, "CYRILLIC CAPITAL A" },
    { 0x0430, UC_Ll, 1, "CYRILLIC SMALL A" },
    { 0x0416, UC_Lu, 1, "CYRILLIC CAPITAL ZHE" },

    /* === Hebrew letters (Lo) === */
    { 0x05D0, UC_Lo, 1, "HEBREW LETTER ALEF" },
    { 0x05EA, UC_Lo, 1, "HEBREW LETTER TAV" },

    /* === Arabic letters (Lo) === */
    { 0x0627, UC_Lo, 1, "ARABIC LETTER ALEF" },
    { 0x064A, UC_Lo, 1, "ARABIC LETTER YEH" },

    /* === Arabic combining marks (Mn) — nonspacing marks ARE word chars === */
    { 0x064E, UC_Mn, 1, "ARABIC FATHA" },
    { 0x0651, UC_Mn, 1, "ARABIC SHADDA" },

    /* === Devanagari letters (Lo) === */
    { 0x0905, UC_Lo, 1, "DEVANAGARI LETTER A" },
    { 0x0915, UC_Lo, 1, "DEVANAGARI LETTER KA" },

    /* === Devanagari combining marks (Mn, Mc) === */
    { 0x0901, UC_Mn, 1, "DEVANAGARI SIGN CANDRABINDU" },
    { 0x093E, UC_Mc, 1, "DEVANAGARI VOWEL SIGN AA" },

    /* === CJK Unified Ideographs (Lo) === */
    { 0x4E00, UC_Lo, 1, "CJK UNIFIED IDEOGRAPH-4E00 (one)" },
    { 0x4E8C, UC_Lo, 1, "CJK UNIFIED IDEOGRAPH-4E8C (two)" },
    { 0x9FBF, UC_Lo, 1, "CJK UNIFIED IDEOGRAPH-9FBF" },

    /* === CJK punctuation and symbols === */
    { 0x3001, UC_Po, 0, "IDEOGRAPHIC COMMA" },
    { 0x3002, UC_Po, 0, "IDEOGRAPHIC FULL STOP" },
    { 0x300C, UC_Ps, 0, "LEFT CORNER BRACKET" },
    { 0x300D, UC_Pe, 0, "RIGHT CORNER BRACKET" },

    /* === CJK fullwidth digits (Nd) === */
    { 0xFF10, UC_Nd, 1, "FULLWIDTH DIGIT ZERO" },
    { 0xFF19, UC_Nd, 1, "FULLWIDTH DIGIT NINE" },

    /* === Fullwidth Latin letters (Lu, Ll) === */
    { 0xFF21, UC_Lu, 1, "FULLWIDTH LATIN CAPITAL A" },
    { 0xFF41, UC_Ll, 1, "FULLWIDTH LATIN SMALL A" },

    /* === Titlecase letter (Lt) === */
    { 0x01C5, UC_Lt, 1, "LATIN CAPITAL LETTER D WITH SMALL LETTER Z" },

    /* === Modifier letter (Lm) === */
    { 0x02B0, UC_Lm, 1, "MODIFIER LETTER SMALL H" },

    /* === Letter number (Nl) === */
    { 0x2160, UC_Nl, 1, "ROMAN NUMERAL ONE" },
    { 0x2170, UC_Nl, 1, "SMALL ROMAN NUMERAL ONE" },

    /* === Other number (No) === */
    { 0x00BD, UC_No, 1, "VULGAR FRACTION ONE HALF" },

    /* === Enclosing mark (Me) === */
    { 0x0488, UC_Me, 1, "CYRILLIC COMBINING HUNDRED THOUSANDS SIGN" },

    /* === Math symbols (Sm) === */
    { 0x2200, UC_Sm, 0, "FOR ALL" },
    { 0x221E, UC_Sm, 0, "INFINITY" },

    /* === Currency symbols (Sc) === */
    { 0x00A3, UC_Sc, 0, "POUND SIGN" },
    { 0x20AC, UC_Sc, 0, "EURO SIGN" },
    { 0x20B9, UC_Sc, 0, "INDIAN RUPEE SIGN" },

    /* === Modifier symbol (Sk) === */
    { 0x005E, UC_Sk, 0, "CIRCUMFLEX ACCENT" },
    { 0x02C6, UC_Lm, 1, "MODIFIER LETTER CIRCUMFLEX ACCENT (Lm, not Sk)" },

    /* === Other symbols (So) === */
    { 0x2620, UC_So, 0, "SKULL AND CROSSBONES" },
    { 0x2665, UC_So, 0, "BLACK HEART SUIT" },

    /* === Line separator (Zl) === */
    { 0x2028, UC_Zl, 0, "LINE SEPARATOR" },

    /* === Paragraph separator (Zp) === */
    { 0x2029, UC_Zp, 0, "PARAGRAPH SEPARATOR" },

    /* === Format characters (Cf) === */
    { 0x200B, UC_Cf, 0, "ZERO WIDTH SPACE" },
    { 0x200D, UC_Cf, 0, "ZERO WIDTH JOINER" },
    { 0xFEFF, UC_Cf, 0, "ZERO WIDTH NO-BREAK SPACE (BOM)" },

    /* === Surrogates (Cs) === */
    { 0xD800, UC_Cs, 0, "SURROGATE HIGH (D800)" },
    { 0xDFFF, UC_Cs, 0, "SURROGATE LOW (DFFF)" },

    /* === Private Use (Co) === */
    { 0xE000, UC_Co, 0, "PRIVATE USE AREA START" },
    { 0xF8FF, UC_Co, 0, "PRIVATE USE AREA END" },

    /* === Unassigned (Cn) — noncharacters === */
    { 0xFFFF, UC_Cn, 0, "NONCHARACTER FFFF" },
    { 0x10FFFF, UC_Cn, 0, "MAX CODE POINT (noncharacter)" },

    /* === Hangul === */
    { 0xAC00, UC_Lo, 1, "HANGUL SYLLABLE GA" },
    { 0xD7A3, UC_Lo, 1, "HANGUL SYLLABLE HIH" },
};

static void run_tests(void)
{
    int i;
    int n = sizeof(tests) / sizeof(tests[0]);

    printf("Running %d Unicode classification tests...\n\n", n);

    for (i = 0; i < n; i++) {
        C_wchar_t c = (C_wchar_t) tests[i].codepoint;
        int cat = uc_general_category(c);
        int word = uc_is_word_char(c);

        tests_run++;
        if (cat != tests[i].expected_cat) {
            printf("FAIL [%d]: U+%04lX %s\n", i, tests[i].codepoint, tests[i].description);
            printf("       category: got %s, expected %s\n",
                   category_names[cat], category_names[tests[i].expected_cat]);
            tests_failed++;
            continue;
        }

        tests_run++;
        if (word != tests[i].expected_word) {
            printf("FAIL [%d]: U+%04lX %s\n", i, tests[i].codepoint, tests[i].description);
            printf("       is_word_char: got %d, expected %d\n", word, tests[i].expected_word);
            tests_failed++;
            continue;
        }

        printf("  OK  U+%06lX  %-12s  word=%d  %s\n",
               tests[i].codepoint, category_names[cat], word, tests[i].description);
    }

    printf("\n---\n");
    printf("Tests run:   %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);

    if (tests_failed > 0)
        printf("SOME TESTS FAILED\n");
    else
        printf("ALL TESTS PASSED\n");
}

int main(void)
{
    /* Quick smoke test: the original bug — curly quotes should NOT be word chars */
    C_wchar_t left_curly = 0x2018;   /* ' */
    C_wchar_t right_curly = 0x2019;  /* ' */
    C_wchar_t straight_quote = 0x27; /* ' */
    C_wchar_t letter_a = 0x61;       /* a */
    C_wchar_t digit_1 = 0x31;        /* 1 */
    C_wchar_t underscore = 0x5F;     /* _ */

    printf("=== Smoke test (original DELIMIT_TEXT bug) ===\n");
    printf("U+%04X (left curly quote '):  category=%s, word=%d (expect word=0)\n",
           left_curly, category_names[uc_general_category(left_curly)],
           uc_is_word_char(left_curly));
    printf("U+%04X (right curly quote '): category=%s, word=%d (expect word=0)\n",
           right_curly, category_names[uc_general_category(right_curly)],
           uc_is_word_char(right_curly));
    printf("U+%04X (straight quote '):   category=%s, word=%d (expect word=0)\n",
           straight_quote, category_names[uc_general_category(straight_quote)],
           uc_is_word_char(straight_quote));
    printf("U+%04X (letter a):           category=%s, word=%d (expect word=1)\n",
           letter_a, category_names[uc_general_category(letter_a)],
           uc_is_word_char(letter_a));
    printf("U+%04X (digit 1):            category=%s, word=%d (expect word=1)\n",
           digit_1, category_names[uc_general_category(digit_1)],
           uc_is_word_char(digit_1));
    printf("U+%04X (underscore _):       category=%s, word=%d (expect word=1)\n",
           underscore, category_names[uc_general_category(underscore)],
           uc_is_word_char(underscore));
    printf("\n");

    run_tests();

    return tests_failed > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
