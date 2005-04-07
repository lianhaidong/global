/*
 * Copyright (c) 2003 Tama Communications Corporation
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

#include "char.h"
#include "strbuf.h"

static char regexchar[256];
static int init;

#define ISREGEXCHAR(c)  (regexchar[(unsigned char)(c)])

/* initialize for isregex() */
static void
initialize(void)
{
	regexchar['^'] = regexchar['$'] = regexchar['{'] =
	regexchar['}'] = regexchar['('] = regexchar[')'] =
	regexchar['.'] = regexchar['*'] = regexchar['+'] =
	regexchar['['] = regexchar[']'] = regexchar['?'] =
	regexchar['\\'] = init = 1;
}
/*
 * isregexchar: test whether or not regular expression char.
 *
 *	i)	c	char
 *	r)		1: is regex, 0: not regex
 */
int
isregexchar(c)
	int c;
{
	if (!init)
		initialize();
	return ISREGEXCHAR(c);
}
/*
 * isregex: test whether or not regular expression
 *
 *	i)	s	string
 *	r)		1: is regex, 0: not regex
 */
int
isregex(s)
	const char *s;
{
	int c;

	if (!init)
		initialize();
	while ((c = *s++) != '\0')
		if (ISREGEXCHAR(c))
			return 1;
	return 0;
}
/*
 * quote string.
 *
 *	'aaa' => \'\a\a\a\'
 */
char *
quote_string(s)
	char *s;
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	for (; *s; s++) {
		strbuf_putc(sb, '\\');
		strbuf_putc(sb, *s);
	}
	return strbuf_value(sb);
}
