#!/usr/bin/env bash
set -eo pipefail

case "$1" in
"help"|"--help"|"-h"|"")
  echo "Issue certificate"
  echo 
  echo "Usage:"
  echo
  echo "  CRT_TOKEN=... # From https://nda.ya.ru/t/G0Xt5TGm3W4VEo "
  echo "  $0 CA host-list \$CRT_TOKEN name-prefix"
  echo
  echo 'Example:'
  echo
  echo "  $0 gpn_ca 'xds.gpn.yandexcloud.net' \$CRT_TOKEN common/client"
  echo
  echo "Also see https://wiki.yandex-team.ru/cloud/infra/crt/create_crt/"
  exit 0
  ;;
esac

set -u
CA=$1
HOSTS=$2
CRT_TOKEN=$3
NAME_PREFIX=$4

if [ -z "$CRT_TOKEN" ]; then
  echo "Empty CRT token."
  exit 1
fi

REQ=$(jq -n \
    --arg ca "$CA" \
    --arg hosts "$HOSTS" \
    '{
      "type": $ca,
      "desired_ttl_days": 365,
      "abc_service": 5793,
      "hosts": $hosts
    }')
echo
echo "Sending request"
echo
jq . <<< "$REQ"
echo

RESP=$(echo "$REQ" | curl -sS -d@- \
    -H "Authorization: OAuth $CRT_TOKEN" \
    -H "Content-Type: application/json" \
    https://crt.cloud.yandex.net/api/certificate)

echo "Got reply:"
echo
jq . <<< "$RESP" || echo "$RESP"
echo

PEM_URL=$(jq --exit-status -r .download2 <<< "$RESP")
PEM=$(curl --fail -sS -H "Authorization: OAuth $CRT_TOKEN" "${PEM_URL}")

# Strips private key.
echo "$PEM" | sed -E '/BEGIN( RSA)? PRIVATE/,/END( RSA)? PRIVATE/d' > "${NAME_PREFIX}.pem"

echo "$PEM" | openssl pkey | $(dirname "$0")/kms_encrypt.sh > "${NAME_PREFIX}_key.json"
