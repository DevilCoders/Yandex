#! /usr/bin/perl
use strict;
use warnings;
use Time::Local;
use POSIX;

my ($time_begin, $time_end, $sec, $rps);
my (@log, @log_sorted, @req_times, @log_rotate);
my %answer_code;
my %mon2num = qw(
    jan 1  feb 2  mar 3  apr 4  may 5  jun 6
    jul 7  aug 8  sep 9  oct 10 nov 11 dec 12
);

@log = qx|timetail -n 300 /var/log/nginx/access.log|;
@log_sorted = sort @log;

if ($log_sorted[0] =~ m|\[(\d+)/([[:alpha:]]+)/(\d+):(\d+):(\d+):(\d+).*|g ) {
        my $day           = $1;
        my $mon           = $mon2num{ lc($2) };
        my $year          = $3;
        my $hour          = $4;
        my $min           = $5;
        my $sec           = $6;
        $time_begin = timelocal( $sec, $min, $hour, $day, $mon - 1, $year );
}

if ($log_sorted[$#log_sorted] =~ m|\[(\d+)/([[:alpha:]]+)/(\d+):(\d+):(\d+):(\d+).*|g ) {
        my $day           = $1;
        my $mon           = $mon2num{ lc($2) };
        my $year          = $3;
        my $hour          = $4;
        my $min           = $5;
        my $sec           = $6;
        $time_end = timelocal( $sec, $min, $hour, $day, $mon - 1, $year );
}

$sec = $time_end - $time_begin;
if ($sec < 295) {
        my $sec_rotate = 300 - $sec;
        @log_rotate = qx|timetail -n $sec_rotate /var/log/nginx/access.log.1|;
        @log = (@log, @log_rotate);
        @log_sorted = sort @log;
}

foreach my $log_sorted_line ( @log_sorted ) {
        # [18/Mar/2015:21:01:11 +0300] root.test.yandex.com 2a02:6b8:0:107:e0e7:9abb:1598:ee49 "GET /command/info HTTP/1.1" 200 "https://root.test.yandex.com/download" "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:36.0) Gecko/20100101 Firefox/36.0" "yandexuid=6499870131425467452" 0.014 - 481
        if ($log_sorted_line =~ m|\[[0-9]{2}/[a-zA-Z]{3}/[0-9]{4}:([0-9:]+) \+[0-9]+\] ([\w\.\/:-]+) ([\w:\.]+) \"([A-Z]+ .*) HTTP/[0-9\.]+\" ([\w]{3}) \"[\w:\.\/-]+\" \".+\" \".+\" ([\w\.]+) [\w-]+ \w+|g ) {
                my $time = $1;
                my $url = $2;
                my $ip = $3;
                my $req = $4;
                my $ans_code = $5;
                #my $upstream = $6;
                #my $request_length = $7;
                #my $bytes_sent = $8;
                my $request_time = $6;
                $answer_code{$ans_code}++;
                push (@req_times, $request_time);
                #print ("$upstream\n");
        }
}

my $count = (scalar @req_times);
my $p90 = POSIX::floor($count*0.90);
my $p95 = POSIX::floor($count*0.95);
my $p98 = POSIX::floor($count*0.98);
my $p99 = POSIX::floor($count*0.99);

@req_times = sort {$a <=> $b} @req_times;

print ("percent.90 $req_times[$p90]\n");
print ("percent.95 $req_times[$p95]\n");
print ("percent.98 $req_times[$p98]\n");
print ("percent.99 $req_times[$p99]\n");

$rps = POSIX::floor((scalar @log_sorted)/$sec);
print ("rps $rps\n");

foreach my $ans_code ( keys %answer_code ) {
        print ("http_" . "$ans_code " . $answer_code{$ans_code}*100/(scalar @log_sorted) . "\n");
}
