/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2005, 2006,
 *	2007, 2009, 2010, 2013, 2014
 *	Tama Communications Corporation
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
#include "checkalloc.h"
#include "conf.h"
#include "compress.h"
#include "dbop.h"
#include "die.h"
#include "format.h"
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

#define HASHBUCKETS	2048

static int compare_path(const void *, const void *);
static int compare_lineno(const void *, const void *);
static int compare_tags(const void *, const void *);
static const char *seekto(const char *, int);
static int is_defined_in_GTAGS(GTOP *, const char *);
static char *get_prefix(const char *, int);
static int gtags_restart(GTOP *);
static void flush_pool(GTOP *, const char *);
static void segment_read(GTOP *);

/**
 * compare_path: compare function for sorting path names.
 */
static int
compare_path(const void *s1, const void *s2)
{
	return strcmp(*(char **)s1, *(char **)s2);
}
/**
 * compare_lineno: compare function for sorting line number.
 */
static int
compare_lineno(const void *s1, const void *s2)
{
	return *(const int *)s1 - *(const int *)s2;
}
/**
 * compare_tags: compare function for sorting tags.
 */
static int
compare_tags(const void *v1, const void *v2)
{
	const GTP *e1 = v1, *e2 = v2;
	int ret;

	if ((ret = strcmp(e1->path, e2->path)) != 0)
		return ret;
	return e1->lineno - e2->lineno;
}
/**
 * @fn static const char *seekto(const char *string, int n)
 * seekto: seek to the specified item of tag record.
 *
 * @par Usage:
 * @code
 *           0         1          2
 * tagline = <file id> <tag name> <line number>
 *
 * <file id>     = seekto(tagline, SEEKTO_FILEID);
 * <tag name>    = seekto(tagline, SEEKTO_TAGNAME);
 * <line number> = seekto(tagline, SEEKTO_LINENO);
 * @endcode
 */
#define SEEKTO_FILEID	0
#define SEEKTO_TAGNAME	1
#define SEEKTO_LINENO	2

static const char *
seekto(const char *string, int n)
{
	const char *p = string;
	while (n--) {
		p = strchr(p, ' ');
		if (p == NULL)
			return NULL;
		p++;
	}
	return p;
}
/**
 * Tag format
 *
 * [@EMPH{Specification of format version 6}]
 * 
 * @par Standard format:
 *
 *	This format is the default format of #GTAGS.
 * 
 * @par
 * @code{.txt}
 *         <file id> <tag name> <line number> <line image>
 * @endcode
 * 
 * @par
 *                 * Separator is single blank.
 * 
 * @par
 * @code{.txt}
 *         [example]
 *         +------------------------------------
 *         |110 func 10 int func(int a)
 *         |110 func 30 func(int a1, int a2)
 * @endcode
 * 
 * @par
 *         Line image might be compressed (#GTAGS_COMPRESS). <br>
 *         Tag name might be compressed (#GTAGS_COMPNAME).
 *
 * @par Compact format:
 * 
 * @par
 *	This format is the default format of #GRTAGS. <br>
 *	It is used for #GTAGS with the @OPTION{-c} option.
 *
 * @par
 * @code{.txt}
 *         <file id> <tag name> <line number>,...
 * @endcode
 * 
 * @par
 *                 * Separator is single blank.
 * 
 * @par
 * @code{.txt}
 *         [example]
 *         +------------------------------------
 *         |110 func 10,30
 * @endcode
 * 
 * @par
 *         Line numbers are sorted in a line. <br>
 *	   Each line number might be expressed as difference from the previous
 *	   line number except for the head (#GTAGS_COMPLINE). <br>
 *           ex: 10,3,2 means '10 13 15'. <br>
 *	   In addition,successive line numbers are expressed as a range. <br>
 *           ex: 10-3 means '10 11 12 13'.
 *
 * @par [Description]
 * 
 * - Standard format is applied to #GTAGS, and compact format is applied
 *     to #GRTAGS by default.
 * - #GSYMS is not used any longer. It is virtually included by GRTAGS.
 * - Above two formats are same to the first line number. So, we can use
 *     common function to sort them.
 * - Separator is single blank.
 *     This decrease disk space used a little, and make it easy to parse
 *     tag record.
 * - Use file id instead of path name.
 *     This allows blanks in path name at least in tag files.
 * - Put file id at the head of tag record.
 *     We can access file id without string processing.
 *     This is advantageous for deleting tag record when incremental updating.
 * 
 * @par [Concept of format version]
 *
 * Since @NAME{GLOBAL}'s tag files are machine independent, they can be distributed
 * apart from @NAME{GLOBAL} itself. For example, if some network file system available,
 * client may execute global using server's tag files. In this case, both
 * @NAME{GLOBAL} are not necessarily the same version. So, we should assume that
 * older version of @NAME{GLOBAL} might access the tag files which generated
 * by new @NAME{GLOBAL}. To deal in such case, we decided to buried a version number
 * to both @XREF{global,1} and tag files. The conclete procedure is like follows:
 *
 * @par
 * -# @XREF{gtags,1} bury the version number in tag files. <br>
 * -# @XREF{global,1} pick up the version number from a tag file. If the number
 *      is larger than its acceptable version number then global give up work
 *      any more and display error message. <br>
 * -# If version number is not found then it assumes version 1.
 *
 * @par [History of format version]
 *
 @verbatim
  GLOBAL-1.0 - 1.8     no idea about format version.
  GLOBAL-1.9 - 2.24    understand format version.
                       support format version 1 (default).
                       if (format > 1) then print error message.
  GLOBAL-3.0 - 4.5     support format version 1 and 2.
                       if (format > 2) then print error message.
  GLOBAL-4.5.1 - 4.8.7 support format version 1, 2 and 3.
                       if (format > 3) then print error message.
  GLOBAL-5.0 -	5.3	support format version only 4.
                       if (format !=  4) then print error message.
  GLOBAL-5.4 - 5.8.2	support format version 4 and 5
                       if (format > 5 || format < 4) then print error message.
  GLOBAL-5.9 -		support only format version 6
                       if (format > 6 || format < 6) then print error message.
 @endverbatim
 *
 * In @NAME{GLOBAL-5.0}, we threw away the compatibility with the past formats.
 * Though we could continue the support for older formats, it seemed
 * not to be worthy. Because keeping maintaining the all formats hinders
 * new optimization and the function addition in the future.
 * Instead, the following error messages are displayed in a wrong usage.
 * @code{.sh}
 *       [older global and new tag file]
 *       $ global -x main
 *       GTAGS seems new format. Please install the latest GLOBAL.
 *       [new global and older tag file]
 *       $ global -x main
 *       GTAGS seems older format. Please remake tag files.
 * @endcode
 */
