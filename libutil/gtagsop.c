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
#include <ctype.h>
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "char.h"
#include "conf.h"
#include "dbop.h"
#include "die.h"
#include "gparam.h"
#include "gtagsop.h"
#include "locatestring.h"
#include "makepath.h"
#include "path.h"
#include "gpathop.h"
#include "split.h"
#include "strbuf.h"
#include "strhash.h"
#include "strlimcpy.h"
#include "strmake.h"
#include "varray.h"

#define HASHBUCKETS	256

static int compare_lno(const void *, const void *);
static void flush_pool(GTOP *);
static const char *unpack_pathindex(const char *);
static const char *genrecord(GTOP *);
static regex_t reg;

/*
 * The concept of format version.
 *
 * Since GLOBAL's tag files are machine independent, they can be distributed
 * apart from GLOBAL itself. For example, if some network file system available,
 * client may execute global using server's tag files. In this case, both
 * GLOBAL are not necessarily the same version. So, we should assume that
 * older version of GLOBAL might access the tag files which generated
 * by new GLOBAL. To deal in such case, we decided to buried a version number
 * to both global(1) and tag files. The conclete procedure is like follows:
 *
 * 1. Gtags(1) bury the version number in tag files.
 * 2. Global(1) pick up the version number from a tag file. If the number
 *    is larger than its acceptable version number then global give up work
 *    any more and display error message.
 * 3. If version number is not found then it assumes version 1.
 *
 * [History of format version]
 *
 * GLOBAL-1.0 - 1.8	no idea about format version.
 * GLOBAL-1.9 - 2.24 	understand format version.
 *			support format version 1 (default).
 *			if (format version > 1) then print error message.
 * GLOBAL-3.0 - 4.5	support format version 1 and 2.
 *			if (format version > 2) then print error message.
 * GLOBAL-4.5.1 -	support format version 1, 2 and 3.
 *			if (format version > 3) then print error message.
 * format version 1 (default):
 *	original format.
 * format version 2 (gtags -c):
 *	compact format + pathindex format
 * format version 3 (gtags -cc):
 *	only pathindex format (undocumented)
 *
 * About GTAGS and GRTAGS, the default format is still version 1.
 * Since if version number is not found then it assumes version 1, we
 * don't bury format version in tag files currently.
 *
 * Since we know the format version 3 is better than 1 in all aspect,
 * the default format version will be changed into 3 in the future.
 *
 * [Example of each format]
 *
 *    [gtags/gtags.c]
 *    +-----------------------------
 *   1|...
 *   2| * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005
 *  .....
 * 203|main(int argc, char **argv)
 *    |{
 *
 * (Standard format)
 *
 * 0                 1   2                3
 * ------------------------------------------------------------------
 * main              203 ./gtags/gtags.c  main(int argc, char **argv)
 *
 * (Pathindex format)
 * 0                 1   2   3
 * ----------------------------------------------------
 * main              203 22  main(int argc, char **argv)
 *
 * (Compact format)
 *
 * 0    1  2
 * ----------------------------------------------------
 * main 22 203
 */
static int support_version = 3;	/* acceptable format version   */
static const char *tagslist[] = {"GPATH", "GTAGS", "GRTAGS", "GSYMS"};
/*
 * dbname: return db name
 *
 *	i)	db	0: GPATH, 1: GTAGS, 2: GRTAGS, 3: GSYMS
 *	r)		dbname
 */
const char *
dbname(int db)
{
	assert(db >= 0 && db < GTAGLIM);
	return tagslist[db];
}
/*
 * makecommand: make command line to make global tag file
 *
 *	i)	comline	skeleton command line
 *	i)	path_list	\0 separated list of path names
 *	o)	sb	command line
 *
 * command skeleton is like this:
 *	'gtags-parser -r %s'
 * following skeleton is allowed too.
 *	'gtags-parser -r'
 */
