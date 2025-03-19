#! /usr/bin/perl

use Getopt::Std;

my $threshold = 3;
my $flag = 0;
my $error = "";
getopts('n:l:f:', \%opts);
my ($opt_sec,$opt_limit,$opt_log) = (60, 10, "/var/log/lighttpd/access.log");
$opt_sec = $opts{'n'} if defined $opts{'n'};
$opt_limit = $opts{'l'} if defined $opts{'l'};
$opt_log =  $opts{'f'} if defined $opts{'f'};
if (! -e $opt_log ) { print "1;Ok,logfile $opt_log doesn't exist"; exit; }

system "timetail -n$opt_sec $opt_log | /usr/bin/500-juggler.pl > /dev/null > /var/tmp/parsed_log";
open LOGFILE, '<', '/var/tmp/parsed_log' or die "1;Ok,Cannot open file: $!\n";

my ($total_answers200,$total_answers500,$total_answers404) = (0, 0, 0);
while (!eof(LOGFILE)) {
        $line = readline LOGFILE;
        ($host, $answers200, $answers500, $answers404) = split ("\t", $line);
        my $allover = $answers200 + $answers500 + $answers404;
        $total_answers200 += $answers200;
        $total_answers500 += $answers500;
        $total_answers404 += $answers404;
}

my $rps = int (($total_answers200 + $total_answers404 + $total_answers500) / 60);

print "200.total $total_answers200\n500.total $total_answers500\nrps.total $rps";

close LOGFILE;
