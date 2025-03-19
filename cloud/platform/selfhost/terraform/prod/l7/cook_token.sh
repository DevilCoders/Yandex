#!/usr/bin/env bash

case "$1" in
"help"|"--help"|"-h"|"")
  echo "Take a token from a secret and store them into files."
  echo 
  echo "Usage:"
  echo
  echo "  $0 sec-id name yav-secret-key"
  echo
  echo 'Will create:'
  echo '  files/name.json'
  exit 0
  ;;
esac

set -eou pipefail

SECRET_ID="$1"
NAME="$2"
YAV_KEY="$3"

KMS_KEY_ID=abjpbk9cdck54cg3er5u

YC_TOKEN=$(yc --endpoint=api.cloud.yandex.net:443 iam create-token)

function kms_encrypt {
  base64 | jq -Rns 'inputs|{plaintext:.}' \
  | curl --fail -Ss -H "Authorization: Bearer $YC_TOKEN" -d @- \
    "https://kms.yandex/kms/v1/keys/${KMS_KEY_ID}:encrypt"
}

yav get version "${SECRET_ID}" -o "${YAV_KEY}" | kms_encrypt > "files/${NAME}.json"