void
makecommand(const char *comline, STRBUF *path_list, STRBUF *sb)
{
	const char *p = locatestring(comline, "%s", MATCH_FIRST);
	const char *path, *end;

	if (p) {
		strbuf_nputs(sb, comline, p - comline);
	} else {
		strbuf_puts(sb, comline);
		strbuf_putc(sb, ' ');
	}

	path = strbuf_value(path_list);
	end = path + strbuf_getlen(path_list);
	while (path < end) {
		strbuf_puts(sb, path);
		path += strlen(path) + 1;
		if (path < end)
			strbuf_putc(sb, ' ');
	}

	if (p)
		strbuf_puts(sb, p + 2);
}
/*
 * formatcheck: check format of tag command's output
 *
 *	i)	ctags_x	tag line (ctags -x format)
 *
 * 0                 1  2              3
 * ----------------------------------------------------
 * func              83 ./func.c       func()
 */
void
formatcheck(const char *ctags_x)		/* virtually const */
{
	const char *p;
	SPLIT ptable;

	/*
	 * Extract parts.
	 */
	split((char *)ctags_x, 4, &ptable);

	/*
	 * line number
	 */
	if (ptable.npart < 4) {
		recover(&ptable);
		die("too small number of parts.\n'%s'", ctags_x);
	}
	for (p = ptable.part[PART_LNO].start; *p; p++) {
		if (!isdigit((unsigned char)*p)) {
			recover(&ptable);
			die("line number includes other than digit.\n'%s'", ctags_x);
		}
	}
	/*
	 * path name
	 */
	p = ptable.part[PART_PATH].start;
	if (!(*p == '.' && *(p + 1) == '/' && *(p + 2))) {
		recover(&ptable);
		die("path name must start with './'.\n'%s'", ctags_x);
	}

	recover(&ptable);
}
/*
 * gtags_open: open global tag.
 *
 *	i)	dbpath	dbpath directory
 *	i)	root	root directory (needed when compact format)
 *	i)	db	GTAGS, GRTAGS, GSYMS
 *	i)	mode	GTAGS_READ: read only
 *			GTAGS_CREATE: create tag
 *			GTAGS_MODIFY: modify tag
 *	i)	flags	GTAGS_COMPACT
 *			GTAGS_PATHINDEX
 *	r)		GTOP structure
 *
 * when error occurred, gtagopen doesn't return.
 * GTAGS_PATHINDEX needs GTAGS_COMPACT.
 */
