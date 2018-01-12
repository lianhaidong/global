/*
 * Copyright (c) 2011 Tama Communications Corporation
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
#include "global-cscope.h"
#include "char.h"
#include "gparam.h"
#include "strbuf.h"

#define FAILED "global command failed"

static char *
common(void)
{
	STATIC_STRBUF(sb);
	strbuf_clear(sb);
	strbuf_sprintf(sb, "%s --encode-path=\" \t\" --result=cscope", quote_shell(global_command));
	if (caseless == YES)
		strbuf_puts(sb, " -i");
	if (absolutepath == YES)
		strbuf_puts(sb, " -a");
	return strbuf_value(sb);
}
static int
mysystem(char *function, char *command)
{
#ifdef DEBUG
	FILE *fp = fopen("/tmp/log", "a");
	fprintf(fp, "%s: %s\n", function, command);
	fclose(fp);
#endif
	return system(command);
}
/*
 * [display.c]
 *
 * {"Find this", "C symbol",                       findsymbol},
 */
char *
findsymbol(char *pattern)
{
	STRBUF  *sb = strbuf_open(0);
	int status;
	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -d %s > %s", quote_shell(pattern), temp1);
	status = mysystem("findsymbol_1", strbuf_value(sb));
	if (status != 0) {
		strbuf_close(sb);
		return FAILED;
	}
	strbuf_reset(sb);
	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -rs %s >> %s", quote_shell(pattern), temp1);
	status = mysystem("findsymbol_2", strbuf_value(sb));
	strbuf_close(sb);
	if (status != 0)
		return FAILED;
	return NULL;
}

/*
 * [display.c]
 *
 * {"Find this", "global definition",              finddef},
 */
char *
finddef(char *pattern)
{
	STRBUF  *sb = strbuf_open(0);
	int status;
	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -d %s > %s", quote_shell(pattern), temp1);
	status = mysystem("finddef", strbuf_value(sb));
	strbuf_close(sb);
	if (status != 0)
		return FAILED;
	return NULL;
}

/*
 * [display.c]
 *
 * {"Find", "functions called by this function (N/A)",     findcalledby},
 *
 * This facility is not implemented, because GLOBAL doesn't have such a facility.
 * Instead, this command is replaced with a more useful one, that is, context jump.
 * It is available in the line mode (with the -l option) of gtags-cscope.
 */
char *
findcalledby(char *pattern)
{
	STRBUF  *sb = strbuf_open(0);
	int status;
	char *p;

	/*
	 * <symbol>:<line number>:<path>
	 */
	for (p = pattern; *p && *p != ':'; p++)
		;
	*p++ = '\0';
	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " --from-here=\"%s\" %s > %s", p, quote_shell(pattern), temp1);
	status = mysystem("findcalledby", strbuf_value(sb));
	strbuf_close(sb);
	if (status != 0)
		return FAILED;
	return NULL;
}

/*
 * [display.c]
 *
 * {"Find", "functions calling this function",     findcalling},
 */
char *
findcalling(char *pattern)
{
	STRBUF  *sb = strbuf_open(0);
	int status;

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -r %s > %s", quote_shell(pattern), temp1);
	status = mysystem("findcalling", strbuf_value(sb));
        strbuf_close(sb);
        if (status != 0)
                return FAILED;
        return NULL;
}

/*
 * [display.c]
 *
 * {"Find this", "text string",                    findstring},
 */
char *
findstring(char *pattern)
{
	STRBUF  *sb = strbuf_open(0);
	int status;

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -g --literal %s > %s", quote_shell(pattern), temp1);
	status = mysystem("findstring", strbuf_value(sb));
        strbuf_close(sb);
        if (status != 0)
                return FAILED;
        return NULL;
}

/*
 * [display.c]
 *
 * {"Change this", "text string",                  findstring},
 */
/*
 * [display.c]
 *
        {"Find this", "egrep pattern",                  findregexp},
 */
char *
findregexp(char *pattern)
{
	STRBUF  *sb = strbuf_open(0);
        int status;

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -g %s > %s", quote_shell(pattern), temp1);
	status = mysystem("findregexp", strbuf_value(sb));
        strbuf_close(sb);
        if (status != 0)
                return FAILED;
        return NULL;
}

/*
 * [display.c]
 *
 * {"Find this", "file",                           findfile},
 */
char *
findfile(char *pattern)
{
	STRBUF  *sb = strbuf_open(0);
	int status;

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -P %s > %s", quote_shell(pattern), temp1);
	status = mysystem("findfile", strbuf_value(sb));
        strbuf_close(sb);
        if (status != 0)
                return FAILED;
        return NULL;
}

/*
 * [display.c]
 *
 * {"Find", "files #including this file",          findinclude},
 */
char *
findinclude(char *pattern)
{
	STRBUF  *sb = strbuf_open(0);
	int status;
#if defined(_WIN32) && !defined(__CYGWIN__)
#define INCLUDE "\"^[ \t]*#[ \t]*include[ \t].*[<\\\"/\\]%s[\\\">]\""
#elif defined(__DJGPP__)
#define INCLUDE "'^[ \t]*#[ \t]*include[ \t].*[\"</\\]%s[\">]'"
#else
#define INCLUDE "'^[ \t]*#[ \t]*include[ \t].*[\"</]%s[\">]'"
#endif
	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -g " INCLUDE " | sed \"s/<unknown>/<global>/\" > %s", quote_string(pattern), temp1);
	status = mysystem("findinclude", strbuf_value(sb));
        strbuf_close(sb);
        if (status != 0)
                return FAILED;
        return NULL;
}
/*
 * [display.c]
 *
 * {"Find", "assignments to this symbol (N/A)",    findassign},
 */
char *
findassign(char *pattern)
{
	/* Since this function has not yet been implemented, it always returns an error. */
	return FAILED;
}
