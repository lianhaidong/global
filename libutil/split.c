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

/*
 * split: split a string into pieces
 *
 *	i)	s	string
 *	i)	sep	separator
 *	i)	max	max parts
 *	o)	parts	parts pointer table
 *	r)		number of parts
 */
int
split(s, sep, max, parts)
char *s;
int sep;
int max;
char *parts[];
{
	char *p;
	int count = 0;

	if (sep == ' ' || sep == '\t') {
		while (1) {
			while (*s && isspace(*s))
				s++;
			if (*s == '\0')
				break;
			parts[count++] = s;
			while (*s && !isspace(*s))
				s++;
			if (*s == '\0' || count >= max)
				break;
			*s++ = '\0';
		}
	} else {
		while (1) {
			parts[count++] = s;
			while (*s && *s != sep)
				s++;
			if (*s == '\0' || count >= max)
				break;
			*s++ = '\0';
		}
	}
	return count;
}
