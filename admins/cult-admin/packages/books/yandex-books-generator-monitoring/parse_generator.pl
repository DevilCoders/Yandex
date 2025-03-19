#!/usr/bin/perl

use strict;
use warnings;

my %pairs;

while (<>) {
        %pairs = &get_values("$_");

        foreach my $i (sort keys %pairs) {
                print "$i = " . $pairs{"$i"};
        }
}

sub get_values {
        $_[0] =~ /([^=\s\(,]*)="?("[^"]*"|[^,)"]*)/g;
}
