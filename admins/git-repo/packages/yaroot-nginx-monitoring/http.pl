#! /usr/bin/perl

use Getopt::Std;
use Config::Simple;

my $conf = "/etc/yandex/juggler-500-check.conf";

Config::Simple->import_from("$conf", \%config);

my $threshold;
if (defined $config{'default.threshold'}) {
        $threshold = $config{'default.threshold'};
} else {
        $threshold = 3;
}

my $flag = 0;
getopts('n:l:f:', \%opts);
my ($opt_sec,$opt_limit,$opt_log) = (300, 100, "/var/log/nginx/access.log");
$opt_sec = $opts{'n'} if defined $opts{'n'};
$opt_limit = $opts{'l'} if defined $opts{'l'};
$opt_log =  $opts{'f'} if defined $opts{'f'};
if (! -e $opt_log ) { print "PASSIVE-CHECK:http;0;Ok,logfile $opt_log doesn't exist"; exit; }

system "timetail -n$opt_sec $opt_log | /usr/bin/500-juggler.pl > /dev/null > /var/tmp/parsed_log";
open LOGFILE, '<', '/var/tmp/parsed_log' or die "PASSIVE-CHECK:http;0;Ok,Cannot open file: $!\n";

my ($total_answers200,$total_answers500,$total_answers404) = (0, 0, 0);
while (!eof(LOGFILE)) {
        $line = readline LOGFILE;
        ($host, $answers200, $answers500, $answers404) = split ("\t", $line);
        my $allover = $answers200 + $answers500 + $answers404;
        $total_answers200 += $answers200;
        $total_answers500 += $answers500;
#       $total_answers404 += $answers404;
        my $perc = ($allover == 0)? 0 : $answers500 / $allover * 100;
        $perc = int($perc * 100) / 100;
        if (($perc > $threshold) && ($answers500 > $opt_limit)) {
                print "PASSIVE-CHECK:http;2;Too many 500 errors in $host ($perc%). Count: $answers500\n";
                $flag = 1;
        }
}

if ($flag == 0 ) {
        print "PASSIVE-CHECK:http;0;Ok, total200=$total_answers200, total50X=$total_answers500";
}

close LOGFILE;
