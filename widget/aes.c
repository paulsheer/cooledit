/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* Copyright (C) 2020  Paul Sheer, All rights reserved. */


/* This file is derived from the OpenSSL-1.1.1 aes*.c source
 * and the Haskell source by Vincent Hanquez (see below)
 * 
 * This code does AES 16-byte blocks only. In software it supports
 * 128, 192, 256 key sizes. For Intel AES-NI (hardware encryption)
 * it supports 256 bit keys only.
 * 
 * For 32-bit builds AES-NI is not supported.
 */

#ifdef __clang__
#define AES_CLANG_ATTR  __attribute__((__always_inline__, __nodebug__, __target__("aes")))
#else
#define AES_CLANG_ATTR
#pragma GCC optimize("O6")
#pragma GCC target("aes")
#endif

#include "inspect.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define GETU32(pt) (((u32)(pt)[0] << 24) ^ ((u32)(pt)[1] << 16) ^ ((u32)(pt)[2] <<  8) ^ ((u32)(pt)[3]))
#define PUTU32(ct, st) { (ct)[0] = (u8)((st) >> 24); (ct)[1] = (u8)((st) >> 16); (ct)[2] = (u8)((st) >>  8); (ct)[3] = (u8)(st); }

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#include "aes_const.h"
#include "aes.h"




int aes_set_encrypt_key(const unsigned char *userKey, const int bits,
                        struct aes_key_st *key)
{E_
    u32 *rk;
    int i = 0;
    u32 temp;

    if (!userKey || !key)
        return -1;
    if (bits != 128 && bits != 192 && bits != 256)
        return -2;

    rk = (u32 *) key->rd_key;

    if (bits == 128)
        key->rounds = 10;
    else if (bits == 192)
        key->rounds = 12;
    else
        key->rounds = 14;

    rk[0] = GETU32(userKey     );
    rk[1] = GETU32(userKey +  4);
    rk[2] = GETU32(userKey +  8);
    rk[3] = GETU32(userKey + 12);
    if (bits == 128) {
        while (1) {
            temp  = rk[3];
            rk[4] = rk[0] ^
                (Te2[(temp >> 16) & 0xff] & 0xff000000) ^
                (Te3[(temp >>  8) & 0xff] & 0x00ff0000) ^
                (Te0[(temp      ) & 0xff] & 0x0000ff00) ^
                (Te1[(temp >> 24)       ] & 0x000000ff) ^
                rcon[i];
            rk[5] = rk[1] ^ rk[4];
            rk[6] = rk[2] ^ rk[5];
            rk[7] = rk[3] ^ rk[6];
            if (++i == 10) {
                return 0;
            }
            rk += 4;
        }
    }
    rk[4] = GETU32(userKey + 16);
    rk[5] = GETU32(userKey + 20);
    if (bits == 192) {
        while (1) {
            temp = rk[ 5];
            rk[ 6] = rk[ 0] ^
                (Te2[(temp >> 16) & 0xff] & 0xff000000) ^
                (Te3[(temp >>  8) & 0xff] & 0x00ff0000) ^
                (Te0[(temp      ) & 0xff] & 0x0000ff00) ^
                (Te1[(temp >> 24)       ] & 0x000000ff) ^
                rcon[i];
            rk[ 7] = rk[ 1] ^ rk[ 6];
            rk[ 8] = rk[ 2] ^ rk[ 7];
            rk[ 9] = rk[ 3] ^ rk[ 8];
            if (++i == 8) {
                return 0;
            }
            rk[10] = rk[ 4] ^ rk[ 9];
            rk[11] = rk[ 5] ^ rk[10];
            rk += 6;
        }
    }
    rk[6] = GETU32(userKey + 24);
    rk[7] = GETU32(userKey + 28);
    if (bits == 256) {
        while (1) {
            temp = rk[ 7];
            rk[ 8] = rk[ 0] ^
                (Te2[(temp >> 16) & 0xff] & 0xff000000) ^
                (Te3[(temp >>  8) & 0xff] & 0x00ff0000) ^
                (Te0[(temp      ) & 0xff] & 0x0000ff00) ^
                (Te1[(temp >> 24)       ] & 0x000000ff) ^
                rcon[i];
            rk[ 9] = rk[ 1] ^ rk[ 8];
            rk[10] = rk[ 2] ^ rk[ 9];
            rk[11] = rk[ 3] ^ rk[10];
            if (++i == 7) {
                return 0;
            }
            temp = rk[11];
            rk[12] = rk[ 4] ^
                (Te2[(temp >> 24)       ] & 0xff000000) ^
                (Te3[(temp >> 16) & 0xff] & 0x00ff0000) ^
                (Te0[(temp >>  8) & 0xff] & 0x0000ff00) ^
                (Te1[(temp      ) & 0xff] & 0x000000ff);
            rk[13] = rk[ 5] ^ rk[12];
            rk[14] = rk[ 6] ^ rk[13];
            rk[15] = rk[ 7] ^ rk[14];

            rk += 8;
            }
    }
    return 0;
}

