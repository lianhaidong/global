/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2002, 2003, 2004
 *	Tama Communications Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include "global.h"
#include "regex.h"
#include "const.h"

static void usage(void);
static void help(void);

const char *gozillarc = ".gozillarc";
#ifdef __DJGPP__
const char *dos_gozillarc = "_gozillarc";
#endif

static const char *alias(const char *);
int main(int, char **);
void getdefinitionURL(const char *, STRBUF *);
void getURL(const char *, STRBUF *);
int isprotocol(const char *);
int issource(const char *);
int convertpath(const char *, const char *, const char *, STRBUF *);
#if !defined(_WIN32) && !defined(__DJGPP__)
void sendbrowser(const char *, const char *);
#endif

#ifndef isblank
#define isblank(c)	((c) == ' ' || (c) == '\t')
#endif
int bflag;
int pflag;
int qflag;
int Cflag;
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

/*
 * alias: get alias value.
 *
 *	i)	alias_name	alias name
 *	r)			its value
 *
 * [$HOME/.gozillarc]
 * +-----------------------
 * |a:http://www.gnu.org
 * |f = file:/usr/share/xxx.html
 * |www	http://www.xxx.yyy/
 */
static const char *
alias(alias_name)
	const char *alias_name;
{
	FILE *ip;
	STRBUF *sb = strbuf_open(0);
	char *p;
	const char *alias = NULL;
	int flag = STRBUF_NOCRLF;

	if (!(p = get_home_directory()))
		goto end;
	if (!test("r", makepath(p, gozillarc, NULL)))
#ifdef __DJGPP__
		if (!test("r", makepath(p, dos_gozillarc, NULL)))
#endif
			goto end;
	if (!(ip = fopen(makepath(p, gozillarc, NULL), "r")))
#ifdef __DJGPP__
		if (!(ip = fopen(makepath(p, dos_gozillarc, NULL), "r")))
#endif
			goto end;
	while ((p = strbuf_fgets(sb, ip, flag)) != NULL) {
		char *name, *value;

		flag &= ~STRBUF_APPEND;
		if (*p == '#')
			continue;
		if (strbuf_unputc(sb, '\\')) {
			flag |= STRBUF_APPEND;
			continue;
		}
		while (*p && isblank(*p))	/* skip spaces */
			p++;
		name = p;
		while (*p && isalnum(*p))	/* get name */
			p++;
		*p++ = 0;
		if (!strcmp(alias_name, name)) {
			while (*p && isblank(*p))	/* skip spaces */
				p++;
			if (*p == '=' || *p == ':') {
				p++;
				while (*p && isblank(*p))/* skip spaces */
					p++;
			}
			value = p;
			while (*p && !isblank(*p))	/* get value */
				p++;
			*p = 0;
			alias = strmake(value, "");
			break;
		}
	}
	fclose(ip);
end:
	strbuf_close(sb);
	return alias;
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char c;
	const char *p, *browser = NULL, *definition = NULL;
	STRBUF *arg = strbuf_open(0);
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
			break;
		default:
			usage();
		}
	}
	if (show_version)
		version(progname, vflag);
	if (!browser && getenv("BROWSER"))
		browser = getenv("BROWSER");
	if (!browser)
		browser = "mozilla";
	if (definition == NULL) {
		if (argc == 0)
			usage();
		strbuf_puts(arg, argv[0]);
		/*
		 * Replace with alias value.
		 */
		if ((p = alias(strbuf_value(arg))) != NULL) {
			strbuf_reset(arg);
			strbuf_puts(arg, p);
		}
	}
	/*
	 * Get URL.
	 */
	if (definition)
		getdefinitionURL(definition, URL);
	else if (isprotocol(strbuf_value(arg)))
		strbuf_puts(URL, strbuf_value(arg));
	else
		getURL(strbuf_value(arg), URL);
	if (pflag) {
		fprintf(stdout, "%s\n", strbuf_value(URL));
		if (vflag)
			fprintf(stdout, "using browser '%s'.\n", browser);
		exit(0);
	}
#ifdef _WIN32
	if (ShellExecute(NULL, NULL, browser, strbuf_value(URL), NULL, SW_SHOWNORMAL) <= (HINSTANCE)32)
		die("Cannot load %s (error = 0x%04x).", browser, GetLastError());
#else
#ifndef __DJGPP__
	/*
	 * send a command to browser.
	 */
	if (locatestring(browser, "mozilla", MATCH_AT_LAST)
		|| locatestring(browser, "netscape", MATCH_AT_LAST)
		|| locatestring(browser, "netscape-remote", MATCH_AT_LAST))
	{
		sendbrowser(browser, strbuf_value(URL));
	}
	/*
	 * execute generic browser.
	 */
	else
