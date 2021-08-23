
#ifndef _3DTEXT_H
#define _3DTEXT_H 

CWidget * CDraw3DObject (const char *identifier, Window parent, int x, int y,
	       int width, int height, int defaults, int max_num_surfaces);
CWidget *CRedraw3DObject (const char *ident, int force);

void CDraw3DCone (const char *ident, double x, double y, double z, double a, double b, double c, double ra, double rb);
void CDraw3DCylinder (const char *ident, double x, double y, double z, double a, double b, double c, double r);
void CDraw3DRoundPlate (const char *ident, double x, double y, double z, double a, double b, double c, double r);
void CDraw3DCappedCylinder (const char *ident, double x, double y, double z, double a, double b, double c, double r);
void CDraw3DScale (const char *ident, double a);
void CDraw3DOffset (const char *ident, double x, double y, double z);
void CDraw3DDensity (const char *ident, double a);
void CDraw3DSurface (const char *ident, int w, int h,...);
void CDraw3DEllipsoid (const char *ident, double x, double y, double z, double a, double b, double c, double f);
void CDraw3DCappedCone (const char *ident, double x, double y, double z, double a, double b, double c, double ra, double rb);
void CDraw3DRectangle (const char *ident, double x, double y, double z, double a, double b, double c);
void CDraw3DSphere (const char *ident, double x, double y, double z, double r);
int CDraw3DFromText (const char *ident, const char *text);

#endif

