%{
/*
 * Copyright (c) 2002, 2004 Tama Communications Corporation
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
 * scanner for C++ source code.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#include "global.h"
#include "anchor.h"
#include "incop.h"
#include "path2url.h"
#include "common.h"
#include "htags.h"
#include "../gctags/cpp_res.h"
#include "lexcommon.h"

#define LEXTEXT	cpptext
#define LEXLENG	cppleng

#ifdef ECHO
#undef ECHO
#endif
#define ECHO	echos(LEXTEXT)

#define YY_USER_ACTION DEFAULT_YY_USER_ACTION

static int last_directive;
%}
 /* Definitions */
H		0[Xx][0-9A-Fa-f]+
N		[0-9]+
L		{N}L?
D1		{N}\.{N}([Ee][+-]?{N})?
D2		\.{N}([Ee][+-]?{N})?
NUMBER		-?({L}|{D1}|{D2})
ALPHA		[a-zA-Z_\x80-\xff]
ALPHANUM	[a-zA-Z_\x80-\xff0-9]
WORD		{ALPHA}{ALPHANUM}*

%start	CPP C_COMMENT CPP_COMMENT SHELL_COMMENT STRING LITERAL PREPROCESSING_DIRECTIVE
%option 8bit noyywrap noyy_top_state stack prefix="cpp"
%%
 /* Backslash-newline */
\\\n		DEFAULT_BACKSLASH_NEWLINE_ACTION

 /* Comment */
<CPP,PREPROCESSING_DIRECTIVE>"/*"	{ echos(comment_begin); ECHO; yy_push_state(C_COMMENT); }
<C_COMMENT>"*/"	{ ECHO; echos(comment_end); yy_pop_state(); }
<C_COMMENT>.	{ put_char(LEXTEXT[0]); }
<CPP,PREPROCESSING_DIRECTIVE>"//"	{ echos(comment_begin); ECHO; yy_push_state(CPP_COMMENT); }

 /* String */
<CPP,PREPROCESSING_DIRECTIVE>\"	{ ECHO; yy_push_state(STRING); }
<STRING>\"	{ ECHO; yy_pop_state(); }
<STRING>\\.	{ put_char(LEXTEXT[0]); put_char(LEXTEXT[1]); }

 /* Literal */
<CPP,PREPROCESSING_DIRECTIVE>\'	{ ECHO; yy_push_state(LITERAL); }
<LITERAL>\'	{ ECHO; yy_pop_state(); }
<LITERAL>\\.	{ put_char(LEXTEXT[0]); put_char(LEXTEXT[1]); }

 /* Preprocessing directive */
<CPP>^[ \t]*\#[ \t]*(include|include_next) {
		int c;

		put_macro(LEXTEXT);
		/*
		 * #include|   <aaa/bbb.h>|
		 *         ~~~~~~~~~~~~~~~~
		 */
		while ((c = input()) && c != '\n' && isspace(c))
			echoc(c);
		if (c == '\n') {
			unput(c);
		} else if (c) {
			char path[MAXPATHLEN+1], *p = path;
			int sep = 0;

			if (c == '"')
				sep = c;
			else if (c == '<')
				sep = '>';
			put_char(c);

			/* pick up path name */
			while ((c = input()) && c != sep && c != '\n')
				*p++ = c;
			*p = '\0';
			if (c == sep) {
				struct data *inc;
				char *basename = locatestring(path, "/", MATCH_LAST);

				if (basename)
					basename++;
				else
					basename = path;
				inc = get_inc(basename);
				if (inc)
					put_include_anchor(inc, path);
				else
					echos(path);
				put_char(sep);
			} else {
				echos(path);
				if (c)
					unput(c);
			}
		}
	}
<CPP>^[ \t]*\#[ \t]*{WORD} {
		int i = 0;
		char *p;

		p = locatestring(LEXTEXT, " ", MATCH_LAST);

		if (!p) {
			p = LEXTEXT;
			i = LEXLENG;
		} else {
			static char buf[IDENTLEN];

			p++;
			if (*p == '#')
				p++;
			buf[i++] = '#';
			while (*p) {
				buf[i++] = *p++;
				if (i >= sizeof(buf))
					die("Too long name '%s'.", LEXTEXT);
			}
			buf[i] = '\0';
			p = buf;
		}
		if (i > 0 && (last_directive = reserved_sharp(p, i)))
			put_macro(LEXTEXT);
		else {
			ECHO;
			if (wflag)
				unknown_preprocessing_directive(p, LINENO);
		}
		yy_push_state(PREPROCESSING_DIRECTIVE);
	}
 /* Null directive */
<CPP>^[ \t]*\#	{ put_macro(LEXTEXT); }

<CPP,PREPROCESSING_DIRECTIVE>{NUMBER}	ECHO;
<CPP,PREPROCESSING_DIRECTIVE>{WORD} {
		if (reserved_word(LEXTEXT, LEXLENG))
			put_reserved_word(LEXTEXT);
		else if (YY_START == PREPROCESSING_DIRECTIVE
		    && (last_directive == SHARP_IF || last_directive == SHARP_ELIF)
		    && strcmp(LEXTEXT, "defined") == 0)
			put_reserved_word(LEXTEXT);
		else {
			struct anchor *a = anchor_get(LEXTEXT, LEXLENG, 0, LINENO);
			if (a) {
				put_anchor(gettag(a), a->type, LINENO);
				a->done = 1;
			} else {
				ECHO;
			}
		}
	}
<CPP,PREPROCESSING_DIRECTIVE>[{}]	{ put_brace(LEXTEXT); }
 /* New line */
\n		DEFAULT_END_OF_LINE_ACTION
.		{ put_char(LEXTEXT[0]); }

%%
void
cpp_parser_init(ip)
	FILE *ip;
{
	DEFAULT_BEGIN_OF_FILE_ACTION
	BEGIN CPP;
}