#endif /* !__DJGPP__ */
	{
		char com[MAXFILLEN+1];

#ifdef __DJGPP__
		char *path;
		/*
		 * assume a Windows browser if it's not on the path.
		 */
		if (!(path = usable(browser)))
		{
			/*
			 * START is an internal command in XP, external in 9X.
			 */
			if (!(path = usable("start")))
				path = "cmd /c start";
			snprintf(com, sizeof(com), "%s %s \"%s\"", path, browser, strbuf_value(URL));
		}
		else
			snprintf(com, sizeof(com), "%s \"%s\"", path, strbuf_value(URL));
#else
		snprintf(com, sizeof(com), "%s \"%s\"", browser, strbuf_value(URL));
#endif /* !__DJGPP__ */
		system(com);
	}
#endif /* _WIN32 */
	exit(0);
}

/*
 * getdefinitionURL: get URL includes specified definition.
 *
 *	i)	arg	definition name
 *	o)	URL	URL begin with 'file:'
 */
void
getdefinitionURL(arg, URL)
	const char *arg;
	STRBUF *URL;
{
	char cwd[MAXPATHLEN+1];
	char root[MAXPATHLEN+1];
	char dbpath[MAXPATHLEN+1];
	char htmldir[MAXPATHLEN+1];
	char *path, *p;
	STRBUF *sb = NULL;
	DBOP *dbop = NULL;
	SPLIT ptable;
	int status = -1;

	/*
	 * get current, root and dbpath directory.
	 * if GTAGS not found, getdbpath doesn't return.
	 */
	getdbpath(cwd, root, dbpath, 0);
	if (test("d", makepath(dbpath, "HTML", NULL)))
		strlimcpy(htmldir, makepath(dbpath, "HTML", NULL), sizeof(htmldir));
	else if (test("d", makepath(root, "HTML", NULL)))
		strlimcpy(htmldir, makepath(root, "HTML", NULL), sizeof(htmldir));
	else
		die("hypertext not found. See htags(1).");
	path = makepath(htmldir, "MAP.db", NULL);
	if (test("f", path))
		dbop = dbop_open(path, 0, 0, 0);
	if (!dbop) {
		path = makepath(htmldir, "MAP", NULL);
		if (!test("f", path))
			die("'%s' not found. Please reconstruct hypertext using the latest htags(1).", path);
		dbop = dbop_open(path, 0, 0, 0);
	}
	if (dbop) {
		if ((p = (char *)dbop_get(dbop, arg)) != NULL) {
			if (split(p, 2, &ptable) != 2)
				die("illegal format.");
			status = 0;
		}
		dbop_close(dbop);
	} else {
		FILE *fp;

		sb = strbuf_open(0);
		fp = fopen(path, "r");
		if (fp) {
			while ((p = strbuf_fgets(sb, fp, STRBUF_NOCRLF)) != NULL) {
				if (split(p, 2, &ptable) != 2)
					die("illegal format.");
				if (!strcmp(arg, ptable.part[0].start)) {
					status = 0;
					break;
				}
			}
			fclose(fp);
		}
	}
	if (status == -1)
		die("definition %s not found.", arg);
	strbuf_reset(URL);
	/*
	 * Make URL.
	 *
	 * c:/dir/a.html => file://c|/dir/a.html
	 */
#if _WIN32 || __DJGPP__
	if (htmldir[1] == ':')
		htmldir[1] = '|';
#endif
	strbuf_sprintf(URL, "file://%s/%s", htmldir, ptable.part[1].start);
	recover(&ptable);
	if (sb != NULL)
		strbuf_close(sb);
}
/*
 * getURL: get specified URL.
 *
 *	i)	arg	definition name
 *	o)	URL	URL begin with 'file:'
 */
