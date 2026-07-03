/* pseudo_wcrtomb.c - fake wcrtomb() driven by LANG=... env var
 *
 * Link with -Wl,--wrap=wcrtomb so this __wrap_wcrtomb replaces the
 * libc wcrtomb at link time.
 *
 * Reads the encoding suffix from LANG (everything after the '.') and
 * returns the byte sequences that detect_encoding.c fingerprints test
 * for that encoding.  Characters not listed for an encoding return
 * (size_t)-1 ("not representable").
 */
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* one (wchar_t, bytes) mapping */
typedef struct {
    unsigned long wc;
    int     len;
    char    bytes[8];
} mapping_t;

/* an encoding with its locale-suffix tag and byte mappings */
typedef struct {
    const char *suffix;       /* e.g. "UTF-8"                */
    const char *enc_name;     /* same as detect_encoding.c   */
    const mapping_t *maps;
    int           nmaps;
} pseudo_enc_t;

/* ------------------------------------------------------------------ */
/*  mapping tables — one per encoding                                 */
/* ------------------------------------------------------------------ */

/* ---- word-32 ---- */
static const mapping_t m_utf32be[] = {
    {0x41 /* L'A' */, 4, {0x00,0x00,0x00,0x41}},
};
static const mapping_t m_utf32le[] = {
    {0x41 /* L'A' */, 4, {0x41,0x00,0x00,0x00}},
};

/* ---- word-16 ---- */
static const mapping_t m_utf16be[] = {
    {0x41 /* L'A' */, 2, {0x00,0x41}},
};
static const mapping_t m_utf16le[] = {
    {0x41 /* L'A' */, 2, {0x41,0x00}},
};

/* ---- UTF-8 (every fingerprint char is representable) ---- */
static const mapping_t m_utf8[] = {
    {0x41 /* L'A' */,  1, {0x41}},
    {0x4E2D /* L'中' */, 3, {0xE4,0xB8,0xAD}},
};

/* ---- GB18030 ---- */
static const mapping_t m_gb18030[] = {
    {0x20AC /* L'€' */,  2, {0xA2,0xE3}},
    {0x4E2D /* L'中' */, 2, {0xD6,0xD0}},
};

/* ---- EUC-CN ---- */
static const mapping_t m_euccn[] = {
    {0x4E2D /* L'中' */, 2, {0xD6,0xD0}},
};

/* ---- EUC-TW ---- */
static const mapping_t m_euctw[] = {
    {0x3400 /* L'㐀' */, 4, {0x8E,0xA2,0xC6,0xA7}},
    {0x4E2D /* L'中' */, 2, {0xA4,0xA4}},
};

/* ---- BIG5 ---- */
static const mapping_t m_big5[] = {
    {0x4E2D /* L'中' */, 2, {0xA4,0xA4}},
};

/* ---- Windows-31J (CP932) ---- */
static const mapping_t m_cp932[] = {
    {0xFFE2 /* L'￢' */, 2, {0x81,0xCA}},
    {0x4E2D /* L'中' */, 2, {0x92,0x86}},
};

/* ---- Shift_JIS ---- */
static const mapping_t m_sjis[] = {
    {0x4E2D /* L'中' */, 2, {0x92,0x86}},
};

/* ---- EUC-JP ---- */
static const mapping_t m_eucjp[] = {
    {0x4E2D /* L'中' */, 2, {0xC3,0xE6}},
};

/* ---- EUC-KR ---- */
static const mapping_t m_euckr[] = {
    {0x4E2D /* L'中' */, 2, {0xE4,0xB8}},
};

/* ---- ISO-8859-13 ---- */
static const mapping_t m_iso8859_13[] = {
    {0x0161 /* L'š' */, 1, {0xFA}},
    {0x0117 /* L'ė' */, 1, {0xE7}},
};

/* ---- Windows-1250 ---- */
static const mapping_t m_cp1250[] = {
    {0x0161 /* L'š' */, 1, {0x9A}},
};

/* ---- ISO-8859-16 ---- */
static const mapping_t m_iso8859_16[] = {
    {0x0219 /* L'ș' */, 1, {0xBA}},
    {0x0161 /* L'š' */, 1, {0xB9}},
};

/* ---- ISO-8859-2 ---- */
static const mapping_t m_iso8859_2[] = {
    {0x0161 /* L'š' */, 1, {0xB9}},
};

/* ---- Windows-1252 ---- */
static const mapping_t m_cp1252[] = {
    {0x0178 /* L'Ÿ' */, 1, {0x9F}},
    {0x20AC /* L'€' */, 1, {0x80}},
};

