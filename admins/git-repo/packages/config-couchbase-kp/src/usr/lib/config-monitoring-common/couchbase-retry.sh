#!/bin/bash

RETRY=`echo "stats proxy" | nc localhost 11211 | grep "pstd_stats:tot_retry_vbucket" | awk '{print $3}' | dos2unix`

if [[ "$RETRY" -lt "1" ]]; then
        echo "0;OK";
else
        echo "2;ANSWER:" $RETRY;
fi
