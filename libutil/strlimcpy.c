/*
 * Copyright (c) 2002
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
#include "die.h"
#include "strlimcpy.h"

/*
 * strlimcpy: copy string with limit.
 *
 *	o)	dist	distination string
 *	i)	source	source string
 *	i)	limit	size of dist
 *
 * NOTE: This function is similar to strlcpy of OpenBSD but is different
 * because strlimcpy abort when it beyond the limit.
 */
void
strlimcpy(dist, source, limit)
char *dist;
const char *source;
const int limit;
{
	int n = (int)limit;
	char *d = dist;

	while (n--)
		if (!(*d++ = *source++))
			return;
	die("buffer overflow (strlimcpy).");
}
