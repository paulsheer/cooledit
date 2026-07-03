/* detectencoding.c - determine locale encoding via wcrtomb fingerprints
 * This is needed to decide if the regular expression library should
 * enable UTF-8 interpretation when the "Use locale encoding" switch
 * is set through the Options menu. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

struct fingerprint_s {
    const char *name;
    unsigned long wc;
    int elen;
    unsigned char expected[8];
};

/*
 * Order within each section matters:  when two encodings produce the
 * same bytes for the same test character, the FIRST entry that matches
 * wins.  Disambiguation entries must appear before the shared entry.
 */
static const struct fingerprint_s fingerprints[] = {

    /* ================================================================ */
    /*  WORD-32  (4 bytes per code unit)                                */
    /* ================================================================ */

    {"UTF-32BE",     0x41 /* L'A' */,   4, {0x00,0x00,0x00,0x41}},
    {"UTF-32LE",     0x41 /* L'A' */,   4, {0x41,0x00,0x00,0x00}},

    /* ================================================================ */
    /*  WORD-16  (2 bytes per code unit)                                */
    /* ================================================================ */

    {"UTF-16BE",     0x41 /* L'A' */,   2, {0x00,0x41}},
    {"UTF-16LE",     0x41 /* L'A' */,   2, {0x41,0x00}},

    /* ================================================================ */
    /*  MULTI-BYTE  (byte-oriented, max > 1, min = 1)                   */
    /* ================================================================ */

    /* --- UTF-8 --- */
    {"UTF-8",        0x4E2D /* L'中' */,  3, {0xE4,0xB8,0xAD}},

    /* --- GB18030 vs EUC-CN ---
     * Both encode 中 as {0xD6,0xD0}.  Disambiguate with €.
     *   GB18030:  € → {0xA2,0xE3}
     *   EUC-CN:   € → fails
     */
    {"GB18030",      0x20AC /* L'€' */,   2, {0xA2,0xE3}},
    {"EUC-CN",       0x4E2D /* L'中' */,  2, {0xD6,0xD0}},
    {"GB18030",      0x4E2D /* L'中' */,  2, {0xD6,0xD0}},   /* fallback if € test skipped */

    /* --- BIG5 vs EUC-TW ---
     * Both encode 中 as {0xA4,0xA4}.  EUC-TW uses SS2 (0x8E) for CNS
     * plane 2+ chars.  U+3400 㐀 → {0x8E,0xA2,0xC6,0xA7} in EUC-TW,
     * fails in BIG5.
     */
    {"EUC-TW",       0x3400 /* L'㐀' */,  4, {0x8E,0xA2,0xC6,0xA7}},
    {"BIG5",         0x4E2D /* L'中' */,  2, {0xA4,0xA4}},

    /* --- Shift_JIS vs Windows-31J (CP932) ---
     * Both encode 中 as {0x92,0x86}.  Disambiguate with FULLWIDTH NOT SIGN.
     *   Windows-31J: ￢ → {0x81,0xCA}
     *   Shift_JIS:     fails
     */
    {"Windows-31J",  0xFFE2 /* L'￢' */,  2, {0x81,0xCA}},
    {"Shift_JIS",    0x4E2D /* L'中' */,  2, {0x92,0x86}},

    /* --- EUC-JP, EUC-KR --- */
    {"EUC-JP",       0x4E2D /* L'中' */,  2, {0xC3,0xE6}},
    {"EUC-KR",       0x4E2D /* L'中' */,  2, {0xE4,0xB8}},

    /* ================================================================ */
    /*  SINGLE-BYTE                                                     */
    /* ================================================================ */

    /* --- Central / Eastern European ---
     * š (U+0161):
     *   ISO-8859-13  → 0xFA
     *   Win-1250     → 0x9A
     *   ISO-8859-2   → 0xB9
     *   ISO-8859-16  → 0xB9  (same; disambiguate with ș below)
     */
    {"ISO-8859-13",  0x0161 /* L'š' */,   1, {0xFA}},
    {"Windows-1250", 0x0161 /* L'š' */,   1, {0x9A}},

    /* ș (U+0219) ISO-8859-16 → 0xBA, ISO-8859-2 → fails */
    {"ISO-8859-16",  0x0219 /* L'ș' */,   1, {0xBA}},
    {"ISO-8859-2",   0x0161 /* L'š' */,   1, {0xB9}},

    /* --- Windows-1254 (Turkish) vs Windows-1252 ---
     * Both have € at 0x80.  Disambiguate with Ğ (U+011E):
     *   Win-1254:  Ğ → 0xD0
     *   Win-1252:  fails
     */
    {"Windows-1254", 0x011E /* L'Ğ' */,   1, {0xD0}},

    /* --- Windows-1253 (Greek) vs Windows-1252 ---
     * Both have € at 0x80.  Disambiguate with Γ (U+0393):
     *   Win-1253:  Γ → 0xC3
     *   Win-1252:  fails
     */
    {"Windows-1253", 0x0393 /* L'Γ' */,   1, {0xC3}},

    /* --- Western ---
     * Ÿ (U+0178):  Win-1252 → 0x9F, ISO-8859-15 → 0xBE, ISO-8859-1 → fails
     * € (U+20AC):   Win-1252 → 0x80, ISO-8859-15 → 0xA4
     */
    {"Windows-1252", 0x0178 /* L'Ÿ' */,   1, {0x9F}},
    {"Windows-1252", 0x20AC /* L'€' */,   1, {0x80}},
    {"ISO-8859-15",  0x20AC /* L'€' */,   1, {0xA4}},

    /* --- Baltic ---
     * ė (U+0117):
     *   Win-1257     → 0xEB
     *   ISO-8859-13  → 0xE7
     *   ISO-8859-4   → 0xEA
     *   ISO-8859-10  → 0xEA  (same; disambiguate with ĸ below)
     */
    {"Windows-1257", 0x0117 /* L'ė' */,   1, {0xEB}},
    {"ISO-8859-13",  0x0117 /* L'ė' */,   1, {0xE7}},

    /* ĸ (U+0138):  ISO-8859-4 → 0xA2, ISO-8859-10 → 0xFF */
    {"ISO-8859-10",  0x0138 /* L'ĸ' */,   1, {0xFF}},
    {"ISO-8859-4",   0x0138 /* L'ĸ' */,   1, {0xA2}},

    /* --- Cyrillic ---
     * ж (U+0436):  Win-1251 → 0xE6,  ISO-8859-5/KOI8-R/KOI8-U → 0xD6
     * Є (U+0404):  KOI8-U → 0xB4,     KOI8-R/ISO-8859-5 → fails
     * Ё (U+0401):  KOI8-R → 0xB3,     ISO-8859-5 → 0xA8
     */
    {"Windows-1251", 0x0436 /* L'ж' */,   1, {0xE6}},
    {"KOI8-U",       0x0404 /* L'Є' */,   1, {0xB4}},
    {"KOI8-R",       0x0401 /* L'Ё' */,   1, {0xB3}},
    {"ISO-8859-5",   0x0401 /* L'Ё' */,   1, {0xA8}},

    /* --- Turkish ---
     * € (U+20AC):  Win-1254 → 0x80,  ISO-8859-9/ISO-8859-3 → fails
     * İ (U+0130):  ISO-8859-9 → 0xDD, ISO-8859-3 → 0xA9
     */
    {"Windows-1254", 0x20AC /* L'€' */,   1, {0x80}},
    {"ISO-8859-9",   0x0130 /* L'İ' */,   1, {0xDD}},

    /* --- ISO-8859-3  (Latin-3, Southern European) ---
     * ħ (U+0127, Maltese): ISO-8859-3 → 0xB1
     */
    {"ISO-8859-3",   0x0127 /* L'ħ' */,   1, {0xB1}},

    /* --- Greek ---
     * € (U+20AC):  Win-1253 → 0x80,  ISO-8859-7 → fails
     * α (U+03B1):  ISO-8859-7 → 0xE1
     */
    {"Windows-1253", 0x20AC /* L'€' */,   1, {0x80}},
    {"ISO-8859-7",   0x03B1 /* L'α' */,   1, {0xE1}},

    /* --- Hebrew ---
     * ₪ (U+20AA):  Win-1255 → 0xBA,  ISO-8859-8 → fails
     * א (U+05D0):  ISO-8859-8 → 0xE0
     */
    {"Windows-1255", 0x20AA /* L'₪' */,   1, {0xBA}},
    {"ISO-8859-8",   0x05D0 /* L'א' */,   1, {0xE0}},

    /* --- Arabic ---
     * ، (U+060C):  Win-1256 → 0xA1,  ISO-8859-6 → 0xAC
     */
    {"Windows-1256", 0x060C /* L'،' */,   1, {0xA1}},
    {"ISO-8859-6",   0x060C /* L'،' */,   1, {0xAC}},

    /* --- Thai ---
     * ก (U+0E01):  ISO-8859-11 → 0xA1
     */
    {"ISO-8859-11",  0x0E01 /* L'ก' */,   1, {0xA1}},

    /* --- ISO-8859-14  (Latin-8, Celtic) ---
     * ŵ (U+0175, Welsh): ISO-8859-14 → 0xD0
     */
    {"ISO-8859-14",  0x0175 /* L'ŵ' */,   1, {0xD0}},

    /* --- ISO-8859-1  (Latin-1) ---
     * Must test AFTER more-specific Latin encodings since it's a
     * common subset.
     */
    {"ISO-8859-1",   0x00E9 /* L'é' */,   1, {0xE9}},

    /* --- US-ASCII  (strict 7-bit) ---
     * All 0x80+ tests above failed because US-ASCII rejects them.
     * 'A' confirms it's at least ASCII-compatible.
     */
    {"US-ASCII",     0x41 /* L'A' */,   1, {0x41}},

    /* --- ASCII-8BIT  (fallback) --- */
    {"ASCII-8BIT",   0x41 /* L'A' */,   1, {0x41}},
};

