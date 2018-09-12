#!/usr/bin/env perl
#
# Copyright (c) 2017 Tama Communications Corporation
#
# This file is part of GNU GLOBAL.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# maps2conf.pl: Generates langmaps which is suitable for gtags.conf.
#
# [usage] maps2conf.pl <ctags-path> <plugin-path> [<label>]
#
# $ maps2conf.pl /usr/local/bin/ctags '$libdir/gtags/universal-ctags.la'
#
my $count = $#ARGV + 1;
if ($count < 2) {
	die("usage: maps2conf.pl ctags-path plugin-path [label]\n");
}
my $ctags = $ARGV[0];
my $plugin = $ARGV[1];
my $label = $ARGV[2];
if ($label eq '') {
	$label = 'default';
}
open(IN, "-|") || exec "$ctags", '--list-maps';
if ($?) {
	die("ctags --list-maps failed.\n");
}
my $map = $parser = '';
$map = "$label:\\\n";
$map .= "\t:ctagscom=$ctags:\\\n";
$map .= "\t:ctagslib=$plugin:\\\n";
while (<IN>) {
	chop;
	@words = split;
	$count = $#words + 1;
	if ($count < 2) {
		next;
	}
	$language = shift @words;
	$map .= "\t:langmap=$language\\:";
	foreach $exp (@words) {
		#
		# C++, C#, 
		#
		if ($exp =~ /^\*\.[a-zA-Z_0-9#+-]+$/) {
			$exp =~ s/^\*\./\./;
		} else {
			$exp = '(' . $exp . ')';
		}
		$map .= $exp;
	}
	$map .= ":\\\n";
	$parser .= "\t:gtags_parser=$language\\:\$ctagslib:\\\n";
}
if (length($parser) > 1) {
	chop($parser);
	chop($parser);
	$parser .= "\n";
}
close(IN);
print $map;
print $parser;
