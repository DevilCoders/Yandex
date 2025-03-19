#!/usr/bin/env bash

LOG_DIR="/var/log/yc-log-groups/"

DAY=$($(dirname "$0")/../common/day.sh)
source $(dirname "$0")/../common/log.sh
COMMAND=$($(dirname "$0")/../common/command.sh $@)

if [[ "$UPSTREAM" == "true" ]]; then
    UPSTREAM_FILTER=""
else
    UPSTREAM_FILTER="grep \"\\\"authority\\\":\\\"log-groups.private-api.ycp.\" | "
fi

CAT="zcat -f ${LOG_DIR}/access-${DAY}*.log.gz"
if [[ "$SILENT" == "true" ]]; then
    CAT="${CAT} 2>/dev/null"
fi
if [[ "${DAY}" == "$(TZ=UTC date +"%Y-%m-%d")" ]]; then
    CAT="{ ${CAT} 2>/dev/null ; cat ${LOG_DIR}/access.log ; }"
fi

$(dirname "$0")/yclogs-cpl.sh "${CAT} | ${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}" | eval $(jq_sort_by_timestamp)
