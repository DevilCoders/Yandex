#!/usr/bin/perl -w

use strict;

my $prev_file = "/tmp/graphite.iostat.%s.prev";

sub prev_file($) {
        my $fname = shift;
        $fname =~ s#/#_#g;
        return sprintf($prev_file, $fname);
}

# Array of block devices
my %devices = ();

if (open(MPOINTS, "< /etc/mtab")) {
    my %results = ();
    my %diskstats;
    
    foreach my $point (<MPOINTS>) {
        chomp($point);
        
        next unless ($point);
        
        # /dev/md0 / ext3 rw,relatime,errors=remount-ro 0 0
        if (my ($full_device, $device) = $point =~ m{^(/dev/(\S+)) /\S* \S+ }s) {
            $full_device = $device = "dm-" . ( (stat($full_device))[6] & 0xff ) if $device =~ m{^mapper/};
            
            # /dev/lxd/containers_teamcity--agent05h----media / ext4 rw 0 0
            if($device =~ m{^lxd/}) {
                use File::Basename;
                my $lvm_rootfs_name = basename($device);
                $lvm_rootfs_name =~ s/\-/--/g;
                $full_device = qx{fgrep -l lxd-$lvm_rootfs_name /sys/block/dm-*/dm/name};
                $full_device = dirname(dirname($full_device));
                next unless(defined($full_device) and ($full_device ne ""));

                if (open(STAT, "$full_device/stat")) {
                    foreach my $stat (<STAT>) {
                        chomp($stat);
                        $diskstats{$device} = $stat;
                    }
                    close(STAT);
                }
            }

            unless ( %diskstats ) {
                if (open(STAT, "/proc/diskstats")) {
                    foreach my $stat (<STAT>) {
                        chomp($stat);
                        next unless ($stat);
                        my ($dev, $val) = (split(/\s+/, $stat, 5))[3,4];
                        $diskstats{$dev} = $val;
                    }
                    close(STAT);
                } else {
                    warn("Error (iostat) $?: \"$!\"");
                }
            }
            # Gathering current values
            if ( exists($diskstats{$device}) ) {
                my $stat = $diskstats{$device};
                    
                # 40985 202299 2609936 528892 16965 59025 609112 1210420 0 233732 1748112
                my ($rd_ios, $rd_merges, $rd_sectors, $rd_ticks, $wr_ios, $wr_merges, $wr_sectors, $wr_ticks, $ticks, $aveq);
                if ((($rd_ios, $rd_merges, $rd_sectors, $rd_ticks, $wr_ios, $wr_merges, $wr_sectors, $wr_ticks, $ticks, $aveq) = $stat =~ m/(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+(\d+)\s+(\d+)/si) ||
                    ((($rd_ios, $rd_sectors, $wr_ios, $wr_sectors) = $stat =~ m/(\d+)\s+(\d+)\s+(\d+)\s+(\d+)/si) &&
                     (($rd_merges, $rd_ticks, $wr_merges, $wr_ticks, $ticks, $aveq) = (0, 0, 0, 0, 0, 0)))) {
                    # Gathering time
                    if (open(CPU, "< /proc/stat")) {
                        my ($us, $ni, $sy, $id, $io, $hi, $si) = <CPU> =~ m/^cpu\s+(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)/si;
                            
                        $devices{$device}{"time"}   = $devices{$device}{"prev_time"}       = $us + $ni + $sy + $id + $io + $hi + $si;   # Current cpu time
                            
                        close(CPU);
                    } else {
                        die("Error (iostat) $?: \"$!\"");
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
                        
                }
            }
            
            # Gathering previous values (if available)
            if (open(PREV, prev_file($device))) {
                foreach my $prev (<PREV>) {
                    chomp($prev);
                    
                    next unless ($prev);
                    
                    # 40985 202299 2609936 528892 16965 59025 609112 1210420 233732 1748112
                    if (my ($time, $rd_ios, $rd_merges, $rd_sectors, $rd_ticks, $wr_ios, $wr_merges, $wr_sectors, $wr_ticks, $ticks, $aveq) = $prev =~ m/(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)\s(\d+)/si) {
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
                ) or warn("Error (iostat) $?: \"$!\"");
                
                close(PREV);
            } else {
                warn("Error (iostat) $?: \"$!\"");
            }
            
            # Calculating statistic
            $devices{$device}{"time"} = ($devices{$device}{"time"} - $devices{$device}{"prev_time"}) / 1000;
            
            $devices{$device}{"rd_sectors"} -= $devices{$device}{"prev_rd_sectors"};
            $devices{$device}{"wr_sectors"} -= $devices{$device}{"prev_wr_sectors"};
            $devices{$device}{"rd_sectors"} += 2**32 if $devices{$device}{"rd_sectors"} < 0;
            $devices{$device}{"wr_sectors"} += 2**32 if $devices{$device}{"wr_sectors"} < 0;
            $devices{$device}{"rd_sectors"} = $devices{$device}{"time"} ? $devices{$device}{"rd_sectors"} / $devices{$device}{"time"} : 0;
            $devices{$device}{"wr_sectors"} = $devices{$device}{"time"} ? $devices{$device}{"wr_sectors"} / $devices{$device}{"time"} : 0;
            
            $results{$device} = $full_device;
        }
    }
    
    foreach my $device (sort({$a cmp $b} keys(%results))) {
        my $full_device = $results{$device};
        printf("%s.rd %.3f\n", $device, $devices{$device}{"rd_sectors"});
        printf("%s.wr %.3f\n", $device, $devices{$device}{"wr_sectors"});
    }
    
    close(MPOINTS);
} else {
    warn("Error (iostat) $?: \"$!\"");
}


1;
