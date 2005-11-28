/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005
 *	Tama Communications Corporation
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
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <ctype.h>
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
#include "regex.h"
#include "const.h"

static void usage(void);
static void help(void);
static void setcom(int);
int main(int, char **);
void makefilter(STRBUF *);
FILE *openfilter(void);
void closefilter(FILE *);
void completion(const char *, const char *, const char *);
void idutils(const char *, const char *);
void grep(const char *, const char *);
void pathlist(const char *, const char *);
void parsefile(int, char **, const char *, const char *, const char *, int);
void printtag(FILE *, const char *);
int search(const char *, const char *, const char *, int);
void ffformat(char *, int, const char *);

STRBUF *sortfilter;			/* sort filter		*/
STRBUF *pathfilter;			/* path convert filter	*/
const char *localprefix;		/* local prefix		*/
int aflag;				/* [option]		*/
int cflag;				/* command		*/
int fflag;				/* command		*/
int gflag;				/* command		*/
int Gflag;				/* [option]		*/
int iflag;				/* [option]		*/
int Iflag;				/* command		*/
int lflag;				/* [option]		*/
int nflag;				/* [option]		*/
int oflag;				/* [option]		*/
int pflag;				/* command		*/
int Pflag;				/* command		*/
int qflag;				/* [option]		*/
int rflag;				/* [option]		*/
int sflag;				/* [option]		*/
int tflag;				/* [option]		*/
int Tflag;				/* [option]		*/
int uflag;				/* command		*/
int vflag;				/* [option]		*/
int xflag;				/* [option]		*/
int show_version;
int show_help;
int show_filter;			/* undocumented command */
int nofilter;
int nosource;				/* undocumented command */
int debug;
int devel;
int fileid;
const char *extra_options;

static void
usage(void)
{
	if (!qflag)
		fputs(usage_const, stderr);
	exit(2);
}
static void
help(void)
{
	fputs(usage_const, stdout);
	fputs(help_const, stdout);
	exit(0);
}

#define SORT_FILTER	1
#define PATH_FILTER	2
#define BOTH_FILTER	(SORT_FILTER|PATH_FILTER)
#define SHOW_FILTER	128

static struct option const long_options[] = {
	{"absolute", no_argument, NULL, 'a'},
	{"completion", no_argument, NULL, 'c'},
	{"regexp", required_argument, NULL, 'e'},
	{"file", no_argument, NULL, 'f'},
	{"local", no_argument, NULL, 'l'},
	{"nofilter", optional_argument, NULL, 'n'},
	{"grep", no_argument, NULL, 'g'},
	{"basic-regexp", no_argument, NULL, 'G'},
	{"ignore-case", no_argument, NULL, 'i'},
	{"idutils", no_argument, NULL, 'I'},
	{"other", no_argument, NULL, 'o'},
	{"print-dbpath", no_argument, NULL, 'p'},
	{"path", no_argument, NULL, 'P'},
	{"quiet", no_argument, NULL, 'q'},
	{"reference", no_argument, NULL, 'r'},
	{"rootdir", no_argument, NULL, 'r'},
	{"symbol", no_argument, NULL, 's'},
	{"tags", no_argument, NULL, 't'},
	{"through", no_argument, NULL, 'T'},
	{"update", no_argument, NULL, 'u'},
	{"verbose", no_argument, NULL, 'v'},
	{"cxref", no_argument, NULL, 'x'},

	/* long name only */
	{"debug", no_argument, &debug, 1},
	{"devel", no_argument, &devel, 1},
	{"fileid", no_argument, &fileid, 1},
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},
	{"filter", optional_argument, NULL, SHOW_FILTER},
	{"nosource", no_argument, &nosource, 1},
	{ 0 }
};

