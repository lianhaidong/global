/*
 * Copyright (c) 2014
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

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <stdio.h>

#include "checkalloc.h"
#include "rewrite.h"

/** @file
Rewrite: sed style rewriting module.

usage:

REWRITE *rewrite = rewrite_open("xyz", "<&>", 0);

rewrite_string(rewrite, "xyzABCxzy", 0);

	=> <xyz>ABCxzy

rewrite_pattern(rewrite, "abc", REG_ICASE);

rewrite_string(rewrite, "xxxABCyyy", 0);

	=> xxx<ABC>yyy

rewrite_close(rewrite);
 */
/**
 * rewrite_open: open rewrite object
 *
 *	@param[in]	pattern
 *			accepts NULL
 *	@param[in]	replace (allows '&')
 *	@param[in]	flags for regcomp(3)
 *	@return	rewrite	#REWRITE structure
 *			NULL: illegal regular expression
 */
REWRITE *
rewrite_open(const char *pattern, const char *replace, int flags)
{
	REWRITE *rewrite = (REWRITE *)check_calloc(sizeof(REWRITE), 1);
	char *p;

	if (pattern) {
		if (regcomp(&rewrite->reg, pattern, flags) != 0) {
			free(rewrite);
			return NULL;
		}
		rewrite->pattern = check_strdup(pattern);
	}
	rewrite->replace = check_strdup(replace);
	if ((p = strchr(rewrite->replace, '&')) != NULL) {
		if (p > rewrite->replace) {
			*p = '\0';
			rewrite->part[REWRITE_LEFT] = rewrite->replace;
			rewrite->len[REWRITE_LEFT] = strlen(rewrite->replace);
		}
		if (*++p != 0) {
			rewrite->part[REWRITE_RIGHT] = p;
			rewrite->len[REWRITE_RIGHT] = strlen(p);
		}
	} else {
		rewrite->part[REWRITE_CENTER]  = rewrite->replace;
		rewrite->len[REWRITE_CENTER]  = strlen(rewrite->replace);
	}
	return rewrite;
}
/**
 * rewrite_pattern: set pattern to the rewrite object
 *
 *	@param[in]	rewrite object
 *	@param[in]	pattern
 *	@param[in]	flags for regcomp(3)
 *	@return		0: normal
 *			-1: illegal regular expression
 */
int
rewrite_pattern(REWRITE *rewrite, const char *pattern, int flags)
{
	int status = 0;

	if (rewrite->pattern) {
		free(rewrite->pattern);
		regfree(&rewrite->reg);
	}
	status = regcomp(&rewrite->reg, pattern, flags);
	rewrite->pattern = (status == 0) ? check_strdup(pattern) : NULL;
	return status;
}
/**
 * rewrite_string: execute rewrite against string
 *
 *	@param[in]	rewrite object
 *			NULL: just print string
 *	@param[in]	string
 *	@param[in]	offset start point of the rewriting
 *	@param[in]	file descriptor
 */
const char *
rewrite_string(REWRITE *rewrite, const char *string, int offset)
{
	STATIC_STRBUF(sb);
	regmatch_t m;

	/* if rewrite object is NULL or does not match, just return the string. */
	if (rewrite == NULL || rewrite->pattern == NULL)
		return string;
	if (regexec(&rewrite->reg, string + offset, 1, &m, 0) != 0)
		return string;
        strbuf_clear(sb);
	strbuf_nputs(sb, string, offset);
	string += offset;
	strbuf_nputs(sb, string, m.rm_so);
	if (rewrite->part[REWRITE_CENTER]) {
		strbuf_puts(sb, rewrite->part[REWRITE_CENTER]);
	} else {
		if (rewrite->part[REWRITE_LEFT])
			strbuf_puts(sb, rewrite->part[REWRITE_LEFT]);
		strbuf_nputs(sb, string + m.rm_so, m.rm_eo - m.rm_so);
		if (rewrite->part[REWRITE_RIGHT])
			strbuf_puts(sb, rewrite->part[REWRITE_RIGHT]);
	}
	strbuf_puts(sb, string + m.rm_eo);
	return (const char *)strbuf_value(sb);
}
/**
 * rewrite_cancel: cancel rewriting.
 *
 *	@param[in]	rewrite object
 */
void
rewrite_cancel(REWRITE *rewrite)
{
	if (rewrite->pattern) {
		free(rewrite->pattern);
		rewrite->pattern = NULL;
	}
}
/**
 * rewrite_dump: dump rewrite object
 *
 *	@param[in]	rewrite object
 */
void
rewrite_dump(REWRITE *rewrite)
{
	int i;
	if (rewrite->pattern)
		fprintf(stdout, "pattern: %s\n", rewrite->pattern);
	for (i = 0; i < REWRITE_PARTS; i++) {
		if (rewrite->part[i])
			fprintf(stdout, "%d: %s\n", i, rewrite->part[i]);
	}
}
/**
 * rewrite_close: free rewrite object.
 *
 *	@param[in]	rewrite object
 */
void
rewrite_close(REWRITE *rewrite)
{
	free(rewrite->replace);
	free(rewrite);	
}
