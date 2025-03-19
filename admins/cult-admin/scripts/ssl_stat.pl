#!/usr/bin/perl

# Show ssl versions statistics by tskv log.
# Usage: cat access.taskv.log | ./ssl_stat.pl

use strict;

use Getopt::Long;

sub get_values {
        $_[0] =~ /(\S+?)=([\S ]*)\t*/g;
}

my %ver_cnt;
my %ver_prc;

my $nr = 0;

my $vhost = ".*";
GetOptions ("vhost=s" => \$vhost);

while (<>) {
        my %pairs = &get_values("$_");

        $nr++;

        if ($pairs{'vhost'} =~ m/$vhost/) {
                if (!exists $pairs{'ssl_protocol'}) {
                        $ver_cnt{'no_data'}++;
                } else {
                        $ver_cnt{"$pairs{'ssl_protocol'}"}++;
                }
        }
}

print "version" . "\t|\t" . "count" . "\t|\t" . "percentage" . "\n";
print "-" x 42 . "\n";

foreach my $i (sort keys %ver_cnt) {
        $ver_prc{"$i"} = $ver_cnt{"$i"} / $nr * 100;
        printf("%s" . "\t|\t" . "%6d" . "\t|\t" . "%6.3f" . "%\n", $i, $ver_cnt{"$i"}, $ver_prc{"$i"});
}
