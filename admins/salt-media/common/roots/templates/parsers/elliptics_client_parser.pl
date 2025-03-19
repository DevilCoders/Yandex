#!/usr/bin/perl

use strict;
use warnings;

my %codes = ();
$codes{"WRITE_size"} = 0;

while (<>)
{
    s/.* ([A-Z]+:.*)/$1/;
    s/(:|,|\.|\/[0-9]*)//g;
    my @fields = split ' ';
    my $key = "";
    if ($fields[0] eq "INFO" && $fields[2] eq "destruction")
    {
        my $op = $fields[3];
        my $err = $fields[19];
        my $time = substr($fields[13]/1e6,0,5);
        my $size = $fields[13];
        $key="$op\_$err";
        $codes{"$op\_timings"} .= "$time ";
        $codes{"WRITE_size"} += $size/1e6;
    }
    elsif ($fields[0] eq "ERROR" && $fields[7] =~ /[A-Z]{3}/ && $fields[14] eq "-6")
    {
        my $op = $fields[7];
        my $err = $fields[14];
        $key="$op\_$err";
    }
    else { next; }
    if (exists $codes{$key})
    {
        $codes{$key} += 1;
    }
    else
    {
        $codes{$key} = 1;
    }
}
        
while ((my $k, my $v) = each %codes) {
    print "$k $v\n";
}
