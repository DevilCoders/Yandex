#!/bin/sh -e

/usr/lib/yandex/@dir@/bin/java-with-classpath.sh \
    org.apache.zookeeper.server.PurgeTxnLog \
    /var/lib/yandex/@dir@ -n 5 \
    "$@"

# vim: set ts=4 sw=4 et:
