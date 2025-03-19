#!/usr/bin/env bash
SERVICE=${1:?Service is not defined}
set -euo pipefail

# add field '{"service": "your_service_name"}' to provided json. this need on create stage.
JSON2POST=$(jq '. + {"service": env.SERVICE}' "$(dirname "$0")/service-${SERVICE}.json")

echo "${JSON2POST}" | curl -v -H "Authorization: OAuth $GORE_OAUTH"  -X POST -T . "https://resps-api.cloud.yandex.net/api/v0/services"

curl -s "https://resps-api.cloud.yandex.net/api/v0/services/${SERVICE}" | jq .
