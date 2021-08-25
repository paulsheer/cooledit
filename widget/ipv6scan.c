/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include <string.h>
#include <stdlib.h>


static void i_out (char **p, int i1, int i2, int base)
{E_
    const char *dig = "0123456789abcdef";
    char s[32], *q;
    i1 <<= 8;
    i1 |= i2;
    q = &s[31];
    *--q = '\0';
    do {
        *--q = dig[i1 % base];
        i1 /= base;
    } while (i1);
    while (*q) {
        **p = *q++;
        (*p)++;
    }
}

void ip_to_text (const void *ip, int addrlen, char *out)
{E_
    const unsigned char *p;
    int i, n;
    p = (const unsigned char *) ip;
    if (addrlen == 16) {
        int maxlen = 0, i_max = -1;
        if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 0 && p[4] == 0 && p[5] == 0 &&
            p[6] == 0 && p[7] == 0 && p[8] == 0 && p[9] == 0 && p[10] == 0xFF && p[11] == 0xFF) {
            addrlen = 4;
            n = 12;
        } else {
            n = 16;
        }
        for (i = 0; i < n; i += 2) {
            int j, zeros = 0;
            for (j = i; j < n; j += 2) {
                if (p[j] || p[j + 1])
                    break;
                zeros += 2;
            }
            if (maxlen < zeros || i_max == -1) {
                maxlen = zeros;
                i_max = i;
            }
            i += zeros;
        }
        for (i = 0; i < n;) {
            if (maxlen > 2 && i_max == i) {     /* single :0000: is not compressed to :: */
                if (!i)
                    *out++ = ':';
                *out++ = ':';
                i += maxlen;
                p += maxlen;
                continue;
            } else {
                i_out (&out, (unsigned int) p[0], (unsigned int) p[1], 16);
                if (i != 14)
                    *out++ = ':';
                p += 2;
                i += 2;
            }
        }
    }
    if (addrlen == 4) {
        for (i = 0; i < 4; i++) {
            i_out (&out, 0, (unsigned int) p[i], 10);
            if (i != 3)
                *out++ = '.';
        }
    }
    *out = '\0';
}

