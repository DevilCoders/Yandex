#!/usr/bin/perl

use strict;
use warnings;
use Data::Dumper;

my %codes = ();

$codes{"READ_client_size"} = 0;
$codes{"WRITE_client_size"} = 0;
$codes{"READ_CACHE_client_size"} = 0;
$codes{"WRITE_CACHE_client_size"} = 0;

while (<STDIN>)
{
    s/.* ([IE][A-Z]+:.*)/$1/;
    s/(:|,|\.|\/[0-9]*)//g;
    my @fields = split ' ';
    if (exists $fields[3] && $fields[3] =~ "client") {
        #print Dumper(@fields);
        my $key = "";
        my $op = $fields[2];
        if ($op !~ /[A-Z]{3}/) { next; }

	if ($op =~ "NEW") {
	    my $err = $fields[17];
        my $time = substr(sprintf('%f', $fields[11]/1e6),0,5);
        $codes{"$op\_timings"} .= "$time ";
        $key="$op\_$err";
	}
    elsif ($op eq "LOOKUP" || $op eq "REMOVE" || $op eq "REVERSE_LOOKUP" ||  $op eq "MONITOR_STAT" || $op eq "BACKEND_STATUS" || $op eq "ROUTE_LIST" || $op eq "JOIN" || $op eq "AUTH") {
	    my $err = $fields[17];
        my $time = substr(sprintf('%f', $fields[11]/1e6),0,5);
        $codes{"$op\_timings"} .= "$time ";
        $key="$op\_$err";
    }
	else {
		my $type = $fields[3];
		$fields[-1] =~ s/\]//;
		my $backend = $fields[-5];
		my $err = $fields[30];
		my $time = substr(sprintf('%f', $fields[25]/1e6),0,5);
        if ($time < 0 ) {
            print $op;
        }
		$codes{"$op\_timings"} .= "$time ";
		$key="$op\_$err";
        }
        #else { next; }
        if (exists $codes{$key})
        {
            $codes{$key} += 1;
        }
        else
        {
            $codes{$key} = 1;
        }
    }
}
while ((my $k, my $v) = each %codes) {
    print "$k $v\n";
}