static int command;
static void
setcom(int c)
{
	if (command == 0)
		command = c;
	else if (command != c)
		usage();
}
int
main(int argc, char **argv)
{
	const char *av = NULL;
	int count;
	int db;
	int optchar;
	int option_index = 0;
	char cwd[MAXPATHLEN+1];			/* current directory	*/
	char root[MAXPATHLEN+1];		/* root of source tree	*/
	char dbpath[MAXPATHLEN+1];		/* dbpath directory	*/
	const char *gtags;

	while ((optchar = getopt_long(argc, argv, "ace:ifgGIlnopPqrstTuvx", long_options, &option_index)) != EOF) {
		switch (optchar) {
		case 0:
			if (!strcmp("idutils", long_options[option_index].name))
				extra_options = optarg;
			break;
		case 'a':
			aflag++;
			break;
		case 'c':
			cflag++;
			setcom(optchar);
			break;
		case 'e':
			av = optarg;
			break;
		case 'f':
			fflag++;
			xflag++;
			setcom(optchar);
			break;
		case 'l':
			lflag++;
			break;
		case 'n':
			nflag++;
			if (optarg) {
				if (!strcmp(optarg, "sort"))
					nofilter = SORT_FILTER;
				else if (!strcmp(optarg, "path"))
					nofilter = PATH_FILTER;
			} else {
				nofilter = BOTH_FILTER;;
			}
			break;
		case 'g':
			gflag++;
			setcom(optchar);
			break;
		case 'G':
			Gflag++;
			break;
		case 'i':
			iflag++;
			break;
		case 'I':
			Iflag++;
			setcom(optchar);
			break;
		case 'o':
			oflag++;
			break;
		case 'p':
			pflag++;
			setcom(optchar);
			break;
		case 'P':
			Pflag++;
			setcom(optchar);
			break;
		case 'q':
			qflag++;
			setquiet();
			break;
		case 'r':
			rflag++;
			break;
		case 's':
			sflag++;
			break;
		case 't':
			tflag++;
			break;
		case 'T':
			Tflag++;
			break;
		case 'u':
			uflag++;
			setcom(optchar);
			break;
		case 'v':
			vflag++;
			break;
		case 'x':
			xflag++;
			break;
		case SHOW_FILTER:
			if (optarg) {
				if (!strcmp(optarg, "sort"))
					show_filter = SORT_FILTER;
				else if (!strcmp(optarg, "path"))
					show_filter = PATH_FILTER;
			} else {
				show_filter = BOTH_FILTER;
			}
			break;
		default:
			usage();
			break;
		}
	}
	if (qflag)
		vflag = 0;
	if (show_help)
		help();

	argc -= optind;
	argv += optind;
	/*
	 * At first, we pickup pattern from -e option. If it is not found
	 * then use argument which is not option.
	 */
	if (!av)
		av = (argc > 0) ? *argv : NULL;

	if (show_version)
		version(av, vflag);
	/*
	 * invalid options.
	 */
	if (sflag && rflag)
		die_with_code(2, "both of -s and -r are not allowed.");
	/*
	 * only -c, -u, -P and -p allows no argument.
	 */
	if (!av && !show_filter) {
		switch (command) {
		case 'c':
		case 'u':
		case 'p':
		case 'P':
			break;
		default:
			usage();
			break;
		}
	}
	/*
	 * -u and -p cannot have any arguments.
	 */
	if (av) {
		switch (command) {
		case 'u':
		case 'p':
			usage();
		default:
			break;
		}
	}
	if (fflag)
		lflag = 0;
	if (tflag)
		xflag = 0;
	if (nflag > 1)
		nosource = 1;	/* to keep compatibility */
	/*
	 * remove leading blanks.
	 */
	if (!Iflag && !gflag && av)
		for (; *av == ' ' || *av == '\t'; av++)
			;
	if (cflag && av && isregex(av))
		die_with_code(2, "only name char is allowed with -c option.");
	/*
	 * get path of following directories.
	 *	o current directory
	 *	o root of source tree
	 *	o dbpath directory
	 *
	 * if GTAGS not found, getdbpath doesn't return.
	 */
	getdbpath(cwd, root, dbpath, (pflag && vflag));
	if (Iflag && !test("f", makepath(root, "ID", NULL)))
		die("You must have id-utils's index at the root of source tree.");
	/*
	 * print dbpath or rootdir.
	 */
	if (pflag) {
		fprintf(stdout, "%s\n", (rflag) ? root : dbpath);
		exit(0);
	}
	/*
	 * incremental update of tag files.
	 */
	gtags = usable("gtags");
	if (!gtags)
		die("gtags command not found.");
	gtags = strdup(gtags);
	if (!gtags)
		die("short of memory.");
	if (uflag) {
		STRBUF	*sb = strbuf_open(0);

		if (chdir(root) < 0)
			die("cannot change directory to '%s'.", root);
		strbuf_puts(sb, gtags);
		strbuf_puts(sb, " -i");
		if (vflag)
			strbuf_putc(sb, 'v');
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, dbpath);
		if (system(strbuf_value(sb)))
			exit(1);
		strbuf_close(sb);
		exit(0);
	}

	/*
	 * complete function name
	 */
	if (cflag) {
		completion(dbpath, root, av);
		exit(0);
	}
	/*
	 * make local prefix.
	 */
	if (lflag) {
		char	*p = cwd + strlen(root);
		STRBUF	*sb = strbuf_open(0);
		/*
		 * getdbpath() assure follows.
		 * cwd != "/" and cwd includes root.
		 */
		strbuf_putc(sb, '.');
		if (*p)
			strbuf_puts(sb, p);
		strbuf_putc(sb, '/');
		localprefix = strdup(strbuf_value(sb));
		if (!localprefix)
			die("short of memory.");
		strbuf_close(sb);
	} else {
		localprefix = NULL;
	}
	/*
	 * Decide tag type.
	 */
	db = (rflag) ? GRTAGS : ((sflag) ? GSYMS : GTAGS);
	/*
	 * make sort filter.
	 */
	{
		int unique = 0;
		const char *sort;

		/*
		 * We cannot depend on the command PATH, because global(1)
		 * might be called from WWW server. Since usable() looks for
		 * the command in BINDIR directory first, POSIX_SORT need not
		 * be in command PATH.
		 */
		sort = usable(POSIX_SORT);
		if (!sort)
			die("%s not found.", POSIX_SORT);
		sortfilter = strbuf_open(0);
		/*
		 * The --devel option is for test.
		 */
		if (devel) {
			if (tflag) { 			/* ctags format */
				strbuf_puts(sortfilter, "gtags --sort --format=ctags");
				if (sflag)
					strbuf_puts(sortfilter, " --unique");
			} else if (fflag) {
				STRBUF *sb = strbuf_open(0);
				/*
				 * By default, the -f option need sort filter,
				 * because there is a possibility that an external
				 * parser is used instead of gtags-parser(1).
				 * Clear the sort filter when understanding that
				 * the parser is gtags-parser.
				 */
				if (!getconfs(dbname(db), sb))
					die("cannot get parser for %s.", dbname(db));
				if (!locatestring(strbuf_value(sb), "gtags-parser", MATCH_FIRST)) {
					strbuf_puts(sortfilter, sort);
					strbuf_puts(sortfilter, " -k 3,3 -k 2,2n");
				}
				strbuf_close(sb);
			} else if (gflag || Pflag) {
				;	/* doesn't use sort filter */
			} else if (xflag) {		/* print details */
				strbuf_puts(sortfilter, "gtags --sort --format=ctags-x");
				if (sflag)
					strbuf_puts(sortfilter, " --unique");
			} else {		/* print just a file name */
				strbuf_puts(sortfilter, "gtags --sort --format=path");
			}
		} else {
			strbuf_puts(sortfilter, sort);
			if (sflag) {
				strbuf_puts(sortfilter, " -u");
				unique = 1;
			}
			if (tflag) 			/* ctags format */
				strbuf_puts(sortfilter, " -k 1,1 -k 2,2 -k 3,3n");
			else if (fflag) {
				STRBUF *sb = strbuf_open(0);
				/*
				 * By default, the -f option need sort filter,
				 * because there is a possibility that an external
				 * parser is used instead of gtags-parser(1).
				 * Clear the sort filter when understanding that
				 * the parser is gtags-parser.
				 */
				if (!getconfs(dbname(db), sb))
					die("cannot get parser for %s.", dbname(db));
				if (locatestring(strbuf_value(sb), "gtags-parser", MATCH_FIRST))
					strbuf_setlen(sortfilter, 0);
				else
					strbuf_puts(sortfilter, " -k 3,3 -k 2,2n");
				strbuf_close(sb);
			} else if (gflag || Pflag)
				strbuf_setlen(sortfilter, 0);
			else if (xflag) {		/* print details */
				strbuf_puts(sortfilter, " -k 1,1 -k 3,3 -k 2,2n");
			} else if (!unique)		/* print just a file name */
				strbuf_puts(sortfilter, " -u");
		}
	}
	/*
	 * make path filter.
	 */
	pathfilter = strbuf_open(0);
	strbuf_puts(pathfilter, gtags);
	if (aflag)	/* absolute path name */
		strbuf_puts(pathfilter, " --path=absolute");
	else
		strbuf_puts(pathfilter, " --path=relative");
	if (tflag)
		strbuf_puts(pathfilter, " --format=ctags");
	else if (xflag)
		strbuf_puts(pathfilter, " --format=ctags-x");
	else
		strbuf_puts(pathfilter, " --format=path");
	if (fileid)
		strbuf_puts(pathfilter, " --fileid");
	strbuf_putc(pathfilter, ' ');
	strbuf_puts(pathfilter, root);
	strbuf_putc(pathfilter, ' ');
	strbuf_puts(pathfilter, cwd);
	strbuf_putc(pathfilter, ' ');
	strbuf_puts(pathfilter, dbpath);
	/*
	 * print filter.
	 */
	if (show_filter) {
		STRBUF  *sb;

		switch (show_filter) {
		case SORT_FILTER:
			if (!(nofilter & SORT_FILTER))
				fprintf(stdout, "%s\n", strbuf_value(sortfilter));
			break;
		case PATH_FILTER:
			if (!(nofilter & PATH_FILTER))
				fprintf(stdout, "%s\n", strbuf_value(pathfilter));
			break;
		case BOTH_FILTER:
			sb = strbuf_open(0);
			makefilter(sb);
			fprintf(stdout, "%s\n", strbuf_value(sb));
			strbuf_close(sb);
			break;
		default:
			die("internal error in show_filter.");
		}
		exit(0);
	}
	/*
	 * Use C locale in order to avoid the degradation of performance
	 * by internationalized sort command.
	 */
	set_env("LC_ALL", "C");
	/*
	 * exec lid(id-utils).
	 */
	if (Iflag) {
		chdir(root);
		idutils(av, dbpath);
		exit(0);
	}
	/*
	 * grep the pattern in a source tree.
	 */
	if (gflag) {
		chdir(root);
		grep(dbpath, av);
		exit(0);
	}
	/*
	 * locate the path including the pattern in a source tree.
	 */
	if (Pflag) {
		chdir(root);
		pathlist(dbpath, av);
		exit(0);
	}
	/*
	 * print function definitions.
	 */
	if (fflag) {
		parsefile(argc, argv, cwd, root, dbpath, db);
		exit(0);
	}
	/*
	 * search in current source tree.
	 */
	count = search(av, root, dbpath, db);
	/*
	 * search in library path.
	 */
	if (getenv("GTAGSLIBPATH") && (count == 0 || Tflag) && !lflag && !rflag) {
		STRBUF *sb = strbuf_open(0);
		char libdbpath[MAXPATHLEN+1];
		char *p, *lib;

		strbuf_puts(sb, getenv("GTAGSLIBPATH"));
		p = strbuf_value(sb);
		while (p) {
			lib = p;
			if ((p = locatestring(p, PATHSEP, MATCH_FIRST)) != NULL)
				*p++ = 0;
			if (!gtagsexist(lib, libdbpath, sizeof(libdbpath), 0))
				continue;
			if (!strcmp(dbpath, libdbpath))
				continue;
			if (!test("f", makepath(libdbpath, dbname(db), NULL)))
				continue;
			strbuf_reset(pathfilter);
			strbuf_puts(pathfilter, gtags);
			if (aflag)	/* absolute path name */
				strbuf_puts(pathfilter, " --path=absolute");
			else
				strbuf_puts(pathfilter, " --path=relative");
			if (tflag)
				strbuf_puts(pathfilter, " --format=ctags");
			else if (xflag)
				strbuf_puts(pathfilter, " --format=ctags-x");
			else
				strbuf_puts(pathfilter, " --format=path");
			strbuf_putc(pathfilter, ' ');
			strbuf_puts(pathfilter, lib);
			strbuf_putc(pathfilter, ' ');
			strbuf_puts(pathfilter, cwd);
			strbuf_putc(pathfilter, ' ');
			strbuf_puts(pathfilter, libdbpath);
			count = search(av, lib, libdbpath, db);
			if (count > 0 && !Tflag) {
				strlimcpy(dbpath, libdbpath, sizeof(dbpath));
				break;
			}
		}
		strbuf_close(sb);
	}
	if (vflag) {
		if (count) {
			if (count == 1)
				fprintf(stderr, "%d object located", count);
			if (count > 1)
				fprintf(stderr, "%d objects located", count);
		} else {
			fprintf(stderr, "'%s' not found", av);
		}
		if (!Tflag)
			fprintf(stderr, " (using '%s').\n", makepath(dbpath, dbname(db), NULL));
	}
	strbuf_close(sortfilter);
	strbuf_close(pathfilter);

	return 0;
}
/*
 * makefilter: make filter string.
 *
 *	o)	filter buffer
 */
