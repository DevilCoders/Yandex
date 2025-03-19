#!/bin/bash

WAIT=`echo "stats proxy" | nc localhost 11211 | grep "pstd_stats:tot_wait_queue_timeout" | awk '{print $3}' | dos2unix`

if [[ "$WAIT" -lt "1" ]]; then
        echo "0;OK";
else
        echo "2;ANSWER:" $WAIT;
fi
