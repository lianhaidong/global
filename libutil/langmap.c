/*
 * Copyright (c) 2002, 2004 Tama Communications Corporation
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
 */
void
setup_langmap(map)
	const char *map;
{
	char *p;
	int flag;

	active_map = strbuf_open(0);
	strbuf_puts(active_map, map);
	flag = 0;
	for (p = strbuf_value(active_map); *p; p++) {
		/*
		 * "c:.c.h,java:.java,cpp:.C.H"
		 */
		if ((flag == 0 && *p == ',') || (flag == 1 && *p == ':'))
			die_with_code(2, "syntax error in langmap '%s'.", map);
		if (*p == ':' || *p == ',') {
			flag ^= 1;
			*p = '\0';
		}
	}
	if (flag == 0)
		die_with_code(2, "syntax error in langmap '%s'.", map);
}

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
		if (locatestring(list, suffix, MATCH_AT_FIRST))
			return 1;
		for (list++; *list && *list != '.'; list++)
			;
	}
	return 0;
}
