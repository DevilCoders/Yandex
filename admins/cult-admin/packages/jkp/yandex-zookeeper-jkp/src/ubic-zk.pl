#!/usr/bin/perl

package Media::ZooKeeperUbic;

use warnings;
use strict;
use Sys::Hostname;
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
my $user = "zookeeper";
my $serviceName = "zookeeper-jkp";
my $classpath = "/etc/yandex/$serviceName:/usr/lib/yandex/$serviceName/*:/usr/lib/yandex/$serviceName/lib/*";
my $javaNetworkOption="-Djava.net.preferIPv6Addresses=true";
my $javaArgs = "$javaNetworkOption -Dzookeeper.log.dir=/var/log/yandex/$serviceName/ -Dzookeeper.root.logger=INFO,CONSOLE -Dlog4j.configuration=file:/etc/yandex/$serviceName/log4j-server.properties -Dcom.sun.management.jmxremote -Dcom.sun.management.jmxremote.local.only=false";
my $configFile="/etc/yandex/$serviceName/zoo.cfg";
my $clientPort=2181;

sub status_impl {
    my $self = shift;
    my $result = $self->SUPER::status_impl();
    return $result if $result->status ne "running";
    if (`echo ruok | netcat -w 5 localhost $clientPort 2>&1` ne 'imok') {
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
