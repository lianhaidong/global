#!/usr/bin/perl
#
# Copyright (c) 2003 Tama Communications Corporation
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
# This command does one of the followings:
#
# [1] Generate option string for gperf(1).
# [2] Generate regular expression which means reserved words for htags(1).
# [3] Generate gperf(1) source from keyword file.
#
# Both output is stdout.
#
$START_RESERVED	= 1001;
$com = $0;
$com =~ s/.*\///;

sub usage {
	print STDERR "usage: $com --prefix=prefix --option\n";
	print STDERR "       $com --prefix=prefix --regex keyword_file\n";
	print STDERR "       $com --prefix=prefix keyword_file\n";
	exit(1);
}

$slot_name = 'name';
$option = 0;
$regex = 0;
$prefix = '';
$keyword_file = '';
while ($ARGV[0] =~ /^-/) {
	$opt = shift;
	if ($opt =~ /^--prefix=(.*)$/) {
		$prefix = $1;
	} elsif ($opt =~ /^--option$/) {
		$option = 1;
	} elsif ($opt =~ /^--regex$/) {
		$regex = 1;
	} else {
		usage();
	}
}
$keyword_file = $ARGV[0];
if (!$prefix) {
	usage();
}
#
# [1] Generate option string for gperf(1).
#
if ($option) {
	print "--key-positions=1,3,\$\n";
	print "--language=C\n";
	print "--struct-type\n";
	print "--slot-name=${slot_name}\n";
	print "--hash-fn-name=${prefix}_hash\n";
	print "--lookup-fn-name=${prefix}_lookup\n";
	exit(0);
}
if (!$keyword_file) {
	usage();
}
#
# [2] Generate regular expression which means reserved words for htags(1).
#
if ($regex) {
	open(IP, $keyword_file) || die("$com: cannot open file '$keyword_file'.\n");
	print "# This part is generated automatically by $com from '$keyword_file'.\n";
	print "\$'${prefix}_reserved_words = \"(";
	$first = 1;
	while(<IP>) {
		next if (/^$/ || /^#/);
		chop;
		if ($first) {
			$first = 0;
		} else {
			print '|';
		}
		print;
	}
	print ")\";\n";
	print "# end of generated part.\n";
	close(IP);
	exit(0);
}
#
# [3] Generate gperf(1) source from keyword file.
#
$PRE = $pre = $prefix;
$PRE =~ tr/a-z/A-Z/;
$pre =~ tr/A-Z/a-z/;

#
# Macro definitions.
#
open(IP, $keyword_file) || die("$com: cannot open file '$keyword_file'.\n");
print "%{\n";
$id = $START_RESERVED;
while(<IP>) {
	next if (/^$/ || /^#/);
	chop;
	$upper = $_;
	$upper =~ tr/a-z/A-Z/;
	print "#define ${PRE}_${upper}\t${id}\n";
	$id++;
}
close(IP);
print "%}\n";
#
# Structure definition.
#
print "struct keyword { char *${slot_name}; int token; }\n";
print "%%\n";
#
# Keyword definitions.
#
open(IP, $keyword_file) || die("$com: cannot open file '$keyword_file'.\n");
while(<IP>) {
	next if (/^$/ || /^#/);
	chop;
	$upper = $_;
	$upper =~ tr/a-z/A-Z/;
	print "$_, ${PRE}_${upper}\n";
}
close(IP);
print "%%\n";
print "static int\n";
print "reserved(str, len)\n";
print "const char *str;\n";
print "int len;\n";
print "{\n";
print "\tstruct keyword *keyword = ${pre}_lookup(str, len);\n";
print "\treturn keyword ? keyword->token : 0;\n";
print "}\n";
exit 0;
