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
#ifndef _LEXCOMMON_H
#define _LEXCOMMON_H

/*
 * The default action for line control.
 * These can be applicable to most languages.
 * You must define C_COMMENT, CPP_COMMENT and SHELL_COMMENT as %start values.
 * It assumed CPP_COMMENT and SHELL_COMMENT is one line comment.
 */
static int lineno;
static int begin_line;

#define LINENO	lineno

#define DEFAULT_BEGIN_OF_FILE_ACTION {					\
        yyin = ip;							\
        yyrestart(yyin);						\
        lineno = 1;							\
        begin_line = 1;							\
}

#define DEFAULT_YY_USER_ACTION {					\
        if (begin_line) {                                               \
                put_begin_of_line(lineno);                              \
                if (YY_START == C_COMMENT)                              \
                        echos(comment_begin);                           \
                begin_line = 0;                                         \
        }                                                               \
}

#define DEFAULT_END_OF_LINE_ACTION {                                    \
	if (YY_START == CPP_COMMENT || YY_START == C_COMMENT || YY_START == SHELL_COMMENT) \
		echos(comment_end);					\
	if (YY_START == CPP_COMMENT || YY_START == SHELL_COMMENT)	\
		yy_pop_state();						\
        put_end_of_line();                                              \
        /* for the next line */                                         \
        lineno++;                                                       \
        begin_line = 1;                                                 \
}

/*
 * Input.
 */
extern FILE *yyin;

/*
 * Output routine.
 */
extern void echo(const char *s, ...);
extern void echoc(int);
extern void echos(const char *s);
extern char *generate_guide(int);
extern void put_anchor(char *, int, int);
extern void put_reserved_word(char *);
extern void put_macro(char *);
extern void put_char(int);
extern void put_string(char *);
extern void put_brace(char *);
extern void put_lineno(int);
extern void put_begin_of_line(int);
extern void put_end_of_line(void);

#endif /* ! _LEXCOMMON_H */
