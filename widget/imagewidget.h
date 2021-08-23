#ifndef IMAGE_WIDGET_H
#define IMAGE_WIDGET_H

#include "3dkit.h"

void grey_scale_to_pixels (void *pixdata, unsigned char *data, int width, int height, int bytedepth);

unsigned char *load_targa_to_grey (const char *fname, long *width, long *height, long rowstart, long rowend);

int write_targa (unsigned char *pic8, const char *fname, long w, long h, int grey);

void color_8bit_to_pixels (void *pixdata, unsigned char *data, int width, int height, int bytedepth);

#endif