GTOP *
gtags_open(const char *dbpath, const char *root, int db, int mode, int flags)
{
	GTOP *gtop;
	int dbmode = 0;

	if ((gtop = (GTOP *)calloc(sizeof(GTOP), 1)) == NULL)
		die("short of memory.");
	gtop->db = db;
	gtop->mode = mode;
	gtop->openflags = flags;
	switch (gtop->mode) {
	case GTAGS_READ:
		dbmode = 0;
		break;
	case GTAGS_CREATE:
		dbmode = 1;
		break;
	case GTAGS_MODIFY:
		dbmode = 2;
		break;
	default:
		assert(0);
	}

	/*
	 * allow duplicate records.
	 */
	gtop->dbop = dbop_open(makepath(dbpath, dbname(db), NULL), dbmode, 0644, DBOP_DUP);
	if (gtop->dbop == NULL) {
		if (dbmode == 1)
			die("cannot make %s.", dbname(db));
		die("%s not found.", dbname(db));
	}
	/*
	 * decide format version.
	 */
	gtop->format_version = 1;
	gtop->format = GTAGS_STANDARD;
	/*
	 * This is a special case. GSYMS had compact format even if
	 * format version 1.
	 */
	if (db == GSYMS)
		gtop->format |= GTAGS_COMPACT;
	if (gtop->mode == GTAGS_CREATE) {
		if (flags & GTAGS_COMPACT) {
			gtop->format |= GTAGS_COMPACT;
			dbop_put(gtop->dbop, COMPACTKEY, COMPACTKEY);
		}
		if (flags & GTAGS_PATHINDEX) {
			gtop->format |= GTAGS_PATHINDEX;
			dbop_put(gtop->dbop, PATHINDEXKEY, PATHINDEXKEY);
		}
		if (gtop->format & (GTAGS_COMPACT|GTAGS_PATHINDEX)) {
			char buf[80];

			if (gtop->format == GTAGS_PATHINDEX)
				gtop->format_version = 3;
			else
				gtop->format_version = 2;
			snprintf(buf, sizeof(buf),
				"%s %d", VERSIONKEY, gtop->format_version);
			dbop_put(gtop->dbop, VERSIONKEY, buf);
		}
	} else {
		/*
		 * recognize format version of GTAGS. 'format version record'
		 * is saved as a META record in GTAGS and GRTAGS.
		 * if 'format version record' is not found, it's assumed
		 * version 1.
		 */
		const char *p;

		if ((p = dbop_get(gtop->dbop, VERSIONKEY)) != NULL) {
			for (p += strlen(VERSIONKEY); *p && isspace((unsigned char)*p); p++)
				;
			gtop->format_version = atoi(p);
		}
		if (gtop->format_version > support_version)
			die("GTAGS seems new format. Please install the latest GLOBAL.");
		if (gtop->format_version > 1) {
			if (dbop_get(gtop->dbop, COMPACTKEY) != NULL)
				gtop->format |= GTAGS_COMPACT;
			if (dbop_get(gtop->dbop, PATHINDEXKEY) != NULL)
				gtop->format |= GTAGS_PATHINDEX;
		}
	}
	if (gtop->format & GTAGS_PATHINDEX || gtop->mode != GTAGS_READ) {
		if (gpath_open(dbpath, dbmode) < 0) {
			if (dbmode == 1)
				die("cannot create GPATH.");
			else
				die("GPATH not found.");
		}
	}
	/*
	 * Stuff for compact format.
	 */
	if (gtop->format & GTAGS_COMPACT) {
		assert(root != NULL);
		strlimcpy(gtop->root, root, sizeof(gtop->root));
		if (gtop->mode == GTAGS_READ)
			gtop->ib = strbuf_open(MAXBUFLEN);
		else {
			gtop->sb = strbuf_open(0);
			gtop->pool = strhash_open(HASHBUCKETS, NULL);
		}
	} else if (gtop->format == GTAGS_PATHINDEX && gtop->mode != GTAGS_READ)
		gtop->sb = strbuf_open(0);
	return gtop;
}
/*
 * gtags_put: put tag record with packing.
 *
 *	i)	gtop	descripter of GTOP
 *	i)	tag	tag name
 *	i)	ctags_x	ctags -x image
 *
 * NOTE: If format is GTAGS_COMPACT or GTAGS_PATHINDEX
 *       then this function is destructive.
 */
void
gtags_put(GTOP *gtop, const char *tag, const char *ctags_x)	/* virtually const */
{
	/*
	 * Standard format.
	 */
	if (gtop->format == GTAGS_STANDARD) {
		/* entab(ctags_x); */
		dbop_put(gtop->dbop, tag, ctags_x);
	}
	/*
	 * Only pathindex format. (not compact format)
	 */
	else if (gtop->format == GTAGS_PATHINDEX) {
		char *p = locatestring(ctags_x, "./", MATCH_FIRST);
		const char *fid, *path;
		int savec;

		if (p == NULL)
			die("path name not found.");
		path = p;
		p += 2;
		while (*p && !isspace((unsigned char)*p))
			p++;
		savec = *p;
		*p = '\0';
		fid = gpath_path2fid(path);
		if (fid == NULL)
			die("GPATH is corrupted.('%s' not found)", path);
		*p = savec;
		strbuf_reset(gtop->sb);
		strbuf_nputs(gtop->sb, ctags_x, path - ctags_x);
		strbuf_puts(gtop->sb, fid);
		strbuf_puts(gtop->sb, p);
		dbop_put(gtop->dbop, tag, strbuf_value(gtop->sb));
	}
	/*
	 * Compact format. (gtop->format & GTAGS_COMPACT)
	 */
	else {
		SPLIT ptable;
		struct sh_entry *entry;
		int *lno;

		if (split((char *)ctags_x, 4, &ptable) != 4) {
			recover(&ptable);
			die("illegal tag format.\n'%s'", ctags_x);
		}
		/*
		 * Flush the pool when path is changed.
		 * Line numbers in the pool will be sorted and duplicated
		 * records will be combined.
		 *
		 * pool    "funcA"   | 1| 3| 7|23|11| 2|...
		 *           v
		 * output  funcA 33 1,2,3,7,11,23...
		 */
		if (gtop->prev_path[0] && strcmp(gtop->prev_path, ptable.part[PART_PATH].start)) {
			flush_pool(gtop);
			strhash_reset(gtop->pool);
		}
		strlimcpy(gtop->prev_path, ptable.part[PART_PATH].start, sizeof(gtop->prev_path));
		/*
		 * Register each record into the pool.
		 *
		 * Pool image:
		 *
		 * tagname   lno
		 * ------------------------------
		 * "funcA"   | 1| 3| 7|23|11| 2|...
		 * "funcB"   |34| 2| 5|66| 3|...
		 * ...
		 */
		entry = strhash_assign(gtop->pool, ptable.part[PART_TAG].start, 1);
		if (entry->value == NULL)
			entry->value = varray_open(sizeof(int), 100);
		lno = varray_append((VARRAY *)entry->value);
		*lno = atoi(ptable.part[PART_LNO].start);
		recover(&ptable);
	}
}
/*
 * compare_lno: compare function for sorting line number.
 */
