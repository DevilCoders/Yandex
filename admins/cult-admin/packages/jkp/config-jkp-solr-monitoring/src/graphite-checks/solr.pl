#!/usr/bin/perl

use strict;
use warnings;
use JSON qw( decode_json );
use LWP::Simple;
use Data::Dumper;
use Sys::Hostname;

my $host = hostname;
my $url = "http://localhost:8983/solr/admin/cores?action=STATUS&wt=json";

my $json = get( $url );
unless(defined $json){
    print "Solr is not running!\n";
    exit 1;
}

my $decoded = decode_json($json);

my $data=$decoded->{'status'};
for my $core_name (keys(%$data)) {
    my $core_size = $data->{$core_name}->{'index'}->{'sizeInBytes'};
    print $core_name.".sizeInBytes ".$core_size."\n";

    my $stat_url="http://localhost:8983/solr/".$core_name."/admin/mbeans?stats=true&wt=json";
    my $stat_json = get ( $stat_url);
    my $decoded_mbeans = decode_json($stat_json);
    my @mbeans = @{$decoded_mbeans->{'solr-mbeans'} };

    my $cache_el = 0;
    my $queryhandler_el = 0;
    my $core_el = 0;

    foreach my $f ( @mbeans) {
        if($cache_el == 1){
#           print Dumper $f;
            foreach my $g ( keys(%$f)) {
                next if $g eq "fieldCache";
                my @params = ('lookups','hits','inserts','evictions','hitratio');
                foreach my $param (@params) {
                    print $core_name.".cache.".$g.".cumulative_".$param." ".$f->{$g}->{"stats"}->{"cumulative_".$param}."\n";
                }
                print $core_name.".cache.".$g.".size ".$f->{$g}->{"stats"}->{"size"}."\n";
            }
            $cache_el = 0;
        }
        if($queryhandler_el == 1){
            foreach my $g ( keys(%$f)) {
                if ($g eq "/select" || $g eq "/update"){
                        my @params = ('requests','avgRequestsPerSecond','avgTimePerRequest','75thPcRequestTime','95thPcRequestTime','99thPcRequestTime','errors','timeouts');
                        foreach my $param (@params) {
                                my $key = substr $g, 1;
                                my $value = sprintf ("%.3f", $f->{$g}->{"stats"}->{$param});
                                print $core_name.".queryhandler.".$key.".".$param." ".$value."\n";

                        }
                }
            }
            $queryhandler_el = 0;
        }
        if($core_el == 1){
            print $core_name.".core.deletedDocs ".$f->{'searcher'}->{'stats'}->{'deletedDocs'}."\n";
            print $core_name.".core.numDocs ".$f->{'searcher'}->{'stats'}->{'numDocs'}."\n";
            $core_el = 0;
        }
        if($f eq "CACHE"){
            $cache_el = 1;
        }
        if($f eq "QUERYHANDLER"){
            $queryhandler_el = 1;
        }
        if($f eq "CORE"){
            $core_el = 1;
        }
    }
}
exit 0;

