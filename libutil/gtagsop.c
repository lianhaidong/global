/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000, 2001
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

#include "conf.h"
#include "gparam.h"
#include "dbop.h"
#include "die.h"
#include "gtagsop.h"
#include "locatestring.h"
#include "makepath.h"
#include "path.h"
#include "pathop.h"
#include "strbuf.h"
#include "strmake.h"
#include "tab.h"

static char	*genrecord(GTOP *);
static int	belongto(GTOP *, char *, char *);
static regex_t reg;

/*
 * format version:
 * GLOBAL-1.0 - 1.8	no idea about format version.
 * GLOBAL-1.9 - 2.24 	understand format version.
 *			support format version 1 (default).
 *			if (format version > 1) then print error message.
 * GLOBAL-3.0 - 4.0	understand format version.
 *			support format version 1 and 2.
 *			if (format version > 2) then print error message.
 * format version 1:
 *	original format.
 * format version 2:
 *	compact format, path index.
 */
static int	support_version = 2;	/* acceptable format version   */
static const char *tagslist[] = {"GPATH", "GTAGS", "GRTAGS", "GSYMS"};
static int init;
static char regexchar[256];
/*
 * dbname: return db name
 *
 *	i)	db	0: GPATH, 1: GTAGS, 2: GRTAGS, 3: GSYMS
 *	r)		dbname
 */
const char *
dbname(db)
int	db;
{
	assert(db >= 0 && db < GTAGLIM);
	return tagslist[db];
}
/*
 * makecommand: make command line to make global tag file
 *
 *	i)	comline	skelton command line
 *	i)	path	path name
 *	o)	sb	command line
 *
 * command skelton is like this:
 *	'gctags -r %s'
 * following skelton is allowed too.
 *	'gctags -r'
 */
void
makecommand(comline, path, sb)
char	*comline;
char	*path;
STRBUF	*sb;
{
	char	*p = locatestring(comline, "%s", MATCH_FIRST);

	if (p) {
		strbuf_nputs(sb, comline, p - comline);
		strbuf_puts(sb, path);
		strbuf_puts(sb, p + 2);
	} else {
		strbuf_puts(sb, comline);
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, path);
	}
}
/*
 * isregex: test whether or not regular expression
 *
 *	i)	s	string
 *	r)		1: is regex, 0: not regex
 */
int
isregex(s)
char	*s;
{
	int	c;

	while ((c = *s++) != '\0')
		if (isregexchar(c))
			return 1;
	return 0;
}
/*
 * formatcheck: check format of tag command's output
 *
 *	i)	line	input
 *	i)	flags	flag
 *	r)	0:	normal
 *		-1:	tag name
 *		-2:	line number
 *		-3:	path
 *
 * [example of right format]
 *
 * $1                $2 $3             $4
 * ----------------------------------------------------
 * main              83 ./ctags.c        main(argc, argv)
 */
int
formatcheck(line, flags)
char	*line;
int	flags;
{
	char	*p, *q;
	/*
	 * $1 = tagname: allowed any char except sepalator.
	 */
	p = q = line;
	while (*p && !isspace(*p))
		p++;
	while (*p && isspace(*p))
		p++;
	if (p == q)
		return -1;
	/*
	 * $2 = line number: must be digit.
	 */
	q = p;
	while (*p && !isspace(*p))
		if (!isdigit(*p))
			return -2;
		else
			p++;
	if (p == q)
		return -2;
	while (*p && isspace(*p))
		p++;
	/*
	 * $3 = path:
	 *	standard format: must start with './'.
	 *	compact format: must be digit.
	 */
	if (flags & GTAGS_PATHINDEX) {
		while (*p && !isspace(*p))
			if (!isdigit(*p))
				return -3;
			else
				p++;
	} else {
		if (!(*p == '.' && *(p + 1) == '/' && *(p + 2)))
			return -3;
	}
	return 0;
}
/*
 * gtagsopen: open global tag.
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
GTOP	*
gtagsopen(dbpath, root, db, mode, flags)
char	*dbpath;
char	*root;
int	db;
int	mode;
int	flags;
{
	GTOP	*gtop;
	int	dbmode = 0;

	/* initialize for isregex() */
	if (!init) {
		regexchar['^'] = regexchar['$'] = regexchar['{'] =
		regexchar['}'] = regexchar['('] = regexchar[')'] =
		regexchar['.'] = regexchar['*'] = regexchar['+'] =
		regexchar['?'] = regexchar['\\'] = init = 1;
	}
	if ((gtop = (GTOP *)calloc(sizeof(GTOP), 1)) == NULL)
		die("short of memory.");
	gtop->db = db;
	gtop->mode = mode;
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
			char	buf[80];

			gtop->format_version = 2;
