#!/bin/sh -e

. /usr/lib/yandex/zookeeper-jkp/bin/zkEnv.sh

RESTMAIN=org.apache.zookeeper.server.jersey.RestMain

exec /usr/lib/yandex/zookeeper-jkp/bin/java-with-classpath.sh \
    -Dlog4j.configuration=file:/etc/yandex/zookeeper-jkp/log4j-rest.properties \
    $JVMFLAGS $RESTMAIN 
    
# vim: set ts=4 sw=4 et:
