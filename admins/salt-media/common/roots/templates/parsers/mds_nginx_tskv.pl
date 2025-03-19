#!/usr/bin/perl

use strict;
use warnings;

use Data::Dumper;

# libdata-messagepack-perl
use Data::MessagePack;
my %codes = ();
my $key = "";

$codes{"global_20x"} = 0;
$codes{"global_30x"} = 0;
$codes{"global_40x"} = 0;
$codes{"global_400"} = 0;
$codes{"global_499"} = 0;
$codes{"global_50x"} = 0;
$codes{"global_int_20x"} = 0;
$codes{"global_int_30x"} = 0;
$codes{"global_int_40x"} = 0;
$codes{"global_int_400"} = 0;
$codes{"global_int_499"} = 0;
$codes{"global_int_434"} = 0;
$codes{"global_int_50x"} = 0;
$codes{"global_timings"} = "";
$codes{"global_req_timings"} = "";
$codes{"global_int_timings"} = "";
$codes{"global_int_req_timings"} = "";

if (exists $ARGV[0] && $ARGV[0] eq "--cache")
{
    my $mp = Data::MessagePack->new();
    my $content;
    open(my $cache, '<', "/var/cache/libmastermind/mds.cache") or die "cannot open file";
    {
        local $/;
        $content = <$cache>;
    }
    close($cache);
    my $unp = $mp->unpack($content);

    foreach my $ns (keys %{$unp->{'namespaces_states'}})
    {
    $codes{"$ns\_20x"} = 0;
    $codes{"$ns\_30x"} = 0;
    $codes{"$ns\_40x"} = 0;
    $codes{"$ns\_400"} = 0;
    $codes{"$ns\_499"} = 0;
    $codes{"$ns\_434"} = 0;
    $codes{"$ns\_50x"} = 0;
    $codes{"$ns\_int_20x"} = 0;
    $codes{"$ns\_int_30x"} = 0;
    $codes{"$ns\_int_40x"} = 0;
    $codes{"$ns\_int_400"} = 0;
    $codes{"$ns\_int_50x"} = 0;
    $codes{"$ns\_HIT"} = 0;
    $codes{"$ns\_MISS"} = 0;
    $codes{"$ns\_ANY"} = 0;
    $codes{"$ns\_EXPIRED"} = 0;
    $codes{"$ns\_RENEW"} = 0;
    $codes{"$ns\_STALE"} = 0;
    $codes{"$ns\_timings"} = "";
    $codes{"$ns\_req_timings"} = "";
    $codes{"$ns\_int_timings"} = "";
    $codes{"$ns\_int_req_timings"} = "";
    }

    $codes{"default_L_timings"} = "";
}

while (<STDIN>)
{
    chomp;
    my %keys = ();
    my @fields = split '\t';
    foreach my $f (@fields)
    {
        (my $k, my $v) = split(/=/, $f);
        $keys{$k} = $v;
    }

    if (grep /^\/get/, $keys{'request'} && grep !/orig-url/, $keys{'request'})
    {
        my $ns = $keys{'request'};

        $ns =~ s/\/get[-\/]([-_\w]+)\/.*/$1/;

        # costyl dlya fotok
        if ($ns =~ /^\d+$/)
        {
            $ns = "default";
            if ($keys{'request'} =~ "0_STATIC") { $ns = "default_static"; }
            elsif ($keys{'request'} =~ "_L\$" && $keys{'upstream_response_time'} !~ "-") { $codes{"$ns\_L_timings"} .= "$keys{'upstream_response_time'} "; }
        }
        elsif ($ns =~ /^\/getinfo/)
        {
            $ns =~ s/\/getinfo[-\/]([-_\w]+)\/.*/$1/;
            $ns .= "_getinfo";
        }
     
        # status codes
        my $status = $keys{'status'};
        $key = "$ns\_$status";
        if ($status >= 500 && $status <= 599) {
            $codes{"$ns\_50x"} += 1;
            $codes{"global_50x"} += 1;
        }
        elsif ($status == 400) {
            $codes{"$ns\_400"} += 1;
            $codes{"global_400"} += 1;
            $codes{"global_40x"} += 1;
        }
	elsif ($status == 499) {
            $codes{"$ns\_499"} += 1;
            $codes{"global_499"} += 1;
            $codes{"global_40x"} += 1;
        }
	elsif ($status >= 400 &&  $status <= 499) {
            $codes{"$ns\_40x"} += 1;
            $codes{"global_40x"} += 1;
        }
        else
        {
            if ($status >= 300 && $status < 400) {
                $codes{"$ns\_30x"} += 1;
                $codes{"global_30x"} += 1;
            }
            elsif ($status >= 200 && $status < 300) {
                $codes{"$ns\_20x"} += 1;
                $codes{"global_20x"} += 1;
            }

        # response timings
            if ($keys{'upstream_response_time'} !~ "-") {
                $codes{"$ns\_timings"} .= "$keys{'upstream_response_time'} ";
                $codes{"global_timings"} .= "$keys{'upstream_response_time'} ";
            }
        # request timings
            if ($keys{'request_time'} !~ "-") {
                $codes{"$ns\_req_timings"} .= "$keys{'request_time'} ";
                $codes{"global_req_timings"} .= "$keys{'request_time'} ";
            }
        }
        
        # cache status
        if ($keys{'upstream_cache_status'} !~ "-")
        {
            my $cache_status = $keys{'upstream_cache_status'};
            $codes{"$ns\_$cache_status"} += 1; 
        }
    }

    elsif (grep /^\/(upload|put)/, $keys{'request'} or grep /orig-url/, $keys{'request'})
    {
        my $ns = $keys{'request'};
        $ns =~ s/\/(get|upload|put)[-\/]([-_a-z]+).*/$2/;
        if ($ns =~ /^\/(get|upload|put)/)
        {
            $ns = "default";
        }

        # status codes
        my $status = $keys{'status'};
        $key = "$ns\_int_$status";
        if ($status >= 500 && $status <= 599) {
            $codes{"$ns\_int_50x"} += 1;
            $codes{"global_int_50x"} += 1;
        }
	elsif ($status == 400) {
            $codes{"$ns\_int_400"} += 1;
            $codes{"global_int_400"} += 1;
            $codes{"global_int_40x"} += 1;
        }
	elsif ($status == 499) {
            $codes{"$ns\_int_499"} += 1;
            $codes{"global_int_499"} += 1;
            $codes{"global_int_40x"} += 1;
        }
	elsif ($status == 434) {
            $codes{"$ns\_int_434"} += 1;
            $codes{"global_int_434"} += 1;
            $codes{"global_int_40x"} += 1;
        }
        elsif ($status > 400 &&  $status <= 499) {
            $codes{"$ns\_int_40x"} += 1;
            $codes{"global_int_40x"} += 1;
        }
        else
        {
            if ($status >= 300 && $status < 400) {
                $codes{"$ns\_int_30x"} += 1;
                $codes{"global_int_30x"} += 1;
            }
            elsif ($status >= 200 && $status < 300 ) {
                $codes{"$ns\_int_20x"} += 1;
                $codes{"global_int_20x"} += 1;
            }

        # response timings
            if ($keys{'upstream_response_time'} !~ "-") {
                $codes{"$ns\_int_timings"} .= "$keys{'upstream_response_time'} ";
                $codes{"global_int_timings"} .= "$keys{'upstream_response_time'} ";
            }
        # request timings
            if ($keys{'request_time'} !~ "-") {
                $codes{"$ns\_int_req_timings"} .= "$keys{'request_time'} ";
                $codes{"global_int_req_timings"} .= "$keys{'request_time'} ";
            }
        }
    }

    elsif (grep /^\/update/, $keys{'request'})
    {
        my $ns = $keys{'request'};
        $ns =~ s/\/update[-\/]([-_a-z]+).*/$1/;
        if ($ns =~ /^\/update/)
        {
            $ns = "default";
        }

        # status codes
        my $status = $keys{'status'};
        $key = "$ns\_update_$status";
    }

    else { next; }
    if (exists $codes{$key})
    {
        $codes{$key} += 1;
    }
    else
    {
        $codes{$key} = 1;
    }

}
while ((my $k, my $v) = each %codes) {
    print "$k $v\n";
}
