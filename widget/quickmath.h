/* quickmath.h - functions for vector mathematics
   Copyright (C) 1996-2018 Paul Sheer
 */


#ifndef QUICK_MATH_H
#define QUICK_MATH_H

#include <config.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

typedef struct {
    double x, y, z;
} Vec;


#define sqr(x) ((x)*(x))

#define fswap(a, b) \
{ \
    double __t_var = (a); \
    (a) = (b); \
    (b) = __t_var; \
}


#define swap(a, b) \
{ \
    long __t_var = (a); \
    (a) = (b); \
    (b) = __t_var; \
}

#define max(x,y)     (((x) > (y)) ? (x) : (y))
#define min(x,y)     (((x) < (y)) ? (x) : (y))

/* #ifndef __GNUC__ */
#if 0

double fsqr (double x);
long lsqr (long x);
double fmax (double a, double b);
double fmin (double a, double b);
double fsgn (double a);
double dot (Vec a, Vec b);
Vec cross (Vec a, Vec b);
Vec plus (Vec a, Vec b);
Vec minus (Vec a, Vec b);
Vec times (Vec a, double f);
double norm (Vec a);

#else

static inline double fsqr (double x)
{
    return x * x;
}

static inline long lsqr (long x)
{
    return (long) x *x;
}

static inline double fmax (double a, double b)
{
    return max (a, b);
}

static inline double fmin (double a, double b)
{
    return min (a, b);
}

static inline double fsgn (double a)
{
    return (a == 0.0 ? 0.0 : (a > 0.0 ? 1.0 : -1.0));
}

static inline double dot (Vec a, Vec b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec cross (Vec a, Vec b)
{
    Vec c;
    c.x = a.y * b.z - a.z * b.y;
    c.y = a.z * b.x - a.x * b.z;
    c.z = a.x * b.y - a.y * b.x;
    return c;
}

static inline Vec plus (Vec a, Vec b)
{
    Vec c;
    c.x = a.x + b.x;
    c.y = a.y + b.y;
    c.z = a.z + b.z;
    return c;
}

static inline Vec minus (Vec a, Vec b)
{
    Vec c;
    c.x = a.x - b.x;
    c.y = a.y - b.y;
    c.z = a.z - b.z;
    return c;
}

static inline Vec times (Vec a, double f)
{
    Vec c;
    c.x = a.x * f;
    c.y = a.y * f;
    c.z = a.z * f;
    return c;
}

static inline double norm (Vec a)
{
    return sqrt (sqr (a.x) + sqr (a.y) + sqr (a.z));
}

#endif

void orth_vectors (Vec X, Vec * r1, Vec * r2, double r);

#endif

