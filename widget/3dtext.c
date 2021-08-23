/* 3dtext.c - converts generic 3D commands to 3D data
   Copyright (C) 1996-2018 Paul Sheer
 */



/* 3dtext */

#include <config.h>
#include <stdlib.h>
#include <math.h>
#include <my_string.h>
#include <stdio.h>
#include <stdarg.h>

/* #include <X11/Intrinsic.h> */
#include <X11/Xresource.h>

#include "app_glob.c"
#include "coolwidget.h"
#include "widget3d.h"
#include "quickmath.h"

/*

   this processes a text file into a 3d world
   the text file contains the following commands separate
   by zero or more newlines. Charaacters after a # at the
   beginning of a line are ignored.


   # x, y, z, a, b, h, w, and c are floats. (x,y,z) is a vector.

   scale a
   # specifies the absolute value of the maximum extent of the scene in 3D space
   # (after offset has been subtracted (see next)). This must come first.

   offset x y z
   # specifies the a vector that is to be subtracted from the given position of
   # forthcoming object. Must also come before any drawing commands.

   cylinder x y z a b c r
   # draws a cylinder beginning at (a,b,c) ending at (a,b,c)+(x,y,z) of radius r

   cappedcylinder x y z a b c r
   # draws a cylinder beginning at (a,b,c) ending at (a,b,c)+(x,y,z) of radius r
   # with closed ends.

   surface a b x y z x y z x y z ... x y z
   # draws a surface of grid size a by b there must be a*b (x, y, z) points.

   trapezium x y z a b c u v w p q r
   # draws a trapezium with one corner at (x,y,z) and the other three at (x,y,z)+(a,b,c) etc.

   pipe r a x y z x y z x y z x y z ... x y z
   # draw a pipe with corners at (x,y,z) the pipe diameter is r and the corner radii are a
   * the first (x,y,z) is the start the last is the finish. Points mus be more than 2a appart

   cappedpipe  r a x y z x y z x y z x y z ... x y z
   # same with closed ends

   rectangle a b c x y z
   # rectangle with (height,width,depth) = (x,y,z), corner at (a,b,c)

   ellipse a b c x y z
   # an ellipse with (height,width,depth) = (x,y,z), centre at (a,b,c)

   density a
   # will set the density of the grid making up any of the specific surfaces above.
   # can be called before each surface command.

 */

/* globals: */

int GridDensity = 6;
double DimensionScale = 1;
Vec DimensionOffset =
{0, 0, 0};

static inline void assignTD (TD_Point * p, Vec v)
{
    p->x = (double) (v.x + DimensionOffset.x) * DimensionScale;
    p->y = (double) (v.y + DimensionOffset.y) * DimensionScale;
    p->z = (double) (v.z + DimensionOffset.z) * DimensionScale;
}




static void third_cyl (double t, TD_Point * p, Vec A, Vec X, Vec r1, Vec r2, int g, double f)
{
    int i = 0;
    double h;
    double alpha = t;
    Vec rv;
    while (alpha < (2 * PI / 3 + t + 0.001)) {
	for (h = 0; h <= 1; h += 0.5) {
	    rv = plus (plus (plus (times (r1, cos (alpha) * (1 + h * (f - 1))), times (r2, sin (alpha) * (1 + h * (f - 1)))), A), times (X, h));
	    assignTD (&(p[i]), rv);
	    i++;
	}
	alpha += (2 * PI / 3) / g;
    }
}



void CDraw3DCone (const char *ident, double x, double y, double z, double a, double b, double c, double ra, double rb)
{
    int g = 4 * GridDensity / 3;
    TD_Point *p = CMalloc ((g + 1) * 3 * sizeof (TD_Point));
    Vec r1;
    Vec r2;
    Vec A, X;
    double f = rb / ra;
    A.x = a;
    A.y = b;
    A.z = c;
    X.x = x;
    X.y = y;
    X.z = z;

    orth_vectors (X, &r1, &r2, ra);

    third_cyl (0, p, A, X, r1, r2, g, f);
    CInitSurfacePoints (ident, 3, g + 1, p);
    third_cyl (2 * PI / 3, p, A, X, r1, r2, g, f);
    CInitSurfacePoints (ident, 3, g + 1, p);
    third_cyl (4 * PI / 3, p, A, X, r1, r2, g, f);
    CInitSurfacePoints (ident, 3, g + 1, p);

    free (p);
}




void CDraw3DCylinder (const char *ident, double x, double y, double z, double a, double b, double c, double r)
{
    CDraw3DCone (ident, x, y, z, a, b, c, r, r);
}



