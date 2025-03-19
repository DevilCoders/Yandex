#!/usr/bin/perl

use strict;
use warnings;
use JSON qw( decode_json );
use LWP::Simple;
use Data::Dumper;
use Sys::Hostname;

my $host = hostname;
my $url = "http://localhost:8983/solr/zookeeper?wt=json&detail=true&path=%2Fclusterstate.json";

my $json = get( $url );
unless(defined $json){
    system("/etc/init.d/yandex-solr", "start");
    print "2; Solr is not running!\n";
    exit 0;
}

my $decoded = decode_json($json);
#print Dumper $decoded;

my $data=$decoded->{'znode'}->{'data'};
my $cores = decode_json($data);
my $crit_hosts = "";
my $warn_hosts = "";
my $exit_code = 0;

for my $core (keys(%$cores)) {
    #TODO: iterate over shards
    my $core_data = $cores->{$core}->{'shards'}->{'shard1'}->{'replicas'};
    for my $core_node (keys(%$core_data)){
        my $node_name = (split /:/, $core_data->{$core_node}->{'node_name'})[0];
        my $state = $core_data->{$core_node}->{'state'};
        if ($state ne "active" && $node_name eq $host) {
            $crit_hosts .= " node $node_name status: $state, core name: $core";
            $exit_code = 2;
        }elsif($state ne "active" && $node_name ne $host && $exit_code != 2 ){
            $warn_hosts .= " node $node_name status: $state ";
            $exit_code = 1;
        }
#       print "Core $core state of $node_name is $state\n";
#       print "Local" if $node_name eq $host;
#       print "-----------------\n";
    }
}

$url="http://localhost:8983/solr/zookeeper?wt=json&path=%2Flive_nodes";
$json = get( $url );
unless(defined $json){
    print "2; Solr is not running!\n";
    exit 0;
}

my $exit_code2 = 1;
$decoded = decode_json($json);
my @data=@{$decoded->{'tree'}};
my $childrens=$data[0];

for my $children (@{$childrens->{'children'} }){
    my $child_host = (split /:/, $children->{'data'}->{'title'})[0];
    if ($host eq $child_host){
        $exit_code2=0;
    }
}

if($exit_code2 == 1){
    print "2; Solr is not alive\n";
    exit 0;
}
if($exit_code == 2){
    print "2; ".$crit_hosts."\n";
    exit 0;
}elsif($exit_code == 1){
    print "1; ".$warn_hosts."\n";
    exit 0;
}elsif($exit_code2 == 0){
    print "0; All cores ok \n";
    exit 0;
}


