#!/usr/bin/env bash

source $(dirname "$0")/common/log.sh
COMMAND=$(COMMAND_DEFAULT_TAIL_N=10 $(dirname "$0")/common/command.sh $@)

if [[ "$FULL" == "true" ]]; then
   JQ_MAP="" 
else 
   JQ_MAP="{timestamp: .timestamp, duration: .duration, request_id: .request_id, request_uri: .request_uri, grpc_status_code: .grpc_status_code, grpc_status_message: .grpc_status_message, hostname: .hostname}"
fi
JQ_MAP="$JQ_MAP" $(dirname "$0")/api-gateway_access.sh "${GRPC_ERROR} | ${COMMAND}"
