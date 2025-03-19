#!/usr/bin/env bash

source $(dirname "$0")/common/log.sh
source $(dirname "$0")/common/profile.sh

if [[ -z "$@" ]]; then
    LAST_REQUEST_ID=$(PROFILE=${PROFILE} JQ_TSV=.request_id $(dirname "$0")/api-gateway_access.sh | tail -n 1)
    COMMAND="grep ${LAST_REQUEST_ID}"
else
    COMMAND="$@"
fi


{
    PROFILE=${PROFILE} JQ_DISABLE=true UPSTREAM=true $(dirname "$0")/api-router_access.sh ${COMMAND} ;
    PROFILE=${PROFILE} JQ_DISABLE=true UPSTREAM=true $(dirname "$0")/api-gateway_envoy_access.sh ${COMMAND} ;
    PROFILE=${PROFILE} JQ_DISABLE=true UPSTREAM=true $(dirname "$0")/api-gateway_access.sh ${COMMAND} ;
} | eval $(jq_sort_by_timestamp)
