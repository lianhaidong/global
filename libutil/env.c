/*
 * Copyright (c) 2003 Tama Communications Corporation
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
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_HOME_ETC_H
#include <home_etc.h>
#endif

#include "die.h"
#include "env.h"

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
	char *env;
	/*
	 * extra 2 bytes needed for '=' and '\0'.
	 *
	 * putenv("TMPDIR=/tmp");
	 * TMPDIR=/tmp\0
	 */
	env = (char *)malloc(strlen(var)+strlen(val)+2);
	if (!env)
		die("short of memory.");
	strcpy(env, var);
	strcat(env, "=");
	strcat(env, val);
	putenv(env);
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
get_home_directory()
{
#ifdef HAVE_HOME_ETC_H
	return _HEdir;
#else
	return getenv("HOME");
#endif
}
