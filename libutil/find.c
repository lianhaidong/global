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

/*
 * If find(1) is available the use it to traverse directory tree.
 * Otherwise use dirent(3).
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#include <ctype.h>
#ifndef HAVE_FIND
#include <sys/types.h>
#include <dirent.h>
#ifndef HAVE_DP_D_TYPE
#include <sys/stat.h>
#endif
#endif
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

#include "gparam.h"
#include "regex.h"
#include "conf.h"
#include "die.h"
#include "find.h"
#include "locatestring.h"
#include "makepath.h"
#include "strbuf.h"

/*
 * usage of ?findxxx()
 *
 *	?findopen();
 *	while (path = ?findread()) {
 *		...
 *	}
 *	?findclose();
 *
 */
static regex_t	skip_area;
static regex_t	*skip = &skip_area;
static int	opened;
static int	retval;

static void	trim(char *);
extern int	debug;
/*
 * trim: remove blanks and '\'.
 */
static void
trim(s)
char	*s;
{
	char	*p;

	for (p = s; *s; s++) {
		if (isspace(*s))
			continue;	
		if (*s == '\\' && *(s + 1))
			s++;
		*p++ = *s;
	}
	*p = 0;
}
#ifndef HAVE_FIND
/*----------------------------------------------------------------------*/
/* dirent version findxxx()						*/
/*----------------------------------------------------------------------*/
#define STACKSIZE 50
static  char    dir[MAXPATHLEN+1];		/* directory path */
static  struct {
	STRBUF  *sb;
	char    *dirp, *start, *end, *p;
} stack[STACKSIZE], *topp, *curp;		/* stack */

static regex_t	suff_area;
static regex_t	*suff = &suff_area;

static int getdirs(char *, STRBUF *);

