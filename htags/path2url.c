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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "global.h"
#include "assoc.h"
#include "htags.h"
#include "path2url.h"

static ASSOC *assoc;
static int nextkey;

/*
 * load_gpath: load gpath tag file.
 *
 * load the contents of GPATH file into the memory.
 */
void
load_gpath(dbpath)
	char *dbpath;
{
	char command[MAXFILLEN];
	STRBUF *sb = strbuf_open(0);
	FILE *gpath;
	char *_;

	assoc = assoc_open('a');

	nextkey = 0;
	snprintf(command, sizeof(command), "gtags --scandb=%s/%s ./", dbpath, dbname(GPATH));
	if (!(gpath = popen(command, "r")))
		die("cannot execute '%s'.", command);
	while ((_ = strbuf_fgets(sb, gpath, STRBUF_NOCRLF)) != NULL) {
		SPLIT ptable;
		char *path, *no;
		int n;

		if (split(_, 2, &ptable) < 2) {
			recover(&ptable);
			die("too small number of parts in load_gpath().\n'%s'", _);
		}
		path = ptable.part[0].start;
		no   = ptable.part[1].start;
		path += 2;			/* remove './' */
		assoc_put(assoc, path, no);
		n = atoi(no);
		if (n > nextkey)
			nextkey = n;
		recover(&ptable);
	}
	if (pclose(gpath) < 0)
		die("command '%s' failed.", command);
	strbuf_close(sb);
}
/*
 * unload_gpath: load gpath tag file.
 *
 * load the contents of GPATH file into the memory.
 */
void
unload_gpath()
{
	assoc_close(assoc);
}
/*
 * path2id: convert the path name into the file id.
 *
 *	i)	path	path name
 *	r)		id
 */
char *
path2id(path)
	char *path;
{
	static char number[32], *p;

	if (strlen(path) > MAXPATHLEN)
		die("path name too long. '%s'", path);
	/*
	 * accept both aaa and ./aaa.
	 */
	if (*path == '.' && *(path + 1) == '/')
		path += 2;
	p = assoc_get(assoc, path);
	if (!p) {
		snprintf(number, sizeof(number), "%d", ++nextkey);
		assoc_put(assoc, path, number);
		p = number;
	}
	return p;
}
/*
 * path2url: convert the path name into the url.
 *
 *	i)	path	path name
 *	r)		url
 */
char *
path2url(path)
	char *path;
{
	static char buf[MAXPATHLEN];
	char *id = path2id(path);

	snprintf(buf, sizeof(buf), "%s.%s", id, HTML);
	return buf;
}
