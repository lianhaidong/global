/*
 * Copyright (c) 2011, 2018 Tama Communications Corporation
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
#include "regex.h"

#define FAILED "global command failed"

#if defined(_WIN32) || defined(__DJGPP__)
/*
 * for Windows
 */
static char *
common(void)
{
	STATIC_STRBUF(sb);
	strbuf_clear(sb);
#if defined(_WIN32) && !defined(__CYGWIN__)
	/*
	 * Get around CMD.EXE's weird quoting rules by sticking another
	 * perceived whitespace in front (also works with Take Command).
	 */
	strbuf_putc(sb, ';');
#endif
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
	int status;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -d %s > %s", quote_shell(pattern), temp1);
	status = mysystem("findsymbol_1", strbuf_value(sb));
	if (status != 0)
		return FAILED;
	strbuf_reset(sb);
	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -rs %s >> %s", quote_shell(pattern), temp1);
	status = mysystem("findsymbol_2", strbuf_value(sb));
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
	int status;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -d %s > %s", quote_shell(pattern), temp1);
	status = mysystem("finddef", strbuf_value(sb));
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
	int status;
	char *p;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	/*
	 * <symbol>:<line number>:<path>
	 */
	for (p = pattern; *p && *p != ':'; p++)
		;
	*p++ = '\0';
	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " --from-here=\"%s\" %s > %s", p, quote_shell(pattern), temp1);
	status = mysystem("findcalledby", strbuf_value(sb));
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
	int status;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -r %s > %s", quote_shell(pattern), temp1);
	status = mysystem("findcalling", strbuf_value(sb));
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
	int status;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -g --literal %s > %s", quote_shell(pattern), temp1);
	status = mysystem("findstring", strbuf_value(sb));
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
        int status;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -g %s > %s", quote_shell(pattern), temp1);
	status = mysystem("findregexp", strbuf_value(sb));
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
	int status;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -P %s > %s", quote_shell(pattern), temp1);
	status = mysystem("findfile", strbuf_value(sb));
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
	int status;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);
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
        if (status != 0)
                return FAILED;
        return NULL;
}
/*
 * [display.c]
 *
 * {"Find", "assignments to this symbol",    findassign},
 */
char *
findassign(char *pattern)
{
	int status;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -d %s | sed -n /\\b%s\\b\"[ \t]*=[^=]\"/p > %s", quote_shell(pattern), quote_shell(pattern), temp1);
	status = mysystem("findassign_1", strbuf_value(sb));
	if (status != 0)
		return FAILED;
	strbuf_reset(sb);
	strbuf_puts(sb, common());
	strbuf_sprintf(sb, " -rs %s | sed -n /\\b%s\\b\"[ \t]*=[^=]\"/p >> %s", quote_shell(pattern), quote_shell(pattern), temp1);
	status = mysystem("findassign_2", strbuf_value(sb));
	if (status != 0)
		return FAILED;
	return NULL;
}
#else /* UNIX */
/*
 * for UNIX
 */
#include "rewrite.h"
#include "secure_popen.h"

static void
common(void)
{
	secure_add_args(global_command);
	secure_add_args("--encode-path= \t");
	secure_add_args("--result=cscope");
	if (caseless == YES)
		secure_add_args("-i");
	if (absolutepath == YES)
		secure_add_args("-a");
}
void
writeto(FILE *ip, char *outfile, int append) {
	FILE *op = fopen(outfile, append ? "a" : "w");
	const char *line;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	if (op == NULL)
		return;
	while ((line = strbuf_fgets(sb, ip, 0)) != NULL) {
		fputs(line, op);
	}
	fclose(op);
}
/*
 * [display.c]
 *
 * {"Find this", "C symbol",                       findsymbol},
 */
