/*
 * Copyright (c) 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2001, 2002 Tama Communications Corporation
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

#define DECLARATIONS	0
#define RULES		1
#define PROGRAMS	2

enum {
	CTOKEN_BASE = 1000,
	C___P,
	C___ATTRIBUTE__,
	C__BOOL,
	C__COMPLEX,
	C__IMAGINARY,
	C_ASM,
	C_AUTO,
	C_BREAK,
	C_CASE,
	C_CHAR,
	C_CONST,
	C_CONTINUE,
	C_DEFAULT,
	C_DO,
	C_DOUBLE,
	C_ELSE,
	C_ENUM,
	C_EXTERN,
	C_FLOAT,
	C_FOR,
	C_GOTO,
	C_IF,
	C_INT,
	C_INLINE,
	C_LONG,
	C_REGISTER,
	C_RESTRICT,
	C_RETURN,
	C_SHORT,
	C_SIGNED,
	C_SIZEOF,
	C_STATIC,
	C_STRUCT,
	C_SWITCH,
	C_TYPEDEF,
	C_UNION,
	C_UNSIGNED,
	C_VOID,
	C_VOLATILE,
	C_WHILE,

	CPTOKEN_BASE = 2000,
	CP_ASSERT,
	CP_DEFINE,
	CP_ELIF,
	CP_ELSE,
	CP_ENDIF,
	CP_ERROR,
	CP_IDENT,
	CP_IF,
	CP_IFDEF,
	CP_IFNDEF,
	CP_INCLUDE,
	CP_LINE,
	CP_PRAGMA,
	CP_SHARP,
	CP_UNDEF,
	CP_WARNING,

	YACCTOKEN_BASE = 3000,
	YACC_SEP,
	YACC_BEGIN,
	YACC_END,
	YACC_LEFT,
	YACC_NONASSOC,
	YACC_RIGHT,
	YACC_START,
	YACC_TOKEN,
	YACC_TYPE,
	YACC_UNION,
	YACC_TERM
};

#define IS_CTOKEN(c)	((c) > CTOKEN_BASE && (c) < CTOKEN_BASE + 1000)
#define IS_CPTOKEN(c)	((c) > CPTOKEN_BASE && (c) < CPTOKEN_BASE + 1000)
#define IS_YACCTOKEN(c)	((c) > YACCTOKEN_BASE && (c) < YACCTOKEN_BASE + 1000)
