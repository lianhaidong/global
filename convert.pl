#!/usr/bin/perl
#
# Copyright (c) 2001 Tama Communications Corporation
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
# Main procedure.
#
$com = $0;
$com =~ s/.*\///;
while ($ARGV[0] =~ /^-/) {
        $opt = shift;
        if ($opt eq '--c') {
                $c = 1;
        } elsif ($opt eq '--perl') {
                $perl = 1;
        } elsif ($opt eq '--man') {
                $man = 1;
	} elsif ($opt eq '--info') {
                $info = 1;
	} elsif ($opt eq '--menu') {
                $menu = 1;
	}
}
if ($menu) {
	&menu'gen(@ARGV);
} else {
	$infile = $ARGV[0];
	open(INFILE, $infile) || die("cannot open '$infile'\n");
	if ($c) {
		&c'gen();
	} elsif ($perl) {
		&perl'gen();
	} elsif ($man) {
		&man'gen();
	} elsif ($info) {
		&info'gen();
	}
	close(INFILE);
}
exit 0;
#------------------------------------------------------------------
#
# Generate langmap statement.
#
#------------------------------------------------------------------
#
# Get a value from configure.ac file by name.
#
sub getvalue {
	my($name) = @_;
	my $value;
	#
	# A value should be defined as:
	# NAME='VALUE'
	#
	my $line=`grep '$name=' configure.ac`;
	($value) = $line =~ /^$name='(.*)'$/;
	unless ($value) {
		print STDERR "$name not found.\n";
		exit(1);
	}
	$value;
}
#
# Generate statements about the langmap variable.
#
sub langmapstatement {
	my($maps) = getvalue('DEFAULTLANGMAP');
	my(@statement);
	my %name = (
		'c'	=> 'C',
		'yacc'	=> 'yacc',
		'asm'	=> 'Assembly',
		'java'	=> 'Java',
		'cpp'	=> 'C++',
		'php'	=> 'PHP',
	);
	my $line = '';
	my @maps = split(/,/, $maps);
	for ($i = 0; $i < @maps; $i++) {
		$_ = $maps[$i];
		my($lang, $suffixes) = /([^:]+):(.*)/;
		if ($i > 0) {
			if ($i + 1 == @maps) {		# last
				$line .= ' and ';
			} else {
				$line .= ', ';
			}
		}
		$line .= $name{$lang};
	}
	$line .= " source files are supported.\n";
	unshift(@statement, $line);
	foreach (@maps) {
		my($lang, $suffixes) = /([^:]+):(.*)/;
		unless ($name{$lang}) {
			print STDERR "ERROR: $lang not defined.\n";
			exit(1);
		}
		$suffixes =~ s/^\.//;
		my(@suffixes) = split(/\./, $suffixes);
		$line = 'Files whose names end in ';
		for ($i = 0; $i < @suffixes; $i++) {
			my $sx = $suffixes[$i];
			$line .= ', ' if ($i > 0);
			$line .= "\@file{.$sx}";
		}
		$line .= " are assumed to be $name{$lang} source files.\n";
		unshift(@statement, $line);
	}
	@statement;
}
#------------------------------------------------------------------
#
# Read line.
#
#------------------------------------------------------------------
@lines = ();
sub getline {
	if (@lines > 0) {
		$_ = pop(@lines);
	} else {
		while (<INFILE>) {
			if (/\@LANGMAPSTATEMENT\@/) {
				@lines = &'langmapstatement();
				$_ = pop(@lines);
			} elsif (/\@DEFAULTLANGMAP\@/) {
				my $value = &'getvalue('DEFAULTLANGMAP');
				s/\@DEFAULTLANGMAP\@/$value/;
			} elsif (/\@DEFAULTINCLUDEFILESUFFIXES\@/) {
				my $value = &'getvalue('DEFAULTINCLUDEFILESUFFIXES');
				s/\@DEFAULTINCLUDEFILESUFFIXES\@/$value/;
			}
			last unless (/^#/);
		}
	}
	($_) ? 1 : 0;
}
sub ungetline {
	push(@lines, $_);
}
#------------------------------------------------------------------
#
# C package.
#
#------------------------------------------------------------------
package c;
sub convert {
	local($arg) = '[^},]+';
	local($macros) = 'arg|code|var|file|name|option';
	chop;
	s/^\s+//;
	while (s/\@($macros)\{($arg)\}/$2/) {
		;
	}
	while (s/\@xref\{($arg),($arg)\}/$1($2)/) {
		;
	}
}
sub gen {
	local($help_const) = 0;
	print "/* This file is generated automatically by $'com from $'infile. */\n";
	while (&'getline()) {
		if (/^\@NAME\s+(.*)\s+-/) {
			print "const char *progname = \"$1\";\n";
		} elsif (/^\@SYNOPSIS$/) {
			local($first) = 1;
			print "const char *usage_const = \"";
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				}
				convert();
				if ($first) {
					$first = 0;
					print "Usage: ";
				} else {
					print "\\n\\\n";
					print "       ";
				}
				print $_;
			}
			print "\\n\";\n";
		} elsif (/^\@COMMANDS$/ || /^\@OPTIONS$/) {
			if (!$help_const) {
				$help_const = 1;
				print "const char *help_const = \"";
			}
			if (/^\@COMMANDS$/) {
				print "Commands:";
			} else {
				print "Options:";
			}
			print "\\n\\\n";
			local($itemize) = 0;
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				}
				convert();
				if (/^\@begin_itemize$/) {
					$itemize = 1;
				} elsif (/^\@end_itemize$/) {
					$itemize = 0;
				} elsif ($itemize) {
					if (/^\@item\{(.*)\}$/) {
						print $1;
					} else {
						print "       ";
						print;
					}
					print "\\n\\\n";
				}
			}
		}
	}
	print "See also:\\n\\\n";
	print "       GNU GLOBAL web site: http://www.gnu.org/software/global/\\n\\\n";
	print "\";\n";
}
#------------------------------------------------------------------
#
# Man package.
#
#------------------------------------------------------------------
package man;
sub convert {
	local($arg) = '[^},]+';
	s/^\s+//;
	while (s/\@arg\{($arg)\}/\\fI$1\\fP/) {
		;
	}
	while (/\@code\{($arg)\}/) {
		$val = $1;
		$val =~ s/\./\\./g;
		s/\@code\{$arg\}/$val/;
	}
	while (s/\@file\{($arg)\}/\\'$1\\'/) {
		;
	}
	while (s/\@(var|name|option)\{($arg)\}/\\fB$2\\fP/) {
		;
	}
	while (s/\@xref\{($arg),($arg)\}/\\fB$1\\fP($2)/) {
		;
	}
	s/\@{/{/g;
	s/\@}/}/g;
	s/\@br$/\n.br/;
}
sub gen {
	local($arg) = '[^,]+';
	print ".\\\" This file is generated automatically by $'com from $'infile.\n";
	while (&'getline()) {
		if (/^\@HEADER\s+($arg),($arg),($arg),($arg)\n$/) {
			print ".TH $1 $2 \"$3\" \"$4\"\n";
		} elsif (/^\@NAME\s+(.*)$/) {
			local($name) = $1;
			$name =~ s/-/\\-/;
			print ".SH NAME\n$name\n";
		} elsif (/^\@(SYNOPSIS)$/) {
			print ".SH $1\n";
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				} elsif (/^$/) {
					print ".PP\n";
					next;
				}
				convert();
				print;
				print ".br\n";
			}
		} elsif (/^\@(.*)$/) {
			$type = $1;
			if ($type =~ /\s+/) {
				$type = "\"$type\"";
			}
			print ".SH $type\n";
			local($itemize) = 0;
			local($verbatim) = 0;
			while (&'getline()) {
				if (/^\s*\@begin_verbatim\s*$/) {
					$verbatim = 1;
					print ".nf\n";
					next;
				} elsif (/^\s*\@end_verbatim\s*$/) {
					$verbatim = 0;
					print ".fi\n";
					next;
				} elsif (/^\@/) {
					&'ungetline();
					last;
				} elsif (/^$/) {
					print ".PP\n";
					next;
				}
				convert();
				if (/^\@begin_itemize$/) {
					$itemize = 1;
				} elsif (/^\@end_itemize$/) {
					$itemize = 0;
				} elsif ($itemize && /^\@item\{(.*)\}$/) {
					print ".TP\n$1\n";
				} else {
					print;
				}
			}
		}
	}
}
#------------------------------------------------------------------
#
# Info package.
#
#------------------------------------------------------------------
package info;
sub convert {
	local($arg) = '[^},]+';
	chop;
	s/^\s+//;
	while (s/\@xref\{($arg),($arg)\}/$1($2)/) {
		;
	}
	s/\@option\{/\@samp\{/g;
	while (s/\@(arg|name)\{($arg)\}/$2/) {
		;
	}
	s/\@br$/\@\*/;
}
sub gen {
	print "\@c This file is generated automatically by $'com from $'infile.\n";
	while (&'getline()) {
		if (/^\@HEADER/) {
			;
		} elsif (/^\@(NAME)\s+(.*)$/) {
			local($name) = $2;
			print "\@unnumberedsubsec $1\n";
			print "$name\n";
		} elsif (/^\@(SYNOPSIS)$/) {
			print "\@unnumberedsubsec $1\n";
			print "\@quotation\n";
			while (&'getline()) {
				if (/^\@/) {
					&'ungetline();
					last;
				} elsif (/^$/) {
					print;
					next;
				}
				convert();
				print;
				print "\@*\n";
			}
			print "\@end quotation\n";
		} elsif (/^\@(.*)$/) {
			$type = $1;
			print "\@unnumberedsubsec $type\n";
			local($itemize) = 0;
			local($verbatim) = 0;
			while (&'getline()) {
				if (/^\s*\@begin_verbatim\s*$/) {
					$verbatim = 1;
					print "\@example\n";
					next;
				} elsif (/^\s*\@end_verbatim\s*$/) {
					$verbatim = 0;
					print "\@end example\n";
					next;
				} elsif ($verbatim) {
					s/\{/@\{/g;
					s/\}/@\}/g;
					print;
					next;
				} elsif (/^\@/) {
					&'ungetline();
					last;
				} elsif (/^$/) {
					print;
					next;
				}
				convert();
				if (/^\@begin_itemize$/) {
					$itemize = 1;
					print "\@table \@asis\n";
				} elsif (/^\@end_itemize$/) {
					$itemize = 0;
					print "\@end table\n";
				} elsif ($itemize && /^\@item\{(.*)\}$/) {
					print "\@item $1\n";
				} else {
					print "$_\n";
				}
			}
		}
	}
}
#------------------------------------------------------------------
#
# Menu package.
#
#------------------------------------------------------------------
package menu;
sub gen {
	local(@args) = @_;
	local($line, $name);
	foreach $dir (@args) {
		$infile = "$dir/manual.in";
		open(INFILE, $infile) || die("cannot open '$infile'\n");
		while (<INFILE>) {
			if (/^\@NAME\s+((\S+)\s+\-\s+.*)$/) {
				$line = $1;
				$name = $2;
				last;
			}
		}
		close(INFILE);
		push(@name, $name);
		push(@line, $line);
	}
	print "\@c This file is generated automatically by $'com from a set of manual.in.\n";
	print "\@menu\n";
	for ($i = 0; $i < @name; $i++) {
		$name = $name[$i];
		$line = $line[$i];
		$name .= '::';
		printf "* %-16s %s\n", $name, $line;
	}
	print "\@end menu\n\n";
	for ($i = 0; $i < @name; $i++) {
		$name = $name[$i];
		$line = $line[$i];
		printf "\@node %s\n", $name;
		printf "\@section %s\n", $line;
		printf "\@include $name.ref\n";
	}
}
