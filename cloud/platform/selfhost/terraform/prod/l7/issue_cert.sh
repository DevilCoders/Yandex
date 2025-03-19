#!/usr/bin/env bash
set -eo pipefail

case "$1" in
"help"|"--help"|"-h"|"")
  echo "Issue certificate"
  echo 
  echo "Usage:"
  echo
  echo "  CRT_TOKEN=... # From https://nda.ya.ru/t/G0Xt5TGm3W4VEo "
  echo "  $0 create host-list \$CRT_TOKEN name folder_id"
  echo "  $0 update host-list \$CRT_TOKEN name certificate_id"
  echo
  echo 'Example:'
  echo
  echo "  $0 create 'xds.cloud.yandex.net' \$CRT_TOKEN xds 123fldr"
  echo "  $0 update 'xds.cloud.yandex.net' \$CRT_TOKEN xds 456cert"
  echo
  echo "Also see https://wiki.yandex-team.ru/cloud/infra/crt/create_crt/"
  exit 0
  ;;
esac

set -u
ACTION="$1"
HOSTS="$2"
CRT_TOKEN="$3"
NAME="$4"
FOLDER_ID="$5"
CERT_ID="$5"

CM_REQ=$(jq -n --arg name "$NAME" '{name:$name}')
case "$ACTION" in 
"create")
  CM_REQ=$(jq --arg x "$FOLDER_ID" '.folder_id=$x' <<< "$CM_REQ")
  ;;
"update")
  CM_REQ=$(jq --arg x "$CERT_ID" '.certificate_id=$x' <<< "$CM_REQ")
  CM_REQ=$(jq '.update_mask={paths:["name","chain","private_key"]}' <<< "$CM_REQ")
  ;;
*)
  echo "Invalid action $ACTION"
  echo 
  usage
  exit 1
esac

if [ -z "$CRT_TOKEN" ]; then
  echo "Empty CRT token."
  exit 1
fi

REQ=$(jq -n \
    --arg hosts "$HOSTS" \
    '{
      "type": "ca",
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

CRT=$(echo "$PEM" | sed -E '/BEGIN( RSA)? PRIVATE/,/END( RSA)? PRIVATE/d')
KEY=$(echo "$PEM" | openssl pkey)

CM_REQ=$(jq \
  --arg crt "$CRT" \
  --arg key "$KEY" \
  '.chain=$crt|.private_key=$key' <<< "$REQUEST")

read -p 'Type "yes" to apply: ' -r
if [[ "$REPLY" != "yes" ]]; then
  echo "Aborted."
fi

ycp --profile=prod certificatemanager v1 certificate create -r- <<< "$CM_REQ"
