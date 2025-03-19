#!/usr/bin/env bash
set -euxo pipefail

SERVICE=${1:?Service is not defined}

curl -v -H "Authorization: OAuth $GORE_OAUTH"  -X PATCH -T "$(dirname "$0")/service-${SERVICE}.json" "https://resps-api.cloud.yandex.net/api/v0/services/${SERVICE}"
curl -s "https://resps-api.cloud.yandex.net/api/v0/services/${SERVICE}" | jq .
