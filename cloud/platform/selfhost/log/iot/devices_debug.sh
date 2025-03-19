#!/usr/bin/env bash

DAY=$($(dirname "$0")/../common/day.sh)
source $(dirname "$0")/../common/log.sh
COMMAND=$($(dirname "$0")/../common/command.sh $@)


CAT="zcat -f /var/log/yc-iot/devices/debug-${DAY}*.log.gz"
if [[ "$SILENT" == "true" ]]; then
    CAT="${CAT} 2>/dev/null"
fi
if [[ "${DAY}" == "$(TZ=UTC date +"%Y-%m-%d")" ]]; then
    CAT="{ ${CAT} 2>/dev/null ; cat /var/log/yc-iot/devices/debug.log ; }"
fi

$(dirname "$0")/devices.sh "${CAT} | ${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}"