int aes_set_decrypt_key(const unsigned char *userKey, const int bits,
                        struct aes_key_st *key)
{E_
    u32 *rk;
    int i, j, status;
    u32 temp;

    status = aes_set_encrypt_key(userKey, bits, key);
    if (status < 0)
        return status;

    rk = (u32 *) key->rd_key;

    for (i = 0, j = 4*(key->rounds); i < j; i += 4, j -= 4) {
        temp = rk[i    ]; rk[i    ] = rk[j    ]; rk[j    ] = temp;
        temp = rk[i + 1]; rk[i + 1] = rk[j + 1]; rk[j + 1] = temp;
        temp = rk[i + 2]; rk[i + 2] = rk[j + 2]; rk[j + 2] = temp;
        temp = rk[i + 3]; rk[i + 3] = rk[j + 3]; rk[j + 3] = temp;
    }

    for (i = 1; i < (key->rounds); i++) {
        rk += 4;
        rk[0] =
            Td0[Te1[(rk[0] >> 24)       ] & 0xff] ^
            Td1[Te1[(rk[0] >> 16) & 0xff] & 0xff] ^
            Td2[Te1[(rk[0] >>  8) & 0xff] & 0xff] ^
            Td3[Te1[(rk[0]      ) & 0xff] & 0xff];
        rk[1] =
            Td0[Te1[(rk[1] >> 24)       ] & 0xff] ^
            Td1[Te1[(rk[1] >> 16) & 0xff] & 0xff] ^
            Td2[Te1[(rk[1] >>  8) & 0xff] & 0xff] ^
            Td3[Te1[(rk[1]      ) & 0xff] & 0xff];
        rk[2] =
            Td0[Te1[(rk[2] >> 24)       ] & 0xff] ^
            Td1[Te1[(rk[2] >> 16) & 0xff] & 0xff] ^
            Td2[Te1[(rk[2] >>  8) & 0xff] & 0xff] ^
            Td3[Te1[(rk[2]      ) & 0xff] & 0xff];
        rk[3] =
            Td0[Te1[(rk[3] >> 24)       ] & 0xff] ^
            Td1[Te1[(rk[3] >> 16) & 0xff] & 0xff] ^
            Td2[Te1[(rk[3] >>  8) & 0xff] & 0xff] ^
            Td3[Te1[(rk[3]      ) & 0xff] & 0xff];
    }
    return 0;
}

void aes_encrypt(const unsigned char *in, unsigned char *out,
                 const struct aes_key_st *key)
{E_
    const u32 *rk;
    u32 s0, s1, s2, s3, t0, t1, t2, t3;
    int r;

    assert(in && out && key);
    rk = (u32 *) key->rd_key;

    s0 = GETU32(in     ) ^ rk[0];
    s1 = GETU32(in +  4) ^ rk[1];
    s2 = GETU32(in +  8) ^ rk[2];
    s3 = GETU32(in + 12) ^ rk[3];

    r = key->rounds >> 1;
    for (;;) {
        t0 =
            Te0[(s0 >> 24)       ] ^
            Te1[(s1 >> 16) & 0xff] ^
            Te2[(s2 >>  8) & 0xff] ^
            Te3[(s3      ) & 0xff] ^
            rk[4];
        t1 =
            Te0[(s1 >> 24)       ] ^
            Te1[(s2 >> 16) & 0xff] ^
            Te2[(s3 >>  8) & 0xff] ^
            Te3[(s0      ) & 0xff] ^
            rk[5];
        t2 =
            Te0[(s2 >> 24)       ] ^
            Te1[(s3 >> 16) & 0xff] ^
            Te2[(s0 >>  8) & 0xff] ^
            Te3[(s1      ) & 0xff] ^
            rk[6];
        t3 =
            Te0[(s3 >> 24)       ] ^
            Te1[(s0 >> 16) & 0xff] ^
            Te2[(s1 >>  8) & 0xff] ^
            Te3[(s2      ) & 0xff] ^
            rk[7];

        rk += 8;
        if (--r == 0) {
            break;
        }

        s0 =
            Te0[(t0 >> 24)       ] ^
            Te1[(t1 >> 16) & 0xff] ^
            Te2[(t2 >>  8) & 0xff] ^
            Te3[(t3      ) & 0xff] ^
            rk[0];
        s1 =
            Te0[(t1 >> 24)       ] ^
            Te1[(t2 >> 16) & 0xff] ^
            Te2[(t3 >>  8) & 0xff] ^
            Te3[(t0      ) & 0xff] ^
            rk[1];
        s2 =
            Te0[(t2 >> 24)       ] ^
            Te1[(t3 >> 16) & 0xff] ^
            Te2[(t0 >>  8) & 0xff] ^
            Te3[(t1      ) & 0xff] ^
            rk[2];
        s3 =
            Te0[(t3 >> 24)       ] ^
            Te1[(t0 >> 16) & 0xff] ^
            Te2[(t1 >>  8) & 0xff] ^
            Te3[(t2      ) & 0xff] ^
            rk[3];
    }

    s0 =
        (Te2[(t0 >> 24)       ] & 0xff000000) ^
        (Te3[(t1 >> 16) & 0xff] & 0x00ff0000) ^
        (Te0[(t2 >>  8) & 0xff] & 0x0000ff00) ^
        (Te1[(t3      ) & 0xff] & 0x000000ff) ^
        rk[0];
    PUTU32(out     , s0);
    s1 =
        (Te2[(t1 >> 24)       ] & 0xff000000) ^
        (Te3[(t2 >> 16) & 0xff] & 0x00ff0000) ^
        (Te0[(t3 >>  8) & 0xff] & 0x0000ff00) ^
        (Te1[(t0      ) & 0xff] & 0x000000ff) ^
        rk[1];
    PUTU32(out +  4, s1);
    s2 =
        (Te2[(t2 >> 24)       ] & 0xff000000) ^
        (Te3[(t3 >> 16) & 0xff] & 0x00ff0000) ^
        (Te0[(t0 >>  8) & 0xff] & 0x0000ff00) ^
        (Te1[(t1      ) & 0xff] & 0x000000ff) ^
        rk[2];
    PUTU32(out +  8, s2);
    s3 =
        (Te2[(t3 >> 24)       ] & 0xff000000) ^
        (Te3[(t0 >> 16) & 0xff] & 0x00ff0000) ^
        (Te0[(t1 >>  8) & 0xff] & 0x0000ff00) ^
        (Te1[(t2      ) & 0xff] & 0x000000ff) ^
        rk[3];
    PUTU32(out + 12, s3);
}

void aes_decrypt(const unsigned char *in, unsigned char *out,
                 const struct aes_key_st *key)
{E_
    const u32 *rk;
    u32 s0, s1, s2, s3, t0, t1, t2, t3;
    int r;

    assert(in && out && key);
    rk = (u32 *) key->rd_key;

    s0 = GETU32(in     ) ^ rk[0];
    s1 = GETU32(in +  4) ^ rk[1];
    s2 = GETU32(in +  8) ^ rk[2];
    s3 = GETU32(in + 12) ^ rk[3];

    r = key->rounds >> 1;
    for (;;) {
        t0 =
            Td0[(s0 >> 24)       ] ^
            Td1[(s3 >> 16) & 0xff] ^
            Td2[(s2 >>  8) & 0xff] ^
            Td3[(s1      ) & 0xff] ^
            rk[4];
        t1 =
            Td0[(s1 >> 24)       ] ^
            Td1[(s0 >> 16) & 0xff] ^
            Td2[(s3 >>  8) & 0xff] ^
            Td3[(s2      ) & 0xff] ^
            rk[5];
        t2 =
            Td0[(s2 >> 24)       ] ^
            Td1[(s1 >> 16) & 0xff] ^
            Td2[(s0 >>  8) & 0xff] ^
            Td3[(s3      ) & 0xff] ^
            rk[6];
        t3 =
            Td0[(s3 >> 24)       ] ^
            Td1[(s2 >> 16) & 0xff] ^
            Td2[(s1 >>  8) & 0xff] ^
            Td3[(s0      ) & 0xff] ^
            rk[7];

        rk += 8;
        if (--r == 0) {
            break;
        }

        s0 =
            Td0[(t0 >> 24)       ] ^
            Td1[(t3 >> 16) & 0xff] ^
            Td2[(t2 >>  8) & 0xff] ^
            Td3[(t1      ) & 0xff] ^
            rk[0];
        s1 =
            Td0[(t1 >> 24)       ] ^
            Td1[(t0 >> 16) & 0xff] ^
            Td2[(t3 >>  8) & 0xff] ^
            Td3[(t2      ) & 0xff] ^
            rk[1];
        s2 =
            Td0[(t2 >> 24)       ] ^
            Td1[(t1 >> 16) & 0xff] ^
            Td2[(t0 >>  8) & 0xff] ^
            Td3[(t3      ) & 0xff] ^
            rk[2];
        s3 =
            Td0[(t3 >> 24)       ] ^
            Td1[(t2 >> 16) & 0xff] ^
            Td2[(t1 >>  8) & 0xff] ^
            Td3[(t0      ) & 0xff] ^
            rk[3];
    }

    s0 =
        ((u32)Td4[(t0 >> 24)       ] << 24) ^
        ((u32)Td4[(t3 >> 16) & 0xff] << 16) ^
        ((u32)Td4[(t2 >>  8) & 0xff] <<  8) ^
        ((u32)Td4[(t1      ) & 0xff])       ^
        rk[0];
    PUTU32(out     , s0);
    s1 =
        ((u32)Td4[(t1 >> 24)       ] << 24) ^
        ((u32)Td4[(t0 >> 16) & 0xff] << 16) ^
        ((u32)Td4[(t3 >>  8) & 0xff] <<  8) ^
        ((u32)Td4[(t2      ) & 0xff])       ^
        rk[1];
    PUTU32(out +  4, s1);
    s2 =
        ((u32)Td4[(t2 >> 24)       ] << 24) ^
        ((u32)Td4[(t1 >> 16) & 0xff] << 16) ^
        ((u32)Td4[(t0 >>  8) & 0xff] <<  8) ^
        ((u32)Td4[(t3      ) & 0xff])       ^
        rk[2];
    PUTU32(out +  8, s2);
    s3 =
        ((u32)Td4[(t3 >> 24)       ] << 24) ^
        ((u32)Td4[(t2 >> 16) & 0xff] << 16) ^
        ((u32)Td4[(t1 >>  8) & 0xff] <<  8) ^
        ((u32)Td4[(t0      ) & 0xff])       ^
        rk[3];
    PUTU32(out + 12, s3);
}

void aes_cbc128_encrypt (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char ivec[16])
{E_
    unsigned char dummy[16];
    int auth = 0;
    int n;
    const unsigned char *iv = ivec;

    if (len == 0)
        return;

    if (!out)
        auth = 1;

    while (len) {
        if (auth)
            out = dummy;
        for (n = 0; n < 16 && n < len; ++n)
            out[n] = in[n] ^ iv[n];
        for (; n < 16; ++n)
            out[n] = iv[n];
        aes_encrypt (out, out, key);
        iv = out;
        if (len <= 16)
            break;
        len -= 16;
        in += 16;
        out += 16;
    }
    memcpy (ivec, iv, 16);
}

