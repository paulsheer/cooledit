/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */

#define AES_MAXNR               14
#define AES_BLOCK_SIZE          16


#ifdef MSWIN
#define uint64_t  UINT64
#endif


struct aes_key_st {
    uint64_t rd_key[4 * (AES_MAXNR + 1)];
    int rounds;
} __attribute__ ((aligned (32)));

int aes_has_aesni (void);

void aes_encrypt(const unsigned char *in, unsigned char *out, const struct aes_key_st *key);
void aes_decrypt(const unsigned char *in, unsigned char *out, const struct aes_key_st *key);
int aes_set_encrypt_key (const unsigned char *userKey, const int bits, struct aes_key_st *key);
int aes_set_decrypt_key (const unsigned char *userKey, const int bits, struct aes_key_st *key);
void aes_cbc128_encrypt (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char ivec[16]);
void aes_cbc128_decrypt (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char ivec[16]);

void aes_ni_cbc_encrypt (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char _iv[16]);
void aes_ni_cbc_decrypt (const unsigned char *in, unsigned char *out, int len, const struct aes_key_st *key, unsigned char _iv[16]);
int aes_ni_set_decrypt_key (const unsigned char *ikey, struct aes_key_st *key);
int aes_ni_set_encrypt_key (const unsigned char *ikey, struct aes_key_st *key);


