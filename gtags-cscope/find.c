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

#define COMMON (caseless == YES) ? "global --encode-path=\" \t\" --result=cscope -i" : "global --encode-path=\" \t\" --result=cscope"
#define FAILED "global command failed"

static char comline[MAXFILLEN];

/*
 * [display.c]
 *
 * {"Find this", "C symbol",                       findsymbol},
 */
char *
findsymbol(char *pattern)
{
	snprintf(comline, sizeof(comline), "%s -d %s > %s", COMMON, quote_shell(pattern), temp1);
	if (system(comline) != 0)
		return FAILED;
	snprintf(comline, sizeof(comline), "%s -rs %s >> %s", COMMON, quote_shell(pattern), temp1);
	if (system(comline) != 0)
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
	snprintf(comline, sizeof(comline), "%s -d %s > %s", COMMON, quote_shell(pattern), temp1);
	if (system(comline) != 0)
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
	char *p;

	/*
	 * <symbol>:<line number>:<path>
	 */
	for (p = pattern; *p && *p != ':'; p++)
		;
	*p++ = '\0';
	snprintf(comline, sizeof(comline), "%s --from-here=\"%s\" %s > %s", COMMON, p, quote_shell(pattern), temp1);
	if (system(comline) != 0)
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
	snprintf(comline, sizeof(comline), "%s -r %s > %s", COMMON, quote_shell(pattern), temp1);
	if (system(comline) != 0)
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
	snprintf(comline, sizeof(comline), "%s -g --literal %s > %s", COMMON, quote_shell(pattern), temp1);
	if (system(comline) != 0)
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
	snprintf(comline, sizeof(comline), "%s -g %s > %s", COMMON, quote_shell(pattern), temp1);
	if (system(comline) != 0)
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
	snprintf(comline, sizeof(comline), "%s -P %s > %s", COMMON, quote_shell(pattern), temp1);
	if (system(comline) != 0)
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
#if defined(_WIN32) && !defined(__CYGWIN__)
#define INCLUDE "\"^[ \t]*#[ \t]*include[ \t].*[<\\\"/\\]%s[\\\">]\""
#elif defined(__DJGPP__)
#define INCLUDE "'^[ \t]*#[ \t]*include[ \t].*[\"</\\]%s[\">]'"
#else
#define INCLUDE "'^[ \t]*#[ \t]*include[ \t].*[\"</]%s[\">]'"
#endif
	snprintf(comline, sizeof(comline), "%s -g " INCLUDE " | sed \"s/<unknown>/<global>/\" > %s",
		COMMON, quote_string(pattern), temp1);
	if (system(comline) != 0)
		return FAILED;
	return NULL;
}
