#!/bin/bash

if [ ! -d /opt/replicator ]; then
    echo "0;Replicator not set up"
    exit
fi

/opt/replicator/tungsten/tungsten-replicator/bin/trepctl status >/tmp/trepctl.status

STATUS=`grep state /tmp/trepctl.status | cut -f2 -d:`
if [ "x$STATUS" != "x ONLINE" ]; then
    echo "2;The status is $STATUS"
    exit
fi

LATENCY=`grep appliedLatency /tmp/trepctl.status | cut -f2 -d: | cut -f1 -d.`
if [ "$LATENCY" -gt 480 ]; then
    echo "2;The applied latency is $LATENCY"
    exit
fi

echo "0;ok"