void
makefilter(STRBUF *sb)
{
	int set_sortfilter = 0;

	if (!(nofilter & SORT_FILTER) && strbuf_getlen(sortfilter)) {
		strbuf_puts(sb, strbuf_value(sortfilter));
		set_sortfilter = 1;
	}
	if (!(nofilter & PATH_FILTER) && strbuf_getlen(pathfilter)) {
		if (set_sortfilter)
			strbuf_puts(sb, " | ");
		strbuf_puts(sb, strbuf_value(pathfilter));
	}
}
/*
 * openfilter: open output filter.
 *
 *	gi)	pathfilter
 *	gi)	sortfilter
 *	r)		file pointer for output filter
 */
FILE *
openfilter(void)
{
	FILE *op;
	STRBUF *sb = strbuf_open(0);

	makefilter(sb);
	if (strbuf_getlen(sb) == 0)
		op = stdout;
	else
		op = popen(strbuf_value(sb), "w");
	strbuf_close(sb);
	return op;
}
void
closefilter(FILE *op)
{
	if (op != stdout)
		if (pclose(op) != 0)
			die("terminated abnormally.");
}
/*
 * completion: print completion list of specified prefix
 *
 *	i)	dbpath	dbpath directory
 *	i)	root	root directory
 *	i)	prefix	prefix of primary key
 */
