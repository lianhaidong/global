/*
 * Copyright (c) 2006
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
#include <stdio.h>
#include "die.h"
#include "format.h"
#include "pathconvert.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "tagsort.h"
#include "usable.h"

#include "filter.h"

/*
 * Output filter
 *
 * (1) External filter
 *
 * Old architecture (- GLOBAL-4.7.8)
 *
 * process1          process2       process3
 * +=============+  +===========+  +===========+
 * |global(write)|->|sort filter|->|path filter|->[stdout]
 * +=============+  +===========+  +===========+
 *
 * If macro EXTERNAL_FILTER is defined, this architecture will be selected.
 *
 * (2) Internal filter (DEFAULT)
 *
 * New architecture (GLOBAL-5.0 -)
 *
 * 1 process
 * +============================================+
 * |global(write) ->[sort filter]->[path filter]|->[stdout]
 * +============================================+
 *
 * This it the default architecture.
 * Function with_pathfilter() plays the role of the pipe between
 * the sort filter and path filter.
 */
/*
 * data for internal filter
 */
/* for both filter */
int format;
/* for sort filter */
int unique;
int passthru;
/* path filter */
int type;
int fileid;

/*
 * data for external filter
 */
char root[MAXPATHLEN+1];
char cwd[MAXPATHLEN+1];
char dbpath[MAXPATHLEN+1];

STRBUF *sortfilter;                     /* sort filter          */
STRBUF *pathfilter;                     /* path convert filter  */
FILE *op;				/* output file		*/

extern int nofilter;
/*----------------------------------------------------------------------*/
/* External filter							*/
/*----------------------------------------------------------------------*/
static const char *
get_gtags()
{
	char *gtags = usable("gtags");

	if (gtags == NULL)
		die("gtags not found.");
	return gtags;
}
/*
 * make_external_sortfilter: make external sort filter
 */
static void
makesortfilter(int format, int unique, int passthru)
{
	if (sortfilter == NULL)
		sortfilter = strbuf_open(0);
	else
		strbuf_reset(sortfilter);
	/*
	 * If passthru is 1, nothing to do.
	 */
	if (passthru)
		return;

	strbuf_puts(sortfilter, get_gtags());
	strbuf_puts(sortfilter, " --sort");
	/*
	 * construct path filter
	 */
	switch (format) {
	case FORMAT_CTAGS:		/* ctags format */
		strbuf_puts(sortfilter, " --format=ctags");
		break;
	case FORMAT_CTAGS_X:		/* ctags -x format */
		strbuf_puts(sortfilter, " --format=ctags-x");
		break;
	case FORMAT_PATH:		/* only file name */
		strbuf_puts(sortfilter, " --format=path");
		break;
	default:
		break;
	}
	if (unique)
		strbuf_puts(sortfilter, " --unique");
}
/*
 * makepathfilter: make external path filter
 */
static void
makepathfilter(int format, int type, int fileid, const char *root, const char *cwd, const char *dbpath)
{
	if (pathfilter == NULL)
		pathfilter = strbuf_open(0);
	else
		strbuf_reset(pathfilter);
	strbuf_puts(pathfilter, get_gtags());

	/*
	 * construct path filter
	 */
	switch (type) {
	case PATH_ABSOLUTE:	/* absolute path name */
		strbuf_puts(pathfilter, " --path=absolute");
		break;
	case PATH_RELATIVE:	/* relative path name */
		strbuf_puts(pathfilter, " --path=relative");
		break;
	default:
		break;
	}
	switch (format) {
	case FORMAT_CTAGS:		/* ctags format */
		strbuf_puts(pathfilter, " --format=ctags");
		break;
	case FORMAT_CTAGS_X:		/* ctags -x format */
		strbuf_puts(pathfilter, " --format=ctags-x");
		break;
	case FORMAT_PATH:		/* only file name */
		strbuf_puts(pathfilter, " --format=path");
		break;
	default:
		break;
	}
	if (fileid)
		strbuf_puts(pathfilter, " --fileid");
	strbuf_putc(pathfilter, ' ');
	strbuf_puts(pathfilter, root);
	strbuf_putc(pathfilter, ' ');
	strbuf_puts(pathfilter, cwd);
	strbuf_putc(pathfilter, ' ');
	strbuf_puts(pathfilter, dbpath);
}
/*
 * makefilter: make filter string.
 *
 *	o)	filter buffer
 */
