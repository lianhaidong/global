dnl
dnl Configuration script for GLOBAL.
dnl Process this file with autoconf to produce a configure script.
dnl
dnl Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2008, 2010,
dnl	2011, 2012, 2013, 2014
dnl	Tama Communications Corporation
dnl
dnl This file is part of GNU GLOBAL.
dnl
dnl This program is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl
AC_INIT([GNU GLOBAL],[6.4])
AC_CONFIG_SRCDIR(global/global.c)
AC_CONFIG_HEADERS([config.h:config-h.in])
AC_PREREQ(2.59)
AM_INIT_AUTOMAKE([1.9.3 gnu subdir-objects])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_PATH_LISPDIR
dnl
dnl Currently, absolute path name of perl is required only for CGI script.
dnl So, if you don't use the -f(--form) option of htags(1), it is not required.
dnl
AC_PATH_PROG(PERL, perl, [/usr/bin/env perl])
AC_CHECK_TOOL(AR, ar)
dnl
dnl for "plugin-factory/pygments_parser.py"
dnl
AM_PATH_PYTHON([2.6],,[PYTHON="/usr/bin/env python"])

AC_GNU_SOURCE

LT_PREREQ([2.2.6b])
LT_CONFIG_LTDL_DIR([libltdl])
dnl
dnl Because autoreconf up to version 2.59 doesn't know LT_INIT,
dnl we use the old name AC_PROG_LIBTOOL.
dnl
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
LTDL_INIT([recursive])

dnl Checks for libraries.

dnl Checks for header files.
AC_CHECK_HEADERS(limits.h string.h unistd.h stdarg.h sys/time.h fcntl.h)
AC_CHECK_HEADERS(sys/resource.h)
AC_HEADER_DIRENT
if test ${ac_header_dirent} = no; then
        AC_MSG_ERROR([dirent(3) is required but not found.])
