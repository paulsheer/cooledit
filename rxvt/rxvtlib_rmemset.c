/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"

/*---------------------------------------------------------------------------*
 * File:	rmemset.c
 *---------------------------------------------------------------------------*
 * $Id: rmemset.c,v 1.17 1999/05/05 09:31:49 mason Exp $
 *
 * Copyright (C) 1997,1998 Geoff Wing <gcw@pobox.com>
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
 *--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*
 * Fast memset()
 * presumptions:
 *   1) intp_t write the best
 *   2) SIZEOF_INT_P= power of 2
 *--------------------------------------------------------------------------*/

#ifndef NO_RMEMSET
/* EXTPROTO */
void            rmemset (void *p, unsigned char c, intp_t len)
{E_
}
#endif

