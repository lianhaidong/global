/*
 * Copyright (c) 1997, 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000 Tama Communications Corporation
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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "gparam.h"
#include "die.h"
#include "makepath.h"
#include "strbuf.h"

static STRBUF	*sb;
/*
 * makepath: make path from directory and file.
 *
 *	i)	dir	directory
 *	i)	file	file
 *	i)	suffix	suffix(optional)
 *	r)		path
 *
 * It is necessary to note the usage of makepath(), because it returns
 * module local area. If makepath() is called again in the function which
 * is passed the return value of makepath(), then the value is overwritten.
 * This may cause the bug which is not understood easily.
 * You must not pass the return value except for the safe functions
 * described below.
 * 
 * Here are safe functions.
 * o functions in standard C library.
 * o following libutil functions:
 *   test(), dbop_open(), strlimcpy(), strbuf_puts(), die()
 */
char	*
makepath(dir, file, suffix)
	const char *dir;
	const char *file;
	const char *suffix;
{
	int length;
	char sep = '/';

	if (sb == NULL)
		sb = strbuf_open(0);
	strbuf_reset(sb);
	if ((length = strlen(dir)) > MAXPATHLEN)
		die("path name too long. '%s'\n", dir);

#if defined(_WIN32) || defined(__DJGPP__)
	/* follows native way. */
	if (dir[0] == '\\' || dir[2] == '\\')
		sep = '\\';
#endif
	strbuf_puts(sb, dir);
	strbuf_unputc(sb, sep);
	strbuf_putc(sb, sep);
	strbuf_puts(sb, file);
	if (suffix) {
		if (*suffix != '.')
			strbuf_putc(sb, '.');
		strbuf_puts(sb, suffix);
	}
	if ((length = strlen(strbuf_value(sb))) > MAXPATHLEN)
		die("path name too long. '%s'\n", strbuf_value(sb));
	return strbuf_value(sb);
}