static int
getdirs(dir, sb)
char    *dir;
STRBUF  *sb;
{
	DIR     *dirp;
	struct dirent *dp;
#ifndef HAVE_DP_D_TYPE
	struct stat st;
#endif

	if ((dirp = opendir(dir)) == NULL)
		return -1;
	while ((dp = readdir(dirp)) != NULL) {
#ifdef HAVE_DP_D_NAMLEN
		if (dp->d_namlen == 1 && dp->d_name[0] == '.')
#else
		if (!strcmp(dp->d_name, "."))
#endif
			continue;
#ifdef HAVE_DP_D_NAMLEN
		if (dp->d_namlen == 2 && dp->d_name[0] == '.' && dp->d_name[1] == '.')
#else
		if (!strcmp(dp->d_name, ".."))
#endif
			continue;
#ifdef HAVE_DP_D_TYPE
		if (dp->d_type == DT_DIR)
			strbuf_putc(sb, 'd');
		else if (dp->d_type == DT_REG)
			strbuf_putc(sb, 'f');
		else if (dp->d_type == DT_LNK)
			strbuf_putc(sb, 'l');
		else
			strbuf_putc(sb, ' ');
		strbuf_puts(sb, dp->d_name);
#else
#ifdef HAVE_LSTAT
		if (lstat(makepath(dir, dp->d_name, NULL), &st) < 0) {
			fprintf(stderr, "cannot lstat '%s'. (Ignored)\n", dp->d_name);
			continue;
		}
#else
		if (stat(makepath(dir, dp->d_name, NULL), &st) < 0) {
			fprintf(stderr, "cannot stat '%s'. (Ignored)\n", dp->d_name);
			continue;
		}
#endif
		if (S_ISDIR(st.st_mode))
			strbuf_putc(sb, 'd');
		else if (S_ISREG(st.st_mode))
			strbuf_putc(sb, 'f');
#ifdef S_ISLNK
		else if (S_ISLNK(st.st_mode))
			strbuf_putc(sb, 'l');
#endif
		else
			strbuf_putc(sb, ' ');
		strbuf_puts(sb, dp->d_name);
#endif /* HAVE_DP_D_TYPE */
		strbuf_putc(sb, '\0');
	}
	(void)closedir(dirp);
	return 0;
}
void
ffindopen()
{
	STRBUF	*sb = strbuf_open(0);
	char	*sufflist = NULL;
	char	*skiplist = NULL;
	int	flags = REG_EXTENDED;

	assert(opened == 0);
	opened = 1;

	/*
	 * load icase_path option.
	 */
	if (getconfb("icase_path"))
		flags |= REG_ICASE;
#ifdef _WIN32
	flags |= REG_ICASE;
#endif /* _WIN32 */
	/*
	 * setup stack.
	 */
	curp = &stack[0];
	topp = curp + STACKSIZE; 
	strcpy(dir, ".");

	curp->dirp = dir + strlen(dir);
	curp->sb = strbuf_open(0);
	if (getdirs(dir, curp->sb) < 0)
		die("cannot open '.' directory.");
	curp->start = curp->p = strbuf_value(curp->sb);
	curp->end   = curp->start + strbuf_getlen(curp->sb);

	/*
	 * preparing regular expression.
	 */
	strbuf_reset(sb);
	if (!getconfs("suffixes", sb))
		die("cannot get suffixes data.");
	sufflist = strdup(strbuf_value(sb));
	if (!sufflist)
		die("short of memory.");
	trim(sufflist);
	strbuf_reset(sb);
	if (getconfs("skip", sb)) {
		skiplist = strdup(strbuf_value(sb));
		if (!skiplist)
			die("short of memory.");
		trim(skiplist);
	}
	{
		char    *suffp;

		strbuf_reset(sb);
		strbuf_puts(sb, "\\.(");       /* ) */
		for (suffp = sufflist; suffp; ) {
			char    *p;

			for (p = suffp; *p && *p != ','; p++) {
				strbuf_putc(sb, '\\');
				strbuf_putc(sb, *p);
			}
			if (!*p)
				break;
			assert(*p == ',');
			strbuf_putc(sb, '|');
			suffp = ++p;
		}
		strbuf_puts(sb, ")$");
		/*
		 * compile regular expression.
		 */
		retval = regcomp(suff, strbuf_value(sb), flags);
		if (debug)
			fprintf(stderr, "find regex: %s\n", strbuf_value(sb));
		if (retval != 0)
			die("cannot compile regular expression.");
	}
	if (skiplist) {
		char    *p, *q;
		/*
		 * construct regular expression.
		 */
		strbuf_reset(sb);
		strbuf_putc(sb, '(');	/* ) */
		for (p = skiplist; p; ) {
			char    *skipf = p;
			if ((p = locatestring(p, ",", MATCH_FIRST)) != NULL)
				*p++ = 0;
			strbuf_putc(sb, '/');
			for (q = skipf; *q; q++) {
				if (*q == '.')
					strbuf_putc(sb, '\\');
				strbuf_putc(sb, *q);
			}
			if (*(q - 1) != '/')
				strbuf_putc(sb, '$');
			if (p)
				strbuf_putc(sb, '|');
		}
		strbuf_putc(sb, ')');
		/*
		 * compile regular expression.
		 */
		retval = regcomp(skip, strbuf_value(sb), flags);
		if (debug)
			fprintf(stderr, "skip regex: %s\n", strbuf_value(sb));
		if (retval != 0)
			die("cannot compile regular expression.");
	} else {
		skip = (regex_t *)0;
	}
	strbuf_close(sb);
	if (sufflist)
		free(sufflist);
	if (skiplist)
		free(skiplist);
}
char    *
ffindread(length)
int	*length;
{
	static	char val[MAXPATHLEN+1];

	for (;;) {
		while (curp->p < curp->end) {
			char	type = *(curp->p);
			char    *unit = curp->p + 1;

			curp->p += strlen(curp->p) + 1;
			if (type == 'f' || type == 'l') {
				char	*path = makepath(dir, unit, NULL);
				if (regexec(suff, path, 0, 0, 0) != 0)
					continue;
				if (skip && regexec(skip, path, 0, 0, 0) == 0)
					continue;
				strcpy(val, path);
				if (length)
					*length = strlen(val);
				return val;
			}
			if (type == 'd') {
				STRBUF  *sb = strbuf_open(0);
				char    *dirp = curp->dirp;

				strcat(dirp, "/");
				strcat(dirp, unit);
				if (getdirs(dir, sb) < 0) {
					fprintf(stderr, "cannot open directory '%s'. (Ignored)\n", dir);
					strbuf_close(sb);
					*(curp->dirp) = 0;
					continue;
				}
				/*
				 * Push stack.
				 */
				if (++curp >= topp)
					die("directory stack over flow.");
				curp->dirp = dirp + strlen(dirp);
				curp->sb = sb;
				curp->start = curp->p = strbuf_value(sb);
				curp->end   = curp->start + strbuf_getlen(sb);
			}
		}
		strbuf_close(curp->sb);
		curp->sb = NULL;
		if (curp == &stack[0])
			break;
		/*
		 * Pop stack.
		 */
		curp--;
		*(curp->dirp) = 0;
	}
	return NULL;
}
void
ffindclose(void)
{
	assert(opened == 1);
	for (curp = &stack[0]; curp < topp; curp++)
		if (curp->sb != NULL)
			strbuf_close(curp->sb);
	regfree(suff);
	if (skip)
		regfree(skip);
	opened = 0;
}
#else /* !HAVE_FIND */
/*----------------------------------------------------------------------*/
/* find command version							*/
/*----------------------------------------------------------------------*/
static FILE	*ip;

