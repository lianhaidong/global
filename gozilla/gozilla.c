/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2002, 2003, 2004, 2006,
 *	2011, 2014
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
/*
 * Don't remove the following code which seems meaningless.
 * Since WIN32 has another SLIST_ENTRY, we removed the definition
 * so as not to cause the conflict.
 */
#ifdef SLIST_ENTRY
#undef SLIST_ENTRY
#endif
#endif

#include "global.h"
#include "regex.h"
#include "const.h"

/**
 gozilla - force mozilla browser to display specified part of a source file.
*/

static void usage(void);
static void help(void);

int main(int, char **);
void getdefinitionURL(const char *, const char *, STRBUF *);
void getURL(const char *, const char *, STRBUF *);
int convertpath(const char *, const char *, const char *, STRBUF *);
void makefileurl(const char *, int, STRBUF *);
void show_page_by_url(const char *, const char *);
#ifndef isblank
#define isblank(c)	((c) == ' ' || (c) == '\t')
#endif

const char *cwd, *root, *dbpath;

int pflag;
int qflag;
int vflag;
int show_version;
int linenumber = 0;
int debug;

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

/**
 * locate_HTMLdir: locate HTML directory made by htags(1).
 *
 *	@return		HTML directory
 */
static const char *
locate_HTMLdir(void)
{
	static char htmldir[MAXPATHLEN];

	if (test("d", makepath(dbpath, "HTML", NULL)))
		strlimcpy(htmldir, makepath(dbpath, "HTML", NULL), sizeof(htmldir));
	else if (test("d", makepath(root, "HTML", NULL)))
		strlimcpy(htmldir, makepath(root, "HTML", NULL), sizeof(htmldir));
	else if (test("d", makepath(root, "html/HTML", NULL)))
		/* Doxygen makes HTML in doxygen's html directory. */
		strlimcpy(htmldir, makepath(root, "html/HTML", NULL), sizeof(htmldir));
	else
		return NULL;
	if (vflag)
		fprintf(stdout, "HTML directory '%s'.\n", htmldir);
	return (const char *)htmldir;
}
int
main(int argc, char **argv)
{
	char c;
	const char *p, *browser = NULL, *definition = NULL;
	STRBUF *URL = strbuf_open(0);

	while (--argc > 0 && ((c = (++argv)[0][0]) == '-' || c == '+')) {
		if (argv[0][1] == '-') {
			if (!strcmp("--help", argv[0]))
				help();
			else if (!strcmp("--version", argv[0]))
				show_version++;
			else if (!strcmp("--quiet", argv[0])) {
				qflag++;
				vflag = 0;
			} else if (!strcmp("--verbose", argv[0])) {
				vflag++;
				qflag = 0;
			} else if (!strcmp("--debug", argv[0])) {
				debug = 1;
			} else
				usage();
			continue;
		}
		if (c == '+') {
			linenumber = atoi(argv[0] + 1);
			continue;
		}
		p = argv[0] + 1;
		switch (*p) {
		case 'b':
			browser = argv[1];
			--argc; ++argv;
			break;
		case 'd':
			definition = argv[1];
			--argc; ++argv;
			break;
		case 'p':
			pflag++;
			break;
		case 'q':
			qflag++;
			setquiet();
			break;
		case 'v':
			vflag++;
			setverbose();
			break;
		default:
			usage();
		}
	}
	if (show_version)
		version(progname, vflag);
	if (!definition) {
		if (argc <= 0)
			usage();
		if (!test("f", argv[0]))
			die("file '%s' not found.", argv[0]);
	}
	/*
	 * Open configuration file.
	 */
	openconf(NULL);
	/*
	 * Decide browser.
	 */
	if (!browser && getenv("BROWSER"))
		browser = getenv("BROWSER");
	/*
	 * In DOS & Windows, let the file: association handle it.
	 */
#if !(_WIN32 || __DJGPP__)
	if (!browser)
		browser = "firefox";
#endif
	/*
	 * Get URL.
	 */
	{
		const char *HTMLdir = NULL;

		if (setupdbpath(0) == 0) {
			cwd = get_cwd();
			root = get_root();
			dbpath = get_dbpath();
			HTMLdir = locate_HTMLdir();
		} 
		/*
		 * Make a URL of hypertext from the argument.
		 */
		if (HTMLdir == NULL)
			die("HTML directory not found.");
		if (definition)
			getdefinitionURL(definition, HTMLdir, URL);
		else
			getURL(argv[0], HTMLdir, URL);
	}
	if (pflag) {
		fprintf(stdout, "%s\n", strbuf_value(URL));
		if (vflag)
			fprintf(stdout, "using browser '%s'.\n", browser);
		exit(0);
	}
	/*
	 * Show URL's page.
	 */
	show_page_by_url(browser, strbuf_value(URL));
	exit(0);
}

