/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* drawings.h - for doing modifiable line drawings in a window
   Copyright (C) 1996-2022 Paul Sheer
 */


#ifndef DRAWINGS_H
#define DRAWINGS_H

/* types */
#define CLINE 1
#define CELLIPSE 2
#define CCIRCLE 3
#define CARC 4
#define CRECTANGLE 5
#define CFILLED_ELLIPSE 6
#define CFILLED_CIRCLE 7
#define CFILLED_ARC 8
#define CFILLED_RECTANGLE 9

typedef struct {
    float x, y;			/*point position (must be floats for scaling) */
    float a, b;			/*width and height for arcs, ellipses
				   and rectangle. Second point for lines */
    char type;			/*type */
    short angle1, angle2;
    unsigned long color;
} CPicturePrimative;

typedef struct {
    int numelements;
    float x1, y1;		/*maximum bounds of elements */
    float x2, y2;
    CPicturePrimative *pp;
} CPicture;


/* sets the widget into which subsequent drawing operations execute.
   returns 1 on error */
int CSetDrawingTarget (const char *picture_ident);

/* returns a pp index to be used for removepp */

int CDrawLine (float x1, float y1, float x2, float y2, unsigned long c);

int CDrawRectangle (float x, float y, float w, float h, unsigned long c);

void CRemovePictureElement (int j);

int CDrawPoint (int x1, int y1, unsigned long c);

int CDrawArc (int x, int y, int width, int height, int angle1, int angle2, unsigned long c);

int CDrawCurvedLine (int x1, int y1, int x2, int y2, int radius, unsigned long c);

int CDrawFilledArc (int x, int y, int width, int height,
		    int angle1, int angle2, int pie, unsigned long c);

void CScalePicture (float s);

void CClearPicture (void);

#endif
