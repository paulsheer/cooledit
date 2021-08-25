/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */


struct symauth;

#define SYMAUTH_BLOCK_SIZE              16
#define SYMAUTH_SHA256_SIZE             32
#define SYMAUTH_AES_KEY_BYTES           SYMAUTH_SHA256_SIZE

struct symauth *symauth_new (int server, unsigned char *aeskey1, unsigned char *aeskey2);
void symauth_free (struct symauth *s);
void symauth_encrypt (struct symauth *symauth, const unsigned char *in, int inlen, unsigned char *out, const unsigned char *_iv, unsigned char *_auth);
int symauth_decrypt (struct symauth *symauth, const unsigned char *in, int inlen, unsigned char *out_, int outlen_, const unsigned char *auth_, const unsigned char *_iv);
int symauth_with_aesni (struct symauth *s);


void symauth_hex_dump (int f, const char *msg, const unsigned char *p, int l);

