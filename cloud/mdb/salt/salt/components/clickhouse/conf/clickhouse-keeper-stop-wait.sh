#!/bin/sh

if [ -f /run/clickhouse-keeper/clickhouse-keeper.pid ]; then
    CH_PID=`cat /run/clickhouse-keeper/clickhouse-keeper.pid`

    kill -TERM $CH_PID

    ch_is_runing() {
        pgrep -s $CH_PID 1> /dev/null 2> /dev/null
    }


    while ch_is_runing; do
        sleep 1
    done
else
    pkill -TERM -f clickhouse-keeper

    while pgrep -f clickhouse-keeper 1> /dev/null 2> /dev/null; do
        sleep 1
    done
fi
