/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
#include "inspect.h"
#include "rxvtlib.h"

/*--------------------------------*-C-*---------------------------------*
 * File:	logging.c
 *----------------------------------------------------------------------*
 * $Id: logging.c,v 1.7.2.1 1999/07/10 04:20:01 mason Exp $
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 1992      John Bovey <jdb@ukc.ac.uk>
 *				- original version
 * Copyright (C) 1993      lipka
 * Copyright (C) 1993      Brian Stempien <stempien@cs.wmich.edu>
 * Copyright (C) 1995      Raul Garcia Garcia <rgg@tid.es>
 * Copyright (C) 1995      Piet W. Plomp <piet@idefix.icce.rug.nl>
 * Copyright (C) 1997      Raul Garcia Garcia <rgg@tid.es>
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
 *----------------------------------------------------------------------*/
