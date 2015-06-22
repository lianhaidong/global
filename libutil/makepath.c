/*
 * Copyright (c) 1997, 1998, 1999, 2000
 *	Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "gparam.h"
#include "die.h"
#include "makepath.h"
#include "strbuf.h"

/**
 * makepath: make path from directory and file.
 *
 *	@param[in]	dir	directory(optional), NULL if not used.
 *	@param[in]	file	file
 *	@param[in]	suffix	suffix(optional), NULL if not used.
 *	@return		path
 *
 * [Note] Only makes a name, no real directories or files are created.
 *
 * The path in argument dir can be an absoulute or relative one (Not sure if
 * can be relative one on WIN32).
 * A '.' (dot) will be added to the suffix (file extension) argument, 
 * if not already present. Any existing suffix within file will not be removed.
 * A separator ('/' or '\\') will be added to dir, if not present,
 * then file is appended to the end of dir. file and dir are used as
 * given. None of the paths given or generated are checked for validity, but they
 * are checked for size (MAXPATHLEN), (calls die() if too big).
 *
 * It is necessary to note the usage of makepath(), because it returns
 * module local area. If makepath() is called again in the function which
 * is passed the return value of makepath(), then the value is overwritten.
 * This may cause the bug which is not understood easily.
 * You must not pass the return value except for the safe functions
 * described below.
 * 
 * Here are safe functions.
 * - functions in standard C library.
 * - following libutil functions:
 *   test(), dbop_open(), strlimcpy(), strbuf_puts(), die()
 */
const char *
makepath(const char *dir, const char *file, const char *suffix)
{
	STATIC_STRBUF(sb);
	char sep = '/';

	strbuf_clear(sb);
	if (dir != NULL) {
		if (strlen(dir) > MAXPATHLEN)
			die("path name too long. '%s'\n", dir);

#if defined(_WIN32) || defined(__DJGPP__)
		/* follows native way. */
		if (dir[0] == '\\' || dir[2] == '\\')
			sep = '\\';
#endif
		strbuf_puts(sb, dir);
		strbuf_unputc(sb, sep);
		strbuf_putc(sb, sep);
	}
	strbuf_puts(sb, file);
	if (suffix) {
		if (*suffix != '.')
			strbuf_putc(sb, '.');
		strbuf_puts(sb, suffix);
	}
	if (strbuf_getlen(sb) > MAXPATHLEN)
		die("path name too long. '%s'\n", strbuf_value(sb));
	return strbuf_value(sb);
}
