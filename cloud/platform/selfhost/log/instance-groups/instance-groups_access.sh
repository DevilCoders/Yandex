#!/usr/bin/env bash

DAY=$($(dirname "$0")/../common/day.sh)
source $(dirname "$0")/../common/log.sh
COMMAND=$($(dirname "$0")/../common/command.sh $@)

if [[ "$UPSTREAM" == "true" ]]; then
    UPSTREAM_FILTER=""
else
    UPSTREAM_FILTER="grep \"\\\"type\\\":\\\"GRPC\" | "
fi

CAT="zcat -f /var/log/instance-group/access-${DAY}*"
if [[ "${DAY}" == "$(TZ=UTC date +"%Y-%m-%d")" ]]; then
    CAT="{ ${CAT} ; cat /var/log/instance-group/access.log ; }"
fi

$(dirname "$0")/instance-groups.sh "${CAT} | ${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}" | eval $(jq_sort_by_timestamp)
