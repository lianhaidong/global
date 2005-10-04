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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>

#include "gparam.h"
#include "find.h"
#include "getdbpath.h"
#include "gpathop.h"
#include "vfind.h"

/*
 * This is virtual layer for file system parsing.
 */
#define FIND_OPEN	1
#define GFIND_OPEN	2

static int opened = 0;
/*
 * vfind_open: start iterator for files.
 *
 *	i)	local	start directory.
 *			if NULL, it assumed '.'.
 *	i)	other	pick up other than source files.
 */
void
vfind_open(const char *local, int other)
{
	char root[MAXPATHLEN+1];
	char dbpath[MAXPATHLEN+1];
	char cwd[MAXPATHLEN+1];

	assert(opened == 0);

	if (other) {
		find_open(local);
		opened = FIND_OPEN;
	} else {
		getdbpath(cwd, root, dbpath, 0);
		gfind_open(dbpath, local);
		opened = GFIND_OPEN;
	}
}
/*
 * vfind_read: read path.
 *
 *	r)		path
 */
const char *
vfind_read(void)
{
	assert(opened != 0);
	return opened == FIND_OPEN ? find_read() : gfind_read();
}
/*
 * vfind_close: close iterator.
 */
void
vfind_close(void)
{
	assert(opened != 0);
	if (opened == FIND_OPEN)
		find_close();
	else
		gfind_close();
	opened = 0;
}