/**
 * getdefinitionURL: get URL includes specified definition.
 *
 *	@param[in]	arg	definition name
 *	@param[in]	htmldir HTML directory
 *	@param[out]	URL	URL begin with 'file:'
 */
void
getdefinitionURL(const char *arg, const char *htmldir, STRBUF *URL)
{
	FILE *fp;
	char *p;
	SPLIT ptable;
	int status = -1;
	STRBUF *sb = strbuf_open(0);
	const char *path = makepath(htmldir, "D", NULL);

	if (!test("d", path))
		die("'%s' not found. Please invoke htags(1) without the -D option.", path);
	path = makepath(htmldir, "MAP", NULL);
	if (!test("f", path))
		die("'%s' not found. Please invoke htags(1) with the --map-file option.", path);
	fp = fopen(path, "r");
	if (!fp)
		die("cannot open '%s'.", path);
	while ((p = strbuf_fgets(sb, fp, STRBUF_NOCRLF)) != NULL) {
		if (split(p, 2, &ptable) != 2)
			die("invalid format.");
		if (!strcmp(arg, ptable.part[0].start)) {
			status = 0;
			break;
		}
	}
	fclose(fp);
	if (status == -1)
		die("definition %s not found.", arg);
	strbuf_reset(URL);
	/*
	 * convert path into URL.
	 */
	makefileurl(makepath(htmldir, ptable.part[1].start, NULL), 0, URL);
	recover(&ptable);
	strbuf_close(sb);
}
/**
 * getURL: get URL of the specified file.
 *
 *	@param[in]	file	file name
 *	@param[in]	htmldir HTML directory
 *	@param[out]	URL	URL begin with 'file:'
 */
void
getURL(const char *file, const char *htmldir, STRBUF *URL)
{
	char *p;
	char buf[MAXPATHLEN];
	STRBUF *sb = strbuf_open(0);
	p = normalize(file, get_root_with_slash(), cwd, buf, sizeof(buf));
	if (p != NULL && convertpath(dbpath, htmldir, p, sb) == 0)
		makefileurl(strbuf_value(sb), linenumber, URL);
	else
		makefileurl(realpath(file, buf), 0, URL);
	strbuf_close(sb);
}
/**
 * convertpath: convert source file into hypertext path.
 *
 *	@param[in]	dbpath	dbpath
 *	@param[in]	htmldir	HTML directory made by htags(1)
 *	@param[in]	path	source file path
 *	@param[out]	sb	string buffer
 *	@return		0: normal, -1: error
 */
int
convertpath(const char *dbpath, const char *htmldir, const char *path, STRBUF *sb)
{
	static const char *suffix[] = {".html", ".htm"};
	static const char *gz = ".gz";
	int i, lim = sizeof(suffix)/sizeof(char *);
	const char *p;

	strbuf_reset(sb);
	strbuf_puts(sb, htmldir);
	strbuf_puts(sb, "/S/");
	/*
	 * new style.
	 */
	if (gpath_open(dbpath, 0) == 0) {
		int tag1 = strbuf_getlen(sb);

		p = gpath_path2fid(path, NULL);
		if (p == NULL) {
			gpath_close();
			return -1;
		}
		gpath_close();
		strbuf_puts(sb, p);
		for (i = 0; i < lim; i++) {
			int tag2 = strbuf_getlen(sb);
			strbuf_puts(sb, suffix[i]);
			if (test("f", strbuf_value(sb)))
				return 0;
			strbuf_puts(sb, gz);
			if (test("f", strbuf_value(sb)))
				return 0;
			strbuf_setlen(sb, tag2);
		}
		strbuf_setlen(sb, tag1);
	}
	/*
	 * old style.
	 */
	for (p = path + 1; *p; p++)
		strbuf_putc(sb, (*p == '/') ? ' ' : *p);
	for (i = 0; i < lim; i++) {
		int tag = strbuf_getlen(sb);
		strbuf_puts(sb, suffix[i]);
		if (test("f", strbuf_value(sb)))
			return 0;
		strbuf_puts(sb, gz);
		if (test("f", strbuf_value(sb)))
			return 0;
		strbuf_setlen(sb, tag);
	}
	return -1;
}
/**
 * makefileurl: make url which start with 'file:'.
 *
 *	@param[in]	path	path name (absolute)
 *	@param[in]	line	!=0: line number
 *	@param[out]	url	URL
 *
 * Examples:
 * makefileurl('/dir/a.html', 10)   => 'file:///dir/a.html#L10'
 *
 * (Windows32 environment)
 * makefileurl('c:/dir/a.html', 10) => 'file://c|/dir/a.html#L10'
 */