void CDraw3DRoundPlate (const char *ident, double x, double y, double z, double a, double b, double c, double r)
{
    TD_Point *p = CMalloc ((GridDensity * 4 + 1) * 2 * sizeof (TD_Point));
    double alpha = 0;
    Vec r1;
    Vec r2;
    Vec rv;
    Vec A;
    Vec X;
    int i = 0;
    A.x = a;
    A.y = b;
    A.z = c;
    X.x = x;
    X.y = y;
    X.z = z;

    orth_vectors (X, &r1, &r2, r);

    while (alpha < (2 * PI + 0.001)) {
	rv = plus (plus (times (r1, cos (alpha)), times (r2, sin (alpha))), A);
	assignTD (&p[i], rv);
	i++;
	assignTD (&p[i], A);
	i++;
	alpha += (2 * PI) / (GridDensity * 4);
    }
    CInitSurfacePoints (ident, 2, GridDensity * 4 + 1, p);
    free (p);
}


void CDraw3DCappedCylinder (const char *ident, double x, double y, double z, double a, double b, double c, double r)
{
    CDraw3DCylinder (ident, x, y, z, a, b, c, r);
    CDraw3DRoundPlate (ident, -x, -y, -z, a, b, c, r);
    CDraw3DRoundPlate (ident, x, y, z, x + a, y + b, z + c, r);
}


void textformaterror (int line, const char *ident)
{
/* "Compile our special discription language into an actual 3D world" */
    CErrorDialog (0, 0, 0, _ (" Compile text to 3D "),
		  _ (" A text format error was encounted at line %d, \n while trying to draw 3d item to widget %s "), line, ident);
}


void CDraw3DScale (const char *ident, double a)
{
    DimensionScale = 32767 / a;
}

void CDraw3DOffset (const char *ident, double x, double y, double z)
{
    DimensionOffset.x = x;
    DimensionOffset.y = y;
    DimensionOffset.z = z;
}

void CDraw3DDensity (const char *ident, double a)
{
    GridDensity = a;
}

void draw3d_surface (const char *ident, int w, int h, Vec * v)
{
    int i;
    TD_Point *p = CMalloc ((w + 1) * (h + 1) * sizeof (TD_Point));
    for (i = 0; i < w * h; i++)
	assignTD (&p[i], v[i]);
    CInitSurfacePoints (ident, w, h, p);
    free (p);
}


void CDraw3DSurface (const char *ident, int w, int h,...)
{
    va_list pa;
    int i;
    TD_Point *p = CMalloc (w * h * sizeof (TD_Point));

    va_start (pa, h);
    for (i = 0; i < w * h; i++) {
	p[i].x = va_arg (pa, double);
	p[i].y = va_arg (pa, double);
	p[i].z = va_arg (pa, double);
	p[i].dirx = 0;
	p[i].diry = 0;
	p[i].dirz = 0;
    }
    va_end (pa);
    CInitSurfacePoints (ident, w, h, p);
    free (p);
}





static void fxchg (double *a, double *b)
{
    double t = *a;
    *a = *b;
    *b = t;
}


void initellipsoidpart (TD_Point * p, double x, double y, double z,
		  double a, double b, double c, int w, int dir, double f)
{
    int i, j;
    Vec v;
    double r;
    int d = 2 * w + 1;
    Vec X;
    X.x = x;
    X.y = y;
    X.z = z;


    for (i = -w; i <= w; i++)
	for (j = -w; j <= w; j++) {
	    v.x = (double) j / w;
	    v.y = (double) i / w;
	    v.z = 1;

	    switch (dir) {
	    case 0:
		v.z = -v.z;
		fxchg (&v.x, &v.y);
		break;
	    case 1:
		v.y = -v.y;
		fxchg (&v.x, &v.z);
		break;
	    case 2:
		v.z = -v.z;
		fxchg (&v.x, &v.z);
		break;
	    case 3:
		v.y = -v.y;
		fxchg (&v.y, &v.z);
		break;
	    case 4:
		v.z = -v.z;
		fxchg (&v.y, &v.z);
		break;
	    }

	    r = norm (v);
	    v.x *= (f + (1 - f) / r) * a;
	    v.y *= (f + (1 - f) / r) * b;
	    v.z *= (f + (1 - f) / r) * c;

	    assignTD (&p[i + w + (j + w) * d], plus (v, X));
	}
}

void CDraw3DEllipsoid (const char *ident, double x, double y, double z, double a, double b, double c, double f)
{
    int w = GridDensity / 2;
    int g = 2 * w + 1;
    TD_Point *p = CMalloc (g * g * sizeof (TD_Point));

    initellipsoidpart (p, x, y, z, a, b, c, w, 0, f);
    CInitSurfacePoints (ident, g, g, p);
    initellipsoidpart (p, x, y, z, a, b, c, w, 1, f);
    CInitSurfacePoints (ident, g, g, p);
    initellipsoidpart (p, x, y, z, a, b, c, w, 2, f);
    CInitSurfacePoints (ident, g, g, p);
    initellipsoidpart (p, x, y, z, a, b, c, w, 3, f);
    CInitSurfacePoints (ident, g, g, p);
    initellipsoidpart (p, x, y, z, a, b, c, w, 4, f);
    CInitSurfacePoints (ident, g, g, p);
    initellipsoidpart (p, x, y, z, a, b, c, w, 5, f);
    CInitSurfacePoints (ident, g, g, p);
    free (p);
}

