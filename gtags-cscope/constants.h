/*===========================================================================
 Copyright (c) 1998-2000, The Santa Cruz Operation 
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 *Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 *Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 *Neither name of The Santa Cruz Operation nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE. 
 =========================================================================*/

/* $Id$ */

/*
 *	preprocessor macro and constant definitions
 *
 *	cscope - interactive C symbol cross-reference
 */

#ifndef CSCOPE_CONSTANTS_H
#define CSCOPE_CONSTANTS_H

#include "config.h"		/* Get OS defines */

/** control character macro */
#define ctrl(x)	(x & 037)

/*
 *	fast string equality tests (avoids most strcmp() calls)
 */
#define	strequal(s1, s2)	(*(s1) == *(s2) && strcmp(s1, s2) == 0)
#define	strnotequal(s1, s2)	(*(s1) != *(s2) || strcmp(s1, s2) != 0)

/** set the mark character for searching the cross-reference file */
#define	setmark(c)	(blockmark = c, block[blocklen] = blockmark)

/** get the next character in the cross-reference.
	note that blockp is assumed not to be null */
#define	getrefchar()	(*(++blockp + 1) != '\0' ? *blockp : \
			(read_block() != NULL ? *blockp : '\0'))

/** skip the next character in the cross-reference.
   note that blockp is assumed not to be null and that
   this macro will always be in a statement by itself */
#define	skiprefchar()	if (*(++blockp + 1) == '\0') (void) read_block()

/** escape character */
#define	ESC	'\033'

/** delete character */
#define	DEL	'\177'

/** use space as a dummy character */
#define	DUMMYCHAR	' '

/** displayed message length */
#define	MSGLEN	((PATLEN) + 80)

/** line number length */
#define	NUMLEN	5

/** file pathname length */
#define	PATHLEN	250

/** symbol pattern length */
#define	PATLEN	250

/** max strlen() of the global temp string */
#define TEMPSTRING_LEN 8191

/** cross-reference output file */
#define	REFFILE	"cscope.out"

/** default list-of-files file */
#define	NAMEFILE "cscope.files"

/** inverted index to the database */
#define	INVNAME	"cscope.in.out"

/** inverted index postings */
#define	INVPOST	"cscope.po.out"

/** follows correct naming convention */
#define	INVNAME2 "cscope.out.in"

/** follows correct naming convention */
#define	INVPOST2 "cscope.out.po"


/** maximum source statement length */
#define	STMTMAX	10000

#define STR2(x) #x
#define STRINGIZE(x) STR2(x)
#define PATLEN_STR STRINGIZE(PATLEN)
#define PATHLEN_STR STRINGIZE(PATHLEN)
#define NUMLEN_STR STRINGIZE(NUMLEN)
#define TEMPSTRING_LEN_STR STRINGIZE(TEMPSTRING_LEN)

/* screen lines */
/** first input field line */
#define	FLDLINE	(LINES - FIELDS - 1)

/** message line */
#define	MSGLINE	0

/** input prompt line */
#define	PRLINE	(LINES - 1)

/** first displayed reference line */
#define REFLINE	3

/* input fields (value matches field order on screen) */
#define	SYMBOL		0
#define DEFINITION	1
#define	CALLEDBY	2
#define	CALLING		3
#define	STRING		4
#define	CHANGE		5
#define	REGEXP		6
#define FILENAME	7
#define INCLUDES	8
#define	FIELDS		9

#if (BSD || V9) && !__NetBSD__ && !__FreeBSD__
	/** no terminfo curses */
# define TERMINFO	0
#else
# define TERMINFO	1
#endif


#if !TERMINFO
# ifndef KEY_BREAK
#  define	KEY_BREAK	0400	/* easier to define than to add #if around the use */
# endif
# ifndef KEY_ENTER
#  define	KEY_ENTER	0401
# endif
# ifndef KEY_BACKSPACE
#  define	KEY_BACKSPACE	0402
# endif

# if !sun
	/** name change */
#  define cbreak()	crmode()
# endif

# if UNIXPC
#  define erasechar() (_tty.c_cc[VERASE])		/* equivalent */
#  define killchar()  (_tty.c_cc[VKILL])		/* equivalent */
# else
#  define erasechar() (_tty.sg_erase)			/* equivalent */
#  define killchar()  (_tty.sg_kill)			/* equivalent */
# endif /* if UNIXPC */
#endif	/* if !TERMINFO */

#endif /* CSCOPE_CONSTANTS_H */
