/*
 * Copyright (c) 1997, 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2002 Tama Communications Corporation
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

#ifndef _LOCATESTRING_H_
#define _LOCATESTRING_H_

#define MATCH_FIRST	0
#define MATCH_AT_FIRST	1
#define MATCH_LAST	2
#define MATCH_AT_LAST	3
#define MATCH_COMPLETE	4
#define IGNORE_CASE	8

char *locatestring(const char *, const char *, int);

#endif /* ! _LOCATESTRING_H_ */
