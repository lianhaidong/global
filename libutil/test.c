/*
 * Copyright (c) 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000
 *             Tama Communications Corporation. All rights reserved.
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
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "locatestring.h"
#include "test.h"

/*
 * test: 
 *
 *	i)	flags	file flags
 *
 *			"f"	[ -f path ]
 *			"d"	[ -d path ]
 *			"r"	[ -r path ]
 *			"s"	[ -s path ]
 *			"w"	[ -w path ]
 *			"x"	[ -x path ]
 *
 *	i)	path	path
 *			if NULL then previous path.
 *	r)		0: no, 1: ok
 *
 * You can specify more than one character. It assumed 'and' test.
 */
int
test(flags, path)
const char *flags;
const char *path;
{
	static struct stat sb;
	int	c;

	if (path != NULL)
		if (stat(path, &sb) < 0)
			return 0;
	while ((c = *flags++) != 0) {
		switch (c) {
		case 'f':
	 		if (!S_ISREG(sb.st_mode))
				return 0;
			break;
		case 'd':
	 		if (!S_ISDIR(sb.st_mode))
				return 0;
			break;
		case 'r':
			if (access(path, R_OK) < 0)
				return 0;
			break;
		case 's':
			if (sb.st_size == 0)
				return 0;
			break;
		case 'w':
			if (access(path, W_OK) < 0)
				return 0;
			break;
		case 'x':
#ifdef _WIN32
			/* Look at file extension to determine executability */
			if (strlen(path) < 5)
				return 0;
			if (!S_ISREG(sb.st_mode))
				return 0;
			if (!locatestring(path, ".exe", MATCH_AT_LAST|IGNORE_CASE) &&
				!locatestring(path, ".com", MATCH_AT_LAST|IGNORE_CASE) &&
				!locatestring(path, ".bat", MATCH_AT_LAST|IGNORE_CASE))
				return 0;
#else
			if (access(path, X_OK) < 0)
				return 0;
#endif
			break;
		default:
			break;
		}
	}
	return 1;
}
