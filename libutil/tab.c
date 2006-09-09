/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2000, 2006
 *	Tama Communications Corporation
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
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
settabs(int n)
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
detab(FILE *op, const char *buf)
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
entab(char *buf)
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
/*
 * Read file converting tabs into spaces.
 *
 *	o)	buf	
 *	i)	size	size of 'buf'
 *	i)	ip	input file
 *	o)	dest_saved	current column in 'buf'
 *	o)	spaces_saved	left spaces
 *	r)		size of data
 *
 * Dest_saved and spaces_saved are control variables.
 * You must initialize them with 0 when the input file is opened.
 */
size_t
read_file_detabing(char *buf, size_t size, FILE *ip, int *dest_saved, int *spaces_saved)
{
	char *p;
	int c, dest, spaces, n;

	if (size == 0)
		return 0;
	p = buf;
	dest = *dest_saved;
	spaces = *spaces_saved;
	if (spaces > 0)
		goto put_spaces;
	do {
		c = getc(ip);
		if (c == EOF) {
			if (ferror(ip))
				die("read error.");
			break;
		}
		if (c == '\t') {
			spaces = tabs - dest % tabs;
put_spaces:
			n = (spaces < size) ? spaces : size;
			dest += n;
			size -= n;
			spaces -= n;
			do {
				*p++ = ' ';
			} while (--n);
		} else {
			*p++ = c;
			dest++;
			if (c == '\n')
				dest = 0;
			size--;
		}
	} while (size > 0);
	*dest_saved = dest;
	*spaces_saved = spaces;
	return p - buf;
}
/*
 * detab_replacing: convert tabs into spaces and print with replacing.
 *
 *	i)	op	FILE *
 *	i)	buf	string including tabs
 *	i)	replace	replacing function
 */
void
detab_replacing(FILE *op, const char *buf, const char *(*replace)(int c))
{
	int dst, spaces;
	int c;

	dst = 0;
	while ((c = *buf++) != '\0') {
		if (c == '\t') {
			spaces = tabs - dst % tabs;
			dst += spaces;
			do {
				putc(' ', op);
			} while (--spaces);
		} else {
			const char *s = replace(c);
			if (s)
				fputs(s, op);
			else
				putc(c, op);
			dst++;
		}
	}
	putc('\n', op);
}
