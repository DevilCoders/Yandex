#!/usr/bin/perl -w

use strict;
use warnings;

sub get_metrics { # parse "nodetool tpstats" output
    my $fd = shift;
    my %metrics = ();
    foreach my $line (<$fd>) {
        chomp($line);
        next unless ($line);
        
        if (my ($key, $active, $pending, $completed, $blocked, $atblocked) = $line =~ m/^([a-zA-Z]+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)/s) {
            $metrics{'pool'}{$key}{'Active'} = $active;
            $metrics{'pool'}{$key}{'Pending'} = $pending;
            $metrics{'pool'}{$key}{'Completed'}= $completed;
            $metrics{'pool'}{$key}{'Blocked'} = $blocked;
            $metrics{'pool'}{$key}{'All time blocked'} = $atblocked;
        } elsif (($key, my $value) = $line =~ m/^([A-Z_]+)\s+(\d+)/s) {
            $metrics{'msg'}{$key}{'Dropped'} = $value;
        }
    }
    return %metrics;
}

sub parse_config { # parse config file: checks and settings
    my $fd = shift;
    my %config = (); 
    my $num = 0;
    foreach my $line (<$fd>) {
        $num++;
        chomp($line);
        next unless ($line);
        if ($line =~ m/^#/) {
            next;
        } elsif (my ($check, $type, $metric, $status, $count, $result) = $line =~ 
                m/^(?:check_(size|inc)):\s+(pool|msg)\s+\"([^\"]+)\"\s+\"([^\"]+)\"\s+(\d+)\s+(warn|crit)/s) {

            $config{'check'}{$check}{$type}{$metric}{$status}{$result} = $count;
        } elsif (my ($tfile, $fpath) = $line =~ m/^(?:file_(tmp|prev)):\s+([^\0]+)/) {
            $config{'files'}{$tfile} = $fpath;
        } else {
            print "2; Can not parse config file: line $num. ";
            return;
        }
    }
    return %config;
}

sub check_config { # check result config: some settings are required
    my (%config) = @_;

    if ( ! $config{'files'}{'prev'} ) {
        print "2; file_prev is not defined. ";
    } elsif (! $config{'files'}{'tmp'}) {
        print "2; file_tmp is not defined. ";
    } elsif (! $config{'check'} ) {
        print "2; No checks are defined. ";
    } else {
        return "ok";
    }

    return;
}

my $usage = "$0 CONFIG_FILE";

my $rcode = 0;
my $rmessage = "nodetool tpstats";

my %previos_metrics = (); # parsed result of nodetool last call 
my %current_metrics = (); # current result

my $config_file = "/etc/monitoring/cassandra-check-tpstats.conf"; # default config file
my %config = ();

if (scalar @ARGV == 1) {
    my $arg = shift @ARGV;
    if ($arg eq '-h') {
        print "$usage\n";
        exit 2;
    } else { $config_file = $arg; }
}

if (open (my $fdconfig, "<$config_file")) {
    %config = &parse_config($fdconfig);
    close ($fdconfig);
} else {
    print "2; Can not open config file \"$config_file\": $!\n";
    exit 2;
}

if (! %config) {
    print "2; Error while parsing config file \"$config_file\"\n";
    exit 2;
} 

if (! &check_config(%config)) {
    print "2; Error while checking config\n";
    exit 2;
}

if (open (my $old_result, "<$config{'files'}{'prev'}")) {
    %previos_metrics = &get_metrics($old_result);
    close ($old_result);
}

if (system ("nodetool tpstats > $config{'files'}{'tmp'}") ) {
    print "2; Error while call \"nodetool tpstats\".\n";
    exit 2;
}

if (open (my $new_result, "<$config{'files'}{'tmp'}")) {
    %current_metrics = &get_metrics($new_result);
    close ($new_result);
    if (! rename $config{'files'}{'tmp'}, $config{'files'}{'prev'}) {
        print "2; Can not mv $config{'files'}{'tmp'} to $config{'files'}{'prev'}\n";
        exit 2;
    } 
} else {
    print "2; Can not open file $config{'files'}{'tmp'} $!\n";
    exit 2;
}

if ( ! %previos_metrics ) {
    print "1; Looks line first run.\n";
    exit 1;
}

##
# Start checks
##

foreach my $check (keys $config{'check'}) {
    foreach my $type (keys $config{'check'}{$check}) {
        foreach my $metric (keys $current_metrics{$type}) {
            foreach my $status (keys $current_metrics{$type}{$metric}) {
                my ($check_val, $check_msg, $old_val) = undef;
                
                my $new_val = $current_metrics{$type}{$metric}{$status};

                my $warn_threshold = $config{'check'}{$check}{$type}{$metric}{$status}{'warn'} ||
                                     $config{'check'}{$check}{$type}{$metric}{'default'}{'warn'} ||
                                     $config{'check'}{$check}{$type}{'default'}{$status}{'warn'} ||
                                     $config{'check'}{$check}{$type}{'default'}{'default'}{'warn'};
                my $crit_threshold = $config{'check'}{$check}{$type}{$metric}{$status}{'crit'} ||
                                     $config{'check'}{$check}{$type}{$metric}{'default'}{'crit'} ||
                                     $config{'check'}{$check}{$type}{'default'}{$status}{'crit'} ||
                                     $config{'check'}{$check}{$type}{'default'}{'default'}{'crit'};

                if ($check eq 'size') {
                    $check_val = $new_val;
                    $check_msg = "$new_val";
                } elsif ($check eq 'inc') {
                    $old_val = $previos_metrics{$type}{$metric}{$status};
                    $check_val = $new_val - $old_val;
                    $check_msg = "$old_val -> $new_val";
                }

                if ( $crit_threshold && ($check_val >= $crit_threshold) ) {
                    $rcode = 2 if ($rcode < 2);
                    $rmessage .= " check_$check crit: $metric($status): $check_msg,";
                } elsif ( $warn_threshold && ($check_val >= $warn_threshold) ) {
                    $rcode = 1 if ($rcode < 1);
                    $rmessage .= " check_$check warn: $metric($status): $check_msg,";
                }
            }
        }
    }
}


print "$rcode; $rmessage\n";
exit $rcode;


