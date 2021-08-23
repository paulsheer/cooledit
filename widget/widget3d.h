
#ifndef WIDGET_3D_H
#define WIDGET_3D_H

#ifdef USING_MATRIXLIB
#include "matrix.h"
void CMatrixToSurface(const char *ident, int surf_width, int surf_height, Matrix *x, Matrix *offset, double scale);
#endif

CWidget * CRedraw3DObject (const char *ident, int force);

CWidget * CDraw3DObject (const char *identifier, Window parent, int x, int y,
	       int width, int height, int defaults, int max_num_surfaces);


void CInitSurfacePoints (const char *ident, int width, int height, TD_Point data[]);

void CClearAllSurfaces(const char *ident);

void render_3d_object (CWidget *wdt, int x, int y, int rendw, int rendh);

#endif

