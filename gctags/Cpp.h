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

enum {
	CPPTOKEN_BASE = 1000,
	CPP___P,
	CPP___ATTRIBUTE__,
	CPP___EXTENSION__,
	CPP_ASM,
	CPP_AUTO,
	CPP_BOOL,
	CPP_BREAK,
	CPP_CASE,
	CPP_CATCH,
	CPP_CHAR,
	CPP_CLASS,
	CPP_CONST,
	CPP_CONST_CAST,
	CPP_CONTINUE,
	CPP_DEFAULT,
	CPP_DELETE,
	CPP_DO,
	CPP_DOUBLE,
	CPP_DYNAMIC_CAST,
	CPP_ELSE,
	CPP_ENUM,
	CPP_EXPLICIT,
	CPP_EXPORT,
	CPP_EXTERN,
	CPP_FALSE,
	CPP_FLOAT,
	CPP_FOR,
	CPP_FRIEND,
	CPP_GOTO,
	CPP_IF,
	CPP_INLINE,
	CPP_INT,
	CPP_LONG,
	CPP_MUTABLE,
	CPP_NAMESPACE,
	CPP_NEW,
	CPP_OPERATOR,
	CPP_PRIVATE,
	CPP_PROTECTED,
	CPP_PUBLIC,
	CPP_REGISTER,
	CPP_REINTERPRET_CAST,
	CPP_RETURN,
	CPP_SHORT,
	CPP_SIGNED,
	CPP_SIZEOF,
	CPP_STATIC,
	CPP_STATIC_CAST,
	CPP_STRUCT,
	CPP_SWITCH,
	CPP_TEMPLATE,
	CPP_THIS,
	CPP_THROW,
	CPP_TRUE,
	CPP_TRY,
	CPP_TYPEDEF,
	CPP_TYPENAME,
	CPP_TYPEID,
	CPP_UNION,
	CPP_UNSIGNED,
	CPP_USING,
	CPP_VIRTUAL,
	CPP_VOID,
	CPP_VOLATILE,
	CPP_WCHAR_T,
	CPP_WHILE,
	CPP_SEP,

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
	CP_WARNING
};

#define IS_CPPTOKEN(c)	((c) > CPPTOKEN_BASE && (c) < CPPTOKEN_BASE + 1000)
#define IS_CPTOKEN(c)	((c) > CPTOKEN_BASE && (c) < CPTOKEN_BASE + 1000)

#define MAXCOMPLETENAME 1024            /* max size of complete name of class */
#define MAXCLASSSTACK   100             /* max size of class stack */
