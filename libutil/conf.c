/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000
 *             Tama Communications Corporation. All rights reserved.
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
#include <assert.h>
#include <ctype.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "gparam.h"
#include "conf.h"
#include "die.h"
#include "locatestring.h"
#include "makepath.h"
#include "strbuf.h"
#include "strmake.h"
#include "test.h"

static FILE	*fp;
static STRBUF	*ib;
static char	*line;
static int	allowed_nest_level = 8;
static int	opened;

static void	trim(char *);
static char	*readrecord(const char *);
static void	includelabel(STRBUF *, const char *, int);

#ifndef isblank
#define isblank(c)	((c) == ' ' || (c) == '\t')
#endif

static void
trim(l)
char	*l;
{
	char	*f, *b;
	int	colon = 0;

	for (f = b = l; *f; f++) {
		if (colon && isblank(*f))
			continue;
		colon = 0;
		if ((*b++ = *f) == ':')
			colon = 1;
	}
	*b = 0;
}
static char	*
readrecord(label)
const char *label;
{
	char	*p, *q;
	int	flag = STRBUF_NOCRLF;
	int	line = 0;

	rewind(fp);
	while ((p = strbuf_fgets(ib, fp, flag)) != NULL) {
		line++;
		flag &= ~STRBUF_APPEND;
		if (*p == '#' || *p == '\0')
			continue;
		if (strbuf_unputc(ib, '\\')) {
			flag |= STRBUF_APPEND;
			continue;
		}
		trim(p);
		for (;;) {
			if ((q = strmake(p, "|:")) == NULL)
				die("invalid config file format (line %d).", line);
			if (!strcmp(label, q)) {
				if (!(p = locatestring(p, ":", MATCH_FIRST)))
					die("invalid config file format (line %d).", line);
				p = strdup(p);
				if (!p)
					die("short of memory.");
				return p;
			}
			p += strlen(q);
			if (*p == ':')
				break;
			else if (*p == '|')
				p++;
			else
				die("invalid config file format (line %d).", line);
		}
	}
	return NULL;
}
static	void
includelabel(sb, label, level)
STRBUF	*sb;
const char *label;
int	level;
{
	char	*savep, *p, *q;

	if (++level > allowed_nest_level)
		die("nested include= (or tc=) over flow.");
	if (!(savep = p = readrecord(label)))
		die("label '%s' not found.", label);
	while ((q = locatestring(p, ":include=", MATCH_FIRST)) || (q = locatestring(p, ":tc=", MATCH_FIRST))) {
		char	inclabel[MAXPROPLEN+1], *c = inclabel;

		strbuf_nputs(sb, p, q - p);
		q = locatestring(q, "=", MATCH_FIRST) + 1;
		while (*q && *q != ':')
			*c++ = *q++;
		*c = 0;
		includelabel(sb, inclabel, level);
		p = q;
	}
	strbuf_puts(sb, p);
	free(savep);
}
/*
 * configpath: get path of configuration file.
 */
static char *
configpath() {
	static char config[MAXPATHLEN+1];
	char *p;

	if ((p = getenv("HOME")) && test("r", makepath(p, GTAGSRC, NULL)))
		strcpy(config, makepath(p, GTAGSRC, NULL));
	else if (test("r", GTAGSCONF))
		strcpy(config, GTAGSCONF);
	else if (test("r", OLD_GTAGSCONF))
		strcpy(config, OLD_GTAGSCONF);
	else if (test("r", DEBIANCONF))
		strcpy(config, DEBIANCONF);
	else if (test("r", OLD_DEBIANCONF))
		strcpy(config, OLD_DEBIANCONF);
	else
		return NULL;
	return config;
}
/*
 * openconf: load configuration file.
 *
 *	go)	line	specified entry
 */
