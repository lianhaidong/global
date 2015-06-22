/*
 * Copyright (c) 2005, 2006, 2010 Tama Communications Corporation
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
#include <stdio.h>
#include <ctype.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

/*
#include "abs2rel.h"
#include "format.h"
#include "gparam.h"
#include "gpathop.h"
#include "gtagsop.h"
#include "convert.h"
#include "strlimcpy.h"
*/
#include "die.h"
#include "strbuf.h"
#include "encodepath.h"

static unsigned char encode[256];
static int encoding;

/**
 * required_encode: return encoded char.
 */
int
required_encode(int c)
{
	return encode[(unsigned char)c];
}
/**
 * set_encode_chars: stores chars to be encoded.
 */
void
set_encode_chars(const unsigned char *chars)
{
	unsigned int i;

	/* clean the table */
	memset(encode, 0, sizeof(encode));
	/* set bits */
	for (i = 0; chars[i]; i++) {
		encode[(unsigned char)chars[i]] = 1;
	}
	/* You cannot encode '.' and '/'. */
	encode['.'] = 0;
	encode['/'] = 0;
	/* '%' is always encoded when encode is enable. */
	encode['%'] = 1;
	encoding = 1;
}
/**
 * use_encoding: 
 */
int
use_encoding(void)
{
	return encoding;
}
#define outofrange(c)	(c < '0' || c > 'f')
#define h2int(c) (c >= 'a' ? c - 'a' + 10 : c - '0')
/**
 * decode_path: decode encoded path name.
 *
 *	@param[in]	path	encoded path name
 *	@return		decoded path name
 */
char *
decode_path(const char *path)
{
	STATIC_STRBUF(sb);
	const char *p;

	if (strchr(path, '%') == NULL)
		return (char *)path;
	strbuf_clear(sb);
	for (p = path; *p; p++) {
		if (*p == '%') {
			unsigned char c1, c2;
			c1 = *++p;
			c2 = *++p;
			if (outofrange(c1) || outofrange(c2))
				die("decode_path: unexpected character. (%%%c%c)", c1, c2);
			strbuf_putc(sb, h2int(c1) * 16 + h2int(c2));
		} else
			strbuf_putc(sb, *p);
	}
	return strbuf_value(sb);
}
