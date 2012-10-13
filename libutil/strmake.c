/*
 * Copyright (c) 1998, 1999, 2000, 2004, 2006, 2010
 *	Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include "strbuf.h"
#include "strmake.h"

/**
 * strmake: make string from original string with limit character.
 *
 *	@param[in]	p	original string.
 *	@param[in]	lim	limitter
 *	@return		result string
 *
 * @par Usage:
 * @code
 *	strmake("aaa:bbb", ":/=")	=> "aaa"
 * @endcode
 *
 * @note The result string area is function local. So, following call
 *	 to this function may destroy the area.
 */
const char *
strmake(const char *p, const char *lim)
{
	STATIC_STRBUF(sb);
	const char *c;

	strbuf_clear(sb);
	for (; *p; p++) {
		for (c = lim; *c; c++)
			if (*p == *c)
				goto end;
		strbuf_putc(sb,*p);
	}
end:
	return strbuf_value(sb);
}

/**
 * strtrim: make string from original string with deleting blanks.
 *
 *	@param[in]	p	original string.
 *	@param[in]	flag	#TRIM_HEAD:	from only head <br>
 *			#TRIM_TAIL:	from only tail <br>
 *			#TRIM_BOTH:	from head and tail <br>
 *			#TRIM_ALL:	from all
 *	@param[out]	len	length of result string <br>
 *			if @CODE{len == NULL} then nothing returned.
 *	@return		result string
 *
 * @par Usage:
 * @code
 *	strtrim(" # define ", TRIM_HEAD, NULL)	=> "# define "
 *	strtrim(" # define ", TRIM_TAIL, NULL)	=> " # define"
 *	strtrim(" # define ", TRIM_BOTH, NULL)	=> "# define"
 *	strtrim(" # define ", TRIM_ALL, NULL)	=> "#define"
 * @endcode
 *
 * @note The result string area is function local. So, following call
 *	 to this function may destroy the area.
 */
const char *
strtrim(const char *p, int flag, int *len)
{
	STATIC_STRBUF(sb);
	int cut_off = -1;

	strbuf_clear(sb);
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
/**
 * strcmp with terminate character.
 *
 *	@param[in]	s1	string1
 *	@param[in]	s2	string2
 *	@param[in]	term	terminate character
 *	@return		==0: equal, !=0: not equal
 *
 * @par Usage:
 * @code
 *	strcmp_withterm("aaa", "aaa", ':')		=> 0
 *	strcmp_withterm("aaa:bbb", "aaa", ':')		=> 0
 *	strcmp_withterm("aaa:bbb", "aaa:ccc", ':')	=> 0
 *	strcmp_withterm("aaa/bbb", "aaa/ccc", ':')	=> -1
 * @endcode
 */
int
strcmp_withterm(const char *s1, const char *s2, int term)
{
	unsigned int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;
		/* replace terminate character with NULL */
		if (c1 == term)
			c1 = '\0';
		if (c2 == term)
			c2 = '\0';
	} while (c1 == c2 && c1 != '\0');

	return c1 - c2;
}
/**
 * strcpy with terminate character.
 *
 *	@param[in]	b	buffer
 *	@param[in]	s	string
 *	@param[in]	size	buffer size
 *	@param[in]	term	terminate character
 *	@return		terminator's position
 */
const char *
strcpy_withterm(char *b, const char *s, int size, int term)
{
	char *endp = b + size - 1;

	while (*s && *s != term)
		if (b < endp)
			*b++ = *s++;
	*b = '\0';

	return s;
}
