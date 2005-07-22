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
#include "gparam.h"
#include "dbop.h"
#include "die.h"
#include "gtagsop.h"
#include "locatestring.h"
#include "makepath.h"
#include "path.h"
#include "gpathop.h"
#include "split.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "strmake.h"
#include "tab.h"

#define HASHBUCKETS	256

static unsigned int hashpjw(const char *);
static int compare_lno(const void *, const void *);
static void gtop_flush_htab(GTOP *);
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
dbname(db)
	int db;
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
makecommand(comline, path_list, sb)
	const char *comline;
	STRBUF *path_list;
	STRBUF *sb;
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
 *	i)	line	input
 *
 * 0                 1  2              3
 * ----------------------------------------------------
 * func              83 ./func.c       func()
 */
void
formatcheck(line)
	const char *line;		/* virtually const */
{
	int n;
	const char *p;
	SPLIT ptable;

	/*
	 * Extract parts.
	 */
	n = split((char *)line, 4, &ptable);

	/*
	 * line number
	 */
	if (n < 4) {
		recover(&ptable);
		die("too small number of parts.\n'%s'", line);
	}
	for (p = ptable.part[1].start; *p; p++) {
		if (!isdigit((unsigned char)*p)) {
			recover(&ptable);
			die("line number includes other than digit.\n'%s'", line);
		}
	}
	/*
	 * path name
	 */
	p = ptable.part[2].start;
	if (!(*p == '.' && *(p + 1) == '/' && *(p + 2))) {
		recover(&ptable);
		die("path name must start with './'.\n'%s'", line);
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
gtags_open(dbpath, root, db, mode, flags)
	const char *dbpath;
	const char *root;
	int db;
	int mode;
	int flags;
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
			gtop->htab = calloc(sizeof(struct gtop_compact_entry *), HASHBUCKETS);
			if (gtop->htab == NULL)
				die("short of memory.");
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
 *	i)	record	ctags -x image
 *
 * NOTE: If format is GTAGS_COMPACT or GTAGS_PATHINDEX
 *       then this function is destructive.
 */
void
gtags_put(gtop, tag, record)
	GTOP *gtop;
	const char *tag;
	const char *record;		/* virtually const */
{
	const char *line, *path;
	SPLIT ptable;
	struct gtop_compact_entry *entry, **prev;
	int *lno;

	if (gtop->format == GTAGS_STANDARD) {
		/* entab(record); */
		dbop_put(gtop->dbop, tag, record);
		return;
	}
	if (gtop->format == GTAGS_PATHINDEX) {
		char *p = locatestring(record, "./", MATCH_FIRST);
		const char *fid;
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
		strbuf_nputs(gtop->sb, record, path - record);
		strbuf_puts(gtop->sb, fid);
		strbuf_puts(gtop->sb, p);
		dbop_put(gtop->dbop, tag, strbuf_value(gtop->sb));
		return;
	}
	/*
	 * gtop->format & GTAGS_COMPACT
	 */
	if (split((char *)record, 4, &ptable) != 4) {
		recover(&ptable);
		die("illegal tag format.\n'%s'", record);
	}
	line = ptable.part[1].start;
	path = ptable.part[2].start;
	/*
	 * Register each record into the hash table.
	 * Duplicated records will be combined.
	 */
	if (gtop->prev_path[0] && strcmp(gtop->prev_path, path))
		gtop_flush_htab(gtop);
	strlimcpy(gtop->prev_path, path, sizeof(gtop->prev_path));
	for (prev = gtop->htab + hashpjw(record) % HASHBUCKETS;
	     (entry = *prev) != NULL;
	     prev = &entry->next) {
		if (strcmp(entry->tag, record) == 0)
			break;
	}
	if (entry == NULL) {
		entry = malloc(offsetof(struct gtop_compact_entry, tag) + strlen(record) + 1);
		if (entry == NULL)
			die("short of memory.");
		entry->next = NULL;
		entry->vb = varray_open(sizeof(int), 100);
		strcpy(entry->tag, record);
		*prev = entry;
	}
	lno = varray_append(entry->vb);
	*lno = atoi(line);
	recover(&ptable);
}
/*
 * hashpjw: calculate hash value for given string.
 *
 *	i)	string
 *	r)	hash value
 */
static unsigned int
hashpjw(s)
	const char *s;
{
	unsigned int h, g;

	h = strlen(s);
	while (*s != '\0') {
		h <<= 4;
		h += (unsigned char)*s++;
		g = h & 0xf0000000;
		if (g != 0)
			h = h ^ (g >> 24) ^ g;
	}

	return h;
}
/*
 * compare_lno: compare function for sorting line number.
 */
static int
compare_lno(s1, s2)
	const void *s1;
	const void *s2;
{
	return *(const int *)s1 - *(const int *)s2;
}
/*
 * gtop_flush_htab: register each record into the tag database and delete from the hash table.
 *
 *	i)	gtop	descripter of GTOP
 */
static void
gtop_flush_htab(gtop)
	GTOP *gtop;
{
	const char *path, *key;
	struct gtop_compact_entry *entry, *next;
	int *lno_array;
	int i, j, savelen;

	path = gtop->prev_path;
	if (gtop->format & GTAGS_PATHINDEX) {
		path = gpath_path2fid(gtop->prev_path);
		if (path == NULL)
			die("GPATH is corrupted.('%s' not found)", gtop->prev_path);
	}

	for (i = 0; i < HASHBUCKETS; i++) {
		for (entry = gtop->htab[i]; entry != NULL; entry = next) {
			/*
			 * extract method when class method definition.
			 *
			 * Ex: Class::method(...)
			 *
			 * key	= 'method'
			 * data = 'Class::method  103 ./class.cpp ...'
			 */
			key = entry->tag;
			if (gtop->flags & GTAGS_EXTRACTMETHOD) {
				if ((key = locatestring(entry->tag, ".", MATCH_LAST)) != NULL)
					key++;
				else if ((key = locatestring(entry->tag, "::", MATCH_LAST)) != NULL)
					key += 2;
				else
					key = entry->tag;
			}
			lno_array = varray_assign(entry->vb, 0, 0);
			qsort(lno_array, entry->vb->length, sizeof(int), compare_lno); 
			strbuf_reset(gtop->sb);
			strbuf_puts(gtop->sb, entry->tag);
			strbuf_putc(gtop->sb, ' ');
			strbuf_puts(gtop->sb, path);
			strbuf_putc(gtop->sb, ' ');
			savelen = strbuf_getlen(gtop->sb);
			for (j = 0; j < entry->vb->length; j++) {
				if ((gtop->flags & GTAGS_UNIQUE) && j > 0
				    && lno_array[j - 1] == lno_array[j])
					continue;
				if (strbuf_getlen(gtop->sb) > savelen)
					strbuf_putc(gtop->sb, ',');
				strbuf_putn(gtop->sb, lno_array[j]);
				if (strbuf_getlen(gtop->sb) > DBOP_PAGESIZE / 4) {
					dbop_put(gtop->dbop, key, strbuf_value(gtop->sb));
					strbuf_setlen(gtop->sb, savelen);
				}
			}
			if (strbuf_getlen(gtop->sb) > savelen)
				dbop_put(gtop->dbop, key, strbuf_value(gtop->sb));
			varray_close(entry->vb);
			next = entry->next;
			free(entry);
		}
		gtop->htab[i] = NULL;
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
gtags_add(gtop, comline, path_list, flags)
	GTOP *gtop;
	const char *comline;
	STRBUF *path_list;
	int flags;
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
	 * Compact format.
	 */
	if ((gtop->format & GTAGS_COMPACT) != 0
	    && locatestring(comline, "gtags-parser", MATCH_FIRST) == NULL
	    && path_num > 1)
		strbuf_puts(sb, "| gnusort -k3,3");
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
gtags_delete(gtop, deleteset)
	GTOP *gtop;
	IDSET *deleteset;
{
	const char *line, *p;
	SPLIT ptable;
	int fid, n;

	for (line = dbop_first(gtop->dbop, NULL, NULL, 0); line; line = dbop_next(gtop->dbop)) {
		n = split((char *)line, 4, &ptable);
		if (gtop->format & GTAGS_COMPACT) {
			if (n != 3)
				die("illegal compact format.\n");
			p = ptable.part[1].start;
		} else {
			if (n < 4) {
				recover(&ptable);
				die("too small number of parts.\n'%s'", line);
			}
			p = ptable.part[2].start;
		}
		if (!(gtop->format & GTAGS_PATHINDEX)) {
			p = gpath_path2fid(p);
			if (p == NULL)
				die("GPATH is corrupted.");
		}
		fid = atoi(p);
		recover(&ptable);
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
gtags_first(gtop, pattern, flags)
	GTOP *gtop;
	const char *pattern;
	int flags;
{
	int dbflags = 0;
	char prefix[IDENTLEN+1];
	regex_t *preg = &reg;
	const char *key, *p, *line;
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

	if (flags & GTOP_NOREGEX) {
		key = pattern;
		preg = NULL;
	} else if (pattern == NULL || !strcmp(pattern, ".*")) {
		key = NULL;
		preg = NULL;
	} else if (isregex(pattern) && regcomp(preg, pattern, regflags) == 0) {
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
	if ((line = dbop_first(gtop->dbop, key, preg, dbflags)) == NULL)
		return NULL;
	if (gtop->format == GTAGS_STANDARD || gtop->flags & GTOP_KEY)
		return line;
	if (gtop->format == GTAGS_PATHINDEX)
		return unpack_pathindex(line);
	/*
	 * Compact format.
	 */
	gtop->line = (char *)line;		/* gtop->line = $0 */
	gtop->opened = 0;
	return genrecord(gtop);
}
/*
 * gtags_next: return followed record
 *
 *	i)	gtop	GTOP structure
 *	r)		record
 *			NULL end of tag
 */
const char *
gtags_next(gtop)
	GTOP *gtop;
{
	const char *line;
	/*
	 * If it is standard format or only key.
	 * Just return it.
	 */
	if (gtop->format == GTAGS_STANDARD || gtop->flags & GTOP_KEY)
		return dbop_next(gtop->dbop);
	/*
	 * Pathindex format.
	 */
	if (gtop->format == GTAGS_PATHINDEX) {
		line = dbop_next(gtop->dbop);
		if (line == NULL)
			return NULL;
		return unpack_pathindex(line);
	}
	/*
	 * gtop->format & GTAGS_COMPACT
	 */
	if ((line = genrecord(gtop)) != NULL)
		return line;
	/*
	 * read next record.
	 */
	if ((line = dbop_next(gtop->dbop)) == NULL)
		return line;
	gtop->line = (char *)line;		/* gtop->line = $0 */
	gtop->opened = 0;
	return genrecord(gtop);
}
/*
 * gtags_close: close tag file
 *
 *	i)	gtop	GTOP structure
 */
void
gtags_close(gtop)
	GTOP *gtop;
{
	if (gtop->htab) {
		if (gtop->prev_path[0])
			gtop_flush_htab(gtop);
		free(gtop->htab);
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
 *	i)	line	tag line
 */
static const char *
unpack_pathindex(line)
	const char *line;		/* virtually const */
{
	STATIC_STRBUF(output);
	SPLIT ptable;
	int n;
	const char *path;

	n = split((char *)line, 4, &ptable);
	if (n < 4) {
		recover(&ptable);
		die("illegal tag format.'%s'\n", line);
	}
	/*
	 * extract path and convert into file number.
	 */
	path = gpath_fid2path(ptable.part[2].start);
	if (path == NULL)
		die("GPATH is corrupted.(fid '%s' not found)", ptable.part[2].start);
	recover(&ptable);
	/*
	 * copy line with converting.
	 */
	strbuf_clear(output);
	strbuf_nputs(output, line, ptable.part[2].start - line);
	strbuf_puts(output, path);
	strbuf_puts(output, ptable.part[2].end);

	return strbuf_value(output);
}
/*
 * genrecord: generate original tag line from compact format.
 *
 *	io)	gtop	GTOP structure
 *	r)		tag line
 */
static const char *
genrecord(gtop)
	GTOP *gtop;
{
	SPLIT ptable;
	static char output[MAXBUFLEN+1];
	char path[MAXPATHLEN+1];
	static char buf[1];
	const char *buffer = buf;
	const char *lnop;
	int tagline;

	if (!gtop->opened) {
		int n;
		const char *p;

		gtop->opened = 1;
		n = split(gtop->line, 3, &ptable);
		if (n != 3) {
			recover(&ptable);
			die("illegal compact format. '%s'\n", gtop->line);
		}
		/*
		 * gtop->tag = part[0]
		 */
		strlimcpy(gtop->tag, ptable.part[0].start, sizeof(gtop->tag));

		/*
		 * gtop->path = part[1]
		 */
		p = ptable.part[1].start;
		if (gtop->format & GTAGS_PATHINDEX) {
			const char *q;
			if ((q = gpath_fid2path(p)) == NULL)
				die("GPATH is corrupted.('%s' not found)", p);
			p = q;
		}
		strlimcpy(gtop->path, p, sizeof(gtop->path));

		/*
		 * gtop->lnop = part[2]
		 */
		gtop->lnop = ptable.part[2].start;

		if (gtop->root)
			snprintf(path, sizeof(path),
				"%s/%s", gtop->root, &gtop->path[2]);
		else
			snprintf(path, sizeof(path), "%s", &gtop->path[2]);
		if (!(gtop->flags & GTOP_NOSOURCE)) {
			if ((gtop->fp = fopen(path, "r")) != NULL)
				gtop->lno = 0;
		}
		recover(&ptable);
	}

	lnop = gtop->lnop;
	if (*lnop >= '0' && *lnop <= '9') {
		/* get line number */
		for (tagline = 0; *lnop >= '0' && *lnop <= '9'; lnop++)
			tagline = tagline * 10 + *lnop - '0';
		if (*lnop == ',')
			lnop++;
		gtop->lnop = lnop;
		if (gtop->fp) {
			if (gtop->lno == tagline)
				return output;
			while (gtop->lno < tagline) {
				if (!(buffer = strbuf_fgets(gtop->ib, gtop->fp, STRBUF_NOCRLF)))
					die("unexpected end of file. '%s'", gtop->path);
				strbuf_trim(gtop->ib);
				gtop->lno++;
			}
		}
		snprintf(output, sizeof(output), "%-16s %4d %-16s %s",
			gtop->tag, tagline, gtop->path, buffer);
		return output;
	}
	if (gtop->opened && gtop->fp != NULL) {
		gtop->opened = 0;
		fclose(gtop->fp);
		gtop->fp = NULL;
	}
	return NULL;
}