static int
compare_lno(const void *s1, const void *s2)
{
	return *(const int *)s1 - *(const int *)s2;
}
/*
 * flush_pool: flush the pool and write is as compact format.
 *
 *	i)	gtop	descripter of GTOP
 */
static void
flush_pool(GTOP *gtop)
{
	struct sh_entry *entry;
	const char *path = gtop->prev_path;

	if (gtop->format & GTAGS_PATHINDEX) {
		path = gpath_path2fid(gtop->prev_path);
		if (path == NULL)
			die("GPATH is corrupted.('%s' not found)", gtop->prev_path);
	}
	/*
	 * Write records as compact format and free line number table
	 * for each entry in the pool.
	 */
	for (entry = strhash_first(gtop->pool); entry; entry = strhash_next(gtop->pool)) {
		VARRAY *vb = (VARRAY *)entry->value;
		int *lno_array = varray_assign(vb, 0, 0);
		const char *key = entry->name;

		/*
		 * extract method when class method definition.
		 *
		 * Ex: Class::method(...)
		 *
		 * key	= 'method'
		 * data = 'Class::method  103 ./class.cpp ...'
		 */
		if (gtop->flags & GTAGS_EXTRACTMETHOD) {
			if ((key = locatestring(entry->name, ".", MATCH_LAST)) != NULL)
				key++;
			else if ((key = locatestring(entry->name, "::", MATCH_LAST)) != NULL)
				key += 2;
			else
				key = entry->name;
		}
		/* Sort line number table */
		qsort(lno_array, vb->length, sizeof(int), compare_lno); 

		strbuf_reset(gtop->sb);
		strbuf_puts(gtop->sb, entry->name);
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, path);
		strbuf_putc(gtop->sb, ' ');
		{
			int savelen = strbuf_getlen(gtop->sb);
			int last = 0;		/* line 0 doesn't exist */
			int i;

			for (i = 0; i < vb->length; i++) {
				int n = lno_array[i];

				if ((gtop->flags & GTAGS_UNIQUE) && n == last)
					continue;
				if (strbuf_getlen(gtop->sb) > savelen)
					strbuf_putc(gtop->sb, ',');
				strbuf_putn(gtop->sb, n);
				if (strbuf_getlen(gtop->sb) > DBOP_PAGESIZE / 4) {
					dbop_put(gtop->dbop, key, strbuf_value(gtop->sb));
					strbuf_setlen(gtop->sb, savelen);
				}
				last = n;
			}
			if (strbuf_getlen(gtop->sb) > savelen)
				dbop_put(gtop->dbop, key, strbuf_value(gtop->sb));
		}
		/* Free line number table */
		varray_close(vb);
	}
}
/*
 * gtags_add: add tags belonging to the path list into tag file.
 *
 *	i)	gtop	descripter of GTOP
 *	i)	comline	tag command line
 *	i)	path_list	\0 separated list of source files
 *	i)	flags	GTAGS_UNIQUE, GTAGS_EXTRACTMETHOD, GTAGS_DEBUG
 */
