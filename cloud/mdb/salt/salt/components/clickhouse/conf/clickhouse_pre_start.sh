#!/bin/sh

NEED_RESETUP=$(sudo salt-call mdb_clickhouse.resetup_required --out newline_values_only)

if [ "${NEED_RESETUP}" = "False" ] || { [ "${NEED_RESETUP}" = "True" ] && [ -f "/etc/clickhouse-server/config.d/resetup_config.xml" ]; }; then
    pkill -9 -f clickhouse-odbc-bridge
    rm -f /run/clickhouse-server/wd.stop
    exit 0
fi

exit 1
