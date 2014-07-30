/*
 * Copyright (c) 2003, 2005, 2014 Tama Communications Corporation
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

#include "conf.h"
#include "die.h"
#include "env.h"
#include "gparam.h"
#include "strbuf.h"

extern char **environ;

/**
 * set_env: put environment variable.
 *
 *	@param[in]	var	environment variable
 *	@param[in]	val	value
 *
 * Machine independent version of @XREF{setenv,3}.
 */
void
set_env(const char *var, const char *val)
{
/*
 * sparc-sun-solaris2.6 doesn't have setenv(3).
 */
#ifdef HAVE_PUTENV
	STRBUF *sb = strbuf_open(0);

	strbuf_sprintf(sb, "%s=%s", var, val);
	putenv(strbuf_value(sb));
	/* Don't free memory. putenv(3) require it. */
#else
	setenv(var, val, 1);
#endif
}
/**
 * get_home_directory: get environment dependent home directory.
 *
 *	@return	home directory
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

/**
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
/**
 * setenv_from_config
 */
static const char *envname[] = {
	"GLOBAL_OPTIONS",
	"GREP_COLOR",
	"GREP_COLORS",
	"GTAGSBLANKENCODE",
	"GTAGSCACHE",
	/*"GTAGSCONF",*/
	/*"GTAGSDBPATH",*/
	"GTAGSFORCECPP",
	"GTAGSGLOBAL",
	"GTAGSGTAGS",
	/*"GTAGSLABEL",*/
	"GTAGSLIBPATH",
	"GTAGSLOGGING",
	/*"GTAGSROOT",*/
	"GTAGSTHROUGH",
	"GTAGS_OPTIONS",
	"HTAGS_OPTIONS",
	"MAKEOBJDIR",
	"MAKEOBJDIRPREFIX",
	"TMPDIR",
};
void
setenv_from_config(void)
{
	int i, lim = sizeof(envname) / sizeof(char *);
	STRBUF *sb = strbuf_open(0);

	for (i = 0; i < lim; i++) {
		if (getenv(envname[i]) == NULL) {
			strbuf_reset(sb);
			if (getconfs(envname[i], sb))
				set_env(envname[i], strbuf_value(sb));
			else if (getconfb(envname[i]))
				set_env(envname[i], "");
		}
	}
	/*
	 * For upper compatibility.
	 * htags_options is deprecated.
	 */
	if (getenv("HTAGS_OPTIONS") == NULL) {
		strbuf_reset(sb);
		if (getconfs("htags_options", sb))
			set_env("HTAGS_OPTIONS", strbuf_value(sb));
	}
	strbuf_close(sb);
}
