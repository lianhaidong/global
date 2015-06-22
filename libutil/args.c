/*
 * Copyright (c) 2010, 2014 Tama Communications Corporation
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
#include <ctype.h>
#include <stdio.h>
#include "checkalloc.h"
#include "die.h"
#include "env.h"
#include "locatestring.h"
#include "strbuf.h"
#include "test.h"
#include "gpathop.h"

#define ARGS_NOP	0
#define ARGS_ARGS	1
#define ARGS_FILELIST	2
#define ARGS_GFIND	3
#define ARGS_BOTH	4

int type;
const char **argslist;
FILE *ip;
GFIND *gp;

/**
 * args_open:
 *
 *	@param[in]	args	args array
 */
void
args_open(const char **args)
{
	type = ARGS_ARGS;
	argslist = args;
}
/**
 * args_open_filelist: args_open like interface for handling output of find(1).
 *
 *	@param[in]	filename	file including list of file names.
 *				When "-" is specified, read from standard input.
 */
void
args_open_filelist(const char *filename)
{
	type = ARGS_FILELIST;
	if (!strcmp(filename, "-")) {
		ip = stdin;
	} else {
		ip = fopen(filename, "r");
		if (ip == NULL)
			die("cannot open '%s'.", filename);
	}
}
/**
 * args_open_both: args_open like interface for argument and file list.
 *
 *	@param[in]	args		args array
 *	@param[in]	filename	file including list of file names.
 *				When "-" is specified, read from standard input.
 */
void
args_open_both(const char **args, const char *filename)
{
	type = ARGS_BOTH;
	argslist = args;
	if (!strcmp(filename, "-")) {
		ip = stdin;
	} else {
		ip = fopen(filename, "r");
		if (ip == NULL)
			die("cannot open '%s'.", filename);
	}
}
/**
 * args_open_gfind: args_open like interface for handling output of gfind.
 *
 *	@param[in]	agp	GFIND descriptor
 */
void
args_open_gfind(GFIND *agp)
{
	type = ARGS_GFIND;
	gp = agp;
}
void
args_open_nop(void)
{
	type = ARGS_NOP;
}
/**
 * args_read: read path From args.
 *
 *	@return		path (NULL: end of argument)
 */
const char *
args_read(void)
{
	const char *p;
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	switch (type) {
	case ARGS_NOP:
		p = NULL;
		break;
	case ARGS_ARGS:
		p = *argslist++;
		break;
	case ARGS_FILELIST:
		p = strbuf_fgets(sb, ip, STRBUF_NOCRLF);
		break;
	case ARGS_GFIND:
		p = gfind_read(gp);
		break;
	case ARGS_BOTH:
		if (*argslist != NULL)
			p = *argslist++;
		else
			p = strbuf_fgets(sb, ip, STRBUF_NOCRLF);
		break;
	default:
		die("args_read: invalid type.");
	}
	return p;
}
/**
 * args_close: close args.
 */
void
args_close(void)
{
	switch (type) {
	case ARGS_NOP:
	case ARGS_ARGS:
		break;
	case ARGS_FILELIST:
	case ARGS_BOTH:
		if (ip != NULL && ip != stdin)
			fclose(ip);
		ip = NULL;
		break;
	case ARGS_GFIND:
		if (gp != NULL)
			gfind_close(gp);
		gp = NULL;
		break;
	default:
		die("something wrong.");
	}
}
/**
 * preparse_options
 *
 *	@param[in]	argc	main()'s argc integer
 *	@param[in]	argv	main()'s argv string array
 *
 * Setup the "GTAGSCONF" and the "GTAGSLABEL" environment variables
 * according to the --gtagsconf and --gtagslabel options.
 */
void
preparse_options(int argc, char *const *argv)
{
	int i;
	char *p;
	char *confpath = NULL;
	char *label = NULL;
	const char *opt_gtagsconf = "--gtagsconf";
	const char *opt_gtagslabel = "--gtagslabel";

	for (i = 1; i < argc; i++) {
		if ((p = locatestring(argv[i], opt_gtagsconf, MATCH_AT_FIRST))) {
			if (*p == '\0') {
				if (++i >= argc)
					die("%s needs an argument.", opt_gtagsconf);
				confpath = argv[i];
			} else {
				if (*p++ == '=' && *p)
					confpath = p;
			}
		} else if ((p = locatestring(argv[i], opt_gtagslabel, MATCH_AT_FIRST))) {
			if (*p == '\0') {
				if (++i >= argc)
					die("%s needs an argument.", opt_gtagslabel);
				label = argv[i];
			} else {
				if (*p++ == '=' && *p)
					label = p;
			}
		}
	}
	if (confpath) {
		char real[MAXPATHLEN];

		if (!test("f", confpath))
			die("%s file not found.", opt_gtagsconf);
		if (!realpath(confpath, real))
			die("cannot get absolute path of %s file.", opt_gtagsconf);
		set_env("GTAGSCONF", real);
	}
	if (label)
		set_env("GTAGSLABEL", label);
}
/**
 * prepend_options: creates a new argv main() array, by prepending (space separated)
 *		options and arguments from the string argument options.
 *
 *	@param[in,out]	argc	pointer to main()'s argc integer
 *	@param[in]	argv	main()'s argv string array
 *	@param[in]	options	string
 *	@return	The new argv array.
 *
 *	The program's name is copied back into: returned[0] (argv[0]).
 */
char **
prepend_options(int *argc, char *const *argv, const char *options)
{
	STRBUF *sb = strbuf_open(0);
	const char *p, *opt = check_strdup(options);
	int count = 1;
	int quote = 0;
	const char **newargv;
	int i = 0, j = 1;

	for (p = opt; *p && isspace(*p); p++)
		;
	for (; *p; p++) {
		int c = *p;

		if (quote) {
			if (quote == c)
				quote = 0;
			else
				strbuf_putc(sb, c);
		} else if (c == '\\') {
			if (*(p + 1))
				strbuf_putc(sb, *++p);
		} else if (c == '\'' || c == '"') {
			quote = c;
		} else if (isspace(c)) {
			strbuf_putc(sb, '\0');
			count++;
			while (*p && isspace(*p))
				p++;
			p--;
		} else {
			strbuf_putc(sb, *p);
		}
	}
	newargv = (const char **)check_malloc(sizeof(char *) * (*argc + count + 1));
	newargv[i++] = argv[0];
	p = strbuf_value(sb);
	while (count--) {
		newargv[i++] = p;
		p += strlen(p) + 1;
	}
	while (j < *argc)
		newargv[i++] = argv[j++];
	newargv[i] = NULL;
	*argc = i;
#ifdef DEBUG
	for (i = 0; i < *argc; i++)
		fprintf(stderr, "newargv[%d] = '%s'\n", i, newargv[i]);
#endif
	/* doesn't close string buffer. */

	return (char **)newargv;
}
