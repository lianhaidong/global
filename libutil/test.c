/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2006, 2010
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

#include "char.h"
#include "die.h"
#include "locatestring.h"
#include "strbuf.h"
#include "test.h"

/**
 * Decide whether or not the @a path is binary file.
 *
 *	@param[in]	path	path to test, <br>
 *			if @VAR{NULL} then use the saved previous path, calls die() if is none.
 *	@return	0: is not binary, 1: is binary
 */
static int
is_binary(const char *path)
{
	int ip;
	char buf[512];
	int i, size;

	ip = open(path, O_RDONLY);
	if (ip < 0)
		die("cannot open file '%s' in read mode.", path);
	size = read(ip, buf, sizeof(buf));
	close(ip);
	if (size < 0)
		return 1;
	if (size >= 7 && locatestring(buf, "!<arch>", MATCH_AT_FIRST))	/* ar */
		return 1;
	if (size >= 4 && locatestring(buf, "%PDF", MATCH_AT_FIRST))	/* PDF */
		return 1;
	for (i = 0; i < size; i++) {
		if (isbinarychar(buf[i]))
			return 1;
	}
	return 0;
}
/**
 * test: 
 *
 *	@param[in]	flags	file flags <br>
 *			@CODE{"f"}	[ -f path ] is @a path a regular file (not a dir, link or pipe etc.)? <br>
 *			@CODE{"d"}	[ -d path ] is @a path a dir? <br>
 *			@CODE{"r"}	[ -r path ] can we read from @a path? <br>
 *			@CODE{"s"}	[ -s path ] has @a path got a non-zero size? <br>
 *			@CODE{"w"}	[ -w path ] can we write to @a path? <br>
 *			@CODE{"x"}	[ -x path ] is @a path an executable file/program? <br>
 *			@CODE{"b"}	[ -b path ] is @a path a binary file?
 *
 *	@param[in]	path	path <br>
 *			if @VAR{NULL} then previous path.
 *	@return		0: no, 1: ok
 *
 * You can specify more than one character. It assumed 'AND' test.
 */
int
test(const char *flags, const char *path)
{
	static struct stat sb;
	STATIC_STRBUF(saved_path);
	int c;

	if (path != NULL) {
		if (stat(path, &sb) < 0)
			return 0;
		strbuf_clear(saved_path);
		strbuf_puts(saved_path, path);
	} else {
		if (strbuf_empty(saved_path))
			die("no saved previous path [test()]");
		path = (const char *) strbuf_value(saved_path);
	}
	while ((c = *flags++) != 0) {
		switch (c) {
		case 'b':
	 		if (!is_binary(path))
				return 0;
			break;
		case 'f':
	 		if (!S_ISREG(sb.st_mode))
				return 0;
			break;
		case 'd':
	 		if (!S_ISDIR(sb.st_mode))
				return 0;
			break;
		case 'r':
			if (access(path, R_OK) < 0)
				return 0;
			break;
		case 's':
			if (sb.st_size == 0)
				return 0;
			break;
		case 'w':
			if (access(path, W_OK) < 0)
				return 0;
			break;
		case 'x':
#ifdef _WIN32
			/* Look at file extension to determine executability */
			if (strlen(path) < 5)
				return 0;
			if (!S_ISREG(sb.st_mode))
				return 0;
			if (!locatestring(path, ".exe", MATCH_AT_LAST|IGNORE_CASE) &&
				!locatestring(path, ".com", MATCH_AT_LAST|IGNORE_CASE) &&
				!locatestring(path, ".bat", MATCH_AT_LAST|IGNORE_CASE))
				return 0;
#else
			if (access(path, X_OK) < 0)
				return 0;
#endif
			break;
		default:
			break;
		}
	}
	return 1;
}
/**
 * filesize: get file size in bytes.
 *
 *	@param[in]	path	path of file
 *	@return		!= -1: file size
 *			== -1: file not found
 */
int
filesize(const char *path)
{
	struct stat sb;

	if (stat(path, &sb) < 0)
		return -1;
	return sb.st_size;
}
