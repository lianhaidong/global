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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "asm_parse.h"
#include "asm_res.h"
#include "die.h"
#include "gctags.h"
#include "linetable.h"
#include "strbuf.h"

#define lex_symbol_generation_rule(x) asm_ ## x
#define LEXTEXT lex_symbol_generation_rule(text)
#define LEXLENG lex_symbol_generation_rule(leng)
#define LEXLINENO lex_symbol_generation_rule(lineno)
#define LEXRESTART lex_symbol_generation_rule(restart)
#define LEXLVAL lex_symbol_generation_rule(lval)
#define LEXLLOC lex_symbol_generation_rule(lloc)

#define YY_INPUT(buf, result, max_size) do {				\
	if ((result = linetable_read(buf, max_size)) == -1)		\
		result = YY_NULL;					\
} while (0)

#define ADD_SYM() do {							\
	LEXLVAL = strbuf_getlen(asm_symtable);				\
	LEXLLOC.first_line = LEXLINENO;					\
	strbuf_puts0(asm_symtable, LEXTEXT);				\
} while (0)

extern const char *asm_input_file;
extern STRBUF *asm_symtable;

%}

H		0[Xx][0-9A-Fa-f]+
N		[0-9]+
L		{N}L?
D1		{N}\.{N}([Ee][+-]?{N})?
D2		\.{N}([Ee][+-]?{N})?
NUMBER		-?({L}|{D1}|{D2})
ALPHA		[a-zA-Z_\x80-\xff]
ALPHANUM	[a-zA-Z_\x80-\xff0-9]
WORD		{ALPHA}{ALPHANUM}*

%x C_COMMENT CPP_COMMENT STRING LITERAL
%s PREPROCESSOR_LINE

%option 8bit yylineno stack noyywrap noyy_top_state prefix="asm_"

%%

 /* Ignore spaces */
[ \f\t\v]+

 /* C style comment */
"/*"		{ yy_push_state(C_COMMENT); }
<C_COMMENT>{
	[^*\n]*
	[^*\n]*\n
	"*"+[^*/\n]*
	"*"+[^*/\n]*\n
	"*"+"/"		{ yy_pop_state(); }
	<<EOF>> {
		if (wflag)
			warning("unexpected eof. [+%d %s]", LEXLINENO, asm_input_file);
		yyterminate();
	}
}

 /* C++ style line comment */
"//"		{ yy_push_state(CPP_COMMENT); }
<CPP_COMMENT>{
	(\\.|[^\\\n])+
	\\\n
	\n		{ yy_pop_state(); unput('\n'); }
}

 /* String */
\"		{ yy_push_state(STRING); }
<STRING>{
	(\\.|[^\"\\\n])+
	\\\n
	\n		{ yy_pop_state(); unput('\n'); return ASM_CONST; }
	\"		{ yy_pop_state(); return ASM_CONST; }
}

 /* Character */
\'		{ yy_push_state(LITERAL); }
<LITERAL>{
	(\\.|[^\'\\\n])+
	\\\n
	\n		{ yy_pop_state(); unput('\n'); return ASM_CONST; }
	\'		{ yy_pop_state(); return ASM_CONST; }
}

 /* Number */
{NUMBER}	{ return ASM_CONST; }

<INITIAL>{
	^[ \t]*\#[ \t]*define	{ yy_push_state(PREPROCESSOR_LINE); return ASM_DEFINE; }
	^[ \t]*\#[ \t]*undef	{ yy_push_state(PREPROCESSOR_LINE); return ASM_UNDEF; }
	^[ \t]*\#[ \t]*{WORD} {
		if (asm_reserved_sharp(LEXTEXT, LEXLENG)) {
			yy_push_state(PREPROCESSOR_LINE);
			return ASM_DIRECTIVE;
		} else {
			yy_push_state(CPP_COMMENT);
		}
	}
	^[ \t]*\#		{ yy_push_state(PREPROCESSOR_LINE); return ASM_DIRECTIVE; }
	(call|jsr)	{ return ASM_CALL; }
	^(ENTRY|ALTENTRY|NENTRY|GLOBAL_ENTRY|JSBENTRY|C_SYMBOL_NAME|C_ENTRY) {
		ADD_SYM(); 
		return ASM_ENTRY;
	}
	(EXT|SYMBOL_NAME|C_LABEL) {
		ADD_SYM();
		return ASM_EXT;
	}
}

<PREPROCESSOR_LINE>{
	{WORD}/\(	{ ADD_SYM(); return ASM_SYMBOL_PAREN; }
	\n		{ yy_pop_state(); return '\n'; }
}

{WORD}		{ ADD_SYM(); return ASM_SYMBOL; }

\\\n
\n		{ return '\n'; }
.		{ return LEXTEXT[0]; }

%%

void
asm_initscan(void)
{
	LEXRESTART(NULL);
	LEXLINENO = 1;
}
