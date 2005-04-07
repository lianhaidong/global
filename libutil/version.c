/*
 * Copyright (c) 1999, 2000, 2001
 *	Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * GNU GLOBAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GNU GLOBAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include "version.h"

const char *copy = "\
Copyright (c) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004\n\
	Tama Communications Corporation\n\
GNU GLOBAL comes with NO WARRANTY, to the extent permitted by law.\n\
You may redistribute copies of GNU GLOBAL under the terms of the\n\
GNU General Public License. For more information about these matters,\n\
see the files named COPYING.\n\
";
/*
 * get_version: get version string.
 */
char *
get_version(void)
{
	return VERSION;
}
/*
 * version: print version information.
 */
void
version(name, verbose)
	const char *name;
	const int verbose;
{
	if (name == NULL)
		name = progname;
	if (verbose) {
		fprintf(stdout, "%s - %s\n", name, PACKAGE_STRING);
		fprintf(stdout, "%s", copy);
	} else {
		fprintf(stdout, "%s\n", VERSION);
	}
	exit(0);
}
