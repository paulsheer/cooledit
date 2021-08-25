/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#ifndef __CW_GLOBAL_H
#define __CW_GLOBAL_H

#if defined(_WIN32) || defined(_WIN64)
#define MSWIN
#define _FILE_OFFSET_BITS       64
#endif

/* Some X servers use a different mask for the Alt key: */
#define MyAltMask alt_modifier_mask
/* #define MyAltMask Mod1Mask */
/* #define MyAltMask Mod2Mask */
/* #define MyAltMask Mod3Mask */
/* #define MyAltMask Mod4Mask */
/* #define MyAltMask Mod5Mask */

/* u_32bit_t should be four bytes */
#define u_32bit_t unsigned int

#endif				/* __CW_GLOBAL_H */

