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
#include <sys/types.h>
#include <sys/stat.h>

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

#include "global.h"
#include "regex.h"

const char *progname  = "global";		/* command name */

static void	usage(void);
static void	help(void);
static void	setcom(int);
int	main(int, char **);
void	makefilter(STRBUF *);
FILE	*openfilter(void);
void	closefilter(FILE *);
void	completion(char *, char *, char *);
void	relative_filter(STRBUF *, char *, char *);
void	idutils(char *, char *);
void	grep(char *, char *);
void	pathlist(char *, char *);
void	parsefile(int, char **, char *, char *, char *, int);
void	printtag(FILE *, char *);
int	search(char *, char *, char *, int);
int	includepath(char *, char *);
void	ffformat(char *, int, char *);

char	sort_command[MAXFILLEN+1];	/* sort command		*/
char	sed_command[MAXFILLEN+1];	/* sed command		*/
STRBUF	*sortfilter;			/* sort filter		*/
STRBUF	*pathfilter;			/* path convert filter	*/
char	*localprefix;			/* local prefix		*/
int	aflag;				/* [option]		*/
int	cflag;				/* command		*/
int	fflag;				/* command		*/
int	gflag;				/* command		*/
int	iflag;				/* command		*/
int	Iflag;				/* command		*/
int	lflag;				/* [option]		*/
int	nflag;				/* [option]		*/
int	pflag;				/* command		*/
int	Pflag;				/* command		*/
int	rflag;				/* [option]		*/
int	sflag;				/* [option]		*/
int	tflag;				/* [option]		*/
int	vflag;				/* [option]		*/
int	xflag;				/* [option]		*/
int	show_version;
int	show_help;
int	show_filter;			/* undocumented command */
int	use_tagfiles;
int	debug;
char	*extra_options;
const char *usage_const = "\
Usage: global [-alnrstvx] pattern\n\
       global -c[sv] [prefix]\n\
       global -f[anrstvx] files\n\
       global -g[alntvx] pattern\n\
       global -i[v]\n\
       global -I[alntvx] pattern\n\
       global -p[rv]\n\
       global -P[alntvx] [pattern]\n";
const char *help_const = "\
Pattern accept POSIX 1003.2 regular expression.\n\
Commands:\n\
     (none) pattern\n\
             print the locations of specified functions.\n\
     -c, --completion [prefix]\n\
             print candidate function names which start with prefix.\n\
     -f, --file files\n\
             print all functions in the files.\n\
     -g, --grep pattern\n\
             print all lines which match to the pattern.\n\
     -i, --incremental\n\
             locate tag files and reconstruct them incrementally.\n\
     -I, --idutils\n\
             print all lines which match to the pattern using idutils.\n\
     -p, --print-dbpath\n\
             print the location of tag files.\n\
     -P, --path [pattern]\n\
             print the path which match to the pattern.\n\
     -s, --symbol pattern\n\
             print the locations of specified symbol other than function.\n\
     --version\n\
             show version number.\n\
     --help  show help.\n\
Options:\n\
     -a, --absolute\n\
             print absolute path name.\n\
     -l, --local\n\
             print just objects which exist under the current directory.\n\
     -n, --nofilter\n\
             suppress sort filter and path conversion filter.\n\
     -r, --reference (--rootdir)\n\
             print the locations of object references.\n\
             with the -p option print the root directory of source tree.\n\
     -t, --tags\n\
             print with standard ctags format.\n\
     -v, --verbose\n\
             verbose mode.\n\
     -x, --cxref\n\
             produce the line number and the line contents.\n\
";

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
	{"absolute", no_argument, NULL, 'a'},
	{"completion", no_argument, NULL, 'c'},
	{"file", no_argument, NULL, 'f'},
	{"local", no_argument, NULL, 'l'},
	{"nofilter", no_argument, NULL, 'n'},
	{"grep", no_argument, NULL, 'g'},
	{"incremental", no_argument, NULL, 'i'},
	{"print-dbpath", no_argument, NULL, 'p'},
	{"path", no_argument, NULL, 'P'},
	{"reference", no_argument, NULL, 'r'},
	{"rootdir", no_argument, NULL, 'r'},
	{"symbol", no_argument, NULL, 's'},
	{"tags", no_argument, NULL, 't'},
	{"verbose", no_argument, NULL, 'v'},
	{"cxref", no_argument, NULL, 'x'},

	/* long name only */
	{"debug", no_argument, &debug, 1},
	{"idutils", optional_argument, &Iflag, 1},
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},
	{"filter", no_argument, &show_filter, 1},
	{ 0 }
};