void CDraw3DCappedCone (const char *ident, double x, double y, double z, double a, double b, double c, double ra, double rb)
{
    CDraw3DCone (ident, x, y, z, a, b, c, ra, rb);
    CDraw3DRoundPlate (ident, -x, -y, -z, a, b, c, ra);
    CDraw3DRoundPlate (ident, x, y, z, x + a, y + b, z + c, rb);
}


void CDraw3DRectangle (const char *ident, double x, double y, double z, double a, double b, double c)
{
    CDraw3DEllipsoid (ident, x, y, z, a, b, c, 1);
}


void CDraw3DSphere (const char *ident, double x, double y, double z, double r)
{
    CDraw3DEllipsoid (ident, x, y, z, r, r, r, 0);
}


/* returns -1 on error, zero on success */
int CDraw3DFromText (const char *ident, const char *text)
{
    char *p = (char *) text;
    int line = 1;
    double x, y, z, a, b, c, r, r2;
    Vec *v;
    int w, h, i, k;

    do {
	p += strspn (p, " \t\r");
	if (!*p)
	    break;
	if (*p == '#' || *p == '\n') {
	    /* comment, do nothing */ ;
	} else if (!strncmp (p, "scale ", 6)) {
	    if (sscanf (p, "scale %lf", &x) == 1)
		CDraw3DScale (ident, x);
	    else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "offset ", 7)) {
	    if (sscanf (p, "offset %lf %lf %lf", &x, &y, &z) == 3)
		CDraw3DOffset (ident, x, y, z);
	    else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "density ", 7)) {
	    if (sscanf (p, "density %lf", &x) == 1)
		CDraw3DDensity (ident, x);
	    else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "cylinder ", 8)) {
	    if (sscanf (p, "cylinder %lf %lf %lf %lf %lf %lf %lf", &x, &y, &z, &a, &b, &c, &r) == 7)
		CDraw3DCylinder (ident, x, y, z, a, b, c, r);
	    else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "roundplate ", 11)) {
	    if (sscanf (p, "roundplate %lf %lf %lf %lf %lf %lf %lf", &x, &y, &z, &a, &b, &c, &r) == 7) {
		CDraw3DRoundPlate (ident, x, y, z, a, b, c, r);
		CDraw3DRoundPlate (ident, -x, -y, -z, a, b, c, r);
	    } else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "cone ", 5)) {
	    if (sscanf (p, "cone %lf %lf %lf %lf %lf %lf %lf %lf", &x, &y, &z, &a, &b, &c, &r, &r2) == 8)
		CDraw3DCone (ident, x, y, z, a, b, c, r, r2);
	    else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "cappedcone ", 11)) {
	    if (sscanf (p, "cappedcone %lf %lf %lf %lf %lf %lf %lf %lf", &x, &y, &z, &a, &b, &c, &r, &r2) == 8)
		CDraw3DCappedCone (ident, x, y, z, a, b, c, r, r2);
	    else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "cappedcylinder ", 15)) {
	    if (sscanf (p, "cappedcylinder %lf %lf %lf %lf %lf %lf %lf", &x, &y, &z, &a, &b, &c, &r) == 7) {
		CDraw3DCappedCylinder (ident, x, y, z, a, b, c, r);
	    } else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "ellipsoid ", 10)) {
	    if (sscanf (p, "ellipsoid %lf %lf %lf %lf %lf %lf %lf", &x, &y, &z, &a, &b, &c, &r) == 7) {
		CDraw3DEllipsoid (ident, x, y, z, a, b, c, r);
	    } else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "rectangle ", 10)) {
	    if (sscanf (p, "rectangle %lf %lf %lf %lf %lf %lf", &x, &y, &z, &a, &b, &c) == 6) {
		CDraw3DRectangle (ident, x, y, z, a, b, c);
	    } else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "sphere ", 7)) {
	    if (sscanf (p, "sphere %lf %lf %lf %lf ", &x, &y, &z, &r) == 4) {
		CDraw3DSphere (ident, x, y, z, r);
	    } else {
		textformaterror (line, ident);
		return -1;
	    }
	} else if (!strncmp (p, "surface ", 8)) {
	    if (sscanf (p, "surface %d %d %n", &w, &h, &i) == 2) {
		v = CMalloc (w * h * sizeof (Vec));
		for (k = 0; k < w * h; k++) {
		    p += i;
		    if (sscanf (p, "%lf %lf %lf %n", &(v[k].x), &(v[k].y), &(v[k].z), &i) != 3) {
			textformaterror (line, ident);
			free (v);
			return -1;
		    }
		}
		draw3d_surface (ident, w, h, v);
		free (v);
	    } else {
		textformaterror (line, ident);
		return -1;
	    }
	} else {
	    textformaterror (line, ident);
	    return -1;
	}

	while (*p != '\n' && *p)
	    p++;
	line++;
    } while (*(p++));

    CRedraw3DObject (ident, 1);
    return 0;
}


