/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000, 2002
 *             Tama Communications Corporation. All rights reserved.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "global.h"
#include "regex.h"
#include "const.h"

static void     usage(void);
static void     help(void);

const char *gozillarc = ".gozillarc";

static char *alias(char *);
int	main(int, char **);
int	isprotocol(char *);
int	issource(char *);
int	convertpath(char *, char *, char *, STRBUF *);
void	sendmozilla(char *);
#ifndef _WIN32
int	sendcommand(char *);
#endif

#ifndef isblank
#define isblank(c)	((c) == ' ' || (c) == '\t')
#endif
int	bflag;
int	pflag;
int	qflag;
int	Cflag;
int	vflag;
int	show_version;
int	linenumber = 0;

static void
usage()
{
	if (!qflag)
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
static char *
alias(alias_name)
char	*alias_name;
{
	char rc[MAXPATHLEN+1];
	FILE *ip;
	STRBUF *sb = strbuf_open(0);
	char *p, *alias = NULL;
	int flag = STRBUF_NOCRLF;

	if (!(p = getenv("HOME")))
		goto end;
	strcpy(rc, makepath(p, gozillarc, NULL));
	if (!test("r", rc))
		goto end;
	if (!(ip = fopen(rc, "r")))
		goto end;
	while (p = strbuf_fgets(sb, ip, flag)) {
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
int	argc;
char	*argv[];
{
	char	c, *p, *q;
	char	*browser = NULL;
	char	*command = NULL;
	char	arg[MAXPATHLEN+1];
	char	URL[MAXPATHLEN+1];
	char	com[MAXFILLEN+1];
	int	status;

	while (--argc > 0 && (c = (++argv)[0][0]) == '-' || c == '+') {
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
		case 'p':
			pflag++;
			setquiet();
			break;
		case 'q':
			qflag++;
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
	if (argc == 0)
		usage();
	strcpy(arg, argv[0]);
	if (p = alias(arg))		/* get alias value */
		strcpy(arg, p);
	if (isprotocol(arg)) {
		strcpy(URL, arg);
	} else {
		char	*abspath;
		char	buf[MAXPATHLEN+1];

		if (!test("f", arg) && !test("d", NULL))
			die("path '%s' not found.", arg);
		if (!(abspath = realpath(arg, buf)))
			die("cannot make absolute path name. realpath(%s) failed.", arg);
		if (!isabspath(abspath))
			die("realpath(3) is not compatible with BSD version.");
		if (issource(abspath)) {
			char	cwd[MAXPATHLEN+1];
			char	root[MAXPATHLEN+1];
			char	dbpath[MAXPATHLEN+1];
			char	htmldir[MAXPATHLEN+1];
			STRBUF *sb = strbuf_open(0);
			/*
			 * get current, root and dbpath directory.
			 * if GTAGS not found, getdbpath doesn't return.
			 */
			getdbpath(cwd, root, dbpath, 0);
			if (test("d", makepath(dbpath, "HTML", NULL)))
				strcpy(htmldir, makepath(dbpath, "HTML", NULL));
			else if (test("d", makepath(root, "HTML", NULL)))
				strcpy(htmldir, makepath(root, "HTML", NULL));
			else
				die("hypertext not found. See htags(1).");
			/*
			 * convert path into hypertext.
			 */
			p = abspath + strlen(root);
			if (convertpath(dbpath, htmldir, p, sb) == -1)
				die("cannot find the hypertext.");
			if (linenumber)
				sprintf(URL, "file:%s#%d", strbuf_value(sb), linenumber);
			else {
				strcpy(URL, "file:");
				strcat(URL, strbuf_value(sb));
			}
		} else {
			strcpy(URL, "file:");
			strcat(URL, abspath);
		}
	}
	if (pflag) {
		fprintf(stdout, "%s\n", URL);
		exit(0);
	}
	/*
	 * execute generic browser.
	 */
	if (browser && !locatestring(browser, "mozilla", MATCH_AT_LAST)) {
		sprintf(com, "%s '%s'", browser, URL);
		system(com);
		exit (0);
	}
	/*
	 * send a command to mozilla.
	 */
	sendmozilla(URL);
	exit(0);
}
/*
 * isprotocol: return 1 if url has a procotol.
 *
 *	i)	url	URL
 *	r)		1: protocol, 0: file
 */
int
isprotocol(url)
char	*url;
{
	char	*p;

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
char	*path;
{
	STRBUF	*sb = strbuf_open(0);
	char	*p;
	char	suff[MAXPATHLEN+1];
	int	retval = 0;

	if (!getconfs("suffixes", sb)) {
		strbuf_close(sb);
		return 0;
	}
	suff[0] = '.';
	for (p = strbuf_value(sb); p; ) {
		char    *unit = p;
		if ((p = locatestring(p, ",", MATCH_FIRST)) != NULL)
			*p++ = 0;
		strcpy(&suff[1], unit);
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
char	*dbpath;
char	*htmldir;
char	*path;
STRBUF	*sb;
{
	static const char *suffix[] = {".html", ".htm"};
	static const char *gz = ".gz";
	int i, lim = sizeof(suffix)/sizeof(char *);
	char *p;

	strbuf_reset(sb);
	strbuf_puts(sb, htmldir);
	strbuf_puts(sb, "/S/");
	/*
	 * new style.
	 */
	if (gpath_open(dbpath, 0, 0) == 0) {
		char key[MAXPATHLEN+1];
		int tag1 = strbuf_getlen(sb);

		strcpy(key, "./");
		strcat(key, path + 1);
		p = gpath_path2fid(key);
		if (p == NULL) {
			gpath_close();
			return -1;
		}
		strcpy(key, p);
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
/*
 * sendmozilla: send message to mozilla.
 *
 *	i)	url	URL
 *
 */
#ifdef _WIN32
void
sendmozilla(url)
char	*url;
{
	char	com[1024], *name, *path;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	if (path = usable("mozilla"))
		name = "mozilla";
	else
		die("mozilla not found in your path.");
	
	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWNORMAL;
	sprintf(com, "%s -remote \"openURL(%s)\"", name, url);
	if (!CreateProcess(path, com, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		die("Cannot load mozilla.(error = 0x%04x)\n", GetLastError());
}
#else
void
sendmozilla(url)
char	*url;
{
	int	pid;
	char	com[1024], *name, *path;

	if (path = usable("mozilla"))
		name = "mozilla";
	else
		die("mozilla not found in your path.");
	snprintf(com, sizeof(com), "openURL(%s)", url);
	if ((pid = fork()) < 0) {
		die("cannot load mozilla (fork).");
	} else if (pid == 0) {
		execlp(path, name, "-remote", com, NULL);
		die("cannot load mozilla (execlp).");
	}
}
#endif /* !_WIN32 */