#ifdef HAVE_SNPRINTF
			snprintf(buf, sizeof(buf), "%s %d", VERSIONKEY, gtop->format_version);
#else
			sprintf(buf, "%s %d", VERSIONKEY, gtop->format_version);
#endif /* HAVE_SNPRINTF */
			dbop_put(gtop->dbop, VERSIONKEY, buf);
			gtop->format |= GTAGS_COMPACT;
			dbop_put(gtop->dbop, COMPACTKEY, COMPACTKEY);
			if (flags & GTAGS_PATHINDEX) {
				gtop->format |= GTAGS_PATHINDEX;
				dbop_put(gtop->dbop, PATHINDEXKEY, PATHINDEXKEY);
			}
		}
	} else {
		/*
		 * recognize format version of GTAGS. 'format version record'
		 * is saved as a META record in GTAGS and GRTAGS.
		 * if 'format version record' is not found, it's assumed
		 * version 1.
		 */
		char	*p;

		if ((p = dbop_get(gtop->dbop, VERSIONKEY)) != NULL) {
			for (p += strlen(VERSIONKEY); *p && isspace(*p); p++)
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
		if (pathopen(dbpath, dbmode) < 0) {
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
		strcpy(gtop->root, root);
		if (gtop->mode == GTAGS_READ)
			gtop->ib = strbuf_open(MAXBUFLEN);
		else
			gtop->sb = strbuf_open(0);
	}
	return gtop;
}
/*
 * gtagsput: put tag record with packing.
 *
 *	i)	gtop	descripter of GTOP
 *	i)	tag	tag name
 *	i)	record	ctags -x image
 */
void
gtagsput(gtop, tag, record)
GTOP	*gtop;
char	*tag;
char	*record;
{
	char	*p, *q;
	char	lno[10];
	char	path[MAXPATHLEN+1];

	if (gtop->format == GTAGS_STANDARD) {
		/* entab(record); */
		dbop_put(gtop->dbop, tag, record);
		return;
	}
	/*
	 * gtop->format & GTAGS_COMPACT
	 */
	p = record;				/* ignore $1 */
	while (*p && !isspace(*p))
		p++;
	while (*p && isspace(*p))
		p++;
	q = lno;				/* lno = $2 */
	while (*p && !isspace(*p))
		*q++ = *p++;
	*q = 0;
	while (*p && isspace(*p))
		p++;
	q = path;				/* path = $3 */
	while (*p && !isspace(*p))
		*q++ = *p++;
	*q = 0;
	/*
	 * First time, it occurs, because 'prev_tag' and 'prev_path' are NULL.
	 */
	if (strcmp(gtop->prev_tag, tag) || strcmp(gtop->prev_path, path)) {
		if (gtop->prev_tag[0])
			dbop_put(gtop->dbop, gtop->prev_tag, strbuf_value(gtop->sb));
		strcpy(gtop->prev_tag, tag);
		strcpy(gtop->prev_path, path);
		/*
		 * Start creating new record.
		 */
		strbuf_reset(gtop->sb);
		strbuf_puts(gtop->sb, strmake(record, " \t"));
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, path);
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, lno);
	} else {
		strbuf_putc(gtop->sb, ',');
		strbuf_puts(gtop->sb, lno);
	}
}
/*
 * gtagsadd: add tags belonging to the path into tag file.
 *
 *	i)	gtop	descripter of GTOP
 *	i)	comline	tag command line
 *	i)	path	source file
 *	i)	flags	GTAGS_UNIQUE, GTAGS_EXTRACTMETHOD, GTAGS_DEBUG
 */
