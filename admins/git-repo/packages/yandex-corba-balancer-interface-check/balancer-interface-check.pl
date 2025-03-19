#! /usr/bin/perl

use strict;
use warnings;
use Getopt::Long;
use Sys::Hostname;
use LWP::Simple;
use Socket qw( SOCK_STREAM SOCK_RAW NI_NUMERICHOST );
use Socket::GetAddrInfo qw( getaddrinfo getnameinfo );

my $DEBUG = 0;

my ($balancer, $port);
my (@ip, @err_ip);

GetOptions (
        'balancer=s' => \$balancer,
        'b=s'        => \$balancer,
        'port=i'     => \$port,
        'p=i'     => \$port,
        ) or die $!;

unless ($port and $balancer) {
        print ("Usage: $0 -b balancer_name -p port\n");
        exit;
}

my $my_hostname = hostname();
print ("my_hostname = $my_hostname\n") if $DEBUG;

sub get_balancer_ip($) {
        my $balancer = shift;
        my %hints = ( socktype => SOCK_RAW );
        my ( $err, @res ) = getaddrinfo( "$balancer", "", \%hints );
        die "Cannot resolve name - $err" if $err;

        foreach my $ai (@res) {
                my ($err, $ipaddr) = getnameinfo($ai->{addr}, NI_NUMERICHOST);
                die "Cannot getnameinfo - $err" if $err;

                push (@ip, $ipaddr);
        }

        return @ip;
}

sub get_my_vip($) {
        my $balancer = shift;
        my $vip_on_balancers = get "https://racktables.yandex.net/export/vip_on_balancers.php?vip=$balancer&vport=80";
        my @vip_on_balancers = split(/\s/, $vip_on_balancers);

        my @temp = grep (/$my_hostname/, @vip_on_balancers);
        my $my_vip;
        foreach my $str (@temp) {
                if ($str =~ m|.*\((.*)/.*\).*|g ) {
                        $my_vip = $1;
                }
        }

        return $my_vip;
}

sub check_interface($) {
        my $vip = shift;
        my $check = system ("ip a sh dev eth0 | grep $vip 2>&1 >/dev/null");

        return $check;
}

@ip = get_balancer_ip($balancer);

if ($DEBUG) {
        print ("ip: $_\n") foreach (@ip);
}
foreach my $balancer_ip (@ip) {
        my $my_vip = get_my_vip($balancer_ip);
        if (check_interface($my_vip)) {
                push (@err_ip, $my_vip);
        }
}

unless (@err_ip) {
        print ("0; Balancer ip's UP\n");
} else {
        print ("2; Balancer ip @err_ip not configured\n");
}
