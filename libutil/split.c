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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "split.h"
/*
 * Substring manager like perl's split.
 *
 * Initial status.
 *              +------------------------------------------------------
 *         line |main         100    ./main.c        main(argc, argv)\n
 *
 * The result of split().
 *
 *              +------------------------------------------------------
 * list    line |main\0       100\0  ./main.c\0      main(argc, argv)\n
 * +---------+   ^   ^        ^  ^   ^       ^       ^
 * |npart=4  |   |   |        |  |   |       |       |
 * +---------+   |   |        |  |   |       |       |
 * | start  *----+   |        |  |   |       |       |
 * | end    *--------+        |  |   |       |       |
 * | save ' '|                |  |   |       |       |
 * +---------+                |  |   |       |       |
 * | start  *-----------------+  |   |       |       |
 * | end    *--------------------+   |       |       |
 * | save ' '|                       |       |       |
 * +---------+                       |       |       |
 * | start  *------------------------+       |       |
 * | end    *--------------------------------+       |
 * | save ' '|                                       |
 * +---------+                                       |
 * | start  *----------------------------------------+
 * | end    *--+
 * | save    | |
 * +---------+ =
 *
 * The result of recover().
 *              +------------------------------------------------------
 *         line |main         100    ./main.c        main(argc, argv)\n
 *
 * Recover() recover initial status of line with saved char in savec.
 */

/*
 * split: split a string into pieces
 *
 *	i)	line	string
 *	i)	npart	parts number
 *	io)	list	split table
 *	r)		part count
 */
int
split(line, npart, list)
char *line;
int npart;
SPLIT *list;
{
	char *s = line;
	struct part *part;
	int count;

	if (npart > NPART)
		npart = NPART;
	npart--;
	for (count = 0; count < npart; count++) {
		while (*s && isspace(*s))
			s++;
		if (*s == '\0')
			break;
		part = &list->part[count];
		part->start = s;
		while (*s && !isspace(*s))
			s++;
		part->end = s;
		part->savec = *s;
		if (*s == '\0')
			break;
		*s++ = '\0';
	}
	if (*s) {
		while (*s && isspace(*s))
			s++;
		part = &list->part[count];
		part->start = s;
		part->end = (char *)0;
		part->savec = 0;
	}
	return list->npart = count + 1;
}
/*
 * recover: recover initial status of line.
 *
 *	io)	list	split table
 */
void
recover(list)
SPLIT *list;
{
	int i, c;
	for (i = 0; i < list->npart; i++) {
		if (c = list->part[i].savec)
			*(list->part[i].end) = c;
	}
}
