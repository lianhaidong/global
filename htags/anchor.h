/*
 * Copyright (c) 2004 Tama Communications Corporation
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
#ifndef _ANCHOR_H_
#define _ANCHOR_H_

/*
 * Anchor table.
 */
#define ANCHOR_NAMELEN	32
struct anchor {
        int lineno;
        char type;
	char done;
	int length;
        char tag[ANCHOR_NAMELEN];
	char *reserve;
};

#define gettag(a)	(a->tag[0] ? a->tag : a->reserve)
#define settag(a, tag)	do {						\
	(a)->length = strlen(tag);					\
	if ((a)->length < ANCHOR_NAMELEN)				\
		strcpy((a)->tag, tag);					\
	else {								\
		(a)->reserve = strdup(tag);				\
		if ((a)->reserve == NULL)				\
			die("short of memory.");			\
	}								\
} while (0)

#define	A_PREV		0
#define	A_NEXT		1
#define	A_FIRST		2
#define	A_LAST		3
#define	A_TOP		4
#define	A_BOTTOM	5
#define A_SIZE		6

#define A_INDEX		6
#define A_HELP		7
#define A_LIMIT		8

void anchor_create();
void anchor_close();
void anchor_load(char *);
void anchor_unload();
struct anchor *anchor_first();
struct anchor *anchor_next();
struct anchor *anchor_get(char *, int, int, int);
int define_line(int);
int *anchor_getlinks(int);
void anchor_dump(FILE *, int);

#endif /* _ANCHOR_H_ */
