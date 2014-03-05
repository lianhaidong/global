/*
 * Copyright (c) 2006, 2013, 2014
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
#include <sys/stat.h>
#include <dirent.h>
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
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif

#include "checkalloc.h"
#include "die.h"
#include "fileop.h"
#include "makepath.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "test.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
#define mkdir(path,mode) mkdir(path)
#endif

/**
 @file

File operation: usage

 @par [WRITE]
 @code
	int compress = cflag ? 1 : 0;

	FILEOP *fileop = open_output_file(path, compress);
	FILE *op = get_descripter(fileop);

	fputs("Hello", op);
	...
	close_file(fileop);
 @endcode

 @par [READ]
 @code
	FILEOP *fileop = open_input_file(path);
	FILE *ip = get_descripter(fileop);

	fgets(buf, sizeof(buf), ip);
	...
	close_file(fileop);
 @endcode
*/
/**
 * open input file.
 *
 *	@param[in]	path	path name
 *	@return		file descripter
 */
FILEOP *
open_input_file(const char *path)
{
	FILE *fp = fopen(path, "r");
	FILEOP *fileop;

	if (fp == NULL)
		die("cannot open file '%s'.", path);
	fileop = check_calloc(sizeof(FILEOP), 1);
	fileop->fp = fp;
	strlimcpy(fileop->path, path, sizeof(fileop->path));
	fileop->type = FILEOP_INPUT;
	return fileop;
}
/**
 * open output file
 *
 *	@param[in]	path	path name
 *	@param[in]	compress 0: normal, 1: compress
 *	@return		file descripter
 *
 *	@note Uses the @NAME{gzip} program to compress, which should already be on your system.
 */
FILEOP *
open_output_file(const char *path, int compress)
{
	FILEOP *fileop;
	FILE *fp;
	char command[MAXFILLEN];

	if (compress) {
		snprintf(command, sizeof(command), "gzip -c >\"%s\"", path);
		fp = popen(command, "w");
		if (fp == NULL)
			die("cannot execute '%s'.", command);
	} else {
		fp = fopen(path, "w");
		if (fp == NULL)
			die("cannot create file '%s'.", path);
	}
	fileop = check_calloc(sizeof(FILEOP), 1);
	strlimcpy(fileop->path, path, sizeof(fileop->path));
	if (compress)
		strlimcpy(fileop->command, command, sizeof(fileop->command));
	fileop->type = FILEOP_OUTPUT;
	if (compress)
		fileop->type |= FILEOP_COMPRESS;
	fileop->fp = fp;
	return fileop;
}
/**
 * get UNIX file descripter
 *
 * ( See open_input_file() and open_output_file() ).
 */
FILE *
get_descripter(FILEOP *fileop)
{
	return fileop->fp;
}
/**
 * close_file: close file
 *
 *	@param[in]	fileop	file descripter
 */
void
close_file(FILEOP *fileop)
{
	if (fileop->type & FILEOP_COMPRESS) {
		if (pclose(fileop->fp) != 0)
			die("terminated abnormally. '%s'", fileop->command);
	} else
		fclose(fileop->fp);
	free(fileop);
}
/**
 * copy file
 *
 *	@param[in]	src	source file
 *	@param[in]	dist	distination file
 */
/*
void
copyfile(const char *src, const char *dist)
{
	char buf[MAXBUFLEN];

	FILEOP *input_file = open_input_file(src);
	FILEOP *output_file = open_output_file(dist, 0);
	while (fgets(buf, sizeof(buf), get_descripter(input_file)) != NULL)
		fputs(buf, get_descripter(output_file));
	close_file(input_file);
	close_file(output_file);
}
*/
/**
 * copy file.
 *
 *	@param[in]	src	source file
 *	@param[in]	dist	distination file
 */
void
copyfile(const char *src, const char *dist)
{
	int ip, op, size;
	char buf[8192];

#ifndef O_BINARY
#define O_BINARY 0
#endif
	ip = open(src, O_RDONLY|O_BINARY);
	if (ip < 0)
		die("cannot open input file '%s'.", src);
	op = open(dist, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0775);
	if (op < 0)
		die("cannot create output file '%s'.", dist);
	while ((size = read(ip, buf, sizeof(buf))) != 0) {
		if (size < 0)
			die("file read error.");
		if (write(op, buf, size) != size)
			die("file write error.");
	}
	close(op);
	close(ip);
}
/**
 * copy directory
 *
 *	@param[in]	srcdir	source directory
 *	@param[in]	distdir	distination directory
 *
 * This function cannot treat nested directory.
 */
void
copydirectory(const char *srcdir, const char *distdir)
{
	DIR *dirp;
	struct dirent *dp;
	struct stat st;

	if (!test("d", srcdir))
		die("directory '%s' not found.", srcdir);
	if (!test("d", distdir))
		if (mkdir(distdir, 0775) < 0)
			die("cannot make directory '%s'.", distdir);
	if ((dirp = opendir(srcdir)) == NULL)
		die("cannot read directory '%s'.", srcdir);
	while ((dp = readdir(dirp)) != NULL) {
		const char *path = makepath(srcdir, dp->d_name, NULL);

		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (stat(path, &st) < 0)
			die("cannot stat file '%s'.", path);
		if (S_ISREG(st.st_mode)) {
			char src[MAXPATHLEN];
			char dist[MAXPATHLEN];

			strlimcpy(src, path, sizeof(src));
			strlimcpy(dist, makepath(distdir, dp->d_name, NULL), sizeof(dist));
			copyfile(src, dist);
		}
	}
	(void)closedir(dirp);
}
/**
 * read the first line of command's output
 *
 *	@param[in]	com	command line
 *	@param[in]	sb	string buffer
 *	@return			0: normal, -1: error
 */
int
read_first_line(const char *com, STRBUF *sb)
{
	FILE *ip = popen(com, "r");
	char *p;

	if (ip == NULL)
		return -1;
	p = strbuf_fgets(sb, ip, STRBUF_NOCRLF);
	pclose(ip);
	return (p == NULL) ? -1 : 0;
}
