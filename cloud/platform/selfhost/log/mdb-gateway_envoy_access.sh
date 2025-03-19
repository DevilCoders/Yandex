#!/usr/bin/env bash

DAY=$($(dirname "$0")/common/day.sh)
source $(dirname "$0")/common/log.sh
COMMAND=$($(dirname "$0")/common/command.sh $@)

if [[ "$UPSTREAM" == "true" ]]; then
    UPSTREAM_FILTER=""
else
    UPSTREAM_FILTER="grep -v \"\\\"app\\\":\\\"upstream\" | "
fi

$(dirname "$0")/mdb-gateway.sh "cat /var/log/fluent/access_log.api-als.${DAY//-/}.log | ${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}" | eval $(jq_sort_by_timestamp)
