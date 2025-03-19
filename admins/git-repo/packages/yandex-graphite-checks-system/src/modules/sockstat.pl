#!/usr/bin/perl -w

use strict;

### try /proc/net/sockstat6 first
my $tcp6_inuse = 0;
my $udp6_inuse = 0;
if (open(SOCKSTAT6, "< /proc/net/sockstat6")) {
    foreach my $source6 (<SOCKSTAT6>) {
        if ($source6 =~ m/TCP6:\sinuse\s(\d+)/) { $tcp6_inuse = $1; }
        elsif ($source6 =~ m/UDP6:\sinuse\s(\d+)/) { $udp6_inuse = $1; }
    }
    close(SOCKSTAT6)
}
if (open(SOURCE, "< /proc/net/sockstat")) {
    foreach my $source (<SOURCE>) {
        chomp($source);
        
        next unless ($source);
        
        if (my ($used) = $source =~ m/sockets:\sused\s(\d+)/si) {
            print "used $used\n";            
        } elsif (my ($tcp_inuse, $tcp_orphan, $tcp_tw, $tcp_alloc, $tcp_mem) = $source =~ m/TCP:\sinuse\s(\d+)\sorphan\s(\d+)\stw\s(\d+)\salloc\s(\d+)\smem\s(\d+)/si) {
            print "tcp.inuse ".($tcp_inuse + $tcp6_inuse)."\n";
            print "tcp.orphan $tcp_orphan\n";
            print "tcp.tw $tcp_tw\n";
            print "tcp.alloc $tcp_alloc\n";
            print "tcp.mem $tcp_mem\n";
        } elsif (my ($udp_inuse, $udp_mem) = $source =~ /UDP:\sinuse\s(\d+)\smem\s(\d+)/si) {
            print "udp.inuse ".($udp_inuse+$udp6_inuse)."\n";
            print "udp.mem $udp_mem\n";
        }
    }
    
    close(SOURCE);
} else {
    warn("Error (sockstat) $?: \"$!\"");
}


1;

