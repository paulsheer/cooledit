/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"

/*--------------------------------*-C-*---------------------------------*
 * File:	netdisp.c
 *----------------------------------------------------------------------*
 * $Id: netdisp.c,v 1.9 1998/11/12 04:51:12 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1996      Chuck Blake <cblake@BBN.COM>
 *				- original version
 * Copyright (C) 1997      mj olesen <olesen@me.queensu.ca>
 * Copyright (C) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
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
/*----------------------------------------------------------------------*
 * support for resolving the actual IP number of the host for remote
 * DISPLAYs.  When the display is local (i.e. :0), we add support for
 * sending the first non-loopback interface IP number as the DISPLAY
 * instead of just sending the incorrect ":0".  This way telnet/rlogin
 * shells can actually get the correct information into DISPLAY for
 * xclients.
 *----------------------------------------------------------------------*/

#ifdef DISPLAY_IS_IP

/*----------------------------------------------------------------------*/
/* return a pointer to a static buffer */
/* EXTPROTO */
char           *network_display (const char *display)
{E_
    static char     ipaddress[32] = "";
    char            buffer[1024], *rval = NULL;
    struct ifconf   ifc;
    struct ifreq   *ifr;
    int             i, skfd;

    if (display[0] != ':' && strncmp (display, "unix:", 5))
	return (char *)display;	/* nothing to do */

    ifc.ifc_len = sizeof (buffer);	/* Get names of all ifaces */
    ifc.ifc_buf = buffer;

    if ((skfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror ("socket");
	return NULL;
    }
    if (ioctl (skfd, SIOCGIFCONF, &ifc) < 0) {
	perror ("SIOCGIFCONF");
	close (skfd);
	return NULL;
    }
    for (i = 0, ifr = ifc.ifc_req;

	 i < (ifc.ifc_len / sizeof (struct ifreq)); i++, ifr++) {
	struct ifreq    ifr2;

	STRCPY (ifr2.ifr_name, ifr->ifr_name);
	if (ioctl (skfd, SIOCGIFADDR, &ifr2) >= 0) {
	    unsigned long   addr;
	    struct sockaddr_in *p_addr;

	    p_addr = (struct sockaddr_in *)&(ifr2.ifr_addr);
	    addr = htonl ((unsigned long)p_addr->sin_addr.s_addr);

	    /*
	     * not "0.0.0.0" or "127.0.0.1" - so format the address
	     */
	    if (addr && addr != 0x7F000001) {
		char           *colon = strchr (display, ':');

		if (colon == NULL)
		    colon = ":0.0";

		sprintf (ipaddress, "%d.%d.%d.%d%s",
			 (int)((addr >> 030) & 0xFF),
			 (int)((addr >> 020) & 0xFF),
			 (int)((addr >> 010) & 0xFF),
			 (int)(addr & 0xFF), colon);

		rval = ipaddress;
		break;
	    }
	}
    }

    close (skfd);
    return rval;
}
#endif				/* DISPLAY_IS_IP */
/*----------------------- end-of-file (C source) -----------------------*/
