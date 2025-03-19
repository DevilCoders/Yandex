#!/bin/sh -e

/usr/lib/yandex/zookeeper-jkp/bin/java-with-classpath.sh \
    org.apache.zookeeper.server.PurgeTxnLog \
    /var/lib/yandex/zookeeper-jkp -n 5 \
    "$@"

# vim: set ts=4 sw=4 et:
