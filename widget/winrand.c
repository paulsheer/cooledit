/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* winrand.c - remote fs access
   Copyright (C) 1996-2022 Paul Sheer
 */


#include <winsock2.h>
#include <ws2ipdef.h>
#include <wincrypt.h>

int windows_get_random (unsigned char *out, const int outlen)
{
    HCRYPTPROV h = 0;
    if (!CryptAcquireContextW (&h, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
        return 1;
    if (!CryptGenRandom (h, outlen, out)) {
        CryptReleaseContext (h, 0);
        return 1;
    }
    if (!CryptReleaseContext (h, 0))
        return 1;
    return 0;
}
