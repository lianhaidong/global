%{
/*
 * Copyright (c) 2003 Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * GNU GLOBAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * GNU GLOBAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * scanner for PHP source code.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "defined.h"
#include "die.h"
#include "gparam.h"
#include "gctags.h"
#include "linetable.h"
#include "strbuf.h"
#include "php_res.h"

#define PHP_TOKEN		1
#define PHP_VARIABLE		2
#define PHP_STRING		3
#define PHP_POINTER		4
#define PHP_DOLLAR		5
#define PHP_LPAREN		'('
#define PHP_RPAREN		')'
#define PHP_LBRACE		'{'
#define PHP_RBRACE		'}'
#define PHP_LBRACK		'['
#define PHP_RBRACK		']'

static void debug_print(const char *, ...);

static int level;			/* block nest level */
static STRBUF *string;			/* string */
extern int vflag;
extern int wflag;
extern int debug;

/*
 * For debug.
 */
static void
debug_print(const char *s, ...)
{
	va_list ap;

	if (!debug)
		return;
	va_start(ap, s);
	(void)vfprintf(stderr, s, ap);
	va_end(ap);
}
#ifdef YYLMAX
#undef YYLMAX
#endif
#define YYLMAX 1024
#ifdef ECHO
#undef ECHO
#endif
#define ECHO debug_print("%s", phptext)
#ifdef PUT
#undef PUT
#endif
#define PUT(tag, lno, file) do {					\
	if (!nflag) {							\
		fprintf(stdout, "%-16s %4d %-16s ",tag, lno, file);	\
		linetable_print(stdout, lno);				\
	}								\
} while (0)

/*
 * IO routine.
 */
#define YY_INPUT(buf,result,max_size) \
	do { \
		if ((result = linetable_read(buf, max_size)) == -1) \
			result = YY_NULL; \
	} while (0)
%}
 /* Definitions */
H		0[Xx][0-9A-Fa-f]+
N		[0-9]+
L		{N}L?
D1		{N}\.{N}([Ee][+-]?{N})?
D2		\.{N}([Ee][+-]?{N})?
NUMBER		-?({L}|{D1}|{D2})
 /* We accept multi-bytes character */
ALPHA		[a-zA-Z_\x80-\xff]
ALPHANUM	[a-zA-Z_\x80-\xff0-9]
WORD		{ALPHA}{ALPHANUM}*
%start	PHP STRING LITERAL
%option 8bit caseless noyywrap yylineno prefix="php"
%%
\n			ECHO;
 /* Start PHP */
<INITIAL>"<?="		ECHO; BEGIN PHP;
<INITIAL>"<?"		ECHO; BEGIN PHP;
<INITIAL>"<?php"	ECHO; BEGIN PHP;
<INITIAL>"<%"		ECHO; BEGIN PHP;
<INITIAL>"<script[ \t]+language=(\")?php(\")?>"	ECHO; BEGIN PHP;
 /* Ignore HTML */
<INITIAL>.		ECHO;
 /* End of PHP */
<PHP>"?>"		ECHO; BEGIN INITIAL;
<PHP>"%>"		ECHO; BEGIN INITIAL;
<PHP>"</script>"	ECHO; BEGIN INITIAL;

 /* Comment */
<PHP>"/*"		{
				int c;

				debug_print("</*");
				while ((c = input()) != EOF) {
					debug_print("%c", c);
					if (c == '*') {
						while ((c = input()) != EOF && c == '*')
							debug_print("%c", c);
						debug_print("%c", c);
						if (c == EOF || c == '/')
							break;
					}
				}
				if (c == EOF)
					die("unexpected end of comment.");
				debug_print(">");
			}
<PHP>"//".*		debug_print("<%s>", phptext);
<PHP>"#".*		debug_print("<%s>", phptext);

 /* String */
<PHP>\"			{ strbuf_reset(string); BEGIN STRING; }
<STRING>\"		{
				debug_print("<S:%s>", strbuf_value(string));
				BEGIN PHP;
					
				return PHP_STRING;
			}
<STRING>\\.		strbuf_puts(string, phptext);
<STRING>.		strbuf_putc(string, phptext[0]);

 /* Literal */
<PHP>\'			{ strbuf_reset(string); BEGIN LITERAL; }
<LITERAL>\'		{
				debug_print("<L:%s>", strbuf_value(string));
				BEGIN PHP;
					
				return PHP_STRING;
			}
<LITERAL>\\.		strbuf_puts(string, phptext);
<LITERAL>.		strbuf_putc(string, phptext[0]);

 /* Cast */
<PHP>\([ \t]*(bool|boolean|int|integer|real|double|float|string|array|object)[ \t]*\)
			;
