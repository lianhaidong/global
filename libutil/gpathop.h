/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2005
 *	Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * GNU GLOBAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * GNU GLOBAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#ifndef _PATHOP_H_
#define _PATHOP_H_
#include <stdio.h>

#include "gparam.h"
#include "dbop.h"

#define NEXTKEY		" __.NEXTKEY"

int gpath_open(const char *, int);
const char *gpath_path2fid(const char *, int *);
const char *gpath_fid2path(const char *, int *);
void gpath_put(const char *, int);
void gpath_delete(const char *);
void gpath_close(void);
int gpath_nextkey(void);
void gfind_open(const char *, const char *, int);
const char *gfind_read(void);
void gfind_close(void);

#endif /* ! _PATHOP_H_ */
