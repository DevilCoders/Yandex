#!/usr/bin/perl -w

use strict;

if (open(SOURCE, "< /proc/meminfo")) {
    my ($mem_total, $mem_free, $mem_used, $mem_buffers, $mem_cached) = ();
    my ($swap_total, $swap_free, $swap_used) = ();
    
    foreach my $line (<SOURCE>) {
        chomp($line);
        
        next unless ($line);
        
        if (my ($value) = $line =~ m/^MemTotal:\s+(\d+)/si) {
            $mem_total = $value;
        } elsif (($value) = $line =~ m/^MemFree:\s+(\d+)/si) {
            $mem_free = $value;
            $mem_used = $mem_total - $mem_free;
        } elsif (($value) = $line =~ m/^Buffers:\s+(\d+)/si) {
            $mem_buffers = $value;
        } elsif (($value) = $line =~ m/^Cached:\s+(\d+)/si) {
            $mem_cached = $value;
        } elsif (($value) = $line =~ m/^SwapTotal:\s+(\d+)/si) {
            $swap_total = $value;
        } elsif (($value) = $line =~ m/^SwapFree:\s+(\d+)/si) {
            $swap_free = $value;
            $swap_used = $swap_total - $swap_free;
        }
    }

    printf("memory.total %d\nmemory.free %d\nmemory.used %d\nmemory.buffers %d\nmemory.cached %d\nswap.total %d\nswap.free %d\nswap.used %d",
                         $mem_total * 1024,
                         $mem_free * 1024,
                         $mem_used * 1024,
                         $mem_buffers * 1024,
                         $mem_cached * 1024,
                         $swap_total * 1024,
                         $swap_free * 1024,
                         $swap_used * 1024);
    
    close(SOURCE);
} else {
    warn("Error (meminfo) $?: \"$!\"");
}


1;
