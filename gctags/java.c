/*
 * Copyright (c) 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2002 Tama Communications Corporation
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
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "gctags.h"
#include "defined.h"
#include "die.h"
#include "java.h"
#include "strlimcpy.h"
#include "token.h"

static int	reserved(char *);

/*
 * java: read java file and pickup tag entries.
 */
void
java()
{
	int	c;
	int	level;					/* brace level */
	int	target;
	int	startclass, startthrows, startequal;
	char	classname[MAXTOKEN];
	char	completename[MAXCOMPLETENAME];
	int	classlevel;
	struct {
		char *classname;
		char *terminate;
		int   level;
	} stack[MAXCLASSSTACK];
	const char *interested = "{}=;";

	stack[0].terminate = completename;
	stack[0].level = 0;
	level = classlevel = 0;
	target = (sflag) ? SYM : ((rflag) ? REF : DEF);
	startclass = startthrows = startequal = 0;

	while ((c = nexttoken(interested, reserved)) != EOF) {
		switch (c) {
		case SYMBOL:					/* symbol */
			for (; c == SYMBOL && peekc(1) == '.'; c = nexttoken(interested, reserved)) {
				if (target == SYM)
					PUT(token, lineno, sp);
			}
			if (c != SYMBOL)
				break;
			if (startclass || startthrows) {
				if (target == REF && defined(token))
					PUT(token, lineno, sp);
			} else if (peekc(0) == '('/* ) */) {
				if (target == DEF && level == stack[classlevel].level && !startequal)
					/* ignore constructor */
					if (strcmp(stack[classlevel].classname, token))
						PUT(token, lineno, sp);
				if (target == REF && (level > stack[classlevel].level || startequal) && defined(token))
					PUT(token, lineno, sp);
			} else {
				if (target == SYM)
					PUT(token, lineno, sp);
			}
			break;
		case '{': /* } */
			DBG_PRINT(level, "{");	/* } */

			++level;
			if (startclass) {
				char *p = stack[classlevel].terminate;
				char *q = classname;

				if (++classlevel >= MAXCLASSSTACK)
					die("class stack over flow.[%s]", curfile);
				if (classlevel > 1)
					*p++ = '.';
				stack[classlevel].classname = p;
				while (*q)
					*p++ = *q++;
				stack[classlevel].terminate = p;
				stack[classlevel].level = level;
				*p++ = 0;
			}
			startclass = startthrows = 0;
			break;
			/* { */
		case '}':
			if (--level < 0) {
				if (wflag)
					fprintf(stderr, "Warning: missing left '{' (at %d).\n", lineno); /* } */
				level = 0;
			}
			if (level < stack[classlevel].level)
				*(stack[--classlevel].terminate) = 0;
			/* { */
			DBG_PRINT(level, "}");
			break;
		case '=':
			startequal = 1;
			break;
		case ';':
			startclass = startthrows = startequal = 0;
			break;
		case J_CLASS:
		case J_INTERFACE:
			if ((c = nexttoken(interested, reserved)) == SYMBOL) {
				strlimcpy(classname, token, sizeof(classname));
				startclass = 1;
				if (target == DEF)
					PUT(token, lineno, sp);
			}
			break;
		case J_NEW:
		case J_INSTANCEOF:
			while ((c = nexttoken(interested, reserved)) == SYMBOL && peekc(1) == '.')
				if (target == SYM)
					PUT(token, lineno, sp);
			if (c == SYMBOL)
				if (target == REF && defined(token))
					PUT(token, lineno, sp);
			break;
		case J_THROWS:
			startthrows = 1;
			break;
		case J_BOOLEAN:
		case J_BYTE:
		case J_CHAR:
		case J_DOUBLE:
		case J_FLOAT:
		case J_INT:
		case J_LONG:
		case J_SHORT:
		case J_VOID:
			if (peekc(1) == '.' && (c = nexttoken(interested, reserved)) != J_CLASS)
				pushbacktoken();
			break;
		default:
			break;
		}
	}
}
		/* sorted by alphabet */
static struct words words[] = {
	{"abstract",	J_ABSTRACT},
	{"boolean",	J_BOOLEAN},
	{"break",	J_BREAK},
	{"byte",	J_BYTE},
	{"case",	J_CASE},
	{"catch",	J_CATCH},
	{"char",	J_CHAR},
	{"class",	J_CLASS},
	{"const",	J_CONST},
	{"continue",	J_CONTINUE},
	{"default",	J_DEFAULT},
	{"do",		J_DO},
	{"double",	J_DOUBLE},
	{"else",	J_ELSE},
	{"extends",	J_EXTENDS},
	{"false",	J_FALSE},
	{"final",	J_FINAL},
	{"finally",	J_FINALLY},
	{"float",	J_FLOAT},
	{"for",		J_FOR},
	{"goto",	J_GOTO},
	{"if",		J_IF},
	{"implements",	J_IMPLEMENTS},
	{"import",	J_IMPORT},
	{"instanceof",	J_INSTANCEOF},
	{"int",		J_INT},
	{"interface",	J_INTERFACE},
	{"long",	J_LONG},
	{"native",	J_NATIVE},
	{"new",		J_NEW},
	{"null",	J_NULL},
	{"package",	J_PACKAGE},
	{"private",	J_PRIVATE},
	{"protected",	J_PROTECTED},
	{"public",	J_PUBLIC},
	{"return",	J_RETURN},
	{"short",	J_SHORT},
	{"static",	J_STATIC},
	{"strictfp",	J_STRICTFP},
	{"super",	J_SUPER},
	{"switch",	J_SWITCH},
	{"synchronized",J_SYNCHRONIZED},
	{"this",	J_THIS},
	{"throw",	J_THROW},
	{"throws",	J_THROWS},
	{"union",	J_UNION},
	{"transient",	J_TRANSIENT},
	{"true",	J_TRUE},
	{"try",		J_TRY},
	{"void",	J_VOID},
	{"volatile",	J_VOLATILE},
	{"while",	J_WHILE},
	{"widefp",	J_WIDEFP},
};

static int
reserved(word)
        char *word;
{
	struct words tmp;
	struct words *result;

	tmp.name = word;
	result = (struct words *)bsearch(&tmp, words, sizeof(words)/sizeof(struct words), sizeof(struct words), cmp);
	return (result != NULL) ? result->val : SYMBOL;
}