static int new_format_version = 6;	/**< new format version */
static int upper_bound_version = 6;	/**< acceptable format version (upper bound) */
static int lower_bound_version = 6;	/**< acceptable format version (lower bound) */
static const char *const tagslist[] = {"GPATH", "GTAGS", "GRTAGS", "GSYMS"};
/**
 * Virtual #GRTAGS, #GSYMS processing:
 *
 * We use a real @NAME{GRTAGS} as virtual @NAME{GRTAGS} and @NAME{GSYMS}.
 * In fact, @NAME{GSYMS} tag file doesn't exist.
 *
 * @code{.txt}
 * Real tag file	virtual tag file
 * --------------------------------------
 * GTAGS =============> GTAGS
 *
 * GRTAGS ============> GRTAGS + GSYMS
 *            +=======> GRTAGS	tags which is defined in GTAGS
 *            +=======> GSYMS	tags which is not defined in GTAGS
 * @endcode
 */
#define VIRTUAL_GRTAGS_GSYMS_PROCESSING(gtop) 						\
	if (gtop->db == GRTAGS || gtop->db == GSYMS) {					\
		int defined = is_defined_in_GTAGS(gtop, gtop->dbop->lastkey);		\
		if ((gtop->db == GRTAGS && !defined) || (gtop->db == GSYMS && defined))	\
			continue;							\
	}
/**
 * is_defined_in_GTAGS: whether or not the name is defined in #GTAGS.
 *
 *	@param[in]	gtop
 *	@param[in]	name	tag name
 *	@return		0: not defined, 1: defined
 *
 * @note It is assumed that the input stream is sorted by the tag name.
 */
static int
is_defined_in_GTAGS(GTOP *gtop, const char *name)
{
	static char prev_name[MAXTOKEN+1];
	static int prev_result;

	if (!strcmp(name, prev_name))
		return prev_result;
	strlimcpy(prev_name, name, sizeof(prev_name));
	return prev_result = dbop_get(gtop->gtags, prev_name) ? 1 : 0;
}
/**
 * dbname: return db name
 *
 *	@param[in]	db	0: #GPATH, 1: #GTAGS, 2: #GRTAGS, 3: #GSYMS
 *	@return		dbname
 */
const char *
dbname(int db)
{
	if (db == GRTAGS + GSYMS)
		db = GRTAGS;
	assert(db >= 0 && db < GTAGLIM);
	return tagslist[db];
}
/**
 * gtags_open: open global tag.
 *
 *	@param[in]	dbpath	dbpath directory
 *	@param[in]	root	root directory (needed when compact format)
 *	@param[in]	db	#GTAGS, #GRTAGS, #GSYMS
 *	@param[in]	mode	#GTAGS_READ: read only <br>
 *			#GTAGS_CREATE: create tag <br>
 *			#GTAGS_MODIFY: modify tag
 *	@param[in]	flags	#GTAGS_COMPACT: compact format
 *	@return		#GTOP structure
 *
 * @note when error occurred, @NAME{gtags_open()} doesn't return.
 */
