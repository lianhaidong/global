/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _PATH_H_
#define _PATH_H_

#if (defined(_WIN32) && !defined(__CYGWIN__)) || defined(__DJGPP__)
#include <unistd.h>
#endif

/*
 * PATHSEP - Define OS-specific directory and path seperators
 */
#if (defined(_WIN32) && !defined(__CYGWIN__)) || defined(__DJGPP__)
#define PATHSEP ";"
#else
#define PATHSEP ":"
#endif

#define isdrivechar(x) (((x) >= 'A' && (x) <= 'Z') || ((x) >= 'a' && (x) <= 'z'))

int isabspath(char *);
char *canonpath(char *);
#if (defined(_WIN32) && !defined(__CYGWIN__)) || defined(__DJGPP__)
char *realpath(char *, char *);
#endif

#endif /* ! _PATH_H_ */
