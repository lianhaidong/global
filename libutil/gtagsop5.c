/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2005, 2006
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
#include "compress.h"
#include "dbop.h"
#include "die.h"
#include "format.h"
#include "gparam.h"
#include "gtagsop.h"
#include "gtagsop5.h"
#include "locatestring.h"
#include "makepath.h"
#include "path.h"
#include "gpathop.h"
#include "split.h"
#include "strbuf.h"
#include "strhash.h"
#include "strlimcpy.h"
#include "varray.h"

#define HASHBUCKETS	256

static int compare_lno(const void *, const void *);
static void flush_pool(GTOP *);
static const char *genrecord(GTOP *, const char *);
static const char *genrecord_compact(GTOP *);
/*
 * Developing version of gtagsop module.
 *
 * You can test this module by gtags -ccc option.
 *
 * STANDARD5 format
 *	<file id> <tag name> <line number> <line image>
 * COMPACT5 format
 *	<file id> <tag name> <line number>[,...]
 */
static int support_version = 4;	/* acceptable format version   */

/*
 * gtags_open5: open global tag.
 *
 *	i)	dbpath	dbpath directory
 *	i)	root	root directory (needed when compact format)
 *	i)	gtop->db	GTAGS, GRTAGS, GSYMS
 *	i)	gtop->mode	GTAGS_READ: read only
 *			GTAGS_CREATE: create tag
 *			GTAGS_MODIFY: modify tag
 *	i)	gtop->openflags	GTAGS_COMPRESS
 *	r)		GTOP structure
 *
 * when error occurred, gtagopen doesn't return.
 */
GTOP *
gtags_open5(GTOP *gtop, const char *dbpath, const char *root)
{
	int dbmode;

	gtop->format_version = 4;
	gtop->format = GTAGS_STANDARD;
	if (gtop->db == GRTAGS || gtop->db == GSYMS)
		gtop->format |= GTAGS_COMPACT;
	if (gtop->db == GTAGS && gtop->openflags & GTAGS_COMPRESS)
		gtop->format |= GTAGS_COMPRESS;
	if (gtop->mode == GTAGS_CREATE) {
		char buf[1024];
		/*
		 * Don't write format information in the tag file.
		 * dbop_put(gtop->dbop, COMPACTKEY, COMPACTKEY);
		 */
		snprintf(buf, sizeof(buf), "%s %d", VERSIONKEY, gtop->format_version);
		dbop_put(gtop->dbop, VERSIONKEY, buf);
		if (gtop->format & GTAGS_COMPACT)
			dbop_put(gtop->dbop, COMPACTKEY, COMPACTKEY);
		if (gtop->format & GTAGS_COMPRESS) {
			snprintf(buf, sizeof(buf), "%s %s", COMPRESSKEY, DEFAULT_ABBREVIATION);
			dbop_put(gtop->dbop, COMPRESSKEY, buf);
			abbrev_open(DEFAULT_ABBREVIATION);
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
		else if (gtop->format_version < support_version)
			die("GTAGS seems older format. Please remake tag files.");
		if (dbop_get(gtop->dbop, COMPACTKEY) != NULL)
			gtop->format |= GTAGS_COMPACT;
		if ((p = dbop_get(gtop->dbop, COMPRESSKEY)) != NULL) {
			for (p += strlen(COMPRESSKEY); *p && isspace((unsigned char)*p); p++)
				;
			abbrev_open(p);
			gtop->format |= GTAGS_COMPRESS;
		}
	}
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
	if (gpath_open(dbpath, dbmode) < 0) {
		if (dbmode == 1)
			die("cannot create GPATH.");
		else
			die("GPATH not found.");
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
	} else if (gtop->mode != GTAGS_READ)
		gtop->sb = strbuf_open(0);
	return gtop;
}
/*
 * gtags_put5: put tag record with packing.
 *
 *	i)	gtop	descripter of GTOP
 *	i)	tag	tag name
 *	i)	ctags_x	ctags -x image
 *
 * NOTE: If format is GTAGS_COMPACT or GTAGS_PATHINDEX
 *       then this function is destructive.
 */
void
gtags_put5(GTOP *gtop, const char *tag, const char *ctags_x)	/* virtually const */
{
	SPLIT ptable;

	if (split((char *)ctags_x, 4, &ptable) != 4) {
		recover(&ptable);
		die("illegal tag format.\n'%s'", ctags_x);
	}
	if (gtop->format & GTAGS_COMPACT) {
		int *lno;
		struct sh_entry *entry;
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
	} else {
		const char *s_fid = gpath_path2fid(ptable.part[PART_PATH].start, NULL);

		strbuf_reset(gtop->sb);
		strbuf_puts(gtop->sb, s_fid);
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, ptable.part[PART_TAG].start);
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, ptable.part[PART_LNO].start);
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, gtop->format & GTAGS_COMPRESS ?
			compress(ptable.part[PART_LINE].start, ptable.part[PART_TAG].start) :
			ptable.part[PART_LINE].start);
		dbop_put(gtop->dbop, tag, strbuf_value(gtop->sb));
	}
	recover(&ptable);
}
/*
 * gtags_delete5: delete records belong to set of fid.
 *
 *	i)	gtop	GTOP structure
 *	i)	deleteset bit array of fid
 */
