/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#ifndef LOAD_TIFF_H
#define LOAD_TIFF_H

unsigned char *loadgreytiff (const char *fname, long *width, long *height,
	long rowstart, long rowend, float gamma);

#endif
