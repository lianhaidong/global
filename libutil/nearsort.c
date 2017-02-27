/*
 * Copyright (c) 2014, 2015, 2017
 *      Tama Communications Corporation
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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include "getdbpath.h"
#include "gparam.h"
#include "path.h"
#include "locatestring.h"
#include "test.h"
#include "nearsort.h"

static char nearbase[MAXPATHLEN];

const char *
set_nearbase_path(const char *path)
{
	char real[MAXPATHLEN];
	const char *root = get_root();
	char *slash = "/";

	if (root[0] == '\0' || realpath(path, real) == NULL)
		return NULL;
#ifdef DEBUG
	fprintf(stderr, "realpath = %s\n", real);
#endif
	if (locatestring(real, root, MATCH_AT_FIRST) == NULL)
		return NULL;
	/*
	 * A slash should be added to the end of the directory.
	 * Avoid the following cases.
	 * .//, ./aaa.c/
	 */
	if (*(real + strlen(root)) == 0 || test("f", real))
		slash = "";
	snprintf(nearbase, sizeof(nearbase), "./%s%s", real + strlen(root) + 1, slash);
#ifdef DEBUG
	fprintf(stderr, "nearbase = %s\n", nearbase);
#endif
	return nearbase;
}
const char *
get_nearbase_path(void)
{
	return nearbase[0] ? (const char *)nearbase : NULL;
}
int
get_nearness(const char *p1, const char *p2)
{
	int parts = 0;
#ifdef DEBUG
	fprintf(stderr, "get_nearness(%s, %s)", p1, p2);
#endif
	for (; *p1 && *p2; p1++, p2++) {
		if (*p1 != *p2)
			break;
		if (*p1 == '/')
			parts++;
	}
	/*
	 * When the argument of --nearness option is a file,
	 * the file is given the highest priority.
	 */
	if (*p1 == 0 && *p2 == 0 && *(p1 - 1) != '/')
		parts++;
#ifdef DEBUG
        fprintf(stderr, " => %d\n", parts);

#endif
	return parts;
}
