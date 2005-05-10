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
#ifdef HAVE_HOME_ETC_H
#include <home_etc.h>
#endif

#include "die.h"
#include "env.h"
#include "strbuf.h"

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
