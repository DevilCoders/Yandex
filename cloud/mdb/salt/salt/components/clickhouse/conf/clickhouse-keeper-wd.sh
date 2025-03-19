#!/bin/sh

[ -f /run/clickhouse-keeper/wd.stop ] && exit 0
if ! /usr/sbin/service clickhouse-keeper status; then
    rm /run/clickhouse-keeper/clickhouse-keeper.pid || true
    /usr/sbin/service clickhouse-keeper start
fi
