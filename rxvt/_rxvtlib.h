/*--------------------------------*-C-*---------------------------------*
 * File:	_rxvtlib.h
 *----------------------------------------------------------------------*
 * $Id: command.c,v 1.85.2.23 1999/08/12 16:32:39 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1992      John Bovey, University of Kent at Canterbury <jdb@ukc.ac.uk>
 *				- original version
 * Copyright (C) 1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 * 				- extensive modifications
 * Copyright (C) 1995      Garrett D'Amore <garrett@netcom.com>
 *				- vt100 printing
 * Copyright (C) 1995      Steven Hirsch <hirsch@emba.uvm.edu>
 *				- X11 mouse report mode and support for
 *				  DEC "private mode" save/restore functions.
 * Copyright (C) 1995      Jakub Jelinek <jj@gnu.ai.mit.edu>
 *				- key-related changes to handle Shift+function
 *				  keys properly.
 * Copyright (C) 1997      MJ Olesen <olesen@me.queensu.ca>
 *				- extensive modifications
 * Copyright (C) 1997      Raul Garcia Garcia <rgg@tid.es>
 *				- modification and cleanups for Solaris 2.x
 *				  and Linux 1.2.x
 * Copyright (C) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
 * Copyright (C) 1998      Geoff Wing <gcw@pobox.com>
 * Copyright (C) 1998      Alfredo K. Kojima <kojima@windowmaker.org>
 * Copyright (C) 1996-2017 Paul Sheer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *----------------------------------------------------------------------*/

/*************************************************************************/
/* This code has only the vaguest resemblance to rxvt-2.6.1 - Paul Sheer */
/*************************************************************************/

#ifndef _RXVT_H			/* include once only */
#include <config.h>
#if defined(__sun) && defined(__SVR4)
#include <sys/strredir.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_UTIL_H
# include <util.h>
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif
#if defined (HAVE_SYS_IOCTL_H) && !defined (__sun__)
# include <sys/ioctl.h>
#endif
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#include <sys/wait.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xfuncproto.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#ifdef GREEK_SUPPORT
# include "grkelot.h"
#endif
#if defined (NO_OLD_SELECTION) && defined(NO_NEW_SELECTION)
# error if you disable both selection styles, how can you select, silly?
#endif
#if 0
#ifdef DEBUG_MALLOC
# include "dmalloc.h"		/* This comes last */
#endif
#endif
#endif				/* _RXVT_H */
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#else
# include <sgtty.h>
#endif				/* HAVE_TERMIOS_H */
#include <X11/Xatom.h>
#include <X11/keysym.h>
#ifndef NO_XSETLOCALE
# include <X11/Xlocale.h>
#else
# ifndef NO_SETLOCALE
#  include <locale.h>
# endif
#endif
#ifdef TTY_GID_SUPPORT
# include <grp.h>
#endif
#if defined(PTYS_ARE_PTMX) && !defined(__FreeBSD__) && !defined(__DragonFly__)
# include <sys/resource.h>	/* for struct rlimit */
# include <sys/stropts.h>	/* for I_PUSH */
#endif
#ifdef UTMP_SUPPORT
# if ! defined(HAVE_STRUCT_UTMPX) && ! defined(HAVE_STRUCT_UTMP)
#  error cannot build with utmp support - no utmp or utmpx struct found
# endif
# ifdef RXVT_UTMP_AS_UTMPX
#  include <utmpx.h>
# else
#  include <utmp.h>
# endif
# ifdef HAVE_LASTLOG_H
#  include <lastlog.h>
# endif
# include <pwd.h>
# ifdef WTMP_SUPPORT
#  ifdef RXVT_UTMP_AS_UTMPX
#   ifdef RXVT_WTMPX_FILE
#   else
#    error cannot build with wtmp support - no wtmpx file found
#   endif
#  else
#   ifdef RXVT_WTMP_FILE
#   else
#    error cannot build with wtmp support - no wtmp file found
#   endif
#  endif
# endif
#endif
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_SYS_BYTEORDER_H
#include <sys/byteorder.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
#include <net/if_arp.h>
#endif
#include "X11/keysymdef.h"
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>		/* get the typedef for CARD32 */
#ifndef NEXT_SCROLLBAR
#ifdef XTERM_SCROLLBAR		/* bitmap scrollbar */
#if (SB_WIDTH != 15)
#error Error, check scrollbar width (SB_WIDTH).It must be 15 for XTERM_SCROLLBAR
#endif
#else				/* XTERM_SCROLLBAR */
#endif				/* ! XTERM_SCROLLBAR */
#else				/* ! NEXT_SCROLLBAR */
#endif				/* ! NEXT_SCROLLBAR */
#ifdef XPM_BACKGROUND
# ifdef XPM_INC_X11
#  include <X11/xpm.h>
# else
#  include <xpm.h>
# endif
#endif
