#!/usr/bin/perl -w

use strict;

if (open(SOURCE, "< /proc/loadavg")) {
    my ($la_1, $la_5, $la_15, $procs_working, $procs_total) = <SOURCE> =~ m/(\d+.\d+)\s(\d+.\d+)\s(\d+.\d+)\s(\d+)\/(\d+)/si;
    
    close(SOURCE);

    print "1min $la_1\n";   
    print "5min $la_5\n";   
    print "15min $la_15\n";   
    print "processes.working $procs_working\n";
    print "processes.total $procs_total\n";

} else {
    warn("Error (la) $?: \"$!\"");
}


1;
