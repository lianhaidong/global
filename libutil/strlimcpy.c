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

/*
 * strlimcpy: copy string by size.
 *
 *	o)	dist	distination string
 *	i)	source	source string
 *	i)	size	size of dist
 *	r)		effectively copied size.
 *
 * NOTE: This function is similar to strlcpy() in OpenBSD
 * but was made to avoid compatibility problems.
 */
int
strlimcpy(dist, source, size)
char *dist;
const char *source;
const int size;
{
	int n = (int)size;
	char *d = dist;

	while (n--)
		if (!(*d++ = *source++))
			break;
	if (n < 0)
		die("buffer overflow (strlimcpy).");
	if (n < 0)
		*d++ = '\0';
	return d - dist - 1;
}
