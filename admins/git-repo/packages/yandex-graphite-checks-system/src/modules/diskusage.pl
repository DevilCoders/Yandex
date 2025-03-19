#!/usr/bin/perl -w

use strict;

if (open(DF, "/bin/df -a -P -l -k -t ext2 -t ext3 -t ext4 -t xfs -t jfs -t simfs -t tmpfs -t btrfs 2>/dev/null  | /bin/grep -v Filesystem |")) {
    foreach my $df (<DF>) {
        chomp($df);

        next unless ($df);

        # /dev/md/d0           141110136  33760868 100181268      26% /local
        if (my (undef, $diskspace, $used, $avail, $percentage, $point) = $df =~ m/(\w+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)%\s+(.*)$/si) {
            my $full_point = $point;
            $point =~ s/^\/$/ROOT/si;
            $point =~ s/^\///si;
            $point =~ s/[\/\.]/_/gsi;

            my $s = sprintf("%s.diskspace %d", $point, $diskspace * 1024);
            my $u = sprintf("%s.used %d", $point, $used * 1024);
            my $a = sprintf("%s.avail %d", $point, $avail * 1024);
            my $p = sprintf("%s.percentage %d", $point, $percentage);

            print "$s\n";
            print "$u\n";
            print "$a\n";
            print "$p\n";
        }
    }
} else {
    warn("Error (diskusage) $?: \"$!\"");
}

1;
