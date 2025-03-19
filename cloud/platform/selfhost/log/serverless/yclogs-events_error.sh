#!/usr/bin/env bash

LOG_DIR="/var/log/yc-log-events/"

DAY=$($(dirname "$0")/../common/day.sh)
source $(dirname "$0")/../common/log.sh
COMMAND=$($(dirname "$0")/../common/command.sh $@)


CAT="zcat -f ${LOG_DIR}/error-${DAY}*.log.gz"
if [[ "$SILENT" == "true" ]]; then
    CAT="${CAT} 2>/dev/null"
fi
if [[ "${DAY}" == "$(TZ=UTC date +"%Y-%m-%d")" ]]; then
    CAT="{ ${CAT} 2>/dev/null ; cat ${LOG_DIR}/error.log ; }"
fi

$(dirname "$0")/yclogs-events.sh "${CAT} | ${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}"
