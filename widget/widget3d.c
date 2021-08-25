/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* widget3d.c - draws a graphics box with a rotatable 3D object
   Copyright (C) 1996-2022 Paul Sheer
 */



#include "inspect.h"
#include <config.h>
#include <stdlib.h>
#include <stdio.h>

/* #include <X11/Intrinsic.h> */

#include <math.h>
#include <my_string.h>

#include "stringtools.h"
#include "app_glob.c"

#include "coolwidget.h"
#include "widget3d.h"

#include "vgagl.h"
#include "3dkit.h"

#ifdef USING_MATRIXLIB
#include "matrix.h"
#endif

/* #define COLOR_3D_BACKGROUND (1*9 + 1*3 + 2) */
#define COLOR_3D_BACKGROUND COLOR_FLAT

#include "imagewidget.h"


extern struct look *look;

static Pixmap widget3d_currentpixmap;
static long widget3d_thres;

void destroy_solid (CWidget * w)
{E_
    if (!w->solid)
	return;
    CClearAllSurfaces (w->ident);
    if (w->solid->surf) {
	free (w->solid->surf);
	w->solid->surf = 0;
    }
    free (w->solid);
    w->solid = 0;
    if (w->gl_graphicscontext) {
	free (w->gl_graphicscontext);
	w->gl_graphicscontext = 0;
    }
}

int eh_threed (CWidget * w, XEvent * xevent, CEvent * cwevent)
{E_
    switch (xevent->type) {
    case Expose:
	render_3d_object (w, xevent->xexpose.x, xevent->xexpose.y, xevent->xexpose.width, xevent->xexpose.height);
	break;
    case ButtonRelease:
    case ButtonPress:
    case MotionNotify:
	resolve_button (xevent, cwevent);
	cwevent->x -= 2;	/*subtract border */
	cwevent->y -= 2;
	cwevent->ident = w->ident;
	break;
    }
    return 0;
}

extern int override_8bit_non_lookup;

void local_striangle (int x0, int y0,
		      int x1, int y1,
		      int x2, int y2, int z0, int bf)
{E_
    long n;
    XPoint p[3];
    p[0].x = x0;
    p[0].y = y0;
    p[1].x = x1;
    p[1].y = y1;
    p[2].x = x2;
    p[2].y = y2;

    if ((((n = (x0 - x1) * (y0 - y2) - (y0 - y1) * (x0 - x2)) > 0) + bf) & 1 || bf == 2) {
	if (abs (n) < widget3d_thres) {
	    if (abs (x0) + abs (x1) + abs (x2) + abs (y0) + abs (y1) + abs (y2) < 32000) {
		if (!override_8bit_non_lookup && CDepth >= 8 && (ClassOfVisual (CVisual) == PseudoColor || ClassOfVisual (CVisual) == StaticGray))
		    CSetColor (z0);
		else
		    CSetColor (color_pixels[z0]);
		XFillPolygon (CDisplay, widget3d_currentpixmap, CGC, p, 3, Convex, CoordModeOrigin);
	    }
	}
    }
}



void local_drawline (int x1, int y1, int x2, int y2, int c)
{E_
    CSetColor(c);
    if(abs(x1) + abs(x2) + abs(y1) + abs(y2) < 32000)
	    CLine(widget3d_currentpixmap, x1, y1, x2, y2);
}


void local_setpixel (int x, int y, int c)
{E_
    CSetColor(c);
    XDrawPoint(CDisplay, widget3d_currentpixmap, CGC, x, y);
}



/* this redraws the object. If force is non-zero then the object WILL
   be redrawn. If not the routine checks if there are any events in the
   event queue and only redraws if the event queue is empty. */
CWidget *CRedraw3DObject (const char *ident, int force)
{E_
    CWidget *w = CIdent (ident);
    Window win = w->winid;
    TD_Solid *object = w->solid;

    if (force || !CCheckWindowEvent (win, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, 0)) {
	gl_setcontext (w->gl_graphicscontext);
	gl_enableclipping ();

	widget3d_thres = w->width * w->height;
	widget3d_currentpixmap = w->pixmap;
	CSetColor (COLOR_3D_BACKGROUND);
	CRectangle (widget3d_currentpixmap, 0, 0, w->width - 4, w->height - 4);
	TD_draw_solid (object);
	XCopyArea (CDisplay, widget3d_currentpixmap, win, CGC, 0, 0, w->width - 4, w->height - 4, 2, 2);
    }
    return w;
}




/* Allocates memory for the TD_Solid structure. returns 0 on error */
TD_Solid *TD_allocate_solid (int max_num_surfaces)
{E_
    TD_Solid *object;

    if ((object = CMalloc (sizeof (TD_Solid))) == NULL)
	return 0;

    memset (object, 0, sizeof (TD_Solid));

    object->num_surfaces = max_num_surfaces;

    if ((object->surf = CMalloc (max_num_surfaces * sizeof (TD_Surface))) == NULL)
	return 0;

    memset (object->surf, 0, max_num_surfaces * sizeof (TD_Surface));

    return object;
}


/*returns NULL on error, else returns a pointer to the surface points */
TD_Surface *Calloc_surf (const char *ident, int surf_width, int surf_height, int *surf)
{E_
    int j = 0;
    CWidget *w = CIdent (ident);

    for (; j < w->solid->num_surfaces; j++)
	if (!w->solid->surf[j].point) {		/* find the first unused surface */
	    if (surf)
		*surf = j;
	    w->solid->surf[j].point = CMalloc ((surf_width + 1) * (surf_height + 1) * sizeof (TD_Point));
	    memset (w->solid->surf[j].point, 0, surf_width * surf_height * sizeof (TD_Point));
	    w->solid->surf[j].w = surf_width;
	    w->solid->surf[j].l = surf_height;
	    return &(w->solid->surf[j]);
	}
    CError ("Max number of surfaces exceeded for this solid.\n");
    return NULL;
}

/* free the i'th surface if widget ident */
void Cfree_surf (CWidget *w, int i)
{E_

    if(w->solid->surf[i].point) {
	free(w->solid->surf[i].point);
	w->solid->surf[i].point = NULL;
    }
}

/* free the last surface of widget ident, returns first available surface in surf */
/* returns 1 if there are any remaining unfree'd surfaces */
int Cfree_last_surf (CWidget *w, int *surf)
{E_
    int j = 0;
    for (; j < w->solid->num_surfaces; j++)
	if (!w->solid->surf[j].point)		/* find the first unused surface */
	    break;
    if (surf)
	*surf = j;
    if(j > 0)
	Cfree_surf(w, --j);
    if(j)
	return 1;
    return 0;
}

void CClearAllSurfaces(const char *ident)
{E_
    CWidget *w = CIdent(ident);
    while(Cfree_last_surf (w, NULL));
}

void CInitSurfacePoints (const char *ident, int width, int height, TD_Point data[])
{E_
    TD_Surface *s = Calloc_surf (ident, width, height, NULL);
    memcpy (s->point, data, width * height * sizeof (TD_Point));
    TD_initcolor (s, -256);
}

#ifdef USING_MATRIXLIB

void CMatrixToSurface (const char *ident, int surf_width, int surf_height, Matrix * x, Matrix * offset, double scale)
{E_
    TD_Point *p = CMalloc (x->columns * sizeof (TD_Point));
    int i;

    for (i = 0; i < x->columns; i++) {
	p[i].x = (double) (Mard (*x, 0, i) + Mard (*offset, 0, 0)) * scale;
	p[i].y = (double) (Mard (*x, 1, i) + Mard (*offset, 1, 0)) * scale;
	p[i].z = (double) -(Mard (*x, 2, i) + Mard (*offset, 2, 0)) * scale;
	p[i].dirx = 0;
	p[i].diry = 0;
	p[i].dirz = 0;
    }

    CInitSurfacePoints (ident, surf_width, surf_height, p);
    free (p);
}

#endif

