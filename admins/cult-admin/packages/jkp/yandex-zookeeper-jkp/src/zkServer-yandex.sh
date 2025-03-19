#!/bin/sh -e


# See the following page for extensive details on setting
# up the JVM to accept JMX remote management:
# http://java.sun.com/javase/6/docs/technotes/guides/management/agent.html
# by default we allow local JMX connections
JMXLOCALONLY=false

if [ "x$JMXDISABLE" = "x" ]
then
    #echo "JMX enabled by default"
    # for some reason these two options are necessary on jdk6 on Ubuntu
    #   accord to the docs they are not necessary, but otw jconsole cannot
    #   do a local attach
    ZOOMAIN="-Dcom.sun.management.jmxremote -Dcom.sun.management.jmxremote.local.only=$JMXLOCALONLY org.apache.zookeeper.server.quorum.QuorumPeerMain"
else
    echo "JMX disabled by user request"
    ZOOMAIN="org.apache.zookeeper.server.quorum.QuorumPeerMain"
fi

. /usr/lib/yandex/zookeeper-jkp/bin/zkEnv.sh

ZOOCFG="/etc/yandex/zookeeper-jkp/zoo.cfg"

KILL=kill

# this is needed for RHEL only
if [ "$1" = "--write-pid" ]; then
    echo "Writing PID=$$ to $2"
    echo $$ >$2
    shift 2
fi

exec /usr/lib/yandex/zookeeper-jkp/bin/java-with-classpath.sh \
    "-Dzookeeper.log.dir=${ZOO_LOG_DIR}" \
    "-Dzookeeper.root.logger=${ZOO_LOG4J_PROP}" \
    -Dlog4j.configuration=file:/etc/yandex/zookeeper-jkp/log4j-server.properties \
    $JVMFLAGS $ZOOMAIN "$ZOOCFG"
    
# vim: set ts=4 sw=4 et:
