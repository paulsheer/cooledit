/* drawings.c - for doing modifiable line drawings in a window
   Copyright (C) 1996-2018 Paul Sheer
 */



#define DRAWINGS_C

#include <config.h>
#include <stdio.h>
#include <my_string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#include "stringtools.h"
#include "app_glob.c"

#include "coolwidget.h"
#include "coollocal.h"


#include "drawings.h"

/*
    the bounds are the maximum extents of the elements.
    only events withing the bounds are evaluated.
*/
void extendbounds (CWidget *wdt, int x1, int y1, int x2, int y2)
{
    int xb = max (x1, x2);
    int xa = min (x1, x2);
    int yb = max (y1, y2);
    int ya = min (y1, y2);

    if (wdt->pic->x1 > xa)
	wdt->pic->x1 = xa;
    if (wdt->pic->x2 < xb)
	wdt->pic->x2 = xb;

    if (wdt->pic->y1 > ya)
	wdt->pic->y1 = ya;
    if (wdt->pic->y2 < yb)
	wdt->pic->y2 = yb;
}

/* returns 1 on error */
int CSetDrawingTarget (const char *picture_ident)
{
    return (!(CDrawTarget = CIdent(picture_ident)));
}


void expose_picture(CWidget *wdt)
{
    CExposeWindowArea (wdt->parentid, 0, wdt->pic->x1 + wdt->x,
	wdt->pic->y1 + wdt->y, wdt->pic->x2 - wdt->pic->x1 + 1,
	wdt->pic->y2 - wdt->pic->y1 + 1);
}


void destroy_picture (CWidget *wdt)
{
    if (wdt->pic->pp) {
	free (wdt->pic->pp);
	wdt->pic->pp = 0;
    }
    if (wdt->pic) {
	free (wdt->pic);
	wdt->pic->pp = 0;
    }
}


void cw_reboundpic(CWidget *wdt)
{
    int j;
    wdt->pic->x1 = 30000;
    wdt->pic->x2 = -30000;
    wdt->pic->y1 = 30000;
    wdt->pic->y2 = -30000;

    if (wdt->pic->numelements) {
	for (j = 0; j < wdt->pic->numelements; j++) {
	    CPicturePrimative *pp = &(wdt->pic->pp[j]);
	    switch (pp->type) {
	    case CRECTANGLE:
	    case CFILLED_RECTANGLE:
		extendbounds (wdt, pp->x, pp->y, pp->x + pp->a, pp->y + pp->b);
		break;
	    case CLINE:
		extendbounds (wdt, pp->x, pp->y, pp->a, pp->b);
		break;
	    }
	}
    }
}


CWidget *CDrawPicture (const char *identifier, Window parent, int x, int y,
		int max_num_elements)
{
    CWidget **w;

    w = find_empty_widget_entry (); /*find first unused list entry in list of widgets*/
    *w = allocate_widget (0, identifier, parent, x, y,
		       0, 0, C_PICTURE_WIDGET);
    (*w)->pic = CMalloc(sizeof(CPicture));
    memset((*w)->pic, 0, sizeof(CPicture));
    (*w)->pic->pp = CMalloc(sizeof(CPicturePrimative) * max_num_elements);
    cw_reboundpic(*w);

    (*w)->eh = default_event_handler(C_PICTURE_WIDGET);
    (*w)->destroy = destroy_picture;
    CSetDrawingTarget (identifier);
    return (*w);
}

void CClearPicture(void)
{
    CWidget *wdt = CDrawTarget;
    wdt->pic->numelements = 0;

    expose_picture (wdt);
    cw_reboundpic(wdt);
}

int CDrawLine (float x1, float y1, float x2, float y2, unsigned long c)
{
    CWidget *wdt = CDrawTarget;
    int last = wdt->pic->numelements;

    wdt->pic->pp[last].x = x1;
    wdt->pic->pp[last].y = y1;
    wdt->pic->pp[last].a = x2;
    wdt->pic->pp[last].b = y2;
    wdt->pic->pp[last].color = c;
    wdt->pic->pp[last].type = CLINE;
    wdt->pic->numelements++;

    extendbounds (wdt, x1, y1, x2, y2);

    expose_picture (wdt);

    return last;
}

