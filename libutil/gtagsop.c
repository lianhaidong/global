/*
 * Copyright (c) 1997, 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2001, 2002 Tama Communications Corporation
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
#include "gpathop.h"
#include "split.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "strmake.h"
#include "tab.h"

static char	*unpack_pathindex(char *);
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
static STRBUF *output;
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
 *	i)	format	format
 *	r)	0:	normal
 *		-1:	tag name
 *		-2:	line number
 *		-3:	path
 *
 * [STANDARD FORMAT]
 * 0                 1  2              3
 * ----------------------------------------------------
 * func              83 ./func.c       func()
 *
 * [PATHINDEX FORMAT]
 * 0                 1  2              3
 * ----------------------------------------------------
 * func              83 38             func()
 *
 * [COMPACT FORMAT]
 * 0    1  2
 * ----------------------------------------------------
 * func 38 83,95,103,205
 */
void
formatcheck(line, format)
char	*line;
int	format;
{
	int n;
	char *p;
	SPLIT ptable;

	/*
	 * Extract parts.
	 */
	n = split(line, 4, &ptable);

	/*
	 * line number
	 */
	if (n < 4) {
		recover(&ptable);
		die("too small number of parts.\n'%s'", line);
	}
	for (p = ptable.part[1].start; *p; p++) {
		if (!isdigit(*p)) {
			recover(&ptable);
			die("line number includes other than digit.\n'%s'", line);
		}
	}
	/*
	 * path name
	 */
	if (format == GTAGS_STANDARD) {
		p = ptable.part[2].start;
		if (!(*p == '.' && *(p + 1) == '/' && *(p + 2))) {
			recover(&ptable);
			die("path name must start with './'.\n'%s'", line);
		}
	}
	if (format & GTAGS_PATHINDEX) {
		for (p = ptable.part[2].start; *p; p++)
			if (!isdigit(*p)) {
				recover(&ptable);
				die("file number includes other than digit.\n'%s'", line);
			}
	}
	recover(&ptable);
}
/*
 * gtags_setinfo: set info string.
 *
 *      i)      info    info string
 *
 * Currently this method is used for postgres.
 */
void
gtags_setinfo(info)
char *info;
{
	dbop_setinfo(info);
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
 *			GTAGS_POSTGRES
 *	r)		GTOP structure
 *
 * when error occurred, gtagopen doesn't return.
 * GTAGS_PATHINDEX needs GTAGS_COMPACT.
 */
GTOP	*
gtags_open(dbpath, root, db, mode, flags)
char	*dbpath;
char	*root;
int	db;
int	mode;
int	flags;
{
	GTOP	*gtop;
	char	*path;
	int	dbmode = 0;
	int	dbopflags = 0;

	/* initialize for isregex() */
	if (!init) {
		regexchar['^'] = regexchar['$'] = regexchar['{'] =
		regexchar['}'] = regexchar['('] = regexchar[')'] =
		regexchar['.'] = regexchar['*'] = regexchar['+'] =
		regexchar['['] = regexchar[']'] = regexchar['?'] =
		regexchar['\\'] = init = 1;
	}
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
	dbopflags = DBOP_DUP;
	if (flags & GTAGS_POSTGRES)
		dbopflags |= DBOP_POSTGRES;
	path = strdup(makepath(dbpath, dbname(db), NULL));
	if (path == NULL)
		die("short of memory.");
	gtop->dbop = dbop_open(path, dbmode, 0644, dbopflags);
	free(path);
	if (gtop->dbop == NULL) {
		if (dbmode == 1)
			die("cannot make %s.", dbname(db));
		die("%s not found.", dbname(db));
	}
	if (gtop->dbop->openflags & DBOP_POSTGRES)
		gtop->openflags |= GTAGS_POSTGRES;
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
			dbop_put(gtop->dbop, COMPACTKEY, COMPACTKEY, "0");
		}
		if (flags & GTAGS_PATHINDEX) {
			gtop->format |= GTAGS_PATHINDEX;
			dbop_put(gtop->dbop, PATHINDEXKEY, PATHINDEXKEY, "0");
		}
		if (flags & (GTAGS_COMPACT|GTAGS_PATHINDEX)) {
			char	buf[80];

			gtop->format_version = 2;
			snprintf(buf, sizeof(buf),
				"%s %d", VERSIONKEY, gtop->format_version);
			dbop_put(gtop->dbop, VERSIONKEY, buf, "0");
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
		if (gpath_open(dbpath, dbmode, dbopflags) < 0) {
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
		else
			gtop->sb = strbuf_open(0);
	}
	return gtop;
}
/*
 * gtags_put: put tag record with packing.
 *
 *	i)	gtop	descripter of GTOP
 *	i)	tag	tag name
 *	i)	record	ctags -x image
 *	i)	fid	file id.
 *
 * NOTE: If format is GTAGS_COMPACT then this function is destructive.
 */
