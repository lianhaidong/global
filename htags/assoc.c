/*
 * Copyright (c) 2004 Tama Communications Corporation
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#include "die.h"
#include "htags.h"
#include "assoc.h"

/*
 * Associate array using dbop.
 * You must specify a character for each array.
 *
 *	ASSOC *a = assoc_open('a');
 *	ASSOC *b = assoc_open('b');
 *
 * This is for internal use.
 *
 * [Why we don't use invisible db file for associate array?]
 *
 * Dbopen() with NULL path name creates invisible db file. It is useful
 * for many applications but I didn't use it because:
 *
 * 1. Temporary db file might grow up to hundreds of mega bytes or more.
 *    If the file is invisible, the administrator of the machine cannot
 *    understand why file system is full though there is no large file.
 *    We shouldn't make invisible, huge temporary db file.
 * 2. It is difficult for us to debug programs using unnamed, invisible
 *    temporary db file.
 */
/*
 * get temporary file name.
 *
 *      i)      uniq    unique character
 *      r)              temporary file name
 */
static const char *
get_tmpfile(uniq)
        int uniq;
{
        static char path[MAXPATHLEN];
        int pid = getpid();

        snprintf(path, sizeof(path), "%s/htag%c%d", tmpdir, uniq, pid);
        return path;
}

/*
 * assoc_open: open associate array.
 *
 *	i)	c	id
 *	r)		descriptor
 */
ASSOC *
assoc_open(c)
	int c;
{
	ASSOC *assoc = (ASSOC *)malloc(sizeof(ASSOC));
	const char *tmpfile = get_tmpfile(c);

	if (!assoc)
		die("short of memory.");
	assoc->dbop = dbop_open(tmpfile, 1, 0600, DBOP_REMOVE);

	if (assoc->dbop == NULL)
		abort();
	return assoc;
}
/*
 * assoc_close: close associate array.
 *
 *	i)	assoc	descriptor
 */
void
assoc_close(assoc)
	ASSOC *assoc;
{
	if (assoc == NULL)
		return;
	if (assoc->dbop == NULL)
		abort();
	dbop_close(assoc->dbop);
	free(assoc);
}
/*
 * assoc_put: put data into associate array.
 *
 *	i)	assoc	descriptor
 *	i)	name	name
 *	i)	value	value
 */
void
assoc_put(assoc, name, value)
	ASSOC *assoc;
	const char *name;
	const char *value;
{
	if (assoc->dbop == NULL)
		abort();
	dbop_put(assoc->dbop, name, value);
}
/*
 * assoc_get: get data from associate array.
 *
 *	i)	assoc	descriptor
 *	i)	name	name
 *	r)		value
 */
const char *
assoc_get(assoc, name)
	ASSOC *assoc;
	const char *name;
{
	if (assoc->dbop == NULL)
		abort();
	return dbop_get(assoc->dbop, name);
}
