#! @PERL@
#
# Copyright (c) 1996, 1997, 1998, 1999
#             Shigio Yamaguchi. All rights reserved.
# Copyright (c) 1999, 2000
#             Tama Communications Corporation. All rights reserved.
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
$com = $0;
$com =~ s/.*\///;
$usage = "Usage: $com [-a][-c][-f][-F][-l][-n][-v][-w][-d tagdir][-S cgidir][-t title][dir]";
$help = <<END_OF_HELP;
$usage
Options:
     -a, --alphabet
             make an alphabetical definition index.
     -c, --compact
             compress html files by gzip(1).
     -f, --form
             support an input form and a dynamic index with a CGI program.
     -F, --frame
             use frame for index and main view.
     -l, --each-line-tag
             make a name tag(<A NAME=line number>) for each line.
     -n, --line-number
             print the line numbers.
     -v, --verbose
             verbose mode.
     -w, --warning
             print warning messages.
     -d, --dbpath tagdir
             specifies the directory in which GTAGS and GRTAGS exist.
     -S, --secure-cgi cgidir
             write cgi script into cgidir to realize a centralised cgi script.
     -t, --title title
             the title of this hypertext.
     --version
             print version.
     --help
             print help message.
     dir     the directory in which hypertext is generated.
END_OF_HELP
$version = `global --version`;
chop($version);

$'w32 = ($^O =~ /^ms(dos|win(32|nt))/i) ? 1 : 0;
#-------------------------------------------------------------------------
# COMMAND EXISTENCE CHECK
#-------------------------------------------------------------------------
foreach $c ('sort', 'gtags', 'global', 'btreeop') {
	if (!&'usable($c)) {
		&'error("'$c' command is required but not found.");
	}
}
#-------------------------------------------------------------------------
# CONFIGURATION
#-------------------------------------------------------------------------
# temporary directory
$'tmp = '/tmp';
if (defined($ENV{'TMPDIR'}) && -d $ENV{'TMPDIR'}) {
	$tmp = $ENV{'TMPDIR'};
}
if (! -d $tmp || ! -w $tmp) {
	&'error("temporary directory '$tmp' not exist or not writable.");
}
$'ncol = 4;					# columns of line number
$'tabs = 8;					# tab skip
$'full_path = 0;				# file index format
$'icon_list = '';				# use icon for file index
$'prolog_script = '';				# include script at first
$'epilog_script = '';				# include script at last
$'show_position = 0;				# show current position
$'table_list = 0;				# tag list using table tag
$'script_alias = '/cgi-bin';			# script alias of WWW server
$'gzipped_suffix = 'ghtml';			# suffix of gzipped html file
$'normal_suffix = 'html';			# suffix of normal html file
$'action = 'cgi-bin/global.cgi';		# default action
$'id = '';					# id (default non)
$'cgi = 1;					# 1: make cgi-bin/
#
# tag
#
$'body_begin     = '<BODY>';
$'body_end       = '</BODY>';
$'table_begin    = '<TABLE>';
$'table_end      = '</TABLE>';
$'title_begin	 = '<FONT COLOR=#cc0000>';
$'title_end	 = '</FONT>';
$'comment_begin  = '<I><FONT COLOR=green>';	# /* ... */
$'comment_end    = '</FONT></I>';
$'sharp_begin    = '<FONT COLOR=darkred>';	# #define, #include or so on
$'sharp_end      = '</FONT>';
$'brace_begin    = '<FONT COLOR=blue>';		# { ... }
$'brace_end      = '</FONT>';
$'reserved_begin = '<B>';			# if, while, for or so on
$'reserved_end   = '</B>';
$'position_begin = '<FONT COLOR=gray>';
$'position_end   = '</FONT>';
#
# Reserved words for C and Java are hard coded.
# (configuration parameter 'reserved_words' was deleted.)
#
$'c_reserved_words =	"auto,break,case,char,continue,default,do,double,else," .
		"extern,float,for,goto,if,int,long,register,return," .
		"short,sizeof,static,struct,switch,typedef,union," .
		"unsigned,void,while";
$'cpp_reserved_words =	"catch,class,delete,enum,friend,inline,new,operator," .
		"private,protected,public,template,this,throw,try," .
		"virtual,volatile" .
		$'c_reserved_words;
$'java_reserved_words  = "abstract,boolean,break,byte,case,catch,char,class," .
		"const,continue,default,do,double,else,extends,false," .
		"final,finally,float,for,goto,if,implements,import," .
		"instanceof,int,interface,long,native,new,null," .
		"package,private,protected,public,return,short," .
		"static,super,switch,synchronized,this,throw,throws," .
		"union,transient,true,try,void,volatile,while";
$'c_reserved_words    =~ s/,/|/g;
$'cpp_reserved_words  =~ s/,/|/g;
$'java_reserved_words =~ s/,/|/g;
#
# read values from global.conf
#
chop($config = `gtags --config`);
if ($config) {
if ($var1 = &'getconf('ncol')) {
	if ($var1 < 1 || $var1 > 10) {
		print STDERR "Warning: parameter 'ncol' ignored becase the value is too large or too small.\n";
	} else {
		$'ncol = $var1;
	}
}
if ($var1 = &'getconf('tabs')) {
	if ($var1 < 1 || $var1 > 32) {
		print STDERR "Warning: parameter 'tabs' ignored becase the value is too large or too small.\n";
	} else {
		$'tabs = $var1;
	}
}
if ($var1 = &'getconf('gzipped_suffix')) {
	$'gzipped_suffix = $var1;
}
if ($var1 = &'getconf('normal_suffix')) {
	$'normal_suffix = $var1;
}
if ($var1 = &'getconf('full_path')) {
	$'full_path = $var1;
}
if ($var1 = &'getconf('table_list')) {
	$'table_list = $var1;
}
if ($var1 = &'getconf('icon_list')) {
	$'icon_list = $var1;
}
if ($var1 = &'getconf('prolog_script')) {
	$'prolog_script = $var1;
}
if ($var1 = &'getconf('epilog_script')) {
	$'epilog_script = $var1;
}
if ($var1 = &'getconf('show_position')) {
	$'show_position = $var1;
}
if ($var1 = &'getconf('script_alias')) {
	$'script_alias = $var1;
}
if (($var1 = &'getconf('body_begin')) && ($var2 = &'getconf('body_end'))) {
	$'body_begin  = $var1;
	$'body_end    = $var2;
}
if (($var1 = &'getconf('table_begin')) && ($var2 = &'getconf('table_end'))) {
	$'table_begin  = $var1;
	$'table_end    = $var2;
}
if (($var1 = &'getconf('title_begin')) && ($var2 = &'getconf('title_end'))) {
	$'title_begin  = $var1;
	$'title_end    = $var2;
}
if (($var1 = &'getconf('comment_begin')) && ($var2 = &'getconf('comment_end'))) {
	$'comment_begin  = $var1;
	$'comment_end    = $var2;
}
if (($var1 = &'getconf('sharp_begin')) && ($var2 = &'getconf('sharp_end'))) {
	$'sharp_begin  = $var1;
	$'sharp_end    = $var2;
}
if (($var1 = &'getconf('brace_begin')) && ($var2 = &'getconf('brace_end'))) {
	$'brace_begin  = $var1;
	$'brace_end    = $var2;
}
if (($var1 = &'getconf('reserved_begin')) && ($var2 = &'getconf('reserved_end'))) {
	$'reserved_begin  = $var1;
	$'reserved_end    = $var2;
}
if (($var1 = &'getconf('position_begin')) && ($var2 = &'getconf('position_end'))) {
	$'position_begin  = $var1;
	$'position_end    = $var2;
}
}
# HTML tag
$'html_begin  = '<HTML>';
$'html_end    = '</HTML>';
$'meta_robots = "<META NAME='ROBOTS' CONTENT='NOINDEX,NOFOLLOW'>";
$'meta_generator = "<META NAME='GENERATOR' CONTENT='GLOBAL-$version'>";
# Titles
$'title_define_index = 'DEFINITIONS';
$'title_file_index = 'FILES';

