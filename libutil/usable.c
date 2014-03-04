/*
 * Copyright (c) 1998, 1999, 2000, 2002, 2014
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
#include <assert.h>
#include <ctype.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "gparam.h"
#include "locatestring.h"
#include "makepath.h"
#include "path.h"
#include "test.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "usable.h"

#if defined(_WIN32) || defined(__DJGPP__)
static const char *suffix[] = {".exe", ".com", ".bat",};
#endif

/**
 * usable: check if command is executable or not.
 *
 *	@param[in]	command
 *	@return		==NULL: not found. <br>
 *			!=NULL: absolute path of @a command.
 */
char *
usable(const char *command)
{
	STRBUF *sb;
	char *p;
	const char *dir;
	static char path[MAXPATHLEN];

#if defined(_WIN32) || defined(__DJGPP__)
	int i, lim = sizeof(suffix)/sizeof(char *);
#endif

	if (isabspath(command) || locatestring(command, "./", MATCH_AT_FIRST)
		|| locatestring(command, "../", MATCH_AT_FIRST)) {
		if (test("fx", command)) {
			strlimcpy(path, command, sizeof(path));
			return path;
		}
		return NULL;
	}
	/*
	 * If found in BINDIR then use it.
	 */
	if (test("fx", makepath(BINDIR, command, NULL))) {
		strlimcpy(path, makepath(BINDIR, command, NULL), sizeof(path));
		return path;
	}
	/*
	 * Locate the command for each path in PATH.
	 */
	*path = 0;
	/* Don't use fixed length buffer for environment variable
	 * because it brings buffer overflow. */
	sb = strbuf_open(0);
	strbuf_puts(sb, getenv("PATH"));
	p = strbuf_value(sb);
	while (p) {
		dir = p;
		if ((p = locatestring(p, PATHSEP, MATCH_FIRST)) != NULL)
			*p++ = 0;
		if (test("fx", makepath(dir, command, NULL))) {
			strlimcpy(path, makepath(dir, command, NULL), sizeof(path));
			goto finish;
		}
#if defined(_WIN32) || defined(__DJGPP__)
		for (i = 0; i < lim; i++)
			if (test("f", makepath(dir, command, suffix[i]))) {
				strlimcpy(path, makepath(dir, command, suffix[i]), sizeof(path));
				goto finish;
			}
#endif
	}
finish:
	strbuf_close(sb);
	return *path ? path : NULL;
}
/**
 * version_check: check if the version meets condition
 *
 *      @param[in]      target_version
 *      @param[in]      required_version
 *      @return         -1: error <br>
 *                      0: does not meet the requirement <br>
 *                      1: meets the requirement
 */
int
check_version(const char *target_version, const char *required_version)
{
	int result = 0;
	const char *tp = target_version;
	const char *rp = required_version;

	while (tp && rp) {
		if (!isdigit(*tp) || !isdigit(*rp))
			return -1;
		result = atoi(tp) - atoi(rp);
		if (result < 0)
			return 0;
		else if (result > 0)
			return 1;
		else {
			if ((tp = strchr(tp, '.')) != 0)
				tp++;
			if ((rp = strchr(rp, '.')) != 0)
				rp++;
		}
	}
	if (tp == 0 && rp == 0)
		return 1;
	else if (rp == 0)
		return 1;
	else
		return 0;
}
