/*
 * Copyright (c) 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2001, 2002, 2003 Tama Communications Corporation
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
#include "const.h"

int main(int, char **);
static void usage(void);
static void help(void);
static int match_suffix_list(const char *, const char *);

struct words {
        const char *name;
        int val;
};
struct words *words;
static int tablesize;

/*
 * language map.
 *
 * By default, default_map is used.
 */
static const char *default_map = "c:.c.h,yacc:.y,asm:.s.S,java:.java,cpp:.c++.cc.cpp.cxx.hxx.hpp.C.H,php:.php.php3.phtml";
static char *langmap;
static STRBUF *active_map;

int bflag;			/* -b: force level 1 block start */
int dflag;			/* -d: treat #define with no argument */
int eflag;			/* -e: force level 1 block end */
int nflag;			/* -n: doen't print tag */
int rflag;			/* -r: function reference */
int sflag;			/* -s: collect symbols */
int tflag;			/* -t: treat typedefs, structs, unions, and enums. */
int wflag;			/* -w: warning message */
int vflag;			/* -v: verbose mode */
int do_check;
int show_version;
int show_help;
int debug;

int yaccfile;		/* yacc file */

static void
usage()
{
	fputs(usage_const, stderr);
	exit(2);
}
static void
help()
{
	fputs(usage_const, stdout);
	fputs(help_const, stdout);
	exit(0);
}
static struct option const long_options[] = {
	{"begin-block", no_argument, NULL, 'b'},
	{"check", no_argument, &do_check, 1},
	{"define", no_argument, NULL, 'd'},
	{"end-block", no_argument, NULL, 'e'},
	{"no-tags", no_argument, NULL, 'n'},
	{"reference", no_argument, NULL, 'r'},
	{"symbol", no_argument, NULL, 's'},
	{"typedef", no_argument, NULL, 't'},
	{"verbose", no_argument, NULL, 'v'},
	{"warning", no_argument, NULL, 'w'},

	/* long name only */
	{"debug", no_argument, &debug, 1},
	{"langmap", required_argument, NULL, 0},
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},
	{ 0 }
};

int
main(argc, argv)
	int argc;
	char **argv;
{
	char *p;
	int flag;
	int optchar;
	int option_index = 0;

	while ((optchar = getopt_long(argc, argv, "bdenrstvw", long_options, &option_index)) != EOF) {
		switch(optchar) {
		case 0:
			p = (char *)long_options[option_index].name;
			if (!strcmp(p, "langmap"))
				langmap = optarg;	
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
		case 'v':
			vflag++;
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
		version(NULL, vflag);
	else if (show_help)
		help();
	else if (do_check) {
		fprintf(stdout, "Part of GLOBAL\n");
		exit(0);
	}
	/*
	 * construct language map.
	 */
	active_map = strbuf_open(0);
	strbuf_puts(active_map, (langmap) ? langmap : default_map);
	flag = 0;
	for (p = strbuf_value(active_map); *p; p++) {
		/*
		 * "c:.c.h,java:.java,cpp:.C.H"
		 */
		if ((flag == 0 && *p == ',') || (flag == 1 && *p == ':'))
			die_with_code(2, "syntax error in --langmap option.");
		if (*p == ':' || *p == ',') {
			flag ^= 1;
			*p = 0;
		}
	}
	if (flag == 0)
		die_with_code(2, "syntax error in --langmap option.");

        argc -= optind;
        argv += optind;

	if (argc < 1)
		usage();
	if (getenv("GTAGSWARNING"))
		wflag++;	
	/*
	 * This is a hack for FreeBSD.
	 * In the near future, it will be removed.
	 */
#ifdef __DJGPP__
	if (test("r", NOTFUNCTION) || test("r", DOS_NOTFUNCTION))
#else
	if (test("r", NOTFUNCTION))
#endif
	{
		FILE *ip;
		STRBUF *sb = strbuf_open(0);
		STRBUF *ib = strbuf_open(0);
		char *p;
		int i;

		if ((ip = fopen(NOTFUNCTION, "r")) == 0)
#ifdef __DJGPP__
			if ((ip = fopen(DOS_NOTFUNCTION, "r")) == 0)
#endif
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
		if (p == NULL)
			die("short of memory.");
		memcpy(p, strbuf_value(sb), strbuf_getlen(sb) + 1);
		for (i = 0; i < tablesize; i++) {
			words[i].name = p;
			p += strlen(p) + 1;
		}
		qsort(words, tablesize, sizeof(struct words), cmp);
		strbuf_close(sb);
		strbuf_close(ib);
	}

	/*
	 * pick up files and parse them.
	 */
	for (; argc > 0; argv++, argc--) {
		char *lang = NULL;
		char *suffix, *list, *tail;

#if defined(_WIN32) || defined(__DJGPP__)
		/* Lower case the file name since names are case insensitive */
		strlwr(argv[0]);
#endif
		/* get suffix of the path. */
		suffix = locatestring(argv[0], ".", MATCH_LAST);
		if (!suffix)
			continue;
		list = strbuf_value(active_map);
		tail = list + strbuf_getlen(active_map);

		/* check whether or not list includes suffix. */
		while (list < tail) {
			lang = list;
			list = lang + strlen(lang) + 1;
			if (match_suffix_list(suffix, list))
				break;
			lang = NULL;
			list += strlen(list) + 1;
		}
		if (lang == NULL)
			continue;

		/*
		 * Initialize token parser. Php() use different parser.
		 */
		if (strcmp(lang, "php"))
			if (!opentoken(argv[0]))
				die("'%s' cannot open.", argv[0]);

		if (!strcmp(suffix, ".h") && isCpp())
			lang = "cpp";
		if (vflag)
			fprintf(stderr, "suffix '%s' assumed language '%s'.\n", suffix, lang);

		/*
		 * call language specific parser.
		 */
		if (!strcmp(lang, "c")) {
			C(0);
		} else if (!strcmp(lang, "yacc")) {
			C(YACC);
		} else if (!strcmp(lang, "asm")) {
			assembler();
		} else if (!strcmp(lang, "java")) {
			java();
		} else if (!strcmp(lang, "cpp")) {
			Cpp();
		} else if (!strcmp(lang, "php")) {
			php(argv[0]);
		}
		if (strcmp(lang, "php"))
			closetoken();
	}
	return 0;
}

/*
 * return true if suffix matches with one in suffix list.
 */
int
match_suffix_list(suffix, list)
	const char *suffix;
	const char *list;
{
	while (*list) {
		if (locatestring(list, suffix, MATCH_AT_FIRST))
			return 1;
		for (list++; *list && *list != '.'; list++)
			;
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
	char *name;
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