GTOP *
gtags_open(const char *dbpath, const char *root, int db, int mode, int flags)
{
	GTOP *gtop;
	char tagfile[MAXPATHLEN];
	int dbmode;

	gtop = (GTOP *)check_calloc(sizeof(GTOP), 1);
	gtop->db = db;
	gtop->mode = mode;
	gtop->openflags = flags;
	/*
	 * Open tag file allowing duplicate records.
	 */
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
	 * GRTAGS and GSYMS are virtual tag file. They are included in a real GRTAGS file.
	 * In fact, GSYMS doesn't exist now.
	 *
	 * GRTAGS:	tags which belongs to GRTAGS, and are defined in GTAGS.
	 * GSYMS:	tags which belongs to GRTAGS, and is not defined in GTAGS.
	 */
	strlimcpy(tagfile, makepath(dbpath, dbname(db == GSYMS ? GRTAGS : db), NULL), sizeof(tagfile));
	gtop->dbop = dbop_open(tagfile, dbmode, 0644, DBOP_DUP|DBOP_SORTED_WRITE);
	if (gtop->dbop == NULL) {
		if (dbmode == 1)
			die("cannot make %s.", dbname(db));
		die("%s not found.", dbname(db));
	}
	if (gtop->mode == GTAGS_READ && db != GTAGS) {
		const char *gtags = makepath(dbpath, dbname(GTAGS), NULL);
		int format_version;

		gtop->gtags = dbop_open(gtags, 0, 0, 0);
		if (gtop->gtags == NULL)
			die("GTAGS not found.");
		format_version = dbop_getversion(gtop->dbop);
		if (format_version > upper_bound_version)
			die("%s seems new format. Please install the latest GLOBAL.", gtags);
		else if (format_version < lower_bound_version)
			die("%s seems older format. Please remake tag files.", gtags);
	}
	if (gtop->mode == GTAGS_CREATE) {
		/*
		 * Decide format.
		 */
		gtop->format = 0;
		gtop->format_version = new_format_version;
		/*
		 * GRTAGS and GSYSM always use compact format.
		 * GTAGS uses compact format only when the -c option specified.
		 */
		if (gtop->db == GRTAGS || gtop->db == GSYMS || gtop->openflags & GTAGS_COMPACT) {
			gtop->format |= GTAGS_COMPACT;
			gtop->format |= GTAGS_COMPLINE;
		} else {
			/* standard format */
			gtop->format |= GTAGS_COMPRESS;
		}
		gtop->format |= GTAGS_COMPNAME;
		if (gtop->format & GTAGS_COMPACT)
			dbop_putoption(gtop->dbop, COMPACTKEY, NULL);
		if (gtop->format & GTAGS_COMPRESS) {
			dbop_putoption(gtop->dbop, COMPRESSKEY, DEFAULT_ABBREVIATION);
			abbrev_open(DEFAULT_ABBREVIATION);
		}
		if (gtop->format & GTAGS_COMPLINE)
			dbop_putoption(gtop->dbop, COMPLINEKEY, NULL);
		if (gtop->format & GTAGS_COMPNAME)
			dbop_putoption(gtop->dbop, COMPNAMEKEY, NULL);
		dbop_putversion(gtop->dbop, gtop->format_version); 
	} else {
		/*
		 * recognize format version of GTAGS. 'format version record'
		 * is saved as a META record in GTAGS and GRTAGS.
		 * if 'format version record' is not found, it's assumed
		 * version 1.
		 */
		const char *p;
		/*
		 * check format version.
		 */
		gtop->format_version = dbop_getversion(gtop->dbop);
		if (gtop->format_version > upper_bound_version)
			die("%s seems new format. Please install the latest GLOBAL.", tagfile);
		else if (gtop->format_version < lower_bound_version)
			die("%s seems older format. Please remake tag files.", tagfile);
		gtop->format = 0;
		if (dbop_getoption(gtop->dbop, COMPACTKEY) != NULL)
			gtop->format |= GTAGS_COMPACT;
		if ((p = dbop_getoption(gtop->dbop, COMPRESSKEY)) != NULL) {
			abbrev_open(p);
			gtop->format |= GTAGS_COMPRESS;
		}
		if (dbop_getoption(gtop->dbop, COMPLINEKEY) != NULL)
			gtop->format |= GTAGS_COMPLINE;
		if (dbop_getoption(gtop->dbop, COMPNAMEKEY) != NULL)
			gtop->format |= GTAGS_COMPNAME;
	}
	if (gpath_open(dbpath, dbmode) < 0) {
		if (dbmode == 1)
			die("cannot create GPATH.");
		else
			die("GPATH not found.");
	}
	if (gtop->mode != GTAGS_READ)
		gtop->sb = strbuf_open(0);	/* This buffer is used for working area. */
	/*
	 * Stuff for compact format.
	 */
	if (gtop->format & GTAGS_COMPACT) {
		assert(root != NULL);
		strlimcpy(gtop->root, root, sizeof(gtop->root));
		if (gtop->mode != GTAGS_READ)
			gtop->path_hash = strhash_open(HASHBUCKETS);
	}
	return gtop;
}
/**
 * gtags_put_using: put tag record with packing.
 *
 *	@param[in]	gtop	descripter of #GTOP
 *	@param[in]	tag	tag name
 *	@param[in]	lno	line number
 *	@param[in]	fid	file id
 *	@param[in]	img	line image
 */
