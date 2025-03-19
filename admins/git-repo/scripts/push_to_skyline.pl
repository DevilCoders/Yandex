#!/usr/bin/perl

use warnings;
use strict;
use v5.14;
use Data::MessagePack;
use Time::Local;
use IO::Socket;
use Data::Dumper;
$|++;

sub daemonize {
    use POSIX;
    POSIX::setsid or die "setsid: $!";
    my $pid = fork();
    if ( $pid < 0 ) {
        die "fork: $!";
    }
    elsif ($pid) {
        exit 0;
    }
    chdir "/";
    umask 0;
    foreach ( 0 .. ( POSIX::sysconf(&POSIX::_SC_OPEN_MAX) || 1024 ) ) {
        POSIX::close $_;
    }
    open( STDIN,  "</dev/null" );
    open( STDOUT, ">/dev/null" );
    open( STDERR, ">&STDOUT" );
    return $$;
}

sub send_to_skyline ($) {
    my $dat = shift;
    my $mp  = Data::MessagePack->new();
    $mp = $mp->utf8;
    my $packed  = $mp->pack($dat);
    my $message = IO::Socket::INET->new(
        Proto    => "udp",
        PeerPort => 2025,
        PeerAddr => "127.0.0.1"
    ) or die "Can't make UDP socket: $@";
    $message->send($packed);
}

my $pid = daemonize;

my %mon2num = qw(
    jan 1  feb 2  mar 3  apr 4  may 5  jun 6
    jul 7  aug 8  sep 9  oct 10 nov 11 dec 12
);

#say $mon2num{lc('Oct')};

open( LOG, "+>>", "/tmp/push_to_skyline.log" );
select LOG;
$| = 1;

our $continue = 1;

while ($continue) {

    my $date;

    #    my $needed = 1;
    #    open( LOG, '/tmp/tomita-access.log' );

    my @log = qx|/usr/bin/timetail -n 60 -t syslog /var/log/tomita/nginx.log|;

    foreach my $log_line (@log) {

#Oct 30 21:47:25 mail-extract02e.mail.yandex.net nginx: [30/Oct/2013:21:47:25 +0400] mail-extract.mail.yandex.net 5.45.198.70 "POST /factextract/?e=abook-contacts,events&email=stepnyak12@mail.ru&mid=2410000000607897392&time=1381747912&types=4&uid=91444827 HTTP/1.1" 200 "-" "-" "-" "-" [proxy (-) : 127.0.0.1:1030 0.018 200 ] 798 179 0.020
        if ($log_line

#            =~ m|.*nginx: \[(\d+)/([[:alpha:]]+)/(\d+):(\d+):(\d+):(\d+).*mail-extract\.mail\.yandex\.net.*(\d+\.\d+)$|g
            =~ m|.*nginx: \[(\d+)/([[:alpha:]]+)/(\d+):(\d+):(\d+):(\d+).*mail-extract\.mail\.yandex\.net.*\[proxy \(-\) : 127\.0\.0\.1:1030 (\d+\.\d+)\ \d{3} \].*(\d+\.\d+)$|g
            )
        {
            my $metric           = 'nginx_response_time';
            my $metric_up        = 'nginx_upstream_response_time';
            my $day              = $1;
            my $mon              = $mon2num{ lc($2) };
            my $year             = $3;
            my $hour             = $4;
            my $min              = $5;
            my $sec              = $6;
            my $time_response_up = $7;
            my $time_response    = $8;

            #            $date = "$1.10.2013 $h:$3:$4";
            #            my ( $mday, $mon, $year, $hour, $min, $sec ) =
            #                split( /[\s.:]+/, $date );
            my $time = timelocal( $sec, $min, $hour, $day, $mon - 1, $year );

            my $dat    = [ $metric,    [ $time, $time_response ] ];
            my $dat_up = [ $metric_up, [ $time, $time_response_up ] ];
            send_to_skyline($dat);
            send_to_skyline($dat_up);

        }

#            =~ m|.*nginx: \[(\d+)/([[:alpha:]]+)/(\d+):(\d+):(\d+):(\d+).*mail-extract\.mail\.yandex\.net.*(\d+\.\d+)$|g
#            =~ m|.*nginx: \[(\d+)/([[:alpha:]]+)/(\d+):(\d+):(\d+):(\d+).*mail-extract\.mail\.yandex\.net.*\[proxy \(-\) : 127\.0\.0\.1:1030 (\d+\.\d+)\ \d{3} \]|g

    }

#Oct 30 23:30:16 mail-extract03f.mail.yandex.net /usr/bin/iexsrv[11194]: INFO: 2013-10-30 23:30:16.061 http_thread_resource.cpp:89 events=done;query=e=abook-contacts,events&email=subscribers@consultant.ru&mid=2220000004066480294&time=1383131631&uid=50330896;time=20784us;fact=0

    #my @tomita_log = qx|timetail -n 60 -t syslog /var/log/tomita/tomita.log|;

    #    close LOG;
    sleep 60;

    #$continue = 0;
}
