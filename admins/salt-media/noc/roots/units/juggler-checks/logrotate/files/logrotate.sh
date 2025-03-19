#!/bin/sh
FIRST_ERROR=$(logrotate -d /etc/logrotate.conf 2>&1| grep ^error:)
if [ -n "$FIRST_ERROR" ]; then # no flaps in this day, retry it!
    logger -p daemon.err "logrotate.sh error: $FIRST_ERROR"
    sleep 15
    SECOND_ERROR=$(logrotate -d /etc/logrotate.conf 2>&1| grep ^error:)
    if [ -n "$SECOND_ERROR" ]; then
        logger -p daemon.err "logrotate.sh error: $SECOND_ERROR"
        echo "PASSIVE-CHECK:logrotate;2;Logrotate broken: $SECOND_ERROR"
    else
	    echo "PASSIVE-CHECK:logrotate;1;Logrotate broken one times: $FIRST_ERROR"
    fi
else
    echo "PASSIVE-CHECK:logrotate;0;OK"
fi