void
gtags_put_using(GTOP *gtop, const char *tag, int lno, const char *fid, const char *img)
{
	const char *key;

	if (gtop->format & GTAGS_COMPACT) {
		struct sh_entry *entry;

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
		entry = strhash_assign(gtop->path_hash, tag, 1);
		if (entry->value == NULL)
			entry->value = varray_open(sizeof(int), 100);
		*(int *)varray_append((VARRAY *)entry->value) = lno;
		return;
	}
	/*
	 * extract method when class method definition.
	 *
	 * Ex: Class::method(...)
	 *
	 * key	= 'method'
	 * data = 'Class::method  103 ./class.cpp ...'
	 */
	if (gtop->flags & GTAGS_EXTRACTMETHOD) {
		if ((key = locatestring(tag, ".", MATCH_LAST)) != NULL)
			key++;
		else if ((key = locatestring(tag, "::", MATCH_LAST)) != NULL)
			key += 2;
		else
			key = tag;
	} else {
		key = tag;
	}
	strbuf_reset(gtop->sb);
	strbuf_puts(gtop->sb, fid);
	strbuf_putc(gtop->sb, ' ');
	strbuf_puts(gtop->sb, (gtop->format & GTAGS_COMPNAME) ? compress(tag, key) : tag);
	strbuf_putc(gtop->sb, ' ');
	strbuf_putn(gtop->sb, lno);
	strbuf_putc(gtop->sb, ' ');
	strbuf_puts(gtop->sb, (gtop->format & GTAGS_COMPRESS) ? compress(img, key) : img);
	dbop_put(gtop->dbop, key, strbuf_value(gtop->sb));
}
/**
 * gtags_flush: Flush the pool for compact format.
 *
 *	@param[in]	gtop	descripter of #GTOP
 *	@param[in]	fid	file id
 */
void
gtags_flush(GTOP *gtop, const char *fid)
{
	if (gtop->format & GTAGS_COMPACT) {
		flush_pool(gtop, fid);
		strhash_reset(gtop->path_hash);
	}
}
/**
 * gtags_delete: delete records belong to set of fid.
 *
 *	@param[in]	gtop	#GTOP structure
 *	@param[in]	deleteset bit array of fid
 */
void
gtags_delete(GTOP *gtop, IDSET *deleteset)
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
/**
 * get_prefix: get as long prefix of the pattern as possible.
 *
 *	@param[in]	pattern
 *	@param[in]	flags for gtags_first()
 *	@return		prefix for search
 */
static char *
get_prefix(const char *pattern, int flags)
{
	static char buffer[IDENTLEN];
	char *prefix = buffer;

	if (pattern == NULL || pattern[0] == 0) {
		prefix = NULL;
	} else if (!isregex(pattern)) {
		if (flags & GTOP_IGNORECASE) {
			buffer[0] = toupper(*pattern);
			buffer[1] = 0;
		} else {
			prefix = NULL;
		}
	} else if (*pattern == '^') {
		int save = 0;
		char *p = (char *)(pattern + 1);
		char *q = locatestring(p, ".*$", MATCH_AT_LAST);

		if (!q)
			q = locatestring(pattern, "$", MATCH_AT_LAST);
		if (!q)
			q = locatestring(pattern, ".*", MATCH_AT_LAST);
		if (q) {
			save = *q;
			*q = 0;
		}
		if (*p == 0 || isregex(p)) {
			prefix = NULL;
		} else {
			if (flags & GTOP_IGNORECASE) {
				prefix[0] = toupper(*p);
				prefix[1] = 0;
			} else
				strlimcpy(buffer, p, sizeof(buffer));
		}
		if (save)
			*q = save;
	}
	return prefix;;
}
/**
 * gtags_restart: restart dbop iterator using lower case prefix.
 *
 *	@param[in]	gtop	#GTOP structure
 *	@return		prepared or not
 *			#0:	cannot continue
 *			#1:	can continue
 */
