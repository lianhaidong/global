/*
 * Copyright (c) 2002, 2004, 2005, 2008, 2015, 2016, 2017
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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "checkalloc.h"
#include "die.h"
#include "locatestring.h"
#include "strbuf.h"
#include "strmake.h"
#include "strhash.h"
#include "langmap.h"
#include "varray.h"
#include "fnmatch.h"

static void trim_suffix_list(STRBUF *, STRHASH *);
static int match_suffix_list(const char *, const char *, const char *);

static STRBUF *active_map;
static int wflag;

/**
 * set warning flag on.
 */
void
set_langmap_wflag()
{
	wflag = 1;
}
/**
 * construct language map.
 *
 * copy string langmap (map) and convert it to language map like this:
 *
 * langmap (string)	"c:.c.h,java:.java,cpp:.C.H"
 *	|
 *	v
 * language map		c\0.c.h\0java\0.java\0cpp\0.C.H\0
 */
void
setup_langmap(const char *map)
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
/**
 * trim suffix list
 *
 * Remove duplicated suffixs from the suffix list.
 */
static void
trim_suffix_list(STRBUF *list, STRHASH *hash) {
	STATIC_STRBUF(sb);
	const char *p;

	strbuf_clear(sb);
	strbuf_puts(sb, strbuf_value(list));
	strbuf_reset(list);
	p = strbuf_value(sb);
	while (*p) {
		STATIC_STRBUF(suffix);
		struct sh_entry *sh;

		strbuf_clear(suffix);
		switch (*p) {
		case '.':	
			strbuf_putc(suffix, *p); 
			for (p++; *p && *p != '.'; p++)
				strbuf_putc(suffix, *p); 
			break;
		case '(':
			strbuf_putc(suffix, *p); 
			for (p++; *p && *p != ')'; p++)
				strbuf_putc(suffix, *p); 
			if (!*p)
				die_with_code(2, "syntax error in the suffix list '%s'.", strbuf_value(sb));
			strbuf_putc(suffix, *p++);
			break;
		default:
			die_with_code(2, "syntax error in the suffix list '%s'.", strbuf_value(sb));
			break;
		}
		if ((sh = strhash_assign(hash, strbuf_value(suffix), 0)) != NULL) {
			if (!sh->value && wflag)
				warning("langmap: suffix '%s' is duplicated. all except for the head is ignored.", strbuf_value(suffix));
			sh->value = (void *)1;
		} else {
			strbuf_puts(list, strbuf_value(suffix));
			(void)strhash_assign(hash, strbuf_value(suffix), 1);
		}
	}
}
/*
 * merge and trim a langmap
 *
 * (1) duplicated suffixes except for the first one are ignored.
 * (2) one more entries which belong to a language are gathered by one.
 *
 * Example:
 * C++:.cpp.c++,Java:.java.cpp,C++:.inl
 *      |                 ----(1)  ----(2)
 *	v
 * C++:.cpp.c++.inl,Java:.java
 */
const char *
trim_langmap(const char *map)
{
	typedef struct {
		char *name;	/* language: C++ */
		char *list;	/* suffixes: .cpp.c++ */
	} SUFFIX;
	STATIC_STRBUF(sb);
	const char *p = map;
	STRBUF *name = strbuf_open(0);
	STRBUF *list = strbuf_open(0);
	STRHASH *hash = strhash_open(10);
	VARRAY *vb = varray_open(sizeof(SUFFIX), 32);
	SUFFIX *ent = NULL;
	int i;

	strbuf_clear(sb);
	while (*p) {
		strbuf_reset(name);
		strbuf_reset(list);
		strbuf_puts(name, strmake(p, ":"));
		p += strbuf_getlen(name) + 1;
		strbuf_puts(list, strmake(p, ","));
		p += strbuf_getlen(list);
		if (*p)
			p++;
		if (strbuf_getlen(name) == 0)
			die_with_code(2, "syntax error in langmap '%s'.", map);
		if (strchr(strbuf_value(name), ','))
			die_with_code(2, "syntax error in langmap '%s'.", map);
		/*
		 * ignores duplicated suffixes.
		 */
		trim_suffix_list(list, hash);
		if (strbuf_getlen(list) == 0)
			continue;
		/*
		 * examine whether it appeared already.
		 */
		ent = NULL;
		for (i = 0; i < vb->length; i++) {
			SUFFIX *ent0 = varray_assign(vb, i, 0);
			if (!strcmp(ent0->name, strbuf_value(name))) {
				ent = ent0;
				break;
			}
		}
		if (ent == NULL) {
			/* set initial values to a new entry */
			ent = varray_append(vb);
			ent->name = check_strdup(strbuf_value(name));
			ent->list = check_strdup(strbuf_value(list));
		} else {
			/* append values to the entry */
			ent->list = check_realloc(ent->list,
				strlen(ent->list) + strbuf_getlen(list) + 1);
			strcat(ent->list, strbuf_value(list));
		}
	}
	for (i = 0; i < vb->length; i++) {
		ent = varray_assign(vb, i, 0);
		if (i > 0)
			strbuf_putc(sb, ',');
		strbuf_puts(sb, ent->name);
		strbuf_putc(sb, ':');
		strbuf_puts(sb, ent->list);
		free(ent->name);
		free(ent->list);
	}
	strbuf_close(name);
	strbuf_close(list);
	strhash_close(hash);
	varray_close(vb);
	return strbuf_value(sb);
}

