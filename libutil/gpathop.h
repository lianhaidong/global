/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000
 *             Tama Communications Corporation. All rights reserved.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _PATHOP_H_
#define _PATHOP_H_
#include <stdio.h>

#include "gparam.h"
#include "dbop.h"

#define NEXTKEY		" __.NEXTKEY"

int	gpath_open(const char *, int, int);
char	*gpath_path2fid(const char *);
char	*gpath_fid2path(const char *);
void	gpath_put(const char *);
void	gpath_delete(const char *);
void	gpath_close(void);
int	gpath_nextkey(void);

#endif /* ! _PATHOP_H_ */