void
gtags_put(gtop, tag, record, fid)
GTOP	*gtop;
char	*tag;
char	*record;
char	*fid;
{
	char *line, *path;
	SPLIT ptable;

	if (gtop->format == GTAGS_STANDARD || gtop->format == GTAGS_PATHINDEX) {
		/* entab(record); */
		dbop_put(gtop->dbop, tag, record, fid);
		return;
	}
	/*
	 * gtop->format & GTAGS_COMPACT
	 */
	if (split(record, 4, &ptable) != 4)
		die("illegal format.");
	line = ptable.part[1].start;
	path = ptable.part[2].start;
	/*
	 * First time, it occurs, because 'prev_tag' and 'prev_path' are NULL.
	 */
	if (strcmp(gtop->prev_tag, tag) || strcmp(gtop->prev_path, path)) {
		if (gtop->prev_tag[0]) {
			dbop_put(gtop->dbop, gtop->prev_tag, strbuf_value(gtop->sb), gtop->prev_fid);
		}
		strlimcpy(gtop->prev_tag, tag, sizeof(gtop->prev_tag));
		strlimcpy(gtop->prev_path, path, sizeof(gtop->prev_path));
		strlimcpy(gtop->prev_fid, fid, sizeof(gtop->prev_fid));
		/*
		 * Start creating new record.
		 */
		strbuf_reset(gtop->sb);
		strbuf_puts(gtop->sb, strmake(record, " \t"));
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, path);
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, line);
	} else {
		strbuf_putc(gtop->sb, ',');
		strbuf_puts(gtop->sb, line);
	}
	recover(&ptable);
}
/*
 * gtags_add: add tags belonging to the path into tag file.
 *
 *	i)	gtop	descripter of GTOP
 *	i)	comline	tag command line
 *	i)	path	source file
 *	i)	flags	GTAGS_UNIQUE, GTAGS_EXTRACTMETHOD, GTAGS_DEBUG
 */
void
gtags_add(gtop, comline, path, flags)
GTOP	*gtop;
char	*comline;
char	*path;
int	flags;
{
	char	*ctags_x;
	FILE	*ip;
	STRBUF	*sb = strbuf_open(0);
	STRBUF	*ib = strbuf_open(MAXBUFLEN);
	STRBUF	*sort_command = strbuf_open(0);
	STRBUF	*sed_command = strbuf_open(0);
	char	*fid;

	/*
	 * get command name of sort and sed.
	 */
	if (!getconfs("sort_command", sort_command))
		die("cannot get sort command name.");
#if defined(_WIN32) || defined(__DJGPP__)
	if (!locatestring(strbuf_value(sort_command), ".exe", MATCH_LAST))
		strbuf_puts(sort_command, ".exe");
#endif
	if (!getconfs("sed_command", sed_command))
		die("cannot get sed command name.");
#if defined(_WIN32) || defined(__DJGPP__)
	if (!locatestring(strbuf_value(sed_command), ".exe", MATCH_LAST))
		strbuf_puts(sed_command, ".exe");
#endif
	/*
	 * add path index if not yet.
	 */
	gpath_put(path);
	/*
	 * make command line.
	 */
	makecommand(comline, path, sb);
	/*
	 * get file id.
	 */
	if (gtop->format & GTAGS_PATHINDEX || gtop->openflags & GTAGS_POSTGRES) {
		if (!(fid = gpath_path2fid(path)))
			die("GPATH is corrupted.('%s' not found)", path);
	} else
		fid = "0";
	/*
	 * Compact format.
	 */
	if (gtop->format & GTAGS_PATHINDEX) {
		strbuf_puts(sb, "| ");
		strbuf_puts(sb, strbuf_value(sed_command));
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, "\"s@");
		strbuf_puts(sb, path);
		strbuf_puts(sb, "@");
		strbuf_puts(sb, fid);
		strbuf_puts(sb, "@\"");
	}
	if (gtop->format & GTAGS_COMPACT) {
		strbuf_puts(sb, "| ");
		strbuf_puts(sb, strbuf_value(sort_command));
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, "+0 -1 +1n -2");
	}
	if (flags & GTAGS_UNIQUE)
		strbuf_puts(sb, " -u");
	if (flags & GTAGS_DEBUG)
		fprintf(stderr, "gtags_add() executing '%s'\n", strbuf_value(sb));
	if (!(ip = popen(strbuf_value(sb), "r")))
		die("cannot execute '%s'.", strbuf_value(sb));
	while ((ctags_x = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		char	*tag, *p;

		strbuf_trim(ib);
		formatcheck(ctags_x, gtop->format);
		tag = strmake(ctags_x, " \t");		 /* tag = $1 */
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
		gtags_put(gtop, tag, ctags_x, fid);
	}
	pclose(ip);
	strbuf_close(sort_command);
	strbuf_close(sed_command);
	strbuf_close(sb);
	strbuf_close(ib);
}
/*
 * belongto: wheather or not record belongs to the path.
 *
 *	i)	gtop	GTOP structure
 *	i)	path	path name (in standard format)
 *			path number (in compact format)
 *	i)	line	record
 *	r)		1: belong, 0: not belong
 *
 */
