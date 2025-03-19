#!/bin/sh

[ -f /run/clickhouse-server/wd.stop ] && exit 0
if ! /usr/sbin/service clickhouse-server status; then
    rm /run/clickhouse-server/clickhouse-server.pid || true
    /usr/sbin/service clickhouse-server start
fi
