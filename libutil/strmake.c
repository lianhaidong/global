/*
 * Copyright (c) 1998, 1999, 2000, 2004
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
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include "strbuf.h"
#include "strmake.h"

/*
 * strmake: make string from original string with limit character.
 *
 *	i)	p	original string.
 *	i)	lim	limitter
 *	r)		result string
 *
 * Usage:
 *	strmake("aaa:bbb", ":/=")	=> "aaaa"
 *
 * Note: The result string area is function local. So, following call
 *	 to this function may destroy the area.
 */
char *
strmake(p, lim)
	const char *p;
	const char *lim;
{
	static STRBUF *sb;
	const char *c;

	if (sb == NULL)
		sb = strbuf_open(0);
	strbuf_reset(sb);
	for (; *p; p++) {
		for (c = lim; *c; c++)
			if (*p == *c)
				goto end;
		strbuf_putc(sb,*p);
	}
end:
	return strbuf_value(sb);
}

/*
 * strtrim: make string from original string with deleting blanks.
 *
 *	i)	p	original string.
 *	i)	flag	TRIM_HEAD	from only head
 *			TRIM_TAIL	from only tail
 *			TRIM_BOTH	from head and tail
 *			TRIM_ALL	from all
 *	o)	len	length of result string
 *			if len == NULL then nothing returned.
 *	r)		result string
 *
 * Usage:
 *	strtrim(" # define ", TRIM_HEAD, NULL)	=> "# define "
 *	strtrim(" # define ", TRIM_TAIL, NULL)	=> " # define"
 *	strtrim(" # define ", TRIM_BOTH, NULL)	=> "# define"
 *	strtrim(" # define ", TRIM_ALL, NULL)	=> "#define"
 *
 * Note: The result string area is function local. So, following call
 *	 to this function may destroy the area.
 */
char *
strtrim(p, flag, len)
	const char *p;
	int flag;
	int *len;
{
	static STRBUF *sb;
	int cut_off = -1;

	if (sb == NULL)
		sb = strbuf_open(0);
	strbuf_reset(sb);
	/*
	 * Delete blanks of the head.
	 */
	if (flag != TRIM_TAIL)
		SKIP_BLANKS(p);
	/*
	 * Copy string.
	 */
	for (; *p; p++) {
		if (isspace(*p)) {
			if (flag != TRIM_ALL) {
				if (cut_off == -1 && flag != TRIM_HEAD)
					cut_off = strbuf_getlen(sb);
				strbuf_putc(sb,*p);
			}
		} else {
			strbuf_putc(sb,*p);
			cut_off = -1;
		}
	}
	/*
	 * Delete blanks of the tail.
	 */
	if (cut_off != -1)
		strbuf_setlen(sb, cut_off);
	if (len)
		*len = strbuf_getlen(sb);
	return strbuf_value(sb);
}
