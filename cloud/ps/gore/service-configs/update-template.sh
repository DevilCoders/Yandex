#!/usr/bin/env bash
set -euxo pipefail

SERVICE=${1:?Service is not defined}

curl -s --insecure -H "Authorization: OAuth $GORE_OAUTH"  -X PATCH -T "$(dirname "$0")/st-template-$SERVICE.json" "https://resps-api.cloud.yandex.net/api/v0/templates/$SERVICE"
curl -s --insecure "https://resps-api.cloud.yandex.net/api/v0/templates/$SERVICE" | jq .