fi
AC_HEADER_STDC
AC_HEADER_STAT
AC_HEADER_TIME

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_C_BIGENDIAN
AC_CHECK_TYPE([int8_t],,[AC_DEFINE_UNQUOTED([int8_t], [signed char],
		[Define to `signed char' if <sys/types.h> does not define.])])
AC_CHECK_TYPE([int16_t],,[AC_DEFINE_UNQUOTED([int16_t], [short],
		[Define to `short' if <sys/types.h> does not define.])])
AC_CHECK_TYPE([int32_t],,[AC_DEFINE_UNQUOTED([int32_t], [int],
		[Define to `int' if <sys/types.h> does not define.])])
AC_CHECK_TYPE([u_int8_t],,[AC_DEFINE_UNQUOTED([u_int8_t], [unsigned char],
		[Define to `unsigned char' if <sys/types.h> does not define.])])
AC_CHECK_TYPE([u_int16_t],,[AC_DEFINE_UNQUOTED([u_int16_t], [unsigned short],
		[Define to `unsigned short' if <sys/types.h> does not define.])])
AC_CHECK_TYPE([u_int32_t],,[AC_DEFINE_UNQUOTED([u_int32_t], [unsigned int],
		[Define to `unsigned int' if <sys/types.h> does not define.])])
AC_CHECK_TYPE([ssize_t],,[AC_DEFINE_UNQUOTED([ssize_t], [int],
		[Define to `int' if <sys/types.h> does not define.])])
AC_CHECK_TYPE([caddr_t],,[AC_DEFINE_UNQUOTED([caddr_t], [char *],
		[Define to `char *' if <sys/types.h> does not define.])])
AC_CHECK_SIZEOF(int)
if test ${ac_cv_sizeof_int} != 4; then
	AC_MSG_ERROR([Integer size isn't 32 bit.])
fi
AC_CHECK_SIZEOF(short)
if test ${ac_cv_sizeof_short} != 2; then
	AC_MSG_ERROR([Short integer size isn't 16 bit.])
fi
AC_CHECK_SIZEOF(char)
if test ${ac_cv_sizeof_char} != 1; then
	AC_MSG_ERROR([Char size isn't 8 bit.])
fi

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MMAP
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS(getcwd putenv lstat snprintf)
AC_CHECK_FUNCS(index rindex bzero bcmp bcopy strchr strrchr memset memcmp memmove)
AC_CHECK_FUNCS(putc_unlocked getc_unlocked)
AC_CHECK_FUNCS(gettimeofday getrusage)
AC_DJGPP

AC_ARG_ENABLE(gtagscscope,
[  --disable-gtagscscope   omit gtags-cscope],,
enable_gtagscscope=yes)
AM_CONDITIONAL([USE_GTAGSCSCOPE], [test "$enable_gtagscscope" != no])

dnl #### begin of gtags-cscope ###
AC_CHECK_CURSES
AM_CONDITIONAL(HAS_CURSES, test "$has_curses" = true)
if test "$has_curses" = false && test "$enable_gtagscscope" = yes; then
	AC_MSG_ERROR([curses library is required but not found.
If you are not going to use gtags-cscope, please try ./configure --disable-gtagscscope])
fi

dnl Checks for libraries.
dnl Replace `main' with a function in -lcurses:
dnl AC_CHECK_LIB(curses, main)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPE(sighandler_t,[],[],[
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif])
dnl This test was ripped from gnuplot's configure.in:
AC_MSG_CHECKING(for sigsetjmp)
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <setjmp.h>]], [[jmp_buf env; sigsetjmp(env, 1);]])],[AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_SIGSETJMP,1,
             [ Define if we have sigsetjmp(). ])],[AC_MSG_RESULT(no)])

dnl Checks for library functions.
AC_CHECK_FUNCS(fixkeypad strerror)

dnl case "$host_os" in
dnl 	linux*)
dnl 		AC_DEFINE(Linux, 1, [We're using some variant of Linux])
dnl 	;;
dnl 	netbsd*|freebsd*|darwin*)
dnl 		AC_DEFINE(BSD, 1, [We're using a BSD-flavoured Unix])
dnl 	;;
dnl esac
dnl #### end of gtags-cscope ###

AM_CPPFLAGS='-I$(top_srcdir)/libparser $(LTDLINCL) -I$(top_srcdir)/libutil -I$(top_srcdir)/libdb -I$(top_srcdir)/libglibc -I../libutil'
DBLIBRARY='../libdb/libglodb.a'

dnl
dnl This option is for debugging GLOBAL. Not for normal use.
dnl
AC_ARG_WITH(db185-compat,
[  --with-db185-compat[[=DIR]] use DB1.85 compatible API included in DB2 or later ],
[
	if test "$withval" != no; then
		case "$withval" in
		''|yes)
			;;
		*)
			if ! test -d "$withval"; then
				AC_MSG_ERROR([directory $withval not found.])
			fi
			if ! test -r "$withval/include/db_185.h"; then
				AC_MSG_ERROR([header $withval/include/db_185.h not found.])
			fi
			AM_CPPFLAGS="$AM_CPPFLAGS -I$withval/include"
			#LDFLAGS="$LDFLAGS -L$withval/lib"
			;;
		esac
		DBLIBRARY="$withval/lib/libdb.a"
		#DBLIBRARY='-ldb'
		AC_DEFINE(USE_DB185_COMPAT,1,[Define if you use DB1.85 compatible API.])
	fi
],[ with_db185_compat=no ])
AM_CONDITIONAL([USE_DB185_COMPAT], [test "$with_db185_compat" != no])

LDADD='../libparser/libgloparser.a ../libutil/libgloutil.a '$DBLIBRARY' ../libglibc/libgloglibc.a'
dnl
dnl Use sqlite3 API.
dnl
AC_MSG_CHECKING(for sqlite3 support)
AC_ARG_WITH(sqlite3,
[  --with-sqlite3          use sqlite3 API ],
[
	#LDFLAGS="$LDFLAGS -lsqlite3"
	AC_DEFINE(USE_SQLITE3,1,[Define if you use sqlite3 API.])
	AM_CONDITIONAL(USE_SQLITE3, true)
	AC_MSG_RESULT(yes)
],[
	AM_CONDITIONAL(USE_SQLITE3, false)
	AC_MSG_RESULT(no)
])

dnl
dnl for home-etc support
dnl
AC_MSG_CHECKING(for home-etc support)
AC_ARG_WITH(home-etc,
[  --with-home-etc[[=DIR]]   include home-etc support [[DIR=/usr/local]] ],
[
        if test "$withval" = no; then
                AC_MSG_RESULT(no)
        else
                case "$withval" in
                ''|yes) HOMEETC=/usr/local;;
                *)      HOMEETC="$withval";;
                esac
                if ! test -d "$HOMEETC"; then
                        AC_MSG_ERROR([directory '$HOMEETC' not found.])
                fi
		if ! test -f "$HOMEETC/include/home_etc.h"; then
                        AC_MSG_ERROR([header '$HOMEETC/include/home_etc.h' not found.])
                fi
                AM_CPPFLAGS="$AM_CPPFLAGS -I$HOMEETC/include"
                LDFLAGS="$LDFLAGS -L$HOMEETC/lib -lhome_etc"
                AC_DEFINE(HAVE_HOME_ETC_H,1,[Define if you use home-etc facility.])
                AC_MSG_RESULT([yes, using $HOMEETC])
        fi
],[
        AC_MSG_RESULT(no)
])

dnl
dnl for pread/pwrite support
dnl
AC_MSG_CHECKING(for pread/pwrite support)
AC_ARG_WITH(pread-pwrite,
[  --with-pread-pwrite     include pread/pwrite support for BSD db library ],
[
        if test "$withval" = no; then
                AC_MSG_RESULT(no)
        else
		AC_DEFINE(HAVE_PREAD, 1, [Define to 1 if you have and use pread.])
		AC_DEFINE(HAVE_PWRITE, 1, [Define to 1 if you have and use pwrite.])
                AC_MSG_RESULT([yes])
        fi
],[
        AC_MSG_RESULT(no)
])

dnl
dnl for gtags.conf and function layer plugin parser sample.
dnl
AC_MSG_CHECKING(for exuberant ctags program)
AC_ARG_WITH(exuberant-ctags,
[  --with-exuberant-ctags=PROGRAM  specify Exuberant Ctags program ],
[
	EXUBERANT_CTAGS="$withval"
        if test "$withval" = no; then
                AC_MSG_RESULT(no)
	else
                AC_MSG_RESULT([using $EXUBERANT_CTAGS])
	fi
],
[
	for d in `echo $PATH | sed -e 's/^:/.:/' -e 's/::/:.:/g' -e 's/:$/:./' -e 's/:/ /g'`
	do
		if test -x "$d/ctags" && (sh -c "'$d/ctags' --version" | grep "Exuberant Ctags") > /dev/null 2>&1; then
			withval="$d/ctags"
			break
		fi
	done
	if test "$withval" = ''; then
		AC_MSG_RESULT(no)
	else
		EXUBERANT_CTAGS="$withval"
		AC_MSG_RESULT([using $EXUBERANT_CTAGS])
	fi
])
if test "$EXUBERANT_CTAGS" != '' && test "$EXUBERANT_CTAGS" != no && test -x "$EXUBERANT_CTAGS"; then
	AC_MSG_CHECKING(whether exuberant ctags --gtags works)
	if (sh -c "'$EXUBERANT_CTAGS' --gtags -x /dev/null") > /dev/null 2>&1; then
		AC_DEFINE(USE_TYPE_STRING,1,[Define if ctags --gtags option is available.])
		AC_MSG_RESULT(yes)
	else
		AC_MSG_RESULT(no)
	fi
fi

AC_DEFINE_UNQUOTED(EXUBERANT_CTAGS, "$EXUBERANT_CTAGS", [Exuberant Ctags program.])
AC_SUBST(EXUBERANT_CTAGS)

dnl
dnl for POSIX sort program.
dnl
AC_MSG_CHECKING(for POSIX sort program)
AC_ARG_WITH(posix-sort,
[  --with-posix-sort=PROGRAM  specify POSIX sort program ],
[
	POSIX_SORT="$withval"
        if test "$withval" = no; then
                AC_MSG_RESULT(no)
	else
                AC_MSG_RESULT([using $POSIX_SORT])
	fi
],
[
	for d in `echo $PATH | sed -e 's/^:/.:/' -e 's/::/:.:/g' -e 's/:$/:./' -e 's/:/ /g'`
	do
		if test -x "$d/sort" && (sh -c "'$d/sort' -t ':' -k 1,1" < /dev/null) > /dev/null 2>&1; then
			withval="$d/sort"
			break
		fi
	done
	if test "$withval" = ''; then
		AC_MSG_RESULT(no)
	else
		POSIX_SORT="$withval"
		AC_MSG_RESULT([using $POSIX_SORT])
	fi
])
AC_DEFINE_UNQUOTED(POSIX_SORT, "$POSIX_SORT", [POSIX sort program.])
AC_SUBST(POSIX_SORT)

AC_SUBST(AM_CPPFLAGS)
AC_SUBST(LDADD)
AC_SUBST(LDFLAGS)

dnl
dnl The definition of config variable skip and suffixes.
dnl
dnl DEFAULTSKIP: You need not list files whoes first character of name is '.'.
dnl		Such files are skipped even in case of not being in the list.
dnl
DEFAULTSKIP='HTML/,HTML.pub/,tags,TAGS,ID,y.tab.c,y.tab.h,gtags.files,cscope.files,cscope.out,cscope.po.out,cscope.in.out,SCCS/,RCS/,CVS/,CVSROOT/,{arch}/,autom4te.cache/,*.orig,*.rej,*.bak,*~,#*#,*.swp,*.tmp,*_flymake.*,*_flymake'
DEFAULTLANGMAP='c:.c.h,yacc:.y,asm:.s.S,java:.java,cpp:.c++.cc.hh.cpp.cxx.hxx.hpp.C.H,php:.php.php3.phtml'
DEFAULTINCLUDEFILESUFFIXES='h,hh,hxx,hpp,H,inc.php'
AC_SUBST(DEFAULTSKIP)
AC_SUBST(DEFAULTLANGMAP)
AC_SUBST(DEFAULTINCLUDEFILESUFFIXES)

dnl
dnl Quoted langmap & prefix. It is only for gtags.conf.
dnl
DEFAULTLANGMAP_QUOTED=`echo $DEFAULTLANGMAP | sed 's/:/\\\\:/g'`
case "$prefix" in
NONE)   prefix_QUOTED=`echo $ac_default_prefix | sed 's/:/\\\\:/g'`;;
*)      prefix_QUOTED=`echo $prefix | sed 's/:/\\\\:/g'`;;
esac
AC_SUBST(DEFAULTLANGMAP_QUOTED)
AC_SUBST(prefix_QUOTED)

AC_CONFIG_FILES([Makefile
	gtags.conf
	Doxyfile
	libutil/langmap.h
	htags/global.cgi.tmpl
	htags/completion.cgi.tmpl
	libutil/Makefile
	gtags/Makefile
	htags/Makefile
	libdb/Makefile
	global/Makefile
	gozilla/Makefile
	gtags-cscope/Makefile
	globash/Makefile
	htags-refkit/Makefile
	htags-server/Makefile
	libglibc/Makefile
	doc/Makefile
	icons/Makefile
	jquery/Makefile
	jquery/images/Makefile
	script/Makefile
	libparser/Makefile
	plugin-factory/Makefile
	plugin-factory/pygments_parser.py
	libltdl/Makefile
])
AC_OUTPUT