static int
gtags_restart(GTOP *gtop)
{
	int upper, lower;

	if (gtop->prefix == NULL)
		die("gtags_restart: impossible.");
	upper = gtop->prefix[0];
	lower = tolower(upper);
	if (upper < lower) {
		gtop->prefix[0] = lower;
		gtop->key = gtop->prefix;
		gtop->prefix = NULL;
		if (gtop->openflags & GTAGS_DEBUG)
			fprintf(stderr, "Using prefix: %s\n", gtop->key);
		return 1;
	}
	if (gtop->openflags & GTAGS_DEBUG)
		fprintf(stderr, "gtags_restart: not prepared.\n");
	return 0;
}
/**
 * gtags_first: return first record
 *
 *	@param[in]	gtop	#GTOP structure
 *	@param[in]	pattern	tag name <br>
 *		- may be regular expression
 *		- may be @VAR{NULL}
 *	@param[in]	flags	#GTOP_PREFIX:	prefix read <br>
 *			#GTOP_KEY:	read key only <br>
 *			#GTOP_PATH:	read path only <br>
 *			#GTOP_NOREGEX:	don't use regular expression. <br>
 *			#GTOP_IGNORECASE:	ignore case distinction. <br>
 *			#GTOP_BASICREGEX:	use basic regular expression. <br>
 *			#GTOP_NOSORT:	don't sort
 *	@return		record
 */
