/*
 * Copyright (c) 1996, 1997, 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000 Tama Communications Corporation
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

#include "tab.h"

static int tabs = 8;

#define TABPOS(i)	((i)%tabs == 0)
/*
 * settabs: set default tab stop
 *
 *	i)	n	tab stop
 */
void
settabs(n)
	int n;
{
	if (n < 1 || n > 32)
		return;
	tabs = n;
}
/*
 * detab: convert tabs into spaces and print
 *
 *	i)	op	FILE *
 *	i)	buf	string including tabs
 */
void
detab(op, buf)
	FILE *op;
	char *buf;
{
	int src, dst;
	char c;

	src = dst = 0;
	while ((c = buf[src++]) != 0) {
		if (c == '\t') {
			do {
				(void)putc(' ', op);
				dst++;
			} while (!TABPOS(dst));
		} else {
			(void)putc(c, op);
			dst++;
		}
	}
	(void)putc('\n', op);
}
/*
 * entab: convert spaces into tabs
 *
 *	io)	buf	string buffer
 */
void
entab(buf)
	char *buf;
{
	int blanks = 0;
	int pos, src, dst;
	char c;

	pos = src = dst = 0;
	while ((c = buf[src++]) != 0) {
		if (c == ' ') {
			if (!TABPOS(++pos)) {
				blanks++;		/* count blanks */
				continue;
			}
			/* don't convert single blank into tab */
			buf[dst++] = (blanks == 0) ? ' ' : '\t';
		} else if (c == '\t') {
			while (!TABPOS(++pos))
				;
			buf[dst++] = '\t';
		} else {
			++pos;
			while (blanks--)
				buf[dst++] = ' ';
			buf[dst++] = c;
		}
		blanks = 0;
	}
	if (blanks > 0)
		while (blanks--)
			buf[dst++] = ' ';
	buf[dst] = 0;
}
