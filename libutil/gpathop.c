/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2005
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "die.h"
#include "dbop.h"
#include "gtagsop.h"
#include "makepath.h"
#include "gpathop.h"
#include "strbuf.h"
#include "strlimcpy.h"

static DBOP *dbop;
static int _nextkey;
static int _mode;
static int opened;
static int created;

/*
 * GPATH format version
 *
 * 1. Gtags(1) bury version number in GPATH.
 * 2. Global(1) pick up the version number from GPATH. If the number
 *    is not acceptable version number then global give up work any more
 *    and display error message.
 * 3. If version number is not found then it assumes version 1.
 * 4. GPATH version is independent with the other tag files.
 *
 * [History of format version]
 *
 * GLOBAL-4.8.7		no idea about format version.
 * GLOBAL-4.8.8(maybe)	understand format version.
 *			support format version 2.
 *			The invoking gtags -i (incremental updating) brings
 *			GPATH to version 2.
 *
 * - Format version 1
 *
 * GPATH has only source files.
 *
 *      key             data
 *      --------------------
 *      ./aaa.c\0       11\0
 *
 * - Format version 2
 *
 * GPATH has not only source files but also other files like README.
 * You can distinguish them by the flag following data value.
 * At present, the flag value is only 'o'(other files).
 *
 *      key             data
 *      --------------------
 *      ./aaa.c\0       11\0
 *      ./README\0      12\0o\0         <=== 'o' means other files.
 */
/*
 * get_flag: get flag value
 */
static const char *
get_flag(DBOP *dbop)
{
	int size;
	const char *dat = dbop_lastdat(dbop, &size);
	const char *flag = "";
	/*
	 * Dat format is like follows.
	 * dat 'xxxxxxx\0ffff\0'
	 *      (data)   (flag)
	 */
	if (dat) {
		int i = strlen(dat) + 1;
		if (size > i)
			flag = dat + i;
	}
	return flag;
}
/*
 * get_version: get format version
 */
static int
get_version(DBOP *dbop)
{
	const char *p;
	int format_version = 1;			/* default format version */

	if ((p = dbop_get(dbop, VERSIONKEY)) != NULL)
		format_version = atoi(p);
	return format_version;
}
/*
 * gpath_open: open gpath tag file
 *
 *	i)	dbpath	GTAGSDBPATH
 *	i)	mode	0: read only
 *			1: create
 *			2: modify
 *	r)		0: normal
 *			-1: error
 */
int
gpath_open(const char *dbpath, int mode)
{
	if (opened++ > 0) {
		if (mode != _mode)
			die("duplicate open with different mode.");
	}
	/*
	 * We create GPATH just first time.
	 */
	_mode = mode;
	if (mode == 1 && created)
		mode = 0;
	dbop = dbop_open(makepath(dbpath, dbname(GPATH), NULL), mode, 0644, 0);
	if (dbop == NULL)
		return -1;
	if (mode == 1)
		_nextkey = 1;
	else {
		const char *path = dbop_get(dbop, NEXTKEY);

		if (path == NULL)
			die("nextkey not found in GPATH.");
		_nextkey = atoi(path);
	}
	return 0;
}
/*
 * gpath_put: put path name
 *
 *	i)	path	path name
 *	i)	type	path type
 *			GPATH_SOURCE: source file
 *			GPATH_OTHER: other file
 */
void
gpath_put(const char *path, int type)
{
	char fid[32];
	STATIC_STRBUF(sb);

	assert(opened > 0);
	if (_mode == 1 && created)
		return;
	if (dbop_get(dbop, path) != NULL)
		return;
	/*
	 * generate new file id for the path.
	 */
	snprintf(fid, sizeof(fid), "%d", _nextkey++);
	/*
	 * path => fid mapping.
	 */
	strbuf_clear(sb);
	strbuf_puts0(sb, fid);
	if (type == GPATH_OTHER)
		strbuf_puts0(sb, "o");
	dbop_put_withlen(dbop, path, strbuf_value(sb), strbuf_getlen(sb));
	/*
	 * fid => path mapping.
	 */
	strbuf_clear(sb);
	strbuf_puts0(sb, path);
	if (type == GPATH_OTHER)
		strbuf_puts0(sb, "o");
	dbop_put_withlen(dbop, fid, strbuf_value(sb), strbuf_getlen(sb));
}
/*
 * gpath_path2fid: convert path into id
 *
 *	i)	path	path name
 *	o)	type	path type
 *			GPATH_SOURCE: source file
 *			GPATH_OTHER: other file
 *	r)		file id
 */
