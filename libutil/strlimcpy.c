/*
 * Copyright (c) 2002 Tama Communications Corporation
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "die.h"
#include "strlimcpy.h"

/*
 * strlimcpy: copy string with limit.
 *
 *	o)	dest	destination string
 *	i)	source	source string
 *	i)	limit	size of dest
 *
 * NOTE: This function is similar to strlcpy of OpenBSD but is different
 * because strlimcpy abort when it beyond the limit.
 */
void
strlimcpy(dest, source, limit)
	char *dest;
	const char *const source;
	const int limit;
{
	int n = (int)limit;
	const char *s = source;

	while (n--)
		if (!(*dest++ = *s++))
			return;
	die("buffer overflow. strlimcpy(dest, '%s', %d).", source, limit);
}
