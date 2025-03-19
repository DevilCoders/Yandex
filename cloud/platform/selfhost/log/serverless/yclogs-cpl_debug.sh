#!/usr/bin/env bash

LOG_DIR="/var/log/yc-log-groups/"

DAY=$($(dirname "$0")/../common/day.sh)
source $(dirname "$0")/../common/log.sh
COMMAND=$($(dirname "$0")/../common/command.sh $@)


CAT="zcat -f ${LOG_DIR}/debug-${DAY}*.log.gz"
if [[ "$SILENT" == "true" ]]; then
    CAT="${CAT} 2>/dev/null"
fi
if [[ "${DAY}" == "$(TZ=UTC date +"%Y-%m-%d")" ]]; then
    CAT="{ ${CAT} 2>/dev/null ; cat ${LOG_DIR}/debug.log ; }"
fi

$(dirname "$0")/yclogs-cpl.sh "${CAT} | ${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}"
