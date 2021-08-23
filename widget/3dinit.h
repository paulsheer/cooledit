/* 3dinit.h
   Copyright (C) 1996-2018 Paul Sheer
 */

/*

File: 3dinit.h

*/

#ifndef SVGALIB
#include "quickmath.h"
#endif

void TD_initcolor (TD_Surface * surf, int n);

void TD_initellipsoidpart (TD_Surface * surf, long x, long y, long z,
		      long a, long b, long c, int w, int dir, int col);
void TD_initellipsoid (TD_Surface * surf1, TD_Surface * surf2, TD_Surface * surf3,
	TD_Surface * surf4, TD_Surface * surf5, TD_Surface * surf6, long x, 
	long y, long z, long a, long b, long c, int w, int col);
void TD_initsellipsoid (TD_Solid *s, int n, long x, 
	long y, long z, long a, long b, long c, int w, int col);


