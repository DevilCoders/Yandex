#!/usr/bin/env bash

[[ -n "$DEBUG" ]] && [[ "$DEBUG" != "0" ]] && set -x

if [[ -n "$ECHO" ]] && [[ "$ECHO" != "0" ]]; then
    PSQL_OPTS+=" -e"
fi

metadb_config=$(dbaas config get | jq -reM .metadb)
dsn=$(echo $metadb_config | jq -reM .dsn)
hosts=$(echo $metadb_config | jq -reM .hosts | sed -E "s/ +/,/g")

if [ -t 0 ]; then
    psql -h "$hosts" "connect_timeout=1 target_session_attrs=read-write $dsn" $PSQL_OPTS
else
    if [[ "$PSQL_MODE" = "command" ]]; then
        psql -h "$hosts" "connect_timeout=1 target_session_attrs=read-write $dsn" $PSQL_OPTS -c "$(cat)"
    else
        psql -h "$hosts" "connect_timeout=1 target_session_attrs=read-write $dsn" $PSQL_OPTS -f - <&0
    fi
fi
