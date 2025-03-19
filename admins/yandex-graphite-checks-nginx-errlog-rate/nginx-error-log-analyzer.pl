#!/usr/bin/perl
use strict;
use warnings;

my %severities;
my %reasons;
my %requests;
my %subrequests;
my %upstreams;
my $total_lines = 0;

while (<>) {
    my $line = $_;
    chomp($line);
    $total_lines++;
    my $next_data = "";

    if ( $line =~ /^
        (?<date>\d{4}+\/\d{2}\/\d{2}\s+\d{2}:\d{2}:\d{2}(?:\.\d{3})?)  \s+
        \[(?<severity>\S+)\]                                           \s+
        (?<pid>\d+)                                                    \#
        (?<tid>\d+)                                                   :\s+\*
        (?<cid>\d+)                                                    \s+
        (?<reason>.+)                                                 ,\s+
        client:\s+(?<client>[^,]*)                                    ,\s+
        server:\s+(?<server>[^,]*)
        (?<other_data>,.+)?
    /x) {
        $severities{$+{'severity'}}++;
        $reasons{$+{'reason'}}++;
        if(!$+{"other_data"}){next;}
        $next_data = $+{"other_data"};
    }
    else {
        print($line . "\n");
        next;
    }


    if ( $next_data  =~ /
        ,\s+request:\s+"(?<request>.+[\/]).*  \s+ (?<httpver>HTTP\/\d.\d)"
        (?<other_data>,\s+.+)
    /x) {
        $requests{$+{'request'}}++;
        if ( $next_data  =~ /subrequest:\s+"(?<subrequest>.+?)"/) {
            $subrequests{$+{'subrequest'}}++;
        }
        if ( $next_data  =~ /upstream:\s+"(?<upstream_prefix>https?:\/\/)?(?<upstream>[^\/]+?)"/) {
            $upstreams{$+{upstream}}++;
        }
    }
    elsif ( $next_data  =~ /
        ,\s+request:\s+"(?<request>.+[\/]).*"
    /x) {
        $requests{$+{'request'}}++;

    }
    else {

    }
    #2018/02/12 15:38:11 [crit] 603379#603379: *170321982 SSL_do_handshake() failed (SSL: error:14094456:SSL routines:ssl3_read_bytes:tlsv1 unsupported extension:SSL alert number 110) while SSL handshaking, client: 93.171.33.38, server: 0.0.0.0:443

}

printf("ngx_errlog_rate %d\n", $total_lines);

while (my ($k,$v)=each %severities){
    $k =~ s/[^a-zA-Z0-9]/_/g;
    $k =~ s/__/_/g;
    printf("nginx.error_log.severities.%s %d\n", $k, $v);
}

while (my ($k,$v)=each %reasons){
    $k =~ s/[^a-zA-Z0-9]/_/g;
    $k =~ s/__/_/g;
    printf("nginx.error_log.reasons.%s %d\n", $k, $v);
}

while (my ($k,$v)=each %upstreams){
    $k =~ s/[^a-zA-Z0-9]/_/g;
    $k =~ s/__/_/g;
    printf("nginx.error_log.upstreams.%s %d\n", $k, $v);
}

while (my ($k,$v)=each %subrequests){
    $k =~ s/[^a-zA-Z0-9]/_/g;
    $k =~ s/__/_/g;
    printf("nginx.error_log.subrequests.%s %d\n", $k, $v);
}

print("\n");

