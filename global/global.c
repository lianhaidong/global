/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006
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
#include "filter.h"

static void usage(void);
static void help(void);
static void setcom(int);
int main(int, char **);
void printtag(const char *);
void printtag_using(const char *, const char *, int, const char *);
void completion(const char *, const char *, const char *);
void idutils(const char *, const char *);
void grep(const char *, const char *);
void pathlist(const char *, const char *);
void parsefile(int, char **, const char *, const char *, const char *, int);
int search(const char *, const char *, const char *, const char *, int);
void tagsearch(const char *, const char *, const char *, const char *, int);
void encode(char *, int, const char *);

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
int result;				/* [option]		*/
int show_version;
int show_help;
int show_filter;			/* undocumented command */
int nofilter;
int nosource;				/* undocumented command */
int debug;
const char *gtags;
int unique;
int format;
int passthru;

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

#define SHOW_FILTER	128
#define RESULT		129

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
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},
	{"filter", optional_argument, NULL, SHOW_FILTER},
	{"result", required_argument, NULL, RESULT},
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
	int db;
	int optchar;
	int option_index = 0;
	char cwd[MAXPATHLEN+1];			/* current directory	*/
	char root[MAXPATHLEN+1];		/* root of source tree	*/
	char dbpath[MAXPATHLEN+1];		/* dbpath directory	*/

	while ((optchar = getopt_long(argc, argv, "ace:ifgGIlnopPqrstTuvx", long_options, &option_index)) != EOF) {
		switch (optchar) {
		case 0:
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
					nofilter |= SORT_FILTER;
				else if (!strcmp(optarg, "path"))
					nofilter |= PATH_FILTER;
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
		case RESULT:
			if (!strcmp(optarg, "ctags-x"))
				format = FORMAT_CTAGS_X;
			else if (!strcmp(optarg, "ctags-xid"))
				format = FORMAT_CTAGS_XID;
			else if (!strcmp(optarg, "ctags"))
				format = FORMAT_CTAGS;
			else if (!strcmp(optarg, "path"))
				format = FORMAT_PATH;
			else if (!strcmp(optarg, "grep"))
				format = FORMAT_GREP;
			else
				die_with_code(2, "unknown format type for the --result option.");
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
	 * get gtags's path.
	 */
	gtags = usable("gtags");
	if (!gtags)
		die("gtags command not found.");
	gtags = strdup(gtags);
	if (!gtags)
		die("short of memory.");
	/*
	 * incremental update of tag files.
	 */
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
	unique = sflag ? 1 : 0;
	/*
	 * decide format.
	 * The -t and -x option are given to priority more than
	 * the --result option.
	 */
	if (fflag || gflag || Pflag || (nofilter & SORT_FILTER))
		passthru = 1;
	if (tflag) { 			/* ctags format */
		format = FORMAT_CTAGS;
	} else if (xflag) {		/* print details */
		format = FORMAT_CTAGS_X;
	} else if (format == 0) {	/* print just a file name */
		format = FORMAT_PATH;
		unique = 1;
	}
	/*
	 * setup sort filter.
	 */
	setup_sortfilter(format, unique, passthru);
	/*
	 * setup path filter.
	 */
	setup_pathfilter(format, aflag ? PATH_ABSOLUTE : PATH_RELATIVE, root, cwd, dbpath);
	/*
	 * print external filter.
	 */
	if (show_filter) {
		STRBUF  *sb;

		switch (show_filter) {
		case SORT_FILTER:
			if (!(nofilter & SORT_FILTER))
				fprintf(stdout, "%s\n", getsortfilter());
			break;
		case PATH_FILTER:
			if (!(nofilter & PATH_FILTER))
				fprintf(stdout, "%s\n", getpathfilter());
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
	 * exec lid(id-utils).
	 */
	if (Iflag) {
		chdir(root);
		idutils(av, dbpath);
	}
	/*
	 * search pattern (regular expression).
	 */
	else if (gflag) {
		chdir(root);
		grep(av, dbpath);
	}
	/*
	 * locate paths including the pattern.
	 */
	else if (Pflag) {
		chdir(root);
		pathlist(av, dbpath);
	}
	/*
	 * parse source files.
	 */
	else if (fflag) {
		parsefile(argc, argv, cwd, root, dbpath, db);
	}
	/*
	 * tag search.
	 */
	else {
		tagsearch(av, cwd, root, dbpath, db);
	}
	return 0;
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
 *	i)	ctags_x	ctags -x format record
 */
void
printtag(const char *ctags_x)		/* virtually const */
{
	SPLIT ptable;

	switch (format) {
	case FORMAT_PATH:
		split((char *)ctags_x, 4, &ptable);
		printtag_using(
			NULL,					/* tag */
			ptable.part[PART_PATH].start,		/* path */
			0,					/* line no */
			NULL);
		recover(&ptable);
		break;
	default:
		filter_put(ctags_x);
		break;
	}
}
/*
 * printtag_using: print a tag's line with arguments
 *
 *	i)	tag	tag name
 *	i)	path	path name
 *	i)	lineno	line number
 *	i)	line	line image
 */
void
printtag_using(const char *tag, const char *path, int lineno, const char *line)
{
	STATIC_STRBUF(sb);
	char edit[MAXPATHLEN];

	strbuf_clear(sb);
	/*
	 * normalize path name.
	 */
	if (*path != '.') {
		int i = 0;
		edit[i++] = '.';
		edit[i++] = '/';
		while ((edit[i++] = *path++) != '\0')
			;
		path = edit;
	}
	/*
	 * The other format than FORMAT_PATH is always written as FORMAT_CTAGS_X.
	 * Format conversion will be done in the following process.
	 */
	switch (format) {
	case FORMAT_PATH:
		strbuf_puts(sb, path);
		break;
	default:
		strbuf_sprintf(sb, "%-16s %4d %-16s %s", tag, lineno, path, line);
		break;
	}
	filter_put(strbuf_value(sb));
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
	FILE *ip;
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
	encode(edit, sizeof(edit), pattern);
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
	strbuf_putc(ib, ' ');
	strbuf_puts(ib, quote_string(pattern));
	if (debug)
		fprintf(stderr, "id-utils: %s\n", strbuf_value(ib));
	if (!(ip = popen(strbuf_value(ib), "r")))
		die("cannot execute '%s'.", strbuf_value(ib));
	filter_open();
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
		switch (format) {
		case FORMAT_PATH:
			printtag_using(NULL, path, 0, NULL);
			break;
		default:
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
			printtag_using(edit, path, linenum, p);
			break;
		}
	}
	if (pclose(ip) < 0)
		die("terminated abnormally.");
	filter_close();
	strbuf_close(ib);
	if (vflag) {
		switch (count) {
		case 0:
			fprintf(stderr, "object not found");
			break;
		case 1:
			fprintf(stderr, "%d object located", count);
			break;
		default:
			fprintf(stderr, "%d objects located", count);
			break;
		}
		fprintf(stderr, " (using id-utils index in '%s').\n", dbpath);
	}
}
/*
 * grep: grep pattern
 *
 *	i)	pattern	POSIX regular expression
 */
void
grep(const char *pattern, const char *dbpath)
{
	FILE *fp;
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
	encode(edit, sizeof(edit), pattern);

	if (!Gflag)
		flags |= REG_EXTENDED;
	if (iflag)
		flags |= REG_ICASE;
	if (regcomp(&preg, pattern, flags) != 0)
		die("invalid regular expression.");
	filter_open();
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
			die("'%s' not found. Please remake tag files by invoking gtags(1).", path);
		linenum = 0;
		while ((buffer = strbuf_fgets(ib, fp, STRBUF_NOCRLF)) != NULL) {
			linenum++;
			if (regexec(&preg, buffer, 0, 0, 0) == 0) {
				count++;
				printtag_using(edit, path, linenum, buffer);
				if (format == FORMAT_PATH)
					break;
			}
		}
		fclose(fp);
	}
	gfind_close(gp);
	filter_close();
	strbuf_close(ib);
	regfree(&preg);
	if (vflag) {
		switch (count) {
		case 0:
			fprintf(stderr, "object not found");
			break;
		case 1:
			fprintf(stderr, "%d object located", count);
			break;
		default:
			fprintf(stderr, "%d objects located", count);
			break;
		}
		fprintf(stderr, " (no index used).\n");
	}
}
/*
 * pathlist: print candidate path list.
 *
 *	i)	dbpath
 */
void
pathlist(const char *pattern, const char *dbpath)
{
	GFIND *gp;
	const char *path, *p;
	regex_t preg;
	int count;

	if (pattern) {
		int flags = 0;
		char edit[IDENTLEN+1];

		if (!Gflag)
			flags |= REG_EXTENDED;
		if (iflag || getconfb("icase_path"))
			flags |= REG_ICASE;
#ifdef _WIN32
		flags |= REG_ICASE;
#endif /* _WIN32 */
		/*
		 * We assume '^aaa' as '^/aaa'.
		 */
		if (*pattern == '^' && *(pattern + 1) != '/') {
			snprintf(edit, sizeof(edit), "^/%s", pattern + 1);
			pattern = edit;
		}
		if (regcomp(&preg, pattern, flags) != 0)
			die("invalid regular expression.");
	}
	if (!localprefix)
		localprefix = "./";
	filter_open();
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
		if (pattern && regexec(&preg, p, 0, 0, 0) != 0)
			continue;
		printtag_using("path", path, 1, " ");
		count++;
	}
	gfind_close(gp);
	filter_close();
	if (pattern)
		regfree(&preg);
	if (vflag) {
		switch (count) {
		case 0:
			fprintf(stderr, "path not found");
			break;
		case 1:
			fprintf(stderr, "%d path located", count);
			break;
		default:
			fprintf(stderr, "%d paths located", count);
			break;
		}
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
	 * teach parser language mapping.
	 */
	{
		STRBUF *sb = strbuf_open(0);

		if (getconfs("langmap", sb))
			set_env("GTAGSLANGMAP", strbuf_value(sb));
		strbuf_close(sb);
	}
	/*
	 * get parser.
	 */
	if (!getconfs(dbname(db), comline))
		die("cannot get parser for %s.", dbname(db));
	filter_open();
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
		printtag(ctags_x);
		count++;
	}
	xargs_close(xp);
	if (chdir(cwd) < 0)
		die("cannot move to '%s' directory.", cwd);
	/*
	 * Settlement
	 */
	gpath_close();
	filter_close();
	strbuf_close(comline);
	strbuf_close(path_list);
	if (vflag) {
		switch (count) {
		case 0:
			fprintf(stderr, "object not found");
			break;
		case 1:
			fprintf(stderr, "%d object located", count);
			break;
		default:
			fprintf(stderr, "%d objects located", count);
			break;
		}
		fprintf(stderr, " (no index used).\n");
	}
}
/*
 * search: search specified function 
 *
 *	i)	pattern		search pattern
 *	i)	root		root of source tree
 *	i)	cwd		current directory
 *	i)	dbpath		database directory
 *	i)	db		GTAGS,GRTAGS,GSYMS
 *	r)			count of output lines
 */
