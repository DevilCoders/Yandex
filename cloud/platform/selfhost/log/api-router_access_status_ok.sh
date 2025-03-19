#!/usr/bin/env bash

source $(dirname "$0")/common/log.sh
COMMAND=$(COMMAND_DEFAULT="cat" $(dirname "$0")/common/command.sh $@)

JQ_TSV=".request_uri,.grpc_status_code" $(dirname "$0")/api-router_access.sh "${GRPC_OK} | ${COMMAND}" | sort | uniq -c | sort -nr
