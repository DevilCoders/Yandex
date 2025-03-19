#!/usr/bin/env bash

DAY=$($(dirname "$0")/common/day.sh)
source $(dirname "$0")/common/log.sh
COMMAND=$($(dirname "$0")/common/command.sh $@)

if [[ "$UPSTREAM" == "true" ]]; then
    UPSTREAM_FILTER=""
else
    UPSTREAM_FILTER="grep -v \"\\\"request\\\":\\\"upstream_cluster\" | "
fi

CAT="zcat -f /var/log/fluent/access_log.api-als.${DAY//-/}.log*.gz"

if [[ "$SILENT" == "true" ]]; then
    CAT="${CAT} 2>/dev/null"
fi
if [[ "${DAY}" == "$(TZ=UTC date +"%Y-%m-%d")" ]]; then
    CAT="{ ${CAT} 2>/dev/null ; cat /var/log/fluent/access_log.api-als.${DAY//-/}.log ; }"
fi

$(dirname "$0")/cpl-router.sh "$CAT | ${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}" | eval $(jq_sort_by_timestamp)