void
gtags_add(GTOP *gtop, const char *comline, STRBUF *path_list, int flags)
{
	const char *ctags_x;
	FILE *ip;
	STRBUF *sb = strbuf_open(0);
	STRBUF *ib = strbuf_open(MAXBUFLEN);
	const char *path, *end;
	int path_num;

	gtop->flags = flags;
	/*
	 * add path index if not yet.
	 */
	path = strbuf_value(path_list);
	end = path + strbuf_getlen(path_list);
	path_num = 0;
	while (path < end) {
		gpath_put(path);
		path_num++;
		path += strlen(path) + 1;
	}
	/*
	 * make command line.
	 */
	makecommand(comline, path_list, sb);
	/*
	 * Compact format requires the output of parser sorted by the path.
	 *
	 * We assume that the output of gtags-parser is sorted by the path.
	 * About the other parsers, it is not guaranteed, so we sort it
	 * using external sort command (gnusort).
	 */
	if ((gtop->format & GTAGS_COMPACT) != 0
	    && locatestring(comline, "gtags-parser", MATCH_FIRST) == NULL
	    && path_num > 1)
		strbuf_puts(sb, "| gnusort -k 3,3");
#ifdef DEBUG
	if (flags & GTAGS_DEBUG)
		fprintf(stderr, "gtags_add() executing '%s'\n", strbuf_value(sb));
#endif
	if (!(ip = popen(strbuf_value(sb), "r")))
		die("cannot execute '%s'.", strbuf_value(sb));
	while ((ctags_x = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		char tag[MAXTOKEN], *p;

		strbuf_trim(ib);
#ifdef DEBUG
		if (flags & GTAGS_DEBUG)
			formatcheck(ctags_x);
#endif
		/* tag = $1 */
		strlimcpy(tag, strmake(ctags_x, " \t"), sizeof(tag));
		/*
		 * extract method when class method definition.
		 *
		 * Ex: Class::method(...)
		 *
		 * key	= 'method'
		 * data = 'Class::method  103 ./class.cpp ...'
		 */
		p = tag;
		if (flags & GTAGS_EXTRACTMETHOD) {
			if ((p = locatestring(tag, ".", MATCH_LAST)) != NULL)
				p++;
			else if ((p = locatestring(tag, "::", MATCH_LAST)) != NULL)
				p += 2;
			else
				p = tag;
		}
		gtags_put(gtop, p, ctags_x);
	}
	if (pclose(ip) != 0)
		die("terminated abnormally.");
	strbuf_close(sb);
	strbuf_close(ib);
}
/*
 * gtags_delete: delete records belong to set of fid.
 *
 *	i)	gtop	GTOP structure
 *	i)	deleteset bit array of fid
 */
void
gtags_delete(GTOP *gtop, IDSET *deleteset)
{
	const char *tagline, *s_fid;
	SPLIT ptable;
	int fid;

	for (tagline = dbop_first(gtop->dbop, NULL, NULL, 0); tagline; tagline = dbop_next(gtop->dbop)) {
		/*
		 * Extract fid from the tag line.
		 */
		split((char *)tagline, 4, &ptable);
		if (gtop->format == GTAGS_STANDARD) {
			if (ptable.npart < 4) {
				recover(&ptable);
				die("too small number of parts.\n'%s'", tagline);
			}
			s_fid = gpath_path2fid(ptable.part[PART_PATH].start);
			if (s_fid == NULL)
				die("GPATH is corrupted.");
		}
		else if (gtop->format == GTAGS_PATHINDEX) {
			if (ptable.npart < 4) {
				recover(&ptable);
				die("too small number of parts in pathindex format.\n'%s'", tagline);
			}
			s_fid = ptable.part[PART_FID_PIDX].start;
		} else {	/* gtop->format & GTAGS_COMPACT */
			if (ptable.npart != 3) {
				recover(&ptable);
				die("too small number of parts in compact format.\n'%s'", tagline);
			}
			s_fid = ptable.part[PART_FID_COMP].start;
		}
		fid = atoi(s_fid);
		recover(&ptable);
		/*
		 * If the file id exists in the deleteset, delete the tagline.
		 */
		if (idset_contains(deleteset, fid))
			dbop_delete(gtop->dbop, NULL);
	}
}
/*
 * gtags_first: return first record
 *
 *	i)	gtop	GTOP structure
 *	i)	pattern	tag name
 *		o may be regular expression
 *		o may be NULL
 *	i)	flags	GTOP_PREFIX	prefix read
 *			GTOP_KEY	read key only
 *			GTOP_NOSOURCE	don't read source file(compact format)
 *			GTOP_NOREGEX	don't use regular expression.
 *			GTOP_IGNORECASE	ignore case distinction.
 *			GTOP_BASICREGEX	use basic regular expression.
 *	r)		record
 */
const char *
gtags_first(GTOP *gtop, const char *pattern, int flags)
{
	int dbflags = 0;
	char prefix[IDENTLEN+1];
	regex_t *preg = &reg;
	const char *key, *tagline;
	int regflags = 0;

	gtop->flags = flags;
	if (flags & GTOP_PREFIX && pattern != NULL)
		dbflags |= DBOP_PREFIX;
	if (flags & GTOP_KEY)
		dbflags |= DBOP_KEY;

	if (!(flags & GTOP_BASICREGEX))
		regflags |= REG_EXTENDED;
	if (flags & GTOP_IGNORECASE)
		regflags |= REG_ICASE;

	/*
	 * Get key and compiled regular expression for dbop_xxxx().
	 */
	if (flags & GTOP_NOREGEX) {
		key = pattern;
		preg = NULL;
	} else if (pattern == NULL || !strcmp(pattern, ".*")) {
		/*
		 * Since the regular expression '.*' matches to any record,
		 * we take sequential read method.
		 */
		key = NULL;
		preg = NULL;
	} else if (isregex(pattern) && regcomp(preg, pattern, regflags) == 0) {
		const char *p;

		/*
		 * If the pattern include '^' + some non regular expression
		 * characters like '^aaa[0-9]', we take prefix read method
		 * with the non regular expression part as the prefix.
		 */
		if (!(flags & GTOP_IGNORECASE) && *pattern == '^' && *(p = pattern + 1) && !isregexchar(*p)) {
			int i = 0;

			while (*p && !isregexchar(*p) && i < IDENTLEN)
				prefix[i++] = *p++;
			prefix[i] = '\0';
			key = prefix;
			dbflags |= DBOP_PREFIX;
		} else {
			key = NULL;
		}
	} else {
		key = pattern;
		preg = NULL;
	}
	tagline = dbop_first(gtop->dbop, key, preg, dbflags);
	if (tagline) {
		if (gtop->format == GTAGS_STANDARD || gtop->flags & GTOP_KEY)
			return tagline;
		else if (gtop->format == GTAGS_PATHINDEX)
			return unpack_pathindex(tagline);
		else {	/* Compact format */
			gtop->line = (char *)tagline;
			gtop->opened = 0;
			return genrecord(gtop);
		}
	}
	return NULL;
}
/*
 * gtags_next: return followed record
 *
 *	i)	gtop	GTOP structure
 *	r)		record
 *			NULL end of tag
 */
const char *
gtags_next(GTOP *gtop)
{
	const char *tagline;
	/*
	 * If it is standard format or only key.
	 * Just return it.
	 */
	if (gtop->format == GTAGS_STANDARD || gtop->flags & GTOP_KEY) {
		return dbop_next(gtop->dbop);
	}
	/*
	 * Pathindex format.
	 */
	else if (gtop->format == GTAGS_PATHINDEX) {
		tagline = dbop_next(gtop->dbop);
		return (tagline == NULL) ? NULL : unpack_pathindex(tagline);
	}
	/*
	 * gtop->format & GTAGS_COMPACT
	 */
	else {
		const char *ctags_x;

		/*
		 * If some unpacked record exists then return one of them
		 * else read the next tag line.
		 */
		if ((ctags_x = genrecord(gtop)) != NULL)
			return ctags_x;
		if ((tagline = dbop_next(gtop->dbop)) == NULL)
			return NULL;
		gtop->line = (char *)tagline;		/* gtop->line = $0 */
		gtop->opened = 0;
		return genrecord(gtop);
	}
}
/*
 * gtags_close: close tag file
 *
 *	i)	gtop	GTOP structure
 */
void
gtags_close(GTOP *gtop)
{
	if (gtop->pool) {
		if (gtop->prev_path[0])
			flush_pool(gtop);
		strhash_close(gtop->pool);
	}
	if (gtop->sb)
		strbuf_close(gtop->sb);
	if (gtop->ib)
		strbuf_close(gtop->ib);
	if (gtop->format & GTAGS_PATHINDEX || gtop->mode != GTAGS_READ)
		gpath_close();
	dbop_close(gtop->dbop);
	free(gtop);
}
/*
 * unpack_pathindex: convert pathindex format into standard format.
 *
 *	i)	tagline	tag line (pathindex format)
 */
static const char *
unpack_pathindex(const char *tagline)	/* virtually const */
{
	STATIC_STRBUF(output);
	SPLIT ptable;
	const char *path;

	split((char *)tagline, 4, &ptable);
	if (ptable.npart < 4) {
		recover(&ptable);
		die("illegal tag format.'%s'\n", tagline);
	}
	/*
	 * extract file id and convert into path name.
	 */
	path = gpath_fid2path(ptable.part[PART_FID_PIDX].start);
	if (path == NULL)
		die("GPATH is corrupted.(fid '%s' not found)", ptable.part[PART_FID_PIDX].start);
	recover(&ptable);
	/*
	 * copy line with converting fid into path name.
	 */
	strbuf_clear(output);
	strbuf_nputs(output, tagline, ptable.part[PART_FID_PIDX].start - tagline);
	strbuf_puts(output, path);
	strbuf_puts(output, ptable.part[PART_FID_PIDX].end);

	return strbuf_value(output);
}
/*
 * genrecord: generate original tag line from compact format.
 *
 *	io)	gtop	GTOP structure
 *	r)		tag line
 */
static const char *
genrecord(GTOP *gtop)
{
	const char *lnop;

	if (!gtop->opened) {
		SPLIT ptable;
		const char *fid, *path;

		gtop->opened = 1;
		split(gtop->line, 3, &ptable);
		if (ptable.npart != 3) {
			recover(&ptable);
			die("illegal compact format. '%s'\n", gtop->line);
		}
		/* Tag name */
		strlimcpy(gtop->tag, ptable.part[PART_TAG].start, sizeof(gtop->tag));

		/* Path name */
		fid = ptable.part[PART_FID_COMP].start;
		if ((path = gpath_fid2path(fid)) == NULL)
			die("GPATH is corrupted.('%s' not found)", fid);
		strlimcpy(gtop->path, path, sizeof(gtop->path));

		/* line number list */
		gtop->lnop = ptable.part[PART_LNO_COMP].start;
		/*
		 * Open source file.
		 */
		if (!(gtop->flags & GTOP_NOSOURCE)) {
			char path[MAXPATHLEN+1];

			if (gtop->root)
				snprintf(path, sizeof(path),
					"%s/%s", gtop->root, &gtop->path[2]);
			else
				snprintf(path, sizeof(path), "%s", &gtop->path[2]);
			gtop->fp = fopen(path, "r");
			gtop->lno = 0;
		}
		recover(&ptable);
	}

	lnop = gtop->lnop;
	if (*lnop >= '0' && *lnop <= '9') {
		const char *src = "";
		static char output[MAXBUFLEN+1];
		int lno;

		/* get line number */
		for (lno = 0; *lnop >= '0' && *lnop <= '9'; lnop++)
			lno = lno * 10 + *lnop - '0';
		if (*lnop == ',')
			lnop++;
		gtop->lnop = lnop;
		if (gtop->fp) {
			/*
			 * If it is duplicate line, return the previous line.
			 */
			if (gtop->lno == lno)
				return output;
			while (gtop->lno < lno) {
				if (!(src = strbuf_fgets(gtop->ib, gtop->fp, STRBUF_NOCRLF)))
					die("unexpected end of file. '%s'", gtop->path);
				strbuf_trim(gtop->ib);
				gtop->lno++;
			}
		}
		snprintf(output, sizeof(output), "%-16s %4d %-16s %s",
			gtop->tag, lno, gtop->path, src);
		return output;
	} else {
		if (gtop->opened && gtop->fp != NULL) {
			gtop->opened = 0;
			fclose(gtop->fp);
			gtop->fp = NULL;
		}
		return NULL;
	}
}
