#!/bin/bash

DOWNSTREAM=`echo "stats proxy" | nc localhost 11211 | grep "pstd_stats:tot_downstream_timeout" | awk '{print $3}' | dos2unix`

if [[ "$DOWNSTREAM" -lt "1" ]]; then
        echo "0;OK";
else
        echo "2;ANSWER:" $DOWNSTREAM;
fi
