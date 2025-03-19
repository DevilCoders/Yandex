#!/usr/bin/env bash

COMMAND=$(COMMAND_DEFAULT="cat" $(dirname "$0")/common/command.sh $@)

JQ_TSV=".request_uri,.grpc_status_code" $(dirname "$0")/cpl-router_access_error.sh "${COMMAND}" | sort | uniq -c | sort -nr
