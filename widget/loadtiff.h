#ifndef LOAD_TIFF_H
#define LOAD_TIFF_H

unsigned char *loadgreytiff (const char *fname, long *width, long *height,
	long rowstart, long rowend, float gamma);

#endif
