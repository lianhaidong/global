/*
 * Copyright (c) 2002, 2004, 2005
 *	Tama Communications Corporation
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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "die.h"
#include "locatestring.h"
#include "strbuf.h"
#include "langmap.h"

static int match_suffix_list(const char *, const char *);

static STRBUF *active_map;

/*
 * construct language map.
 *
 * copy string langmap and convert it to language map like this:
 *
 * langmap (string)	"c:.c.h,java:.java,cpp:.C.H"
 *	|
 *	v
 * language map		c\0.c.h\0java\0.java\0cpp\0.C.H\0
 */
void
setup_langmap(map)
	const char *map;
{
	char *p;
	int onsuffix = 0;		/* not on suffix string */

	active_map = strbuf_open(0);
	strbuf_puts(active_map, map);
	for (p = strbuf_value(active_map); *p; p++) {
		/*
		 * "c:.c.h,java:.java,cpp:.C.H"
		 */
		if ((onsuffix == 0 && *p == ',') || (onsuffix == 1 && *p == ':'))
			die_with_code(2, "syntax error in langmap '%s'.", map);
		if (*p == ':' || *p == ',') {
			onsuffix ^= 1;
			*p = '\0';
		}
	}
	if (onsuffix == 0)
		die_with_code(2, "syntax error in langmap '%s'.", map);
	/* strbuf_close(active_map); */
}

/*
 * decide the language of the suffix.
 */
const char *
decide_lang(suffix)
	const char *suffix;
{
	char *lang, *list, *tail;

	list = strbuf_value(active_map);
	tail = list + strbuf_getlen(active_map);
	lang = list;

	/* check whether or not list includes suffix. */
	while (list < tail) {
		list = lang + strlen(lang) + 1;
		if (match_suffix_list(suffix, list))
			return lang;
		lang = list + strlen(list) + 1;
	}

	return NULL;
}

/*
 * return true if suffix matches with one in suffix list.
 */
static int
match_suffix_list(suffix, list)
	const char *suffix;
	const char *list;
{
	while (*list) {
		if (locatestring(list, suffix, MATCH_AT_FIRST
#if defined(_WIN32) || defined(__DJGPP__)
							     |IGNORE_CASE
#endif
									 ))
			return 1;
		for (list++; *list && *list != '.'; list++)
			;
	}
	return 0;
}

/*
 * make the suffixes value from langmap value.
 */
void
make_suffixes(langmap, sb)
	char *langmap;
	STRBUF *sb;
{
	char *p;
	int onsuffix = 0;		/* not on suffix string */
	int first_dot = 1;

	for (p = langmap; *p; p++) {
		/*
		 * "c:.c.h,java:.java,cpp:.C.H"
		 */
		if ((onsuffix == 0 && *p == ',') || (onsuffix == 1 && *p == ':'))
			die_with_code(2, "syntax error in langmap '%s'.", langmap);
		if (*p == ':')
			onsuffix = 1;
		else if (*p == ',')
			onsuffix = 0;
		else if (onsuffix) {
			if (*p == '.') {
				if (first_dot)
					first_dot = 0;
				else
					strbuf_putc(sb, ',');
			} else 
				strbuf_putc(sb, *p);
		}
	}
	if (onsuffix == 0)
		die_with_code(2, "syntax error in langmap '%s'.", langmap);
}
