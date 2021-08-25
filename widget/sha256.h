/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#ifndef SHA256_h
#define SHA256_h

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t h[8];
    uint8_t  m[64];
    uint64_t length;
    uint8_t  posn;

} sha256_context_t;

void sha256_reset(sha256_context_t *context);
void sha256_update(sha256_context_t *context, const void *data, size_t size);
void sha256_finish(sha256_context_t *context, uint8_t *hash);

#ifdef __cplusplus
};
#endif

#endif