# Anchor image
@anchor_label = ('&lt;', '&gt;', '^', 'v', 'top', 'bottom', 'index', 'help');
@anchor_icons = ('left.jpg', 'right.jpg', 'first.jpg', 'last.jpg', 'top.jpg', 'bottom.jpg', 'index.jpg', 'help.jpg');
@anchor_comment = ('previous', 'next', 'first', 'last', 'top', 'bottom', 'index', 'help');
$back_icon = 'back.jpg';
$dir_icon  = 'dir.jpg';
$file_icon = 'c.jpg';
@anchor_msg   = ('Previous definition.',
		'Next definition.',
		'First definition in this file.',
		'Last definition in this file.',
		'Top of this file.',
		'Bottom of this file.',
		'Return to index page.',
		'You are seeing now.',
);
#-------------------------------------------------------------------------
# JAVASCRIPT PARTS
#-------------------------------------------------------------------------
$'begin_script="<SCRIPT LANGUAGE=javascript>\n<!--\n";
$'end_script="<!-- end of script -->\n</SCRIPT>\n";
# escaped angle
$'langle  = sprintf("unescape('%s')", &'escape('<'));
$'rangle  = sprintf("unescape('%s')", &'escape('>'));
# staus line
$'status_line  =
"function show(type, lno, file) {\n" .
"	if (lno > 0) {\n" .
"		msg = (type == 'R') ? 'Defined at' : 'Refered from';\n" .
"		msg += ' ' + lno;\n" .
"		if (file != '')\n" .
"			msg += ' in ' + file;\n" .
"	} else {\n" .
"		msg = 'Multiple ';\n" .
"		msg += (type == 'R') ? 'defined' : 'refered';\n" .
"	}\n" .
"	msg += '.';\n" .
"	self.status = msg;\n" .
"}\n";
#-------------------------------------------------------------------------
# DEFINITION
#-------------------------------------------------------------------------
# unit for a path
$'SRCS   = 'S';
$'DEFS   = 'D';
$'REFS   = 'R';
$'INCS   = 'I';
sub set_header {
	local($title) = @_;
	local($head) = '';
	$head .= "<HEAD>\n";
	$head .= "<TITLE>$title</TITLE>\n";
	$head .= "$'meta_robots\n$'meta_generator\n";
	$head .= $'begin_script;
	$head .= "self.defaultStatus = '$title'\n";
	$head .= $'status_line;
	$head .= $'end_script;	
	$head .= "</HEAD>\n";
	$head;
}
#-------------------------------------------------------------------------
# UTILITIES
#-------------------------------------------------------------------------
sub getcwd {
        local($dir) = `pwd`;
	if ($w32) { $dir =~ s!\\!/!g; }
        chop($dir);
        $dir;
}
sub realpath {
	local($dir) = @_;
	local($cwd) = &getcwd;
	chdir($dir) || &'error("cannot change directory '$dir'.");
        local($new) = &getcwd;
	chdir($cwd) || &'error("cannot recover current directory '$cwd'.");
	$new;
}
sub date {
	local($date) = `date`;
	chop($date);
	$date;
}
sub error {
	&clean();
	printf STDERR "$com: $_[0]\n";
	exit 1;
}
sub clean {
	&anchor'finish();
	&cache'close();
}
sub escape {
	local($c) = @_;
	'%' . sprintf("%x", ord($c));
}
sub usable {
	local($command) = @_;
	local($pathsep) = ($'w32) ? ';' : ':';
	foreach (split(/$pathsep/, $ENV{'PATH'})) {
		if ($w32) {
			return "$_/$command.com" if (-f "$_/$command.com");
			return "$_/$command.exe" if (-f "$_/$command.exe");
		} else {
			return "$_/$command" if (-x "$_/$command");
		}
	}
	return '';
}
sub duplicatefile {
	local($file, $from, $to) = @_;
	if ($'w32) {
		&'copy("$from/$file", "$to/$file")
			|| &'error("cannot copy $file.");
	} else {
		link("$from/$file", "$to/$file")
			|| &'copy("$from/$file", "$to/$file")
			|| &'error("cannot copy $file.");
	}
}
sub copy {
	local($from, $to) = @_;
	open(FROM, $from) || return 0;
	open(TO, ">$to") || return 0;
	print TO <FROM>;
	close(TO);
	close(FROM);
	return 1;
}
sub getconf {
	local($name) = @_;
	local($val);
	chop($val = `gtags --config $name`);
	if ($? != 0) { $val = ''; }
	$val;
}
sub path2url {
	local($path) = @_;
	$path = './' . $path if ($path !~ /^\./);
	if (!defined($'GPATH{$path})) {
		$'GPATH{$path} = ++$nextkey;
	}
	$'GPATH{$path} . '.' . $'HTML;
}
#-------------------------------------------------------------------------
# LIST PROCEDURE
#-------------------------------------------------------------------------
sub list_begin {
	$'table_list ? "$'table_begin\n<TR ALIGN=center><TH NOWRAP>tag</TH><TH NOWRAP>line</TH><TH NOWRAP>file</TH><TH NOWRAP>source code</TH></TR>\n" :  "<PRE>\n";
}
sub list_body {
	local($srcdir, $s) = @_;	# $s must be choped.
	local($name, $lno, $filename, $line) = ($s =~ /^(\S+)\s+(\d+)\s+\.\/(\S+) (.*)$/);
	local($html) = &'path2url($filename);

	$s =~ s/\.\///;
	$s =~ s/&/&amp;/g;
	$s =~ s/</&lt;/g;
	$s =~ s/>/&gt;/g;
	if ($'table_list) {
		$line =~ s/ /&nbsp;&nbsp;/g;
		$s = "<TR><TD NOWRAP><A HREF=$srcdir\/$html#$lno>$name</A></TD><TD NOWRAP>$lno</TD><TD NOWRAP>$filename</TD><TD NOWRAP>$line</TD></TR>";
	} else {
		$s =~ s/^($name)/<A HREF=$srcdir\/$html#$lno>$1<\/A>/;
	}
	$s . "\n";
}
sub list_end {
	local($s) = $'table_list ? $'table_end : "</PRE>";
	$s . "\n";
}
#-------------------------------------------------------------------------
# PROCESS START
#-------------------------------------------------------------------------
# include prolog_script if needed.
require($'prolog_script) if ($'prolog_script && -f $'prolog_script);
#
# options check.
#
$'aflag = $'cflag = $'fflag = $'Fflag = $'lflag = $'nflag = $'Sflag = $'vflag = $'wflag = '';
$show_version = 0;
$show_help = 0;
$action_value = '';
$id_value = '';
$cgidir = '';
while ($ARGV[0] =~ /^-/) {
	$opt = shift;
	if ($opt =~ /^--action=(.*)$/) {
		$action_value = $1;
	} elsif ($opt =~ /^--id=(.*)$/) {
		$id_value = $1;
	} elsif ($opt =~ /^--nocgi$/) {
		$'cgi = 0;
	} elsif ($opt =~ /^--version$/) {
		$show_version = 1;
	} elsif ($opt =~ /^--help$/) {
		$show_help = 1;
	} elsif ($opt =~ /^--alphabet$/) {
		$'aflag = 'a';
	} elsif ($opt =~ /^--compact$/) {
		$'cflag = 'c';
	} elsif ($opt =~ /^--form$/) {
		$'fflag = 'f';
	} elsif ($opt =~ /^--frame$/) {
		$'Fflag = 'F';
	} elsif ($opt =~ /^--each-line-tag$/) {
		$'lflag = 'l';
	} elsif ($opt =~ /^--line-number$/) {
		$'nflag = 'n';
	} elsif ($opt =~ /^--verbose$/) {
		$'vflag = 'v';
	} elsif ($opt =~ /^--warning$/) {
		$'wflag = 'w';
	} elsif ($opt =~ /^--title$/) {
		$opt = shift;
		last if ($opt eq '');
		$title = $opt;
	} elsif ($opt =~ /^--dbpath$/) {
		$opt = shift;
		last if ($opt eq '');
		$dbpath = $opt;
	} elsif ($opt =~ /^--secure-cgi$/) {
		$'Sflag = 'S';
		$'cgidir = shift;
	} elsif ($opt =~ /^--/) {
		print STDERR $usage, "\n";
		exit 1;
	} elsif ($opt =~ /[^-acdfFlnsStvwtd]/) {
		print STDERR $usage, "\n";
		exit 1;
	} else {
		if ($opt =~ /a/) { $'aflag = 'a'; }
		if ($opt =~ /c/) { $'cflag = 'c'; }
		if ($opt =~ /f/) { $'fflag = 'f'; }
		if ($opt =~ /F/) { $'Fflag = 'F'; }
		if ($opt =~ /l/) { $'lflag = 'l'; }
		if ($opt =~ /n/) { $'nflag = 'n'; }
		if ($opt =~ /v/) { $'vflag = 'v'; }
		if ($opt =~ /w/) { $'wflag = 'w'; }
		if ($opt =~ /t/) {
			$opt = shift;
			last if ($opt eq '');
			$title = $opt;
		} elsif ($opt =~ /d/) {
			$opt = shift;
			last if ($opt eq '');
			$dbpath = $opt;
		} elsif ($opt =~ /S/) {
			$'Sflag = 'S';
			$'cgidir = shift;
		}
	}
}
if ($show_version) {
	$com = 'global --version';
	$com .= ' --verbose' if ($vflag);
	$com .= ' htags';
	system($com);
	exit 0;
}
if ($show_help) {
	print STDOUT $help;
	exit 1;
}
if ($'cflag && !&'usable('gzip')) {
	print STDERR "Warning: 'gzip' command not found. -c option ignored.\n";
	$'cflag = '';
}
if (!$title) {
	@cwd = split('/', &'getcwd);
	$title = $cwd[$#cwd];
}
#
# decide directory in which we make hypertext.
#
$dist = &'getcwd() . '/HTML';
if ($ARGV[0]) {
	$cwd = &'getcwd();
	unless (-w $ARGV[0]) {
		 &'error("'$ARGV[0]' is not writable directory.");
	}
	chdir($ARGV[0]) || &'error("directory '$ARGV[0]' not found.");
	$dist = &'getcwd() . '/HTML';
	chdir($cwd) || &'error("cannot return to original directory.");
}
if ($'Sflag) {
	$script_alias =~ s!/$!!;
	$'action = "$script_alias/global.cgi";
	$'id = $dist;
}
# --action, --id overwrite Sflag's value.
if ($action_value) {
	$'action = $action_value;
}
if ($id_value) {
	$'id = $id_value;
}
# If $dbpath is not specified then listen to global(1).
if (!$dbpath) {
	local($cwd) = &'getcwd();
	local($root) = `global -pr`;
	chop($root);
	if ($cwd eq $root) {
		$dbpath = `global -p`;
		chop($dbpath);
	} else {
		$dbpath = '.';
	}
}
unless (-r "$dbpath/GTAGS" && -r "$dbpath/GRTAGS") {
	&'error("GTAGS and/or GRTAGS not found. Htags needs both of them.");
}
$dbpath = &'realpath($dbpath);
#
# for global(1)
#
$ENV{'GTAGSROOT'} = &'getcwd();
$ENV{'GTAGSDBPATH'} = $dbpath;
delete $ENV{'GTAGSLIBPATH'};
#
# check directories
#
if ($'fflag || $'cflag) {
	if ($'cgidir && ! -d $'cgidir) {
		&'error("'$'cgidir' not found.");
	}
	if (!$'Sflag) {
		$'cgidir = "$dist/cgi-bin";
	}
} else {
	$'Sflag = $'cgidir = '';
}
#
# find filter
#
$'findcom = "gtags --find";
#-------------------------------------------------------------------------
# MAKE FILES
#-------------------------------------------------------------------------
#	HTML/cgi-bin/global.cgi	... CGI program (1)
#	HTML/cgi-bin/ghtml.cgi	... unzip script (1)
#	HTML/.htaccess		... skelton of .htaccess (1)
#	HTML/help.html		... help file (2)
#	HTML/$REFS/*		... references (3)
#	HTML/$DEFS/*		... definitions (3)
#	HTML/defines.html	... definitions index (4)
#	HTML/defines/*		... definitions index (4)
#	HTML/files.html		... file index (5)
#	HTML/files/*		... file index (5)
#	HTML/index.html		... index file (6)
#	HTML/mains.html		... main index (7)
#	HTML/null.html		... main null html (7)
#	HTML/$SRCS/		... source files (8)
#	HTML/$INCS/		... include file index (8)
#	HTML/search.html	... search index (9)
#-------------------------------------------------------------------------
$'HTML = ($'cflag) ? $'gzipped_suffix : $'normal_suffix;
print STDERR "[", &'date, "] ", "Htags started\n" if ($'vflag);
#
# (#) check if GTAGS, GRTAGS is the latest.
#
print STDERR "[", &'date, "] ", "(#) checking tag files ...\n" if ($'vflag);
$gtags_ctime = (stat("$dbpath/GTAGS"))[10];
open(FIND, "$'findcom |") || &'error("cannot fork.");
while (<FIND>) {
	chop;
	if ($gtags_ctime < (stat($_))[10]) {
		&'error("GTAGS is not the latest one. Please remake it.");
	}
}
close(FIND);
if ($?) { &'error("cannot traverse directory."); }
#
# (0) make directories
#
print STDERR "[", &'date, "] ", "(0) making directories ...\n" if ($'vflag);
mkdir($dist, 0777) || &'error("cannot make directory '$dist'.") if (! -d $dist);
foreach $d ($SRCS, $INCS, $DEFS, $REFS, 'files', 'defines') {
	mkdir("$dist/$d", 0775) || &'error("cannot make HTML directory") if (! -d "$dist/$d");
}
if ($'cgi && ($'fflag || $'cflag)) {
	mkdir("$dist/cgi-bin", 0775) || &'error("cannot make cgi-bin directory") if (! -d "$dist/cgi-bin");
}
#
# (1) make CGI program
#
if ($'cgi && $'fflag) {
	if ($'cgidir) {
		print STDERR "[", &'date, "] ", "(1) making CGI program ...\n" if ($'vflag);
		&makeprogram("$cgidir/global.cgi") || &'error("cannot make CGI program.");
		chmod(0755, "$cgidir/global.cgi") || &'error("cannot chmod CGI program.");
	}
	if ($'Sflag) {
		# Don't grant execute permission to bless script.
		&makebless("$dist/bless.sh") || &'error("cannot make bless script.");
		chmod(0644, "$dist/bless.sh") || &'error("cannot chmod bless script.");
	}
	foreach $f ('GTAGS', 'GRTAGS', 'GSYMS', 'GPATH') {
		if (-f "$dbpath/$f") {
			unlink("$dist/cgi-bin/$f");
			&duplicatefile($f, $dbpath, "$dist/cgi-bin");
		}
	}
}
if ($'cgi && $'cflag) {
	&makehtaccess("$dist/.htaccess") || &'error("cannot make .htaccess skelton.");
	chmod(0644, "$dist/.htaccess") || &'error("cannot chmod .htaccess skelton.");
	if ($'cgidir) {
		&makeghtml("$cgidir/ghtml.cgi") || &'error("cannot make unzip script.");
		chmod(0755, "$cgidir/ghtml.cgi") || &'error("cannot chmod unzip script.");
	}
}
#
# (2) make help file
#
print STDERR "[", &'date, "] ", "(2) making help.html ...\n" if ($'vflag);
&makehelp("$dist/help.$'normal_suffix");
#
# (#) load GPATH
#
local($command) = "btreeop -L2 -k './' \"$dbpath/GPATH\"";
open(GPATH, "$command |") || &'error("cannot fork.");
$nextkey = 0;
while (<GPATH>) {
	chop;
	local($path, $no) = split;
	$'GPATH{$path} = $no;
	if ($no > $nextkey) {
		$nextkey = $no;
	}
}
close(GPATH);
if ($?) {&'error("'$command' failed."); }
#
# (3) make function entries ($DEFS/* and $REFS/*)
#     MAKING TAG CACHE
#
print STDERR "[", &'date, "] ", "(3) making duplicate entries ...\n" if ($'vflag);
sub suddenly { &'clean(); exit 1}
$SIG{'INT'} = 'suddenly';
$SIG{'QUIT'} = 'suddenly';
$SIG{'TERM'} = 'suddenly';
&cache'open();
$func_total = &makedupindex($dist);
print STDERR "Total $func_total functions.\n" if ($'vflag);
#
# (4) make function index (defines.html and defines/*)
#     PRODUCE @defines
#
print STDERR "[", &'date, "] ", "(4) making function index ...\n" if ($'vflag);
$func_total = &makedefineindex($dist, "$dist/defines.$'normal_suffix", $func_total);
print STDERR "Total $func_total functions.\n" if ($'vflag);
#
# (5) make file index (files.html and files/*)
#     PRODUCE @files %includes
#
print STDERR "[", &'date, "] ", "(5) making file index ...\n" if ($'vflag);
$file_total = &makefileindex($dist, "$dist/files.$'normal_suffix", "$dist/$INCS");
print STDERR "Total $file_total files.\n" if ($'vflag);
#
# [#] make a common part for mains.html and index.html
#     USING @defines @files
#
print STDERR "[", &'date, "] ", "(#) making a common part ...\n" if ($'vflag);
$index = &makecommonpart($title);
#
# (6)make index file (index.html)
#
print STDERR "[", &'date, "] ", "(6) making index file ...\n" if ($'vflag);
&makeindex("$dist/index.$'normal_suffix", $title, $index);
#
# (7) make main index (mains.html)
#
print STDERR "[", &'date, "] ", "(7) making main index ...\n" if ($'vflag);
&makemainindex("$dist/mains.$'normal_suffix", $index);
#
# (#) make anchor database
#
print STDERR "[", &'date, "] ", "(#) making temporary database ...\n" if ($'vflag);
&anchor'create();
#
# (8) make HTML files ($SRCS/*)
#     USING TAG CACHE, %includes and anchor database.
#
print STDERR "[", &'date, "] ", "(8) making hypertext from source code ...\n" if ($'vflag);
&makehtml($dist, $file_total);
#
# (9) search index. (search.html)
#
if ($'Fflag && $'fflag) {
	print STDERR "[", &'date, "] ", "(9) making search index ...\n" if ($'vflag);
	&makesearchindex("$dist/search.$'normal_suffix");
}
&'clean();
print STDERR "[", &'date, "] ", "Done.\n" if ($'vflag);
if ($'vflag && $'cgi && ($'cflag || $'fflag)) {
	print STDERR "\n";
	print STDERR "[Information]\n";
	print STDERR "\n";
	if ($'cflag) {
		print STDERR " Your system may need to be setup to decompress *.$'gzipped_suffix files.\n";
		print STDERR " This can be done by having your browser compiled with the relevant\n";
		print STDERR " options, or by configuring your http server to treat these as\n";
		print STDERR " gzipped files. (Please see 'HTML/.htaccess')\n";
		print STDERR "\n";
	}
	if ($'fflag) {
		local($path) = ($'action =~ /^\//) ? "DOCUMENT_ROOT$'action" : "HTML/$'action";
		print STDERR " You need to setup http server so that $path\n";
		print STDERR " is executed as a CGI script. (DOCUMENT_ROOT means WWW server's data root.)\n";
		print STDERR "\n";
	}
	print STDERR " Good luck!\n";
	print STDERR "\n";
}
# This is not supported.
if ($'icon_list && -f $'icon_list) {
	system("tar xzf $'icon_list -C $dist");
}
# include epilog_script if needed.
require($'epilog_script) if ($'epilog_script && -f $'epilog_script);
exit 0;
#-------------------------------------------------------------------------
# SUBROUTINES
#-------------------------------------------------------------------------
#
# makeprogram: make CGI program
#
sub makeprogram {
	local($file) = @_;
	local($globalpath) = &'usable('global');
	local($btreeoppath) = &'usable('btreeop');

	open(PROGRAM, ">$file") || &'error("cannot make CGI program.");
	$program = <<'END_OF_SCRIPT';
#! @PERL@
#------------------------------------------------------------------
# SORRY TO HAVE SURPRISED YOU!
# IF YOU SEE THIS UNREASONABLE FILE WHILE BROUSING, FORGET PLEASE.
# IF YOU ARE A ADMINISTRATOR OF THIS SITE, PLEASE SETUP HTTP SERVER
# SO THAT THIS SCRIPT CAN BE EXECUTED AS A CGI COMMAND. THANK YOU.
#------------------------------------------------------------------
$htmlbase = $ENV{'HTTP_REFERER'};
if ($htmlbase) {
	$htmlbase =~ s/\/[^\/]*$//;
} else {
	$htmlbase = '..';
}
print "Content-type: text/html\n\n";
print "@html_begin@\n";
print "@body_begin@\n";
if (! -x '@globalpath@' || ! -x '@btreeoppath@') {
	print "<H1><FONT COLOR=#cc0000>Error</FONT></H1>\n";
	print "<H3>Server side command not found. <A HREF=$htmlbase/mains.@normal_suffix@>[return]</A></H3>\n";
	print "@body_end@\n";
	print "@html_end@\n";
	exit 0;
}
@pairs = split (/&/, $ENV{'QUERY_STRING'});
foreach $p (@pairs) {
	($name, $value) = split(/=/, $p);
	$value =~ tr/+/ /;
	$value =~ s/%([\dA-Fa-f][\dA-Fa-f])/pack("C", hex($1))/eg;
	$form{$name} = $value;
}
if ($form{'pattern'} eq '') {
	print "<H1><FONT COLOR=#cc0000>Error</FONT></H1>\n";
	print "<H3>Pattern not specified. <A HREF=$htmlbase/mains.@normal_suffix@>[return]</A></H3>\n";
	print "@body_end@\n";
	print "@html_end@\n";
	exit 0;
}
$pattern = $form{'pattern'};
$flag = '';
$words = 'definitions';
if ($form{'type'} eq 'reference') {
	$flag = 'r';
	$words = 'references';
} elsif ($form{'type'} eq 'symbol') {
	$flag = 's';
	$words = 'symbols';
} elsif ($form{'type'} eq 'path') {
	$flag = 'P';
	$words = 'paths';
}
if ($form{'id'}) {
	chdir("$form{'id'}/cgi-bin");
	if ($?) {	
		print "<H1><FONT COLOR=#cc0000>Error</FONT></H1>\n";
		print "<H3>Couldn't find tag directory in secure mode. <A HREF=$htmlbase/mains.@normal_suffix@>[return]</A></H3>\n";
		print "@body_end@\n";
		print "@html_end@\n";
		exit 0;
	}
}
$pattern =~ s/"//g;			# to shut security hole
unless (open(PIPE, "@globalpath@ -x$flag \"$pattern\" |")) {
	print "<H1><FONT COLOR=#cc0000>Error</FONT></H1>\n";
	print "<H3>Cannot execute global. <A HREF=$htmlbase/mains.@normal_suffix@>[return]</A></H3>\n";
	print "@body_end@\n";
	print "@html_end@\n";
	exit 0;
}
print "<H1><FONT COLOR=#cc0000>" . $pattern . "</FONT></H1>\n";
print "Following $words are matched to above pattern.<HR>\n";
$cnt = 0;
local($tag, $lno, $filename, $fileno);
print "<PRE>\n";
while (<PIPE>) {
	$cnt++;
	($tag, $lno, $filename) = split;
	chop($fileno = `@btreeoppath@ -K "./$filename" GPATH`);
	s!($tag)!<A HREF=$htmlbase/@SRCS@/$fileno.@HTML@#$lno>$1<\/A>!;
	print;
}
close(PIPE);
print "</PRE>\n";
if ($cnt == 0) {
	print "<H3>Pattern not found. <A HREF=$htmlbase/mains.@normal_suffix@>[return]</A></H3>\n";
}
print "@body_end@\n";
print "@html_end@\n";
exit 0;
#------------------------------------------------------------------
# SORRY TO HAVE SURPRISED YOU!
# IF YOU SEE THIS UNREASONABLE FILE WHILE BROUSING, FORGET PLEASE.
# IF YOU ARE A ADMINISTRATOR OF THIS SITE, PLEASE SETUP HTTP SERVER
# SO THAT THIS SCRIPT CAN BE EXECUTED AS A CGI COMMAND. THANK YOU.
#------------------------------------------------------------------
END_OF_SCRIPT

	$quoted_body_begin = $'body_begin;
	$quoted_body_begin =~ s/"/\\"/g;
	$quoted_body_end = $'body_end;
	$quoted_body_end =~ s/"/\\"/g;
	$program =~ s/\@html_begin\@/$'html_begin/g;
	$program =~ s/\@html_end\@/$'html_end/g;
	$program =~ s/\@body_begin\@/$quoted_body_begin/g;
	$program =~ s/\@body_end\@/$quoted_body_end/g;
	$program =~ s/\@normal_suffix\@/$'normal_suffix/g;
	$program =~ s/\@SRCS\@/$'SRCS/g;
	$program =~ s/\@HTML\@/$'HTML/g;
	$program =~ s/\@globalpath\@/$globalpath/g;
	$program =~ s/\@btreeoppath\@/$btreeoppath/g;
	print PROGRAM $program;
	close(PROGRAM);
}
#
# makebless: make bless script
#
sub makebless {
	local($file) = @_;

	open(SCRIPT, ">$file") || &'error("cannot make bless script.");
	$script = <<'END_OF_SCRIPT';
#!/bin/sh
#
# Bless.sh: rewrite id's value of html for centralised cgi script.
#
# Usage:
#	% htags -S		<- works well at generated place.
#	% mv HTML /var/obj	<- move to another place. It doesn't work.
#	% cd /var/obj/HTML
#	% sh bless.sh		<- OK. It will work well!
#
pattern='INPUT TYPE=hidden NAME=id VALUE='
[ $1 ] && verbose=1
id=`pwd`
for f in mains.html index.html search.html; do
	if [ -f $f ]; then
		sed "s!<$pattern.*>!<$pattern$id>!" $f > $f.new;
		if ! cmp $f $f.new >/dev/null; then
			mv $f.new $f
			[ $verbose ] && echo "$f was blessed."
		else
			rm -f $f.new
		fi
	fi
done
END_OF_SCRIPT
	print SCRIPT $script;
	close(SCRIPT);
}
#
# makeghtml: make unzip script
#
sub makeghtml {
	local($file) = @_;
	open(PROGRAM, ">$file") || &'error("cannot make unzip script.");
	$program = <<END_OF_SCRIPT;
#!/bin/sh
echo "content-type: text/html"
echo
gzip -S $'HTML -d -c "\$PATH_TRANSLATED"
END_OF_SCRIPT

	print PROGRAM $program;
	close(PROGRAM);
}
#
# makehtaccess: make .htaccess skelton file.
#
sub makehtaccess {
	local($file) = @_;
	open(SKELTON, ">$file") || &'error("cannot make .htaccess skelton file.");
	$skelton = <<END_OF_SCRIPT;
#
# Skelton file for .htaccess -- This file was generated by htags(1).
#
# Htags have made gzipped hypertext because you specified -c option.
# If your browser doesn't decompress gzipped hypertext, you will need to
# setup your http server to treat this hypertext as gzipped files first.
# There are many way to do it, but one of the method is to put .htaccess
# file in 'HTML' directory.
#
# Please rewrite '/cgi-bin/ghtml.cgi' to the true value in your web site.
#
AddHandler htags-gzipped-html $'gzipped_suffix
Action htags-gzipped-html /cgi-bin/ghtml.cgi
END_OF_SCRIPT
	print SKELTON $skelton;
	close(SKELTON);
}
#
# makehelp: make help file
#
sub makehelp {
	local($file) = @_;
	local(@label) = ($'icon_list) ? @'anchor_comment : @'anchor_label;
	local(@icons) = @'anchor_icons;
	local(@msg)   = @'anchor_msg;

	open(HELP, ">$file") || &'error("cannot make help file.");
	print HELP $'html_begin, "\n";
	print HELP &'set_header('HELP');
	print HELP $'body_begin, "\n";
	print HELP "<H2>Usage of Links</H2>\n";

	print HELP "<PRE>/* ";
	foreach $n (0 .. $#label) {
		if ($'icon_list) {
			print HELP "<IMG SRC=icons/$icons[$n] ALT=\[$label[$n]\] HSPACE=3 BORDER=0>";
			if ($n < $#label) {
				print HELP " ";
			}
		} else {
			print HELP "\[$label[$n]\]";
		}
	}
	if ($'show_position) {
		print HELP "[+line file]";
	}
	print HELP " */</PRE>\n";
	print HELP "<DL>\n";
	foreach $n (0 .. $#label) {
		print HELP "<DT>";
		if ($'icon_list) {
			print HELP "<IMG SRC=icons/$icons[$n] ALT=\[$label[$n]\] HSPACE=3 BORDER=0>";
		} else {
			print HELP "[$label[$n]]";
		}
		print HELP "<DD>$msg[$n]\n";
	}
	if ($'show_position) {
		print HELP "<DT>[+line file]";
		print HELP "<DD>Current position (line number and file name).\n";
	}
	print HELP "</DL>\n";
	print HELP $'body_end, "\n";
	print HELP $'html_end, "\n";
	close(HELP);
}
#
# makedupindex: make duplicate entries index ($DEFS/* and $REFS/*)
#
#	go)	tag cache
#	r)	$count
#
sub makedupindex {
	local($dist) = @_;
	local($count) = 0;
	local($srcdir) = "../$'SRCS";

	foreach $db ('GRTAGS', 'GTAGS') {
		local($kind) = $db eq 'GTAGS' ? "definitions" : "references";
		local($option) = $db eq 'GTAGS' ? '' : 'r';
		local($prev) = '';
		local($first_line);
		local($writing) = 0;

		$count = 0;
		local($command) = "global -nx$option \".*\" | sort +0 -1 +2 -3 +1n -2";
		open(LIST, "$command |") || &'error("cannot fork.");
		while (<LIST>) {
			chop;
			local($tag) = split;
			if ($prev ne $tag) {
				$count++;
				print STDERR " [$count] adding $tag $kind\n" if ($'vflag);
				if ($writing) {
					print FILE &'list_end;
					print FILE $'body_end, "\n";
					print FILE $'html_end, "\n";
					close(FILE);
					$writing = 0;
				}
				# single entry
				if ($first_line) {
					&cache'put($db, $prev, $first_line);
				}
				$first_line = $_;
				$prev = $tag;
			} else {
				# duplicate entry
				if ($first_line) {
					&cache'put($db, $tag, " $count");
					local($type) = ($db eq 'GTAGS') ? $'DEFS : $'REFS;
					if ($'cflag) {
						open(FILE, "| gzip -c >$dist/$type/$count.$'HTML") || &'error("cannot make file '$dist/$type/$count.$'HTML'.");
					} else {
						open(FILE, ">$dist/$type/$count.$'HTML") || &'error("cannot make file '$dist/$type/$count.$'HTML'.");
					}
					$writing = 1;
					print FILE $'html_begin, "\n";
					print FILE &'set_header($tag);
					print FILE $'body_begin, "\n";
					print FILE &'list_begin;
					print FILE &'list_body($srcdir, $first_line);
					$first_line = '';
				}
				print FILE &'list_body($srcdir, $_);
			}
		}
		close(LIST);
		if ($?) { &'error("'$command' failed."); }
		if ($writing) {
			print FILE &'list_end;
			print FILE $'body_end, "\n";
			print FILE $'html_end, "\n";
			close(FILE);
		}
		if ($first_line) {
			&cache'put($db, $prev, $first_line);
		}
	}
	$count;
}
#
# makedefineindex: make definition index (including alphabetic index)
#
#	i)	dist		distribution directory
#	i)	file		definition index file
#	i)	total		definitions total
#	gi)	tag cache
#	go)	@defines
#
sub makedefineindex {
	local($dist, $file, $total) = @_;
	local($count) = 0;
	local($indexlink) = ($'Fflag) ? "../defines.$'normal_suffix" : "../mains.$'normal_suffix";
	local($target) = ($'Fflag) ? 'mains' : '_top';
	open(DEFINES, ">$file") || &'error("cannot make function index '$file'.");
	print DEFINES $'html_begin, "\n";
	print DEFINES &'set_header($'title_define_index);
	print DEFINES $'body_begin, "\n";
	if ($'Fflag) {
		print DEFINES "<A HREF=defines.$'normal_suffix><H2>$'title_define_index</H2></A>\n";
	} else {
		print DEFINES "<H2>$'title_define_index</H2>\n";
	}
	if (!$'aflag && !$'Fflag) {
		$indexlink = "mains.$'normal_suffix";
		print DEFINES "<A HREF=$indexlink>[index]</A>\n";
	}
	print DEFINES "<OL>\n" if (!$'aflag);
	local($old) = select(DEFINES);
	local($command) = "global -c";
	open(TAGS, "$command |") || &'error("cannot fork.");
	local($alpha, $alpha_f);
	@defines = ();	# [A][B][C]...
	while (<TAGS>) {
		$count++;
		chop;
		local($tag) = $_;
		print STDERR " [$count/$total] adding $tag\n" if ($'vflag);
		if ($'aflag && ($alpha eq '' || $tag !~ /^$alpha/)) {
			if ($alpha) {
				print ALPHA "</OL>\n";
				print ALPHA "<A HREF=$indexlink>";
				print ALPHA $'icon_list ? "<IMG SRC=../icons/$'back_icon ALT='[index]' HSPACE=3 BORDER=0>" : "[index]";
				print ALPHA "</A>\n";
				print ALPHA $'body_end, "\n";
				print ALPHA $'html_end, "\n";
				close(ALPHA);
			}
			# for multi-byte code
			local($c0, $c1);
			$c0 = substr($tag, 0, 1);
			if (ord($c0) > 127) {
				$c1 = substr($tag, 1, 1);
				$alpha   = $c0 . $c1;
				$alpha_f = "" . ord($c0) . ord($c1);
			} else {
				$alpha = $alpha_f = $c0;
				# for CD9660 or FAT file system
				# 97 == 'a', 122 == 'z'
				if (ord($c0) >= 97 && ord($c0) <= 122) {
					$alpha_f = "l$c0";
				}
			}
			push(@defines, "<A HREF=defines/$alpha_f.$'HTML>[$alpha]</A>\n");
			if ($'cflag) {
				open(ALPHA, "| gzip -c >$dist/defines/$alpha_f.$'HTML") || &'error("cannot make alphabetical function index.");
			} else {
				open(ALPHA, ">$dist/defines/$alpha_f.$'HTML") || &'error("cannot make alphabetical function index.");
			}
			print ALPHA $'html_begin, "\n";
			print ALPHA &'set_header("[$alpha]");
			print ALPHA $'body_begin, "\n";
			print ALPHA "<H2>[$alpha]</H2>\n";
			print ALPHA "<A HREF=$indexlink>";
			print ALPHA $'icon_list ? "<IMG SRC=../icons/$'back_icon ALT='[index]' HSPACE=3 BORDER=0>" : "[index]";
			print ALPHA "</A>\n";
			print ALPHA "<OL>\n";
			select(ALPHA);
		}
		local($line) = &cache'get('GTAGS', $tag);
		if ($line =~ /^ (.*)/) {
			print "<LI><A HREF=", ($'aflag) ? "../" : "", "$'DEFS/$1.$'HTML TARGET=$target>$tag</A>\n";
		} else {
			local($tag, $lno, $filename) = split(/[ \t]+/, $line);
			$filename = &'path2url($filename);
			print "<LI><A HREF=", ($'aflag) ? "../" : "", "$'SRCS/$filename#$lno TARGET=$target>$tag</A>\n";
		}
	}
	close(TAGS);
	if ($?) { &'error("'$command' failed."); }
	select($old);
	if ($'aflag) {
		print ALPHA "</OL>\n";
		print ALPHA "<A HREF=$indexlink>";
		print ALPHA $'icon_list ? "<IMG SRC=../icons/$'back_icon ALT='[index]' HSPACE=3 BORDER=0>" : "[index]";
		print ALPHA "</A>\n";
		print ALPHA $'body_end, "\n";
		print ALPHA $'html_end, "\n";
		close(ALPHA);

		print DEFINES @defines;
	}
	print DEFINES "</OL>\n" if (!$'aflag);
	if (!$'aflag && !$'Fflag) {
		print DEFINES "<A HREF=$indexlink>[index]</A>\n";
	}
	print DEFINES $'body_end, "\n";
	print DEFINES $'html_end, "\n";
	close(DEFINES);
	$count;
}
#
# makefileindex: make file index
#
#	i)	dist		distribution directory
#	i)	file		file name
#	i)	$incdir		$INC directory
#	go)	@files
#	go)	%includes
#
sub makefileindex {
	local($dist, $file, $incdir) = @_;
	local($count) = 0;
	local($indexlink) = ($'Fflag) ? "../files.$'normal_suffix" : "../mains.$'normal_suffix";
	local($target) = ($'Fflag) ? 'mains' : '_top';
	local(@dirstack, @fdstack);
	local($command) = "$'findcom | sort";
	open(FIND, "$command |") || &'error("cannot fork.");
	open(FILES, ">$file") || &'error("cannot make file '$file'.");
	print FILES $'html_begin, "\n";
	print FILES &'set_header($'title_file_index);
	print FILES $'body_begin, "\n";
	print FILES "<A HREF=files.$'normal_suffix><H2>$'title_file_index</H2></A>\n";
	print FILES "<OL>\n";

	local($org) = select(FILES);
	local(@push, @pop, $file);

	while (<FIND>) {
		$count++;
		chop;
		s!^\./!!;
		print STDERR " [$count] adding $_\n" if ($'vflag);
		@push = split('/');
		$file = pop(@push);
		@pop  = @dirstack;
		while ($push[0] && $pop[0] && $push[0] eq $pop[0]) {
			shift @push;
			shift @pop;
		}
		if (@push || @pop) {
			while (@pop) {
				pop(@dirstack);
				local($parent) = (@dirstack) ? &'path2url(join('/', @dirstack)) : $indexlink;
				print "</OL>\n";
				print "<A HREF=$parent>" .
					($'icon_list ? "<IMG SRC=../icons/$'back_icon ALT='[..]' HSPACE=3 BORDER=0>" : "[..]") .
					"</A>\n";
				print $'body_end, "\n";
				print $'html_end, "\n";
				$path = pop(@fdstack);
				close($path);
				select($fdstack[$#fdstack]) if (@fdstack);
				pop(@pop);
			}
			while (@push) {
				local($parent) = (@dirstack) ? &'path2url(join('/', @dirstack)) : $indexlink;
				push(@dirstack, shift @push);
				$path = join('/', @dirstack);
				$cur = "$dist/files/" . &'path2url($path);
				local($last) = $path;
				if (!$'full_path) {
					$last =~ s!.*/!!;
				}
				local($li) = "<LI>" .
					"<A HREF=" . (@dirstack == 1 ? 'files/' : '') . &path2url($path) . ">" .
					($'icon_list ? "<IMG SRC=" . (@dirstack == 1 ? '' : '../') . "icons/$'dir_icon ALT=[$path/] HSPACE=3 BORDER=0>" : '') .
					"$last/</A>\n";
				if (@dirstack == 1) {
					push(@files, $li);
				} else {
					print $li;
				}
				if ($'cflag) {
					open($cur, "| gzip -c >\"$cur\"") || &'error("cannot make directory index.");
				} else {
					open($cur, ">$cur") || &'error("cannot make directory index.");
				}
				select($cur);
				push(@fdstack, $cur);
				print $'html_begin, "\n";
				print &'set_header("$path/");
				print $'body_begin, "\n";
				print "<H2>";
				local(@p);
				foreach $n (0 .. $#dirstack) {
					push(@p, $dirstack[$n]);
					local($url) = &'path2url(join('/', @p));
					print "<A HREF=$url>" if ($n < $#dirstack);
					print "$dirstack[$n]";
					print "</A>" if ($n < $#dirstack);
					print "/";
				}
				print "</H2>\n";
				print "<A HREF=$parent>" .
					($'icon_list ? "<IMG SRC=../icons/$'back_icon ALT='[..]' HSPACE=3 BORDER=0>" : "[..]") .
					"</A>\n";
				print "<OL>\n";
			}
		}
		# collect include files. format is
		#	counter + '\n' + path1 + '\n' + path2 ...
		if (/.*\.[hH]$/) {
			if (! defined $includes{$file}) {
				$includes{$file} = "$count\n$_";
			} else {
				# duplicate entries
				$includes{$file} = "$includes{$file}\n$_";
			}
		}
		local($url) = &'path2url($_);
		local($last) = $_;
		if (!$'full_path) {
			$last =~ s!.*/!!;
		}
		local($li) = "<LI>" .
			"<A HREF=" . (@dirstack == 0 ? '' : '../') . "S/$url TARGET=$target>" .
			($'icon_list ? "<IMG SRC=" . (@dirstack == 0 ? '' : '../') . "icons/$'file_icon ALT=[$_] HSPACE=3 BORDER=0>" : '') .
			"$last</A>\n";
		if (@dirstack == 0) {
			push(@files, $li);
		} else {
			print $li;
		}
	}
	close(FIND);
	while (@dirstack) {
		pop(@dirstack);
		local($parent) = (@dirstack) ? &'path2url(join('/', @dirstack)) : $indexlink;
		print "</OL>\n";
		print "<A HREF=$parent>" .
			($'icon_list ? "<IMG SRC=../icons/$'back_icon ALT='[..]' HSPACE=3 BORDER=0>" : "[..]") .
			"</A>\n";
		print $'body_end, "\n";
		print $'html_end, "\n";
		$path = pop(@fdstack);
		close($path);
		select($fdstack[$#fdstack]) if (@fdstack);
	}
	print FILES @files;
	print FILES "</OL>\n";
	print FILES $'body_end, "\n";
	print FILES $'html_end, "\n";
        close(FILES);

	select($org);
	foreach $last (keys %includes) {
		local($no, @incs) = split(/\n/, $includes{$last});
		if (@incs > 1) {
			local($path) = "$incdir/$no.$'HTML";
			if ($'cflag) {
				open(INCLUDE, "| gzip -c >$path") || &'error("cannot open file '$path'.");
			} else {
				open(INCLUDE, ">$path") || &'error("cannot open file '$path'.");
			}
			print INCLUDE $'html_begin, "\n";
			print INCLUDE &'set_header($last);
			print INCLUDE $'body_begin, "\n";
			print INCLUDE "<PRE>\n";
			foreach $filename (@incs) {
				$path = &'path2url($filename);
				print INCLUDE "<A HREF=../$'SRCS/$path TARGET=$target>$filename</A>\n";
			}
			print INCLUDE "</PRE>\n";
			print INCLUDE $'body_end, "\n";
			print INCLUDE $'html_end, "\n";
			close(INCLUDE);
			# '' means that information already written to file.
			$includes{$last} = $no;
		}
	}
	$count;
}
#
# makesearchpart: make search part
#
#	i)	$action	action url
#	i)	$id	hidden variable
#	i)	$target	target
#	r)		html
#
sub makesearchpart {
	local($action, $id, $target) = @_;
	local($index) = '';

	if ($'Fflag) {
		$index .= "<A HREF=search.$'normal_suffix><H2>SEARCH</H2></A>\n";
	} else {
		$index .= "<H2>SEARCH</H2>\n";
	}
	if (!$target) {
		$index .= "Please input object name and select [Search]. POSIX's regular expression is allowed.<P>\n"; 
	}
	$index .= "<FORM METHOD=GET ACTION=$action";
	$index .= " TARGET=$target" if ($target);
	$index .= ">\n";
	$index .= "<INPUT NAME=pattern>\n";
	$index .= "<INPUT TYPE=hidden NAME=id VALUE=$id>\n";
	$index .= "<INPUT TYPE=submit VALUE=Search>\n";
	$index .= "<INPUT TYPE=reset VALUE=Reset><BR>\n";
	$index .= "<INPUT TYPE=radio NAME=type VALUE=definition CHECKED>";
	$index .= ($target) ? "Def" : "Definition";
	$index .= "\n<INPUT TYPE=radio NAME=type VALUE=reference>";
	$index .= ($target) ? "Ref" : "Reference";
	$index .= "\n<INPUT TYPE=radio NAME=type VALUE=symbol>";
	if (-f "$dbpath/GSYMS") {
		$index .= ($target) ? "Sym" : "Other symbol";
		$index .= "\n<INPUT TYPE=radio NAME=type VALUE=path>";
	}
	$index .= ($target) ? "Path" : "Path name";
	$index .= "\n</FORM>\n";
	$index;
}
#
# makecommonpart: make a common part for mains.html and index.html
#
#	gi)	@files
#	gi)	@defines
#
sub makecommonpart {
	local($title) = @_;
	local($index) = '';

	$index .= "<H1>$'title_begin$'title$'title_end</H1>\n";
	$index .= "<P ALIGN=right>\n";
	$index .= "Last updated " . &'date . "<BR>\n";
	$index .= "This hypertext was generated by <A HREF=http://www.tamacom.com/global/ TARGET=_top>GLOBAL-$'version</A>.<BR>\n";
	$index .= "</P>\n<HR>\n";
	if ($'fflag) {
		$index .= &makesearchpart($'action, $'id);
		$index .= "<HR>\n";
	}
	$index .= "<H2>MAINS</H2>\n";
	local($command) = "global -nx main | sort +0 -1 +2 -3 +1n -2";
	open(PIPE, "$command |") || &'error("cannot fork.");
	$index .= &'list_begin();
	while (<PIPE>) {
		chop;
		$index .= &'list_body($'SRCS, $_);
	}
	$index .= &'list_end();
	close(PIPE);
	if ($?) { &'error("'$command' failed."); }
	$index .= "<HR>\n";
	if ($'aflag && !$'Fflag) {
		$index .= "<H2>$'title_define_index</H2>\n";
		foreach $f (@defines) {
			$index .= $f;
		}
	} else {
		$index .= "<H2><A HREF=defines.$'normal_suffix>$'title_define_index</A></H2>\n";
	}
	$index .= "<HR>\n";
	if ($'Fflag) {
		$index .= "<H2><A HREF=files.$'normal_suffix>$'title_file_index</A></H2>\n";
	} else {
		$index .= "<H2>$'title_file_index</H2>\n";
		$index .= "<OL>\n";
		foreach $f (@files) {
			$index .= $f;
		}
		$index .= "</OL>\n<HR>\n";
	}
	$index;
}
#
# makeindex: make index file
#
#	i)	$file	file name
#	i)	$title	title of index file
#	i)	$index	common part
#
sub makeindex {
	local($file, $title, $index) = @_;

	if ($'Fflag) {
		open(FRAME, ">$file") || &'error("cannot open file '$file'.");
		print FRAME $'html_begin, "\n";
		print FRAME "<HEAD>\n<TITLE>$title</TITLE>\n$'meta_robots\n$'meta_generator\n</HEAD>\n";
		print FRAME "<FRAMESET COLS='200,*'>\n";
		if ($'fflag) {
			print FRAME "<FRAMESET ROWS='33%,33%,*'>\n";
			print FRAME "<FRAME NAME=search SRC=search.$'normal_suffix>\n";
		} else {
			print FRAME "<FRAMESET ROWS='50%,*'>\n";
		}
		print FRAME "<FRAME NAME=defines SRC=defines.$'normal_suffix>\n";
		print FRAME "<FRAME NAME=files SRC=files.$'normal_suffix>\n";
		print FRAME "</FRAMESET>\n";
		print FRAME "<FRAME NAME=mains SRC=mains.$'normal_suffix>\n";
		print FRAME "<NOFRAMES>\n";
		print FRAME $'body_begin, "\n";
		print FRAME $index;
		print FRAME $'body_end, "\n";
		print FRAME "</NOFRAMES>\n";
		print FRAME "</FRAMESET>\n";
		print FRAME $'html_end, "\n";
		close(FRAME);
	} else {
		open(FILE, ">$file") || &'error("cannot open file '$file'.");
		print FILE $'html_begin, "\n";
		print FILE &'set_header($title);
		print FILE $'body_begin, "\n";
		print FILE $index;
		print FILE $'body_end, "\n";
		print FILE $'html_end, "\n";
		close(FILE);
	}
}
#
# makemainindex: make main index
#
#	i)	$file	file name
#	i)	$index	common part
#
sub makemainindex {
	local($file, $index) = @_;

	open(INDEX, ">$file") || &'error("cannot create file '$file'.");
	print INDEX $'html_begin, "\n";
	print INDEX &'set_header($title);
	print INDEX $'body_begin, "\n";
	print INDEX $index;
	print INDEX $'body_end, "\n";
	print INDEX $'html_end, "\n";
	close(INDEX);
}
#
# makesearchindex: make search html
#
#	i)	$file	file name
#
sub makesearchindex {
	local($file) = @_;

	open(SEARCH, ">$file") || &'error("cannot create file '$file'.");
	print SEARCH $'html_begin, "\n";
	print SEARCH &'set_header('SEARCH');
	print SEARCH $'body_begin, "\n";
	print SEARCH &makesearchpart($'action, $'id, 'mains');
	print SEARCH $'body_end, "\n";
	print SEARCH $'html_end, "\n";
	close(SEARCH);
}
#
# makehtml: make html files
#
#	i)	total	number of files.
#
sub makehtml {
	local($dist, $total) = @_;
	local($count) = 0;

	open(FIND, "$'findcom |") || &'error("cannot fork.");
	while (<FIND>) {
		chop;
		$count++;
		local($path) = $_;
		$path =~ s/^\.\///;
		print STDERR " [$count/$total] converting $_\n" if ($'vflag);
		$path = &'path2url($path);
		&convert'src2html($_, "$dist/$'SRCS/$path");
	}
	close(FIND);
	if ($?) { &'error("cannot traverse directory."); }
}
#=========================================================================
# CONVERT PACKAGE
#=========================================================================
package convert;
#
# src2html: convert source code into HTML
#
#	i)	$file	source file	- Read from
#	i)	$hfile	HTML file	- Write to
#	gi)	%includes
#			pairs of include file and the path
#
sub src2html {
	local($file, $hfile) = @_;
	local($ncol) = $'ncol;
	local($tabs) = $'tabs;
	local(%ctab) = ('&', '&amp;', '<', '&lt;', '>', '&gt;');
	local($isjava) = ($file =~ /\.java$/) ? 1 : 0;
	local($iscpp) = ($file =~ /\.(h|c\+\+|cc|cpp|cxx|hxx|C|H)$/) ? 1 : 0;
	local($command);

	if ($'cflag) {
		$command = "gzip -c >";
		$command .= ($'w32) ? "\"$hfile\"" : "'$hfile'";
		open(HTML, "| $command") || &'error("cannot create file '$hfile'.");
	} else {
		open(HTML, ">$hfile") || &'error("cannot create file '$hfile'.");
	}
	local($old) = select(HTML);
	#
	# load tags belonging to this file.
	#
	&anchor'load($file);
	$command = "gtags --expand -$tabs ";
	$command .= ($'w32) ? "\"$file\"" : "'$file'";
	open(SRC, "$command |") || &'error("cannot fork.");
	#
	# print the header
	#
	$file =~ s/^\.\///;
	print $'html_begin, "\n";
	print &'set_header($file);
	print $'body_begin, "\n";
	print "<A NAME=TOP><H2>$file</H2>\n";
	print "$'comment_begin/* ";
	print &link_format(&anchor'getlinks(0));
	print " */$'comment_end";
	if ($'show_position) {
		print $'position_begin;
		print "[+1 $file]";
		print $'position_end;
	}
	print "\n<HR>\n";
	print "<H2>$'title_define_index</H2>\n";
	print "This source file includes following functions.\n";
	print "<OL>\n";
	local($lno, $tag, $type);
	for (($lno, $tag, $type) = &anchor'first(); $lno; ($lno, $tag, $type) = &anchor'next()) {
		print "<LI><A HREF=#$lno onMouseOver=\"show('R',$lno,'')\">$tag</A>\n" if ($type eq 'D');
	}
	print "</OL>\n";
	print "<HR>\n";
	#
	# print source code
	#
	print "<PRE>\n";
	$INCOMMENT = 0;			# initial status is out of comment
	local($LNO, $TAG, $TYPE) = &anchor'first();
	while (<SRC>) {
		local($converted);
		s/\r$//;
		# make link for include file
		if (!$INCOMMENT && /^#[ \t]*include/) {
			local($last, $sep) = m![</"]([^</"]+)([">])!;
			if (defined $'includes{$last}) {
				local($link);
				local($no, @incs) = split(/\n/, $'includes{$last});
				if (@incs == 1) {
					$link = &'path2url($incs[0]);
				} else {
					$link = "../$'INCS/$no.$'HTML";
				}
				# quote path name.
				$last =~ s/([\[\]\.\*\+])/\\\1/g;
				if ($sep eq '"') {
					s!"(.*$last)"!"<A HREF=$link>$1</A>"!;
				} else {
					s!<(.*$last)>!&lt;<A HREF=$link>$1</A>&gt;!;
				}
				$converted = 1;
			}
		}
		# translate '<', '>' and '&' into entity name
		if (!$converted) { s/([&<>])/$ctab{$1}/ge; }
		&protect_line();	# protect quoted char, strings and comments
		# painting source code
		s/({|})/$'brace_begin$1$'brace_end/g;
		local($sharp) = s/^(#[ \t]*\w+)// ? $1 : '';
		if ($sharp !~ '#[ \t]*include') {
			if ($isjava) {
				s/\b($'java_reserved_words)\b/$'reserved_begin$1$'reserved_end/go;
			} elsif ($iscpp) {
				s/\b($'cpp_reserved_words)\b/$'reserved_begin$1$'reserved_end/go;
			} else {
				s/\b($'c_reserved_words)\b/$'reserved_begin$1$'reserved_end/go;
			}
		}
		s/^/$'sharp_begin$sharp$'sharp_end/ if ($sharp);	# recover macro
		local($define_line) = 0;
		local(@links) = ();
		local($count) = 0;
		local($lno_printed) = 0;

		if ($'lflag || $. == 1) {
			print "<A NAME=$.>";
			$lno_printed = 1;
		}
		for (; int($LNO) == $.; ($LNO, $TAG, $TYPE) = &anchor'next()) {
			if (!$lno_printed) {
				print "<A NAME=$.>";
				$lno_printed = 1;
			}
			$define_line = $LNO if ($TYPE eq 'D');
			$db = ($TYPE eq 'R') ? 'GTAGS' : 'GRTAGS';
			local($line) = &cache'get($db, $TAG);
			if (defined($line)) {
				local($href);
				if ($line =~ /^ (.*)/) {
					local($type) = ($TYPE eq 'R') ? $'DEFS : $'REFS;
					local($msg) = 'Multiple ';
					$msg .= ($TYPE eq 'R') ? 'defined.' : 'refered.';
					$href = "<A HREF=../$type/$1.$'HTML onMouseOver=\"show('$TYPE',-1,'')\">$TAG</A>";
				} else {
					local($nouse, $lno, $filename) = split(/[ \t]+/, $line);
					$nouse = '';	# to make perl quiet
					local($url) = &'path2url($filename);
					$filename =~ s!\./!!; 
					local($msg) = ($TYPE eq 'R') ? 'Defined at' : 'Refered from';
					$href = "<A HREF=../$'SRCS/$url#$lno onMouseOver=\"show('$TYPE',$lno,'$filename')\">$TAG</A>";
				}
				# set tag marks and save hyperlink into @links
				if (ord($TAG) > 127) {	# for multi-byte code
					if (s/([\x00-\x7f])$TAG([ \t]*\()/$1\005$count\005$2/ || s/([\x00-\x7f])$TAG([\x00-\x7f])/$1\005$count\005$2/) {
						$count++;
						push(@links, $href);
					} else {
						print STDERR "Error: $file $LNO $TAG($TYPE) tag must exist.\n" if ($'wflag);
					}
				} else {
					if (s/\b$TAG([ \t]*\()/\005$count\005$1/ || s/\b$TAG\b/\005$count\005/ || s/\b_$TAG\b/_\005$count\005/)
					{
						$count++;
						push(@links, $href);
					} else {
						print STDERR "Error: $file $LNO $TAG($TYPE) tag must exist.\n" if ($'wflag);
					}
				}
			} else {
				print STDERR "Warning: $file $LNO $TAG($TYPE) found but not referred.\n" if ($'wflag);
			}
		}
		# implant links
		local($s);
		for ($count = 0; @links; $count++) {
			$s = shift @links;
			unless (s/\005$count\005/$s/) {
				print STDERR "Error: $file $LNO $TAG($TYPE) tag must exist.\n" if ($'wflag);
			}
		}
		&unprotect_line();
		# print a line
		printf "%${ncol}d ", $. if ($'nflag);
		print;
		# print guide
		if ($define_line) {
			print ' ' x ($ncol + 1) if ($'nflag);
			print "$'comment_begin/* ";
			print &link_format(&anchor'getlinks($define_line));
			if ($'show_position) {
				print $'position_begin;
				print "[+$define_line $file]";
				print $'position_end;
			}
			print " */$'comment_end\n";
		}
	}
	print "</PRE>\n";
	print "<HR>\n";
	print "<A NAME=BOTTOM>\n";
	print "$'comment_begin/* ";
	print &link_format(&anchor'getlinks(-1));
	if ($'show_position) {
		print $'position_begin;
		print "[+$. $file]";
		print $'position_end;
	}
	print " */$'comment_end";
	print "\n";
	print $'body_end, "\n";
	print $'html_end, "\n";
	close(SRC);
	if ($?) { &'error("cannot open file '$file'."); }
	close(HTML);
	select($old);

}
#
# protect_line: protect quoted strings
#
#	io)	$_	source line
#
#	\001	quoted(\) char
#	\002	quoted('') char
#	\003	quoted string
#	\004	comment
#	\005	line comment
#	\032	temporary mark
#
sub protect_line {
	@quoted_char1 = ();
	while (s/(\\.)/\001/) {
		push(@quoted_char1, $1);
	}
	@quoted_char2 = ();
	while (s/('[^']')/\002/) {
		push(@quoted_char2, $1);
	}
	@quoted_strings = ();
	while (s/("[^"]*")/\003/) {
		push(@quoted_strings, $1);
	}
	@comments = ();
	s/^/\032/ if ($INCOMMENT);
	while (1) {
		if ($INCOMMENT == 0) {
			if (s/\/\*/\032\/\*/) {		# start comment
				$INCOMMENT = 1;
			} else {
				last;
			}
		} else {
			# This regular expression was drived from
			# perl FAQ 4.27 (ftp://ftp.cis.ufl.edu/pub/perl/faq/FAQ)
			if (s!\032((/\*)?[^*]*\*+([^/*][^*]*\*+)*/)!\004!) {
				push(@comments, $1);
				$INCOMMENT = 0;
			} else {
				s/\032(.*)$/\004/;	# mark comment
				push(@comments, $1);
			}
			last if ($INCOMMENT);
		}
	}
	$line_comment = '';
	if (s!(//.*)$!\005! || s!(/\004.*)$!\005!) {
		$line_comment = $1;
		# ^     //   /*   $	... '//' invalidate '/*'.
		# ^     //*       $	... Assumed '//' + '*', not '/' + '/*'.
		$INCOMMENT = 0;
	}
}
#
# unprotect_line: recover quoted strings
#
#	i)	$_	source line
#
sub unprotect_line {
	local($s);

	if ($line_comment) {
		s/\005/$'comment_begin$line_comment$'comment_end/;
	}
	while (@comments) {
		$s = shift @comments;
		# nested tag can be occured but no problem.
		s/\004/$'comment_begin$s$'comment_end/;
	}
	while (@quoted_strings) {
		$s = shift @quoted_strings;
		s/\003/$s/;
	}
	while (@quoted_char2) {
		$s = shift @quoted_char2;
		s/\002/$s/;
	}
	while (@quoted_char1) {
		$s = shift @quoted_char1;
		s/\001/$s/;
	}
}
#
# link_format: format hyperlinks.
#
#	i)	(previous, next, first, last, top, bottom)
#
sub link_format {
	local(@tag) = @_;
	local(@label) = ($'icon_list) ? @'anchor_comment : @'anchor_label;
	local(@icons) = @'anchor_icons;
	local($line);

	for $n (0 .. $#label) {
		if ($n == 6) {
			$line .=  "<A HREF=../mains.$'normal_suffix>";
		} elsif ($n == 7) {
			$line .=  "<A HREF=../help.$'normal_suffix>";
		} elsif ($tag[$n]) {
			$line .=  "<A HREF=#$tag[$n]>";
		}
		if ($'icon_list) {
			$icon = ($tag[$n] || $n > 5) ? "$icons[$n]" : "n_$icons[$n]";
			$line .= "<IMG SRC=../icons/$icon ALT=\[$label[$n]\] BORDER=0 ALIGN=MIDDLE>";
		} else {
			$line .=  "\[$label[$n]\]";
		}
		$line .=  "</A>" if ($n > 5 || $tag[$n]);
		if ($'icon_list && $n < $#label) {
			$line .= ' ';
		}
	}
	$line;
}

#=========================================================================
# ANCHOR PACKAGE
#=========================================================================
package anchor;
#
# create: create anchors temporary database
#
#	go)	%PATHLIST
#
sub create {
	$ANCH = "$'tmp/ANCH$$";
	open(ANCH, ">$ANCH") || &'error("cannot create file '$ANCH'.");
	close(ANCH);
	chmod ($ANCH, 0600);
	local($command) = "btreeop -C $ANCH";
	open(ANCH, "| $command") || &'error("cannot fork.");
	local($fcount) = 1;
	local($fnumber);
	foreach $db ('GTAGS', 'GRTAGS') {
		local($option) = ($db eq 'GTAGS') ? '' : 'r';
		local($command) = "global -nnx$option \".*\"";
		open(PIPE, "$command |") || &'error("cannot fork.");
		while (<PIPE>) {
			local($tag, $lno, $filename, $image) = split;
			$fnumber = $PATHLIST{$filename};
			if (!$fnumber) {
				$PATHLIST{$filename} = $fnumber = $fcount++;
			}
			if ($db eq 'GTAGS') {
				$type = ($image =~ /^#[ \t]*define/) ? 'M' : 'D';
			} else {
				$type = 'R';
			}
			print ANCH "$fnumber $lno $tag $type\n";
		}
		close(PIPE);
		if ($?) { &'error("'$command' failed."); }
	}
	close(ANCH);
	if ($?) { &'error("'$command' failed."); }
}
#
# finish: remove anchors database
#
sub finish {
	unlink("$ANCH") if (defined($ANCH));
}
#
# load: load anchors belonging to specified file.
#
#	i)	$file	source file
#	gi)	%PATHLIST
#	go)	FIRST	first definition
#	go)	LAST	last definition
#
sub load {
	local($file) = @_;
	local($fnumber);

	@ANCHORS = ();
	$FIRST = $LAST = 0;

	$file = './' . $file if ($file !~ /^\.\//);
	if (!($fnumber = $PATHLIST{$file})) {
		return;
	}
	local($command) = "btreeop -K $fnumber $ANCH";
	open(ANCH, "$command |") || &'error("cannot fork.");
	while (<ANCH>) {
		local($fnumber, $lno, $tag, $type) = split;
		local($line);
		# don't refer to macros which is defined in other C source.
		if ($type eq 'R' && ($line = &cache'get('GTAGS', $tag))) {
			local($nouse1, $nouse2, $f, $def) = split(/[ \t]+/, $line);
			if ($f !~ /\.h$/ && $f ne $file && $def =~ /^#/) {
				print STDERR "Information: $file $lno $tag($type) skipped, because this is a macro which is defined in other C source.\n" if ($'wflag);
				next;
			}
		}
		push(@ANCHORS, "$lno,$tag,$type");
	}
	close(ANCH);
	if ($?) {&'error("'$command' failed."); }
	local(@keys);
	foreach (@ANCHORS) {
		push(@keys, (split(/,/))[0]);
	}
	sub compare { $keys[$a] <=> $keys[$b]; }
	@ANCHORS = @ANCHORS[sort compare 0 .. $#keys];
	local($c);
	for ($c = 0; $c < @ANCHORS; $c++) {
		local($lno, $tag, $type) = split(/,/, $ANCHORS[$c]);
		if ($type eq 'D') {
			$FIRST = $lno;
			last;
		}
	}
	for ($c = $#ANCHORS; $c >= 0; $c--) {
		local($lno, $tag, $type) = split(/,/, $ANCHORS[$c]);
		if ($type eq 'D') {
			$LAST = $lno;
			last;
		}
	}
}
#
# first: get first anchor
#
sub first {
	$CURRENT = 0;
	local($lno, $tag, $type) = split(/,/, $ANCHORS[$CURRENT]);
	$CURRENTDEF = $CURRENT if ($type eq 'D');

	($lno, $tag, $type);
}
#
# next: get next anchor
#
sub next {
	if (++$CURRENT > $#ANCHORS) {
		return ('', '', '');
	}
	local($lno, $tag, $type) = split(/,/, $ANCHORS[$CURRENT]);
	$CURRENTDEF = $CURRENT if ($type eq 'D');

	($lno, $tag, $type);
}
#
# getlinks: get links
#
#	i)	linenumber	>= 1: line number
#				0: header, -1: tailer
#	gi)	@ANCHORS tag table in current file
#	r)		(previous, next, first, last, top, bottom)
#
sub getlinks {
	local($linenumber) = @_;
	local($prev, $next, $first, $last, $top, $bottom);

	$prev = $next = $first = $last = $top = $bottom = 0;
	if ($linenumber >= 1) {
		local($c, $p, $n);
		if ($CURRENTDEF == 0) {
			for ($c = 0; $c <= $#ANCHORS; $c++) {
				local($lno, $tag, $type) = split(/,/, $ANCHORS[$c]);
				if ($lno == $linenumber && $type eq 'D') {
					last;
				}
			}
			$CURRENTDEF = $c;
		} else {
			for ($c = $CURRENTDEF; $c >= 0; $c--) {
				local($lno, $tag, $type) = split(/,/, $ANCHORS[$c]);
				if ($lno == $linenumber && $type eq 'D') {
					last;
				}
			}
		}
		$p = $n = $c;
		while (--$p >= 0) {
			local($lno, $tag, $type) = split(/,/, $ANCHORS[$p]);
			if ($type eq 'D') {
				$prev = $lno;
				last;
			}
		}
		while (++$n <= $#ANCHORS) {
			local($lno, $tag, $type) = split(/,/, $ANCHORS[$n]);
			if ($type eq 'D') {
				$next = $lno;
				last;
			}
		}
	}
	$first = $FIRST if ($FIRST > 0 && $linenumber != $FIRST);
	$last  = $LAST if ($LAST > 0 && $linenumber != $LAST);
	$top = 'TOP' if ($linenumber != 0);
	$bottom = 'BOTTOM' if ($linenumber != -1);
	if ($FIRST > 0 && $FIRST == $LAST) {
		$last  = '' if ($linenumber == 0);
		$first = '' if ($linenumber == -1);
	}

	($prev, $next, $first, $last, $top, $bottom);
}

#=========================================================================
# CACHE PACKAGE
#=========================================================================
package cache;
#
# open: open tag cache
#
#	i)	size	cache size
#			   -1: all cache
#			    0: no cache
#			other: sized cache
#
sub open {
	$CACH  = "$'tmp/CACH$$";
	dbmopen(%CACH, $CACH, 0600) || &'error("cannot make cache database.");
}
#
# put: put tag into cache
#
#	i)	$db	database name
#	i)	$tag	tag name
#	i)	$line	tag line
#
sub put {
	local($db, $tag, $line) = @_;
	local($label) = ($db eq 'GTAGS') ? 'D' : 'R';

	$CACH{$label.$tag} = $line;
}
#
# get: get tag from cache
#
#	i)	$db	database name
#	i)	$tag	tag name
#	r)		tag line
#
sub get {
	local($db, $tag) = @_;
	local($label) = ($db eq 'GTAGS') ? 'D' : 'R';

	defined($CACH{$label.$tag}) ? $CACH{$label.$tag} : undef;
}
#
# close: close cache
#
sub close {
	if ($CACH) {
		dbmclose(%CACH);
		unlink("$CACH.db", "$CACH.pag", "$CACH.dir");
	}
}