void
makefileurl(const char *path, int line, STRBUF *url)
{
	strbuf_puts(url, "file://");
#if _WIN32 || __DJGPP__
	/*
	 * copy drive name. (c: -> c|)
	 */
	if (isalpha(*path) && *(path+1) == ':') {
		strbuf_putc(url, *path);
		strbuf_putc(url, '|');
		path += 2;
	}
#endif
	strbuf_puts(url, path);
	if (line) {
		strbuf_puts(url, "#L");
		strbuf_putn(url, line);
	}
}
/**
 * show_page_by_url: show page by url
 *
 *	@param[in]	browser browser name
 *	@param[in]	url	URL
 */
#if defined(_WIN32)
/* Windows32 version */
void
show_page_by_url(const char *browser, const char *url)
{
	const char *lpFile, *lpParameters;
	if (browser) {
		lpFile = browser;
		lpParameters = url;
	} else {
		lpFile = url;
		lpParameters = NULL;
	}
	if (ShellExecute(NULL, NULL, lpFile, lpParameters, NULL, SW_SHOWNORMAL) <= (HINSTANCE)32)
		die("Cannot load %s (error = 0x%04x).", lpFile, GetLastError());
}
#elif defined(__DJGPP__)
/* DJGPP version */
void
show_page_by_url(const char *browser, const char *url)
{
	char com[MAXFILLEN];
	char *path;

	if (!browser) {
		browser = "";
	}
	/*
	 * assume a Windows browser if it's not on the path.
	 */
	if (!(path = usable(browser))) {
		/*
		 * START is an internal command in XP, external in 9X.
		 */
		if (!(path = usable("start")))
			path = "cmd /c start \"\"";
		snprintf(com, sizeof(com), "%s %s \"%s\"", path, browser, url);
	} else {
		snprintf(com, sizeof(com), "%s \"%s\"", path, url);
	}
	system(com);
}
#else
/* UNIX version */
/*
 * Make a html file to use OSX's open command.
 */
#include <pwd.h>
static char urlfile[MAXPATHLEN];
static char *
make_url_file(const char *url)
{
	STATIC_STRBUF(sb);
	FILE *op;
	struct passwd *pw = getpwuid(getuid());

	strbuf_clear(sb);
	getconfs("localstatedir", sb);
	snprintf(urlfile, sizeof(urlfile), "%s/gtags/lasturl-%s.html", strbuf_value(sb),
			pw ? pw->pw_name : "nobody");
	op = fopen(urlfile, "w");
	if (op == NULL)
		die("cannot make url file.");
	fprintf(op, "<!-- You may remove this file. It was needed by gozilla(1) temporarily. -->\n");
	fprintf(op, "<html><head>\n");
	fprintf(op, "<meta http-equiv='refresh' content='0;url=\"%s\"' />\n", url);
	fprintf(op, "</head></html>\n");
	fclose(op);

	return urlfile;
}
void
dump_argv(char *argv[]) {
	int i;
	for (i = 0; argv[i] != NULL; i++) {
		fprintf(stderr, "argv[%d] = |%s|\n", i, argv[i]);
	}
}
void
show_page_by_url(const char *browser, const char *url)
{
	char *argv[4];
	STRBUF  *sb = strbuf_open(0);
	STRBUF  *arg = strbuf_open(0);
	/*
	 * Browsers which have openURL() command.
	 */
	if (locatestring(browser, "mozilla", MATCH_AT_LAST) ||
	/*
	 * Firefox has removed the -remote command line option since version 39.
	    locatestring(browser, "firefox", MATCH_AT_LAST) ||
	 */
	    locatestring(browser, "netscape", MATCH_AT_LAST) ||
	    locatestring(browser, "netscape-remote", MATCH_AT_LAST))
	{
		if (debug)
			fprintf(stderr, "Netscape\n");
		argv[0] = (char *)browser;
		argv[1] = "-remote";
		strbuf_sprintf(arg, "openURL(%s)", url);
		argv[2] = strbuf_value(arg);
		argv[3] = NULL;
		if (debug)
			dump_argv(argv);
		execvp(browser, argv);
	}
	/*
	 * Load default browser of OSX.
	 */
	else if (!strcmp(browser, "osx-default")) {
		if (debug)
			fprintf(stderr, "OSX default\n");
		argv[0] = "open";
		argv[1] = make_url_file(url);
		argv[2] = NULL;
		if (debug)
			dump_argv(argv);
		execvp("open", argv);
	}
	/*
	 * Generic browser.
	 */
	else {
		if (debug)
			fprintf(stderr, "Generic browser\n");
		argv[0] = (char *)browser;
		argv[1] = (char *)url;
		argv[2] = NULL;
		if (debug)
			dump_argv(argv);
		execvp(browser, argv);
	}
	strbuf_close(sb);
	strbuf_close(arg);
}
#endif
