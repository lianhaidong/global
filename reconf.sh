#!/bin/sh
#
# Copyright (c) 2001, 2003 Tama Communications Corporation
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
# Usage:
#
#	% sh reconf.sh [--configure|--make|--install]
#
case $1 in
--help)	echo "Usage: sh reconf.sh [--configure|--make|--install]"
	exit 0;;
esac
prog='autoreconf flex gperf'	# required programs
file='convert.pl configure.ac Makefile.am gctags/reserved.pl'	# required files

echo "- File existent checking..."
for f in `echo $file`; do
	if [ ! -f $f ]; then
		echo "*** File '$f' not found."
		echo "You must execute this command at the root of GLOBAL source directory."
		exit 1
	fi
	echo "+ $f"
done

echo "- Program existent checking..."
for p in `echo $prog`; do
	found=0
	for d in `echo $PATH | sed -e 's/^:/.:/' -e 's/::/:.:/g' -e 's/:$/:./' -e 's/:/ /g'`
	do
		if [ -x $d/$p ]; then
			#echo "Found at $d/$p."
			found=1
			echo "+ $d/$p"
			break
		fi
	done
	case $found in
	0)	echo "*** Program '$p' not found."
		echo "Please install automake and autoconf."
		exit 1;;
	esac
done

#
# We should do this before packaging so that user can build it without
# flex and gperf.
#
echo "- Preparing parser source ..."
(cd gctags; set -x
perl ./reserved.pl --prefix=sharp --perl c_res.in >htags_res.pl
for lang in c cpp java php; do
	name=${lang}_res
	perl ./reserved.pl --prefix=$lang --perl ${name}.in >>htags_res.pl
	perl ./reserved.pl --prefix=$lang ${lang}_res.in > ${name}.gpf
	option=`perl ./reserved.pl --prefix=$lang --option`
	gperf $option < ${name}.gpf > ${name}.h
	if [ -f $lang.l ]; then
		flex -o$lang.c $lang.l
	fi
done
)

echo "- Collecting reference manuals ..."
commands="global gtags htags gctags gozilla";
perl ./convert.pl --menu $commands > doc/reference.txi
for d in `echo $commands`; do
	perl ./convert.pl --info $d/manual.in > doc/$d.ref
	echo "+ doc/$d.ref"
	perl ./convert.pl --man  $d/manual.in > $d/$d.1
	echo "+ $d/$d.1"
	if [ $d = 'htags' ]; then
		perl ./convert.pl --perl $d/manual.in > $d/const.pl
		echo "+ $d/const.pl"
	else
		perl ./convert.pl --c $d/manual.in > $d/const.h
		echo "+ $d/const.h"
	fi
done

echo "- Clean up config.cache..."
rm -f config.cache

echo "- Generating configure items..."
(set -x; autoreconf --symlink --force --verbose --install) &&
case $1 in
'')	echo "You are ready to execute ./configure"
	;;
-c|--configure|--make|--install)
	./configure
	;;
esac && case $1 in
--make)	make
	;;
--install)
	make install
	;;
esac
