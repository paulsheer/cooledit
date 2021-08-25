/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#include "inspect.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "aes.h"
#include "sha256.h"
#include "symauth.h"


struct symauth {
#define SYMAUTH_MAGIC           0x542c72f2
    unsigned int magic;
    struct aes_key_st aes_encrypt_key;
    struct aes_key_st aes_decrypt_key;
    struct aes_key_st aes_authsend_key;
    struct aes_key_st aes_authrecv_key;
    void (*aes_encrypt_fn) (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char ivec[16]);
    void (*aes_decrypt_fn) (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char ivec[16]);
    int with_aesni;
};

int symauth_with_aesni (struct symauth *s)
{E_
    return s->with_aesni;
}

void symauth_hex_dump (int f, const char *msg, const unsigned char *p, int l)
{E_
    int i, nl;
    printf ("%s ", msg);
    for (i = 0; i < f && l > 0; i++, l--)
        printf ("%02x ", (unsigned int) *p++);
    printf ("\n");
    nl = 1;
    for (i = 0; i < l; i++) {
        printf ("%02x ", (unsigned int) p[i]);
        nl = 0;
        if (!((i + 1) % 16))
            printf ("  ");
        if (!((i + 1) % 32)) {
            printf ("\n");
            nl = 1;
        }
    }
    if (!nl)
        printf ("\n");
    printf ("----------------\n");
}

struct symauth *symauth_new (int server, unsigned char *aeskey1, unsigned char *aeskey2)
{E_
    struct symauth *c;
    sha256_context_t sha256;
    unsigned char aeskey3[SYMAUTH_SHA256_SIZE];
    unsigned char aeskey4[SYMAUTH_SHA256_SIZE];
    unsigned char *k1;
    unsigned char *k2;
    unsigned char *k3;
    unsigned char *k4;

    assert (SYMAUTH_AES_KEY_BYTES == SYMAUTH_SHA256_SIZE);

    c = (struct symauth *) malloc (sizeof (struct symauth));
    memset (c, '\0', sizeof (*c));

    sha256_reset (&sha256);
    sha256_update (&sha256, aeskey1, SYMAUTH_SHA256_SIZE);
    sha256_finish (&sha256, aeskey3);

    sha256_reset (&sha256);
    sha256_update (&sha256, aeskey2, SYMAUTH_SHA256_SIZE);
    sha256_finish (&sha256, aeskey4);

    if (server) {
        k1 = aeskey1;
        k2 = aeskey2;
        k3 = aeskey3;
        k4 = aeskey4;
    } else {
        k1 = aeskey2;
        k2 = aeskey1;
        k3 = aeskey4;
        k4 = aeskey3;
    }

    if (aes_has_aesni ()) {
        c->with_aesni = 1;
        aes_ni_set_encrypt_key (k1, &c->aes_encrypt_key);
        aes_ni_set_decrypt_key (k2, &c->aes_decrypt_key);
        aes_ni_set_encrypt_key (k3, &c->aes_authsend_key);
        aes_ni_set_encrypt_key (k4, &c->aes_authrecv_key);
        c->aes_encrypt_fn = aes_ni_cbc_encrypt;
        c->aes_decrypt_fn = aes_ni_cbc_decrypt;
    } else {
        aes_set_encrypt_key (k1, SYMAUTH_AES_KEY_BYTES * 8, &c->aes_encrypt_key);
        aes_set_decrypt_key (k2, SYMAUTH_AES_KEY_BYTES * 8, &c->aes_decrypt_key);
        aes_set_encrypt_key (k3, SYMAUTH_AES_KEY_BYTES * 8, &c->aes_authsend_key);
        aes_set_encrypt_key (k4, SYMAUTH_AES_KEY_BYTES * 8, &c->aes_authrecv_key);
        c->aes_encrypt_fn = aes_cbc128_encrypt;
        c->aes_decrypt_fn = aes_cbc128_decrypt;
    }

    c->magic = SYMAUTH_MAGIC;