void aes_cbc128_decrypt (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char ivec[16])
{E_
    int n;
    union {
        long t[16 / sizeof (long)];
        unsigned char c[16];
    } tmp;

    if (len == 0)
        return;

    while (len) {
        unsigned char c;
        aes_decrypt (in, tmp.c, key);
        for (n = 0; n < 16 && n < len; ++n) {
            c = in[n];
            out[n] = tmp.c[n] ^ ivec[n];
            ivec[n] = c;
        }
        if (len <= 16) {
            memcpy (&ivec[n], in + n, 16 - n);  /* use memcpy instead of loop to avoid wierd warning on 32-bit gcc */
            break;
        }
        len -= 16;
        in += 16;
        out += 16;
    }
}


#if defined(__x86_64) || defined(__x86_64__)

/*
 * Copyright (c) 2012-2013 Vincent Hanquez <vincent@snarc.org>
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of his contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <wmmintrin.h>

struct aes_block {
    uint8_t b[16];
} __attribute__ ((aligned (32)));



static void cpuid (uint32_t info, uint32_t * eax, uint32_t * ebx, uint32_t * ecx, uint32_t * edx)
{E_
    *eax = info;
    asm volatile
     (
         "mov %%rbx, %%rdi;"
         "cpuid;" "mov %%ebx, %%esi;"
         "mov %%rdi, %%rbx;"
         :"+a" (*eax), "=S" (*ebx), "=c" (*ecx), "=d" (*edx)
         ::"edi");
}

int aes_has_aesni (void)
{E_
    uint32_t eax, ebx, ecx, edx;
    uint32_t aesni;
    cpuid (1, &eax, &ebx, &ecx, &edx);
    aesni = (ecx & 0x02000000);
    aesni = (aesni != 0);
    return aesni;
}

AES_CLANG_ATTR
void aes_ni_cbc_encrypt (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char _iv[16])
{E_
    int auth = 0;
    struct aes_block b;
    __m128i *k = (__m128i *) key->rd_key;
    __m128i iv;
    if (!out)
        auth = 1;
    memcpy (&b, _iv, 16);
    iv = _mm_loadu_si128 ((__m128i *) &b);
    __m128i K0 = _mm_loadu_si128 (k + 0);
    __m128i K1 = _mm_loadu_si128 (k + 1);
    __m128i K2 = _mm_loadu_si128 (k + 2);
    __m128i K3 = _mm_loadu_si128 (k + 3);
    __m128i K4 = _mm_loadu_si128 (k + 4);
    __m128i K5 = _mm_loadu_si128 (k + 5);
    __m128i K6 = _mm_loadu_si128 (k + 6);
    __m128i K7 = _mm_loadu_si128 (k + 7);
    __m128i K8 = _mm_loadu_si128 (k + 8);
    __m128i K9 = _mm_loadu_si128 (k + 9);
    __m128i K10 = _mm_loadu_si128 (k + 10);
    __m128i K11 = _mm_loadu_si128 (k + 11);
    __m128i K12 = _mm_loadu_si128 (k + 12);
    __m128i K13 = _mm_loadu_si128 (k + 13);
    __m128i K14 = _mm_loadu_si128 (k + 14);
    for (; len > 0; in += 16, out += 16, len -= 16) {
        memcpy (&b, in, 16);
        __m128i m = _mm_loadu_si128 ((__m128i *) &b);
        m = _mm_xor_si128 (m, iv);
        m = _mm_xor_si128 (m, K0);
        m = _mm_aesenc_si128 (m, K1);
        m = _mm_aesenc_si128 (m, K2);
        m = _mm_aesenc_si128 (m, K3);
        m = _mm_aesenc_si128 (m, K4);
        m = _mm_aesenc_si128 (m, K5);
        m = _mm_aesenc_si128 (m, K6);
        m = _mm_aesenc_si128 (m, K7);
        m = _mm_aesenc_si128 (m, K8);
        m = _mm_aesenc_si128 (m, K9);
        m = _mm_aesenc_si128 (m, K10);
        m = _mm_aesenc_si128 (m, K11);
        m = _mm_aesenc_si128 (m, K12);
        m = _mm_aesenc_si128 (m, K13);
        m = _mm_aesenclast_si128 (m, K14);
        iv = m;
        _mm_storeu_si128 ((__m128i *) &b, m);
        if (!auth)
            memcpy (out, &b, 16);
    }
    _mm_storeu_si128 ((__m128i *) &b, iv);
    memcpy (_iv, &b, 16);
}

AES_CLANG_ATTR
void aes_ni_cbc_decrypt (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char _iv[16])
{E_
    struct aes_block b;
    __m128i *k = (__m128i *) key->rd_key;
    __m128i iv;
    memcpy (&b, _iv, 16);
    iv = _mm_loadu_si128 ((__m128i *) &b);
    __m128i K0 = _mm_loadu_si128 (k + 14 + 0);
    __m128i K1 = _mm_loadu_si128 (k + 14 + 1);
    __m128i K2 = _mm_loadu_si128 (k + 14 + 2);
    __m128i K3 = _mm_loadu_si128 (k + 14 + 3);
    __m128i K4 = _mm_loadu_si128 (k + 14 + 4);
    __m128i K5 = _mm_loadu_si128 (k + 14 + 5);
    __m128i K6 = _mm_loadu_si128 (k + 14 + 6);
    __m128i K7 = _mm_loadu_si128 (k + 14 + 7);
    __m128i K8 = _mm_loadu_si128 (k + 14 + 8);
    __m128i K9 = _mm_loadu_si128 (k + 14 + 9);
    __m128i K10 = _mm_loadu_si128 (k + 14 + 10);
    __m128i K11 = _mm_loadu_si128 (k + 14 + 11);
    __m128i K12 = _mm_loadu_si128 (k + 14 + 12);
    __m128i K13 = _mm_loadu_si128 (k + 14 + 13);
    __m128i K14 = _mm_loadu_si128 (k + 0);
    for (; len > 0; in += 16, out += 16, len -= 16) {
        memcpy (&b, in, 16);
        __m128i m = _mm_loadu_si128 ((__m128i *) &b);
        __m128i ivnext = m;
        m = _mm_xor_si128 (m, K0);
        m = _mm_aesdec_si128 (m, K1);
        m = _mm_aesdec_si128 (m, K2);
        m = _mm_aesdec_si128 (m, K3);
        m = _mm_aesdec_si128 (m, K4);
        m = _mm_aesdec_si128 (m, K5);
        m = _mm_aesdec_si128 (m, K6);
        m = _mm_aesdec_si128 (m, K7);
        m = _mm_aesdec_si128 (m, K8);
        m = _mm_aesdec_si128 (m, K9);
        m = _mm_aesdec_si128 (m, K10);
        m = _mm_aesdec_si128 (m, K11);
        m = _mm_aesdec_si128 (m, K12);
        m = _mm_aesdec_si128 (m, K13);
        m = _mm_aesdeclast_si128 (m, K14);
        m = _mm_xor_si128 (m, iv);
        _mm_storeu_si128 ((__m128i *) &b, m);
        memcpy (out, &b, 16);
        iv = ivnext;
    }
    _mm_storeu_si128 ((__m128i *) &b, iv);
    memcpy (_iv, &b, 16);
}

static __m128i aes_128_key_expansion_ff (__m128i key, __m128i keygened)
{E_
    keygened = _mm_shuffle_epi32 (keygened, 0xff);
    key = _mm_xor_si128 (key, _mm_slli_si128 (key, 4));
    key = _mm_xor_si128 (key, _mm_slli_si128 (key, 4));
    key = _mm_xor_si128 (key, _mm_slli_si128 (key, 4));
    return _mm_xor_si128 (key, keygened);
}

static __m128i aes_128_key_expansion_aa (__m128i key, __m128i keygened)
{E_
    keygened = _mm_shuffle_epi32 (keygened, 0xaa);
    key = _mm_xor_si128 (key, _mm_slli_si128 (key, 4));
    key = _mm_xor_si128 (key, _mm_slli_si128 (key, 4));
    key = _mm_xor_si128 (key, _mm_slli_si128 (key, 4));
    return _mm_xor_si128 (key, keygened);
}

AES_CLANG_ATTR
int aes_ni_set_encrypt_key (const unsigned char *ikey, struct aes_key_st *key)
{E_
    __m128i k[28];
    uint64_t *out = (uint64_t *) key->rd_key;
    int i;

    if (!ikey || !key)
        return -1;

#define AES_256_key_exp_1(K1, K2, RCON) aes_128_key_expansion_ff(K1, _mm_aeskeygenassist_si128(K2, RCON))
#define AES_256_key_exp_2(K1, K2)       aes_128_key_expansion_aa(K1, _mm_aeskeygenassist_si128(K2, 0x00))
    k[0] = _mm_loadu_si128 ((const __m128i *) ikey);
    k[1] = _mm_loadu_si128 ((const __m128i *) (ikey + 16));
    k[2] = AES_256_key_exp_1 (k[0], k[1], 0x01);
    k[3] = AES_256_key_exp_2 (k[1], k[2]);
    k[4] = AES_256_key_exp_1 (k[2], k[3], 0x02);
    k[5] = AES_256_key_exp_2 (k[3], k[4]);
    k[6] = AES_256_key_exp_1 (k[4], k[5], 0x04);
    k[7] = AES_256_key_exp_2 (k[5], k[6]);
    k[8] = AES_256_key_exp_1 (k[6], k[7], 0x08);
    k[9] = AES_256_key_exp_2 (k[7], k[8]);
    k[10] = AES_256_key_exp_1 (k[8], k[9], 0x10);
    k[11] = AES_256_key_exp_2 (k[9], k[10]);
    k[12] = AES_256_key_exp_1 (k[10], k[11], 0x20);
    k[13] = AES_256_key_exp_2 (k[11], k[12]);
    k[14] = AES_256_key_exp_1 (k[12], k[13], 0x40);

    k[15] = _mm_aesimc_si128 (k[13]);
    k[16] = _mm_aesimc_si128 (k[12]);
    k[17] = _mm_aesimc_si128 (k[11]);
    k[18] = _mm_aesimc_si128 (k[10]);
    k[19] = _mm_aesimc_si128 (k[9]);
    k[20] = _mm_aesimc_si128 (k[8]);
    k[21] = _mm_aesimc_si128 (k[7]);
    k[22] = _mm_aesimc_si128 (k[6]);
    k[23] = _mm_aesimc_si128 (k[5]);
    k[24] = _mm_aesimc_si128 (k[4]);
    k[25] = _mm_aesimc_si128 (k[3]);
    k[26] = _mm_aesimc_si128 (k[2]);
    k[27] = _mm_aesimc_si128 (k[1]);
    for (i = 0; i < 28; i++)
        _mm_storeu_si128 (((__m128i *) out) + i, k[i]);

    return 0;
}

AES_CLANG_ATTR
int aes_ni_set_decrypt_key (const unsigned char *ikey, struct aes_key_st *key)
{E_
    return aes_ni_set_encrypt_key (ikey, key);
}

#else

void aes_ni_cbc_encrypt (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char _iv[16])
{E_
    (void) in;
    (void) out;
    (void) len;
    (void) key;
    (void) _iv;
}

void aes_ni_cbc_decrypt (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char _iv[16])
{E_
    (void) in;
    (void) out;
    (void) len;
    (void) key;
    (void) _iv;
}

int aes_ni_set_encrypt_key (const unsigned char *ikey, struct aes_key_st *key)
{E_
    (void) ikey;
    (void) key;
    return 1;
}

int aes_ni_set_decrypt_key (const unsigned char *ikey, struct aes_key_st *key)
{E_
    (void) ikey;
    (void) key;
    return -1;
}

int aes_has_aesni (void)
{E_
    return 0;
}

#endif

