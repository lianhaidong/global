/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2004
 *	Tama Communications Corporation
 * #ifdef __DJGPP__
 * Contributed by Jason Hood <jadoxa@yahoo.com.au>, 2001.
 # #endif
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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef __DJGPP__
#include <fcntl.h>			/* for _USE_LFN */
#endif

#include "gparam.h"
#include "path.h"
#include "strlimcpy.h"

/*
 * isabspath: whether absolute path or not
 *
 *	i)	path	path
 *	r)		1: absolute, 0: not absolute
 */
int
isabspath(p)
	const char *p;
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

/*
 * canonpath: make canonical path name.
 *
 *	io)	path	path
 *	r)		path
 *
 * Note: canonpath rewrite argument buffer.
 */
char *
canonpath(path)
	char *path;
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
/*
 * realpath: get the complete path
 */
char *
realpath(in_path, out_path)
	const char *in_path;
	char *out_path;
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
