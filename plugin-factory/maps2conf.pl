#!/usr/bin/perl
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