void
openconf()
{
	STRBUF *sb;
	char *config;
	extern int vflag;

	assert(opened == 0);
	opened = 1;

	/*
	 * if GTAGSCONF not set then check standard config files.
	 */
	if ((config = getenv("GTAGSCONF")) == NULL)
		config = configpath();
	/*
	 * if config file not found then return default value.
	 */
	if (!config) {
		if (vflag)
			fprintf(stderr, " Using default configuration.\n");
		line = "";
	}
	/*
	 * if it doesn't start with '/' then assumed config value itself.
	 */
	else if (*config != '/') {
		line = strdup(config);
	}
	/*
	 * else load value from config file.
	 */
	else {
		const char *label;

		if (test("d", config))
			die("config file '%s' is a directory.", config);
		if (!test("f", config))
			die("config file '%s' not found.", config);
		if (!test("r", config))
			die("config file '%s' is not readable.", config);
		if ((label = getenv("GTAGSLABEL")) == NULL)
			label = "default";
	
		if (!(fp = fopen(config, "r")))
			die("cannot open '%s'.", config);
		if (vflag)
			fprintf(stderr, " Using config file '%s'.\n", config);
		ib = strbuf_open(MAXBUFLEN);
		sb = strbuf_open(0);
		includelabel(sb, label, 0);
		line = strdup(strbuf_value(sb));
		strbuf_close(ib);
		strbuf_close(sb);
		fclose(fp);
	}
	/*
	 * make up lacked variables.
	 */
	sb = strbuf_open(0);
	strbuf_puts(sb, line);
	strbuf_unputc(sb, ':');
	if (!getconfs("suffixes", NULL)) {
		strbuf_puts(sb, ":suffixes=");
		strbuf_puts(sb, DEFAULTSUFFIXES);
	}
	if (!getconfs("skip", NULL)) {
		strbuf_puts(sb, ":skip=");
		strbuf_puts(sb, DEFAULTSKIP);
	}
	/*
	 * GTAGS, GRTAGS and GSYMS have no default values but non of them
	 * specified then use default values.
	 * (Otherwise, nothing to do for gtags.)
	 */
	if (!getconfs("GTAGS", NULL) && !getconfs("GRTAGS", NULL) && !getconfs("GSYMS", NULL)) {
		strbuf_puts(sb, ":GTAGS=gctags %s");
		strbuf_puts(sb, ":GRTAGS=gctags -r %s");
		strbuf_puts(sb, ":GSYMS=gctags -s %s");
	}
	if (!getconfs("sort_command", NULL))
		strbuf_puts(sb, ":sort_command=sort");
	if (!getconfs("sed_command", NULL))
		strbuf_puts(sb, ":sed_command=sed");
	strbuf_unputc(sb, ':');
	strbuf_putc(sb, ':');
	line = strdup(strbuf_value(sb));
	strbuf_close(sb);
	if (!line)
		die("short of memory.");
	return;
}
/*
 * getconfn: get property number
 *
 *	i)	name	property name
 *	o)	num	value (if not NULL)
 *	r)		1: found, 0: not found
 */
int
getconfn(name, num)
const char *name;
int	*num;
{
	char	*p;
	char	buf[MAXPROPLEN+1];

	if (!opened)
		openconf();
#ifdef HAVE_SNPRINTF
	snprintf(buf, sizeof(buf), ":%s#", name);
#else
	sprintf(buf, ":%s#", name);
#endif /* HAVE_SNPRINTF */
	if ((p = locatestring(line, buf, MATCH_FIRST)) != NULL) {
		p += strlen(buf);
		if (num != NULL)
			*num = atoi(p);
		return 1;
	}
	return 0;
}
/*
 * getconfs: get property string
 *
 *	i)	name	property name
 *	o)	sb	string buffer (if not NULL)
 *	r)		1: found, 0: not found
 */
int
getconfs(name, sb)
const char *name;
STRBUF	*sb;
{
	char	*p;
	char	buf[MAXPROPLEN+1];
	int	all = 0;
	int	exist = 0;

	if (!opened)
		openconf();
	if (!strcmp(name, "suffixes") || !strcmp(name, "skip"))
		all = 1;
#ifdef HAVE_SNPRINTF
	snprintf(buf, sizeof(buf), ":%s=", name);
#else
	sprintf(buf, ":%s=", name);
#endif /* HAVE_SNPRINTF */
	p = line;
	while ((p = locatestring(p, buf, MATCH_FIRST)) != NULL) {
		if (exist && sb)
			strbuf_putc(sb, ',');		
		exist = 1;
		for (p += strlen(buf); *p && *p != ':'; p++) {
			if (*p == '\\')	/* quoted charactor */
				p++;
			if (sb)
				strbuf_putc(sb, *p);
		}
		if (!all)
			break;
	}
	return exist;
}
/*
 * getconfb: get property bool value
 *
 *	i)	name	property name
 *	r)		1: TRUE, 0: FALSE
 */
int
getconfb(name)
const char *name;
{
	char	buf[MAXPROPLEN+1];

	if (!opened)
		openconf();
#ifdef HAVE_SNPRINTF
	snprintf(buf, sizeof(buf), ":%s:", name);
#else
	sprintf(buf, ":%s:", name);
#endif /* HAVE_SNPRINTF */
	if (locatestring(line, buf, MATCH_FIRST) != NULL)
		return 1;
	return 0;
}
/*
 * getconfline: print loaded config entry.
 */
char *
getconfline()
{
	if (!opened)
		openconf();
	return line;
}
void
closeconf()
{
	if (!opened)
		return;
	free(line);
	line = NULL;
	opened = 0;
}
