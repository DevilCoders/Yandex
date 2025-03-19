#!/usr/bin/perl

#tcpdump -n port 80 -w file
#tcpdump -ttnr file > tcpdump.file

#Получаем время от первого syn до первого ack.

use v5.10;
use warnings;
use strict;
use Data::Dumper;

open (TCPDUMP,"tcpdump.file");

my %session;
my %session_in;
my %session_out;

while (<TCPDUMP>) {
#1381839428.189827 IP 93.158.136.226.2561 > 93.158.135.183.80: Flags [S], seq 2825780416, win 17820, options [mss 8910,sackOK,TS val 1082529238 ecr 0,nop,wscale 4], length 0

#1381839435.969899 IP 93.158.136.226.2974 > 93.158.135.183.80: Flags [.], seq 1:1399, ack 1, win 1114, options [nop,nop,TS val 1082531183 ecr 363293366], length 1398


	if ( $_ =~ m/(^[0-9\.]+)\ IP (93\.158\.136\.226\.[0-9]+) > 93\.158\.135\.183\.80: Flags \[S\].*/g ) {
	$session_in{$2} = $1;
	}

	if ( $_ =~ m/(^[0-9\.]+)\ IP 93\.158\.135\.183\.80 > (93\.158\.136\.226\.[0-9]+): Flags \[S\.\],.*/g ) {
	$session_out{$2} = $1;
	}
}

foreach my $sess ( keys %session_out ) {

	$session{$sess} = $session_out{$sess} - $session_in{$sess};
	$session{$sess} = sprintf("%.8f",$session{$sess});
}

foreach my $sess ( keys %session ) {
#	say $session{$sess};
	if ( $session{$sess} > 0.00001 ) {
		say $sess;
	 }
}
