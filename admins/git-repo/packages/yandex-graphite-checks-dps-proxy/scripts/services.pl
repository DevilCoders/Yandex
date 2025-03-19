#! /usr/bin/env perl

use strict;
use warnings;
use Storable;

my $tmp_file = "/tmp/services.tmp";
my %old_serv = eval { %{retrieve($tmp_file)} };
my %serv;
my @logs = qx|timetail -n60 /var/log/nginx/access.log|;

foreach my $log (@logs) {
        $log =~ /.*db\/([\w\d]+)\/(.*)/;
        my $service = $1;
        $serv{$1}++ if $service;
}
while (my ($key, $value) = (each %serv)) {
	if ($old_serv{$key}) {
		$value -= $old_serv{$key};
	}
}

store \%serv, $tmp_file;

while (my ($key, $value) = (each %old_serv)) {
        print("$key $value\n");
}
