#!/usr/bin/env bash

DAY=$($(dirname "$0")/common/day.sh)
source $(dirname "$0")/common/log.sh
COMMAND=$($(dirname "$0")/common/command.sh $@)

$(dirname "$0")/api-gateway.sh "cat /var/log/fluent/error_log.api-als.${DAY//-/}.log | ${COMMAND} | ${HOSTNAME_TO_LOG}" | eval $(jq_sort_by_timestamp)
