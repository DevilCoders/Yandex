#!/usr/bin/perl -w

use strict;
use warnings;

sub get_metrics { # parse "nodetool status" output
    my $data = shift;
    my %metrics = ();
    my $dc = undef;
    foreach my $line (split /^/, $data) {
        chomp($line);
        next unless ($line);
        
        if (my ($status, $state, $address, $load, $load_unit, $tokens, $owns, $id, $rack) = $line =~
                    m/^([A-Z])([A-Z])\s+([^\s]+)\s+([\d\.]+)\s+([A-Z]+)\s+(\d+)\s+([\d\.]+)\%\s+([^\s]+)\s+(\w+)/) {
                    #UN  2a02:6b8:0:1a4f:0:0:0:7b  215.16 MB  256     35.2%             b4232565-4f46-481b-940d-329d70ea37b6  RACK1
            $metrics{$dc}{$address}{'status'} = $status;
            $metrics{$dc}{$address}{'state'} = $state;
            $metrics{$dc}{$address}{'address'} = $address;
            $metrics{$dc}{$address}{'load'} = $load;
            $metrics{$dc}{$address}{'load_unit'} = $load_unit;
            $metrics{$dc}{$address}{'tokens'} = $tokens;
            $metrics{$dc}{$address}{'owns'} = $owns;
            $metrics{$dc}{$address}{'id'} = $id;
            $metrics{$dc}{$address}{'rack'} = $rack;
        } elsif ( $line =~ m/^Datacenter: (\w+)/) { $dc = $1;};
    }
    return %metrics;
}

sub parse_config {  # parse config file: checks and settings
    my $fd = shift;
    my %config = (); 
    my $num = 0;
    foreach my $line (<$fd>) {
        $num++;
        chomp($line);
        next unless ($line);
        if ($line =~ m/^#/) {
            next;
        } elsif (my ($check, $rule, $warn, $crit) = $line =~ m/^(?:check_(status|state|owns)):\s+\"(\d+|[A-Z]+)\"\s+warn=(\d+)\s+crit=(\d+)/s) {
            # TODO: verify settings
            if (! $config{'check'}{$check}) {
                $config{'check'}{$check}{'rule'}= $rule;
                $config{'check'}{$check}{'warn'} = $warn;
                $config{'check'}{$check}{'crit'} = $crit;
            } else {
                print "2; Line $num: check \"$check\" already defined. ";
                return;
            }
        } elsif ($line =~ m/^keyspaces:\s+(.*)/) {
            foreach my $ks (split / /, $1) {
                $config{'keyspace'}{$ks} = 0;
            }
        } else {
            print "2; Can not parse line $num. ";
            return
        }
    }   
    return %config;
}

sub check_config {  # check result config: some settings are required
    my (%config) = @_; 

    if (! $config{'check'} ) { 
        print "2; No checks are defined. ";
    } elsif (! $config{'keyspace'} ){
        print "2; At least one keyspace expected. ";
    } else {
        return "ok";
    }

    return;
}


my $usage = "$0 CONFIG";

my $rcode = 0;
my $rmessage = "nodetool status";

my %current_metrics = (); # parsed nodetool output
my $config_file = "/etc/monitoring/cassandra-check-status.conf";
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


my %report = (); 

foreach my $ks (keys $config{'keyspace'}) {
    my $result = `nodetool status $ks`;

    if ( $? != 0 ) {
        print "2; Error while call \"nodetool status $ks\".\n";
        exit 2;
    }

    my %metrics = &get_metrics($result);

    my %quota = ();
    foreach my $dc (keys %metrics) {
        my $num = keys $metrics{$dc};
        $quota{$dc} = 100/$num;
    }

    foreach my $check ('state', 'status', 'owns') {
        $report{$ks}{$check}{'count'} = 0;
        $report{$ks}{$check}{'message'} = ''; 
    }

    foreach my $dc (keys %metrics) {
        foreach my $node (keys $metrics{$dc}) {
            if ( $config{'check'}{'status'} && (index ($config{'check'}{'status'}{'rule'}, $metrics{$dc}{$node}{'status'}) != -1) ) {
                $report{$ks}{'status'}{'count'}++;
                $report{$ks}{'status'}{'message'} .= "$node($dc) = $metrics{$dc}{$node}{'status'}, ";
            } 
            if ( $config{'check'}{'state'} && (index ($config{'check'}{'state'}{'rule'}, $metrics{$dc}{$node}{'state'}) != -1 )) {
                $report{$ks}{'state'}{'count'}++;
                $report{$ks}{'state'}{'message'} .= "$node($dc) = $metrics{$dc}{$node}{'state'}, ";
            } 
            if ( $config{'check'}{'owns'} && ( abs($metrics{$dc}{$node}{'owns'}-$quota{$dc}) > $config{'check'}{'owns'}{'rule'} )) {
                $report{$ks}{'owns'}{'count'}++;
                $report{$ks}{'owns'}{'message'} .= "$node($dc) = $metrics{$dc}{$node}{'owns'}%, ";
            }
        }
    }
}


foreach my $ks (keys %report) {
    $rmessage .= "; $ks ";
    foreach my $check (keys $report{$ks}) {
        my $check_val = $report{$ks}{$check}{'count'};
        my $check_msg = $report{$ks}{$check}{'message'};

        my $crit_threshold = $config{'check'}{$check}{'crit'};
        my $warn_threshold = $config{'check'}{$check}{'warn'};

        if ( $crit_threshold && ($check_val >= $crit_threshold) ) {
            $rcode = 2 if ($rcode < 2);
            $rmessage .= "check_$check crit: $check_msg";
        } elsif ( $warn_threshold && ($check_val >= $warn_threshold) ) {
            $rcode = 1 if ($rcode < 1);
            $rmessage .= "check_$check warn: $check_msg";
        }
    }
}

print "$rcode; $rmessage\n";
exit $rcode;

