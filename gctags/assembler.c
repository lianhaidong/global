/*
 * Copyright (c) 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2002, 2003 Tama Communications Corporation
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
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "gctags.h"
#include "defined.h"
#include "token.h"

static int      reserved_word(const char *, int);

#define A_CALL		1001
#define A_DEFINE	1002
#define A_ENTRY		1003
#define A_EXT		1004
#define A_ALTENTRY	1005
#define A_NENTRY	1006
#define A_SYMBOL_NAME	1007
#define A_C_LABEL	1008
#define A_GLOBAL_ENTRY	1009
#define A_JSBENTRY	1010

void
assembler()
{
	int	c;
	int	target;
	const   char *interested = NULL;	/* get all token */
	int	startline = 1;
	int	level;				/* not used */

	level = 0;				/* to satisfy compiler */
	/* symbol search doesn't supported. */
	if (sflag)
		return;
	target = (rflag) ? REF : DEF;

	cmode = 1;
	crflag = 1;

	while ((c = nexttoken(interested, reserved_word)) != EOF) {
		switch (c) {
		case '\n':
			startline = 1;
			continue;
		case A_CALL:
			if (!startline || target != REF)
				break;
			if ((c = nexttoken(interested, reserved_word)) == A_EXT || c == A_SYMBOL_NAME || c == A_C_LABEL) {
				if ((c = nexttoken(interested, reserved_word)) == '('/* ) */)
					if ((c = nexttoken(interested, reserved_word)) == SYMBOL)
						if (defined(token))
							PUT(token, lineno, sp);
			} else if (c == SYMBOL && *token == '_') {
				if (defined(&token[1]))
					PUT(&token[1], lineno, sp);
			}
			break;
		case A_ENTRY:
		case A_ALTENTRY:
		case A_NENTRY:
		case A_GLOBAL_ENTRY:
		case A_JSBENTRY:
			if (!startline || target != DEF)
				break;
			if ((c = nexttoken(interested, reserved_word)) == '('/* ) */) {
				if ((c = nexttoken(interested, reserved_word)) == SYMBOL)
					PUT(token, lineno, sp);
				while ((c = nexttoken(interested, reserved_word)) != EOF && c != '\n' && c != /* ( */ ')')
					;
			}
			break;
		case A_DEFINE:
			if (!startline || target != DEF)
				break;
			if ((c = nexttoken(interested, reserved_word)) == SYMBOL) {
				if (peekc(1) == '('/* ) */) {
					PUT(token, lineno, sp);
					while ((c = nexttoken(interested, reserved_word)) != EOF && c != '\n' && c != /* ( */ ')')
						;
					while ((c = nexttoken(interested, reserved_word)) != EOF && c != '\n')
						;
				}
			}
		default:
			break;
		}
		startline = 0;
	}
}
static int
reserved_word(word, length)
        const char *word;
	int length;
{
	switch (*word) {
	case '#':
		if (!strcmp(word, "#define"))
			return A_DEFINE;
		break;
	case 'A':
		if (!strcmp(word, "ALTENTRY"))
			return A_ALTENTRY;
		break;
	case 'C':
		if (!strcmp(word, "C_LABEL"))
			return A_C_LABEL;
		break;
	case 'E':
		if (!strcmp(word, "ENTRY"))
			return A_ENTRY;
		else if (!strcmp(word, "EXT"))
			return A_EXT;
		break;
	case 'G':
		if (!strcmp(word, "GLOBAL_ENTRY"))
			return A_GLOBAL_ENTRY;
		break;
	case 'J':
		if (!strcmp(word, "JSBENTRY"))
			return A_JSBENTRY;
		break;
	case 'N':
		if (!strcmp(word, "NENTRY"))
			return A_NENTRY;
		break;
	case 'S':
		if (!strcmp(word, "SYMBOL_NAME"))
			return A_SYMBOL_NAME;
		break;
	case 'c':
		if (!strcmp(word, "call"))
			return A_CALL;
		break;
	default:
		break;
	}
	return SYMBOL;
}
