#!/usr/bin/env bash

DAY=$($(dirname "$0")/common/day.sh)
source $(dirname "$0")/common/log.sh
COMMAND=$($(dirname "$0")/common/command.sh $@)

if [[ "$ENABLE_HEALTH_CHECK" == "true" ]]; then
    HEALTH_CHECK_FILTER=""
else
    HEALTH_CHECK_FILTER="grep -v \"\\\"request_uri\\\":\\\"/grpc.health.v1.Health/Check\\\"\" | "
fi

if [[ "$DISABLE_UNAVAILABLE" == "true" ]]; then
    UNAVAILABLE_FILTER="grep -v \"\\\"grpc_status_code\\\":14\" | "
else
    UNAVAILABLE_FILTER=""
fi

if [[ "$UPSTREAM" == "true" ]]; then
    UPSTREAM_FILTER=""
else
    UPSTREAM_FILTER="grep -v \"\\\"app\\\":\\\"upstream\" | "
fi

$(dirname "$0")/api-gateway.sh "cat /var/log/fluent/access_log.api-gateway.${DAY//-/}.log | grep \"^{\" | ${HEALTH_CHECK_FILTER}${UNAVAILABLE_FILTER}${UPSTREAM_FILTER}${COMMAND} | ${HOSTNAME_TO_LOG}" | eval $(jq_sort_by_timestamp)
