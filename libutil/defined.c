/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000
 *             Tama Communications Corporation. All rights reserved.
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
#include "dbop.h"
#include "defined.h"
#include "makepath.h"

static DBOP	*dbop = NULL;

/*
 * Tag command that supports referenced tag must call this function
 * to decide whether or not the tag is defined.
 */
int
defined(name)
const char *name;
{
	if (dbop == NULL) {
		const char *dbpath;

		/*
		 * gtags(1) set GTAGSDBPATH to the path GTAGS exist.
		 */
		if (!(dbpath = getenv("GTAGSDBPATH")))
			dbpath = ".";
		dbop = dbop_open(makepath(dbpath, "GTAGS", NULL), 0, 0, 0);
		if (dbop == NULL)
			die("GTAGS not found. (%s)", makepath(dbpath, "GTAGS", NULL));
	}
	if (dbop_get(dbop, name))
		return 1;
	return 0;
}
