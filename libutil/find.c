/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2005, 2006, 2008,
 *	2009, 2011 Tama Communications Corporation
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
#ifdef HAVE_DIRENT_H
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
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

#include "abs2rel.h"
#include "char.h"
#include "checkalloc.h"
#include "conf.h"
#include "die.h"
#include "find.h"
#include "getdbpath.h"
#include "is_unixy.h"
#include "langmap.h"
#include "locatestring.h"
#include "makepath.h"
#include "path.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "test.h"
#include "varray.h"

/*
 * use an appropriate string comparison for the file system; define the position of the root slash.
 */
#if defined(_WIN32) || defined(__DJGPP__)
#define STRCMP stricmp
#define STRNCMP strnicmp
#define ROOT 2
#else
#define STRCMP strcmp
#define STRNCMP strncmp
#define ROOT 0
#endif

/*
 * usage of find_xxx()
 *
 *	find_open(NULL);
 *	while (path = find_read()) {
 *		...
 *	}
 *	find_close();
 *
 */
static regex_t skip_area;
static regex_t *skip;			/* regex for skipping units */
static regex_t suff_area;
static regex_t *suff = &suff_area;	/* regex for suffixes */
static STRBUF *list;
static int list_count;
static char **listarray;		/* list for skipping full path */
static FILE *ip;
static FILE *temp;
static char rootdir[PATH_MAX];
static char cwddir[MAXPATHLEN];
static int find_mode;
static int find_eof;
#define FIND_OPEN	1
#define FILELIST_OPEN	2

static void trim(char *);
static char *find_read_traverse(void);
static char *find_read_filelist(void);

extern int qflag;
#ifdef DEBUG
extern int debug;
#endif
int allow_blank = 0;
int check_looplink = 0;
/*
 * trim: remove blanks and '\'.
 */
