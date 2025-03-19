#!/bin/bash

META_FILE="/etc/yandex-dataproc"

STATUS_NOT_RUNNING=3

DAEMON="${1:-hadoop-hdfs-secondarynamenode}"

if [ ! -f "$META_FILE" ] ; then
    exit 0
fi

# Check status of daemon
"/etc/init.d/$DAEMON" status
RETVAL=$?

# Start daemon only if it not started
if [[ "$RETVAL" == "$STATUS_NOT_RUNNING" ]] ;then
    "/etc/init.d/$DAEMON" start 2>&1 ; logger -p daemon.err "$DAEMON started"
fi
