#!/bin/sh -e

. /usr/lib/yandex/@dir@/bin/zkEnv.sh

RESTMAIN=org.apache.zookeeper.server.jersey.RestMain

exec /usr/lib/yandex/@dir@/bin/java-with-classpath.sh \
    -Dlog4j.configuration=file:/etc/yandex/@dir@/log4j-rest.properties \
    -Dlog4j.formatMsgNoLookups=true  \
    -Dlog4j2.formatMsgNoLookups=true \
    $JVMFLAGS $RESTMAIN 
    
# vim: set ts=4 sw=4 et:
