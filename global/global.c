/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005, 2006,
 *	2007, 2008, 2010, 2011, 2012, 2013, 2014, 2015
 *	Tama Communications Corporation
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
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
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
#if defined(_WIN32) && !defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef SLIST_ENTRY
#endif
#include "getopt.h"

#include "global.h"
#include "parser.h"
#include "regex.h"
#include "const.h"
#include "output.h"
#include "literal.h"
#include "convert.h"

/*
 * ensure GTAGSLIBPATH compares correctly
 */
#if defined(_WIN32) || defined(__DJGPP__)
#define STRCMP stricmp
#define back2slash(sb) do {		\
	char *p = strbuf_value(sb);	\
	for (; *p; p++) 		\
		if (*p == '\\')         \
			*p = '/';       \
} while (0)
#else
#define STRCMP strcmp
#define back2slash(sb)
#endif

/*
 * enable [set] globbing, if available
 */
#ifdef __CRT_GLOB_BRACKET_GROUPS__
int _CRT_glob = __CRT_GLOB_USE_MINGW__ | __CRT_GLOB_BRACKET_GROUPS__;
#endif

/**
 * global - print locations of the specified object.
 */
static void usage(void);
static void help(void);
static void setcom(int);
int decide_tag_by_context(const char *, const char *, int);
int main(int, char **);
int completion_tags(const char *, const char *, const char *, int);
void completion(const char *, const char *, const char *, int);
void completion_idutils(const char *, const char *, const char *);
void completion_path(const char *, const char *);
void idutils(const char *, const char *);
void grep(const char *, char *const *, const char *);
void pathlist(const char *, const char *);
void parsefile(char *const *, const char *, const char *, const char *, int);
int search(const char *, const char *, const char *, const char *, int);
void tagsearch(const char *, const char *, const char *, const char *, int);
void encode(char *, int, const char *);

const char *localprefix;		/**< local prefix		*/
int aflag;				/* [option]		*/
int cflag;				/* command		*/
int dflag;				/* command		*/
int fflag;				/* command		*/
int gflag;				/* command		*/
int Gflag;				/* [option]		*/
int iflag;				/* [option]		*/
int Iflag;				/* command		*/
int Lflag;				/* [option]		*/
int Mflag;				/* [option]		*/
int Nflag;				/* [option]		*/
int nflag;				/* [option]		*/
int oflag;				/* [option]		*/
int Oflag;				/* [option]		*/
int pflag;				/* command		*/
int Pflag;				/* command		*/
int qflag;				/* [option]		*/
int rflag;				/* [option]		*/
int sflag;				/* [option]		*/
int Sflag;				/* [option]		*/
int tflag;				/* [option]		*/
int Tflag;				/* [option]		*/
int uflag;				/* command		*/
int vflag;				/* [option]		*/
int Vflag;				/* [option]		*/
int xflag;				/* [option]		*/
int show_version;
int show_help;
int nofilter;
int nosource;				/**< undocumented command */
int debug;
int literal;				/**< 1: literal search	*/
int print0;				/**< --print0 option	*/
int format;
int type;				/**< path conversion type */
int match_part;				/**< match part only	*/
int abslib;				/**< absolute path only in library project */
char use_color;				/**< coloring */
const char *cwd;			/**< current directory	*/
const char *root;			/**< root of source tree	*/
const char *dbpath;			/**< dbpath directory	*/
const char *nearbase;			/**< nearbase directory	*/
char *context_file;
char *context_lineno;
char *file_list;
char *scope;
char *encode_chars;
char *single_update;
char *path_style;

/*
 * Path filter
 */
int do_path;
int convert_type = PATH_RELATIVE;

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

#define OPT_RESULT		128
#define OPT_FROM_HERE		129
#define OPT_ENCODE_PATH		130
#define OPT_MATCH_PART		131
#define OPT_SINGLE_UPDATE	132
#define OPT_PATH_STYLE		133
#define OPT_PATH_CONVERT	134
#define OPT_USE_COLOR		135
#define OPT_GTAGSCONF		136
#define OPT_GTAGSLABEL		137
#define SORT_FILTER     1
#define PATH_FILTER     2
#define BOTH_FILTER     (SORT_FILTER|PATH_FILTER)
#define MATCH_PART_FIRST 1
#define MATCH_PART_LAST  2
#define MATCH_PART_ALL   3

static struct option const long_options[] = {
	{"absolute", no_argument, NULL, 'a'},
	{"completion", no_argument, NULL, 'c'},
	{"definition", no_argument, NULL, 'd'},
	{"extended-regexp", no_argument, NULL, 'E'},
	{"regexp", required_argument, NULL, 'e'},
	{"file", no_argument, NULL, 'f'},
	{"first-match", no_argument, NULL, 'F'},
	{"local", no_argument, NULL, 'l'},
	{"file-list", required_argument, NULL, 'L'},
	{"match-case", no_argument, NULL, 'M'},
	{"nofilter", optional_argument, NULL, 'n'},
	{"nearness", optional_argument, NULL, 'N'},
	{"grep", no_argument, NULL, 'g'},
	{"basic-regexp", no_argument, NULL, 'G'},
	{"ignore-case", no_argument, NULL, 'i'},
	{"idutils", no_argument, NULL, 'I'},
	{"other", no_argument, NULL, 'o'},
	{"only-other", no_argument, NULL, 'O'},
	{"print-dbpath", no_argument, NULL, 'p'},
	{"path", no_argument, NULL, 'P'},
	{"quiet", no_argument, NULL, 'q'},
	{"reference", no_argument, NULL, 'r'},
	{"rootdir", no_argument, NULL, 'r'},
	{"symbol", no_argument, NULL, 's'},
	{"scope", required_argument, NULL, 'S'},
	{"tags", no_argument, NULL, 't'},
	{"through", no_argument, NULL, 'T'},
	{"update", no_argument, NULL, 'u'},
	{"verbose", no_argument, NULL, 'v'},
	{"invert-match", optional_argument, NULL, 'V'},
	{"cxref", no_argument, NULL, 'x'},

	/* long name only */
	{"color", optional_argument, NULL, OPT_USE_COLOR},
	{"encode-path", required_argument, NULL, OPT_ENCODE_PATH},
	{"from-here", required_argument, NULL, OPT_FROM_HERE},
	{"debug", no_argument, &debug, 1},
	{"gtagsconf", required_argument, NULL, OPT_GTAGSCONF},
	{"gtagslabel", required_argument, NULL, OPT_GTAGSLABEL},
	{"literal", no_argument, &literal, 1},
	{"match-part", required_argument, NULL, OPT_MATCH_PART},
	{"path-style", required_argument, NULL, OPT_PATH_STYLE},
	{"path-convert", required_argument, NULL, OPT_PATH_CONVERT},
	{"print0", no_argument, &print0, 1},
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},
	{"result", required_argument, NULL, OPT_RESULT},
	{"nosource", no_argument, &nosource, 1},
	{"single-update", required_argument, NULL, OPT_SINGLE_UPDATE},
	{ 0 }
};

static int command;
static void
setcom(int c)
{
	if (command == 0)
		command = c;
	else if (c == 'c' && (command == 'I' || command == 'P'))
		command = c;
	else if (command == 'c' && (c == 'I' || c == 'P'))
		;
	else if (command != c)
		usage();
}
/**
 * decide_tag_by_context: decide tag type by context
 *
 *	@param[in]	tag	tag name
 *	@param[in]	file	context file
 *	@param[in]	lineno	context lineno
 *	@return		GTAGS, GRTAGS, GSYMS
 */
int
decide_tag_by_context(const char *tag, const char *file, int lineno)
{
#define NEXT_NUMBER(p) do {                                                         \
	for (n = 0; isdigit(*p); p++)                                               \
		n = n * 10 + (*p - '0');                                            \
} while (0)
	STRBUF *sb = NULL;
	char path[MAXPATHLEN], s_fid[MAXFIDLEN];
	const char *tagline, *p;
	DBOP *dbop;
	int db = GSYMS;
	int iscompline = 0;

	if (normalize(file, get_root_with_slash(), cwd, path, sizeof(path)) == NULL)
		die("'%s' is out of the source project.", file);
	/*
	 * get file id
	 */
	if (gpath_open(dbpath, 0) < 0)
		die("GPATH not found.");
	if ((p = gpath_path2fid(path, NULL)) == NULL)
		die("path name in the context is not found.");
	strlimcpy(s_fid, p, sizeof(s_fid));
	gpath_close();
	/*
	 * read btree records directly to avoid the overhead.
	 */
	dbop = dbop_open(makepath(dbpath, dbname(GTAGS), NULL), 0, 0, 0);
	if (dbop == NULL)
		die("cannot open GTAGS.");
	if (dbop_getoption(dbop, COMPLINEKEY))
		iscompline = 1;
	tagline = dbop_first(dbop, tag, NULL, 0);
	if (tagline) {
		db = GTAGS;
		for (; tagline; tagline = dbop_next(dbop)) {
			/*
			 * examine whether the definition record include the context.
			 */
			p = locatestring(tagline, s_fid, MATCH_AT_FIRST);
			if (p != NULL && *p == ' ') {
				for (p++; *p && *p != ' '; p++)
					;
				if (*p++ != ' ' || !isdigit(*p))
					die("Impossible! decide_tag_by_context(1)");
				/*
				 * Standard format	n <blank> <image>$
				 * Compact format	d,d,d,d$
				 */
				if (!iscompline) {			/* Standard format */
					if (atoi(p) == lineno) {
						db = GRTAGS;
						goto finish;
					}
				} else {				/* Compact format */
					int n, cur, last = 0;

					do {
						if (!isdigit(*p))
							die("Impossible! decide_tag_by_context(2)");
						NEXT_NUMBER(p);
						cur = last + n;
						if (cur == lineno) {
							db = GRTAGS;
							goto finish;
						}
						last = cur;
						if (*p == '-') {
							if (!isdigit(*++p))
								die("Impossible! decide_tag_by_context(3)");
							NEXT_NUMBER(p);
							cur = last + n;
							if (lineno >= last && lineno <= cur) {
								db = GRTAGS;
								goto finish;
							}
							last = cur;
						}
						if (*p) {
							if (*p == ',')
								p++;
							else
								die("Impossible! decide_tag_by_context(4)");
						}
					} while (*p);
				}
			}
		}
	}
finish:
	dbop_close(dbop);
	if (db == GSYMS && getenv("GTAGSLIBPATH")) {
		char libdbpath[MAXPATHLEN];
		char *libdir = NULL, *nextp = NULL;

		sb = strbuf_open(0);
		strbuf_puts(sb, getenv("GTAGSLIBPATH"));
		back2slash(sb);
		for (libdir = strbuf_value(sb); libdir; libdir = nextp) {
			 if ((nextp = locatestring(libdir, PATHSEP, MATCH_FIRST)) != NULL)
                                *nextp++ = 0;
			if (!gtagsexist(libdir, libdbpath, sizeof(libdbpath), 0))
				continue;
			if (!STRCMP(dbpath, libdbpath))
				continue;
			dbop = dbop_open(makepath(libdbpath, dbname(GTAGS), NULL), 0, 0, 0);
			if (dbop == NULL)
				continue;
			tagline = dbop_first(dbop, tag, NULL, 0);
			dbop_close(dbop);
			if (tagline != NULL) {
				db = GTAGS;
				break;
			}
		}
		strbuf_close(sb);
	}
	return db;
}
int
main(int argc, char **argv)
{
	const char *av = NULL;
	int db;
	int optchar;
	int option_index = 0;
	int status = 0;

	/*
	 * get path of following directories.
	 *	o current directory
	 *	o root of source tree
	 *	o dbpath directory
	 *
	 * if GTAGS not found, exit with an error message.
	 */
	status = setupdbpath(0);
	if (status == 0) {
		cwd = get_cwd();
		root = get_root();
		dbpath = get_dbpath();
	}
	/*
	 * Setup GTAGSCONF and GTAGSLABEL environment variable
	 * according to the --gtagsconf and --gtagslabel option.
	 */
	preparse_options(argc, argv);
	/*
	 * Open configuration file.
	 */
	openconf(root);
	setenv_from_config();
	logging_arguments(argc, argv);
	while ((optchar = getopt_long(argc, argv, "acde:EifFgGIlL:MnNoOpPqrsS:tTuvVx", long_options, &option_index)) != EOF) {
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
		case 'd':
			dflag++;
			break;
		case 'e':
			av = optarg;
			break;
		case 'E':
			Gflag = 0;
			break;
		case 'f':
			fflag++;
			xflag++;
			setcom(optchar);
			break;
		case 'F':
			Tflag = 0;
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
		case 'l':
			Sflag++;
			scope = ".";
			break;
		case 'L':
			file_list = optarg;
			break;
		case 'M':
			Mflag++;
			iflag = 0;
			break;
		case 'n':
			nflag++;
			if (optarg) {
				if (!strcmp(optarg, "sort"))
					nofilter |= SORT_FILTER;
				else if (!strcmp(optarg, "path"))
					nofilter |= PATH_FILTER;
			} else {
				nofilter = BOTH_FILTER;
			}
			break;
		case 'N':
			Nflag++;
			if (optarg)
				nearbase = optarg;
			break;
		case 'o':
			oflag++;
			break;
		case 'O':
			Oflag++;
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
		case 'S':
			Sflag++;
			scope = optarg;
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
			setverbose();
			break;
		case 'V':
			Vflag++;
			break;
		case 'x':
			xflag++;
			break;
		case OPT_USE_COLOR:
			if (optarg) {
				if (!strcmp(optarg, "never"))
					use_color = 0;
				else if (!strcmp(optarg, "always"))
					use_color = 1;
				else if (!strcmp(optarg, "auto"))
					use_color = 2;
				else
					die_with_code(2, "unknown color type.");
			} else {
				use_color = 2;
			}
			break;
		case OPT_ENCODE_PATH:
			encode_chars = optarg;
			break;
		case OPT_FROM_HERE:
			{
			char *p = optarg;
			const char *usage = "usage: global --from-here=lineno:path.";

			context_lineno = p;
			while (*p && isdigit(*p))
				p++;
			if (*p != ':')
				die_with_code(2, usage);
			*p++ = '\0';
			if (!*p)
				die_with_code(2, usage);
			context_file = p;
			}
			break;
		case OPT_GTAGSCONF:
		case OPT_GTAGSLABEL:
			/* These options are already parsed in preparse_options() */
			break;
		case OPT_MATCH_PART:
			if (!strcmp(optarg, "first"))
				match_part = MATCH_PART_FIRST;
			else if (!strcmp(optarg, "last"))
				match_part = MATCH_PART_LAST;
			else if (!strcmp(optarg, "all"))
				match_part = MATCH_PART_ALL;
			else
				die_with_code(2, "unknown part type for the --match-part option.");
			break;
		case OPT_PATH_CONVERT:
			do_path = 1;
			if (!strcmp("absolute", optarg))
				convert_type = PATH_ABSOLUTE;
			else if (!strcmp("relative", optarg))
				convert_type = PATH_RELATIVE;
			else if (!strcmp("through", optarg))
				convert_type = PATH_THROUGH;
			else
				die("Unknown path type.");
			break;
		case OPT_PATH_STYLE:
			path_style = optarg;
			break;
		case OPT_RESULT:
			if (!strcmp(optarg, "ctags-x"))
				format = FORMAT_CTAGS_X;
			else if (!strcmp(optarg, "ctags-xid"))
				format = FORMAT_CTAGS_XID;
			else if (!strcmp(optarg, "ctags"))
				format = FORMAT_CTAGS;
			else if (!strcmp(optarg, "ctags-mod"))
				format = FORMAT_CTAGS_MOD;
			else if (!strcmp(optarg, "path"))
				format = FORMAT_PATH;
			else if (!strcmp(optarg, "grep"))
				format = FORMAT_GREP;
			else if (!strcmp(optarg, "cscope"))
				format = FORMAT_CSCOPE;
			else
				die_with_code(2, "unknown format type for the --result option.");
			break;
		case OPT_SINGLE_UPDATE:
			single_update = optarg;
			break;
		default:
			usage();
			break;
		}
	}
	if (qflag)
		vflag = 0;
	if (show_version)
		version(av, vflag);
	if (show_help)
		help();
	if (dbpath == NULL)
		die_with_code(-status, gtags_dbpath_error);
	if (nearbase) {
		if (!test("d", nearbase)) 
			die("'%s' not found.", nearbase);
		if (set_nearbase_path(nearbase) == NULL)
			die("internal error: '%s'.", nearbase);
	}
	/*
	 * decide format.
	 * The --result option is given to priority more than the -t and -x option.
	 */
	if (format == 0) {
		if (tflag) { 			/* ctags format */
			format = FORMAT_CTAGS;
		} else if (xflag) {		/* print details */
			format = FORMAT_CTAGS_X;
		} else {			/* print just a file name */
			format = FORMAT_PATH;
		}
	}
	/*
	 * GTAGSBLANKENCODE will be used in less(1).
	 */
	switch (format) {
	case FORMAT_CTAGS_X:
	case FORMAT_CTAGS_XID:
		if (encode_chars == NULL && getenv("GTAGSBLANKENCODE"))
			encode_chars = " \t";
		break;
	}
	if (encode_chars) {
		if (strlen(encode_chars) > 255)
			die("too many encode chars.");
		if (strchr(encode_chars, '/') || strchr(encode_chars, '.'))
			warning("cannot encode '/' and '.' in the path. Ignored.");
		set_encode_chars((unsigned char *)encode_chars);
	}
	if (getenv("GTAGSTHROUGH"))
		Tflag++;
	if (use_color) {
#if defined(_WIN32) && !defined(__CYGWIN__)
		if (!(getenv("ANSICON") || LoadLibrary("ANSI32.dll")) && use_color == 2)
			use_color = 0;
#endif
		if (use_color == 2 && !isatty(1))
			use_color = 0;
		if (Vflag)
			use_color = 0;
	}
	argc -= optind;
	argv += optind;
	/*
	 * Path filter
	 */
	if (do_path) {
		/*
		 * This code is needed for globash.rc.
		 * This code extract path name from tag line and
		 * replace it with the relative or the absolute path name.
		 *
		 * By default, if we are in src/ directory, the output
		 * should be converted like follows:
		 *
		 * main      10 ./src/main.c  main(argc, argv)\n
		 * main      22 ./libc/func.c   main(argc, argv)\n
		 *		v
		 * main      10 main.c  main(argc, argv)\n
		 * main      22 ../libc/func.c   main(argc, argv)\n
		 *
		 * Similarly, the --path-convert=absolute option specified, then
		 *		v
		 * main      10 /prj/xxx/src/main.c  main(argc, argv)\n
		 * main      22 /prj/xxx/libc/func.c   main(argc, argv)\n
		 */
		STRBUF *ib = strbuf_open(MAXBUFLEN);
		CONVERT *cv;
		char *ctags_x;

		if (argc < 3)
			die("global --path-convert: 3 arguments needed.");
		cv = convert_open(convert_type, FORMAT_CTAGS_X, argv[0], argv[1], argv[2], stdout, NOTAGS);
		while ((ctags_x = strbuf_fgets(ib, stdin, STRBUF_NOCRLF)) != NULL)
			convert_put(cv, ctags_x);
		convert_close(cv);
		strbuf_close(ib);
		exit(0);
	}
	/*
	 * At first, we pickup pattern from -e option. If it is not found
	 * then use argument which is not option.
	 */
	if (!av) {
		av = *argv;
		/*
		 * global -g pattern [files ...]
		 *           av      argv
		 */
		if (gflag && av)
			argv++;
	}
	if (single_update) {
		if (command == 0) {
			uflag++;
			command = 'u';
		} else if (command != 'u') {
			;	/* ignored */
		}
	}
	/*
	 * only -c, -u, -P and -p allows no argument.
	 */
	if (!av) {
		switch (command) {
		case 'c':
		case 'u':
		case 'p':
		case 'P':
			break;
		case 'f':
			if (file_list)
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
	if (tflag)
		xflag = 0;
	if (nflag > 1)
		nosource = 1;	/* to keep compatibility */
	if (print0)
		set_print0();
	if (cflag && match_part == 0)
		match_part = MATCH_PART_ALL;
	/*
	 * remove leading blanks.
	 */
	if (!Iflag && !gflag && av)
		for (; *av == ' ' || *av == '\t'; av++)
			;
	if (cflag && !Pflag && av && isregex(av))
		die_with_code(2, "only name char is allowed with -c option.");
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
	if (uflag) {
		STRBUF	*sb = strbuf_open(0);
		char *gtags_path = usable("gtags");

		if (!gtags_path)
			die("gtags command not found.");
		if (chdir(root) < 0)
			die("cannot change directory to '%s'.", root);
#if defined(_WIN32) && !defined(__CYGWIN__)
		/*
		 * Get around CMD.EXE's weird quoting rules by sticking another
		 * perceived whitespace in front (also works with Take Command).
		 */
		strbuf_putc(sb, ';');
#endif
		strbuf_puts(sb, quote_shell(gtags_path));
		strbuf_puts(sb, " -i");
		if (vflag)
			strbuf_puts(sb, " -v");
		if (single_update) {
			if (!isabspath(single_update)) {
				static char regular_path_name[MAXPATHLEN];

				if (rel2abs(single_update, cwd, regular_path_name, sizeof(regular_path_name)) == NULL)
					die("rel2abs failed.");
				single_update = regular_path_name;
			}
			strbuf_puts(sb, " --single-update ");
			strbuf_puts(sb, quote_shell(single_update));
		}
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, quote_shell(dbpath));
		if (system(strbuf_value(sb)))
			exit(1);
		strbuf_close(sb);
		exit(0);
	}
	/*
	 * decide tag type.
	 */
	if (context_file) {
		if (isregex(av))
			die_with_code(2, "regular expression is not allowed with the --from-here option.");
		db = decide_tag_by_context(av, context_file, atoi(context_lineno));
	} else {
		if (dflag)
			db = GTAGS;
		else if (rflag && sflag)
			db = GRTAGS + GSYMS;
		else
			db = (rflag) ? GRTAGS : ((sflag) ? GSYMS : GTAGS);
	}
	/*
	 * complete function name
	 */
	if (cflag) {
		if (Iflag)
			completion_idutils(dbpath, root, av);
		else if (Pflag)
			completion_path(dbpath, av);
		else
			completion(dbpath, root, av, db);
		exit(0);
	}
	/*
	 * make local prefix.
	 * local prefix must starts with './' and ends with '/'.
	 */
	if (Sflag) {
		STRBUF *sb = strbuf_open(0);
		static char buf[MAXPATHLEN];
		const char *path = scope;
	
		/*
		 * normalize the path of scope directory.
		 */
		if (!test("d", path))
			die("'%s' not found or not a directory.", scope);
		if (!isabspath(path))
			path = makepath(cwd, path, NULL);
		if (realpath(path, buf) == NULL)
			die("cannot get real path of '%s'.", scope);
		if (!in_the_project(buf))
			die("'%s' is out of the source project.", scope);
		scope = buf;
		/*
		 * make local prefix.
		 */
		strbuf_putc(sb, '.');
		if (strcmp(root, scope) != 0) {
			const char *p = scope + strlen(root);
			if (*p != '/')
				strbuf_putc(sb, '/');
			strbuf_puts(sb, p);
		}
		strbuf_putc(sb, '/');
		localprefix = check_strdup(strbuf_value(sb));
		strbuf_close(sb);
#ifdef DEBUG
		fprintf(stderr, "root=%s\n", root);
		fprintf(stderr, "cwd=%s\n", cwd);
		fprintf(stderr, "localprefix=%s\n", localprefix);
#endif
	}
	/*
	 * convert the file-list path into an absolute path.
	 */
	if (file_list && strcmp(file_list, "-") && !isabspath(file_list)) {
		static char buf[MAXPATHLEN];

		if (realpath(file_list, buf) == NULL)
			die("'%s' not found.", file_list);
		file_list = buf;
	}
	/*
	 * decide path conversion type.
	 */
	if (nofilter & PATH_FILTER)
		type = PATH_THROUGH;
	else if (aflag)
		type = PATH_ABSOLUTE;
	else
		type = PATH_RELATIVE;
	if (path_style) {
		if (!strcmp(path_style, "relative"))
			type = PATH_RELATIVE;
		else if (!strcmp(path_style, "absolute"))
			type = PATH_ABSOLUTE;
		else if (!strcmp(path_style, "through"))
			type = PATH_THROUGH;
		else if (!strcmp(path_style, "shorter"))
			type = PATH_SHORTER;
		else if (!strcmp(path_style, "abslib")) {
			type = PATH_RELATIVE;
			abslib++;
		} else
			die("invalid path style.");
	}
	/*
	 * exec lid(idutils).
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
		grep(av, argv, dbpath);
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
		chdir(root);
		parsefile(argv, cwd, root, dbpath, db);
	}
	/*
	 * tag search.
	 */
	else {
		tagsearch(av, cwd, root, dbpath, db);
	}
	return 0;
}
/**
 * completion_tags: print completion list of specified prefix
 *
 *	@param[in]	dbpath	dbpath directory
 *	@param[in]	root	root directory
 *	@param[in]	prefix	prefix of primary key
 *	@param[in]	db	GTAGS,GRTAGS,GSYMS
 *	@return		number of words
 */
int
completion_tags(const char *dbpath, const char *root, const char *prefix, int db)
{
	int flags = GTOP_KEY | GTOP_NOREGEX | GTOP_PREFIX;
	GTOP *gtop = gtags_open(dbpath, root, db, GTAGS_READ, 0);
	GTP *gtp;
	int count = 0;

	if (iflag)
		flags |= GTOP_IGNORECASE;
	for (gtp = gtags_first(gtop, prefix, flags); gtp; gtp = gtags_next(gtop)) {
		fputs(gtp->tag, stdout);
		fputc('\n', stdout);
		count++;
	}
	if (debug)
		gtags_show_statistics(gtop);
	gtags_close(gtop);
	return count;
}
/**
 * completion: print completion list of specified prefix
 *
 *	@param[in]	dbpath	dbpath directory
 *	@param[in]	root	root directory
 *	@param[in]	prefix	prefix of primary key
 *	@param[in]	db	GTAGS,GRTAGS,GSYMS
 */
void
completion(const char *dbpath, const char *root, const char *prefix, int db)
{
	int count, total = 0;
	char libdbpath[MAXPATHLEN];

	if (prefix && *prefix == 0)	/* In the case global -c '' */
		prefix = NULL;
	count = completion_tags(dbpath, root, prefix, db);
	/*
	 * search in library path.
	 */
	if (db == GTAGS && getenv("GTAGSLIBPATH") && (count == 0 || Tflag) && !Sflag) {
		STRBUF *sb = strbuf_open(0);
		char *libdir, *nextp = NULL;

		strbuf_puts(sb, getenv("GTAGSLIBPATH"));
		back2slash(sb);
		/*
		 * search for each tree in the library path.
		 */
		for (libdir = strbuf_value(sb); libdir; libdir = nextp) {
			if ((nextp = locatestring(libdir, PATHSEP, MATCH_FIRST)) != NULL)
				*nextp++ = 0;
			if (!gtagsexist(libdir, libdbpath, sizeof(libdbpath), 0))
				continue;
			if (!STRCMP(dbpath, libdbpath))
				continue;
			if (!test("f", makepath(libdbpath, dbname(db), NULL)))
				continue;
			/*
			 * search again
			 */
			count = completion_tags(libdbpath, libdir, prefix, db);
			total += count;
			if (count > 0 && !Tflag)
				break;
		}
		strbuf_close(sb);
	}
	/* return total; */
}
/**
 * completion_idutils: print completion list of specified prefix
 *
 *	@param[in]	dbpath	dbpath directory
 *	@param[in]	root	root directory
 *	@param[in]	prefix	prefix of primary key
 */
void
completion_idutils(const char *dbpath, const char *root, const char *prefix)
{
	FILE *ip;
	STRBUF *sb = strbuf_open(0);
	const char *lid = usable("lid");
	char *line, *p;

	if (prefix && *prefix == 0)	/* In the case global -c '' */
		prefix = NULL;
	/*
	 * make lid command line.
	 * Invoke lid with the --result=grep option to generate grep format.
	 */
	if (!lid)
		die("lid(idutils) not found.");
	strbuf_puts(sb, lid);
	strbuf_sprintf(sb, " --file=%s/ID", quote_shell(dbpath));
	strbuf_puts(sb, " --key=token");
	if (iflag)
		strbuf_puts(sb, " --ignore-case");
	if (prefix) {
		strbuf_putc(sb, ' ');
		strbuf_putc(sb, '"');
		strbuf_putc(sb, '^');
		strbuf_puts(sb, prefix);
		strbuf_putc(sb, '"');
	}
	if (debug)
		fprintf(stderr, "completion_idutils: %s\n", strbuf_value(sb));
	if (chdir(root) < 0)
		die("cannot move to '%s' directory.", root);
	if (!(ip = popen(strbuf_value(sb), "r")))
		die("cannot execute '%s'.", strbuf_value(sb));
	while ((line = strbuf_fgets(sb, ip, STRBUF_NOCRLF)) != NULL) {
		for (p = line; *p && *p != ' '; p++)
			;
		if (*p == '\0') {
			warning("Invalid line: %s", line);
			continue;
		}
		*p = '\0';
		puts(line);
	}
	if (pclose(ip) != 0)
		die("terminated abnormally (errno = %d).", errno);
	strbuf_close(sb);
}
/**
 * completion_path: print candidate path list.
 *
 *	@param[in]	dbpath	dbpath directory
 *	@param[in]	prefix	prefix of primary key
 */
void
completion_path(const char *dbpath, const char *prefix)
{
	GFIND *gp;
	const char *localprefix = "./";
	DBOP *dbop = dbop_open(NULL, 1, 0600, DBOP_RAW);
	const char *path;
	int prefix_length;
	int target = GPATH_SOURCE;
	int flags = (match_part == MATCH_PART_LAST) ? MATCH_LAST : MATCH_FIRST;

	if (dbop == NULL)
		die("cannot open temporary file.");
	if (prefix && *prefix == 0)	/* In the case global -c '' */
		prefix = NULL;
	prefix_length = (prefix == NULL) ? 0 : strlen(prefix);
	if (oflag)
		target = GPATH_BOTH;
	if (Oflag)
		target = GPATH_OTHER;
	if (iflag || getconfb("icase_path"))
		flags |= IGNORE_CASE;
#if _WIN32 || __DJGPP__
	else if (!Mflag)
		flags |= IGNORE_CASE;
#endif
	gp = gfind_open(dbpath, localprefix, target);
	while ((path = gfind_read(gp)) != NULL) {
		path++;					/* skip '.'*/
		if (prefix == NULL) {
			dbop_put(dbop, path + 1, "");
		} else if (match_part == MATCH_PART_ALL) {
			const char *p = path;

			while ((p = locatestring(p, prefix, flags)) != NULL) {
				dbop_put(dbop, p, "");
				p += prefix_length;
			}
		} else {
			const char *p = locatestring(path, prefix, flags);
			if (p != NULL) {
				dbop_put(dbop, p, "");
			}
		}
	}
	gfind_close(gp);
	for (path = dbop_first(dbop, NULL, NULL, DBOP_KEY); path != NULL; path = dbop_next(dbop)) {
		fputs(path, stdout);
		fputc('\n', stdout);
	}
	dbop_close(dbop);
}
/*
 * Output filter
 *
 * (1) Old architecture (- GLOBAL-4.7.8)
 *
 * process1          process2       process3
 * +=============+  +===========+  +===========+
 * |global(write)|->|sort filter|->|path filter|->[stdout]
 * +=============+  +===========+  +===========+
 *
 * (2) Recent architecture (GLOBAL-5.0 - 5.3)
 *
 * 1 process
 * +===========================================+
 * |global(write)->[sort filter]->[path filter]|->[stdout]
 * +===========================================+
 *
 * (3) Current architecture (GLOBAL-5.4 -)
 *
 * 1 process
 * +===========================================+
 * |[sort filter]->global(write)->[path filter]|->[stdout]
 * +===========================================+
 *
 * Sort filter is implemented in gtagsop module (libutil/gtagsop.c).
 * Path filter is implemented in convert module (global/convert.c).
 */
/**
 * print number of object.
 *
 * This procedure is commonly used except for the -P option.
 */
void
print_count(int number)
{
	const char *target = format == FORMAT_PATH ? "file" : "object";

	switch (number) {
	case 0:
		fprintf(stderr, "object not found");
		break;
	case 1:
		fprintf(stderr, "1 %s located", target);
		break;
	default:
		fprintf(stderr, "%d %ss located", number, target);
		break;
	}
}
/**
 * idutils:  lid(idutils) pattern
 *
 *	@param[in]	pattern	POSIX regular expression
 *	@param[in]	dbpath	"GTAGS" directory
 */
void
idutils(const char *pattern, const char *dbpath)
{
	FILE *ip;
	CONVERT *cv;
	STRBUF *ib = strbuf_open(0);
	char encoded_pattern[IDENTLEN];
	char path[MAXPATHLEN];
	const char *lid;
	int linenum, count;
	char *p, *q, *grep;

	lid = usable("lid");
	if (!lid)
		die("lid(idutils) not found.");
	if (!test("f", makepath(dbpath, "ID", NULL)))
		die("ID file not found.");
	/*
	 * convert spaces into %FF format.
	 */
	encode(encoded_pattern, sizeof(encoded_pattern), pattern);
	/*
	 * make lid command line.
	 * Invoke lid with the --result=grep option to generate grep format.
	 */
#if _WIN32 || __DJGPP__
	strbuf_puts(ib, lid);
#else
	strbuf_sprintf(ib, "PWD=%s %s", quote_shell(root), lid);
#endif
	strbuf_sprintf(ib, " --file=%s/ID", quote_shell(dbpath));
	strbuf_puts(ib, " --separator=newline");
	if (format == FORMAT_PATH)
		strbuf_puts(ib, " --result=filenames --key=none");
	else
		strbuf_puts(ib, " --result=grep");
	if (iflag)
		strbuf_puts(ib, " --ignore-case");
	if (literal)
		strbuf_puts(ib, " --literal");
	else if (isregex(pattern))
		strbuf_puts(ib, " --regexp");
	strbuf_putc(ib, ' ');
	strbuf_puts(ib, quote_shell(pattern));
	if (debug)
		fprintf(stderr, "idutils: %s\n", strbuf_value(ib));
	if (!(ip = popen(strbuf_value(ib), "r")))
		die("cannot execute '%s'.", strbuf_value(ib));
	cv = convert_open(type, format, root, cwd, dbpath, stdout, NOTAGS);
	cv->tag_for_display = encoded_pattern;
	count = 0;
	strcpy(path, "./");
	while ((grep = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		q = path + 2;
		/* extract path name */
		if (*grep == '/')
			die("The path in the output of lid is assumed relative.\n'%s'", grep);
		p = grep;
		while (*p && *p != ':')
			*q++ = *p++;
		*q = '\0'; 
		if ((xflag || tflag) && !*p)
			die("invalid lid(idutils) output format(1). '%s'", grep);
		p++;
		if (Sflag) {
			if (!locatestring(path, localprefix, MATCH_AT_FIRST))
				continue;
		}
		count++;
		switch (format) {
		case FORMAT_PATH:
			convert_put_path(cv, NULL, path);
			break;
		default:
			/* extract line number */
			while (*p && isspace(*p))
				p++;
			linenum = 0;
			for (linenum = 0; *p && isdigit(*p); linenum = linenum * 10 + (*p++ - '0'))
				;
			if (*p != ':')
				die("invalid lid(idutils) output format(2). '%s'", grep);
			if (linenum <= 0)
				die("invalid lid(idutils) output format(3). '%s'", grep);
			p++;
			/*
			 * print out.
			 */
			convert_put_using(cv, pattern, path, linenum, p, NULL);
			break;
		}
	}
	if (pclose(ip) != 0)
		die("terminated abnormally (errno = %d).", errno);
	convert_close(cv);
	strbuf_close(ib);
	if (vflag) {
		print_count(count);
		fprintf(stderr, " (using idutils index in '%s').\n", dbpath);
	}
}
/**
 * grep: grep pattern
 *
 *	@param[in]	pattern	POSIX regular expression
 *	@param	argv
 *	@param	dbpath
 */
void
grep(const char *pattern, char *const *argv, const char *dbpath)
{
	FILE *fp;
	CONVERT *cv;
	GFIND *gp = NULL;
	STRBUF *ib = strbuf_open(MAXBUFLEN);
	const char *path;
	char encoded_pattern[IDENTLEN];
	const char *buffer;
	int linenum, count;
	int flags = 0;
	int target = GPATH_SOURCE;
	regex_t	preg;
	int user_specified = 1;

	/*
	 * convert spaces into %FF format.
	 */
	encode(encoded_pattern, sizeof(encoded_pattern), pattern);
	/*
	 * literal search available?
	 */
	if (!literal) {
		const char *p = pattern;
		int normal = 1;

		for (; *p; p++) {
			if (!(isalpha(*p) || isdigit(*p) || isblank(*p) || *p == '_')) {
				normal = 0;
				break;
			}
		}
		if (normal)
			literal = 1;
	}
	if (oflag)
		target = GPATH_BOTH;
	if (Oflag)
		target = GPATH_OTHER;
	if (literal) {
		literal_comple(pattern);
	} else {
		if (!Gflag)
			flags |= REG_EXTENDED;
		if (iflag)
			flags |= REG_ICASE;
		if (regcomp(&preg, pattern, flags) != 0)
			die("invalid regular expression.");
	}
	cv = convert_open(type, format, root, cwd, dbpath, stdout, NOTAGS);
	cv->tag_for_display = encoded_pattern;
	count = 0;

	if (*argv && file_list)
		args_open_both(argv, file_list);
	else if (*argv)
		args_open(argv);
	else if (file_list)
		args_open_filelist(file_list);
	else {
		args_open_gfind(gp = gfind_open(dbpath, localprefix, target));
		user_specified = 0;
	}
	while ((path = args_read()) != NULL) {
		if (user_specified) {
			static char buf[MAXPATHLEN];

			if (normalize(path, get_root_with_slash(), cwd, buf, sizeof(buf)) == NULL) {
				warning("'%s' is out of the source project.", path);
				continue;
			}
			if (test("d", buf)) {
				warning("'%s' is a directory. Ignored.", path);
				continue;
			}
			if (!test("f", buf)) {
				warning("'%s' not found. Ignored.", path);
				continue;
			}
			path = buf;
		}
		if (Sflag && !locatestring(path, localprefix, MATCH_AT_FIRST))
			continue;
		if (literal) {
			int n = literal_search(cv, path);
			if (n > 0)
				count += n;
		} else {
			if (!(fp = fopen(path, "r")))
				die("cannot open file '%s'.", path);
			linenum = 0;
			while ((buffer = strbuf_fgets(ib, fp, STRBUF_NOCRLF)) != NULL) {
				int result = regexec(&preg, buffer, 0, 0, 0);
				linenum++;
				if ((!Vflag && result == 0) || (Vflag && result != 0)) {
					count++;
					if (format == FORMAT_PATH) {
						convert_put_path(cv, NULL, path);
						break;
					} else {
						convert_put_using(cv, pattern, path, linenum, buffer,
							(user_specified) ? NULL : gp->dbop->lastdat);
					}
				}
			}
			fclose(fp);
		}
	}
	args_close();
	convert_close(cv);
	strbuf_close(ib);
	if (literal == 0)
		regfree(&preg);
	if (vflag) {
		print_count(count);
		fprintf(stderr, " (no index used).\n");
	}
}
/**
 * pathlist: print candidate path list.
 *
 *	@param[in]	pattern
 *	@param[in]	dbpath
 */
void
pathlist(const char *pattern, const char *dbpath)
{
	GFIND *gp;
	CONVERT *cv;
	const char *path, *p;
	regex_t preg;
	int count;
	int target = GPATH_SOURCE;

	if (oflag)
		target = GPATH_BOTH;
	if (Oflag)
		target = GPATH_OTHER;
	if (pattern) {
		int flags = 0;
		char edit[IDENTLEN];

		if (!Gflag)
			flags |= REG_EXTENDED;
		if (iflag || getconfb("icase_path"))
			flags |= REG_ICASE;
#if _WIN32 || __DJGPP__
		else if (!Mflag)
			flags |= REG_ICASE;
#endif /* _WIN32 */
		/*
		 * We assume '^aaa' as '^/aaa'.
		 */
		if (literal) {
			strlimcpy(edit, quote_string(pattern), sizeof(edit));
			pattern = edit;
		} else if (*pattern == '^' && *(pattern + 1) != '/') {
			snprintf(edit, sizeof(edit), "^/%s", pattern + 1);
			pattern = edit;
		}
		if (regcomp(&preg, pattern, flags) != 0)
			die("invalid regular expression.");
	}
	if (!localprefix)
		localprefix = "./";
	cv = convert_open(type, format, root, cwd, dbpath, stdout, GPATH);
	cv->tag_for_display = "path";
	count = 0;

	gp = gfind_open(dbpath, localprefix, target);
	while ((path = gfind_read(gp)) != NULL) {
		/*
		 * skip localprefix because end-user doesn't see it.
		 */
		p = path + strlen(localprefix) - 1;
		if (pattern) {
			int result = regexec(&preg, p, 0, 0, 0);

			if ((!Vflag && result != 0) || (Vflag && result == 0))
				continue;
		} else if (Vflag)
			continue;
		if (format == FORMAT_PATH)
			convert_put_path(cv, pattern, path);
		else
			convert_put_using(cv, pattern, path, 1, " ", gp->dbop->lastdat);
		count++;
	}
	gfind_close(gp);
	convert_close(cv);
	if (pattern)
		regfree(&preg);
	if (vflag) {
		switch (count) {
		case 0:
			fprintf(stderr, "file not found");
			break;
		case 1:
			fprintf(stderr, "1 file located");
			break;
		default:
			fprintf(stderr, "%d files located", count);
			break;
		}
		fprintf(stderr, " (using '%s').\n", makepath(dbpath, dbname(GPATH), NULL));
	}
}
/**
 * void parsefile(char *const *argv, const char *cwd, const char *root, const char *dbpath, int db)
 *
 * parsefile: parse file to pick up tags.
 *
 *	@param[in]	argv
 *	@param[in]	cwd	current directory
 *	@param[in]	root	root directory of source tree
 *	@param[in]	dbpath	dbpath
 *	@param[in]	db	type of parse
 */
#define TARGET_DEF	(1 << GTAGS)
#define TARGET_REF	(1 << GRTAGS)
#define TARGET_SYM	(1 << GSYMS)
struct parsefile_data {
	CONVERT *cv;
	DBOP *dbop;
	int target;
	int extractmethod;
	int count;
	const char *fid;			/**< fid of the file under processing */
};
static void
put_syms(int type, const char *tag, int lno, const char *path, const char *line_image, void *arg)
{
	struct parsefile_data *data = arg;
	const char *key;

	if (format == FORMAT_PATH && data->count > 0)
		return;
	switch (type) {
	case PARSER_DEF:
		if (!(data->target & TARGET_DEF))
			return;
		break;
	case PARSER_REF_SYM:
		if (!(data->target & (TARGET_REF | TARGET_SYM)))
			return;
		/*
		 * extract method when class method definition.
		 *
		 * Ex: Class::method(...)
		 *
		 * key	= 'method'
		 * data = 'Class::method  103 ./class.cpp ...'
		 */
		if (data->extractmethod) {
			if ((key = locatestring(tag, ".", MATCH_LAST)) != NULL)
				key++;
			else if ((key = locatestring(tag, "::", MATCH_LAST)) != NULL)
				key += 2;
			else
				key = tag;
		} else {
			key = tag;
		}
		if (data->target == TARGET_REF || data->target == TARGET_SYM) {
			if (dbop_get(data->dbop, key) != NULL) {
				if (!(data->target & TARGET_REF))
					return;
			} else {
				if (!(data->target & TARGET_SYM))
					return;
			}
		}
		break;
	default:
		return;
	}
	convert_put_using(data->cv, tag, path, lno, line_image, data->fid);
	data->count++;
}
void
parsefile(char *const *argv, const char *cwd, const char *root, const char *dbpath, int db)
{
	int count = 0;
	int flags = 0;
	STRBUF *sb = strbuf_open(0);
	char *langmap;
	const char *plugin_parser, *av;
	char path[MAXPATHLEN];
	struct parsefile_data data;

	flags = 0;
	if (vflag)
		flags |= PARSER_VERBOSE;
	if (debug)
		flags |= PARSER_DEBUG;
	/*
	if (wflag)
		flags |= PARSER_WARNING;
	*/
	if (db == GRTAGS + GSYMS)
		data.target = TARGET_REF|TARGET_SYM;
	else
		data.target = 1 << db;
	data.extractmethod = getconfb("extractmethod");
	if (getconfs("langmap", sb))
		langmap = check_strdup(strbuf_value(sb));
	else
		langmap = NULL;
	strbuf_reset(sb);
	if (getconfs("gtags_parser", sb))
		plugin_parser = strbuf_value(sb);
	else
		plugin_parser = NULL;
	data.cv = convert_open(type, format, root, cwd, dbpath, stdout, db);
	if (gpath_open(dbpath, 0) < 0)
		die("GPATH not found.");
	if (data.target == TARGET_REF || data.target == TARGET_SYM) {
		data.dbop = dbop_open(makepath(dbpath, dbname(GTAGS), NULL), 0, 0, 0);
		if (data.dbop == NULL)
			die("%s not found.", dbname(GTAGS));
	} else {
		data.dbop = NULL;
	}
	data.fid = NULL;
	parser_init(langmap, plugin_parser);
	if (langmap != NULL)
		free(langmap);

	if (*argv && file_list)
		args_open_both(argv, file_list);
	else if (*argv)
		args_open(argv);
	else if (file_list)
		args_open_filelist(file_list);
	else
		args_open_nop();
	while ((av = args_read()) != NULL) {
		/*
		 * convert the path into relative to the root directory of source tree.
		 */
		if (normalize(av, get_root_with_slash(), cwd, path, sizeof(path)) == NULL) {
			if (!qflag)
				die("'%s' is out of the source project.", av);
			continue;
		}
		if (!test("f", makepath(root, path, NULL))) {
			if (!qflag) {
				if (test("d", NULL))
					die("'%s' is not a source file.", av);
				else
					die("'%s' not found.", av);
			}
			continue;
		}
		/*
		 * Memorize the file id of the path. This is used in put_syms().
		 */
		{
			static char s_fid[MAXFIDLEN];
			int type = 0;
			const char *p = gpath_path2fid(path, &type);

			if (!p || type != GPATH_SOURCE) {
				if (!qflag)
					die("'%s' is not a source file.", av);
				continue;
			}
			strlimcpy(s_fid, p, sizeof(s_fid));
			data.fid = s_fid;
		}
		if (Sflag && !locatestring(path, localprefix, MATCH_AT_FIRST))
			continue;
		data.count = 0;
		parse_file(path, flags, put_syms, &data);
		count += data.count;
	}
	args_close();
	parser_exit();
	/*
	 * Settlement
	 */
	if (data.dbop != NULL)
		dbop_close(data.dbop);
	gpath_close();
	convert_close(data.cv);
	strbuf_close(sb);
	if (vflag) {
		print_count(count);
		fprintf(stderr, " (no index used).\n");
	}
}
/**
 * search: search specified function 
 *
 *	@param[in]	pattern		search pattern
 *	@param[in]	root		root of source tree
 *	@param[in]	cwd		current directory
 *	@param[in]	dbpath		database directory
 *	@param[in]	db		GTAGS,GRTAGS,GSYMS
 *	@return			count of output lines
 */
int
search(const char *pattern, const char *root, const char *cwd, const char *dbpath, int db)
{
	CONVERT *cv;
	int count = 0;
	GTOP *gtop;
	GTP *gtp;
	int flags = 0;

	start_output();
	/*
	 * open tag file.
	 */
	gtop = gtags_open(dbpath, root, db, GTAGS_READ, debug ? GTAGS_DEBUG : 0);
	cv = convert_open(type, format, root, cwd, dbpath, stdout, db);
	/*
	 * search through tag file.
	 */
	if (nofilter & SORT_FILTER)
		flags |= GTOP_NOSORT;
	else if (Nflag)
		flags |= GTOP_NEARSORT;
	if (literal)
		flags |= GTOP_NOREGEX;
	else if (Gflag)
		flags |= GTOP_BASICREGEX;
	if (format == FORMAT_PATH)
		flags |= GTOP_PATH;
	if (iflag)
		flags |= GTOP_IGNORECASE;
	for (gtp = gtags_first(gtop, pattern, flags); gtp; gtp = gtags_next(gtop)) {
		if (Sflag && !locatestring(gtp->path, localprefix, MATCH_AT_FIRST))
			continue;
		count += output_with_formatting(cv, gtp, root, gtop->format);
	}
	convert_close(cv);
	if (debug)
		gtags_show_statistics(gtop);
	gtags_close(gtop);
	end_output();
	return count;
}
/**
 * tagsearch: execute tag search
 *
 *	@param[in]	pattern		search pattern
 *	@param[in]	cwd		current directory
 *	@param[in]	root		root of source tree
 *	@param[in]	dbpath		database directory
 *	@param[in]	db		GTAGS,GRTAGS,GSYMS
 */
void
tagsearch(const char *pattern, const char *cwd, const char *root, const char *dbpath, int db)
{
	int count, total = 0;
	char buffer[IDENTLEN], *p = buffer;
	char libdbpath[MAXPATHLEN];

	/*
	 * trim pattern (^<no regex>$ => <no regex>)
	 */
	if (!literal && pattern) {
		strlimcpy(p, pattern, sizeof(buffer));
		if (*p++ == '^') {
			char *q = p + strlen(p);
			if (*--q == '$') {
				*q = 0;
				if (*p == 0 || !isregex(p))
					pattern = p;
			}
		}
	}
	/*
	 * search in current source tree.
	 */
	count = search(pattern, root, cwd, dbpath, db);
	total += count;
	/*
	 * search in library path.
	 */
	if (abslib)
		type = PATH_ABSOLUTE;
	if (db == GTAGS && getenv("GTAGSLIBPATH") && (count == 0 || Tflag) && !Sflag) {
		STRBUF *sb = strbuf_open(0);
		char *libdir, *nextp = NULL;

		strbuf_puts(sb, getenv("GTAGSLIBPATH"));
		back2slash(sb);
		/*
		 * search for each tree in the library path.
		 */
		for (libdir = strbuf_value(sb); libdir; libdir = nextp) {
			if ((nextp = locatestring(libdir, PATHSEP, MATCH_FIRST)) != NULL)
				*nextp++ = 0;
			if (!gtagsexist(libdir, libdbpath, sizeof(libdbpath), 0))
				continue;
			if (!STRCMP(dbpath, libdbpath))
				continue;
			if (!test("f", makepath(libdbpath, dbname(db), NULL)))
				continue;
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
		print_count(total);
		if (!Tflag)
			fprintf(stderr, " (using '%s')", makepath(dbpath, dbname(db), NULL));
		fputs(".\n", stderr);
	}
}
/*
 * encode: string copy with converting blank chars into %ff format.
 *
 *	@param[out]	to	result
 *	@param[in]	size	size of to buffer
 *	@param[in]	from	string
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