static int
belongto(gtop, path, line)
GTOP	*gtop;
char	*path;
char	*line;
{
	char *p;
	int status, n;
	SPLIT ptable;

	/*
	 * Get path.
	 */
	n = split(p, 4, &ptable);
	if (gtop->format == GTAGS_STANDARD || gtop->format == GTAGS_PATHINDEX) {
		if (n < 4)
			die("too small number of parts.");
		p = ptable.part[2].start;
	} else if (gtop->format & GTAGS_COMPACT) {
		if (n != 3)
			die("illegal compact format.\n");
		p = ptable.part[1].start;
	}
	status = !strcmp(p, path) ? 1 : 0;
	recover(&ptable);
	return status;
}
/*
 * gtags_delete: delete records belong to path.
 *
 *	i)	gtop	GTOP structure
 *	i)	path	path name
 */
void
gtags_delete(gtop, path)
GTOP	*gtop;
char	*path;
{
	char *p, *fid;
	/*
	 * In compact format, a path is saved as a file number.
	 */
	if (gtop->format & GTAGS_PATHINDEX)
		if ((path = gpath_fid2path(path)) == NULL)
			die("GPATH is corrupted.('%s' not found)", path);
#ifdef USE_POSTGRES
	if (gtop->openflags & GTAGS_POSTGRES) {
		char *fid;

		if ((fid = gpath_path2fid(path)) == NULL)
			die("GPATH is corrupted.('%s' not found)", path);
		dbop_delete_by_fid(gtop->dbop, fid);
		return;
	}
#endif
	/*
	 * read sequentially, because db(1) has just one index.
	 */
	for (p = dbop_first(gtop->dbop, NULL, NULL, 0); p; p = dbop_next(gtop->dbop))
		if (belongto(gtop, path, p))
			dbop_delete(gtop->dbop, NULL);
	/*
	 * don't delete from path index.
	 */
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
 *	r)		record
 */
char *
gtags_first(gtop, pattern, flags)
GTOP	*gtop;
char	*pattern;
int	flags;
{
	int	dbflags = 0;
	char	*line;
	char    prefix[IDENTLEN+1], *p;
	regex_t *preg = &reg;
	char	*key;
	int	regflags = REG_EXTENDED;

	gtop->flags = flags;
	if (flags & GTOP_PREFIX && pattern != NULL)
		dbflags |= DBOP_PREFIX;
	if (flags & GTOP_KEY)
		dbflags |= DBOP_KEY;
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
	gtop->line = line;			/* gtop->line = $0 */
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
char *
gtags_next(gtop)
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
	gtop->line = line;			/* gtop->line = $0 */
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
GTOP	*gtop;
{
	if (gtop->format & GTAGS_PATHINDEX || gtop->mode != GTAGS_READ)
		gpath_close();
	if (gtop->sb && gtop->prev_tag[0])
		dbop_put(gtop->dbop, gtop->prev_tag, strbuf_value(gtop->sb), "0");
	if (gtop->sb)
		strbuf_close(gtop->sb);
	if (gtop->ib)
		strbuf_close(gtop->ib);
	dbop_close(gtop->dbop);
	free(gtop);
}
/*
 * unpack_pathindex: convert pathindex format into standard format.
 *
 *	i)	line	tag line
 */
static char *
unpack_pathindex(line)
char *line;
{
	SPLIT ptable;
	int n, i;
	char *path, *fid;

	n = split(line, 4, &ptable);
	if (n < 4) {
		recover(&ptable);
		die("illegal tag format.'%s'\n", line);
	}
	/*
	 * extract path and convert into file number.
	 */
	path = gpath_fid2path(ptable.part[2].start);
	if (path == NULL)
		die("GPATH is corrupted.(fid '%s' not found)", fid);
	recover(&ptable);
	/*
	 * copy line with converting.
	 */
	if (output == NULL)
		output = strbuf_open(MAXBUFLEN);
	else
		strbuf_reset(output);
	strbuf_nputs(output, line, ptable.part[2].start - line);
	strbuf_puts(output, path);
	strbuf_puts(output, ptable.part[2].end);

	return strbuf_value(output);
}
static char *
genrecord(gtop)
GTOP	*gtop;
{
	SPLIT ptable;
	static char	output[MAXBUFLEN+1];
	char    path[MAXPATHLEN+1];
	static char	buf[1];
	char	*buffer = buf;
	char	*lnop;
	int	tagline;

	if (!gtop->opened) {
		int n;
		char *p;

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
			char *q;
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
