/* app_glob.c: Declare the common global variables for X applications.
   Copyright (C) 1996-2018 Paul Sheer
 */



#ifndef APP_GLOB_C		/* To ensure that it is included once */
#define APP_GLOB_C

#include "xim.h"
#include "my_string.h"
#include "xdnd.h"

#undef max
#undef min
#define max(x,y)     (((x) > (y)) ? (x) : (y))
#define min(x,y)     (((x) < (y)) ? (x) : (y))

#ifdef DEF_APP_GLOB		/* Defined in the initapp.c file */

Display *CDisplay = NULL;	/* Connection to X display     */
Window CRoot;			/* root window */
#ifdef USE_XIM
XIM CIM = 0; 			/* input method */
#endif
int endian_little = 0;
unsigned int alt_modifier_mask = 0;
struct font_object *all_fonts = 0;
struct font_object *current_font = 0;
DndClass _CDndClass;
DndClass *CDndClass = &_CDndClass;
unsigned long color_pixels[512];	/*for pixel */
int color_last_pixel = 27;
unsigned long color_planes[256];	/*and plane values from alloccolor. */
char *CAppName;			/* Application's name    */
Window CFirstWindow = 0;	/* first window created i.e. the main window */
Visual *CVisual;
Colormap CColormap;
int CDepth;
int CXimageLSBFirst;
int option_using_grey_scale;
char *local_home_dir = 0;
char *temp_dir = 0;
char current_dir[MAX_PATH_LEN + 1];
XWindowAttributes MainXWA;	/* Attributes of main window */
int option_text_line_spacing = 1;
int option_interwidget_spacing = 4;

#define option_low_bandwidth 0
#if 0
int option_low_bandwidth = 0;
#endif

#include "bitmap/cross.bitmap"
#include "bitmap/tick.bitmap"
#include "bitmap/save.bitmap"
#include "bitmap/switchon.bitmap"
#include "bitmap/switchoff.bitmap"
#include "bitmap/exclam.bitmap"

#else

extern Display *CDisplay;
extern Window CRoot;
#ifdef USE_XIM
extern XIM CIM;
#endif
extern int endian_little;
extern unsigned int alt_modifier_mask;
extern struct font_object *all_fonts;
extern struct font_object *current_font;
extern DndClass *CDndClass;
extern unsigned long color_pixels[512];	/*for pixel */
extern int color_last_pixel;
extern unsigned long color_planes[256];	/*and plane values from alloccolor. */
extern char *CAppName;
extern Window CFirstWindow;
extern Visual *CVisual;
extern Colormap CColormap;
extern int CDepth;
extern int CXimageLSBFirst;
extern int option_using_grey_scale;
extern char *local_home_dir;
extern char *temp_dir;
extern char current_dir[MAX_PATH_LEN + 1];
extern XWindowAttributes MainXWA;
extern int option_text_line_spacing;
extern int option_interwidget_spacing;
#define option_low_bandwidth 0
#if 0
extern int option_low_bandwidth;
#endif

extern const char *cross_bits[];
extern const char *tick_bits[];
extern const char *save_pixmap[];
extern const char *switchon_bits[];
extern const char *switchoff_bits[];
extern const char *exclam_bits[];

#endif				/* #ifdef DEF_APP_GLOB */
#endif				/* #ifndef APP_GLOB_C  */
