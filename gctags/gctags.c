/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000, 2001
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
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "getopt.h"

#include "global.h"
#include "gctags.h"

const char *progname = "gctags";	/* program name */
char	*notfunction;

int	main(int, char **);
static	void usage(void);

struct words *words;
static int tablesize;

int	bflag;			/* -b: force level 1 block start */
int	dflag;			/* -d: treat #define with no argument */
int	eflag;			/* -e: force level 1 block end */
int	nflag;			/* -n: doen't print tag */
int	rflag;			/* -r: function reference */
int	sflag;			/* -s: collect symbols */
int	tflag;			/* -t: treat typedefs, structs, unions, and enums. */
int	wflag;			/* -w: warning message */
int	vflag;			/* -v: verbose mode */
int	show_version;
int	show_help;
int	debug;

int	yaccfile;		/* yacc file */

static void
usage()
{
	(void)fprintf(stderr, "usage: gctags [-bdenrstw] file ...\n");
	exit(2);
}

static struct option const long_options[] = {
	{"begin-block", no_argument, NULL, 'b'},
	{"define", no_argument, NULL, 'd'},
	{"end-block", no_argument, NULL, 'e'},
	{"no-tags", no_argument, NULL, 'n'},
	{"reference", no_argument, NULL, 'r'},
	{"symbol", no_argument, NULL, 's'},
	{"typedef", no_argument, NULL, 't'},
	{"warning", no_argument, NULL, 'w'},

	/* long name only */
	{"debug", no_argument, &debug, 1},
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},
	{ 0 }
};

int
main(argc, argv)
	int	argc;
	char	**argv;
{
	int	optchar;
	char	*p;

	while ((optchar = getopt_long(argc, argv, "bdenrstw", long_options, NULL)) != EOF) {
		switch(optchar) {
		case 0:
			break;
		case 'b':
			bflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 'e':
			eflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'r':
			rflag++;
			sflag = 0;
			break;
		case 's':
			sflag++;
			rflag = 0;
			break;
		case 't':
			tflag++;
			break;
		case 'w':
			wflag++;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	if (show_version)
		version(NULL, 0);
	else if (show_help)
		usage();

        argc -= optind;
        argv += optind;

	if (argc < 1)
		usage();
	if (getenv("GTAGSWARNING"))
		wflag++;	
	if (test("r", NOTFUNCTION)) {
		FILE	*ip;
		STRBUF	*sb = strbuf_open(0);
		STRBUF	*ib = strbuf_open(0);
		char	*p;
		int	i;

		if ((ip = fopen(NOTFUNCTION, "r")) == 0)
			die("'%s' cannot read.", NOTFUNCTION);
		for (tablesize = 0; (p = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL; tablesize++)
			strbuf_puts0(sb, p);
		fclose(ip);
		if ((words = (struct words *)malloc(sizeof(struct words) * tablesize)) == NULL)
			die("short of memory.");
		/*
		 * Don't free *p.
		 */
		p = (char *)malloc(strbuf_getlen(sb) + 1);
		memcpy(p, strbuf_value(sb), strbuf_getlen(sb) + 1);
		for (i = 0; i < tablesize; i++) {
			words[i].name = p;
			p += strlen(p) + 1;
		}
		qsort(words, tablesize, sizeof(struct words), cmp);
		strbuf_close(sb);
		strbuf_close(ib);
	}
	for (; argc > 0; argv++, argc--) {
		if (!opentoken(argv[0]))
			die("'%s' cannot open.", argv[0]);
#if defined(_WIN32) || defined(__DJGPP__)
		/* Lower case the file name since names are case insensitive */
		strlwr(argv[0]);
#endif
		/*
		 * yacc
		 */
		if (locatestring(argv[0], ".y", MATCH_AT_LAST))
			C(YACC);
		/*
		 * assembler
		 */
		else if (locatestring(argv[0], ".s", MATCH_AT_LAST)	||
			locatestring(argv[0], ".S", MATCH_AT_LAST))
			assembler();
		/*
		 * java
		 */
		else if (locatestring(argv[0], ".java", MATCH_AT_LAST))
			java();
		/*
		 * C++
		 */
		else if (locatestring(argv[0], ".c++", MATCH_AT_LAST)	||
			locatestring(argv[0], ".cc", MATCH_AT_LAST)	||
			locatestring(argv[0], ".cpp", MATCH_AT_LAST)	||
			locatestring(argv[0], ".cxx", MATCH_AT_LAST)	||
			locatestring(argv[0], ".hxx", MATCH_AT_LAST)	||
			locatestring(argv[0], ".C", MATCH_AT_LAST)	||
			locatestring(argv[0], ".H", MATCH_AT_LAST))
			Cpp();
		/*
		 * C
		 */
		else if (locatestring(argv[0], ".c", MATCH_AT_LAST))
			C(0);
		/*
		 * C or C++
		 */
		else if (locatestring(argv[0], ".h", MATCH_AT_LAST)) {
			if (isCpp())
				Cpp();
			else
				C(0);
		}
		closetoken();
	}
	return 0;
}

int
cmp(s1, s2)
	const void *s1, *s2;
{
	return strcmp(((struct words *)s1)->name, ((struct words *)s2)->name);
}

int
isnotfunction(name)
	char	*name;
{
	struct words tmp;
	struct words *result;

	if (words == NULL)
		return 0;
	tmp.name = name;
	result = (struct words *)bsearch(&tmp, words, tablesize, sizeof(struct words), cmp);
	return (result != NULL) ? 1 : 0;
}

void
dbg_print(level, s)
	int level;
	const char *s;
{
	if (!debug)
		return;
	fprintf(stderr, "[%04d]", lineno);
	for (; level > 0; level--)
		fprintf(stderr, "    ");
	fprintf(stderr, "%s\n", s);
}
