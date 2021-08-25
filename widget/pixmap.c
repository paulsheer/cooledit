/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* pixmap.c - creates XImages and pixmaps from XPM formats
   Copyright (C) 1996-2022 Paul Sheer
 */


#include "inspect.h"
#include "coolwidget.h"

extern struct look *look;

XImage *CCreateImage (const char *data[], int width, int height, char start_char)
{E_
    XImage *image;
    int bpp, x, y, u;

    if (CDepth <= 8) {
	bpp = 1;
    } else if (CDepth <= 16) {
	bpp = 2;
    } else {
	bpp = 4;
    }

    if (width & 1)
	u = 8;
    else if (width & 2)
	u = 16;
    else
	u = 32;
    
    image = XCreateImage (CDisplay, CVisual, CDepth, ZPixmap,
    		0, CMalloc (width * height * bpp), width, height, u, 0);
    if (!image)
	return 0;

    for (y = 0; y < height; y++)
	for (x = 0; x < width; x++) {
	    if (data[y][x] == ' ')
		XPutPixel (image, x, y, COLOR_FLAT);
	    else
		XPutPixel (image, x, y, color_pixels[data[y][x] - start_char]);
	}
    return image;
}

XImage *CCreateMaskImage (const char *data[], int width, int height, char backing_char)
{E_
    XImage *image;
    void *p;
    int x, y;
    p = CMalloc (width * height);
    memset(p, '\0', width * height);
    image = XCreateImage (CDisplay, CVisual, 1, ZPixmap,
			  0, p, width, height, 32, 0);
    if (!image)
	return 0;
    for (y = 0; y < height; y++)
	for (x = 0; x < width; x++) {
	    if (data[y][x] == backing_char)
		XPutPixel (image, x, y, 1);
	    else
		XPutPixel (image, x, y, 0);
	}
    return image;
}

Pixmap CCreateClipMask (const char *data[], int width, int height, char backing_char)
{E_
    Pixmap pixmap;
    XImage *image;
    XGCValues gcv;
    GC gc;
    memset(&gcv, '\0', sizeof(gcv));
    memset(&gc, '\0', sizeof(gc));
    image = CCreateMaskImage (data, width, height, backing_char);
    if (!image)
	return 0;
    pixmap = XCreatePixmap (CDisplay, CRoot, width, height, 1);
    gc = XCreateGC (CDisplay, pixmap, 0, &gcv);
    XPutImage (CDisplay, pixmap, gc, image, 0, 0, 0, 0, width, height);
    XFreeGC (CDisplay, gc);
    free (image->data);
    image->data = 0;
    XDestroyImage (image);
    return pixmap;
}

Pixmap CCreatePixmap (const char *data[], int width, int height, char start_char)
{E_
    Pixmap pixmap;
    XImage *image;
    image = CCreateImage (data, width, height, start_char);
    if (!image)
	return 0;
    pixmap = XCreatePixmap (CDisplay, CRoot, width, height, CDepth);
    XPutImage (CDisplay, pixmap, CGC, image, 0, 0, 0, 0, width, height);
    free (image->data);
    image->data = 0;
    XDestroyImage (image);
    return pixmap;
}