void
gtagsadd(gtop, comline, path, flags)
GTOP	*gtop;
char	*comline;
char	*path;
int	flags;
{
	char	*tagline;
	FILE	*ip;
	STRBUF	*sb = strbuf_open(0);
	STRBUF	*ib = strbuf_open(MAXBUFLEN);
	char	sort_command[MAXFILLEN+1];
	char	sed_command[MAXFILLEN+1];

	/*
	 * get command name of sort and sed.
	 */
	if (!getconfs("sort_command", sb))
		die("cannot get sort command name.");
#ifdef _WIN32
	if (!locatestring(strbuf_value(sb), ".exe", MATCH_LAST))
		strbuf_puts(sb, ".exe");
#endif
	strcpy(sort_command, strbuf_value(sb));
	strbuf_reset(sb);
	if (!getconfs("sed_command", sb))
		die("cannot get sed command name.");
#ifdef _WIN32
	if (!locatestring(strbuf_value(sb), ".exe", MATCH_LAST))
		strbuf_puts(sb, ".exe");
#endif
	strcpy(sed_command, strbuf_value(sb));
	/*
	 * add path index if not yet.
	 */
	pathput(path);
	/*
	 * make command line.
	 */
	strbuf_reset(sb);
	makecommand(comline, path, sb);
	/*
	 * Compact format.
	 */
	if (gtop->format & GTAGS_PATHINDEX) {
		char	*pno;

		if ((pno = pathget(path)) == NULL)
			die("GPATH is corrupted.('%s' not found)", path);
		strbuf_puts(sb, "| ");
		strbuf_puts(sb, sed_command);
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, "\"s@");
		strbuf_puts(sb, path);
		strbuf_puts(sb, "@");
		strbuf_puts(sb, pno);
		strbuf_puts(sb, "@\"");
	}
	if (gtop->format & GTAGS_COMPACT) {
		strbuf_puts(sb, "| ");
		strbuf_puts(sb, sort_command);
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, "+0 -1 +1n -2");
	}
	if (flags & GTAGS_UNIQUE)
		strbuf_puts(sb, " -u");
	if (flags & GTAGS_DEBUG)
		fprintf(stderr, "gtagsadd() executing '%s'\n", strbuf_value(sb));
	if (!(ip = popen(strbuf_value(sb), "r")))
		die("cannot execute '%s'.", strbuf_value(sb));
	while ((tagline = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		char	*tag, *p;

		strbuf_trim(ib);
		if (formatcheck(tagline, gtop->format) < 0)
			die("illegal parser output.\n'%s'", tagline);
		tag = strmake(tagline, " \t");		 /* tag = $1 */
		/*
		 * extract method when class method definition.
		 *
		 * Ex: Class::method(...)
		 *
		 * key	= 'method'
		 * data = 'Class::method  103 ./class.cpp ...'
		 */
		if (flags & GTAGS_EXTRACTMETHOD) {
			if ((p = locatestring(tag, ".", MATCH_LAST)) != NULL)
				tag = p + 1;
			else if ((p = locatestring(tag, "::", MATCH_LAST)) != NULL)
				tag = p + 2;
		}
		gtagsput(gtop, tag, tagline);
	}
	pclose(ip);
	strbuf_close(sb);
	strbuf_close(ib);
}
/*
 * belongto: wheather or not record belongs to the path.
 *
 *	i)	gtop	GTOP structure
 *	i)	path	path name (in standard format)
 *			path number (in compact format)
 *	i)	p	record
 *	r)		1: belong, 0: not belong
 */
static int
belongto(gtop, path, p)
GTOP	*gtop;
char	*path;
char	*p;
{
	char	*q;
	int	length = strlen(path);

	/*
	 * seek to path part.
	 */
	if (gtop->format & GTAGS_PATHINDEX) {
		for (q = p; *q && !isspace(*q); q++)
			;
		if (*q == 0)
			die("illegal tag format. '%s'", p);
		for (; *q && isspace(*q); q++)
			;
	} else
		q = locatestring(p, "./", MATCH_FIRST);
	if (*q == 0)
		die("illegal tag format. '%s'", p);
	if (!strncmp(q, path, length) && isspace(*(q + length)))
		return 1;
	return 0;
}
/*
 * gtagsdelete: delete records belong to path.
 *
 *	i)	gtop	GTOP structure
 *	i)	path	path name
 */
void
gtagsdelete(gtop, path)
GTOP	*gtop;
char	*path;
{
	char	*p, *key;

	/*
	 * In compact format, a path is saved as a file number.
	 */
	key = path;
	if (gtop->format & GTAGS_PATHINDEX)
		if ((key = pathget(path)) == NULL)
			die("GPATH is corrupted.('%s' not found)", path);
	/*
	 * read sequentially, because db(1) has just one index.
	 */
	for (p = dbop_first(gtop->dbop, NULL, NULL, 0); p; p = dbop_next(gtop->dbop))
		if (belongto(gtop, key, p))
			dbop_del(gtop->dbop, NULL);
	/*
	 * don't delete from path index.
	 */
}
/*
 * gtagsfirst: return first record
 *
 *	i)	gtop	GTOP structure
 *	i)	pattern	tag name
 *		o may be regular expression
 *		o may be NULL
 *	i)	flags	GTOP_PREFIX	prefix read
 *			GTOP_KEY	read key only
 *			GTOP_NOSOURCE	don't read source file(compact format)
 *			GTOP_NOREGEX	don't use regular expression.
 *	r)		record
 */