int CDrawRectangle (float x, float y, float w, float h, unsigned long c)
{
    CWidget *wdt = CDrawTarget;
    int last = wdt->pic->numelements;

    wdt->pic->pp[last].x = x;
    wdt->pic->pp[last].y = y;
    wdt->pic->pp[last].a = w;
    wdt->pic->pp[last].b = h;
    wdt->pic->pp[last].color = c;
    wdt->pic->pp[last].type = CRECTANGLE;
    wdt->pic->numelements++;

    extendbounds (wdt, x, y, x +  w, y + h);

    expose_picture (wdt);

    return last;
}


void CRemovePictureElement (int j)
{
    if (CDrawTarget->pic->numelements <= j || j < 0)
	return;

    if (CDrawTarget->pic->numelements - 1 == j)
	CDrawTarget->pic->numelements--;
    else
	CDrawTarget->pic->pp[j].type = 0;

    expose_picture (CDrawTarget);
    cw_reboundpic(CDrawTarget);
}


void CScalePicture (float s)
{
    CWidget *wdt = CDrawTarget;
    int j = 0;

    if (s == 1)
	return;

    if (s < 1)
	expose_picture (wdt);
    for (; j < wdt->pic->numelements; j++) {
	CPicturePrimative *pp = &(wdt->pic->pp[j]);
	pp->x *= s;
	pp->y *= s;
	pp->a *= s;
	pp->b *= s;
    }
    wdt->pic->x1 *= s;
    wdt->pic->x2 *= s;
    wdt->pic->y1 *= s;
    wdt->pic->y2 *= s;
    cw_reboundpic (wdt);

    if (s > 1)
	expose_picture (wdt);
}


#define is_between(a,b,c) \
		    ((a) >= (b) && (a) <= (c))

int eh_picture (CWidget *wdt, XEvent * xevent, CEvent * cwevent)
{
    int last = wdt->pic->numelements;
    CPicturePrimative *pp = wdt->pic->pp;
    Window win = wdt->parentid;
    int x = wdt->x;
    int y = wdt->y;

    switch (xevent->type) {
    case Expose:
	if (last) {
/* check if the expose covers the region of the picture */
	    int w = max (max (xevent->xexpose.x, xevent->xexpose.x + xevent->xexpose.width),
			 wdt->pic->x2 + x)
	    - min (min (xevent->xexpose.x, xevent->xexpose.x + xevent->xexpose.width),
		   wdt->pic->x1 + x);

	    int h = max (max (xevent->xexpose.y, xevent->xexpose.y + xevent->xexpose.height),
			 wdt->pic->y2 + y)
	    - min (min (xevent->xexpose.y, xevent->xexpose.y + xevent->xexpose.height),
		   wdt->pic->y1 + y);

	    if (h < wdt->pic->y2 - wdt->pic->y1 + abs (xevent->xexpose.height)
		&& w < wdt->pic->x2 - wdt->pic->x1 + abs (xevent->xexpose.width)) {
		int j = 0;
		for (; j < last; j++)
		    switch (pp[j].type) {
		    case CLINE:
			CSetColor (pp[j].color);
			CLine (win, pp[j].x + x, pp[j].y + y, pp[j].a + x, pp[j].b + y);
			break;
		    case CFILLED_RECTANGLE:
			CSetColor (pp[j].color);
			CRectangle (win, pp[j].x + x, pp[j].y + y, pp[j].a, pp[j].b);
			break;
		    case CRECTANGLE:
			CSetColor (pp[j].color);
			CLine (win, pp[j].x + x, pp[j].y + y, pp[j].x + x + pp[j].a, pp[j].y + y);
			CLine (win, pp[j].x + x, pp[j].y + y, pp[j].x + x, pp[j].y + y + pp[j].b);
			CLine (win, pp[j].x + x + pp[j].a, pp[j].y + y + pp[j].b, pp[j].x + x + pp[j].a, pp[j].y + y);
			CLine (win, pp[j].x + x + pp[j].a, pp[j].y + y + pp[j].b, pp[j].x + x, pp[j].y + y + pp[j].b);
			break;
		    }
	    }
	}
	break;
    case ButtonPress:
	break;
    }
    return 0;
}