char *
findsymbol(char *pattern)
{
	char **argv;
	FILE *ip;

	secure_open_args();
	common();
	secure_add_args("-d");
	secure_add_args(pattern);
	argv = secure_close_args();
	if (!(ip = secure_popen(global_command, "r", argv)))
		return FAILED;
	writeto(ip, temp1, 0);
	if (secure_pclose(ip) != 0)
		return FAILED;

	secure_open_args();
	common();
	secure_add_args("-rs");
	secure_add_args(pattern);
	argv = secure_close_args();
	if (!(ip = secure_popen(global_command, "r", argv)))
		return FAILED;
	writeto(ip, temp1, 1);
	if (secure_pclose(ip) != 0)
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
	char **argv;
	FILE *ip;

	secure_open_args();
	common();
	secure_add_args("-d");
	secure_add_args(pattern);
	argv = secure_close_args();
	if (!(ip = secure_popen(global_command, "r", argv)))
		return FAILED;
	writeto(ip, temp1, 0);
	if (secure_pclose(ip) != 0)
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
	char **argv;
	FILE *ip;
	char *p;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	/*
	 * <symbol>:<line number>:<path>
	 */
	for (p = pattern; *p && *p != ':'; p++)
		;
	*p++ = '\0';
	strbuf_puts(sb, "--from-here=");
	strbuf_puts(sb, p);

	secure_open_args();
	common();
	secure_add_args(strbuf_value(sb));
	secure_add_args(pattern);
	argv = secure_close_args();
	if (!(ip = secure_popen(global_command, "r", argv)))
		return FAILED;
	writeto(ip, temp1, 0);
	if (secure_pclose(ip) != 0)
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
	char **argv;
	FILE *ip;

	secure_open_args();
	common();
	secure_add_args("-r");
	secure_add_args(pattern);
	argv = secure_close_args();
	if (!(ip = secure_popen(global_command, "r", argv)))
		return FAILED;
	writeto(ip, temp1, 0);
	if (secure_pclose(ip) != 0)
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
	char **argv;
	FILE *ip;

	secure_open_args();
	common();
	secure_add_args("-g");
	secure_add_args("--literal");
	secure_add_args(pattern);
	argv = secure_close_args();
	if (!(ip = secure_popen(global_command, "r", argv)))
		return FAILED;
	writeto(ip, temp1, 0);
	if (secure_pclose(ip) != 0)
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
	char **argv;
	FILE *ip;

	secure_open_args();
	common();
	secure_add_args("-g");
	secure_add_args(pattern);
	argv = secure_close_args();
	if (!(ip = secure_popen(global_command, "r", argv)))
		return FAILED;
	writeto(ip, temp1, 0);
	if (secure_pclose(ip) != 0)
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
	char **argv;
	FILE *ip;

	secure_open_args();
	common();
	secure_add_args("-P");
	secure_add_args(pattern);
	argv = secure_close_args();
	if (!(ip = secure_popen(global_command, "r", argv)))
		return FAILED;
	writeto(ip, temp1, 0);
	if (secure_pclose(ip) != 0)
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
	char **argv;
	FILE *ip, *op;
	REWRITE *rw;
	const char *line;
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	secure_open_args();
	common();
	secure_add_args("-g");
	strbuf_puts(sb, "^[ \t]*#[ \t]*include[ \t].*[\"</]");
	strbuf_puts(sb, quote_string(pattern));
	strbuf_puts(sb, "[\">]");
	secure_add_args(strbuf_value(sb));
	argv = secure_close_args();
	if (!(ip = secure_popen(global_command, "r", argv)))
		return FAILED;
	op = fopen(temp1, "w");
	if (op == NULL)
		return FAILED;
	rw = rewrite_open("<unknown>", "<global>", 0);
	while ((line = strbuf_fgets(sb, ip, 0)) != NULL) {
		line = rewrite_string(rw, line, 0);
		fputs(line, op);
	}
	rewrite_close(rw);
	fclose(op);
	if (secure_pclose(ip) != 0)
		return FAILED;
	return NULL;
}
/*
 * [display.c]
 *
 * {"Find", "assignments to this symbol",    findassign},
 */
char *
findassign(char *pattern)
{
	regex_t reg;
	const char *line;
	char **argv;
	FILE *ip, *op;
	int i;
	char *opts[] = {"-d", "-rs", NULL};
	STATIC_STRBUF(sb);
	strbuf_clear(sb);

	/*
	 * For the time being I will support only C-colleagues.
	 * Lisp, Cobol and etc are out of support.
	 */
	strbuf_sprintf(sb, "\\b%s\\b[ \t]*=[^=]", pattern);
	if (regcomp(&reg, strbuf_value(sb), 0) != 0)
		return FAILED;

	op = fopen(temp1, "w");
	if (op == NULL)
		return FAILED;
	for (i = 0; opts[i] != NULL; i++) {
		secure_open_args();
		common();
		secure_add_args(opts[i]);
		secure_add_args(pattern);
		argv = secure_close_args();
		if (!(ip = secure_popen(global_command, "r", argv)))
			return FAILED;
		while ((line = strbuf_fgets(sb, ip, 0)) != NULL) {
			if (regexec(&reg, line, 0, 0, 0) == 0)
				fputs(line, op);
		}
		if (secure_pclose(ip) != 0)
			return FAILED;
	}
	fclose(op);
	return NULL;
}
#endif
