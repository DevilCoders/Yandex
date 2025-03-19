#!/bin/bash
recover_count=4
max_stale=5
recover_dc_logs=`find /srv/ -name 'dnet_recovery.log'  -mtime +$max_stale`
for log in $recover_dc_logs
do
    if [ -e $log.$recover_count.zst ]
    then
        rm $log.$recover_count.zst
    fi
    for c in $(seq $((recover_count-1)) -1 1)
    do
        if [ -e $log.$c.zst ]
        then
            mv $log.$c.zst $log.$((c+1)).zst
        fi
    done

    if [ -e $log ]
    then
        mv $log $log.1
        zstd $log.1
    fi
done