int
search(const char *pattern, const char *root, const char *cwd, const char *dbpath, int db)
{
	const char *ctags_x;
	int count = 0;
	GTOP *gtop;
	int flags = 0;
	STRBUF *sb = NULL;

	/*
	 * open tag file.
	 */
	gtop = gtags_open(dbpath, root, db, GTAGS_READ, 0);
	filter_open();
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
		printtag(ctags_x);
		count++;
	}
	filter_close();
	if (sb)
		strbuf_close(sb);
	gtags_close(gtop);
	return count;
}
/*
 * tagsearch: execute tag search
 *
 *	i)	pattern		search pattern
 *	i)	cwd		current directory
 *	i)	root		root of source tree
 *	i)	dbpath		database directory
 *	i)	db		GTAGS,GRTAGS,GSYMS
 */
void
tagsearch(const char *pattern, const char *cwd, const char *root, const char *dbpath, int db)
{
	int count, total = 0;
	char libdbpath[MAXPATHLEN+1];

	/*
	 * search in current source tree.
	 */
	count = search(pattern, root, cwd, dbpath, db);
	total += count;
	/*
	 * search in library path.
	 */
	if (getenv("GTAGSLIBPATH") && (count == 0 || Tflag) && !lflag && !rflag) {
		STRBUF *sb = strbuf_open(0);
		char *libdir, *nextp = NULL;

		strbuf_puts(sb, getenv("GTAGSLIBPATH"));
		/*
		 * search for each tree in the library path.
		 */
		for (libdir = strbuf_value(sb); libdir; libdir = nextp) {
			if ((nextp = locatestring(libdir, PATHSEP, MATCH_FIRST)) != NULL)
				*nextp++ = 0;
			if (!gtagsexist(libdir, libdbpath, sizeof(libdbpath), 0))
				continue;
			if (!strcmp(dbpath, libdbpath))
				continue;
			if (!test("f", makepath(libdbpath, dbname(db), NULL)))
				continue;

			/*
			 * reconstruct path filter
			 */
			setup_pathfilter(format, aflag ? PATH_ABSOLUTE : PATH_RELATIVE, libdir, cwd, libdbpath);
			/*
			 * search again
			 */
			count = search(pattern, libdir, cwd, libdbpath, db);
			total += count;
			if (count > 0 && !Tflag) {
				/* for verbose message */
				dbpath = libdbpath;
				break;
			}
		}
		strbuf_close(sb);
	}
	if (vflag) {
		switch (total) {
		case 0:
			fprintf(stderr, "'%s' not found", pattern);
			break;
		case 1:
			fprintf(stderr, "%d object located", total);
			break;
		default:
			fprintf(stderr, "%d objects located", total);
			break;
		}
		if (!Tflag)
			fprintf(stderr, " (using '%s')", makepath(dbpath, dbname(db), NULL));
		fputs(".\n", stderr);
	}
}
/*
 * encode: string copy with converting blank chars into %ff format.
 *
 *	o)	to	result
 *	i)	size	size of 'to' buffer
 *	i)	from	string
 */
void
encode(char *to, int size, const char *from)
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