void
makefilter(STRBUF *sb)
{
	int set_sortfilter = 0;

	if (!(nofilter & SORT_FILTER) && strbuf_getlen(sortfilter) > 0) {
		strbuf_puts(sb, strbuf_value(sortfilter));
		set_sortfilter = 1;
	}
	if (!(nofilter & PATH_FILTER) && strbuf_getlen(pathfilter) > 0) {
		if (set_sortfilter)
			strbuf_puts(sb, " | ");
		strbuf_puts(sb, strbuf_value(pathfilter));
	}
}
/*
 * getsortfilter: get sort filter string
 */
const char *
getsortfilter()
{
	return strbuf_value(sortfilter);
}
/*
 * getpathfilter: get sort filter string
 */
const char *
getpathfilter()
{
	return strbuf_value(pathfilter);
}
/*
 * makesortfilter: make sort filter
 */
void
setup_sortfilter(int a_format, int a_unique, int a_passthru)
{
	makesortfilter(a_format, a_unique, a_passthru);
	format = a_format;
	unique = a_unique;
	passthru = a_passthru;
}
/*
 * makepathfilter: make path filter
 */
void
setup_pathfilter(int a_format, int a_type, int a_fileid, const char *a_root, const char *a_cwd, const char *a_dbpath)
{
	makepathfilter(format, a_type, a_fileid, a_root, a_cwd, a_dbpath);
	format = a_format;
	type = a_type;
	fileid = a_fileid;
	strlimcpy(root, a_root, sizeof(root));
	strlimcpy(cwd, a_cwd, sizeof(cwd));
	strlimcpy(dbpath, a_dbpath, sizeof(dbpath));
}

#ifdef EXTERNAL_FILTER
/*
 * filter_open: open output filter.
 *
 *	gi)	pathfilter
 *	gi)	sortfilter
 *	r)		tagsort structure
 */
void
filter_open(void)
{
	STRBUF *sb = strbuf_open(0);

	op = stdout;
	makefilter(sb);
	if (strbuf_getlen(sb) > 0) {
		op = popen(strbuf_value(sb), "w");
		if (op == NULL)
			die("cannot open output filter. '%s'", strbuf_value(sb));
	}
	strbuf_close(sb);
}
void
filter_put(const char *s)
{
	fputs(s, op);
	fputc('\n', op);
}
/*
 * filter_close: close output filter.
 *
 *	i)	ts	tagsort structure
 */
void
filter_close(void)
{
	if (op != stdout)
		if (pclose(op) != 0)
			die("terminated abnormally.");
}
#else

/*----------------------------------------------------------------------*/
/* Internal filter							*/
/*----------------------------------------------------------------------*/
TAGSORT *ts;
CONVERT *cv;

void
with_pathfilter(const char *s)
{
	convert_put(cv, s);
}
void
without_pathfilter(const char *s)
{
	fputs(s, stdout);
	fputc('\n', stdout);
}
/*
 * filter_open: open output filter.
 */
void
filter_open(void)
{
	void (*output)(const char *) = (nofilter & PATH_FILTER) ?
				without_pathfilter : with_pathfilter;
			
	cv = convert_open(type, format, fileid, root, cwd, dbpath, stdout);
	ts = tagsort_open(output, format, unique, passthru);
}
void
filter_put(const char *s)
{
	tagsort_put(ts, s);
}
/*
 * filter_close: close output filter.
 */
void
filter_close()
{
	/*
	 * You must call tagsort_close() first, because some records may
	 * be left in the sort buffer.
	 */
	tagsort_close(ts);
	convert_close(cv);
	ts = NULL;
	cv = NULL;
}
#endif /* EXTERNAL_FILTER */