GTP *
gtags_first(GTOP *gtop, const char *pattern, int flags)
{
	int regflags = 0;
	static regex_t reg;
	const char *tagline;
	STATIC_STRBUF(regex);

	strbuf_clear(regex);
	gtop->preg = &reg;
	gtop->key = NULL;
	gtop->prefix = NULL;
	gtop->flags = flags;
	gtop->dbflags = 0;
	gtop->readcount = 1;

	/* Settlement for last time if any */
	if (gtop->path_hash) {
		strhash_close(gtop->path_hash);
		gtop->path_hash = NULL;
	}
	if (gtop->path_array) {
		free(gtop->path_array);
		gtop->path_array = NULL;
	}

	if (flags & GTOP_KEY)
		gtop->dbflags |= DBOP_KEY;
	if (!(flags & GTOP_BASICREGEX))
		regflags |= REG_EXTENDED;

	/*
	 * decide a read method
	 */
	if (pattern == NULL)
		gtop->preg = NULL;
	else if (pattern[0] == 0)
		return NULL;
	else if (!strcmp(pattern, ".*") || !strcmp(pattern, "^.*$") ||
		!strcmp(pattern, "^") || !strcmp(pattern, "$") ||
		!strcmp(pattern, "^.*") || !strcmp(pattern, ".*$")) {
		/*
		 * Since these regular expressions match to any record,
		 * we take sequential read method.
		 */
		gtop->preg = NULL;
	} else if (flags & GTOP_IGNORECASE) {
		regflags |= REG_ICASE;
		if (!isregex(pattern) || flags & GTOP_NOREGEX) {
			gtop->prefix = get_prefix(pattern, flags);
			if (gtop->openflags & GTAGS_DEBUG)
				if (gtop->prefix != NULL)
					fprintf(stderr, "Using prefix: %s\n", gtop->prefix);
			if (gtop->prefix == NULL)
				die("gtags_first: impossible (1).");
			strbuf_putc(regex, '^');
			strbuf_puts(regex, pattern);
			if (!(flags & GTOP_PREFIX))
				strbuf_putc(regex, '$');
		} else if (*pattern == '^' && (gtop->prefix = get_prefix(pattern, flags)) != NULL) {
			if (gtop->openflags & GTAGS_DEBUG)
				fprintf(stderr, "Using prefix: %s\n", gtop->prefix);
			strbuf_puts(regex, pattern);
		} else {
			strbuf_puts(regex, pattern);
		}
	} else {
		if (!isregex(pattern) || flags & GTOP_NOREGEX) {
			if (flags & GTOP_PREFIX)
				gtop->dbflags |= DBOP_PREFIX;
			gtop->key = pattern;
			gtop->preg = NULL;
		} else if (*pattern == '^' && (gtop->key = get_prefix(pattern, flags)) != NULL) {
			if (gtop->openflags & GTAGS_DEBUG)
				fprintf(stderr, "Using prefix: %s\n", gtop->key);
			gtop->dbflags |= DBOP_PREFIX;
			gtop->preg = NULL;
		} else {
			strbuf_puts(regex, pattern);
		}
	}
	if (gtop->prefix) {
		if (gtop->key)
			die("gtags_first: impossible (2).");
		gtop->key = gtop->prefix;
		gtop->dbflags |= DBOP_PREFIX;
	}
	if (strbuf_getlen(regex) > 0) {
		if (gtop->preg == NULL)
			die("gtags_first: impossible (3).");
		if (regcomp(gtop->preg, strbuf_value(regex), regflags) != 0)
			die("illegal regular expression.");
	}
	/*
	 * If GTOP_PATH is set, at first, we collect all path names in a pool and
	 * sort them. gtags_first() and gtags_next() returns one of the pool.
	 */
	if (gtop->flags & GTOP_PATH) {
		struct sh_entry *entry;
		char *p;
		const char *cp;
		unsigned long i;

		gtop->path_hash = strhash_open(HASHBUCKETS);
		/*
		 * Pool path names.
		 *
		 * fid		path name
		 * +--------------------------
		 * |100		./aaa/a.c
		 * |105		./aaa/b.c
		 *  ...
		 */
again0:
		for (tagline = dbop_first(gtop->dbop, gtop->key, gtop->preg, gtop->dbflags);
		     tagline != NULL;
		     tagline = dbop_next(gtop->dbop))
		{
			VIRTUAL_GRTAGS_GSYMS_PROCESSING(gtop);
			/* extract file id */
			p = locatestring(tagline, " ", MATCH_FIRST);
			if (p == NULL)
				die("Illegal tag record. '%s'\n", tagline);
			*p = '\0';
			entry = strhash_assign(gtop->path_hash, tagline, 1);
			/* new entry: get path name and set. */
			if (entry->value == NULL) {
				cp = gpath_fid2path(tagline, NULL);
				if (cp == NULL)
					die("GPATH is corrupted.(file id '%s' not found)", tagline);
				entry->value = strhash_strdup(gtop->path_hash, cp, 0);
			}
		}
		if (gtop->prefix && gtags_restart(gtop))
			goto again0;
		/*
		 * Sort path names.
		 *
		 * fid		path name	path_array (sort)
		 * +--------------------------	+---+
		 * |100		./aaa/a.c <-------* |
		 * |105		./aaa/b.c <-------* |
		 *  ...				...
		 */
		gtop->path_array = (char **)check_malloc(gtop->path_hash->entries * sizeof(char *));
		i = 0;
		for (entry = strhash_first(gtop->path_hash); entry != NULL; entry = strhash_next(gtop->path_hash))
			gtop->path_array[i++] = entry->value;
		if (i != gtop->path_hash->entries)
			die("Something is wrong. 'i = %lu, entries = %lu'" , i, gtop->path_hash->entries);
		if (!(gtop->flags & GTOP_NOSORT))
			qsort(gtop->path_array, gtop->path_hash->entries, sizeof(char *), compare_path);
		gtop->path_count = gtop->path_hash->entries;
		gtop->path_index = 0;

		if (gtop->path_index >= gtop->path_count)
			return NULL;
		gtop->gtp.path = gtop->path_array[gtop->path_index++];
		return &gtop->gtp;
	} else if (gtop->flags & GTOP_KEY) {
again1:
		for (gtop->gtp.tag = dbop_first(gtop->dbop, gtop->key, gtop->preg, gtop->dbflags);
		     gtop->gtp.tag != NULL;
		     gtop->gtp.tag = dbop_next(gtop->dbop))
		{
			VIRTUAL_GRTAGS_GSYMS_PROCESSING(gtop);
			break;
		}
		if (gtop->gtp.tag == NULL) {
			if (gtop->prefix && gtags_restart(gtop))
				goto again1;
		}
		return gtop->gtp.tag ? &gtop->gtp : NULL;
	} else {
		if (gtop->vb == NULL)
			gtop->vb = varray_open(sizeof(GTP), 200);
		else
			varray_reset(gtop->vb);
		if (gtop->segment_pool == NULL)
			gtop->segment_pool = pool_open();
		else
			pool_reset(gtop->segment_pool);
		if (gtop->path_hash == NULL)
			gtop->path_hash = strhash_open(HASHBUCKETS);
		else
			strhash_reset(gtop->path_hash);
again2:
		tagline = dbop_first(gtop->dbop, gtop->key, gtop->preg, gtop->dbflags);
		if (tagline == NULL) {
			if (gtop->prefix && gtags_restart(gtop))
				goto again2;
			return NULL;
		}
		/*
		 * Dbop_next() wil read the same record again.
		 */
		dbop_unread(gtop->dbop);
		/*
		 * Read a tag segment with sorting.
		 */
		segment_read(gtop);
		return  &gtop->gtp_array[gtop->gtp_index++];
	}
}
/**
 * gtags_next: return next record.
 *
 *	@param[in]	gtop	#GTOP structure
 *	@return		record
 *			@VAR{NULL} end of tag
 */
