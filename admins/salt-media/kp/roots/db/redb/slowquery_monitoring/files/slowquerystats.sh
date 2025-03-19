#!/bin/bash

# run interval in seconds
#interval=300
slowlog="/var/log/mysql/mysql-slow.log"

#now_ts=$(date +%s)
#let past_ts=$now_ts-$interval
#now=$(date -d@$now_ts +%Y-%m-%d\ %H:%M:%S)
#past=$(date -d@$past_ts +%Y-%m-%d\ %H:%M:%S)

pt-query-digest --order-by Query_time:sum --no-report $slowlog --filter '(print make_checksum($event->{fingerprint}) ." ". $event->{Query_time} . "\n") && 1' | perl -e 'while (<>) { if($_ =~ m/(\w+) (\d+.\d+)/){ $q{$1}+=$2; $qcount{$1}+=1 }  }; foreach(keys %q){ print "slowquerystats.totaltimeperquery.".${_}." ".$q{$_}."\n" }; foreach(keys %qcount){ print "slowquerystats.querycalls.".${_}." ".$qcount{$_}."\n" };' | sort -k 2 -n
#pt-query-digest --order-by Query_time:sum --no-report $slowlog --filter '(print make_checksum($event->{fingerprint}) ." ". $event->{Query_time} . "\n") && 1' | perl -e 'while (<>) { if($_ =~ m/(\w+) (\d+.\d+)/){ $q{$1}+=$2; $qcount{$1}+=1 }  }; foreach(keys %q){ print "slowquerystats.totaltimeperquery.".${_}." ".$q{$_}."\n" }; foreach(keys %qcount){ print "slowquerystats.querycalls.".${_}." ".$qcount{$_}."\n" };' | sort -k 2 -n
