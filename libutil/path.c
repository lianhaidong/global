/*
 * Copyright (c) 1997, 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2001, 2002 Tama Communications Corporation
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include <dos.h>			/* for intdos() */
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
	char *p;
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
		/* Ensure we're using a complete long name, not a mixture
		 * of long and short.
		 */
		union REGS regs;
		regs.x.ax = 0x7160;
		regs.x.cx = 0x8002;
		regs.x.si = (unsigned)path;
		regs.x.di = (unsigned)path;
		intdos( &regs, &regs );
		/*
		 * A non-existant file returns error code 3; get the path,
		 * strip the filename, LFN the path and put the filename back.
		 */
		if (regs.x.cflag && regs.h.al == 3) {
			char filename[261];
			regs.x.ax = 0x7160;
			regs.h.cl = 0;
			intdos(&regs, &regs);
			p = basename(path);
			strlimcpy(filename, p, sizeof(filename));
			*p = 0;
			regs.x.ax = 0x7160;
			regs.h.cl = 2;
			intdos( &regs, &regs );
			strcat(path, filename);
		}
	}
	/* Lowercase the drive letter and convert to slashes. */
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

#ifdef __DJGPP__
/*
 * realpath: get the complete path
 */
char *
realpath(in_path, out_path)
	char *in_path;
	char *out_path;
{
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
	return out_path;
}
#endif