void
ffindopen()
{
	char	*findcom, *p, *q;
	STRBUF	*sb;
	char	*sufflist = NULL;
	char	*skiplist = NULL;
	int	flags = REG_EXTENDED|REG_NEWLINE;

	assert(opened == 0);
	opened = 1;

	/*
	 * load icase_path option.
	 */
	if (getconfb("icase_path"))
		flags |= REG_ICASE;
#ifdef _WIN32
	flags |= REG_ICASE;
#endif /* _WIN32 */
	sb = strbuf_open(0);
	if (!getconfs("suffixes", sb))
		die("cannot get suffixes data.");
	sufflist = strdup(strbuf_value(sb));
	if (!sufflist)
		die("short of memory.");
	trim(sufflist);
	strbuf_reset(sb);
	if (getconfs("skip", sb)) {
		skiplist = strdup(strbuf_value(sb));
		if (!skiplist)
			die("short of memory.");
		trim(skiplist);
	}

	strbuf_reset(sb);
	strbuf_puts(sb, "find . \\( -type f -o -type l \\) \\(");
	for (p = sufflist; p; ) {
		char	*suff = p;
		if ((p = locatestring(p, ",", MATCH_FIRST)) != NULL)
			*p++ = 0;
		strbuf_puts(sb, " -name '*.");
		strbuf_puts(sb, suff);
		strbuf_puts(sb, "'");
		if (p)
			strbuf_puts(sb, " -o");
	}
	strbuf_puts(sb, " \\) -print");
	findcom = strbuf_value(sb);
	if (debug)
		fprintf(stderr, "find com: %s\n", findcom);

	if (skiplist) {
		char	*reg;
		STRBUF	*sbb = strbuf_open(0);
		/*
		 * construct regular expression.
		 */
		strbuf_putc(sbb, '(');	/* ) */
		for (p = skiplist; p; ) {
			char    *skipf = p;
			if ((p = locatestring(p, ",", MATCH_FIRST)) != NULL)
				*p++ = 0;
			strbuf_putc(sbb, '/');
			for (q = skipf; *q; q++) {
				if (*q == '.')
					strbuf_putc(sbb, '\\');
				strbuf_putc(sbb, *q);
			}
			if (*(q - 1) != '/')
				strbuf_putc(sbb, '$');
			if (p)
				strbuf_putc(sbb, '|');
		}
		strbuf_putc(sbb, ')');
		reg = strbuf_value(sbb);
		/*
		 * compile regular expression.
		 */
		retval = regcomp(skip, reg, flags);
		if (debug)
			fprintf(stderr, "skip regex: %s\n", reg);
		if (retval != 0)
			die("cannot compile regular expression.");
		strbuf_close(sbb);
	} else {
		skip = (regex_t *)0;
	}
	if (!(ip = popen(findcom, "r")))
		die("cannot execute find.");
	strbuf_close(sb);
	if (sufflist)
		free(sufflist);
	if (skiplist)
		free(skiplist);
}
char	*
ffindread(length)
int	*length;
{
	static char	path[MAXPATHLEN+1];
	char	*p;

	assert(opened == 1);
	while (fgets(path, MAXPATHLEN, ip)) {
		if (!skip || regexec(skip, path, 0, 0, 0) != 0) {
			/*
			 * chop(path)
			 */
			p = path + strlen(path) - 1;
			if (*p != '\n')
				die("output of find(1) is wrong (findread).");
			*p = 0;
			if (length)
				*length = p - path;
			return path;
		}
	}
	return NULL;
}
void
ffindclose(void)
{
	assert(opened == 1);
	pclose(ip);
	opened = 0;
}
#endif /* !HAVE_FIND */

/*
 * Faster findXXX() using GPATH.
 *
 * gfindxxx() does almost same with findxxx() but much faster,
 * because gfindxxx() use GPATH (file index).
 * If GPATH exist then you should use this.
 */
#include "dbop.h"
#include "gtagsop.h"

static DBOP	*dbop;
static int      opened;
static int      first;
static char	prefix[MAXPATHLEN+1];

void
gfindopen(dbpath, local)
char	*dbpath;
char	*local;
{
	assert(opened == 0);
	assert(first == 0);
	dbop = dbop_open(makepath(dbpath, dbname(GPATH), NULL), 0, 0, 0);
	if (dbop == NULL)
		die("GPATH not found.");
	strcpy(prefix, (local) ? local : "./");
	opened = 1;
	first = 1;
}
char	*
gfindread()
{
	assert(opened == 1);
	if (first) {
		first = 0;
		return dbop_first(dbop, prefix, NULL, DBOP_KEY | DBOP_PREFIX);
	}
	return dbop_next(dbop);
}
void
gfindclose(void)
{
	dbop_close(dbop);
	opened = first = 0;
}
