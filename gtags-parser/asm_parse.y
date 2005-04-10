%{
/*
 * Copyright (c) 2004 Tama Communications Corporation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <ctype.h>
#include <stdio.h>

#include "defined.h"
#include "die.h"
#include "gctags.h"
#include "linetable.h"
#include "strbuf.h"

#undef PUT
#define PUT(tag, lno) do {						\
	if (!nflag) {							\
		printf("%-16s %4d %-16s ", tag, lno, asm_input_file);	\
		linetable_print(stdout, lno);				\
	}								\
} while (0)

#define GET_SYM(offset) ((offset) < strbuf_getlen(asm_symtable)		\
			 ? &strbuf_value(asm_symtable)[offset]		\
			 : (die("BUG!!"), (char *)NULL))

int yylex(void);
void asm_initscan(void);

const char *asm_input_file;
STRBUF *asm_symtable;

static void yyerror(const char *);

static int target;

%}

%token ASM_CONST		/* number, string, character */

%token ASM_CALL			/* call, jsr */
%token ASM_ENTRY		/* ENTRY, ALTENTRY, ... */
%token ASM_EXT			/* EXT, SYMBOL_NAME, ... */
%token ASM_SYMBOL_PAREN		/* sym( */
%token ASM_SYMBOL

%token ASM_DEFINE "#define"
%token ASM_UNDEF "#undef"
%token ASM_DIRECTIVE		/* #xxx */

%start input
%name-prefix="asm_"

%%

input:	/* empty */
	| input line
;

line:	ASM_ENTRY '(' ASM_SYMBOL ')' error '\n'
		{
			if (target == REF) {
				char *sym = GET_SYM($1);

				if (defined(sym))
					PUT(sym, @1.first_line);
			}
			if (target == DEF)
				PUT(GET_SYM($3), @3.first_line);
			strbuf_reset(asm_symtable);
		}
	| ASM_CALL ASM_SYMBOL error '\n'
		{
			if (target == REF) {
				char *sym = GET_SYM($2);

				if (sym[0] == '_') {
					int c = (unsigned char)sym[1];

					if ((isalpha(c) || c == '_' || c >= 0x80)
					    && defined(&sym[1]))
						PUT(&sym[1], @2.first_line);
				}
			}
			strbuf_reset(asm_symtable);
		}
	| ASM_CALL ASM_EXT '(' ASM_SYMBOL ')' error '\n'
		{
			if (target == REF) {
				char *sym;

				sym = GET_SYM($2);
				if (defined(sym))
					PUT(sym, @2.first_line);

				sym = GET_SYM($4);
				if (defined(sym))
					PUT(sym, @4.first_line);
			}
			strbuf_reset(asm_symtable);
		}
	| "#define" ASM_SYMBOL error '\n'
		{
			if (target == DEF && dflag)
				PUT(GET_SYM($2), @2.first_line);
			strbuf_reset(asm_symtable);
		}
	| "#define" ASM_SYMBOL_PAREN error '\n'
		{
			if (target == DEF)
				PUT(GET_SYM($2), @2.first_line);
			strbuf_reset(asm_symtable);
		}
	| "#undef" ASM_SYMBOL error '\n'
		{
			if (target == DEF && dflag)
				PUT(GET_SYM($2), @2.first_line);
			strbuf_reset(asm_symtable);
		}
	| error '\n'
		{ strbuf_reset(asm_symtable); }
;

%%

void
assembler(file)
const char *file;
{
	/* symbol search doesn't supported. */
	if (sflag)
		return;
	target = (rflag) ? REF : DEF;

	if (linetable_open(file) == -1)
		die("'%s' cannot open.", file);
	asm_input_file = file;

	asm_symtable = strbuf_open(0);
	asm_initscan();

	asm_parse();

	strbuf_close(asm_symtable);
	linetable_close();
}

static void
yyerror(s)
const char *s;
{

}