<PHP,STRING>$\{{WORD}\}	{
				/*
				 * 0123456	yyleng = 6
				 * ${abc}\0
				 */
				if (YY_START == STRING)
					strbuf_puts(string, phptext);
				memcpy(phptext, &phptext[2], phpleng - 3);
				phptext[phpleng - 3] = '\0';
				phpleng = phpleng - 3;
				debug_print("<V:%s>", phptext);

				return PHP_VARIABLE;
			}
<PHP,STRING>${WORD}	{
				/*
				 * 01234	yyleng = 4
				 * $abc\0
				 */
				if (YY_START == STRING)
					strbuf_puts(string, phptext);
				memcpy(phptext, &phptext[1], phpleng - 1);
				phptext[phpleng - 1] = '\0';
				phpleng = phpleng - 1;
				debug_print("<V:%s>", phptext);

				return PHP_VARIABLE;
			}
<PHP>{NUMBER}		debug_print("<N:%s>", phptext);
<PHP>{WORD}		{
				int id = reserved_word(phptext, phpleng);
				if (id) {
					debug_print("<Reserved:%s>", phptext);
					return id;
				} else {
					debug_print("<T:%s>", phptext);
					return PHP_TOKEN;
				}
			}
 /* Operator */
<PHP>[{}]		{
				int c = phptext[0];
				if (c == PHP_LBRACE)
					level++;
				else
					level--;
				debug_print("%c[%d]", c, level);

				return c;
			}
<PHP>[][()]		{
				return phptext[0];
			}
<PHP>[-+*/%&~^]=?	ECHO;
<PHP>[=><!]=		ECHO;
<PHP>[-+&|<>]{2}=?	ECHO;
<PHP>"<>"|"<<<"		ECHO;
<PHP>"$"		{ ECHO; return PHP_DOLLAR; }
<PHP>.			ECHO;

%%
/*
 * php: read PHP file and pickup tag entries.
 */
void
php(file)
const char *file;
{
	int token;
	int target = (sflag) ? SYM : (rflag) ? REF : DEF;

	string = strbuf_open(0);
	if (linetable_open(file) == -1)
		die("'%s' cannot open.", file);

	yyrestart(NULL);
	while ((token = phplex()) != 0) {
		switch (token) {
		case PHP_DEFINE:
			if (phplex() != PHP_LPAREN)
				break;
			if (phplex() != PHP_STRING)
				break;
			if (target == DEF)
				PUT(strbuf_value(string), phplineno, file);
			break;
		case PHP_CLASS:
			if (phplex() != PHP_TOKEN)
				break;
			if (target == DEF)
				PUT(phptext, phplineno, file);
			break;
		case PHP_FUNCTION:
		case PHP_CFUNCTION:
		case PHP_OLD_FUNCTION:
			if (phplex() != PHP_TOKEN)
				break;
			if (target == DEF)
				PUT(phptext, phplineno, file);
			break;
		case PHP_VARIABLE:
			if (reserved_variable(phptext, phpleng)) {
				if (target == SYM)
					PUT(phptext, phplineno, file);
				if (phplex() == PHP_LBRACK && phplex() == PHP_STRING && phplex() == PHP_RBRACK) {
					char *str = strbuf_value(string);

					if (strchr(str, '$') == 0)
						if (target == SYM)
							PUT(str, phplineno, file);
				}
			} else if (!strcmp(phptext, "this")) {
				;
			} else {
				if (target == SYM)
					PUT(phptext, phplineno, file);
			}
			break;
		case PHP_POINTER:
			if (phplex() != PHP_TOKEN)
				break;
			/* FALL THROUTH */
		case PHP_TOKEN:
			if (target == REF) {
				if (defined(phptext))
					PUT(phptext, phplineno, file);
			} else if (target == SYM) {
				if (!defined(phptext))
					PUT(phptext, phplineno, file);
			}
			break;
		case PHP_NEW:
			if (phplex() != PHP_TOKEN)
				break;
			if (target == REF)
				PUT(phptext, phplineno, file);
			break;
		/*
		 * ${x->y}
		 */
		case PHP_DOLLAR:
			if (phplex() != PHP_LBRACE)
				break;
			while ((token = phplex()) != PHP_RBRACE) {
				if (token == PHP_TOKEN) {
					if (target == REF) {
						if (defined(phptext))
							PUT(phptext, phplineno, file);
					} else if (target == SYM) {
						if (!defined(phptext))
							PUT(phptext, phplineno, file);
					}
				}
			}
			break;
		default:
			break;
		}
	}
	linetable_close();
	strbuf_close(string);
}
