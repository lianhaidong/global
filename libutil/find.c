/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000, 2001, 2002
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
#include "is_unixy.h"
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
static regex_t	*skip;			/* regex for skipping units */
static regex_t	suff_area;
static regex_t	*suff = &suff_area;	/* regex for suffixes */
static STRBUF *list;
static int list_count;
static char **listarray;		/* list for skipping full path */
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
/*
 * prepare_source: preparing regular expression.
 *
 *	i)	flags	flags for regcomp.
 *	go)	suff	regular expression for source files.
 */
static void
prepare_source()
{
	STRBUF	*sb = strbuf_open(0);
	char	*sufflist = NULL;
	int	flags = REG_EXTENDED;

	/*
	 * load icase_path option.
	 */
	if (getconfb("icase_path"))
		flags |= REG_ICASE;
#if defined(_WIN32) || defined(__DJGPP__)
	flags |= REG_ICASE;
#endif
	strbuf_reset(sb);
	if (!getconfs("suffixes", sb))
		die("cannot get suffixes data.");
	sufflist = strdup(strbuf_value(sb));
	if (!sufflist)
		die("short of memory.");
	trim(sufflist);
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
	strbuf_close(sb);
	if (sufflist)
		free(sufflist);
}
/*
 * prepare_skip: prepare skipping files.
 *
 *	go)	skip	regular expression for skip files.
 *	go)	listarry[] skip list.
 *	go)	list_count count of skip list.
 */
static void
prepare_skip()
{
	char *skiplist;
	STRBUF *reg = strbuf_open(0);
	int reg_count = 0;
	char *p, *q;
	int     flags = REG_EXTENDED|REG_NEWLINE;

	/*
	 * load icase_path option.
	 */
	if (getconfb("icase_path"))
		flags |= REG_ICASE;
#if defined(_WIN32) || defined(__DJGPP__)
	flags |= REG_ICASE;
#endif
	/*
	 * initinalize common data.
	 */
	if (!list)
		list = strbuf_open(0);
	else
		strbuf_reset(list);
	list_count = 0;
	if (listarray)
		(void)free(listarray);
	listarray = (char **)0;
	/*
	 * load skip data.
	 */
	if (!getconfs("skip", reg))
		return;
	skiplist = strdup(strbuf_value(reg));
	if (!skiplist)
		die("short of memory.");
	trim(skiplist);
	strbuf_reset(reg);
	/*
	 * construct regular expression.
	 */
	strbuf_putc(reg, '(');	/* ) */
	for (p = skiplist; p; ) {
		char    *skipf = p;
		if ((p = locatestring(p, ",", MATCH_FIRST)) != NULL)
			*p++ = 0;
		if (*skipf == '/') {
			list_count++;
			strbuf_puts0(list, skipf);
		} else {
			reg_count++;
			strbuf_putc(reg, '/');
			for (q = skipf; *q; q++) {
				if (*q == '.')
					strbuf_putc(reg, '\\');
				strbuf_putc(reg, *q);
			}
			if (*(q - 1) != '/')
				strbuf_putc(reg, '$');
			if (p)
				strbuf_putc(reg, '|');
		}
	}
	strbuf_unputc(reg, '|');
	strbuf_putc(reg, ')');
	if (reg_count > 0) {
		/*
		 * compile regular expression.
		 */
		skip = &skip_area;
		retval = regcomp(skip, strbuf_value(reg), flags);
		if (debug)
			fprintf(stderr, "skip regex: %s\n", strbuf_value(reg));
		if (retval != 0)
			die("cannot compile regular expression.");
	} else {
		skip = (regex_t *)0;
	}
	if (list_count > 0) {
		int i;
		listarray = (char **)malloc(sizeof(char *) * list_count);
		p = strbuf_value(list);
		if (debug)
			fprintf(stderr, "skip list: ");
		for (i = 0; i < list_count; i++) {
			if (debug) {
				fprintf(stderr, "%s", p);
				if (i + 1 < list_count)
					fputc(',', stderr);
			}
			listarray[i] = p;
			p += strlen(p) + 1;
		}
		if (debug)
			fputc('\n', stderr);
	}
	strbuf_close(reg);
	free(skiplist);
}
/*
 * skipthisfile: check whether or not we accept this file.
 *
 *	i)	path	path name (must start with ./)
 *	r)		1: skip, 0: dont skip
 */
