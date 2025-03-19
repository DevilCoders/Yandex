#!/usr/bin/perl

use strict;
use locale;
binmode(STDIN);
binmode(STDOUT);
$|=1;

sub prepare($) {
	my $record=shift;
	chomp($record);
	return undef if ! defined($record);
	$record=~s/\0/\\0/gs;
	$record=~s/unixtime_microsec_utc=(\d+)(\d{6})\b/unixtime_microsec_utc=$1.$2/s;
	$record=~s/\n/\\n/gs;
	$record=~s/\r/\\r/gs;
	$record=~s/[\t]{1}(?!\S+=)/\\t/gs;
	$record=~s/\t(\S+)=/"\t".lc($1)."="/egs;
	return $record;
}

my ($line,$buffer,$prev_id,$current_id,$data)=(undef,undef,undef,undef,undef);
while ($line=<>) {
	$line=~/^((\d+;){3})(.*)/;
	($current_id,$data)=($1,$3);
	if ($data!~/^tskv\t/) {
		$buffer.='\n'.&prepare($data);
	} else {
		if ($buffer) {
			print "$prev_id$buffer\n";
			undef($buffer);
		}
		$buffer=&prepare($data);
	}
	$prev_id=$current_id;
}
print "$prev_id$buffer\n";

exit 0;
