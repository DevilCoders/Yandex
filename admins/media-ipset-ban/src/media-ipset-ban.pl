#!/usr/bin/perl

use strict;
use warnings;

use v5.10;

use POSIX qw(strftime);
use Regexp::IPv6 qw($IPv6_re);

my $ipset = '/sbin/ipset';
my $iptables = '/sbin/iptables';
my $ip6tables = '/sbin/ip6tables';

my $conf_dir = "/etc/media-ipset-ban.d";
my $log_file = "/var/log/media-ipset-ban.log";

my $log_fh;

sub logger {
    print $log_fh strftime('[%F %T] ', localtime());
    say $log_fh join(' ', @_);
}

sub ipset_check {
    my ($setname, $family) = @_;

    my $type = 'hash:net';

    my $rule = "$ipset -exist create $setname $type family $family timeout 0";
    system("$rule");
    if ($? != 0) {
        logger("can't create ipset: $rule");
        exit 1;
    }
}

sub ipset_add {
    my ($setname, $addr, $timeout, $comment) = @_;

    my $cmd;

    $cmd = "$ipset -exist add $setname $addr timeout $timeout";

    logger(sprintf "( %s ) %s", $comment, $cmd);
    system("$cmd");
    if ($? != 0) {
        logger("can't add $addr to ipset $setname");
        exit 1;
    }
}

sub iptables_create {
    my ($setname, $chain, $table, $dports, $ipt, $target) = @_;

    my $cmd = "$ipt -w -t $table -A $chain -p tcp -m multiport --dports $dports -m set --match-set $setname src -j $target";

    logger "$cmd";
    system("$cmd");
    if ($? != 0) {
        logger("can't create $ipt rule for ipset $setname");
    }
}

sub iptables_check {
    my ($setname, $chain, $table, $dports, $family, $target) = @_;

    my $ipt;
    if ("$family" eq 'inet') {
        $ipt = $iptables;
    } else {
        $ipt = $ip6tables;
    }

    system("$ipt -w -t $table -C $chain -p tcp -m multiport --dports $dports -m set --match-set $setname src -j $target");
    if ($? != 0) {
        iptables_create("$setname", "$chain", "$table", "$dports", "$ipt", "$target")
    }
}

open($log_fh, ">>", "$log_file")
    or die "can't open $log_file $!";

ipset_check("ban4", 'inet');
ipset_check("ban6", 'inet6');

iptables_check("ban4", "INPUT", "filter", "0:65535", 'inet', "DROP");
iptables_check("ban6", "INPUT", "filter", "0:65535", 'inet6', "DROP");

opendir(DIR, $conf_dir) ||
    logger("Can't open dir $conf_dir: $!") &&
    exit -1;

my $file;
my $addr;
my $setname;

for (readdir(DIR)) {
    $file = $_;

    open(FILE, "$conf_dir/$file");

    while (<FILE>) {
        chomp;
        $addr = $_;

        if ($addr =~ /^$IPv6_re(\/\d+)?$/) {
            $setname = "ban6";
        } else {
            $setname = "ban4";
        }

        ipset_add("$setname", "$addr", "120", "$conf_dir/$file");
    }
    close(FILE);
}

close($log_fh);

exit 0;