    return c;
}

void symauth_free (struct symauth *s)
{E_
    assert (s->magic == SYMAUTH_MAGIC);
    memset (s, '\0', sizeof (*s));
    free (s);
}

static void gen_auth (struct symauth *symauth, struct aes_key_st *aes, const unsigned char *in1, const unsigned char *in2, const unsigned char *in3, unsigned char *out)
{E_
    sha256_context_t sha256;
    unsigned char synth[SYMAUTH_BLOCK_SIZE];
    unsigned char iv[SYMAUTH_BLOCK_SIZE];

    memset (iv, '\0', SYMAUTH_BLOCK_SIZE);
    (*symauth->aes_encrypt_fn) (in1, synth, SYMAUTH_BLOCK_SIZE, aes, iv);

    sha256_reset (&sha256);
    sha256_update (&sha256, in1, SYMAUTH_BLOCK_SIZE);
    sha256_update (&sha256, in2, SYMAUTH_BLOCK_SIZE);
    sha256_update (&sha256, in3, SYMAUTH_BLOCK_SIZE);
    sha256_update (&sha256, synth, SYMAUTH_BLOCK_SIZE);
    sha256_finish (&sha256, out);

    assert (SYMAUTH_BLOCK_SIZE * 2 == SYMAUTH_SHA256_SIZE);
}

void symauth_encrypt (struct symauth *symauth, const unsigned char *in, int inlen, unsigned char *out, const unsigned char *_iv, unsigned char *_auth)
{E_
    unsigned char auth[SYMAUTH_BLOCK_SIZE * 2];
    unsigned char iv[SYMAUTH_BLOCK_SIZE];

    assert (symauth->magic == SYMAUTH_MAGIC);

    memcpy (iv, _iv, SYMAUTH_BLOCK_SIZE);

    (*symauth->aes_encrypt_fn) (in, out, inlen, &symauth->aes_encrypt_key, iv);

    gen_auth (symauth, &symauth->aes_authsend_key, _iv, (unsigned char *) out, iv, auth);
    (*symauth->aes_encrypt_fn) ((unsigned char *) out, NULL, inlen, &symauth->aes_authsend_key, auth);
    (*symauth->aes_encrypt_fn) (auth + SYMAUTH_BLOCK_SIZE, NULL, SYMAUTH_BLOCK_SIZE, &symauth->aes_authsend_key, auth);

    memcpy (_auth, auth, SYMAUTH_BLOCK_SIZE);
}

int symauth_decrypt (struct symauth *symauth, const unsigned char *in, int inlen, unsigned char *out_, int outlen_, const unsigned char *auth_, const unsigned char *_iv)
{E_
    unsigned char iv[SYMAUTH_BLOCK_SIZE];
    unsigned char first_block[SYMAUTH_BLOCK_SIZE];
    unsigned char auth[SYMAUTH_BLOCK_SIZE * 2];
    int outlen;

    outlen = inlen - SYMAUTH_BLOCK_SIZE * 2;
    assert (outlen == outlen_);
    memcpy (iv, _iv, SYMAUTH_BLOCK_SIZE);
    memcpy (first_block, in, SYMAUTH_BLOCK_SIZE);

    assert (symauth->magic == SYMAUTH_MAGIC);

    gen_auth (symauth, &symauth->aes_authrecv_key, iv, in, in + inlen - SYMAUTH_BLOCK_SIZE, auth);
    (*symauth->aes_encrypt_fn) (in, NULL, inlen, &symauth->aes_authrecv_key, auth);
    (*symauth->aes_encrypt_fn) (auth + SYMAUTH_BLOCK_SIZE, NULL, SYMAUTH_BLOCK_SIZE, &symauth->aes_authrecv_key, auth);
    (*symauth->aes_decrypt_fn) (in, out_, inlen, &symauth->aes_decrypt_key, iv);

    return memcmp (auth, auth_, SYMAUTH_BLOCK_SIZE) != 0;
}

