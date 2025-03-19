#!/usr/bin/perl

use strict;
use warnings;
use Config::General;
use File::Basename;

my $path = $ARGV[0];

unless ($path) {
    print STDERR "Usage: gen-make.pl <PATH TO BUILD>\n";
    exit 1;
}

my %content = Config::General::ParseConfig("$path/content");
unless (%content) {
    print STDERR "Error parsing $path/content";
    exit 1;
}

my @Makefile;

push(@Makefile, "install:");

my %made_dirs;

foreach my $file (sort keys %content) {
    unless (-f "$path/$file") {
        print STDERR "$path/$file not found. Fix it in $path/content";
        exit 1;
    }
    my (undef, $destdir) = fileparse($content{$file});
    if (not defined($made_dirs{$destdir})) {
        push(@Makefile, "\tinstall -m 755 -d \$(DESTDIR)/$destdir");
	$made_dirs{$destdir} = "";
    }
    my $mode = 644;
    $mode = 755 if (-x "$path/$file");
    push(@Makefile, "\tinstall -m $mode $file \$(DESTDIR)/$content{$file}");
}

open(MAKEFILE, "> $path/Makefile") or die "Cannot open $path/Makefile for writing";

print MAKEFILE join("\n", @Makefile);
print MAKEFILE "\n";

close(MAKEFILE);