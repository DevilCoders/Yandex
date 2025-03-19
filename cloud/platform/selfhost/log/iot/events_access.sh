#!/usr/bin/env bash

DAY=$($(dirname "$0")/../common/day.sh)
source $(dirname "$0")/../common/log.sh
source $(dirname "$0")/common_log.sh
COMMAND=$($(dirname "$0")/../common/command.sh $@)

CAT="zcat -f /var/log/yc-iot/events/access-${DAY}T*.log.gz"
if [[ "$SILENT" == "true" ]]; then
    CAT="${CAT} 2>/dev/null"
fi
if [[ "${DAY}" == "$(TZ=UTC date +"%Y-%m-%d")" ]]; then
    CAT="{ ${CAT} 2>/dev/null ; cat /var/log/yc-iot/events/access.log ; }"
fi

$(dirname "$0")/events.sh "${CAT} | ${NAME_FILTER_CMD} ${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}" | eval $(iot_jq_sort_by_timestamp)