int
skipthisfile(path)
char *path;
{
	char *first, *last;
	int i;

	/*
	 * unit check.
	 */
	if (skip && regexec(skip, path, 0, 0, 0) == 0)
		return 1;
	/*
	 * list check.
	 */
	if (list_count == 0)
		return 0;
	for (i = 0; i < list_count; i++) {
		first = listarray[i];
		last = first + strlen(first);
		/*
		 * the path must start with "./".
		 */
		if (*(last - 1) == '/') {	/* it's a directory */
			if (!strncmp(path + 1, first, last - first))
				return 1;
		} else {
			if (!strcmp(path + 1, first))
				return 1;
		}
	}
	return 0;
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

static int getdirs(char *, STRBUF *);

/*
 * getdirs: get directory list
 *
 *	i)	dir	directory
 *	o)	sb	string buffer
 *	r)		-1: error, 0: normal
 *
 * format of directory list:
 * |ddir1\0ffile1\0llink\0|
 * means directory 'dir1', file 'file1' and symbolic link 'link'.
 */
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
/*
 * find_open: start iterator without GPATH.
 */
void
find_open()
{
	assert(opened == 0);
	opened = 1;

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
	 * prepare regular expressions.
	 */
	prepare_source();
	prepare_skip();
}
/*
 * find_read: read path without GPATH.
 *
 *	r)		path
 */
char    *
find_read(void)
{
	static	char val[MAXPATHLEN+1];

	for (;;) {
		while (curp->p < curp->end) {
			char	type = *(curp->p);
			char    *unit = curp->p + 1;

			curp->p += strlen(curp->p) + 1;
			if (type == 'f' || type == 'l') {
				char	*path = makepath(dir, unit, NULL);
				if (skipthisfile(path))
					continue;
				if (regexec(suff, path, 0, 0, 0) == 0) {
					/* source file */
					strcpy(val, path);
				} else {
					/* other file like 'Makefile' */
					val[0] = ' ';
					strcpy(&val[1], path);
				}
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
/*
 * find_close: close iterator.
 */
void
find_close(void)
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

/*
 * find_open: start iterator without GPATH.
 */
void
find_open()
{
	char	*findcom = "find . -type f -print";

	assert(opened == 0);
	opened = 1;

	if (debug)
		fprintf(stderr, "find com: %s\n", findcom);
	/*
	 * prepare regular expressions.
	 */
	prepare_source();
	prepare_skip();
	if (!(ip = popen(findcom, "r")))
		die("cannot execute find.");
}
/*
 * find_read: read path without GPATH.
 *
 *	r)		path
 */
char	*
find_read(void)
{
	static char	val[MAXPATHLEN+2];
	char	*path = &val[1];
	char	*p;

	assert(opened == 1);
	while (fgets(path, MAXPATHLEN, ip)) {
		/*
		 * chop(path)
		 */
		p = path + strlen(path) - 1;
		if (*p != '\n')
			die("output of find(1) is wrong (findread).");
		*p = 0;
		if (skipthisfile(path))
			continue;
		if (regexec(suff, path, 0, 0, 0) != 0) {
			/* other file like 'Makefile' */
			val[0] = ' ';
			path = val;
		}
		return path;
	}
	return NULL;
}
/*
 * find_close: close iterator.
 */
void
find_close(void)
{
	assert(opened == 1);
	pclose(ip);
	opened = 0;
	regfree(suff);
	if (skip)
		regfree(skip);
}
#endif /* !HAVE_FIND */
