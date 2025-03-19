#!/usr/bin/env bash

JQ_TSV=".request_uri,.grpc_status_code" $(dirname "$0")/api-gateway_access_error.sh cat | sort | uniq -c | sort -nr