static void
trim(char *s)
{
	char *p;

	for (p = s; *s; s++) {
		if (isspace((unsigned char)*s))
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
prepare_source(void)
{
	STRBUF *sb = strbuf_open(0);
	char *sufflist = NULL;
	char *langmap = NULL;
	int flags = REG_EXTENDED;

	/*
	 * load icase_path option.
	 */
	if (getconfb("icase_path"))
		flags |= REG_ICASE;
#if defined(_WIN32) || defined(__DJGPP__)
	flags |= REG_ICASE;
#endif
	/*
	 * make suffix list.
	 */
	strbuf_reset(sb);
	if (getconfs("langmap", sb)) {
		langmap =  check_strdup(strbuf_value(sb));
	}
	strbuf_reset(sb);
	make_suffixes(langmap ? langmap : DEFAULTLANGMAP, sb);
	sufflist = check_strdup(strbuf_value(sb));
	trim(sufflist);
	{
		const char *suffp;
		int retval;

		strbuf_reset(sb);
		strbuf_puts(sb, "\\.(");       /* ) */
		for (suffp = sufflist; suffp; ) {
			const char *p;

			for (p = suffp; *p && *p != ','; p++) {
				if (!isalnum((unsigned char)*p))
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
		if (retval != 0)
			die("cannot compile regular expression.");
	}
	strbuf_close(sb);
	if (langmap)
		free(langmap);
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
prepare_skip(void)
{
	char *skiplist;
	STRBUF *reg = strbuf_open(0);
	int reg_count = 0;
	char *p, *q;
	int flags = REG_EXTENDED|REG_NEWLINE;

	/*
	 * load icase_path option.
	 */
	if (getconfb("icase_path"))
		flags |= REG_ICASE;
#if defined(_WIN32) || defined(__DJGPP__)
	flags |= REG_ICASE;
#endif
	/*
	 * initialize common data.
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
	if (!getconfs("skip", reg)) {
		strbuf_close(reg);
		return;
	}
	skiplist = check_strdup(strbuf_value(reg));
	trim(skiplist);
	strbuf_reset(reg);
	/*
	 * construct regular expression.
	 */
	strbuf_putc(reg, '(');	/* ) */
	/*
	 * Hard coded skip files:
	 * (1) files which start with '.'
	 * (2) tag files
	 */
	/* skip files which start with '.' e.g. .cvsignore */
	strbuf_puts(reg, "/\\.[^/]+$|");
	strbuf_puts(reg, "/\\.[^/]+/|");
	/* skip tag files */
	strbuf_puts(reg, "/GTAGS$|");
	strbuf_puts(reg, "/GRTAGS$|");
	strbuf_puts(reg, "/GSYMS$|");
	strbuf_puts(reg, "/GPATH$|");
	for (p = skiplist; p; ) {
		char *skipf = p;
		if ((p = locatestring(p, ",", MATCH_FIRST)) != NULL)
			*p++ = 0;
		if (*skipf == '/') {
			list_count++;
			strbuf_puts0(list, skipf);
		} else {
			reg_count++;
			strbuf_putc(reg, '/');
			for (q = skipf; *q; q++) {
				if (isregexchar(*q))
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
		int retval;

		/*
		 * compile regular expression.
		 */
		skip = &skip_area;
		retval = regcomp(skip, strbuf_value(reg), flags);
		if (retval != 0)
			die("cannot compile regular expression.");
	} else {
		skip = (regex_t *)0;
	}
	if (list_count > 0) {
		int i;
		listarray = (char **)check_malloc(sizeof(char *) * list_count);
		p = strbuf_value(list);
		for (i = 0; i < list_count; i++) {
			listarray[i] = p;
			p += strlen(p) + 1;
		}
	}
	strbuf_close(reg);
	free(skiplist);
}
/*
 * skipthisfile: check whether or not we accept this file.
 *
 *	i)	path	path name (must start with ./)
 *	r)		1: skip, 0: don't skip
 *
 * Specification of required path name.
 * o Path must start with "./".
 * o Directory path name must end with "/".
 */
static int
skipthisfile(const char *path)
{
	const char *first, *last;
	int i;

	/*
	 * unit check.
	 */
	if (skip && regexec(skip, path, 0, 0, 0) == 0) {
		return 1;
	}
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
			if (!STRNCMP(path + 1, first, last - first)) {
				return 1;
			}
		} else {
			if (!STRCMP(path + 1, first)) {
				return 1;
			}
		}
	}
	return 0;
}

/*
 * Directory Stack
 */
static char dir[MAXPATHLEN];			/* directory path */
static VARRAY *stack;				/* dynamic allocated array */
struct stack_entry {
	STRBUF *sb;
	char *real;
	char *dirp, *start, *end, *p;
};
static int current_entry;			/* current entry of the stack */

/*
 * getrealpath: return a real path of dir using allocated area.
 */
char *
getrealpath(const char *dir)
{
	char real[PATH_MAX];

	if (realpath(dir, real) == NULL)
		die("cannot get real path of '%s'.", trimpath(dir));
	return check_strdup(real);
}
/*
 * has_symlinkloop: whether or not dir has a symbolic link loops.
 *
 *	i)	dir	directory (should end by '/')
 *	r)		1: has a loop, 0: don't have a loop
 */
int
has_symlinkloop(const char *dir)
{
	struct stack_entry *sp;
	char real[PATH_MAX], *p;
	int i;

	if (!strcmp(dir, "./"))
		return 0;
	if (realpath(dir, real) == NULL)
		die("cannot get real path of '%s'.", trimpath(dir));
#ifdef SLOOPDEBUG
	fprintf(stderr, "======== has_symlinkloop ======\n");
	fprintf(stderr, "dir = '%s', real path = '%s'\n", dir, real);
	fprintf(stderr, "TEST-1\n");
	fprintf(stderr, "\tcheck '%s' < '%s'\n", real, rootdir);
#endif
	p = locatestring(rootdir, real, MATCH_AT_FIRST);
	if (p && (*p == '/' || *p == '\0' || !strcmp(real, "/")))
		return 1;
	sp = varray_assign(stack, 0, 0);
#ifdef SLOOPDEBUG
	fprintf(stderr, "TEST-2\n");
#endif
	for (i = current_entry; i >= 0; i--) {
#ifdef SLOOPDEBUG
		fprintf(stderr, "%d:\tcheck '%s' == '%s'\n", i, real, sp[i].real);
#endif
		if (!strcmp(sp[i].real, real))
			return 1;
	}
#ifdef SLOOPDEBUG
	fprintf(stderr, "===============================\n");
#endif
	return 0;
}

/*
 * getdirs: get directory list
 *
 *	i)	dir	directory (should end by '/')
 *	o)	sb	string buffer
 *	r)		-1: error, 0: normal
 *
 * format of directory list:
 * |ddir1\0ffile1\0|
 * means directory 'dir1', file 'file1'.
 */
static int
getdirs(const char *dir, STRBUF *sb)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat st;

	if (check_looplink && has_symlinkloop(dir)) {
		warning("symbolic link loop detected. '%s'. ignored.", trimpath(dir));
		return -1;
	}
	if ((dirp = opendir(dir)) == NULL) {
		warning("cannot open directory '%s'. ignored.", trimpath(dir));
		return -1;
	}
	while ((dp = readdir(dirp)) != NULL) {
		if (!strcmp(dp->d_name, "."))
			continue;
		if (!strcmp(dp->d_name, ".."))
			continue;
		if (stat(makepath(dir, dp->d_name, NULL), &st) < 0) {
			warning("cannot stat '%s'. ignored.", trimpath(dp->d_name));
			continue;
		}
		if (S_ISDIR(st.st_mode))
			strbuf_putc(sb, 'd');
		else if (S_ISREG(st.st_mode))
			strbuf_putc(sb, 'f');
		else
			strbuf_putc(sb, ' ');
		strbuf_puts(sb, dp->d_name);
		strbuf_putc(sb, '\0');
	}
	(void)closedir(dirp);
	return 0;
}
/*
 * find_open: start iterator without GPATH.
 *
 *	i)	start	start directory
 *			If NULL, assumed '.' directory.
 */
void
find_open(const char *start)
{
	struct stack_entry *curp;
	assert(find_mode == 0);
	find_mode = FIND_OPEN;

	/*
	 * This is temporary measures. It doesn't decide how to become final.
	 */
	if (getenv("GTAGSTESTING")) {
		allow_blank = 1;
		check_looplink = 1;
	}
	if (getenv("GTAGSALLOWBLANK"))
		allow_blank = 1;
	if (!start)
		start = "./";
        if (realpath(start, rootdir) == NULL)
                die("cannot get real path of '%s'.", trimpath(dir));
	/*
	 * setup stack.
	 */
	stack = varray_open(sizeof(struct stack_entry), 50);
	current_entry = 0;
	curp = varray_assign(stack, current_entry, 1);
	strlimcpy(dir, start, sizeof(dir));
	curp->dirp = dir + strlen(dir);
	curp->sb = strbuf_open(0);
	curp->real = getrealpath(dir);
	if (getdirs(dir, curp->sb) < 0)
		die("Work is given up.");
	curp->start = curp->p = strbuf_value(curp->sb);
	curp->end   = curp->start + strbuf_getlen(curp->sb);
	strlimcpy(cwddir, get_root(), sizeof(cwddir));
	/*
	 * prepare regular expressions.
	 */
	prepare_source();
	prepare_skip();
}
/*
 * find_open_filelist: find_open like interface for handling output of find(1).
 *
 *	i)	filename	file including list of file names.
 *				When "-" is specified, read from standard input.
 *	i)	root		root directory of source tree
 */
void
find_open_filelist(const char *filename, const char *root)
{
	assert(find_mode == 0);
	find_mode = FILELIST_OPEN;

	/*
	 * This is temporary measures. It doesn't decide how to become final.
	 */
	if (getenv("GTAGSTESTING")) {
		allow_blank = 1;
		check_looplink = 1;
	}
	if (getenv("GTAGSALLOWBLANK"))
		allow_blank = 1;
	if (!strcmp(filename, "-")) {
		/*
		 * If the filename is '-', copy standard input onto
		 * temporary file to be able to read repeatedly.
		 */
		if (temp == NULL) {
			char buf[MAXPATHLEN];

			temp = tmpfile();
			while (fgets(buf, sizeof(buf), stdin) != NULL)
				fputs(buf, temp);
		}
		rewind(temp);
		ip = temp;
	} else {
		ip = fopen(filename, "r");
		if (ip == NULL)
			die("cannot open '%s'.", trimpath(filename));
	}
	/*
	 * rootdir always ends with '/'.
	 */
	if (!strcmp(root+ROOT, "/"))
		strlimcpy(rootdir, root, sizeof(rootdir));
	else
		snprintf(rootdir, sizeof(rootdir), "%s/", root);
	strlimcpy(cwddir, root, sizeof(cwddir));
	/*
	 * prepare regular expressions.
	 */
	prepare_skip();
	prepare_source();
}
/*
 * find_read: read path without GPATH.
 *
 *	r)		path
 */
char *
find_read(void)
{
	static char *path;

	assert(find_mode != 0);
	if (find_eof)
		path = NULL;
	else if (find_mode == FILELIST_OPEN)
		path = find_read_filelist();
	else if (find_mode == FIND_OPEN)
		path = find_read_traverse();
	else
		die("find_read: internal error.");
	return path;
}
/*
 * find_read_traverse: read path without GPATH.
 *
 *	r)		path
 */
char *
find_read_traverse(void)
{
	static char val[MAXPATHLEN];
	char path[MAXPATHLEN];
	struct stack_entry *curp = varray_assign(stack, current_entry, 1);

	for (;;) {
		while (curp->p < curp->end) {
			char type = *(curp->p);
			const char *unit = curp->p + 1;

			curp->p += strlen(curp->p) + 1;

			/*
			 * Skip files described in the skip list.
			 */
				/* makepath() returns unsafe module local area. */
			strlimcpy(path, makepath(dir, unit, NULL), sizeof(path));
			if (type == 'd')
				strcat(path, "/");
			if (skipthisfile(path))
				continue;
			if (type == 'f') {
				/*
				 * Skip the following:
				 * o directory
				 * o file which does not exist
				 * o dead symbolic link
				 */
				if (!test("f", path)) {
					if (test("d", path))
						warning("'%s' is a directory. ignored.", trimpath(path));
					else
						warning("'%s' not found. ignored.", trimpath(path));
					continue;
				}
				/*
				 * GLOBAL cannot treat path which includes blanks.
				 * It will be improved in the future.
				 */
				if (!allow_blank && locatestring(path, " ", MATCH_FIRST)) {
					warning("'%s' ignored, because it includes blank.", trimpath(path));
					continue;
				}
				/*
				 * A blank at the head of path means
				 * other than source file.
				 */
				if (regexec(suff, path, 0, 0, 0) == 0) {
					/* source file */
					strlimcpy(val, path, sizeof(val));
				} else {
					/* other file like 'Makefile' */
					val[0] = ' ';
					strlimcpy(&val[1], path, sizeof(val) - 1);
				}
				val[sizeof(val) - 1] = '\0';
				return val;
			}
			if (type == 'd') {
				STRBUF *sb = strbuf_open(0);
				char *dirp = curp->dirp;
				strcat(dirp, unit);
				strcat(dirp, "/");
				if (getdirs(dir, sb) < 0) {
					strbuf_close(sb);
					*(curp->dirp) = 0;
					continue;
				}
				/*
				 * Push stack.
				 */
				curp = varray_assign(stack, ++current_entry, 1);
				curp->dirp = dirp + strlen(dirp);
				curp->real = getrealpath(dir);
				curp->sb = sb;
				curp->start = curp->p = strbuf_value(sb);
				curp->end   = curp->start + strbuf_getlen(sb);
			}
		}
		strbuf_close(curp->sb);
		curp->sb = NULL;
		free(curp->real);
		curp->real = NULL;
		if (current_entry == 0)
			break;
		/*
		 * Pop stack.
		 */
		curp = varray_assign(stack, --current_entry, 0);
		*(curp->dirp) = 0;
	}
	find_eof = 1;
	return NULL;
}
/*
 * find_read_filelist: read path from file
 *
 *	r)		path
 */
static char *
find_read_filelist(void)
{
	STATIC_STRBUF(ib);
	static char buf[MAXPATHLEN + 1];
	static char *path;

	strbuf_clear(ib);
	for (;;) {
		path = strbuf_fgets(ib, ip, STRBUF_NOCRLF);
		if (path == NULL) {
			/* EOF */
			find_eof = 1;
			return NULL;
		}
		if (*path == '\0') {
			/* skip empty line.  */
			continue;
		}
		/*
		 * Lines which start with ". " are considered to be comments.
		 */
		if (*path == '.' && *(path + 1) == ' ')
			continue;
		/*
		 * Skip the following:
		 * o directory
		 * o file which does not exist
		 * o dead symbolic link
		 */
		if (!test("f", path)) {
			if (test("d", path))
				warning("'%s' is a directory. ignored.", trimpath(path));
			else
				warning("'%s' not found. ignored.", trimpath(path));
			continue;
		}
		/*
		 * normalize path name.
		 *
		 *	rootdir  /a/b/
		 *	buf      /a/b/c/d.c -> c/d.c -> ./c/d.c
		 */
		if (normalize(path, rootdir, cwddir, buf, sizeof(buf)) == NULL) {
			warning("'%s' is out of source tree. ignored.", trimpath(path));
			continue;
		}
		path = buf;
		/*
		 * GLOBAL cannot treat path which includes blanks.
		 * It will be improved in the future.
		 */
		if (!allow_blank && locatestring(path, " ", MATCH_LAST)) {
			warning("'%s' ignored, because it includes blank.", trimpath(path));
			continue;
		}
		if (skipthisfile(path))
			continue;
		/*
		 * A blank at the head of path means
		 * other than source file.
		 */
		if (regexec(suff, path, 0, 0, 0) != 0)
			*--path = ' ';
		return path;
	}
}
/*
 * find_close: close iterator.
 */
void
find_close(void)
{
	assert(find_mode != 0);
	if (find_mode == FIND_OPEN) {
		if (stack)
			varray_close(stack);
	} else if (find_mode == FILELIST_OPEN) {
		/*
		 * The --file=- option is specified, we don't close file
		 * to read it repeatedly.
		 */
		if (ip != temp)
			fclose(ip);
	} else {
		die("find_close: internal error.");
	}
	regfree(suff);
	if (skip)
		regfree(skip);
	find_eof = find_mode = 0;
}
