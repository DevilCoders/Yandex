#!/usr/bin/env bash

if [[ "$ENABLE_HEALTH_CHECK" == "true" ]]; then
    HEALTH_CHECK_FILTER=""
else
    HEALTH_CHECK_FILTER="grep -v \"\\\"request_uri\\\":\\\"/grpc.health.v1.Health/Check\\\"\" | "
fi

COMMAND=$(COMMAND_DEFAULT_TAIL_N=10 $(dirname "$0")/common/command.sh $@) 

if [[ "$FULL" == "true" ]]; then
   JQ_MAP="" 
else 
   JQ_MAP="{timestamp: .timestamp, duration: .duration, request_id: .request_id, grpc_service_with_grpc_method: (.grpc_service + \\\"/\\\" + .grpc_method), grpc_status_code: .grpc_status_code, hostname: .hostname}"
fi
JQ_MAP="$JQ_MAP" $(dirname "$0")/api-gateway_envoy_access.sh "grep -i -P \"duration\\\":\\d{5,}\" | ${HEALTH_CHECK_FILTER}${COMMAND}"