void
getURL(arg, URL)
	const char *arg;
	STRBUF *URL;
{
	char cwd[MAXPATHLEN+1];
	char root[MAXPATHLEN+1];
	char dbpath[MAXPATHLEN+1];
	char htmldir[MAXPATHLEN+1];
	char *abspath, *p;
	char buf[MAXPATHLEN+1];

	if (!test("f", arg) && !test("d", NULL))
		die("path '%s' not found.", arg);
	if (!(abspath = realpath(arg, buf)))
		die("cannot make absolute path name. realpath(%s) failed.", arg);
	if (!isabspath(abspath))
		die("realpath(3) is not compatible with BSD version.");
	if (issource(abspath)) {
		STRBUF *sb = strbuf_open(0);
		/*
		 * get current, root and dbpath directory.
		 * if GTAGS not found, getdbpath doesn't return.
		 */
		getdbpath(cwd, root, dbpath, 0);
		if (test("d", makepath(dbpath, "HTML", NULL)))
			strlimcpy(htmldir, makepath(dbpath, "HTML", NULL), sizeof(htmldir));
		else if (test("d", makepath(root, "HTML", NULL)))
			strlimcpy(htmldir, makepath(root, "HTML", NULL), sizeof(htmldir));
		else
			die("hypertext not found. See htags(1).");
		/*
		 * convert path into hypertext.
		 */
		p = abspath + strlen(root);
		if (convertpath(dbpath, htmldir, p, sb) == -1)
			die("cannot find the hypertext.");
		p = strbuf_value(sb);
		/*
		 * Make URL.
		 *
		 * c:/dir/a.html => file://c|/dir/a.html
		 */
#if _WIN32 || __DJGPP__
		if (p[1] == ':')
			p[1] = '|';
#endif
		strbuf_sprintf(URL, "file://%s", p);
		if (linenumber)
			strbuf_sprintf(URL, "#%d", linenumber);
		strbuf_close(sb);
	} else {
		/*
		 * Make URL.
		 *
		 * c:/dir/a.html => file://c|/dir/a.html
		 */
#if _WIN32 || __DJGPP__
		if (abspath[1] == ':')
			abspath[1] = '|';
#endif
		strbuf_sprintf(URL, "file://%s", abspath);
	}
}
/*
 * isprotocol: return 1 if url has a procotol.
 *
 *	i)	url	URL
 *	r)		1: protocol, 0: file
 */
int
isprotocol(url)
	const char *url;
{
	const char *p;

	if (!strncmp(url, "file:", 5))
		return 1;
	/*
	 * protocol's style is like http://xxx.
	 */
	for (p = url; *p && *p != ':'; p++)
		if (!isalnum(*p))
			return 0;
	if (!*p)
		return 0;
	if (*p++ == ':' && *p++ == '/' && *p == '/')
		return 1;
	return 0;
}
/*
 * issource: return 1 if path is a source file.
 *
 *	i)	path	path
 *	r)		1: source file, 0: not source file
 */
int
issource(path)
	const char *path;
{
	STRBUF *sb = strbuf_open(0);
	char *p;
	char suff[MAXPATHLEN+1];
	int retval = 0;

	if (!getconfs("suffixes", sb)) {
		strbuf_close(sb);
		return 0;
	}
	suff[0] = '.';
	for (p = strbuf_value(sb); p; ) {
		char *unit = p;
		if ((p = locatestring(p, ",", MATCH_FIRST)) != NULL)
			*p++ = 0;
		strlimcpy(&suff[1], unit, sizeof(suff) - 1);
		if (locatestring(path, suff, MATCH_AT_LAST)) {
			retval = 1;
			break;
		}
	}
	strbuf_close(sb);
	return retval;

}
/*
 * convertpath: convert source file into hypertext path.
 *
 *	i)	dbpath	dbpath
 *	i)	htmldir	HTML directory made by htags(1)
 *	i)	path	source file path
 *	o)	sb	string buffer
 *	r)		0: normal, -1: error
 */
int
convertpath(dbpath, htmldir, path, sb)
	const char *dbpath;
	const char *htmldir;
	const char *path;
	STRBUF *sb;
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
		char key[MAXPATHLEN+1];
		int tag1 = strbuf_getlen(sb);

		strlimcpy(key, "./", sizeof(key));
		strcat(key, path + 1);
		p = gpath_path2fid(key);
		if (p == NULL) {
			gpath_close();
			return -1;
		}
		strlimcpy(key, p, sizeof(key));
		gpath_close();
		strbuf_puts(sb, key);
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
#if !defined(_WIN32) && !defined(__DJGPP__)
/*
 * sendbrowser: send message to mozilla.
 *
 *	i)	browser {mozilla|netscape}
 *	i)	url	URL
 *
 */
void
sendbrowser(browser, url)
	const char *browser;
	const char *url;
{
	int pid;
	char com[1024], *path;

	if (!(path = usable(browser)))
		die("%s not found in your path.", browser);
	snprintf(com, sizeof(com), "openURL(%s)", url);
	if ((pid = fork()) < 0) {
		die("cannot load mozilla (fork).");
	} else if (pid == 0) {
		(void)close(1);
		execlp(path, browser, "-remote", com, NULL);
		die("cannot load %s (execlp).", browser);
	}
}
#endif /* !_WIN32 and !__DJGPP__ */
