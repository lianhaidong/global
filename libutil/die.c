/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include "die.h"

static int quiet;
static int verbose;
static int debug;
static void (*exit_proc)();

void
setquiet()
{
	quiet = 1;
}
void
setverbose()
{
	verbose = 1;
}
void
setdebug()
{
	debug = 1;
}
void
sethandler(proc)
	void (*proc)();
{
	exit_proc = proc;
}
void
#ifdef HAVE_STDARG_H
die(const char *s, ...)
#else
die(s, va_alist)
	char *s;
	va_dcl
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
	if (exit_proc)
		(*exit_proc)();
	if (debug)
		abort();
	exit(1);
}

void
#ifdef HAVE_STDARG_H
die_with_code(int n, const char *s, ...)
#else
die_with_code(n, s, va_alist)
	int n;
	char *s;
	va_dcl
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
	if (exit_proc)
		(*exit_proc)();
	if (debug)
		abort();
	exit(n);
}
void
#ifdef HAVE_STDARG_H
message(const char *s, ...)
#else
message(s, va_alist)
	char *s;
	va_dcl
#endif
{
	va_list ap;

	if (!quiet && verbose) {
#ifdef HAVE_STDARG_H
		va_start(ap, s);
#else
		va_start(ap);
#endif
		(void)vfprintf(stderr, s, ap);
		va_end(ap);
		fputs("\n", stderr);
	}
}
void
#ifdef HAVE_STDARG_H
warning(const char *s, ...)
#else
warning(s, va_alist)
	char *s;
	va_dcl
#endif
{
	va_list ap;

	if (!quiet) {
		fputs("Warning: ", stderr);
#ifdef HAVE_STDARG_H
		va_start(ap, s);
#else
		va_start(ap);
#endif
		(void)vfprintf(stderr, s, ap);
		va_end(ap);
		fputs("\n", stderr);
	}
}
