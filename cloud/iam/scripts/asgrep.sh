#!/bin/bash
# https://docs.yandex-team.ru/iam-cookbook/5.internals/logging_clickhouse

REQUEST_ID=$1
FORMAT=$2

if [[ -z ${REQUEST_ID} ]]; then
  echo usage "$0 <request-id> [format=Vertical]"
  exit 0
fi

clickhouse-client --format ${FORMAT:-Vertical} \
  --query "SELECT * FROM logs.access_log WHERE request_id = '${REQUEST_ID}'"