CWidget * CDraw3DObject (const char *identifier, Window parent, int x, int y,
	       int width, int height, int defaults, int max_num_surfaces)
{E_
    int i, j, n = 0;
    CWidget *w;

    TD_Solid *object;

    if (!(object = TD_allocate_solid (max_num_surfaces)))
	CError ("Error allocating memeory in call to CDraw3DObject.\n");

    width &= 0xFFFFFFFCUL;	/* width must be a multiple of 4 */

    if (defaults) {

	n = object->num_surfaces;

	for (j = 0; j < n; j++) {
	    object->surf[j].backfacing = 1;	/*don't draw any of surface that faces away */
	    object->surf[j].depth_per_color = 6;	/*2^6 = 64 colors in the grey scale */
	    object->surf[j].bitmap1 = NULL;
	    object->surf[j].bitmap2 = NULL;
	    object->surf[j].mesh_color = color_palette (6);
	}

	object->alpha = 0;	/* begin all at zero (flight dynamics */
	object->beta = 0;	/* says plane is level */
	object->gamma = 0;

	object->xlight = -147;	/* lighting out of the screen,... */
	object->ylight = -147;	/* ...to the right,... */
	object->zlight = 147;	/* ...and from the top. */

	object->distance = 85000;	/* distance of the camera from the */
	/* origin */

	object->x_cam = 85000;
	object->y_cam = 0;
	object->z_cam = 0;

/* These two are scale factors for the screen: */
/* xscale is now calculated so that the maximum volume (-2^15 to 2^15 or
   -2^31 to 2^31) will just fit inside the screen width at this distance: */
	object->yscale = object->xscale = (long) object->distance * (width + height) / (32768 * 4);
/*to get display aspect square */

/*The above gives an average (not to telescopic, and not to wide angle) view */

/*use any triangle or linedrawing routine: */
	object->draw_triangle = gl_triangle;
	object->draw_striangle = local_striangle;
	object->draw_wtriangle = gl_wtriangle;
	object->draw_swtriangle = gl_swtriangle;
	object->draw_line = local_drawline;
	object->draw_point = local_setpixel;

/* very important to set TDOPTION_INIT_ROTATION_MATRIX if you don't
   calculate the rotation matrix yourself. */

	object->option_flags = TDOPTION_INIT_ROTATION_MATRIX
	    | TDOPTION_ALL_SAME_RENDER | TDOPTION_SORT_SURFACES
	    | TDOPTION_ROTATE_OBJECT | TDOPTION_LIGHT_SOURCE_CAM
	    | TDOPTION_FLAT_TRIANGLE;

	object->render = TD_MESH_AND_SOLID;	/*how we want to render it */

	object->posx = width / 2;
	object->posy = height / 2;
    }

    w = CSetupWidget (identifier, parent, x, y,
			  width + 4, height + 4, C_THREED_WIDGET, INPUT_MOTION, COLOR_3D_BACKGROUND, 1);
    set_hint_pos (x + width + WIDGET_SPACING + 4, y + height + WIDGET_SPACING + 4);
    w->eh = eh_threed;
    w->solid = object;
    w->destroy = destroy_solid;

    w->pixmap = XCreatePixmap (CDisplay, CFirstWindow, width, height, CDepth);

    if (defaults) {
	for (j = 0; j < n; j++) {
	    if (CDepth >= 8 && (ClassOfVisual (CVisual) == PseudoColor || ClassOfVisual (CVisual) == StaticGray)) {
/*In this case only we can use the actual pixel values because the
   triangle routines can write the calculated pixel values to the buffer
   which will be the same as the actual (hardware interpreted) pixel values */
		object->surf[j].shadow = color_grey (10);
		object->surf[j].maxcolor = color_grey (61);
	    } else {
/*In all other cases, the triangle routine must calculate the color
   and then convert it into a actual pixel value from the lookup
   table. The triangle routine itself checks if
   bytesperpixel is non-unity and performs lookup if so. */
		object->surf[j].shadow = 43 + 10;
		object->surf[j].maxcolor = 43 + 61;
	    }
	}
    }

/*now use vgagl graphics contexts */
    gl_setcontextvirtual (width, height, (CDepth + 7) / 8,
			  CDepth, 0);
    w->gl_graphicscontext = CMalloc (sizeof (GraphicsContext));
    gl_getcontext (w->gl_graphicscontext);

    for(i=0;i<256;i++)
	gl_trisetcolorlookup(i, color_pixels[i]);

    CRedraw3DObject (identifier, 2);

    return w;
}



void render_3d_object (CWidget *wdt, int x, int y, int rendw, int rendh)
{E_
    int w = wdt->width;
    int h = wdt->height;
    Window win = wdt->winid;
    int xim, yim, xwin, ywin;

    xim = x - 2;
    yim = y - 2;
    xwin = x;
    ywin = y;
    if (xim < 0) {
	rendw += xim;
	xim = 0;
	xwin = 2;
    }
    if (yim < 0) {
	rendh += yim;
	yim = 0;
	ywin = 2;
    }

    XCopyArea(CDisplay, widget3d_currentpixmap, win, CGC, xim, yim, rendw, rendh, xwin, ywin);

    render_bevel (win, 0, 0, w - 1, h - 1, 2, 1);
}