char *
gtagsfirst(gtop, pattern, flags)
GTOP	*gtop;
char	*pattern;
int	flags;
{
	int	dbflags = 0;
	char	*line;
	char    buf[IDENTLEN+1], *p;
	regex_t *preg = &reg;
	char	*key;

	gtop->flags = flags;
	if (flags & GTOP_PREFIX && pattern != NULL)
		dbflags |= DBOP_PREFIX;
	if (flags & GTOP_KEY)
		dbflags |= DBOP_KEY;

	if (flags & GTOP_NOREGEX) {
		key = pattern;
		preg = NULL;
	} else if (pattern == NULL || !strcmp(pattern, ".*")) {
		key = NULL;
		preg = NULL;
	} else if (isregex(pattern) && regcomp(preg, pattern, REG_EXTENDED) == 0) {
		if (*pattern == '^' && *(p = pattern + 1) && !isregexchar(*p)) {
			char    *prefix = buf;

			*prefix++ = *p++;
			while (*p && !isregexchar(*p))
				*prefix++ = *p++;
			*prefix = 0;
			key = buf;
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
	/*
	 * Compact format.
	 */
	gtop->line = line;			/* gtop->line = $0 */
	gtop->opened = 0;
	return genrecord(gtop);
}
/*
 * gtagsnext: return followed record
 *
 *	i)	gtop	GTOP structure
 *	r)		record
 *			NULL end of tag
 */
char *
gtagsnext(gtop)
GTOP	*gtop;
{
	char	*line;

	/*
	 * If it is standard format or only key.
	 * Just return it.
	 */
	if (gtop->format == GTAGS_STANDARD || gtop->flags & GTOP_KEY)
		return dbop_next(gtop->dbop);
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
	gtop->line = line;			/* gtop->line = $0 */
	gtop->opened = 0;
	return genrecord(gtop);
}
/*
 * gtagsclose: close tag file
 *
 *	i)	gtop	GTOP structure
 */
void
gtagsclose(gtop)
GTOP	*gtop;
{
	if (gtop->format & GTAGS_PATHINDEX || gtop->mode != GTAGS_READ)
		pathclose();
	if (gtop->sb && gtop->prev_tag[0])
		dbop_put(gtop->dbop, gtop->prev_tag, strbuf_value(gtop->sb));
	if (gtop->sb)
		strbuf_close(gtop->sb);
	if (gtop->ib)
		strbuf_close(gtop->ib);
	dbop_close(gtop->dbop);
	free(gtop);
}
static char *
genrecord(gtop)
GTOP	*gtop;
{
	static char	output[MAXBUFLEN+1];
	char	path[MAXPATHLEN+1];
	static char	buf[1];
	char	*buffer = buf;
	char	*lnop;
	int	tagline;

	if (!gtop->opened) {
		char	*p, *q;

		gtop->opened = 1;
		p = gtop->line;
		q = gtop->tag;				/* gtop->tag = $1 */
		while (!isspace(*p))
			*q++ = *p++;
		*q = 0;
		for (; isspace(*p) ; p++)
			;
		if (gtop->format & GTAGS_PATHINDEX) {	/* gtop->path = $2 */
			char	*name;

			q = path;
			while (!isspace(*p))
				*q++ = *p++;
			*q = 0;
			if ((name = pathget(path)) == NULL)
				die("GPATH is corrupted.('%s' not found)", path);
			strcpy(gtop->path, name);
		} else {
			q = gtop->path;
			while (!isspace(*p))
				*q++ = *p++;
			*q = 0;
		}
		for (; isspace(*p) ; p++)
			;
		gtop->lnop = p;			/* gtop->lnop = $3 */

		if (gtop->root)
#ifdef HAVE_SNPRINTF
			snprintf(path, sizeof(path), "%s/%s", gtop->root, &gtop->path[2]);
#else
			sprintf(path, "%s/%s", gtop->root, &gtop->path[2]);
#endif /* HAVE_SNPRINTF */
		else
#ifdef HAVE_SNPRINTF
			snprintf(path, sizeof(path), "%s", &gtop->path[2]);
#else
			sprintf(path, "%s", &gtop->path[2]);
#endif /* HAVE_SNPRINTF */
		if (!(gtop->flags & GTOP_NOSOURCE)) {
			if ((gtop->fp = fopen(path, "r")) != NULL)
				gtop->lno = 0;
		}
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
					die("unexpected end of file. '%s'", path);
				strbuf_trim(gtop->ib);
				gtop->lno++;
			}
		}
#ifdef HAVE_SNPRINTF
		snprintf(output, sizeof(output), "%-16s %3d %-16s %s",
				gtop->tag, tagline, gtop->path, buffer);
#else
		sprintf(output, "%-16s %3d %-16s %s",
				gtop->tag, tagline, gtop->path, buffer);
#endif /* HAVE_SNPRINTF */
		return output;
	}
	if (gtop->opened && gtop->fp != NULL) {
		gtop->opened = 0;
		fclose(gtop->fp);
		gtop->fp = NULL;
	}
	return NULL;
}