void
completion(const char *dbpath, const char *root, const char *prefix)
{
	const char *p;
	int flags = GTOP_KEY;
	GTOP *gtop;
	int db;

	flags |= GTOP_NOREGEX;
	if (prefix && *prefix == 0)	/* In the case global -c '' */
		prefix = NULL;
	if (prefix)
		flags |= GTOP_PREFIX;
	db = (sflag) ? GSYMS : GTAGS;
	gtop = gtags_open(dbpath, root, db, GTAGS_READ, 0);
	for (p = gtags_first(gtop, prefix, flags); p; p = gtags_next(gtop)) {
		fputs(p, stdout);
		fputc('\n', stdout);
	}
	gtags_close(gtop);
}
/*
 * printtag: print a tag's line
 *
 *	i)	op	output stream
 *	i)	ctags_x	ctags -x format record
 */
void
printtag(FILE *op, const char *ctags_x)		/* virtually const */
{
	if (xflag) {
		fputs(ctags_x, op);
	} else {
		SPLIT ptable;

		/*
		 * Split tag line.
		 */
		split((char *)ctags_x, 4, &ptable);

		if (tflag) {
			fputs(ptable.part[PART_TAG].start, op);	/* tag */
			(void)putc('\t', op);
			fputs(ptable.part[PART_PATH].start, op);/* path */
			(void)putc('\t', op);
			fputs(ptable.part[PART_LNO].start, op);	/* line number */
		} else {
			fputs(ptable.part[PART_PATH].start, op);/* path */
		}
		recover(&ptable);
	}
	fputc('\n', op);
}
/*
 * idutils:  lid(id-utils) pattern
 *
 *	i)	pattern	POSIX regular expression
 *	i)	dbpath	GTAGS directory
 */
