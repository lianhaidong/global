/*
 * Copyright (c) 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2002, 2003, 2004 Tama Communications Corporation
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
#include "die.h"
#include "token.h"
#include "asm_res.h"

void
assembler()
{
	int c;
	int target;
	const char *interested = NULL;	/* get all token */
	int startline = 1;
	int level = 0;			/* not used */
	int piflevel = 0;		/* condition macro level */

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
		case ASM_CALL:
			if (!startline || target != REF)
				break;
			if ((c = nexttoken(interested, reserved_word)) == ASM_EXT || c == ASM_SYMBOL_NAME || c == ASM_C_LABEL) {
				if ((c = nexttoken(interested, reserved_word)) == '('/* ) */)
					if ((c = nexttoken(interested, reserved_word)) == SYMBOL)
						if (defined(token))
							PUT(token, lineno, sp);
			} else if (c == SYMBOL && *token == '_') {
				if (defined(&token[1]))
					PUT(&token[1], lineno, sp);
			}
			if (c == '\n')
				pushbacktoken();
			break;
		case ASM_ENTRY:
		case ASM_ALTENTRY:
		case ASM_NENTRY:
		case ASM_GLOBAL_ENTRY:
		case ASM_JSBENTRY:
			if (!startline || target != DEF)
				break;
			if ((c = nexttoken(interested, reserved_word)) == '('/* ) */) {
				if ((c = nexttoken(interested, reserved_word)) == SYMBOL)
					PUT(token, lineno, sp);
			}
			if (c == '\n')
				pushbacktoken();
			break;
		/*
		 * #xxx
		 */
		case SHARP_DEFINE:
		case SHARP_UNDEF:
			if (!startline || target != DEF)
				break;
			if ((c = nexttoken(interested, reserved_word)) == SYMBOL) {
				if (peekc(1) == '('/* ) */ || dflag) {
					PUT(token, lineno, sp);
				}
			}
			if (c == '\n')
				pushbacktoken();
			break;
		case SHARP_IFDEF:
		case SHARP_IFNDEF:
		case SHARP_IF:
			DBG_PRINT(piflevel, "#if");
			piflevel++;
			break;
		case SHARP_ELIF:
		case SHARP_ELSE:
			DBG_PRINT(piflevel - 1, "#else");
			break;
		case SHARP_ENDIF:
			if (--piflevel < 0) {
				piflevel = 0;
				if (wflag)
					warning("#if block unmatched. reseted. [+%d %s]", lineno, curfile);
			}
			DBG_PRINT(piflevel, "#endif");
			break;
		default:
			break;
		}
		startline = 0;
	}
	if (piflevel != 0 && wflag)
		warning("#if block unmatched. (last at level %d.)[+%d %s]", piflevel, lineno, curfile);
}