void
gtags_delete5(GTOP *gtop, IDSET *deleteset)
{
	const char *tagline;
	int fid;

	for (tagline = dbop_first(gtop->dbop, NULL, NULL, 0); tagline; tagline = dbop_next(gtop->dbop)) {
		/*
		 * Extract path from the tag line.
		 */
		fid = atoi(tagline);
		/*
		 * If the file id exists in the deleteset, delete the tagline.
		 */
		if (idset_contains(deleteset, fid))
			dbop_delete(gtop->dbop, NULL);
	}
}
/*
 * gtags_first5: return first record
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
gtags_first5(GTOP *gtop, const char *pattern, int flags)
{
	int dbflags = 0;
	int regflags = 0;
	char prefix[IDENTLEN+1];
	static regex_t reg;
	regex_t *preg = &reg;
	const char *key = NULL;
	const char *tagline;

	gtop->flags = flags;
	if (flags & GTOP_PREFIX && pattern != NULL)
		dbflags |= DBOP_PREFIX;
	if (flags & GTOP_KEY)
		dbflags |= DBOP_KEY;

	if (!(flags & GTOP_BASICREGEX))
		regflags |= REG_EXTENDED;
	if (flags & GTOP_IGNORECASE)
		regflags |= REG_ICASE;
	if (gtop->format & GTAGS_COMPACT)
		gtop->fp = NULL;
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
	/*
	 * Read first record.
	 */
	tagline = dbop_first(gtop->dbop, key, preg, dbflags);
	if (tagline == NULL || gtop->flags & GTOP_KEY)
		return tagline;
	return genrecord(gtop, tagline);
}
/*
 * gtags_next5: return followed record
 *
 *	i)	gtop	GTOP structure
 *	r)		record
 *			NULL end of tag
 */
const char *
gtags_next5(GTOP *gtop)
{
	const char *tagline;

	if (gtop->format & GTAGS_COMPACT && gtop->lnop != NULL)
		return genrecord_compact(gtop);
	tagline = dbop_next(gtop->dbop);
	if (tagline == NULL || gtop->flags & GTOP_KEY)
		return tagline;
	return genrecord(gtop, tagline);
}
/*
 * gtags_close5: close tag file
 *
 *	i)	gtop	GTOP structure
 */
void
gtags_close5(GTOP *gtop)
{
	if (gtop->format & GTAGS_COMPACT && gtop->fp != NULL)
		fclose(gtop->fp);
	if (gtop->format & GTAGS_COMPRESS)
		abbrev_close();
	if (gtop->pool) {
		if (gtop->prev_path[0])
			flush_pool(gtop);
		strhash_close(gtop->pool);
	}
	if (gtop->sb)
		strbuf_close(gtop->sb);
	if (gtop->ib)
		strbuf_close(gtop->ib);
	gpath_close();
	dbop_close(gtop->dbop);
	free(gtop);
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
	const char *s_fid = gpath_path2fid(gtop->prev_path, NULL);

	if (s_fid == NULL)
		die("GPATH is corrupted.('%s' not found)", gtop->prev_path);
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
		strbuf_puts(gtop->sb, s_fid);
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, entry->name);
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
static char output[MAXBUFLEN+1];
/*
 * genrecord: generate tag record
 *
 *	i)	gtop	GTOP structure
 *	i)	tagline	tag line
 *	r)		tag record
 */
