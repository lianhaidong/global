/*
 * Copyright (c) 2003, 2005 Tama Communications Corporation
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
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_HOME_ETC_H
#include <home_etc.h>
#endif

#include "die.h"
#include "env.h"
#include "strbuf.h"

extern char **environ;

#if !defined(ARG_MAX) && defined(_SC_ARG_MAX)
#define ARG_MAX         sysconf(_SC_ARG_MAX)
#endif

/*
 * set_env: put environment variable.
 *
 *	i)	var	environment variable
 *	i)	val	value
 */
void
set_env(var, val)
	const char *var;
	const char *val;
{
#ifdef HAVE_PUTENV
	STRBUF *sb = strbuf_open(0);

	strbuf_sprintf(sb, "%s=%s", var, val);
	putenv(strbuf_value(sb));
	/* Don't free memory. putenv(3) require it. */
#else
	setenv(var, val, 1);
#endif
}
/*
 * get_home_directory: get environment dependent home directory.
 *
 *	r)	home directory
 */
char *
get_home_directory(void)
{
#ifdef HAVE_HOME_ETC_H
	return _HEdir;
#else
	return getenv("HOME");
#endif
}

/*
 * env_size: calculate the size of area used by environment.
 */
int
env_size(void)
{
	char **e;
	int size = 0;

	for (e = environ; *e != NULL; e++)
		size += strlen(*e) + 1;

	return size;
}

/*
 * exec_line_limit: upper limit of bytes of exec line.
 *
 *	i)	length	command line length
 *	r)	0: unknown or cannot afford long line.
 *		> 0: upper limit of exec line
 */
int
exec_line_limit(length)
	int length;
{
	int limit = 0;

#ifdef ARG_MAX
	/*
	 * POSIX.2 limits the exec(2) line length to ARG_MAX - 2048.
	 */
	limit = ARG_MAX - 2048;
	/*
	 * The reason is unknown but the xargs(1) in GNU findutils
	 * use this limit.
	 */
	if (limit > 20 * 1024)
		limit = 20 * 1024;
	/*
	 * Add the command line length.
	 * We estimates additional 80 bytes for popen(3) and space for
	 * the additional sort command.
	 *
	 * for "/bin/sh -c "				11bytes
	 * for " | gnusort -k 1,1 -k 3,3 -k 2,2n"	32bytes
	 * reserve					37bytes
	 * ----------------------------------------------------
	 * Total					80 bytes
	 */
	limit -= length + 80;

	limit -= env_size();
	if (limit < 0)
		limit = 0;
#endif
	return limit;
}
