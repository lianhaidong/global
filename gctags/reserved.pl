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
$RANGE		= 1000;
$START_WORD	= 1001;
$START_VARIABLE	= $START_WORD + $RANGE;
$com = $0;
$com =~ s/.*\///;

sub usage {
	print STDERR "usage: $com --prefix=prefix --option\n";
	print STDERR "       $com --prefix=prefix --perl keyword_file\n";
	print STDERR "       $com --prefix=prefix keyword_file\n";
	exit(1);
}

$slot_name = 'name';
$option = 0;
$perl = 0;
$prefix = '';
$keyword_file = '';
while ($ARGV[0] =~ /^-/) {
	$opt = shift;
	if ($opt =~ /^--prefix=(.*)$/) {
		$prefix = $1;
	} elsif ($opt =~ /^--option$/) {
		$option = 1;
	} elsif ($opt =~ /^--perl$/) {
		$perl = 1;
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
	print "--key-positions=1-3,6,\$\n";
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
if ($perl) {
	open(IP, $keyword_file) || die("$com: cannot open file '$keyword_file'.\n");
	print "# This part is generated automatically by $com from '$keyword_file'.\n";
	print "\$'${prefix}_reserved_words = \"(";
	$first = 1;
	while(<IP>) {
		chop;
		next if (/^$/ || /^#/);
		($id, $type) = split;
		if ($type eq 'word') {
			if ($first) {
				$first = 0;
			} else {
				print '|';
			}
			print $id;
			if ($id !~ /^_/ && $prefix eq 'php') {
				$upper = $id;
				$upper =~ tr/a-z/A-Z/;
				$cap = substr($id, 0, 1);
				$cap =~ tr/a-z/A-Z/;
				$cap .= substr($id, 1);
				print '|', $upper, '|', $cap;
			}
		}
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
$i_word = $START_WORD;
$i_variable = $START_VARIABLE;
print "#define RANGE\t$RANGE\n";
print "#define START_WORD\t$i_word\n";
print "#define START_VARIABLE\t$i_variable\n\n";
while(<IP>) {
	chop;
	next if (/^$/ || /^#/);
	($id, $type) = split;
	if ($type eq 'word') {
		$upper = $id;
		$upper =~ tr/a-z/A-Z/;
		print "#define ${PRE}_${upper}\t${i_word}\n";
		$i_word++;
	} elsif ($type eq 'variable') {
		print "#define ${PRE}_${id}\t${i_variable}\n";
		$i_variable++;
	}
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
	chop;
	next if (/^$/ || /^#/);
	($id, $type) = split;
	if ($type eq 'word') {
		$upper = $id;
		$upper =~ tr/a-z/A-Z/;
		print "$id, ${PRE}_${upper}\n";
		if ($id !~ /^_/ && $prefix eq 'php') {
			$cap = substr($id, 0, 1);
			$cap =~ tr/a-z/A-Z/;
			$cap .= substr($id, 1);
			print "${upper}, ${PRE}_${upper}\n";
			print "${cap}, ${PRE}_${upper}\n";
		}
	} elsif ($type eq 'variable') {
		print "${id}, ${PRE}_${id}\n";
	}
}
close(IP);
print "%%\n";
print "static int\n";
print "reserved_word(str, len)\n";
print "const char *str;\n";
print "int len;\n";
print "{\n";
print "\tstruct keyword *keyword = ${pre}_lookup(str, len);\n";
print "\tint n = keyword ? keyword->token : 0;\n";
print "\treturn (n >= START_WORD && n < START_WORD + RANGE) ? n : 0;\n";
print "}\n";
if ($i_variable > $START_VARIABLE) {
	print "static int\n";
	print "reserved_variable(str, len)\n";
	print "const char *str;\n";
	print "int len;\n";
	print "{\n";
	print "\tstruct keyword *keyword = ${pre}_lookup(str, len);\n";
	print "\tint n = keyword ? keyword->token : 0;\n";
	print "\treturn (n >= START_VARIABLE && n < START_VARIABLE + RANGE) ? n : 0;\n";
	print "}\n";
}
exit 0;
