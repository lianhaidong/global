/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000
 *             Tama Communications Corporation. All rights reserved.
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

#define C___P		1001
#define C_ASM		1002
#define C_AUTO		1003
#define C_BREAK		1004
#define C_CASE		1005
#define C_CHAR		1006
#define C_CONTINUE	1007
#define C_DEFAULT	1008
#define C_DO		1009
#define C_DOUBLE	1010
#define C_ELSE		1011
#define C_ENUM		1012
#define C_EXTERN	1013
#define C_FLOAT		1014
#define C_FOR		1015
#define C_GOTO		1016
#define C_IF		1017
#define C_INT		1018
#define C_LONG		1019
#define C_REGISTER	1020
#define C_RETURN	1021
#define C_SHORT		1022
#define C_SIGNED	1023
#define C_SIZEOF	1024
#define C_STATIC	1025
#define C_STRUCT	1026
#define C_SWITCH	1027
#define C_TYPEDEF	1028
#define C_UNION		1029
#define C_UNSIGNED	1030
#define C_VOID		1031
#define C_WHILE		1032
#define CP_ELIF		2001
#define CP_ELSE		2002
#define CP_DEFINE	2003
#define CP_IF		2004
#define CP_IFDEF	2005
#define CP_IFNDEF	2006
#define CP_INCLUDE	2007
#define CP_PRAGMA	2008
#define CP_SHARP	2009
#define CP_ERROR	2010
#define CP_UNDEF	2011
#define CP_ENDIF	2012
#define CP_LINE		2013
#define CP_WARNING	2014
#define CP_IDENT	2015
#define YACC_SEP	3001
#define YACC_BEGIN	3002
#define YACC_END	3003
#define YACC_LEFT	3004
#define YACC_NONASSOC	3005
#define YACC_RIGHT	3006
#define YACC_START	3007
#define YACC_TOKEN	3008
#define YACC_TYPE	3009
#define YACC_UNION	3010
#define YACC_TERM	3011

#define IS_CTOKEN(c)	(c > 1000 && c < 2001)
#define IS_CPTOKEN(c)	(c > 2000 && c < 3001)
#define IS_YACCTOKEN(c)	(c > 3000 && c < 4001)
