/*
 * Copyright (c) 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2003 Tama Communications Corporation
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
 * java's reserved words.
 */
enum {
	JTOKEN_BASE = 1000,
	J_ABSTRACT,
	J_BOOLEAN,
	J_BREAK,
	J_BYTE,
	J_CASE,
	J_CATCH,
	J_CHAR,
	J_CLASS,
	J_CONST,
	J_CONTINUE,
	J_DEFAULT,
	J_DO,
	J_DOUBLE,
	J_ELSE,
	J_EXTENDS,
	J_FALSE,
	J_FINAL,
	J_FINALLY,
	J_FLOAT,
	J_FOR,
	J_GOTO,
	J_IF,
	J_IMPLEMENTS,
	J_IMPORT,
	J_INSTANCEOF,
	J_INT,
	J_INTERFACE,
	J_LONG,
	J_NATIVE,
	J_NEW,
	J_NULL,
	J_PACKAGE,
	J_PRIVATE,
	J_PROTECTED,
	J_PUBLIC,
	J_RETURN,
	J_SHORT,
	J_STATIC,
	J_SUPER,
	J_SWITCH,
	J_SYNCHRONIZED,
	J_THIS,
	J_THROW,
	J_THROWS,
	J_UNION,
	J_TRANSIENT,
	J_TRUE,
	J_TRY,
	J_VOID,
	J_VOLATILE,
	J_WHILE,
	J_STRICTFP,
	J_WIDEFP
};

#define MAXCOMPLETENAME 1024            /* max size of complete name of class */
#define MAXCLASSSTACK   100             /* max size of class stack */
