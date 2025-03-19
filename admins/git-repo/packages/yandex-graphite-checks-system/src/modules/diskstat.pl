#!/usr/bin/perl -w

use strict;
use Time::HiRes qw(time);

# Array of block devices
my %devices = ();
my $prev_file = "/tmp/graphite.diskstat.%s.prev";

sub prev_file($) {
    return sprintf($prev_file, $_[0]);
}

if (open(PARTITIONS, "< /proc/partitions")) {
    my %results = ();
    
    foreach my $partition (<PARTITIONS>) {
        chomp($partition);
        
        next unless ($partition);
        
        # 8 0 71687325 sda | 8 0 71687325 cciss/c0d0 |  252 0 314572800 dm-0
        if (my ($device) = $partition =~ m/^\s*\d+\s+\d+\s+\d+\s+([a-zA-Z_]+|cciss\/c\d+d\d+|dm\-\d+)$/si) {
            $device =~ s/\//\!/gsi;

            # Gathering current values
            if (open(STAT, "< /sys/block/" . $device . "/stat")) {
                foreach my $stat (<STAT>) {
                    chomp($stat);
                    
                    next unless ($stat);
                    
                    # 40985 202299 2609936 528892 16965 59025 609112 1210420 0 233732 1748112
                    if (my ($rd_ios, $rd_merges, $rd_sectors, $rd_ticks, $wr_ios, $wr_merges, $wr_sectors, $wr_ticks, $ticks, $aveq) = $stat =~ m/\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+(\d+)\s+(\d+)/si) {
                        # Gathering time
                        if (open(CPU, "< /proc/stat")) {
                            my ($us, $ni, $sy, $id, $io, $hi, $si) = <CPU> =~ m/^cpu\s+(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)/si;
                            
                            $devices{$device}{"time"}   = $devices{$device}{"prev_time"}       = time();   # Current real time
                            
                            close(CPU);
                        } else {
                            die("Error (diskstat) $?: \"$!\"");
                        }
                        
                        $devices{$device}{"rd_ios"}     = $devices{$device}{"prev_rd_ios"}     = $rd_ios;       # Read I/O operations
                        $devices{$device}{"rd_merges"}  = $devices{$device}{"prev_rd_merges"}  = $rd_merges;    # Reads merged
                        $devices{$device}{"rd_sectors"} = $devices{$device}{"prev_rd_sectors"} = $rd_sectors;   # Sectors read
                        $devices{$device}{"rd_ticks"}   = $devices{$device}{"prev_rd_ticks"}   = $rd_ticks;     # Time in queue + service for read
                        $devices{$device}{"wr_ios"}     = $devices{$device}{"prev_wr_ios"}     = $wr_ios;       # Write I/O operations
                        $devices{$device}{"wr_merges"}  = $devices{$device}{"prev_wr_merges"}  = $wr_merges;    # Writes merged
                        $devices{$device}{"wr_sectors"} = $devices{$device}{"prev_wr_sectors"} = $wr_sectors;   # Sectors written
                        $devices{$device}{"wr_ticks"}   = $devices{$device}{"prev_wr_ticks"}   = $wr_ticks;     # Time in queue + service for write
                        $devices{$device}{"ticks"}      = $devices{$device}{"prev_ticks"}      = $ticks;        # Time of requests in queue
                        $devices{$device}{"aveq"}       = $devices{$device}{"prev_aveq"}       = $aveq;         # Average queue length
                        
                        last;
                    }
                }
                
                close(STAT);
            } else {
                warn("Error (diskstat) $?: \"$!\"");
            }
            
            # Gathering previous values (if available)
            if (open(PREV, prev_file($device))) {
                foreach my $prev (<PREV>) {
                    chomp($prev);
                    
                    next unless ($prev);
                    
                    # 40985 202299 2609936 528892 16965 59025 609112 1210420 233732 1748112
                    if (my ($time, $rd_ios, $rd_merges, $rd_sectors, $rd_ticks, $wr_ios, $wr_merges, $wr_sectors, $wr_ticks, $ticks, $aveq) = $prev =~ m/([\d\.]+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)/si) {
                        $devices{$device}{"prev_time"}       = $time;
                        $devices{$device}{"prev_rd_ios"}     = $rd_ios;
                        $devices{$device}{"prev_rd_merges"}  = $rd_merges;
                        $devices{$device}{"prev_rd_sectors"} = $rd_sectors;
                        $devices{$device}{"prev_rd_ticks"}   = $rd_ticks;
                        $devices{$device}{"prev_wr_ios"}     = $wr_ios;
                        $devices{$device}{"prev_wr_merges"}  = $wr_merges;
                        $devices{$device}{"prev_wr_sectors"} = $wr_sectors;
                        $devices{$device}{"prev_wr_ticks"}   = $wr_ticks;
                        $devices{$device}{"prev_ticks"}      = $ticks;
                        $devices{$device}{"prev_aveq"}       = $aveq;
                        
                        last;
                    }
                }
                
                close(PREV);
            }
            
            # Saving current values
            if (open(PREV, ">", prev_file($device))) {
                print(PREV
                    $devices{$device}{"time"}       . " " .
                    $devices{$device}{"rd_ios"}     . " " .
                    $devices{$device}{"rd_merges"}  . " " .
                    $devices{$device}{"rd_sectors"} . " " .
                    $devices{$device}{"rd_ticks"}   . " " .
                    $devices{$device}{"wr_ios"}     . " " .
                    $devices{$device}{"wr_merges"}  . " " .
                    $devices{$device}{"wr_sectors"} . " " .
                    $devices{$device}{"wr_ticks"}   . " " .
                    $devices{$device}{"ticks"}      . " " .
                    $devices{$device}{"aveq"}
                ) or warn("Error (diskstat) $?: \"$!\"");
                
                close(PREV);
            } else {
                warn("Error (diskstat) $?: \"$!\"");
            }
            
            # Calculating statistic
            $devices{$device}{"time"} = ($devices{$device}{"time"} - $devices{$device}{"prev_time"});
            if ($devices{$device}{"time"} < 0) {
                warn("Time since last run is negative! Setting it to 0...");
                $devices{$device}{"time"} = 0;
            }

            $devices{$device}{"n_ios"} = ($devices{$device}{"rd_ios"} + $devices{$device}{"wr_ios"}) - ($devices{$device}{"prev_rd_ios"} + $devices{$device}{"prev_wr_ios"});
            if ($devices{$device}{"n_ios"} < 0) {
                warn("Requests since last run are negative! Setting them to 0...");
                $devices{$device}{"n_ios"} = 0;
            }

            $devices{$device}{"n_ticks"} = ($devices{$device}{"rd_ticks"} + $devices{$device}{"wr_ticks"}) - ($devices{$device}{"prev_rd_ticks"} + $devices{$device}{"prev_wr_ticks"});
            if ($devices{$device}{"n_ticks"} < 0) {
                warn("Ticks since last run are negative! Setting them to 0");
                $devices{$device}{"n_ticks"} = 0;
            }

            $devices{$device}{"wait"} = $devices{$device}{"n_ios"} ? $devices{$device}{"n_ticks"} / $devices{$device}{"n_ios"} : 0;
            $devices{$device}{"util"} = $devices{$device}{"time"} ? 100 * ($devices{$device}{"ticks"} - $devices{$device}{"prev_ticks"}) / ($devices{$device}{"time"} * 1000) : 0;

            $results{"wait"}{$device} = $devices{$device}{"wait"};
            $results{"util"}{$device} = $devices{$device}{"util"};

            foreach my $metric (grep {$_ !~ /^prev/} keys %{$devices{$device}}) {
                if(defined($devices{$device}{"prev_$metric"})) {
                    $results{$metric}{$device} = $devices{$device}{$metric} - $devices{$device}{"prev_$metric"};
                }
            }

        }
    }
    
    my $lxd_self_short_name = undef;
    open(my $mtab_fh, "<", "/etc/mtab");
    if(<$mtab_fh> =~ qr{^/dev/lxd/(\S+)\s+/}) {
        $lxd_self_short_name = "lxd-$1";
        $lxd_self_short_name =~ s/\-\-+/-/g;
    }
    close($mtab_fh);
    
    foreach my $metric (sort keys %results) {
        foreach my $device (sort({$a cmp $b} keys(%{$results{$metric}}))) {
            my $device_name;
            if($device =~ /^dm\-/) {
                open(my $fh, "<", "/sys/block/$device/dm/name");
                chomp($device_name = <$fh>);
                $device_name =~ s/\-\-+/-/g;
                close($fh);
                ### Inside LXD container show info only about our own LVM
                next if defined($lxd_self_short_name) and ($device_name ne $lxd_self_short_name);
            }else {
                $device_name = $device;
            }
            my $sender = sprintf("%s.%s %.3f", $device_name, $metric, $devices{$device}{$metric});
            print "$sender\n";
            if(defined($lxd_self_short_name) and ($device_name eq $lxd_self_short_name)) {
                printf("%s.%s %.3f\n", "self", $metric, $devices{$device}{$metric});
            }
        }
    }
    
    close(PARTITIONS);
} else {
    warn("Error (diskstat) $?: \"$!\"");
}

1;
