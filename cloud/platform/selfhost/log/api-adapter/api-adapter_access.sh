#!/usr/bin/env bash

DAY=$($(dirname "$0")/../common/day.sh)
source $(dirname "$0")/../common/log.sh
COMMAND=$($(dirname "$0")/../common/command.sh $@)

if [[ "$UPSTREAM" == "true" ]]; then
    UPSTREAM_FILTER=""
else
    UPSTREAM_FILTER="grep \"\\\"authority\\\":\\\"api-adapter.private-api.cloud\" | "
fi

CAT="zcat -f /var/log/yc-api-adapter/access-${DAY}*"
if [[ "${DAY}" == "$(TZ=UTC date +"%Y-%m-%d")" ]]; then
    CAT="{ ${CAT} ; cat /var/log/yc-api-adapter/access.log ; }"
fi


$(dirname "$0")/api-adapter.sh "${CAT} | ${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}" | eval $(jq_sort_by_timestamp)
