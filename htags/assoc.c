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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
 */
/*
 * get temporary file name.
 *
 *      i)      uniq    unique character
 *      r)              temporary file name
 */
static char *
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
	char *tmpfile = get_tmpfile(c);

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
char *
assoc_get(assoc, name)
	ASSOC *assoc;
	const char *name;
{
	char *p;

	if (assoc->dbop == NULL)
		abort();
	p = dbop_get(assoc->dbop, name);

	return p;
}
