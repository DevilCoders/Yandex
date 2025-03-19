#!/usr/bin/env bash

LOG_DIR="/var/log/fluent/"
DAY=$($(dirname "$0")/../common/day.sh)
source $(dirname "$0")/../common/log.sh
COMMAND=$($(dirname "$0")/../common/command.sh $@)

CAT="zcat -f ${LOG_DIR}/access_log.serverless-packer.log*"
if [[ "$SILENT" == "true" ]]; then
    CAT="${CAT} 2>/dev/null"
fi

$(dirname "$0")/ycf-packer.sh "${CAT} | egrep "^$DAY" | ${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}"
