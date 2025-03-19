#!/usr/bin/perl

package Media::ZooKeeperUbic;

use warnings;
use strict;
use Sys::Hostname;
use Ubic::Daemon qw(check_daemon);
use Ubic::Logger;
use parent qw(Ubic::Service::SimpleDaemon);

my $javaPath;

if (-e "/usr/local/java8/bin/java") {
    $javaPath = "/usr/local/java8/bin/java";
} elsif (-e "/usr/local/java7/bin/java") {
    $javaPath = "/usr/local/java7/bin/java";
} else {
    die "Java (7 or 8) not found";
}

my $ulimit = 65000;
my $user = "@user@";
my $serviceName = "@dir@";
my $classpath = "/etc/yandex/$serviceName:/usr/lib/yandex/$serviceName/*:/usr/lib/yandex/$serviceName/lib/*";
my $javaNetworkOption="@javaNetworkOption@";
my $javaSecurityOption="@javaSecurityOption@";
my $javaMemOption="@javaMemOption@";
my $javaZookeeperOpts="@javaZookeeperOpts@";
my $javaArgs = "$javaNetworkOption $javaSecurityOption $javaMemOption $javaZookeeperOpts -Dsun.net.inetaddr.ttl=60 -Dzookeeper.log.dir=/var/log/yandex/$serviceName/ -Dzookeeper.root.logger=INFO,CONSOLE -Dlog4j.configuration=file:/etc/yandex/$serviceName/log4j-server.properties -Dlog4j.formatMsgNoLookups=true -Dlog4j2.formatMsgNoLookups=true";
my $configFile="/etc/yandex/$serviceName/zoo.cfg";
my $clientPort=@port@;

sub status_impl {
    my $self = shift;
    my $result = $self->SUPER::status_impl();
    return $result if $result->status ne "running";

    my $daemon = check_daemon($self->pidfile);
    my $start = (stat "/proc/".$daemon->pid."/stat")[10];
    my $zk_uptime = time - $start;
    if ($zk_uptime < 180) {
        INFO("Zk uptime too small ".$zk_uptime." sec, skip healthcheck");
        return $result;
    }
    my $node='/healthcheck-'.`hostname -f`;
    chomp($node);
    my $timestamp = time;
    my $retry = 1;
    my $last_ts = 0;
    while ($retry < 25) {  # retry 50 seconds sleep 2 seconds
        $last_ts=`/bin/echo '
            create $node "Last synced timestamp: $timestamp"
            set $node "Last synced timestamp: $timestamp"
            get $node
        '|/usr/lib/yandex/$serviceName/bin/zkCli.sh 2>/dev/null`;
        $last_ts =~ m/^Last synced timestamp: (\d+)/m;
        $last_ts = $1;
        if (defined $last_ts) {
            last;
        }
        $last_ts = 0;
        INFO("Cannot get healthcheck timestamp, attempt ". $retry);
        $retry++;
        sleep 2;
    }
    if ($last_ts == 0) {
        INFO("Failed to get healthcheck timestamp");
        return 'broken';
    }
    INFO("healthcheck ts: ".$last_ts.", current ts: ".$timestamp);
    my $healthcheck_age = $timestamp - $last_ts;
    if ( $healthcheck_age > 300 ) {
        # zk can't sync timestamp last 5 min, consider it broken
        INFO("healthcheck timestamp too old (".$healthcheck_age." sec), consider zk as broken!");
        return 'broken';
    }
    return $result;
}

sub start_impl {
    my $self = shift;
    my $host = hostname;

    `grep $host $configFile | grep '^server\\.' | sed 's,=.*,, ; s,.*\\.,, ' > /var/lib/yandex/$serviceName/myid`;

    return $self->SUPER::start_impl();
}

sub timeout_options {
    my $self = shift;
    return { start => { step => 1, trials => 3 } } ;
}


return Media::ZooKeeperUbic->new({
    bin => ["sh", "-c", "ulimit -n $ulimit; exec /usr/bin/chuid-run $user $javaPath -cp $classpath $javaArgs org.apache.zookeeper.server.quorum.QuorumPeerMain $configFile"]
});
