/* mathfuncs.c: standalone log(), sqrt(), pow() functions not requiring libm
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <stdlib.h>
#include <stdio.h>

/* this file is to avoid having to link with the whole libm when all
   we need is a pow() function. But its more an interesting exercise. */

/* to force use of softcoded pow even on linux-x86's: */
/* #define USE_OWN_POW */

#define floating_p_error(x) float_error(__FILE__, __LINE__)

static void float_error (char *file, int line)
{
    fprintf (stderr, "%s:%d: floating point error\n", file, line);
    abort ();
}

#if !defined(__i386__) || !defined(__GNUC__) || !defined(__linux__) || defined(USE_OWN_POW)

/*
   Libc pow() using an FPU is timed to be
   23 times faster than this soft-coded
   pow function. In the test below.
   486DX4-100
   gcc-2.7.2 -O3 -fomit-frame-pointer
   libc-5.4.17 -O2
*/

/*
   Principles:

   x^y :

   taylor(x^y) =
      ... + y^5*z^5/120 + y^4*z^4/24 + y^3*z^3/6 + y^2*z^2/2 + y*z + 1,
         where z = log_e(x)

   log_e(q + x) :

   taylor(log_e(q + x)) =
      log_e(q) + ... + x^5/(5*q^5)-x^4/(4*q^4) + x^3/(3*q^3)-x^2/(2*q^2) + x/q
 */

/* results are this accurate */
#define F_ACCURACY 1e-15

/* e^4, e, e^0.25 */
#define VITAMIN_E4   54.598150033144239078
#define VITAMIN_E    2.7182818284590452353
#define VITAMIN_E25  1.2840254166877414840

#define my_fabs(t) ((t) >= 0 ? (t) : -(t))

double my_log (double x)
{
    int i = 1, j;
    double t, q = 1, ans = 0;

    if (x <= 0)
	floating_p_error(0);

/* find an initial approximation */
    if (x > 1.0) {
	do {
	    ans += 4;
	} while ((q *= VITAMIN_E4) < x);
	do {
	    ans -= 1;
	} while ((q /= VITAMIN_E) > x);
	while ((q *= VITAMIN_E25) < x)
	    ans += 0.25;
	q /= VITAMIN_E25;
    } else if (x < 1.0) {
	do {
	    ans -= 4;
	} while ((q /= VITAMIN_E4) > x);
	do {
	    ans += 1;
	} while ((q *= VITAMIN_E) < x);
	do {
	    ans -= 0.25;
	} while ((q /= VITAMIN_E25) > x);
    } else
	return 0.0;

    x -= q;

/* taylor series */
    do {
	t = 1;
	for (j = 0; j < i; j++)
	    t *= -x / q;
	t /= (double) i++;
	ans -= t;
	if (i > 200)	/* shouldn't happen */
	    floating_p_error(0);
    } while (my_fabs (t * ans) > F_ACCURACY);

    return ans;
}


double my_sqrt (double x)
{
    double last_ans, ans = 2;

    if (x < 0.0)
	floating_p_error(0);
    if (x == 0.0)
	return 0.0;

    do {
	last_ans = ans;
	ans = (ans + x / ans) / 2;
    } while (my_fabs ((ans - last_ans) / ans) > F_ACCURACY);

    return ans;
}


double my_pow (double x, double y)
{
    double z, ans = 1, ans2 = 1, t;
    long i, j, inv = 0;
    unsigned long max, negative = 0;

    if (y == 0.0)
	return 1.0;
    if (x == 0.0) {
	if (y < 0.0) {
	    floating_p_error(0);
	} else
	    return 0.0;
    }
    if (y == 1.0)
	return x;
    if (y < 0.0) {
	y = -y;
	inv = 1;
    }
    z = my_log (x);

    max = (unsigned long) -1;
    if ((double) y > (double) max / 4) {
	if (inv)
	    return 0.0;
	else
	    floating_p_error(0);
    }

    if (x < 0.0) {
	negative = ((long) y);
	if (y != (double) negative)
	    floating_p_error(0);
	negative &= 1;
	x = -x;
    }

    y *= 2;
    j = y;
    y -= (double) j;
    y /= 2;

    ans2 = x;

/* calc to the nearest 0.5 of a power, */
    if (j % 2)
	ans = my_sqrt (x);

/* multiply it up in squares */
    while ((j >>= 1)) {
	if (j & 1)
	    ans *= ans2;
	ans2 *= ans2;
    }

    j = 1;
    ans2 = 1;
/* taylor series for the remaining */
    do {
	t = 1;
	for (i = 1; i <= j; i++)
	    t *= y * z / i;
	ans2 += t;
	j++;
	if (j > 200)
	    floating_p_error(0);	/* shouldn't happen */
    } while (my_fabs (t / (ans * ans2)) > F_ACCURACY);

    if (negative)
	ans = -ans;

    if (inv)
	return 1.0 / (ans * ans2);
    else
	return ans * ans2;
}


#else				/* defined(__i386__) && defined(__GNUC__) */

/* The following routines are from libc-5.4.17
   written by Hongjiu Lu, but are changed a lot here */


double my_log (double x)
{
    if (x <= 0.0)
	floating_p_error(0);

    __asm__ __volatile__ ("fldln2\n\t"
			  "fxch %%st(1)\n\t"
			  "fyl2x"
			  :"=t" (x):"0" (x));
    return x;
}

double my_sqrt (double x)
{
    const double zero = 0.0;

    if (x >= zero) {
	if (x != zero) {
	    __asm__ __volatile__ ("fsqrt"
				  :"=t" (x):"0" (x));
	}
	return x;
    }
    floating_p_error(0);
    return 0;
}

double my_pow (double x, double y)
{
    int negative;
    __volatile__ unsigned short cw, saved_cw;

    if (y == 0.0)
	return 1.0;
    if (x == 0.0) {
	if (y < 0.0)
	    floating_p_error(0);
	else
	    return 0.0;
    }
    if (y == 1.0)
	return x;

    if (x < 0.0) {
	long tmp;	/* should be long long */
	tmp = (long) y;	/* should be (long long) */
	negative = tmp & 1;
	if (y != (double) tmp)
	    floating_p_error(0);
	x = -x;
    } else {
	negative = 0;
    }

    __asm__ __volatile__ ("fnstcw %0":"=m" (cw):);
    saved_cw = cw;

    cw &= 0xf3ff;
    cw |= 0x003f;

    __asm__ __volatile__ ("fldcw %0"::"m" (cw));

    __asm__ __volatile__ ("fyl2x;fstl %2;frndint;fstl %%st(2);fsubrp;f2xm1;"
			  "fld1;faddp;fscale"
			  :"=t" (x):"0" (x), "u" (y));

    __asm__ __volatile__ ("fldcw %0"::"m" (saved_cw));

    return (negative) ? -x : x;
}

#endif				/* defined(__i386__) && defined(__GNUC__) */

/* #define TEST_MY_POW */

#ifdef TEST_MY_POW

void main (void)
{
    int i = 0;
    double p, q, h, g;
    for (q = -10; q < 10; q += 0.23) {
	for (p = 0.1; p < 10; p += 0.23) {
	    i++;
	    g = pow (p, q);
	    h = my_pow (p, q);
	    if (g != 0) {
		if (my_fabs (h - g) / g > 1e-13)
		    abort ();
	    } else {
		if (h != 0)
		    abort ();
	    }
	}
    }
    printf ("Done %d.\n", i);
}

#endif				/* TEST_MY_POW */

