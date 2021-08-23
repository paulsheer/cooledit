/* quickmath.c - functions for vector mathematics
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <math.h>
#include "quickmath.h"

#if 0

inline double fsqr (double x)
{
    return x * x;
}

inline long lsqr (long x)
{
    return (long) x * x;
}

inline double fmax (double a, double b)
{
    return max(a, b);
}

inline double fmin (double a, double b)
{
    return min(a, b);
}

inline double fsgn (double a)
{
	return (a == 0.0 ? 0.0 : (a > 0.0 ? 1.0 : -1.0));
}

inline double dot (Vec a, Vec b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec cross (Vec a, Vec b)
{
    Vec c;
    c.x = a.y * b.z - a.z * b.y;
    c.y = a.z * b.x - a.x * b.z;
    c.z = a.x * b.y - a.y * b.x;
    return c;
}

Vec plus (Vec a, Vec b)
{
    Vec c;
    c.x = a.x + b.x;
    c.y = a.y + b.y;
    c.z = a.z + b.z;
    return c;
}

Vec minus (Vec a, Vec b)
{
    Vec c;
    c.x = a.x - b.x;
    c.y = a.y - b.y;
    c.z = a.z - b.z;
    return c;
}

Vec times (Vec a, double f)
{
    Vec c;
    c.x = a.x * f;
    c.y = a.y * f;
    c.z = a.z * f;
    return c;
}

double norm (Vec a)
{
    return sqrt (sqr(a.x) + sqr(a.y) + sqr(a.z));
}

#endif

void orth_vectors(Vec X, Vec *r1, Vec *r2, double r)
{
    if (X.x == 0 && X.y == 0) {
	r1->x = 1;
	r1->y = 0;
	r1->z = 0;
    } else {
	r1->x = X.y / sqrt (X.x * X.x + X.y * X.y);
	r1->y = -X.x / sqrt (X.x * X.x + X.y * X.y);
	r1->z = 0;
    }
    *r1 = times (*r1, r);		/* r1 now has length r */

    *r2 = cross (X, *r1);
    *r2 = times (*r2, r / norm (*r2));	/* r2 now has length r */

/* r1 and r2 are now two vectors prependicular to each other and to (x,y,z) */
}

