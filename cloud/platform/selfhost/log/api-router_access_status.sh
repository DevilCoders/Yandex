#!/usr/bin/env bash

COMMAND=$(COMMAND_DEFAULT="cat" $(dirname "$0")/common/command.sh $@)

JQ_TSV=".grpc_status_code" $(dirname "$0")/api-router_access.sh "${COMMAND}" | sort | uniq -c | sort -nr
