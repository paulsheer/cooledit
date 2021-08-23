/* lkeysym.h - for any undefined keys
   Copyright (C) 1996-2018 Paul Sheer
 */


#include <X11/keysym.h>

#ifndef XK_Page_Up
#define XK_Page_Up XK_Prior
#endif

#ifndef XK_Page_Down
#define XK_Page_Down XK_Next
#endif

#ifndef XK_KP_Page_Up
#define XK_KP_Page_Up XK_KP_Prior
#endif

#ifndef XK_KP_Page_Down
#define XK_KP_Page_Down XK_KP_Next
#endif

#ifndef XK_ISO_Left_Tab
#define	XK_ISO_Left_Tab					0xFE20
#endif
