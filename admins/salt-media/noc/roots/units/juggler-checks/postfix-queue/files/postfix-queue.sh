#!/bin/bash

me=${0##*/}	# strip path
me=${me%.*}	# strip extension
CONF=/etc/monitoring/${me}.conf

if [ -r $CONF ]; then
    CRIT=$(awk -F= '/critical/ {print $2}' $CONF | tr -d ' ')
    WARN=$(awk -F= '/warning/ {print $2}' $CONF | tr -d ' ')
fi
queue_size=$(/usr/bin/mailq 2>&1 | awk '/Kbytes in [0-9]+ Requests/ {print $(NF-1)}')

if [ -z $queue_size ]; then
    echo "PASSIVE-CHECK:postfix-queue;0;OK; mail queue is empty"
    exit 0
fi

if [ ! -z $CRIT ] ; then 
    if [ $queue_size -ge $CRIT ] ; then
        echo "PASSIVE-CHECK:postfix-queue;2; Failed; mail queue is $queue_size"
        exit 0
    fi
fi

if [ ! -z $WARN ] ; then
    if [ $queue_size -ge $WARN ] ; then
        echo "PASSIVE-CHECK:postfix-queue;1; Warning; mail queue is $queue_size"
        exit 0
    fi
fi

echo "PASSIVE-CHECK:postfix-queue;0; OK; mail queue is $queue_size"
exit 0
