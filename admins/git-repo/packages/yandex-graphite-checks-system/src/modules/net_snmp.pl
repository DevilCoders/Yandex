#!/usr/bin/perl

use strict;
use warnings;

use v5.10;

my $snmp = '/proc/net/snmp';

open(FD, $snmp) || die "Can't open '$snmp': $!\n";

my @F;

while (<FD>) {
    if (/^Tcp:\s*\d+/) {
        @F = split /\s+/;

        say "tcp.ActiveOpens $F[5]";
        say "tcp.PassiveOpens $F[6]";
        say "tcp.AttemptFails $F[7]";
        say "tcp.EstabResets $F[8]";
        say "tcp.CurrEstab $F[9]";
        say "tcp.InSegs $F[10]";
        say "tcp.OutSegs $F[11]";
        say "tcp.RetransSegs $F[12]";
        say "tcp.InErrs $F[13]";
        say "tcp.OutRsts $F[14]";
        say "tcp.InCsumErrors $F[15]";
    }
}

close(FD);
