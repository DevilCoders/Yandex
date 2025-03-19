#!/usr/bin/perl -w

use strict;
use warnings;

use LWP;
use Net::INET6Glue::INET_is_INET6;
use Sys::Hostname::Long;
use XML::Simple;
use Data::Dumper;

our @opts = ("host", "port", "url", "name", "title", "warn", "crit");

sub parse_config {  # parse config file: checks and settings
    my $fd = shift;
    my %config = (); 
    my $num = 0;
    my $c_regexp = join ("|", @opts);

    foreach my $opt (@opts) {
        $config{$opt} = undef;
    }  

    foreach my $line (<$fd>) {
        $num++;
        chomp($line);
        next unless ($line);
        if ($line =~ m/^#/) {
            next;
        } elsif (my ($name, $value) = $line =~ m/^($c_regexp):\s+(.*)\s?$/s){
            if (! $config{$name}) {
                if ($name eq 'host' && $value =~ m/HOSTNAME_LONG/) {
                    $value = hostname_long;
                }
                $config{$name} = $value;
            } else {
                print "2; Line $num: option \"$name\" already defined. ";
                return;
            }
        } else {
            print "2; Can not parse line $num. ";
            return;
        }
    }
    return %config;
}

sub check_config {  # check result config: some settings are required
    my (%config) = @_; 
    
    foreach my $opt (@opts) {
        if (! $config{$opt}) {
            print "2; '$opt' must be defined. ";
            return;
        } 
    }
    return "ok";
}


my $usage = "$0 CONFIG";

my $rcode = 0;
my $rmessage = "cassandra-mx4j: ";

my %current_metrics = (); # parsed nodetool output
my $config_file = "/root/cassandra-mx4j.conf";
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

my $ua = LWP::UserAgent->new;
my $resp = $ua->get( "http://$config{'host'}:$config{'port'}/$config{'url'}" );

if ( ! $resp->is_success ) {
    print "2; Error while connecting to mx4j\n";
    exit 2;
}

my $ref  = XMLin($resp->content);

my $new_val = $ref;
foreach my $pname (split (',', $config{'name'})) {
    chomp $pname;
    if (defined $new_val->{$pname}) {
        $new_val = $new_val->{$pname};
    }
}

if ($new_val >= $config{'crit'}) {
    $rcode = 2;
    $rmessage .= "crit ";
} elsif ($new_val >= $config{'warn'}) {
    $rcode = 1;
    $rmessage .= "warn ";
}

printf "%d; %s = %.2f (%0.2f/%0.2f)\n", $rcode, $config{'title'}, $new_val, $config{'warn'}, $config{'crit'};
exit $rcode;

