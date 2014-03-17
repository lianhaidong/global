/*
 * Copyright (c) 2005, 2006, 2010 Tama Communications Corporation
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
#include <stdio.h>
#include <ctype.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "abs2rel.h"
#include "checkalloc.h"
#include "die.h"
#include "encodepath.h"
#include "format.h"
#include "gparam.h"
#include "gpathop.h"
#include "gtagsop.h"
#include "convert.h"
#include "strbuf.h"
#include "strlimcpy.h"

static int newline = '\n';
/**
 * set_print0: change newline to @CODE{'\0'}.
 */
void
set_print0(void)
{
	newline = '\0';
}
/**
 * Path filter for the output of @XREF{global,1}.
 */
static const char *
convert_pathname(CONVERT *cv, const char *path)
{
	static char buf[MAXPATHLEN];
	const char *a, *b;

	if (cv->type != PATH_THROUGH) {
		/*
		 * make absolute path name.
		 * 'path + 1' means skipping "." at the head.
		 */
		strbuf_setlen(cv->abspath, cv->start_point);
		strbuf_puts(cv->abspath, path + 1);
		/*
		 * print path name with converting.
		 */
		switch (cv->type) {
		case PATH_ABSOLUTE:
			path = strbuf_value(cv->abspath);
			break;
		case PATH_RELATIVE:
		case PATH_SHORTER:
			a = strbuf_value(cv->abspath);
			b = cv->basedir;
#if defined(_WIN32) || defined(__DJGPP__)
			while (*a != '/')
				a++;
			while (*b != '/')
				b++;
#endif
			if (!abs2rel(a, b, buf, sizeof(buf)))
				die("abs2rel failed. (path=%s, base=%s).", a, b);
			path = buf;
			if (cv->type == PATH_SHORTER && strlen(path) > strbuf_getlen(cv->abspath))
				path = strbuf_value(cv->abspath);
			break;
		default:
			die("unknown path type.");
			break;
		}
	}
	/*
	 * encoding of the path name.
	 */
	if (use_encoding()) {
		const char *p;
		int required = 0;

		for (p = path; *p; p++) {
			if (required_encode(*p)) {
				required = 1;
				break;
			}
		}
		if (required) {
			static char buf[MAXPATHLEN];
			char c[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
			char *q = buf;

			for (p = path; *p; p++) {
				if (required_encode(*p)) {
					*q++ = '%';
					*q++ = c[*p / 16];
					*q++ = c[*p % 16];
				} else
					*q++ = *p;
			}
			*q = '\0';
			path = buf;
		}
	}
	return (const char *)path;
}
/**
 * convert_open: open convert filter
 *
 *	@param[in]	type	#PATH_ABSOLUTE, #PATH_RELATIVE, #PATH_THROUGH
 *	@param[in]	format	tag record format
 *	@param[in]	root	root directory of source tree
 *	@param[in]	cwd	current directory
 *	@param[in]	dbpath	dbpath directory
 *	@param[in]	op	output file
 *	@param[in]	db	tag type (#GTAGS, #GRTAGS, #GSYMS, #GPATH, #NOTAGS) <br>
 *			only for @NAME{cscope} format
 */
CONVERT *
convert_open(int type, int format, const char *root, const char *cwd, const char *dbpath, FILE *op, int db)
{
	CONVERT *cv = (CONVERT *)check_calloc(sizeof(CONVERT), 1);
	/*
	 * set base directory.
	 */
	cv->abspath = strbuf_open(MAXPATHLEN);
	strbuf_puts(cv->abspath, root);
	strbuf_unputc(cv->abspath, '/');
	cv->start_point = strbuf_getlen(cv->abspath);
	/*
	 * copy elements.
	 */
	if (strlen(cwd) > MAXPATHLEN)
		die("current directory name too long.");
	strlimcpy(cv->basedir, cwd, sizeof(cv->basedir));
	cv->type = type;
	cv->format = format;
	cv->op = op;
	cv->db = db;
	/*
	 * open GPATH.
	 */
	if (gpath_open(dbpath, 0) < 0)
		die("GPATH not found.");
	return cv;
}
/**
 * convert_put: convert path into relative or absolute and print.
 *
 *	@param[in]	cv	#CONVERT structure
 *	@param[in]	ctags_x	tag record (@NAME{ctags-x} format)
 *
 * @note This function is only called by @NAME{gtags} with the @OPTION{--path} option.
 */
void
convert_put(CONVERT *cv, const char *ctags_x)
{
	char *tagnextp = NULL;
	int tagnextc = 0;
	char *tag = NULL, *lineno = NULL, *path, *rest = NULL;
	const char *fid = NULL;

	if (cv->format == FORMAT_PATH)
		die("convert_put: internal error.");	/* Use convert_put_path() */
	/*
	 * parse tag line.
	 * Don't use split() function not to destroy line image.
	 */
	{
		char *p = (char *)ctags_x;
		/*
		 * tag name
		 */
		tag = p;
		for (; *p && !isspace(*p); p++)
			;
		if (*p == '\0')
			die("illegal ctags-x format (line number not found).");
		tagnextp = p;
		tagnextc = *p;
		*p++ = '\0';
		/* skip blanks */
		for (; *p && isspace(*p); p++)
			;
		if (*p == '\0')
			die("illegal ctags-x format (line number not found).");
		/*
		 * line number
		 */
		lineno = p;
		for (; *p && !isspace(*p); p++)
			;
		if (*p == '\0')
			die("illegal ctags-x format (path name not found).");
		*p++ = '\0';
		/* skip blanks */
		for (; *p && isspace(*p); p++)
			;
		if (*p == '\0')
			die("illegal ctags-x format (path name not found).");
		/*
		 * path name
		 */
		path = p;
		for (; *p && !isspace(*p); p++)
			;
		if (*p == '\0')
			die("illegal ctags-x format (line image not found).");
		*p++ = '\0';
		rest = p;
	}
	/*
	 * The path name has already been encoded.
	 */
	path = decode_path(path);
	switch (cv->format) {
	case FORMAT_CTAGS:
		fputs(tag, cv->op);
		fputc('\t', cv->op);
		fputs(convert_pathname(cv, path), cv->op);
		fputc('\t', cv->op);
		fputs(lineno, cv->op);
		break;
	case FORMAT_CTAGS_XID:
		fid = gpath_path2fid(path, NULL);
		if (fid == NULL)
			die("convert_put: unknown file. '%s'", path);
		fputs(fid, cv->op);
		fputc(' ', cv->op);
		/* PASS THROUGH */
	case FORMAT_CTAGS_X:
		/*
		 * print until path name.
		 */
		*tagnextp = tagnextc;
		fputs(ctags_x, cv->op);
		fputc(' ', cv->op);
		/*
		 * print path name and the rest.
		 */
		fputs(convert_pathname(cv, path), cv->op);
		fputc(' ', cv->op);
		fputs(rest, cv->op);
		break;
	case FORMAT_CTAGS_MOD:
		fputs(convert_pathname(cv, path), cv->op);
		fputc('\t', cv->op);
		fputs(lineno, cv->op);
		fputc('\t', cv->op);
		fputs(rest, cv->op);
		break;
	case FORMAT_GREP:
		fputs(convert_pathname(cv, path), cv->op);
		fputc(':', cv->op);
		fputs(lineno, cv->op);
		fputc(':', cv->op);
		fputs(rest, cv->op);
		break;
	case FORMAT_CSCOPE:
		fputs(convert_pathname(cv, path), cv->op);
		fputc(' ', cv->op);
		fputs(tag, cv->op);
		fputc(' ', cv->op);
		fputs(lineno, cv->op);
		fputc(' ', cv->op);
		for (; *rest && isspace(*rest); rest++)
			;
		if (*rest)
			fputs(rest, cv->op);
		else
			fputs("<unknown>", cv->op);
		break;
	default:
		die("unknown format type.");
	}
	(void)fputc(newline, cv->op);
}
/**
 * convert_put_path: convert @a path into relative or absolute and print.
 *
 *	@param[in]	cv	#CONVERT structure
 *	@param[in]	path	path name
 */
void
convert_put_path(CONVERT *cv, const char *path)
{
	if (cv->format != FORMAT_PATH)
		die("convert_put_path: internal error.");
	fputs(convert_pathname(cv, path), cv->op);
	(void)fputc(newline, cv->op);
}
/**
 * convert_put_using: convert @a path into relative or absolute and print.
 *
 *	@param[in]	cv	#CONVERT structure
 *      @param[in]      tag     tag name
 *      @param[in]      path    path name
 *      @param[in]      lineno  line number
 *      @param[in]      rest    line image
 *	@param[in]	fid	file id (only when @CODE{fid != NULL})
 */
void
convert_put_using(CONVERT *cv, const char *tag, const char *path, int lineno, const char *rest, const char *fid)
{
	switch (cv->format) {
	case FORMAT_PATH:
		fputs(convert_pathname(cv, path), cv->op);
		break;
	case FORMAT_CTAGS:
		fputs(tag, cv->op);
		fputc('\t', cv->op);
		fputs(convert_pathname(cv, path), cv->op);
		fputc('\t', cv->op);
		fprintf(cv->op, "%d", lineno);
		break;
	case FORMAT_CTAGS_XID:
		if (fid == NULL) {
			fid = gpath_path2fid(path, NULL);
			if (fid == NULL)
				die("convert_put_using: unknown file. '%s'", path);
		}
		fputs(fid, cv->op);
		fputc(' ', cv->op);
		/* PASS THROUGH */
	case FORMAT_CTAGS_X:
		fprintf(cv->op, "%-16s %4d %-16s %s",
			tag, lineno, convert_pathname(cv, path), rest);
		break;
	case FORMAT_CTAGS_MOD:
		fputs(convert_pathname(cv, path), cv->op);
		fputc('\t', cv->op);
		fprintf(cv->op, "%d", lineno);
		fputc('\t', cv->op);
		fputs(rest, cv->op);
		break;
	case FORMAT_GREP:
		fputs(convert_pathname(cv, path), cv->op);
		fputc(':', cv->op);
		fprintf(cv->op, "%d", lineno);
		fputc(':', cv->op);
		fputs(rest, cv->op);
		break;
	case FORMAT_CSCOPE:
		fputs(convert_pathname(cv, path), cv->op);
		fputc(' ', cv->op);
		fputs(tag, cv->op);
		fputc(' ', cv->op);
		fprintf(cv->op, "%d", lineno);
		fputc(' ', cv->op);
		for (; *rest && isspace(*rest); rest++)
			;
		if (*rest)
			fputs(rest, cv->op);
		else
			fputs("<unknown>", cv->op);
		break;
	default:
		die("unknown format type.");
	}
	(void)fputc(newline, cv->op);
}
void
convert_close(CONVERT *cv)
{
	strbuf_close(cv->abspath);
	gpath_close();
	free(cv);
}
