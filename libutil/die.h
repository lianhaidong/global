/*
 * Copyright (c) 1997, 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2001, 2002 Tama Communications Corporation
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

#ifndef _DIE_H_
#define _DIE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDARG_H 
#include <stdarg.h>
#else
#include <varargs.h>
#endif

extern	const char *progname;

void setquiet();
#ifdef HAVE_STDARG_H
void die(const char *s, ...);
#else
void die();
#endif
#ifdef HAVE_STDARG_H
void die_with_code(int n, const char *s, ...);
#else
void die_with_code();
#endif
#ifdef HAVE_STDARG_H
void verbose(const char *s, ...);
#else
void verbose();
#endif

#endif /* ! _DIE_H_ */
