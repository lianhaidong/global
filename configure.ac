dnl
dnl Configuration script for GLOBAL.
dnl Process this file with autoconf to produce a configure script.
dnl
dnl Copyright (c) 2000, 2001, 2002, 2003, 2004 Tama Communications Corporation
dnl
dnl This file is part of GNU GLOBAL.
dnl
dnl GNU GLOBAL is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl GNU GLOBAL is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
dnl
AC_INIT(global/global.c)
AM_CONFIG_HEADER(config.h:config-h.in)
AM_INIT_AUTOMAKE(global, 4.7.1)
AC_PREREQ(2.57)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
dnl
dnl GLOBAL needs following tools for execution.
dnl	perl, sed, sort
dnl (Find is no longer needed.)
dnl
AC_PATH_PROG(PERL, perl, no)
if test ${ac_cv_path_PERL} = no; then
	AC_MSG_ERROR([PERL is required but not found.])
fi
AC_PATH_PROG(SORT, sort, no)
if test ${ac_cv_path_SORT} = no; then
	AC_MSG_ERROR([SORT is required but not found.])
fi
AC_PATH_PROG(SED, sed, no)
if test ${ac_cv_path_SED} = no; then
	AC_MSG_ERROR([SED is required but not found.])
fi
AC_PATH_PROG(FIND, find, no)
if test ${ac_cv_path_FIND} != no; then
	AC_DEFINE(HAVE_FIND,1,[Define if your system have find(1).])
fi

dnl Checks for libraries.

dnl Checks for header files.
AC_CHECK_HEADERS(limits.h string.h unistd.h stdarg.h sys/time.h)
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_STAT
AC_HEADER_TIME
AG_BYTE_ORDER

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_STRUCT_ST_BLKSIZE
dnl
dnl In GNU/Linux, struct dirent has d_type but the value is always 0.
dnl So, we decided not to use these macros for the time being.
dnl
dnl AG_STRUCT_DP_D_NAMLEN
dnl AG_STRUCT_DP_D_TYPE
dnl
AC_CHECK_TYPE(int8_t, signed char)
AC_CHECK_TYPE(int16_t, short)
AC_CHECK_TYPE(int32_t, int)
AC_CHECK_TYPE(u_int8_t, unsigned char)
AC_CHECK_TYPE(u_int16_t, unsigned short)
AC_CHECK_TYPE(u_int32_t, unsigned int)
AC_CHECK_TYPE(ssize_t, int)
AC_CHECK_TYPE(caddr_t, char *)
AC_CHECK_SIZEOF(int, 4)
if test ${ac_cv_sizeof_int} != 4; then
	AC_MSG_ERROR([Integer size isn't 32 bit.])
fi
AC_CHECK_SIZEOF(short, 2)
if test ${ac_cv_sizeof_short} != 2; then
	AC_MSG_ERROR([Short integer size isn't 16 bit.])
fi
AC_CHECK_SIZEOF(char, 1)
if test ${ac_cv_sizeof_char} != 1; then
	AC_MSG_ERROR([Char size isn't 8 bit.])
fi

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS(getcwd putenv strdup lstat snprintf utimes)
AC_CHECK_FUNCS(index rindex bzero bcmp bcopy strchr strrchr memset memcmp memmove)
AG_DJGPP

INCLUDES='-I$(top_srcdir)/libutil -I$(top_srcdir)/libdb -I$(top_srcdir)/libglibc -I../libutil'
LDADD='../libutil/libgloutil.a ../libdb/libglodb.a ../libglibc/libgloglibc.a'

dnl
dnl for Postgres support
dnl
AC_MSG_CHECKING(for postgres support)
AC_ARG_WITH(postgres,
[  --with-postgres[=DIR]     include postgres support ],
[
        if test "$withval" = 'no'; then
                AC_MSG_RESULT(no)
        else
                case $withval in
                ''|yes) POSTGRES=/usr/local/pgsql;;
                *)      POSTGRES=$withval;;
                esac
                if ! test -d $POSTGRES; then
                        AC_MSG_ERROR([directory '$POSTGRES' not found.])
                fi
                INCLUDES="$INCLUDES -I$POSTGRES/include"
                LDFLAGS="$LDFLAGS -L$POSTGRES/lib -lpq -lcrypt"
                AC_DEFINE(USE_POSTGRES,1,[Define if you use postgres.])
                AC_MSG_RESULT([yes, using $POSTGRES])
        fi
],[
        AC_MSG_RESULT(no)
])
dnl
dnl for home-etc support
dnl
AC_MSG_CHECKING(for home-etc support)
AC_ARG_WITH(home-etc,
[  --with-home-etc[=DIR]     include home-etc support ],
[
        if test "$withval" = 'no'; then
                AC_MSG_RESULT(no)
        else
                case $withval in
                ''|yes) HOMEETC=/usr/local;;
                *)      HOMEETC=$withval;;
                esac
                if ! test -d $HOMEETC; then
                        AC_MSG_ERROR([directory '$HOMEETC' not found.])
                fi
		if ! test -f $HOMEETC/include/home_etc.h; then
                        AC_MSG_ERROR([header '$HOMEETC/include/home_etc.h' not found.])
                fi
                INCLUDES="$INCLUDES -I$HOMEETC/include"
                LDFLAGS="$LDFLAGS -L$HOMEETC/lib -lhome_etc"
                AC_DEFINE(HAVE_HOME_ETC_H,1,[Define if you use home-etc facility.])
                AC_MSG_RESULT([yes, using $HOMEETC])
        fi
],[
        AC_MSG_RESULT(no)
])

AC_SUBST(INCLUDES)
AC_SUBST(LDADD)
AC_SUBST(LDFLAGS)

dnl
dnl The definition of config variable skip and suffixes.
dnl
DEFAULTSUFFIXES='c,h,y,c++,cc,cpp,cxx,hxx,hpp,C,H,s,S,java,php,php3,phtml'
DEFAULTSKIP='GPATH,GTAGS,GRTAGS,GSYMS,HTML/,tags,TAGS,ID,y.tab.c,y.tab.h,.notfunction,cscope.out,.gdbinit,SCCS/,RCS/,CVS/,CVSROOT/,{arch}/,.svn/,.cvsrc,.cvsignore,.cvspass,.cvswrappers,.deps/,autom4te.cache/'
AC_SUBST(DEFAULTSUFFIXES)
AC_SUBST(DEFAULTSKIP)

AC_OUTPUT([Makefile
	gtags.conf
	libutil/conf.h
	libutil/Makefile
	gtags/Makefile
	htags/Makefile
	gctags/Makefile
	libdb/Makefile
	global/Makefile
	gozilla/Makefile
	libglibc/Makefile
	doc/Makefile
])