static int	command;
static void
setcom(c)
int	c;
{
	if (command == 0)
		command = c;
	else if (command != c)
		usage();
}
int
main(argc, argv)
int	argc;
char	*argv[];
{
	char	*av;
	int	count;
	int	db;
	int	optchar;
	int	option_index = 0;
	char	cwd[MAXPATHLEN+1];		/* current directory	*/
	char	root[MAXPATHLEN+1];		/* root of source tree	*/
	char	dbpath[MAXPATHLEN+1];		/* dbpath directory	*/

	while ((optchar = getopt_long(argc, argv, "acfgGiIlnpPrstvx", long_options, &option_index)) != EOF) {
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
			break;
		case 'g':
			gflag++;
			setcom(optchar);
			break;
		case 'i':
			iflag++;
			setcom(optchar);
			break;
		case 'I':
			Iflag++;
			setcom(optchar);
			break;
		case 'p':
			pflag++;
			setcom(optchar);
			break;
		case 'P':
			Pflag++;
			setcom(optchar);
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
		case 'v':
			vflag++;
			break;
		case 'x':
			xflag++;
			break;
		default:
			usage();
			break;
		}
	}
	if (show_help)
		help();

	argc -= optind;
	argv += optind;
	av = (argc > 0) ? *argv : NULL;

	if (show_version)
		version(av, vflag);
	/*
	 * invalid options.
	 */
	if (sflag && rflag)
		die("both of -s and -r are not allowed.");
	/*
	 * only -c, -i, -P and -p allows no argment.
	 */
	if (!av && !show_filter) {
		switch (command) {
		case 'c':
		case 'i':
		case 'p':
		case 'P':
			break;
		default:
			usage();
			break;
		}
	}
	/*
	 * -i and -p cannot have any arguments.
	 */
	if (av) {
		switch (command) {
		case 'i':
		case 'p':
			usage();
		default:
			break;
		}
	}
	if (fflag)
		lflag = 0;
	/*
	 * remove leading blanks.
	 */
	if (!Iflag && !gflag && av)
		for (; *av == ' ' || *av == '\t'; av++)
			;
	if (cflag && av && notnamechar(av))
		die("only name char is allowed with -c option.");
	/*
	 * get path of following directories.
	 *	o current directory
	 *	o root of source tree
	 *	o dbpath directory
	 *
	 * if GTAGS not found, getdbpath doesn't return.
	 */
	getdbpath(cwd, root, dbpath, (pflag && vflag));
	if (Iflag && strcmp(root, dbpath))
		die("You must have idutils's index at the root of source tree.");
	/*
	 * print dbpath or rootdir.
	 */
	if (pflag) {
		char *dir = (rflag) ? root : dbpath;
		fprintf(stdout, "%s\n", dir);
		exit(0);
	}
	/*
	 * incremental update of tag files.
	 */
	if (iflag) {
		STRBUF	*sb = strbuf_open(0);

		if (chdir(root) < 0)
			die("cannot change directory to '%s'.", root);
		strbuf_puts(sb, "gtags -i");
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
	 * get command name of sort and sed.
	 */
	{
		STRBUF	*sb = strbuf_open(0);
		if (!getconfs("sort_command", sb))
			die("cannot get sort command name.");
#ifdef _WIN32
		if (!locatestring(strbuf_value(sb), ".exe", MATCH_LAST))
			strbuf_puts(sb, ".exe");
#endif
		strcpy(sort_command, strbuf_value(sb));
		strbuf_reset(sb);
		if (!getconfs("sed_command", sb))
			die("cannot get sed command name.");
#ifdef _WIN32
		if (!locatestring(strbuf_value(sb), ".exe", MATCH_LAST))
			strbuf_puts(sb, ".exe");
#endif
		strcpy(sed_command, strbuf_value(sb));
		strbuf_close(sb);
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
	 * make sort filter.
	 */
	sortfilter = strbuf_open(0);
	strbuf_puts(sortfilter, sort_command);
	strbuf_putc(sortfilter, ' ');
	if (tflag) 			/* ctags format */
		strbuf_puts(sortfilter, "+0 -1 +1 -2 +2n -3");
	else if (fflag)
		strbuf_setlen(sortfilter, 0);
	else if (xflag)			/* print details */
		strbuf_puts(sortfilter, "+0 -1 +2 -3 +1n -2");
	else 				/* print just a file name */
		strbuf_puts(sortfilter, "-u");
	/*
	 * make path filter.
	 */
	pathfilter = strbuf_open(0);
	if (aflag) {				/* absolute path name */
		strbuf_puts(pathfilter, sed_command);
		strbuf_putc(pathfilter, ' ');
		strbuf_puts(pathfilter, "-e \"s@\\.@");
		strbuf_puts(pathfilter, root);
		strbuf_puts(pathfilter, "@\"");
	} else if (lflag) {
		strbuf_puts(pathfilter, sed_command);
		strbuf_putc(pathfilter, ' ');
		strbuf_puts(pathfilter, "-e \"s@\\");
		strbuf_puts(pathfilter, localprefix);
		strbuf_puts(pathfilter, "@@\"");
	} else {				/* relative path name */
		relative_filter(pathfilter, root, cwd);
	}
	/*
	 * print filter.
	 */
	if (show_filter) {
		STRBUF  *sb = strbuf_open(0);

		makefilter(sb);
		fprintf(stdout, "%s\n", strbuf_value(sb));
		strbuf_close(sb);
		exit(0);
	}
	/*
	 * exec gid(idutils).
	 */
	if (Iflag) {
		if (!usable("gid"))
			die("gid(idutils) not found.");
		chdir(root);
		idutils(av, dbpath);
		exit(0);
	}
	/*
	 * grep the pattern in a source tree.
	 */
	if (gflag) {
		chdir(root);
		grep(av, dbpath);
		exit(0);
	}
	/*
	 * locate the path including the pattern in a source tree.
	 */
	if (Pflag) {
		pathlist(dbpath, av);
		exit(0);
	}
	db = (rflag) ? GRTAGS : ((sflag) ? GSYMS : GTAGS);
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
	if (count == 0 && !lflag && !rflag && !sflag && !notnamechar(av) && getenv("GTAGSLIBPATH")) {
		STRBUF  *sb = strbuf_open(0);
		char	libdbpath[MAXPATHLEN+1];
		char	*p, *lib;

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
			if (aflag) {		/* absolute path name */
				strbuf_reset(pathfilter);
				strbuf_puts(pathfilter, sed_command);
				strbuf_putc(pathfilter, ' ');
				strbuf_puts(pathfilter, "-e \"s@\\.@");
				strbuf_puts(pathfilter, lib);
				strbuf_puts(pathfilter, "@\"");
			} else {
				strbuf_reset(pathfilter);
				relative_filter(pathfilter, lib, cwd);
			}
			count = search(av, lib, libdbpath, db);
			if (count > 0) {
				strcpy(dbpath, libdbpath);
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
makefilter(sb)
STRBUF	*sb;
{
	if (!nflag) {
		strbuf_puts(sb, strbuf_value(sortfilter));
		if (strbuf_getlen(sortfilter) && strbuf_getlen(pathfilter))
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
FILE	*
openfilter(void)
{
	FILE	*op;
	STRBUF  *sb = strbuf_open(0);

	makefilter(sb);
	if (strbuf_getlen(sb) == 0)
		op = stdout;
	else
		op = popen(strbuf_value(sb), "w");
	strbuf_close(sb);
	return op;
}
void
closefilter(op)
FILE	*op;
{
	if (op != stdout)
		pclose(op);
}
/*
 * completion: print completion list of specified prefix
 *
 *	i)	dbpath	dbpath directory
 *	i)	root	root directory
 *	i)	prefix	prefix of primary key
 */
void
completion(dbpath, root, prefix)
char	*dbpath;
char	*root;
char	*prefix;
{
	char	*p;
	int	flags = GTOP_KEY;
	GTOP	*gtop;
	int	db;

	flags |= GTOP_NOREGEX;
	if (prefix && *prefix == 0)	/* In the case global -c '' */
		prefix = NULL;
	if (prefix)
		flags |= GTOP_PREFIX;
	db = (sflag) ? GSYMS : GTAGS;
	gtop = gtagsopen(dbpath, root, db, GTAGS_READ, 0);
	for (p = gtagsfirst(gtop, prefix, flags); p; p = gtagsnext(gtop))
		(void)fprintf(stdout, "%s\n", p);
	gtagsclose(gtop);
}
/*
 * relative_filter: make relative path filter
 *
 *	o)	sb	buffer for the result
 *	i)	root	the root directory of source tree
 *	i)	cwd	current directory
 */
void
relative_filter(sb, root, cwd)
STRBUF	*sb;
char	*root;
char	*cwd;
{
	char	*p, *c, *branch;

	/*
	 * get branch point.
	 */
	branch = cwd;
	for (p = root, c = cwd; *p && *c && *p == *c; p++, c++)
		if (*c == '/')
			branch = c;
	if (*p == 0 && (*c == 0 || *c == '/'))
		branch = c;
	/*
	 * forward to root.
	 */
	strbuf_puts(sb, sed_command);
	strbuf_putc(sb, ' ');
	strbuf_puts(sb, "-e \"s@\\./@");
	for (c = branch; *c; c++)
		if (*c == '/')
			strbuf_puts(sb, "../");
	p = root + (branch - cwd);
	/*
	 * backward to leaf.
	 */
	if (*p) {
		p++;
		strbuf_puts(sb, p);
		strbuf_putc(sb, '/');
	}
	strbuf_puts(sb, "@\"");
	/*
	 * remove redundancy.
	 */
	if (*branch) {
		char	unit[256];

		p = unit;
		for (c = branch + 1; ; c++) {
			if (*c == 0 || *c == '/') {
				*p = 0;
				strbuf_puts(sb, " -e \"s@\\.\\./");
				strbuf_puts(sb, unit);
				strbuf_puts(sb, "/@@\"");
				if (*c == 0)
					break;
				p = unit;
			} else
				*p++ = *c;
		}
	}
}
/*
 * printtag: print a tag's line
 *
 *	i)	op	output stream
 *	i)	bp	tag's line
 */
void
printtag(op, bp)
FILE	*op;
char	*bp;
{
	if (tflag) {
		char	buf[MAXBUFLEN+1];
		char	lno[20], *l = lno;
		char	*p = bp;
		char	*q = buf;

		while (*p && !isspace(*p))
			*q++ = *p++;
		*q++ = '\t';
		for (; *p && isspace(*p); p++)
			;
		while (*p && isdigit(*p))
			*l++ = *p++;
		*l = 0;
		for (; *p && isspace(*p); p++)
			;
		while (*p && !isspace(*p))
			*q++ = *p++;
		*q++ = '\t';
		l = lno;
		while (*l)
			*q++ = *l++;
		*q = 0;
		fprintf(op, "%s\n", buf); 
	} else if (!xflag) {
		char	*p = locatestring(bp, "./", MATCH_FIRST);

		if (p == NULL)
			die("illegal tag format (path not found).");
		fputs(strmake(p, " \t"), op);
		(void)putc('\n', op);
	} else
		fprintf(op, "%s\n", bp); 
}
/*
 * idutils:  gid(idutils) pattern
 *
 *	i)	pattern	POSIX regular expression
 *	i)	dbpath	GTAGS directory
 */
void
idutils(pattern, dbpath)
char	*pattern;
char	*dbpath;
{
	FILE	*ip, *op;
	STRBUF	*ib = strbuf_open(0);
	char	edit[IDENTLEN+1];
	char    *line, *p, *path, *lno;
	int     linenum, count, editlen;

	/*
	 * convert spaces into %FF format.
	 */
	ffformat(edit, sizeof(edit), pattern);
	editlen = strlen(edit);
	/*
	 * make gid command line.
	 */
	strbuf_puts(ib, "gid ");
	strbuf_puts(ib, "--separator=newline ");
	if (!tflag && !xflag)
		strbuf_puts(ib, "--result=filenames --key=none ");
	else
		strbuf_puts(ib, "--result=grep ");
	if (extra_options) {
		strbuf_puts(ib, extra_options);
		strbuf_putc(ib, ' ');
	}
	strbuf_putc(ib, '\'');
	strbuf_puts(ib, pattern);
	strbuf_putc(ib, '\'');
	if (debug)
		fprintf(stderr, "idutils: %s\n", strbuf_value(ib));
	if (!(ip = popen(strbuf_value(ib), "r")))
		die("cannot execute '%s'.", strbuf_value(ib));
	if (!(op = openfilter()))
		die("cannot open output filter.");
	count = 0;
	while ((line = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		p = line;
		/* extract filename */
		path = p;
		while (*p && *p != ':')
			p++;
		if ((xflag || tflag) && !*p)
			die("illegal gid(idutils) output format. '%s'", line);
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
			die("illegal grep output format. '%s'", line);
		*p++ = 0;
		linenum = atoi(lno);
		if (linenum <= 0)
			die("illegal grep output format. '%s'", line);
		/*
		 * print out.
		 */
		if (tflag)
			fprintf(op, "%s\t./%s\t%d\n", edit, path, linenum);
		else {
			char	buf[MAXPATHLEN+1];

#ifdef HAVE_SNPRINTF
			snprintf(buf, sizeof(buf), "./%s", path);
#else
			sprintf(buf, "./%s", path);
#endif /* HAVE_SNPRINTF */
			if (editlen >= 16 && linenum >= 1000)
				fprintf(op, "%-16s %4d %-16s %s\n",
					edit, linenum, buf, p);
			else
				fprintf(op, "%-16s%4d %-16s %s\n",
					edit, linenum, buf, p);
		}
	}
	pclose(ip);
	closefilter(op);
	strbuf_close(ib);
	if (vflag) {
		if (count == 0)
			fprintf(stderr, "object not found");
		if (count == 1)
			fprintf(stderr, "%d object located", count);
		if (count > 1)
			fprintf(stderr, "%d objects located", count);
		fprintf(stderr, " (using idutils index in '%s').\n", dbpath);
	}
}
/*
 * grep: grep pattern
 *
 *	i)	pattern	POSIX regular expression
 *	i)	dbpath	GTAGS directory
 */
#if defined(HAVE_XARGS) && defined(HAVE_GREP)
void
grep(pattern, dbpath)
char	*pattern;
char	*dbpath;
{
	FILE	*ip, *op;
	STRBUF	*ib = strbuf_open(0);
	char	edit[IDENTLEN+1];
	char    *line, *p, *path, *lno;
	int     linenum, count, editlen;

	/*
	 * convert spaces into %FF format.
	 */
	ffformat(edit, sizeof(edit), pattern);
	editlen = strlen(edit);
	/*
	 * make grep command line.
	 * (/dev/null needed when single argment specified.)
	 */
	strbuf_puts(ib, "gtags --find ");
	if (lflag)
		strbuf_puts(ib, localprefix);
	strbuf_puts(ib, "| xargs grep ");
	if (!tflag && !xflag)
		strbuf_puts(ib, "-l ");
	else
		strbuf_puts(ib, "-n ");
	strbuf_putc(ib, '\'');
	strbuf_puts(ib, pattern);
	strbuf_putc(ib, '\'');
	strbuf_puts(ib, " /dev/null");
	if (debug)
		fprintf(stderr, "grep: %s\n", strbuf_value(ib));
	if (!(ip = popen(strbuf_value(ib), "r")))
		die("cannot execute '%s'.", strbuf_value(ib));
	if (!(op = openfilter()))
		die("cannot open output filter.");
	count = 0;
	while ((line = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		p = line;
		/* extract filename */
		path = p;
		while (*p && *p != ':')
			p++;
		if ((xflag || tflag) && !*p)
			die("illegal grep output format. '%s'", line);
		*p++ = 0;
		if (!xflag && !tflag) {
			fprintf(op, "%s\n", path);
			continue;
		}
		/* extract line number */
		while (*p && isspace(*p))
			p++;
		lno = p;
		while (*p && isdigit(*p))
			p++;
		if (*p != ':')
			die("illegal grep output format. '%s'", line);
		*p++ = 0;
		linenum = atoi(lno);
		if (linenum <= 0)
			die("illegal grep output format. '%s'", line);
		/*
		 * print out.
		 */
		count++;
		if (tflag)
			fprintf(op, "%s\t%s\t%d\n",
				edit, path, linenum);
		else if (!xflag) {
			fprintf(op, "%s\n", path);
		} else if (editlen >= 16 && linenum >= 1000)
			fprintf(op, "%-16s %4d %-16s %s\n",
				edit, linenum, path, p);
		else
			fprintf(op, "%-16s%4d %-16s %s\n",
				edit, linenum, path, p);
	}
	pclose(ip);
	closefilter(op);
	strbuf_close(ib);
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
#else /* HAVE_XARGS && HAVE_GREP */
void
grep(pattern, dbpath)
char	*pattern;
char	*dbpath;
{
	FILE	*op, *fp;
	STRBUF	*ib = strbuf_open(MAXBUFLEN);
	char	*path;
	char	edit[IDENTLEN+1];
	char	*buffer;
	int	linenum, count, editlen;
	regex_t	preg;

	/*
	 * convert spaces into %FF format.
	 */
	ffformat(edit, sizeof(edit), pattern);
	editlen = strlen(edit);

	if (regcomp(&preg, pattern, REG_EXTENDED) != 0)
		die("illegal regular expression.");
	if (!(op = openfilter()))
		die("cannot open output filter.");
	count = 0;
	for (gfindopen(dbpath, localprefix); (path = gfindread()) != NULL; ) {
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
					fprintf(op, "%s\n", path);
					break;
				} else if (editlen >= 16 && linenum >= 1000)
					fprintf(op, "%-16s %4d %-16s %s\n",
						edit, linenum, path, buffer);
				else
					fprintf(op, "%-16s%4d %-16s %s\n",
						edit, linenum, path, buffer);
			}
		}
		fclose(fp);
	}
	gfindclose();
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
#endif /* HAVE_XARGS && HAVE_GREP */
/*
 * pathlist: print candidate path list.
 *
 *	i)	dbpath
 */
void
pathlist(dbpath, av)
char	*dbpath;
char	*av;
{
	FILE	*op;
	char	*path, *p;
	regex_t preg;
	int	count;

	if (av) {
		int	flags = REG_EXTENDED;

		if (getconfb("icase_path"))
			flags |= REG_ICASE;
#ifdef _WIN32
		flags |= REG_ICASE;
#endif /* _WIN32 */
		if (regcomp(&preg, av, flags) != 0)
			die("illegal regular expression.");
	}
	if (!localprefix)
		localprefix = "./";
	if (!(op = openfilter()))
		die("cannot open output filter.");
	count = 0;
	for (gfindopen(dbpath, localprefix); (path = gfindread()) != NULL; ) {
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
		else
			fprintf(op, "%s\n", path);
		count++;
	}
	gfindclose();
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
parsefile(argc, argv, cwd, root, dbpath, db)
int	argc;
char	**argv;
char	*cwd;
char	*root;
char	*dbpath;
int	db;
{
	char	buf[MAXPATHLEN+1], *path;
	char	*p;
	FILE	*ip, *op;
	char	*parser, *av;
	int	count;
	STRBUF  *sb = strbuf_open(0);
	STRBUF	*com = strbuf_open(0);
	STRBUF  *ib = strbuf_open(MAXBUFLEN);

	/*
	 * teach parser where is dbpath.
	 */
#ifdef HAVE_PUTENV
	{
		STRBUF *env = strbuf_open(0);

		strbuf_puts(env, "GTAGSDBPATH=");
		strbuf_puts(env, dbpath);
		putenv(strbuf_value(env));
		strbuf_close(env);
	}
#else
	setenv("GTAGSDBPATH", dbpath, 1);
#endif /* HAVE_PUTENV */

	/*
	 * get parser.
	 */
	if (!getconfs(dbname(db), sb))
		die("cannot get parser for %s.", dbname(db));
	parser = strbuf_value(sb);

	if (!(op = openfilter()))
		die("cannot open output filter.");
	if (pathopen(dbpath, 0) < 0)
		die("GPATH not found.");
	count = 0;
	for (; argc > 0; argv++, argc--) {
		av = argv[0];
		if (test("d", av)) {
			fprintf(stderr, "'%s' is a directory.\n", av);
			continue;
		}
		if (!test("f", NULL)) {
			fprintf(stderr, "'%s' not found.\n", av);
			continue;
		}
		/*
		 * convert path into relative from root directory of source tree.
		 */
		path = realpath(av, buf);
		if (!isabspath(path))
			die("realpath(3) is not compatible with BSD version.");
		if (strncmp(path, root, strlen(root))) {
			fprintf(stderr, "'%s' is out of source tree.\n", path);
			continue;
		}
		path += strlen(root) - 1;
		*path = '.';
		if (!pathget(path)) {
			fprintf(stderr, "'%s' not found in GPATH.\n", path);
			continue;
		}
		if (chdir(root) < 0)
			die("cannot move to '%s' directory.", root);
		/*
		 * make command line.
		 */
		strbuf_reset(com);
		makecommand(parser, path, com);
		if (debug)
			fprintf(stderr, "executing %s\n", strbuf_value(com));
		if (!(ip = popen(strbuf_value(com), "r")))
			die("cannot execute '%s'.", strbuf_value(com));
		while ((p = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
			count++;
			printtag(op, p);
		}
		pclose(ip);
		if (chdir(cwd) < 0)
			die("cannot move to '%s' directory.", cwd);
	}
	pathclose();
	closefilter(op);
	strbuf_close(sb);
	strbuf_close(com);
	strbuf_close(ib);
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
search(pattern, root, dbpath, db)
char	*pattern;
char	*root;
char	*dbpath;
int	db;
{
	char	*p;
	int	count = 0;
	FILE	*op;
	GTOP	*gtop;
	int	flags = 0;

	/*
	 * open tag file.
	 */
	gtop = gtagsopen(dbpath, root, db, GTAGS_READ, 0);
	if (!(op = openfilter()))
		die("cannot open output filter.");
	/*
	 * search through tag file.
	 */
	if (nflag > 1)
		flags |= GTOP_NOSOURCE;
	for (p = gtagsfirst(gtop, pattern, flags); p; p = gtagsnext(gtop)) {
		if (lflag) {
			char	*q;
			/* locate start point of a path */
			q = locatestring(p, "./", MATCH_FIRST);
			if (!locatestring(q, localprefix, MATCH_AT_FIRST))
				continue;
		}
		printtag(op, p);
		count++;
	}
	closefilter(op);
	gtagsclose(gtop);
	return count;
}
/*
 * includepath: check if the path included in tag line or not.
 *
 *	i)	line	tag line
 *	i)	path	path
 *	r)		0: doesn't included, 1: included
 */
int
includepath(line, path)
char	*line;
char	*path;
{
	char	*p;
	int	length;

	if (!(p = locatestring(line, "./", MATCH_FIRST)))
		die("illegal tag format (path not found).");
	length = strlen(path);
	if (strncmp(p, path, length))
		return 0;
	p += length;
	if (*p == ' ' || *p == '\t')
		return 1;
	return 0;
}
/*
 * ffformat: string copy with converting blank chars into %ff format.
 *
 *	o)	to	result
 *	i)	size	size of 'to' buffer
 *	i)	from	string
 */
void
ffformat(to, size, from)
char	*to;
int	size;
char	*from;
{
	char	*p, *e = to;

	for (p = from; *p; p++) {
		if (*p == '%' || *p == ' ' || *p == '\t') {
			if (size <= 3)
				break;
#ifdef HAVE_SNPRINTF
			snprintf(e, size, "%%%02x", *p);
#else
			sprintf(e, "%%%02x", *p);
#endif /* HAVE_SNPRINTF */
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