/**
 * decide language of the suffix.
 *
 * 		Though '*.h' files are shared by C and C++, GLOBAL treats them
 * 		as C source files by default. If you set an environment variable
 *		'GTAGSFORCECPP' then C++ parser will be invoked.
 */
const char *
decide_lang(const char *suffix)
{
	const char *lang, *list, *tail;

	/*
	 * Though '*.h' files are shared by C and C++, GLOBAL treats them
	 * as C source files by default. If you set an environment variable
	 * 'GTAGS_FORCECPP' then C++ parser will be invoked.
	 */
	if (!strcmp(suffix, ".h") && getenv("GTAGSFORCECPP") != NULL)
		return "cpp";
	lang = strbuf_value(active_map);
	tail = lang + strbuf_getlen(active_map);

	/* check whether or not list includes suffix. */
	while (lang < tail) {
		list = lang + strlen(lang) + 1;
		if (match_suffix_list(suffix, NULL, list))
			return lang;
		lang = list + strlen(list) + 1;
	}
	return NULL;
}
/**
 * decide language of the path.
 */
const char *
decide_lang_path(const char *path)
{
	const char *lang, *list, *tail;
	const char *suffix = locatestring(path, ".", MATCH_LAST);
	const char *basename = locatestring(path, "/", MATCH_LAST);

	if (basename)
		basename++;
	/*
	 * Though '*.h' files are shared by C and C++, GLOBAL treats them
	 * as C source files by default. If you set an environment variable
	 * 'GTAGS_FORCECPP' then C++ parser will be invoked.
	 */
	if (!strcmp(suffix, ".h") && getenv("GTAGSFORCECPP") != NULL)
		return "cpp";
	lang = strbuf_value(active_map);
	tail = lang + strbuf_getlen(active_map);

	/* check whether or not list includes suffix. */
	while (lang < tail) {
		list = lang + strlen(lang) + 1;
		if (match_suffix_list(suffix, basename, list))
			return lang;
		lang = list + strlen(list) + 1;
	}
	return NULL;
}

/**
 * return true if the suffix exists in the list.
 * suffix may include '(<glob pattern>)'.
 */
STATIC_STRBUF(lastmatch);
const char *
get_last_match() {
	return strbuf_value(lastmatch);
}
static int
match_suffix_list(const char *suffix, const char *basename, const char *list)
{
	const char *p;

	strbuf_clear(lastmatch);
	suffix++;	/* skip '.' */
	/*
	 * list includes suffixes and/or patterns.
	 */
	while (*list) {
		if (*list == '.') {
			p = strmake(++list, ".(");
			if (locatestring(p, suffix, MATCH_COMPLETE
#if defined(_WIN32) || defined(__DJGPP__)
								|IGNORE_CASE
#endif
			)) {
				strbuf_putc(lastmatch, '.');
				strbuf_puts(lastmatch, p);
				return 1;
			}
			list += strlen(p);
		} else if (*list == '(') {
			p = strmake(++list, ")");
			if (basename && fnmatch(p, basename, 0) == 0) {
				strbuf_putc(lastmatch, '(');
				strbuf_puts(lastmatch, p);
				strbuf_putc(lastmatch, ')');
				return 1;
			}
			list += strlen(p) + 1;
		}
	}
	return 0;
}
