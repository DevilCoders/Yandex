#!/usr/bin/env bash
set -euxo pipefail

SERVICE=${1:?Service is not defined}

curl -s "https://resps-api.cloud.yandex.net/api/v0/services/${SERVICE}" | jq . --indent 4