GTP *
gtags_next(GTOP *gtop)
{
	gtop->readcount++;
	if (gtop->flags & GTOP_PATH) {
		if (gtop->path_index >= gtop->path_count)
			return NULL;
		gtop->gtp.path = gtop->path_array[gtop->path_index++];
		return &gtop->gtp;
	} else if (gtop->flags & GTOP_KEY) {
		gtop->gtp.tag = dbop_next(gtop->dbop);
again3:
		for (; gtop->gtp.tag != NULL; gtop->gtp.tag = dbop_next(gtop->dbop))
		{
			VIRTUAL_GRTAGS_GSYMS_PROCESSING(gtop);
			break;
		}
		if (gtop->gtp.tag == NULL) {
			if (gtop->prefix && gtags_restart(gtop)) {
				gtop->gtp.tag = dbop_first(gtop->dbop, gtop->key, gtop->preg, gtop->dbflags);
				goto again3;
			}
		}
		return gtop->gtp.tag ? &gtop->gtp : NULL;
	} else {
		/*
		 * End of segment.
		 * Reset resources and read new segment again.
		 */
		if (gtop->gtp_index >= gtop->gtp_count) {
			varray_reset(gtop->vb);
			pool_reset(gtop->segment_pool);
			/* strhash_reset(gtop->path_hash); */
			segment_read(gtop);
		}
		if (gtop->gtp_index >= gtop->gtp_count) {
			if (gtop->prefix && gtags_restart(gtop)) {
				gtop->gtp.tag = dbop_first(gtop->dbop, gtop->key, gtop->preg, gtop->dbflags);
				dbop_unread(gtop->dbop);
				segment_read(gtop);
			} else
				return NULL;
		}
		return &gtop->gtp_array[gtop->gtp_index++];
	}
}
void
gtags_show_statistics(GTOP *gtop)
{
	fprintf(stderr, "Numbers of gtags (%s): %d\n", dbname(gtop->db), gtop->readcount);
	fprintf(stderr, "Numbers of dbop  (%s): %d\n", dbname(gtop->db), gtop->dbop->readcount);
}
/**
 * gtags_close: close tag file
 *
 *	@param[in]	gtop	#GTOP structure
 */
void
gtags_close(GTOP *gtop)
{
	if (gtop->format & GTAGS_COMPRESS)
		abbrev_close();
	if (gtop->segment_pool)
		pool_close(gtop->segment_pool);
	if (gtop->path_array)
		free(gtop->path_array);
	if (gtop->sb)
		strbuf_close(gtop->sb);
	if (gtop->vb)
		varray_close(gtop->vb);
	if (gtop->path_hash)
		strhash_close(gtop->path_hash);
	gpath_close();
	dbop_close(gtop->dbop);
	if (gtop->gtags)
		dbop_close(gtop->gtags);
	free(gtop);
}
/**
 * flush_pool: flush and write the pool as compact format.
 *
 *	@param[in]	gtop	descripter of #GTOP
 *	@param[in]	s_fid
 */
static void
flush_pool(GTOP *gtop, const char *s_fid)
{
	struct sh_entry *entry;
	int header_offset;
	int i, last;

	if (s_fid == NULL)
		die("flush_pool: impossible");
	/*
	 * Write records as compact format and free line number table
	 * for each entry in the pool.
	 */
	for (entry = strhash_first(gtop->path_hash); entry; entry = strhash_next(gtop->path_hash)) {
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
		qsort(lno_array, vb->length, sizeof(int), compare_lineno); 

		strbuf_reset(gtop->sb);
		strbuf_puts(gtop->sb, s_fid);
		strbuf_putc(gtop->sb, ' ');
		if (gtop->format & GTAGS_COMPNAME) {
			strbuf_puts(gtop->sb, compress(entry->name, key));
		} else {
			strbuf_puts(gtop->sb, entry->name);
		}
		strbuf_putc(gtop->sb, ' ');
		header_offset = strbuf_getlen(gtop->sb);
		/*
		 * If GTAGS_COMPLINE flag is set, each line number is expressed as the
		 * difference from the previous line number except for the head.
		 * GTAGS_COMPLINE is set by default in format version 5.
		 */
		if (gtop->format & GTAGS_COMPLINE) {
			int cont = 0;

			last = 0;			/* line 0 doesn't exist */
			for (i = 0; i < vb->length; i++) {
				int n = lno_array[i];

				if (n == last)
					continue;
				if (last > 0 && n == last + 1) {
					if (!cont) {
						/*
						 * Don't use range expression at the head.
						 */
						if (strbuf_getlen(gtop->sb) == header_offset)
							strbuf_putn(gtop->sb, n);
						else
							cont = last;
					}
				} else {
					/*
					 * Range expression. ex: 10-2 means 10 11 12
					 */
					if (cont) {
						strbuf_putc(gtop->sb, '-');
						strbuf_putn(gtop->sb, last - cont);
						cont = 0;
					}
					if (strbuf_getlen(gtop->sb) > header_offset) {
						strbuf_putc(gtop->sb, ',');
						strbuf_putn(gtop->sb, n - last);
					} else {
						strbuf_putn(gtop->sb, n);
					}
					if (strbuf_getlen(gtop->sb) > DBOP_PAGESIZE / 4) {
						dbop_put(gtop->dbop, key, strbuf_value(gtop->sb));
						strbuf_setlen(gtop->sb, header_offset);
					}
				}
				last = n;
			}
			if (cont) {
				strbuf_putc(gtop->sb, '-');
				strbuf_putn(gtop->sb, last - cont);
			}
		} else {
			/*
			 * This code is to support older format (version 4).
			 */
			last = 0;			/* line 0 doesn't exist */
			for (i = 0; i < vb->length; i++) {
				int n = lno_array[i];

				if (n == last)
					continue;
				if (strbuf_getlen(gtop->sb) > header_offset)
					strbuf_putc(gtop->sb, ',');
				strbuf_putn(gtop->sb, n);
				if (strbuf_getlen(gtop->sb) > DBOP_PAGESIZE / 4) {
					dbop_put(gtop->dbop, key, strbuf_value(gtop->sb));
					strbuf_setlen(gtop->sb, header_offset);
				}
				last = n;
			}
		}
		if (strbuf_getlen(gtop->sb) > header_offset) {
			dbop_put(gtop->dbop, key, strbuf_value(gtop->sb));
		}
		/* Free line number table */
		varray_close(vb);
	}
}
/**
 * Read a tag segment with sorting.
 *
 *	@param[in]	gtop	#GTOP structure <br>
 *		Output:	@CODE{gtop->gtp_array}		segment table <br>
 *		Output:	@CODE{gtop->gtp_count}		segment table size <br>
 *		Output:	@CODE{gtop->gtp_index}		segment table index (initial value = 0) <br>
 *		Output:	@CODE{gtop->cur_tagname}	current tag name
 *
 * A segment is a set of tag records which have same tag name. <br>
 * This function read a segment from tag file, sort it and put it on segment table. <br>
 * This function can treat both of standard format and compact format.
 *
 * Sorting is done by three keys.
 *	- 1st key: tag name
 *	- 2nd key: file name
 *	- 3rd key: line number
 *
 * Since all records in a segment have same tag name, you need not think about 1st key.
 */
void
segment_read(GTOP *gtop)
{
	const char *tagline, *fid, *path, *lineno;
	GTP *gtp;
	struct sh_entry *sh;

	/*
	 * Save tag lines.
	 */
	gtop->cur_tagname[0] = '\0';
	while ((tagline = dbop_next(gtop->dbop)) != NULL) {
		VIRTUAL_GRTAGS_GSYMS_PROCESSING(gtop);
		/*
		 * get tag name and line number.
		 *
		 * tagline = <file id> <tag name> <line number>
		 */
		if (gtop->cur_tagname[0] == '\0') {
			strlimcpy(gtop->cur_tagname, gtop->dbop->lastkey, sizeof(gtop->cur_tagname));
		} else if (strcmp(gtop->cur_tagname, gtop->dbop->lastkey) != 0) {
			/*
			 * Dbop_next() wil read the same record again.
			 */
			dbop_unread(gtop->dbop);
			break;
		}
		gtp = varray_append(gtop->vb);
		gtp->tagline = pool_strdup(gtop->segment_pool, tagline, 0);
		gtp->tag = (const char *)gtop->cur_tagname;
		/*
		 * convert fid into hashed path name to save memory.
		 */
		fid = (const char *)strmake(tagline, " ");
		path = gpath_fid2path(fid, NULL);
		if (path == NULL)
			die("gtags_first: path not found. (fid=%s)", fid);
		sh = strhash_assign(gtop->path_hash, path, 1);
		gtp->path = sh->name;
		lineno = seekto(gtp->tagline, SEEKTO_LINENO);
		if (lineno == NULL)
			die("illegal tag record.\n%s", tagline);
		gtp->lineno = atoi(lineno);
	}
	/*
	 * Sort tag lines.
	 */
	gtop->gtp_array = varray_assign(gtop->vb, 0, 0);
	gtop->gtp_count = gtop->vb->length;
	gtop->gtp_index = 0;
	if (!(gtop->flags & GTOP_NOSORT))
		qsort(gtop->gtp_array, gtop->gtp_count, sizeof(GTP), compare_tags);
}
