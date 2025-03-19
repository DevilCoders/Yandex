#!/bin/sh

if [ -f /run/clickhouse-server/clickhouse-server.pid ]; then
    CH_PID=`cat /run/clickhouse-server/clickhouse-server.pid`

    kill -TERM $CH_PID

    ch_is_runing() {
        pgrep -s $CH_PID 1> /dev/null 2> /dev/null
    }


    while ch_is_runing; do
        sleep 1
    done
else
    pkill -TERM -f clickhouse-server

    while pgrep -f clickhouse-server 1> /dev/null 2> /dev/null; do
        sleep 1
    done
fi