static const char *
genrecord(GTOP *gtop, const char *tagline)
{
	SPLIT ptable;
	const char *path;

	/*
	 * Extract tag name and the path.
	 *
	 * Standard5 format
	 * PART_FID5 PART_TAG5 PART_LNO5 PART_LINE5
	 * +----------------------------------------------------
	 * |100 main 227 main(int argc, argv **char)
	 *
	 * Compact5 format
	 * PART_FID5 PART_TAG5 PART_LNO5
	 * +----------------------------------------------------
	 * |100 main 227,230,245,260
	 */
	split((char *)tagline, 3, &ptable);
	if (ptable.npart < 3) {
		recover(&ptable);
		die("illegal tag format.'%s'\n", tagline);
	}
	/* Convert from file id into the path name. */
	path = gpath_fid2path(ptable.part[PART_FID5].start, NULL);
	if (path == NULL)
		die("GPATH is corrupted.(file id '%s' not found)", ptable.part[PART_FID5].start);
	/*
	 * Compact format
	 */
	if (gtop->format & GTAGS_COMPACT) {
		/*
		 * Copy elements.
		 */
		gtop->line = (char *)tagline;
		strlimcpy(gtop->tag, ptable.part[PART_TAG5].start, sizeof(gtop->tag));
		strlimcpy(gtop->path, path, sizeof(gtop->path));
		gtop->lnop = ptable.part[PART_LNO5].start;
		recover(&ptable);
		/*
		 * Open source file.
		 */
		if (!(gtop->flags & GTOP_NOSOURCE)) {
			char srcpath[MAXPATHLEN+1];

			if (gtop->root)
				snprintf(srcpath, sizeof(srcpath),
					"%s/%s", gtop->root, &gtop->path[2]);
			else
				snprintf(srcpath, sizeof(srcpath), "%s", &gtop->path[2]);
			/* close file if already opened. */
			if (gtop->fp != NULL)
				fclose(gtop->fp);
			gtop->fp = fopen(srcpath, "r");
			if (gtop->fp == NULL)
				warning("source file '%s' is not available.\n", srcpath);
			gtop->lno = 0;
		}
		return genrecord_compact(gtop);
	}
	/*
	 * Standard format
	 */
	else {
		const char *src;

		/*
		 * Seek to the start point of source line.
		 */
		for (src = ptable.part[PART_LNO5].start; *src && !isspace(*src); src++)
			;
		if (*src != ' ')
			die("illegal standard format.");
		src++;
		snprintf(output, sizeof(output), "%-16s %4d %-16s %s",
			ptable.part[PART_TAG5].start,
			atoi(ptable.part[PART_LNO5].start),
			path,
			gtop->format & GTAGS_COMPRESS ?
				uncompress(src, ptable.part[PART_TAG5].start) :
				src);
		recover(&ptable);
		return output;
	}
}
/*
 * genrecord_compact: generate tag record from compact format
 *
 *	i)	gtop	GTOP structure
 *	r)		tag record
 */
static const char *
genrecord_compact(GTOP *gtop)
{
	const char *lnop = gtop->lnop;
	const char *src = "";
	int lno = 0;

	/* Sanity check */
	if (lnop == NULL || *lnop < '0' || *lnop > '9')
		die("illegal compact format.");
	/* get line number */
	for (; *lnop >= '0' && *lnop <= '9'; lnop++)
		lno = lno * 10 + *lnop - '0';
	/*
	 * The ',' is a mark that the following line number exists.
	 */
	gtop->lnop = (*lnop == ',') ? lnop + 1 : NULL;
	if (gtop->fp) {
		/*
		 * If it is duplicate line, return the previous line.
		 */
		if (gtop->lno == lno)
			return output;
		while (gtop->lno < lno) {
			if (!(src = strbuf_fgets(gtop->ib, gtop->fp, STRBUF_NOCRLF)))
				die("unexpected end of file. '%s'", gtop->path);
			gtop->lno++;
		}
	}
	snprintf(output, sizeof(output), "%-16s %4d %-16s %s",
		gtop->tag, lno, gtop->path, src);
	return output;
}
