#!/usr/bin/perl -w

use strict;

if (open(SOURCE, "< /proc/sys/fs/file-nr")) {
    my ($assigned, $unused, $max) = <SOURCE> =~ m/(\d+)\s+(\d+)\s+(\d+)/si;
    
    close(SOURCE);

    print "assigned $assigned\n";
    print "unused $unused\n";
    print "max $max\n";

} else {
    warn("Error (files) $?: \"$!\"");
}


1;