static int hex_dig_ord (unsigned char c)
{E_
    if (c >= '0' && c <= '9')
        return c - '0' + 0;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

#define STATE_V6                (1<<0)
#define STATE_V6CLEAR           (~STATE_V6)
#define STATE_V4                (1<<1)
#define STATE_V4MAPPED          (1<<2)
#define STATE_V4CLEAR           (~(STATE_V4 | STATE_V4MAPPED))

#define FMT_INV                 0
#define FMT_UNSET               1
#define FMT_HEX                 2
#define FMT_OCT                 3

#define NEXT                    last_last = last, last = c, c = *s++, consumed++
#define ER(en,x)                if(x) { res = -(en); goto errout; }

int text_to_ip (const char *s, int *consumed_, void *out, int *addr_len)
{E_
    int res = 0;
    unsigned char *r;
    unsigned short *p, a[8], b[8];
    unsigned long v4[4];
    unsigned long o = 0, d = 0, h = 0;
    int an = 0, bn = 0, v4n = 0;
    int i, *pn, digval;
    int last_last = -1, last = -1, c = -1, consumed = 0, colons = 0;
    pn = &an;
    p = a;
    *addr_len = 0;
    for (NEXT;; NEXT) {
        int num_format = FMT_INV, hex_chars = 0;
        ER (100, c == ':' && last == ':' && last_last == ':');
        ER (110, c == '.' && last == '.');
        if ((digval = hex_dig_ord (c)) != -1) {
            unsigned long last_o = 0, last_d = 0, last_h = 0;
            o = d = h = 0;
            num_format = FMT_UNSET;
            ER (120, last == ':' && last_last != ':' && !(an + bn));    /* :3 */
            do {
                hex_chars += (digval > 9);
#define ONEDIG(em,fmt,t,base) \
                t *= base; t += digval; if (num_format == fmt) { \
                    ER (em + 0, v4n > 0 && t > 255); \
                    ER (em + 1, t > 0xFFFFFFFF); \
                    ER (em + 2, last_##t > t); } \
                last_##t = t;
                ONEDIG (130, FMT_OCT, o, 8);
                ONEDIG (140, FMT_UNSET, d, 10);
                ONEDIG (150, FMT_HEX, h, 16);
                ER (160, colons && h > 0xFFFF);
                ER (170, hex_chars && num_format == FMT_UNSET && h > 0xFFFF);   /* abcde::1 */
                NEXT;
                if (!h && (c == 'x' || c == 'X')) {
                    /* ER (180, an + bn > 0); *//* 1::0x2.0x3.0x4.0x5 */
                    num_format = FMT_HEX;
                    NEXT;
                }
                digval = hex_dig_ord (c);
                if (!h && num_format != FMT_HEX && c >= '1' && c <= '9')
                    num_format = FMT_OCT;
            } while (digval != -1);
        }
        if (c != '.' && !v4n && num_format != FMT_INV) {
            ER (190, c == ':' && num_format == FMT_HEX);        /* 0x1: */
            ER (200, c != ':' && num_format == FMT_HEX && colons);      /* ::0x1 */
            ER (210, c == ':' && h > 0xffff);   /* 10000:3 */
            if (num_format == FMT_UNSET || num_format == FMT_OCT) {
                ER (220, an + bn >= 8);
                p[*pn] = h;
                (*pn)++;
            }
        }
        if (c == ':') {
            ER (230, an + bn >= 8);
            colons++;
            if (last == ':') {
                ER (240, p != a);
                p = b;
                pn = &bn;
            }
            continue;
        }
        if (c != ':' && num_format != FMT_INV) {
            if (colons && !v4n && c != '.') {
                /* 1::2 */
            } else {
                ER (245, colons && !(an == 6 || p == b));       /* 1:2.3.4.5 */
                ER (250, v4n >= 4);
                if (num_format == FMT_HEX) {
                    /* 1::0x2.0x3.0x4.0x5 or 0x2.0x3.0x4.0x5 or 0x1234 */
                    v4[v4n] = h;
                } else if (num_format == FMT_OCT) {
                    /* 1::02.03.04.05 or 02.03.04.05 or 01234 */
                    v4[v4n] = o;
                } else if (num_format == FMT_UNSET) {
                    /* 1::2.3.4.5 or 2.3.4.5 or 1234 */
                    ER (260, hex_chars && !colons);     /* abcd */
                    v4[v4n] = d;
                }
                ER (270, v4n && v4[v4n] > 255);
                v4n++;
            }
        }
        if (c == '.') {
            ER (280, v4n >= 4);
            ER (290, an + bn > 6);
            continue;
        }
        ER (300, last == ':' && last_last != ':');
        ER (310, last == '.');
        break;
    }

#define APPEND(a,b) do { *r++ = (a); *r++ = (b); } while (0)
    r = (unsigned char *) out;
    if (colons) {
        *addr_len = 16;
        ER (320, v4n != 0 && v4n != 4);
        ER (330, v4n == 4 && an + bn > 6);
        for (i = 0; i < an; i++)
            APPEND ((a[i] >> 8) & 0xFF, (a[i] >> 0) & 0xFF);
        for (i = 0; i < (v4n == 4 ? 6 : 8) - an - bn; i++)
            APPEND (0, 0);
        for (i = 0; i < bn; i++)
            APPEND ((b[i] >> 8) & 0xFF, (b[i] >> 0) & 0xFF);
    } else {
        ER (340, v4n != 4 && v4n != 1);
        *addr_len = 4;
    }
    if (v4n == 0) {
    } else if (v4n == 1) {
        APPEND ((v4[0] >> 24) & 0xff, (v4[0] >> 16) & 0xff);
        APPEND ((v4[0] >> 8) & 0xff, (v4[0] >> 0) & 0xff);
    } else {
        ER (350, v4n != 4);
        ER (360, v4[0] > 255);
        APPEND (v4[0], v4[1]);
        APPEND (v4[2], v4[3]);
    }

    if (consumed_)
        *consumed_ = consumed - 1;
    return 0;

  errout:
    if (consumed_)
        *consumed_ = consumed - 1;
    return res;
}


struct iprange_item {
    struct iprange_item *next;
    char addr1[16];
    int addr1len;
    char addr2[16];
    int addr2len;
};

struct iprange_list {
    struct iprange_item *first;
};

void iprange_free (struct iprange_list *l)
{E_
    struct iprange_item *p, *next;
    for (p = l->first; p; p = next) {
        next = p->next;
        free (p);
    }
    free (l);
}

int iprange_match (struct iprange_list *l, const void *a, int addrlen)
{E_
    struct iprange_item *p;
    int c = 0;
    for (p = l->first; p; p = p->next) {
        c++;
        if (addrlen != p->addr1len)
            continue;
        if (memcmp (a, p->addr1, addrlen) >= 0 && memcmp (a, p->addr2, addrlen) <= 0)
            return c;
    }
    return 0;
}

void iprange_to_text (struct iprange_list *l, char *out, int outlen)
{E_
    struct iprange_item *p;

    for (p = l->first; p; p = p->next) {
        int len;
        if (outlen < 128)
            return;

        ip_to_text (p->addr1, p->addr1len, out);
        len = strlen (out);
        out += len;
        outlen -= len;

        if (memcmp (p->addr1, p->addr2, p->addr2len)) {
            *out++ = '-';
            outlen--;
            ip_to_text (p->addr2, p->addr2len, out);
            len = strlen (out);
            out += len;
            outlen -= len;
        }

        if (p->next) {
            *out++ = ',';
            outlen--;
        }

        *out = '\0';
    }
}

struct iprange_list *iprange_parse (const char *text, int *consumed__)
{E_
    struct iprange_list *l = NULL;
    struct iprange_item n, *p, *last = NULL;
    int consumed_ = 0;

    l = (struct iprange_list *) malloc (sizeof (*l));
    memset (l, '\0', sizeof (*l));

    for (;;) {
        int consumed = 0;
        while (*text && (*text == ',' || ((unsigned char) *text) <= ' ')) {
            text++;
            consumed_++;
        }
        if (!*text)
            break;
        memset (&n, '\0', sizeof (n));
        if (text_to_ip (text, &consumed, n.addr1, &n.addr1len))
            goto errout;
        text += consumed;
        consumed_ += consumed;
        if (*text == '-') {
            text++;
            consumed_++;
            if (text_to_ip (text, &consumed, n.addr2, &n.addr2len))
                goto errout;
            if (n.addr2len != n.addr1len)
                goto errout;
            text += consumed;
            consumed_ += consumed;
        } else if (*text == ',' || !*text) {
            memcpy (n.addr2, n.addr1, sizeof (n.addr2));
            n.addr2len = n.addr1len;
        } else if (*text == '/') {
            int i, mask = 0;
            text++;
            consumed_++;
            memcpy (n.addr2, n.addr1, sizeof (n.addr2));
            n.addr2len = n.addr1len;
            if (!(*text >= '1' && *text <= '9'))
                goto errout;
            do {
                mask *= 10;
                mask += *text - '0';
                text++;
                consumed_++;
            } while (*text >= '0' && *text <= '9');
            if (mask > n.addr1len * 8)
                goto errout;
            for (i = 0; i < n.addr1len * 8; i++) {
                if (i >= mask) {
                    n.addr1[i / 8] &= ~(0x80 >> (i % 8));
                    n.addr2[i / 8] |= (0x80 >> (i % 8));
                }
            }
        }
        p = (struct iprange_item *) malloc (sizeof (*p));
        *p = n;
        p->next = NULL;
        if (!l->first) {
            l->first = p;
        } else {
            last->next = p;
        }
        last = p;
    }

    if (consumed__)
        *consumed__ = consumed_;
    return l;

  errout:
    if (consumed__)
        *consumed__ = consumed_;
    iprange_free (l);
    return NULL;
}


#ifdef UNITTEST



#include <stdio.h>
#include <assert.h>
#include <arpa/inet.h>

#define E(in,out_) \
    do { \
        in_ = in; \
        consumed = 0; \
        if (!(e = text_to_ip (in_, &consumed, ip, &l))) { \
            printf ("error1 line %d: {%s} => {%s}, \"%s\"\n", __LINE__, in_, ip, &in_[consumed]); \
            return 1; \
        } \
        /* ip_to_text (ip, l, out); */ \
        if (strcmp (&in_[consumed], out_)) { \
            printf ("error2 line %d(%d): {%s} => {}, \"%s\"\n", __LINE__, -e, in_, &in_[consumed]); \
            return 1; \
        } \
    } while (0)

#define S(in,out_,out2_) \
    do { \
        in_ = in; \
        consumed = 0; \
        if ((e = text_to_ip (in_, &consumed, ip, &l))) { \
            printf ("error3 line %d (%d): {%s} => {%s}, \"%s\"\n", __LINE__, -e, in_, ip, &in_[consumed]); \
            return 1; \
        } \
        ip_to_text (ip, l, out); \
        if (strcmp (out, out_)) { \
            printf ("error4 line %d: {%s} => {%s}, \"%s\"\n", __LINE__, in_, out, &in_[consumed]); \
            return 1; \
        } \
        if (strcmp (&in_[consumed], out2_)) { \
            printf ("error5 line %d: {%s} => {%s}, \"%s\"\n", __LINE__, in_, out, &in_[consumed]); \
            return 1; \
        } \
    } while (0)



int main (int argc, char **argv)
{E_
    int e;
    char *in_;
    char out[64];
    char ip[18];
    int l, consumed;

    consumed = 0;
    memset (ip, '~', sizeof (ip));

    if (argc == 2) {
        if ((e = text_to_ip (argv[1], &consumed, ip, &l))) {
            printf ("error line %d: [%s]\n", -e, &argv[1][consumed]);
        } else {
            assert (ip[16] == '~');
            ip_to_text (ip, l, out);
            printf ("%s\n[%s]\n", out, &argv[1][consumed]);
        }
        printf ("\n");
    }

    {
        const char *ex;
        int c, j;
        unsigned short y[8];
        ex = "fe80:0:0:0:b7f9:119e:1e73:d230";
//            fe80:0:0:0:b7f9:119e:1e73:d230
        c = sscanf (ex, "%hx:%hx:%hx:%hx:%hx:%hx:%hx:%hx", &y[0], &y[1], &y[2], &y[3], &y[4], &y[5], &y[6], &y[7]);
        printf ("c=%d\n", c);
        for (j = 0; j < 8; j++)
            y[j] = htons (y[j]);
        printf ("%hx:%hx:%hx:%hx:%hx:%hx:%hx:%hx", ntohs (y[0]), ntohs (y[1]), ntohs (y[2]), ntohs (y[3]), ntohs (y[4]), ntohs (y[5]), ntohs (y[6]), ntohs (y[7]));
        printf ("\n");
    }

    consumed = 0;

    {
        int ll;
        char lp[16];
        struct iprange_list *l;
        char range_Text[1024];
        const char *input = "1.2.3.4,1.2.3.8-1.2.3.10,FF00::1/8,10.10.10.255/30";
        l = iprange_parse (input, &consumed);
        iprange_to_text (l, range_Text, sizeof (range_Text));
        printf ("[%s]\n", input);
        printf ("[%s]\n", range_Text);

        text_to_ip ("10.10.11.0", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 0);
        text_to_ip ("10.10.10.255", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 4);
        text_to_ip ("10.10.10.254", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 4);
        text_to_ip ("10.10.10.252", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 4);
        text_to_ip ("10.10.10.251", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 0);

        text_to_ip ("1.2.3.4", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 1);
        text_to_ip ("1.2.3.8", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 2);
        text_to_ip ("1.2.3.9", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 2);
        text_to_ip ("1.2.3.10", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 2);
        text_to_ip ("1::1", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 0);
        text_to_ip ("FF00::", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 3);
        text_to_ip ("FFFF::", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 3);
        text_to_ip ("FE00::", NULL, lp, &ll);
        assert (iprange_match (l, lp, ll) == 0);

        input = "";
        l = iprange_parse (input, &consumed);
        assert (iprange_match (l, lp, ll) == 0);
    }

    consumed = 0;
    memset (ip, '~', sizeof (ip));

/* not every error is reachable. i.e. there is a degree of defensive programming. */
    E("1.", "");
    E("1.2.", "");
    E("1.2.3.", "");
    E("1.2.3.4.", ".");
    E("1.256.3.4", "6.3.4");
    E("1.2.3.4.5", ".5");
    E(":::", ":");
    E("1:::", ":");
    E("1:2:::", ":");
    E("1:2:3:::", ":");
    E("::3:::", "::");
    E("::3::4:5:6", ":4:5:6");
    E("..", ".");
    E("1..", ".");
    E("1.2..", ".");
    E("1.2.3..", ".");
    E("1.2.3.4..", "..");
    E("1:2:3:4:5:6:7:8.9", ".9");
    E(":4.5.6.7", "4.5.6.7");
    E(":4.5.6", "4.5.6");
    E(":4.5", "4.5");
    E(":4", "4");
    E("3:4.5.6.7", ".5.6.7");
    E("2:3:4.5.6.7", ".5.6.7");
    E("1:2:3:4.5.6.7", ".5.6.7");
    E("0:1:2:3:4.5.6.7", ".5.6.7");
    E("3:", "");
    E("3:4:", "");
    E("3:4:5:", "");
    E(":3", "3");
    E(":3:4", "3:4");
    E(":3:4:5", "3:4:5");
    S("1::2", "1::2", "");
    S("1::0x2.0x3.0x4.0x5", "1::203:405", "");
    S("0x2.0x3.0x4.0x5", "2.3.4.5", "");
    S("0x1234", "0.0.18.52", "");
    S("1::02.03.04.05", "1::203:405", "");
    S("02.03.04.05", "2.3.4.5", "");
    S("01234", "0.0.2.156", "");
    S("1::2.3.4.5", "1::203:405", "");
    S("2.3.4.5", "2.3.4.5", "");
    S("1234", "0.0.4.210", "");
    E("a", "");
    E("abcd", "");
    E("abcde", "e");
    E("0x2:3", ":3");
    E("10000:3", ":3");
    E("1:10000:3", "0:3");
    E("1:0x2:3", ":3");
    E("1:0x2::3", "::3");
    E("1:0x2", "");

/* these are not usually allowed, but I am making this flexible: */
    S("::ffff:1.2.3.013", "::ffff:1.2.3.11", "");
    S("::ffff:013.2.3.4", "::ffff:11.2.3.4", "");
    S("::ffff:1.2.3.0xB", "::ffff:1.2.3.11", "");
    S("::ffff:0xB.2.3.4", "::ffff:11.2.3.4", "");

    S("::1.2.3.013", "::102:30b", "");
    S("::013.2.3.4", "::b02:304", "");
    S("::1.2.3.0xB", "::102:30b", "");
    S("::0xB.2.3.4", "::b02:304", "");

    E("040000000000", "0");
    S("037777777777", "255.255.255.255", "");
    S("4294967295", "255.255.255.255", "");
    E("4294967296", "6");
    S("0xFFFFFFFF", "255.255.255.255", "");
    E("0x100000000", "0");

    S("0377.000000.0.01", "255.0.0.1", "");
    S("0::0", "::", "");
    S("017700000001", "127.0.0.1", "");
    S("0177.000000.0.01", "127.0.0.1", "");
    E("0177.000000", "");
    E("0177.000000.0", "");
    S("0x000000000000000000000007F.0000000000x0000000000000.00x0000000000000.00x000000001", "127.0.0.1", "");
    S("0x7F.0x0.0x0.0x1", "127.0.0.1", "");
    S("0X7F.0X0.0X0.0X1", "127.0.0.1", "");
    S("0XfF.0Xff.0XFf.0XFF", "255.255.255.255", "");
    S("0Xf1.0Xf2.0XF3.0XF4", "241.242.243.244", "");

    S("2130706433", "127.0.0.1", "");
    S("017700000001", "127.0.0.1", "");
    S("0017700000001", "127.0.0.1", "");
    S("000000000000000000000000000000000000000000000017700000001", "127.0.0.1", "");
    S("0xFFFFFFFF", "255.255.255.255", "");
    S("0XFFFFFFFF", "255.255.255.255", "");
    S("0x0FFFFFFFF", "255.255.255.255", "");
    S("0x00FFFFFFFF", "255.255.255.255", "");
    S("0x0000000000000000000000000000000000000000000000FFFFFFFF", "255.255.255.255", "");
    S("00xFFFFFFFF", "255.255.255.255", "");
    S("000xFFFFFFFF", "255.255.255.255", "");
    S("00000000000000000000000000000000000000000000000xFFFFFFFF", "255.255.255.255", "");
    S("0xFFFFFFFF", "255.255.255.255", "");
    S("00000000000000000000000x000000000000000000000000FFFFFFFF", "255.255.255.255", "");

    E("abcdef",  "ef");
    E("abcde",  "e");
    E("abc",  "");
    E("a",  "");
    E("",  "");

    S("::", "::", "");
    S("0::", "::", "");
    S("0::x", "::", "x");
    S("::x", "::", "x");
    S("::0y", "::", "y");
    S("0::0y", "::", "y");

    S("0::0", "::", "");
    S("0:0::0", "::", "");
    S("0:0:0::0", "::", "");
    S("0:0:0:0::0", "::", "");
    S("0:0:0:0:0::0", "::", "");
    S("0:0:0:0:0:0::0", "::", "");
    S("0:0:0:0:0:0:0::0", "::", "");
    E("0:0:0:0:0:0:0:0::0", "::0");

    E("0xFFFFFFFF1", "1");
    S("0xFFFFFFFF", "255.255.255.255", "");
    S("0xFFFFFFFFx", "255.255.255.255", "x");
    S("0xFFFFFFFFxx", "255.255.255.255", "xx");
    S("1.2.3.4", "1.2.3.4", "");
    S("255.255.255.255", "255.255.255.255", "");
    S("255.255.255.255x", "255.255.255.255", "x");
    S("4294967295", "255.255.255.255", "");
    S("4294967295x", "255.255.255.255", "x");
    E("4294967296", "6");
    S("1", "0.0.0.1", "");
    S("12", "0.0.0.12", "");
    S("123", "0.0.0.123", "");
    S("1234", "0.0.4.210", "");
    S("12345", "0.0.48.57", "");
    S("123456", "0.1.226.64", "");
    S("1234567", "0.18.214.135", "");
    S("12345678", "0.188.97.78", "");
    S("123456789", "7.91.205.21", "");
    S("1234567890", "73.150.2.210", "");
    E("12345678901", "1");
    S("1x", "0.0.0.1", "x");
    S("2130706433", "127.0.0.1", "");
    S("2130706433x", "127.0.0.1", "x");
    S("2130706434x", "127.0.0.2", "x");
    S("1.2.3.255", "1.2.3.255", "");
    S("1.2.3.255x", "1.2.3.255", "x");
    E("1.2.3.256", "6");

    S("1::2", "1::2", "");
    S("1::2x", "1::2", "x");
    S("::2", "::2", "");
    S("::2x", "::2", "x");
    S("::1234", "::1234", "");
    S("::1234x", "::1234", "x");
    S("::FFFF", "::ffff", "");
    S("::FFFFx", "::ffff", "x");
    E("::FFFF1", "1");
    S("::0FFFF", "::ffff", "");
    S("::0FFFFx", "::ffff", "x");
    S("::0000000000000000FFFF", "::ffff", "");
    S("::0000000000000000FFFFx", "::ffff", "x");

    S("2::", "2::", "");
    S("2::x", "2::", "x");
    S("1234::", "1234::", "");
    S("1234::x", "1234::", "x");
    S("FFFF::", "ffff::", "");
    S("FFFF::x", "ffff::", "x");
    E("FFFF1::", "1::");
    S("0FFFF::", "ffff::", "");
    S("0FFFF::x", "ffff::", "x");
    S("0000000000000000FFFF::", "ffff::", "");
    S("0000000000000000FFFF::x", "ffff::", "x");

    S("01:02:03:04:05:06:07:08", "1:2:3:4:5:6:7:8", "");
    S("1:2:3:4:5:6:7:8", "1:2:3:4:5:6:7:8", "");
    S("1:2:3:4:5:6:7:8x", "1:2:3:4:5:6:7:8", "x");

/* left most length of equal zeros is compressed to :: */
    S("1:0:0:4:5:0:0:8", "1::4:5:0:0:8", "");
    S("1:0:0:4:5:6:0:0", "1::4:5:6:0:0", "");
    S("::3:4:5:6:0:0", "::3:4:5:6:0:0", "");

    S("1::3:4:5:6:7:8", "1:0:3:4:5:6:7:8", "");
    S("1::3:4:5:6:7:8x", "1:0:3:4:5:6:7:8", "x");
    S("1:2::4:5:6:7:8", "1:2:0:4:5:6:7:8", "");
    S("1:2::4:5:6:7:8x", "1:2:0:4:5:6:7:8", "x");
    S("1:2:3::5:6:7:8", "1:2:3:0:5:6:7:8", "");
    S("1:2:3::5:6:7:8x", "1:2:3:0:5:6:7:8", "x");
    S("1:2:3:4::6:7:8", "1:2:3:4:0:6:7:8", "");
    S("1:2:3:4::6:7:8x", "1:2:3:4:0:6:7:8", "x");
    S("1:2:3:4:5::7:8", "1:2:3:4:5:0:7:8", "");
    S("1:2:3:4:5::7:8x", "1:2:3:4:5:0:7:8", "x");
    S("1:2:3:4:5:6::8", "1:2:3:4:5:6:0:8", "");
    S("1:2:3:4:5:6::8x", "1:2:3:4:5:6:0:8", "x");
    E("1:2:3:4:5:6:7:", "");
    E("1:2:3:4:5:6:7:x", "x");
    S("1:2:3:4:5:6:7::", "1:2:3:4:5:6:7:0", "");
    S("1:2:3:4:5:6:7::x", "1:2:3:4:5:6:7:0", "x");
    E("1:2:3:4:5:6:7:8::", "::");
    E("1:2:3:4:5:6:7:8::x", "::x");

    S("::FfFf:0177.0.0000.0000377", "::ffff:127.0.0.255", "");
    S("::FfFf:0xFF.0xFe.0xFd.000x000fC", "::ffff:255.254.253.252", "");
    S("::FfFf:0xFF.0xFe.0xFd.000x000fCx", "::ffff:255.254.253.252", "x");

    S("::FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    E("::ffFf:1.2.3", "");
    E("::FfFF:1.2", "");
    S("::ffFf:a", "::ffff:a", "");
    S("::ffFf:1", "::ffff:1", "");
    S("::FfFf:1x", "::ffff:1", "x");
    S("::ffFf:1234", "::ffff:1234", "");
    S("::ffFf:1234x", "::ffff:1234", "x");
    E("::ffFf:0x1234", "");
    S("::ffFf:0123", "::ffff:123", "");
    E("::ffFf:ffff1", "1");
    S("::ffFf:ffff", "::ffff:ffff", "");

    S("0::FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0:0::FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0:0:0::FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0:0:0:0::FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0:0:0:0:0::FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    E("0:0:0:0:0:0::FfFf:1.2.3.4", ".2.3.4");

    S("0::0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0:0::0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0:0:0::0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0:0:0:0::0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    E("0:0:0:0:0::0:FfFf:1.2.3.4", ".2.3.4");
    E("0:0:0:0:0:0::0:FfFf:1.2.3.4", ":1.2.3.4");

    S("0::0:0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0:0::0:0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0:0:0::0:0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    E("0:0:0:0::0:0:FfFf:1.2.3.4", ".2.3.4");
    E("0:0:0:0:0::0:0:FfFf:1.2.3.4", ":1.2.3.4");
    E("0:0:0:0:0:0::0:0:FfFf:1.2.3.4", ":FfFf:1.2.3.4");

    S("0::0:0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0::0:0:0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("0::0:0:0:0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    E("0::0:0:0:0:0:FfFf:1.2.3.4", ".2.3.4");
    E("0::0:0:0:0:0:0:FfFf:1.2.3.4", ":1.2.3.4");
    E("0::0:0:0:0:0:0:0:FfFf:1.2.3.4", ":FfFf:1.2.3.4");

    S("::0:0:0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("::0:0:0:0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    S("::0:0:0:0:0:FfFf:1.2.3.4", "::ffff:1.2.3.4", "");
    E("::0:0:0:0:0:0:FfFf:1.2.3.4", ".2.3.4");
    E("::0:0:0:0:0:0:0:FfFf:1.2.3.4", ":1.2.3.4");
    E("::0:0:0:0:0:0:0:0:FfFf:1.2.3.4", ":FfFf:1.2.3.4");

    printf ("Success\n");

    return 0;
}

#endif



