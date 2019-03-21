/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006,
 *      2007, 2008, 2010, 2011, 2012, 2013, 2015
 * Tama Communications Corporation
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
#include <sys/types.h>
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
#include "compress.h"
#include "convert.h"
#include "die.h"
#include "format.h"
#include "gparam.h"
#include "makepath.h"
#include "output.h"
#include "strbuf.h"
#include "strlimcpy.h"

/**
 * Stuff for the compact format
 */
static char curpath[MAXPATHLEN];	/**< current path */
static char curtag[IDENTLEN];		/**< current tag */
static int cur_lineno;			/**< current line number */
static int last_lineno;			/**< last line number */
static FILE *fp;			/**< file descripter */
static const char *src;			/**< source code */

static int put_compact_format(CONVERT *, GTP *, const char *, int);
static void put_standard_format(CONVERT *, GTP *, int);
static int nosource;
static int format;

static STRBUF *sb_uncompress;

/** get next number and seek to the next character */
#define GET_NEXT_NUMBER(p) do {                                                \
                if (!isdigit(*p))                                              \
                        p++;                                                   \
                for (n = 0; isdigit(*p); p++)                                  \
                        n = n * 10 + (*p - '0');                               \
        } while (0)

void
start_output(int a_format, int a_nosource)
{
	format = a_format;
	nosource = a_nosource;
	curpath[0] = curtag[0] = '\0';
	cur_lineno = last_lineno = 0;
	fp = NULL;
	src = "";
	sb_uncompress = strbuf_open(0);
}
void
end_output(void)
{
	if (sb_uncompress)
		strbuf_close(sb_uncompress);
	if (fp)
		fclose(fp);
}
/**
 * output_with_formatting: pass records to the convert filter.
 *
 *	@param[in]	cv	convert descripter
 *	@param[in]	gtp	record descripter
 *	@param[in]	root	project root directory
 *	@param[in]	flags	format flags
 *	@return		outputted number of records
 */
int
output_with_formatting(CONVERT *cv, GTP *gtp, const char *root, int flags)
{
	int count = 0;

	if (format == FORMAT_PATH) {
		convert_put_path(cv, NULL, gtp->path);
		count++;
	} else if (flags & GTAGS_COMPACT) {
		count += put_compact_format(cv, gtp, root, flags);
	} else {
		put_standard_format(cv, gtp, flags);
		count++;
	}
	return count;
}
/*
 * Compact format:
 */
static int
put_compact_format(CONVERT *cv, GTP *gtp, const char *root, int flags)
{
	STATIC_STRBUF(ib);
	int count = 0;
	char *p = (char *)gtp->tagline;
	const char *fid, *tagname;
	int n = 0;

	strbuf_clear(ib);
	/*                    a          b
	 * tagline = <file id> <tag name> <line no>,...
	 */
	fid = p;
	while (*p != ' ')
		p++;
	*p++ = '\0';			/* a */
	tagname = p;
	while (*p != ' ')
		p++;
	*p++ = '\0';			/* b */
	/*
	 * Reopen or rewind source file.
	 */
	if (!nosource) {
		if (strcmp(gtp->path, curpath) != 0) {
			if (curpath[0] != '\0' && fp != NULL)
				fclose(fp);
			strlimcpy(curtag, tagname, sizeof(curtag));
			strlimcpy(curpath, gtp->path, sizeof(curpath));
			/*
			 * Use absolute path name to support GTAGSROOT
			 * environment variable.
			 */
			fp = fopen(makepath(root, curpath, NULL), "r");
			if (fp == NULL)
				warning("source file '%s' is not available.", curpath);
			last_lineno = cur_lineno = 0;
		} else if (strcmp(gtp->tag, curtag) != 0) {
			strlimcpy(curtag, gtp->tag, sizeof(curtag));
			if (atoi(p) < last_lineno && fp != NULL) {
				rewind(fp);
				cur_lineno = 0;
			}
			last_lineno = 0;
		}
	}
	/*
	 * Unfold compact format.
	 */
	if (!isdigit(*p))
		die("invalid compact format.");
	if (flags & GTAGS_COMPNAME)
		tagname = (char *)uncompress(tagname, gtp->tag, sb_uncompress);
	if (flags & GTAGS_COMPLINE) {
		/*
		 * If GTAGS_COMPLINE flag is set, each line number is expressed as
		 * the difference from the previous line number except for the head.
		 * Please see flush_pool() in libutil/gtagsop.c for the details.
		 */
		int last = 0, cont = 0;

		while (*p || cont > 0) {
			if (cont > 0) {
				n = last + 1;
				if (n > cont) {
					cont = 0;
					continue;
				}
			} else if (isdigit(*p)) {
				GET_NEXT_NUMBER(p);
			}  else if (*p == '-') {
				GET_NEXT_NUMBER(p);
				cont = n + last;
				n = last + 1;
			} else if (*p == ',') {
				GET_NEXT_NUMBER(p);
				n += last;
			}
			if (last_lineno != n && fp) {
				while (cur_lineno < n) {
					if (!(src = strbuf_fgets(ib, fp, STRBUF_NOCRLF))) {
						src = "";
						fclose(fp);
						fp = NULL;
						break;
					}
					cur_lineno++;
				}
			}
			convert_put_using(cv, tagname, gtp->path, n, src, fid);
			count++;
			last_lineno = last = n;
		}
	} else {
		/*
		 * In fact, when GTAGS_COMPACT is set, GTAGS_COMPLINE is allways set.
		 * Therefore, the following code are not actually used.
		 * However, it is left for some test.
		 */
		while (*p) {
			for (n = 0; isdigit(*p); p++)
				n = n * 10 + *p - '0';
			if (*p == ',')
				p++;
			if (last_lineno == n)
				continue;
			if (last_lineno != n && fp) {
				while (cur_lineno < n) {
					if (!(src = strbuf_fgets(ib, fp, STRBUF_NOCRLF))) {
						src = "";
						fclose(fp);
						fp = NULL;
						break;
					}
					cur_lineno++;
				}
			}
			convert_put_using(cv, tagname, gtp->path, n, src, fid);
			count++;
			last_lineno = n;
		}
	}
	return count;
}
/*
 * Standard format:
 */
static void
put_standard_format(CONVERT *cv, GTP *gtp, int flags)
{
	char *p = (char *)gtp->tagline;
	char namebuf[IDENTLEN];
	const char *fid, *tagname, *image;

	/*                    a          b         c
	 * tagline = <file id> <tag name> <line no> <line image>
	 */
	fid = p;
	while (*p != ' ')
		p++;
	*p++ = '\0';			/* a */
	tagname = p;
	while (*p != ' ')
		p++;
	*p++ = '\0';			/* b */
	if (flags & GTAGS_COMPNAME) {
		strlimcpy(namebuf, (char *)uncompress(tagname, gtp->tag, sb_uncompress), sizeof(namebuf));
		tagname = namebuf;
	}
	if (nosource) {
		image = " ";
	} else {
		while (*p != ' ')
			p++;
		image = p + 1;		/* c + 1 */
		if (flags & GTAGS_COMPRESS)
			image = (char *)uncompress(image, gtp->tag, sb_uncompress);
	}
	convert_put_using(cv, tagname, gtp->path, gtp->lineno, image, fid);
}
