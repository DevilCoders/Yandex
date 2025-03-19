#! /usr/bin/perl -w
use strict;

my $memory = `ps aux | grep "/usr/sbin/fastcgi-daemon2 --config=/etc/fastcgi2/available/dps-proxy.conf" | grep -v grep | awk '{ print \$6 }'`;
my $mb_memory = int($memory / 1024);
print("Usage_memory $mb_memory\n");