/* ---- ISO-8859-15 ---- */
static const mapping_t m_iso8859_15[] = {
    {0x20AC /* L'€' */, 1, {0xA4}},
};

/* ---- Windows-1257 ---- */
static const mapping_t m_cp1257[] = {
    {0x0117 /* L'ė' */, 1, {0xEB}},
};

/* ---- ISO-8859-10 ---- */
static const mapping_t m_iso8859_10[] = {
    {0x0138 /* L'ĸ' */, 1, {0xFF}},
};

/* ---- ISO-8859-4 ---- */
static const mapping_t m_iso8859_4[] = {
    {0x0138 /* L'ĸ' */, 1, {0xA2}},
};

/* ---- Windows-1251 ---- */
static const mapping_t m_cp1251[] = {
    {0x0436 /* L'ж' */, 1, {0xE6}},
};

/* ---- KOI8-U ---- */
static const mapping_t m_koi8u[] = {
    {0x0404 /* L'Є' */, 1, {0xB4}},
};

/* ---- KOI8-R ---- */
static const mapping_t m_koi8r[] = {
    {0x0401 /* L'Ё' */, 1, {0xB3}},
};

/* ---- ISO-8859-5 ---- */
static const mapping_t m_iso8859_5[] = {
    {0x0401 /* L'Ё' */, 1, {0xA8}},
};

/* ---- Windows-1254 ---- */
static const mapping_t m_cp1254[] = {
    {0x011E /* L'Ğ' */, 1, {0xD0}},
    {0x20AC /* L'€' */, 1, {0x80}},
};

/* ---- ISO-8859-9 ---- */
static const mapping_t m_iso8859_9[] = {
    {0x0130 /* L'İ' */, 1, {0xDD}},
};

/* ---- ISO-8859-3 ---- */
static const mapping_t m_iso8859_3[] = {
    {0x0127 /* L'ħ' */, 1, {0xB1}},
};

/* ---- Windows-1253 ---- */
static const mapping_t m_cp1253[] = {
    {0x0393 /* L'Γ' */, 1, {0xC3}},
    {0x20AC /* L'€' */, 1, {0x80}},
};

/* ---- ISO-8859-7 ---- */
static const mapping_t m_iso8859_7[] = {
    {0x03B1 /* L'α' */, 1, {0xE1}},
};

/* ---- Windows-1255 ---- */
static const mapping_t m_cp1255[] = {
    {0x20AA /* L'₪' */, 1, {0xBA}},
};

/* ---- ISO-8859-8 ---- */
static const mapping_t m_iso8859_8[] = {
    {0x05D0 /* L'א' */, 1, {0xE0}},
};

/* ---- Windows-1256 ---- */
static const mapping_t m_cp1256[] = {
    {0x060C /* L'،' */, 1, {0xA1}},
};

/* ---- ISO-8859-6 ---- */
static const mapping_t m_iso8859_6[] = {
    {0x060C /* L'،' */, 1, {0xAC}},
};

/* ---- ISO-8859-11 ---- */
static const mapping_t m_iso8859_11[] = {
    {0x0E01 /* L'ก' */, 1, {0xA1}},
};

/* ---- ISO-8859-14 ---- */
static const mapping_t m_iso8859_14[] = {
    {0x0175 /* L'ŵ' */, 1, {0xD0}},
};

/* ---- ISO-8859-1 ---- */
static const mapping_t m_iso8859_1[] = {
    {0x00E9 /* L'é' */, 1, {0xE9}},
};

/* ---- US-ASCII ---- */
static const mapping_t m_usascii[] = {
    {0x41 /* L'A' */, 1, {0x41}},
};

/* ---- ASCII-8BIT ---- */
static const mapping_t m_ascii8bit[] = {
    {0x41 /* L'A' */, 1, {0x41}},
    {0x00E9 /* L'é' */, 1, {0xE9}},   /* passes bytes through */
};

/* ------------------------------------------------------------------ */
/*  master table                                                      */
/* ------------------------------------------------------------------ */