void
idutils(const char *pattern, const char *dbpath)
{
	FILE *ip, *op;
	STRBUF *ib = strbuf_open(0);
	char edit[IDENTLEN+1];
	const char *path, *lno, *lid;
	int linenum, count;
	char *p, *grep;

	lid = usable("lid");
	if (!lid)
		die("lid(id-utils) not found.");
	/*
	 * convert spaces into %FF format.
	 */
	ffformat(edit, sizeof(edit), pattern);
	/*
	 * make lid command line.
	 * Invoke lid with the --result=grep option to generate grep format.
	 */
	strbuf_puts(ib, lid);
	strbuf_puts(ib, " --separator=newline");
	if (!tflag && !xflag)
		strbuf_puts(ib, " --result=filenames --key=none");
	else
		strbuf_puts(ib, " --result=grep");
	if (iflag)
		strbuf_puts(ib, " --ignore-case");
	if (extra_options) {
		strbuf_putc(ib, ' ');
		strbuf_puts(ib, extra_options);
	}
	strbuf_putc(ib, ' ');
	strbuf_puts(ib, quote_string(pattern));
	if (debug)
		fprintf(stderr, "id-utils: %s\n", strbuf_value(ib));
	if (!(ip = popen(strbuf_value(ib), "r")))
		die("cannot execute '%s'.", strbuf_value(ib));
	if (!(op = openfilter()))
		die("cannot open output filter.");
	count = 0;
	while ((grep = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		p = grep;
		/* extract filename */
		path = p;
		while (*p && *p != ':')
			p++;
		if ((xflag || tflag) && !*p)
			die("invalid lid(id-utils) output format. '%s'", grep);
		*p++ = 0;
		if (lflag) {
			if (!locatestring(path, localprefix + 2, MATCH_AT_FIRST))
				continue;
		}
		count++;
		if (!xflag && !tflag) {
			fprintf(op, "./%s\n", path);
			continue;
		}
		/* extract line number */
		while (*p && isspace(*p))
			p++;
		lno = p;
		while (*p && isdigit(*p))
			p++;
		if (*p != ':')
			die("invalid lid(id-utils) output format. '%s'", grep);
		*p++ = 0;
		linenum = atoi(lno);
		if (linenum <= 0)
			die("invalid lid(id-utils) output format. '%s'", grep);
		/*
		 * print out.
		 */
		if (tflag)
			fprintf(op, "%s\t./%s\t%d\n", edit, path, linenum);
		else {
			char	buf[MAXPATHLEN+1];

			snprintf(buf, sizeof(buf), "./%s", path);
			fprintf(op, "%-16s %4d %-16s %s\n",
					edit, linenum, buf, p);
		}
	}
	if (pclose(ip) < 0)
		die("terminated abnormally.");
	closefilter(op);
	strbuf_close(ib);
	if (vflag) {
		if (count == 0)
			fprintf(stderr, "object not found");
		if (count == 1)
			fprintf(stderr, "%d object located", count);
		if (count > 1)
			fprintf(stderr, "%d objects located", count);
		fprintf(stderr, " (using id-utils index in '%s').\n", dbpath);
	}
}
/*
 * grep: grep pattern
 *
 *	i)	pattern	POSIX regular expression
 */
void
grep(const char *dbpath, const char *pattern)
{
	FILE *op, *fp;
	GFIND *gp;
	STRBUF *ib = strbuf_open(MAXBUFLEN);
	const char *path;
	char edit[IDENTLEN+1];
	const char *buffer;
	int linenum, count;
	int flags = 0;
	regex_t	preg;

	/*
	 * convert spaces into %FF format.
	 */
	ffformat(edit, sizeof(edit), pattern);

	if (!Gflag)
		flags |= REG_EXTENDED;
	if (iflag)
		flags |= REG_ICASE;
	if (regcomp(&preg, pattern, flags) != 0)
		die("invalid regular expression.");
	if (!(op = openfilter()))
		die("cannot open output filter.");
	count = 0;
	/*
	 * The older version (4.8.7 or former) of GPATH doesn't have files
	 * other than source file. The oflag requires new version of GPATH.
	 */
	gp = gfind_open(dbpath, localprefix, oflag);
	if (gp->version < 2 && oflag)
		die("GPATH is old format. Please remake it by invoking gtags(1).");
	while ((path = gfind_read(gp)) != NULL) {
		if (!(fp = fopen(path, "r")))
			die("cannot open file '%s'.", path);
		linenum = 0;
		while ((buffer = strbuf_fgets(ib, fp, STRBUF_NOCRLF)) != NULL) {
			linenum++;
			if (regexec(&preg, buffer, 0, 0, 0) == 0) {
				count++;
				if (tflag)
					fprintf(op, "%s\t%s\t%d\n",
						edit, path, linenum);
				else if (!xflag) {
					fputs(path, op);
					fputc('\n', op);
					break;
				} else {
					fprintf(op, "%-16s %4d %-16s %s\n",
						edit, linenum, path, buffer);
				}
			}
		}
		fclose(fp);
	}
	gfind_close(gp);
	closefilter(op);
	strbuf_close(ib);
	regfree(&preg);
	if (vflag) {
		if (count == 0)
			fprintf(stderr, "object not found");
		if (count == 1)
			fprintf(stderr, "%d object located", count);
		if (count > 1)
			fprintf(stderr, "%d objects located", count);
		fprintf(stderr, " (no index used).\n");
	}
}
/*
 * pathlist: print candidate path list.
 *
 *	i)	dbpath
 */
void
pathlist(const char *dbpath, const char *av)
{
	GFIND *gp;
	FILE *op;
	const char *path, *p;
	regex_t preg;
	int count;

	if (av) {
		int flags = 0;

		if (!Gflag)
			flags |= REG_EXTENDED;
		if (iflag || getconfb("icase_path"))
			flags |= REG_ICASE;
#ifdef _WIN32
		flags |= REG_ICASE;
#endif /* _WIN32 */
		if (regcomp(&preg, av, flags) != 0)
			die("invalid regular expression.");
	}
	if (!localprefix)
		localprefix = ".";
	if (!(op = openfilter()))
		die("cannot open output filter.");
	count = 0;
	/*
	 * The older version (4.8.7 or former) of GPATH doesn't have files
	 * other than source file. The oflag requires new version of GPATH.
	 */
	gp = gfind_open(dbpath, localprefix, oflag);
	if (gp->version < 2 && oflag)
		die("GPATH is old format. Please remake it by invoking gtags(1).");
	while ((path = gfind_read(gp)) != NULL) {
		/*
		 * skip localprefix because end-user doesn't see it.
		 */
		p = path + strlen(localprefix) - 1;
		if (av && regexec(&preg, p, 0, 0, 0) != 0)
			continue;
		if (xflag)
			fprintf(op, "path\t1 %s \n", path);
		else if (tflag)
			fprintf(op, "path\t%s\t1\n", path);
		else {
			fputs(path, op);
			fputc('\n', op);
		}
		count++;
	}
	gfind_close(gp);
	closefilter(op);
	if (av)
		regfree(&preg);
	if (vflag) {
		if (count == 0)
			fprintf(stderr, "path not found");
		if (count == 1)
			fprintf(stderr, "%d path located", count);
		if (count > 1)
			fprintf(stderr, "%d paths located", count);
		fprintf(stderr, " (using '%s').\n", makepath(dbpath, dbname(GPATH), NULL));
	}
}
/*
 * parsefile: parse file to pick up tags.
 *
 *	i)	argc
 *	i)	argv
 *	i)	cwd	current directory
 *	i)	root	root directory of source tree
 *	i)	dbpath	dbpath
 *	i)	db	type of parse
 */
void
parsefile(int argc, char **argv, const char *cwd, const char *root, const char *dbpath, int db)
{
	char rootdir[MAXPATHLEN+1];
	char buf[MAXPATHLEN+1], *path;
	FILE *op;
	int count = 0;
	STRBUF *comline = strbuf_open(0);
	STRBUF *path_list = strbuf_open(MAXPATHLEN);
	XARGS *xp;
	char *ctags_x;

	snprintf(rootdir, sizeof(rootdir), "%s/", root);
	/*
	 * teach parser where is dbpath.
	 */
	set_env("GTAGSDBPATH", dbpath);
	/*
	 * get parser.
	 */
	if (!getconfs(dbname(db), comline))
		die("cannot get parser for %s.", dbname(db));
	if (!(op = openfilter()))
		die("cannot open output filter.");
	if (gpath_open(dbpath, 0) < 0)
		die("GPATH not found.");
	/*
	 * Make a path list while checking the validity of path name.
	 */
	for (; argc > 0; argv++, argc--) {
		const char *av = argv[0];

		if (!test("f", av)) {
			if (test("d", av)) {
				if (!qflag)
					fprintf(stderr, "'%s' is a directory.\n", av);
			} else {
				if (!qflag)
					fprintf(stderr, "'%s' not found.\n", av);
			}
			continue;
		}
		/*
		 * convert path into relative from root directory of source tree.
		 */
		path = realpath(av, buf);
		if (path == NULL)
			die("realpath(%s, buf) failed. (errno=%d).", av, errno);
		if (!isabspath(path))
			die("realpath(3) is not compatible with BSD version.");
		/*
		 * Remove the root part of path and insert './'.
		 *      rootdir  /a/b/
		 *      path     /a/b/c/d.c -> c/d.c -> ./c/d.c
		 */
		path = locatestring(path, rootdir, MATCH_AT_FIRST);
		if (path == NULL) {
			if (!qflag)
				fprintf(stderr, "'%s' is out of source tree.\n", buf);
			continue;
		}
		path -= 2;
		*path = '.';
		if (!gpath_path2fid(path, NULL)) {
			if (!qflag)
				fprintf(stderr, "'%s' not found in GPATH.\n", path);
			continue;
		}
		/*
		 * Add a path to the path list.
		 */
		strbuf_puts0(path_list, path);
	}
	/*
	 * Execute parser in the root directory of source tree.
	 */
	if (chdir(root) < 0)
		die("cannot move to '%s' directory.", root);
	xp = xargs_open_with_strbuf(strbuf_value(comline), 0, path_list);
	while ((ctags_x = xargs_read(xp)) != NULL) {
		printtag(op, ctags_x);
		count++;
	}
	xargs_close(xp);
	if (chdir(cwd) < 0)
		die("cannot move to '%s' directory.", cwd);
	/*
	 * Settlement
	 */
	gpath_close();
	closefilter(op);
	strbuf_close(comline);
	strbuf_close(path_list);
	if (vflag) {
		if (count == 0)
			fprintf(stderr, "object not found");
		if (count == 1)
			fprintf(stderr, "%d object located", count);
		if (count > 1)
			fprintf(stderr, "%d objects located", count);
		fprintf(stderr, " (no index used).\n");
	}
}
/*
 * search: search specified function 
 *
 *	i)	pattern		search pattern
 *	i)	root		root of source tree
 *	i)	dbpath		database directory
 *	i)	db		GTAGS,GRTAGS,GSYMS
 *	r)			count of output lines
 */
int
search(const char *pattern, const char *root, const char *dbpath, int db)
{
	const char *ctags_x;
	int count = 0;
	FILE *op;
	GTOP *gtop;
	int flags = 0;
	STRBUF *sb = NULL;

	/*
	 * open tag file.
	 */
	gtop = gtags_open(dbpath, root, db, GTAGS_READ, 0);
	if (!(op = openfilter()))
		die("cannot open output filter.");
	/*
	 * search through tag file.
	 */
	if (nosource)
		flags |= GTOP_NOSOURCE;
	if (iflag) {
		if (!isregex(pattern)) {
			sb = strbuf_open(0);
			strbuf_putc(sb, '^');
			strbuf_puts(sb, pattern);
			strbuf_putc(sb, '$');
			pattern = strbuf_value(sb);
		}
		flags |= GTOP_IGNORECASE;
	}
	if (Gflag)
		flags |= GTOP_BASICREGEX;
	for (ctags_x = gtags_first(gtop, pattern, flags); ctags_x; ctags_x = gtags_next(gtop)) {
		if (lflag) {
			const char *q;
			/* locate start point of a path */
			q = locatestring(ctags_x, "./", MATCH_FIRST);
			if (!locatestring(q, localprefix, MATCH_AT_FIRST))
				continue;
		}
		printtag(op, ctags_x);
		count++;
	}
	closefilter(op);
	if (sb)
		strbuf_close(sb);
	gtags_close(gtop);
	return count;
}
/*
 * ffformat: string copy with converting blank chars into %ff format.
 *
 *	o)	to	result
 *	i)	size	size of 'to' buffer
 *	i)	from	string
 */
void
ffformat(char *to, int size, const char *from)
{
	const char *p;
	char *e = to;

	for (p = from; *p; p++) {
		if (*p == '%' || *p == ' ' || *p == '\t') {
			if (size <= 3)
				break;
			snprintf(e, size, "%%%02x", *p);
			e += 3;
			size -= 3;
		} else {
			if (size <= 1)
				break;
			*e++ = *p;
			size--;
		}
	}
	*e = 0;
}