const char *
gpath_path2fid(const char *path, int *type)
{
	const char *fid = dbop_get(dbop, path);
	assert(opened > 0);
	if (fid && type) {
		const char *flag = get_flag(dbop);
		*type = (*flag == 'o') ? GPATH_OTHER : GPATH_SOURCE;
			
	}
	return fid;
}
/*
 * gpath_fid2path: convert id into path
 *
 *	i)	fid	file id
 *	o)	type	path type
 *			GPATH_SOURCE: source file
 *			GPATH_OTHER: other file
 *	r)		path name
 */
const char *
gpath_fid2path(const char *fid, int *type)
{
	const char *path = dbop_get(dbop, fid);
	assert(opened > 0);
	if (path && type) {
		const char *flag = get_flag(dbop);
		*type = (*flag == 'o') ? GPATH_OTHER : GPATH_SOURCE;
	}
	return path;
}
/*
 * gpath_delete: delete specified path record
 *
 *	i)	path	path name
 */
void
gpath_delete(const char *path)
{
	const char *fid;

	assert(opened > 0);
	assert(_mode == 2);
	assert(path[0] == '.' && path[1] == '/');
	fid = dbop_get(dbop, path);
	if (fid == NULL)
		return;
	dbop_delete(dbop, fid);
	dbop_delete(dbop, path);
}
/*
 * gpath_nextkey: return next key
 *
 *	r)		next id
 */
int
gpath_nextkey(void)
{
	assert(_mode != 1);
	return _nextkey;
}
/*
 * gpath_close: close gpath tag file
 */
void
gpath_close(void)
{
	char fid[32];

	if (--opened > 0)
		return;
	if (_mode == 1 && created) {
		dbop_close(dbop);
		return;
	}
	if (_mode == 1 || _mode == 2) {
		snprintf(fid, sizeof(fid), "%d", _nextkey);
		dbop_update(dbop, NEXTKEY, fid);
		dbop_update(dbop, VERSIONKEY, "2");
	}
	dbop_close(dbop);
	if (_mode == 1)
		created = 1;
}

/*
 * gfind iterator using GPATH.
 *
 * gfind_xxx() does almost same with find_xxx() but much faster,
 * because gfind_xxx() use GPATH (file index).
 * If GPATH exist then you should use this.
 */

/*
 * gfind_open: start iterator using GPATH.
 *
 *	i)	dbpath	dbpath
 *	i)	local	local prefix
 *			if NULL specified, it assumes "./";
 *	i)	other	treat files other than source file.
 *	r)		GFIND structure
 */
GFIND *
gfind_open(const char *dbpath, const char *local, int other)
{
	GFIND *gfind = (GFIND *)malloc(sizeof(GFIND));

	if (gfind == NULL)
		die("short of memory.");
	gfind->dbop = dbop_open(makepath(dbpath, dbname(GPATH), NULL), 0, 0, 0);
	if (gfind->dbop == NULL)
		die("GPATH not found.");
	gfind->path = NULL;
	gfind->prefix = strdup(local ? local : "./");
	if (gfind->prefix == NULL)
		die("short of memory.");
	gfind->first = 1;
	gfind->eod = 0;
	gfind->other = other;
	gfind->type = GPATH_SOURCE;
	gfind->version = get_version(gfind->dbop);
	return gfind;
}
/*
 * gfind_read: read path using GPATH.
 *
 *	i)	gfind	GFIND structure
 *	r)		path
 */
const char *
gfind_read(GFIND *gfind)
{
	const char *flag;

	gfind->type = GPATH_SOURCE;
	if (gfind->eod)
		return NULL;
	for (;;) {
		if (gfind->first) {
			gfind->first = 0;
			gfind->path = dbop_first(gfind->dbop, gfind->prefix, NULL, DBOP_KEY | DBOP_PREFIX);
		} else {
			gfind->path = dbop_next(gfind->dbop);
		}
		if (gfind->path == NULL) {
			gfind->eod = 1;
			break;
		}
		/*
		 * if gfind->other == 0, return only source files.
		 * *flag == 'o' means 'other files' like README.
		 */
		flag = get_flag(gfind->dbop);
		gfind->type = (*flag == 'o') ? GPATH_OTHER : GPATH_SOURCE;
		if (gfind->other || gfind->type == GPATH_SOURCE)
			break;
	}
	return gfind->path;
}
/*
 * gfind_close: close iterator.
 */
void
gfind_close(GFIND *gfind)
{
	dbop_close(gfind->dbop);
	free((void *)gfind->prefix);
	free(gfind);
}