static const pseudo_enc_t encodings[] = {
    {"UTF-32BE",    "UTF-32BE",     m_utf32be,     1},
    {"UTF-32LE",    "UTF-32LE",     m_utf32le,     1},
    {"UTF-16BE",    "UTF-16BE",     m_utf16be,     1},
    {"UTF-16LE",    "UTF-16LE",     m_utf16le,     1},
    {"UTF-8",       "UTF-8",        m_utf8,        2},
    {"utf8",        "UTF-8",        m_utf8,        2},   /* lower-case variant */
    {"GB18030",     "GB18030",      m_gb18030,     2},
    {"EUC-CN",      "EUC-CN",       m_euccn,       1},
    {"EUC-TW",      "EUC-TW",       m_euctw,       2},
    {"BIG5",        "BIG5",         m_big5,        1},
    {"Windows-31J", "Windows-31J",  m_cp932,       2},
    {"CP932",       "Windows-31J",  m_cp932,       2},
    {"Shift_JIS",   "Shift_JIS",    m_sjis,        1},
    {"EUC-JP",      "EUC-JP",       m_eucjp,       1},
    {"EUC-KR",      "EUC-KR",       m_euckr,       1},
    {"ISO-8859-13", "ISO-8859-13",  m_iso8859_13,  2},
    {"CP1250",      "Windows-1250", m_cp1250,       1},
    {"Windows-1250","Windows-1250", m_cp1250,       1},
    {"ISO-8859-16", "ISO-8859-16",  m_iso8859_16,  2},
    {"ISO-8859-2",  "ISO-8859-2",   m_iso8859_2,   1},
    {"CP1252",      "Windows-1252", m_cp1252,       2},
    {"Windows-1252","Windows-1252", m_cp1252,       2},
    {"ISO-8859-15", "ISO-8859-15",  m_iso8859_15,  1},
    {"CP1257",      "Windows-1257", m_cp1257,       1},
    {"Windows-1257","Windows-1257", m_cp1257,       1},
    {"ISO-8859-10", "ISO-8859-10",  m_iso8859_10,  1},
    {"ISO-8859-4",  "ISO-8859-4",   m_iso8859_4,   1},
    {"CP1251",      "Windows-1251", m_cp1251,       1},
    {"Windows-1251","Windows-1251", m_cp1251,       1},
    {"KOI8-U",      "KOI8-U",       m_koi8u,       1},
    {"KOI8-R",      "KOI8-R",       m_koi8r,       1},
    {"ISO-8859-5",  "ISO-8859-5",   m_iso8859_5,   1},
    {"CP1254",      "Windows-1254", m_cp1254,       1},
    {"Windows-1254","Windows-1254", m_cp1254,       1},
    {"ISO-8859-9",  "ISO-8859-9",   m_iso8859_9,   1},
    {"ISO-8859-3",  "ISO-8859-3",   m_iso8859_3,   1},
    {"CP1253",      "Windows-1253", m_cp1253,       1},
    {"Windows-1253","Windows-1253", m_cp1253,       1},
    {"ISO-8859-7",  "ISO-8859-7",   m_iso8859_7,   1},
    {"CP1255",      "Windows-1255", m_cp1255,       1},
    {"Windows-1255","Windows-1255", m_cp1255,       1},
    {"ISO-8859-8",  "ISO-8859-8",   m_iso8859_8,   1},
    {"CP1256",      "Windows-1256", m_cp1256,       1},
    {"Windows-1256","Windows-1256", m_cp1256,       1},
    {"ISO-8859-6",  "ISO-8859-6",   m_iso8859_6,   1},
    {"ISO-8859-11", "ISO-8859-11",  m_iso8859_11,  1},
    {"ISO-8859-14", "ISO-8859-14",  m_iso8859_14,  1},
    {"ISO-8859-1",  "ISO-8859-1",   m_iso8859_1,   1},
    {"US-ASCII",    "US-ASCII",     m_usascii,     1},
    {"ASCII-8BIT",  "ASCII-8BIT",   m_ascii8bit,   2},
    {"C",           "US-ASCII",     m_usascii,     1},
    {"POSIX",       "US-ASCII",     m_usascii,     1},
};

/* find the encoding that matches the LANG suffix */
static const pseudo_enc_t *find_encoding(const char *lang)
{
    int i;
    if (!lang) return NULL;
    const char *dot = strrchr(lang, '.');
    const char *tag = dot ? dot + 1 : lang;   /* "C" has no dot */

    for (i = 0; i < (int)(sizeof(encodings)/sizeof(encodings[0])); i++) {
        if (strcmp(tag, encodings[i].suffix) == 0)
            return &encodings[i];
    }
    return NULL;
}

/* --- public entry point --- */

size_t __wrap_wcrtomb(char *s, wchar_t wc, mbstate_t *ps)
{
    int i;
    (void)ps;   /* stateless for all encodings we test */

    const char *lang = getenv("LANG");
    const pseudo_enc_t *enc = find_encoding(lang);
    if (!enc) return (size_t)-1;

    for (i = 0; i < enc->nmaps; i++) {
        if (enc->maps[i].wc == wc) {
            if (s) memcpy(s, enc->maps[i].bytes, enc->maps[i].len);
            return (size_t)enc->maps[i].len;
        }
    }
    return (size_t)-1;
}