static int test_fingerprint(const struct fingerprint_s *fp)
{
    mbstate_t state;
    char buf[16];
    size_t len;

    memset(&state, 0, sizeof(state));
    len = wcrtomb(buf, fp->wc, &state);
    if (len == (size_t)-1)
        return 0;
    if ((int)len != fp->elen)
        return 0;
    return memcmp(buf, fp->expected, fp->elen) == 0;
}

const char *detect_locale_encoding(void)
{
    int i;
    setlocale(LC_CTYPE, "");

    for (i = 0; i < (int)(sizeof(fingerprints)/sizeof(fingerprints[0])); i++) {
        if (test_fingerprint(&fingerprints[i]))
            return fingerprints[i].name;
    }

    return "ASCII-8BIT";
}

#ifdef STANDALONE_TEST

#include <assert.h>

typedef struct {
    const char *lang;
    const char *expected;
} test_case_t;

static const test_case_t tests[] = {
    /* word-32 */
    {"en_US.UTF-32BE",     "UTF-32BE"},
    {"en_US.UTF-32LE",     "UTF-32LE"},
    /* word-16 */
    {"en_US.UTF-16BE",     "UTF-16BE"},
    {"en_US.UTF-16LE",     "UTF-16LE"},
    /* UTF-8 */
    {"en_US.UTF-8",        "UTF-8"},
    {"en_US.utf8",         "UTF-8"},
    /* multi-byte CJK */
    {"zh_CN.GB18030",      "GB18030"},
    {"zh_CN.EUC-CN",       "EUC-CN"},
    {"zh_TW.EUC-TW",       "EUC-TW"},
    {"zh_TW.BIG5",         "BIG5"},
    {"ja_JP.Windows-31J",  "Windows-31J"},
    {"ja_JP.CP932",        "Windows-31J"},
    {"ja_JP.Shift_JIS",    "Shift_JIS"},
    {"ja_JP.EUC-JP",       "EUC-JP"},
    {"ko_KR.EUC-KR",       "EUC-KR"},
    /* single-byte: Central/Eastern European */
    {"en_US.ISO-8859-13",  "ISO-8859-13"},
    {"pl_PL.CP1250",       "Windows-1250"},
    {"pl_PL.Windows-1250", "Windows-1250"},
    {"en_US.ISO-8859-16",  "ISO-8859-16"},
    {"en_US.ISO-8859-2",   "ISO-8859-2"},
    /* single-byte: Western */
    {"en_US.CP1252",       "Windows-1252"},
    {"en_US.Windows-1252", "Windows-1252"},
    {"en_US.ISO-8859-15",  "ISO-8859-15"},
    /* single-byte: Baltic */
    {"en_US.CP1257",       "Windows-1257"},
    {"en_US.Windows-1257", "Windows-1257"},
    {"en_US.ISO-8859-10",  "ISO-8859-10"},
    {"en_US.ISO-8859-4",   "ISO-8859-4"},
    /* single-byte: Cyrillic */
    {"ru_RU.CP1251",       "Windows-1251"},
    {"ru_RU.Windows-1251", "Windows-1251"},
    {"ru_RU.KOI8-U",       "KOI8-U"},
    {"ru_RU.KOI8-R",       "KOI8-R"},
    {"ru_RU.ISO-8859-5",   "ISO-8859-5"},
    /* single-byte: Turkish */
    {"tr_TR.CP1254",       "Windows-1254"},
    {"tr_TR.Windows-1254", "Windows-1254"},
    {"tr_TR.ISO-8859-9",   "ISO-8859-9"},
    /* single-byte: ISO-8859-3 */
    {"en_US.ISO-8859-3",   "ISO-8859-3"},
    /* single-byte: Greek */
    {"el_GR.CP1253",       "Windows-1253"},
    {"el_GR.Windows-1253", "Windows-1253"},
    {"el_GR.ISO-8859-7",   "ISO-8859-7"},
    /* single-byte: Hebrew */
    {"he_IL.CP1255",       "Windows-1255"},
    {"he_IL.Windows-1255", "Windows-1255"},
    {"he_IL.ISO-8859-8",   "ISO-8859-8"},
    /* single-byte: Arabic */
    {"ar_SA.CP1256",       "Windows-1256"},
    {"ar_SA.Windows-1256", "Windows-1256"},
    {"ar_SA.ISO-8859-6",   "ISO-8859-6"},
    /* single-byte: Thai */
    {"th_TH.ISO-8859-11",  "ISO-8859-11"},
    /* single-byte: Celtic */
    {"en_US.ISO-8859-14",  "ISO-8859-14"},
    /* single-byte: Latin-1 */
    {"en_US.ISO-8859-1",   "ISO-8859-1"},
    /* ASCII */
    {"en_US.US-ASCII",     "US-ASCII"},
    {"C",                  "US-ASCII"},
    {"POSIX",              "US-ASCII"},
    /* ASCII-8BIT shares é→0xE9 with ISO-8859-1, which is tested first */
    {"en_US.ASCII-8BIT",   "ISO-8859-1"},
};

int main(void)
{
    int passed = 0, failed = 0;
    int i;

    /* Ensure LC_ALL and LC_CTYPE don't override LANG */
    unsetenv("LC_ALL");
    unsetenv("LC_CTYPE");

    for (i = 0; i < (int)(sizeof(tests)/sizeof(tests[0])); i++) {
        setenv("LANG", tests[i].lang, 1);
        const char *detected = detect_locale_encoding();
        if (strcmp(detected, tests[i].expected) == 0) {
            printf("PASS: LANG=%-24s => %-16s\n", tests[i].lang, detected);
            passed++;
        } else {
            printf("FAIL: LANG=%-24s => %-16s (expected %s)\n",
                   tests[i].lang, detected, tests[i].expected);
            failed++;
        }
    }

    printf("\n%d passed, %d failed, %d total\n", passed, failed, passed + failed);
    return failed ? 1 : 0;
}
#endif

#ifdef STANDALONE_TEST2
int main(void)
{
    printf("%s\n", detect_locale_encoding());
    return 0;
}
#endif
