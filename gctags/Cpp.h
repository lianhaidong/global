/*
 * Copyright (c) 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000, 2002
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

#define CPP___P		1001
#define CPP___ATTRIBUTE__	1002
#define CPP_ASM		1003
#define CPP_AUTO	1004
#define CPP_BREAK	1005
#define CPP_CASE	1006
#define CPP_CATCH	1007
#define CPP_CHAR	1008
#define CPP_CLASS	1009
#define CPP_CONST	1010
#define CPP_CONTINUE	1011
#define CPP_DEFAULT	1012
#define CPP_DELETE	1013
#define CPP_DO		1014
#define CPP_DOUBLE	1015
#define CPP_ELSE	1016
#define CPP_ENUM	1017
#define CPP_EXPLICIT	1018
#define CPP_EXTERN	1019
#define CPP_FLOAT	1020
#define CPP_FOR		1021
#define CPP_FRIEND	1022
#define CPP_GOTO	1023
#define CPP_IF		1024
#define CPP_INLINE	1025
#define CPP_INT		1026
#define CPP_LONG	1027
#define CPP_MUTABLE	1028
#define CPP_NAMESPACE	1029
#define CPP_NEW		1030
#define CPP_OPERATOR	1031
#define CPP_OVERLOAD	1032
#define CPP_PRIVATE	1033
#define CPP_PROTECTED	1034
#define CPP_PUBLIC	1035
#define CPP_REGISTER	1036
#define CPP_RETURN	1037
#define CPP_SHORT	1038
#define CPP_SIGNED	1039
#define CPP_SIZEOF	1040
#define CPP_STATIC	1041
#define CPP_STRUCT	1042
#define CPP_SWITCH	1043
#define CPP_TEMPLATE	1044
#define CPP_THIS	1045
#define CPP_THROW	1046
#define CPP_TRY		1047
#define CPP_TYPEDEF	1048
#define CPP_TYPENAME	1049
#define CPP_UNION	1050
#define CPP_UNSIGNED	1051
#define CPP_USING	1052
#define CPP_VIRTUAL	1053
#define CPP_VOID	1054
#define CPP_VOLATILE	1055
#define CPP_WHILE	1056
#define CPP_SEP		1057

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
#define CP_ASSERT	2016

#define IS_CPPTOKEN(c)	(c > 1000 && c < 2001)
#define IS_CPTOKEN(c)	(c > 2000 && c < 3001)
#define MAXCOMPLETENAME 1024            /* max size of complete name of class */
#define MAXCLASSSTACK   100             /* max size of class stack */
