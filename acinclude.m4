# Local additions to Autoconf macros.
#
# Copyright (c) 2000
#        Tama Communications Corporation. All rights reserved.
#
# This file is part of GNU GLOBAL.
#
# GNU GLOBAL is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# GNU GLOBAL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
AC_DEFUN(AG_STRUCT_DP_D_NAMLEN,
[AC_CACHE_CHECK([for dp_d_namlen in struct dirent], ac_cv_struct_dp_d_namlen,
[AC_TRY_COMPILE([#include <sys/types.h>
#include <dirent.h>], [struct dirent s; s.d_namlen;],
ac_cv_struct_dp_d_namlen=yes, ac_cv_struct_dp_d_namlen=no)])
if test $ac_cv_struct_dp_d_namlen = yes; then
  AC_DEFINE(HAVE_DP_D_NAMLEN)
fi
])

AC_DEFUN(AG_STRUCT_DP_D_TYPE,
[AC_CACHE_CHECK([for dp_d_type in struct dirent], ac_cv_struct_dp_d_type,
[AC_TRY_COMPILE([#include <sys/types.h>
#include <dirent.h>], [struct dirent s; s.d_type;],
ac_cv_struct_dp_d_type=yes, ac_cv_struct_dp_d_type=no)])
if test $ac_cv_struct_dp_d_type = yes; then
  AC_DEFINE(HAVE_DP_D_TYPE)
fi
])
