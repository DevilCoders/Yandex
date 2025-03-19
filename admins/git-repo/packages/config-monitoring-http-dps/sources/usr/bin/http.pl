#! /usr/bin/perl

use Getopt::Std;

my $threshold = 3;
my $flag = 0;
my $error = "";
getopts('n:l:f:', \%opts);
my ($opt_sec,$opt_limit,$opt_log) = (300, 100, "/var/log/lighttpd/access.log");
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
#       $total_answers404 += $answers404;
        my $perc = ($allover == 0)? 0 : $answers500 / $allover * 100;
        $perc = int($perc * 100) / 100;
        if (($perc > $threshold) && ($answers500 > $opt_limit)) {
                $error=$error." $host($perc%/$answers500),";
                $flag = 1;
        }
}

if ($flag == 0 ) {
        print "0;Ok, total200=$total_answers200, total50X=$total_answers500";
} elsif ($flag == 1 ) {
        $error =~ s/yandex/y/g;
        $error =~ s/yandex-team/y-t/g;
        $error =~ s/,\z//;
        print "2;Too many 500 errors in$error\n";
} else {
        print "1;smth strange";
}

close LOGFILE;
