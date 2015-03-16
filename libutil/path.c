/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2004, 2008, 2011,
 *	2014
 *	Tama Communications Corporation
 * #ifdef __DJGPP__
 * Contributed by Jason Hood <jadoxa@yahoo.com.au>, 2001.
 # #endif
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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __DJGPP__
#include <fcntl.h>			/* for _USE_LFN */
#endif

#include "gparam.h"
#include "path.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "test.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
#define mkdir(path,mode) mkdir(path)
#endif


/**
 * isabspath: whether absolute path or not
 *
 *	@param[in]	p	path
 *	@return		1: absolute, 0: not absolute
 */
int
isabspath(const char *p)
{
	if (p[0] == '/')
		return 1;
#if defined(_WIN32) || defined(__DJGPP__)
	if (p[0] == '\\')
		return 1;
	if (isdrivechar(p[0]) && p[1] == ':' && (p[2] == '\\' || p[2] == '/'))
		return 1;
#endif
	return 0;
}

/**
 * canonpath: make canonical path name.
 *
 *	@param[in,out]	path	path
 *	@return		path
 *
 * @attention canonpath() rewrite argument buffer.
 */
char *
canonpath(char *path)
{
#ifdef __DJGPP__
	char *p;

	if (_USE_LFN) {
		char name[260], sfn[13];
		char *base;

		/*
		 * Ensure we're using a complete long name, not a mixture
		 * of long and short.
		 */
		_truename(path, path);
		/*
		 * _truename will successfully convert the path of a non-
		 * existant file, but it's probably still a mixture of long and
		 * short components - convert the path separately.
		 */
		if (access(path, F_OK) != 0) {
			base = basename(path);
			strcpy(name, base);
			*base = '\0';
			_truename(path, path);
			strcat(path, name);
		}
		/*
		 * Convert the case of 8.3 names, as other djgpp functions do.
		 */
		if (!_preserve_fncase()) {
			for (p = path+3, base = p-1; *base; p++) {
				if (*p == '\\' || *p == '\0') {
					memcpy(name, base+1, p-base-1);
					name[p-base-1] = '\0';
					if (!strcmp(_lfn_gen_short_fname(name, sfn), name)) {
						while (++base < p)
							if (*base >= 'A' && *base <= 'Z')
								*base += 'a' - 'A';
					} else
					   base = p;
				}
			}
		}
	}
	/*
	 * Lowercase the drive letter and convert to slashes.
	 */
	path[0] = tolower(path[0]);
	for (p = path+2; *p; ++p)
		if (*p == '\\')
			*p = '/';
#else
#ifdef _WIN32
	char *p, *s;
	p = path;
	/*
	 * Change \ to / in a path (for DOS/Windows paths)
	 */
	while ((p = strchr(p, '\\')) != NULL)
		*p = '/';
#ifdef __CYGWIN__
	/*
	 * On NT with CYGWIN, getcwd can return something like
	 * "//c/tmp", which isn't usable. We change that to "c:/tmp".
	 */
	p = path;
	if (p[0] == '/' && p[1] == '/' && isdrivechar(p[2]) && p[3] == '/') {
		s = &p[2];		/* point drive char */
		*p++ = *s++;
		*p++ = ':';
		while (*p++ = *s++)
			;
	}
#endif /* __CYGWIN__ */
#endif /* _WIN32 */
#endif /* __DJGPP__ */
	return path;
}

#if (defined(_WIN32) && !defined(__CYGWIN__)) || defined(__DJGPP__)
/**
 * realpath: get the complete path
 *
 * @param[in]	in_path
 * @param[out]	out_path	result string
 * @return	@a out_path
 */
char *
realpath(const char *in_path, char *out_path)
{
#ifdef __DJGPP__
	/*
	 * I don't use _fixpath or _truename in LFN because neither guarantee
	 * a complete long name. This is mainly DOS's fault, since the cwd can
	 * be a mixture of long and short components.
	 */
	if (_USE_LFN) {
		strlimcpy(out_path, in_path, MAXPATHLEN);
		canonpath(out_path);
	} else
		_fixpath(in_path, out_path);
#else
	_fullpath(out_path, in_path, MAXPATHLEN);
	canonpath(out_path);
#endif
	return out_path;
}
#endif

#define SEP '/'

/**
 * makedirectories: make directories on the path like @XREF{mkdir,1} with the @OPTION{-p} option.
 *
 *	@param[in]	base	base directory
 *	@param[in]	rest	path from the base
 *	@param[in]	verbose 1: verbose mode, 0: not verbose mode
 *	@return		0: success <br>
 *			-1: base directory not found <br>
 *			-2: permission error <br>
 *			-3: cannot make directory
 *
 *	@remark Directories are created in mode 0775.
 */
int
makedirectories(const char *base, const char *rest, int verbose)
{
	STRBUF *sb;
	const char *p, *q;

	if (!test("d", base))
		return -1;
	if (!test("drw", base))
		return -2;
	sb = strbuf_open(0);
	strbuf_puts(sb, base);
	if (*rest == SEP)
		rest++;
	for (q = rest; *q;) {
		p = q;
		while (*q && *q != SEP)
			q++;
		strbuf_putc(sb, SEP);
		strbuf_nputs(sb, p, q - p);
		p = strbuf_value(sb);
		if (!test("d", p)) {
			if (verbose)
				fprintf(stderr, " Making directory '%s'.\n", p);
			if (mkdir(p, 0775) < 0) {
				strbuf_close(sb);
				return -3;
			}
		}
		if (*q == SEP)
			q++;
	}
	strbuf_close(sb);
	return 0;
}
/**
 * trimpath: trim path name
 *
 * @remark Just skips over @CODE{"./"} at the beginning of @a path, if present.
 */
const char *
trimpath(const char *path)
{
	if (*path == '.' && *(path + 1) == '/')
		path += 2;
	return path;
}
/**
 * get the current directory
 *
 * @param[out]	buf	result string
 * @param[in]	size	size of @a buf
 * @return	@a buf or NULL
 */
char *
vgetcwd(char *buf, size_t size) {
	char *p;

	if (getenv("GTAGSLOGICALPATH")) {
		if ((p = getenv("PWD")) != NULL) {
			strlimcpy(buf, p, size);
			return buf;
		}
	}
	if (getcwd(buf, size) != NULL)
		return buf;
	return NULL;
}
