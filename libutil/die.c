/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000, 2001
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>

#include "die.h"

static int quiet;

void
setquiet()
{
	quiet = 1;
}

void
#ifdef HAVE_STDARG_H
die(const char *s, ...)
#else
die(s, va_alist)
	char *s;
	va_dcl;
#endif
{
	va_list ap;

	if (!quiet) {
		fprintf(stderr, "%s: ", progname);
#ifdef HAVE_STDARG_H
		va_start(ap, s);
#else
		va_start(ap);
#endif
		(void)vfprintf(stderr, s, ap);
		va_end(ap);
		fputs("\n", stderr);
	}
	exit(1);
}
