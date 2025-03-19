package TimeTail;

use 5.008004;
use strict;
use warnings;

sub print_part($$$) {
    my ($fh, $s, $e) = @_;
    my $portion = 128*1024;

    seek($fh, $s, 0);
    my $remain = $e - $s;
    my $buf;
    while ($remain) {
        $portion = $remain if $portion > $remain;
        my $rc = read($fh, $buf, $portion);
        $remain -= $rc;
        print $buf;
    }
}

sub Timetail($$$$$$) {
    my $logfile = shift;
    my $fromtime = shift;
    my $deltatime = shift;
    my $timere = shift;
    my $max_junk = shift;
    my $debug = shift;

    my $LOG;
    open($LOG, "$logfile");

    my $now = $fromtime == 0 ? time() : $fromtime;
    my $timeborder = $now - $deltatime;
    my $ts = File::TimeSearch->new(file => $LOG, timere => $timere, max_junk => $max_junk, debug => $debug);
    if (defined $ts){ #any valid line found in logfile
        my $start = $ts->find_pos($timeborder);
        my $end = $ts->find_pos($now);
        print_part($LOG, $start, $end);
    }
    close ($LOG);
}

1;
