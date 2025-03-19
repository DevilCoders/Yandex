#!/usr/bin/env bash

case "$1" in
"help"|"--help"|"-h"|"")
  echo "Take cert/key from a secret and store them into files."
  echo 
  echo "Usage:"
  echo
  echo "  $0 sec-id name_preifx"
  echo
  echo 'Will create:'
  echo
  echo '  files/name_preifx.pem'
  echo '  files/name_preifx_key.json'
  exit 0
  ;;
esac

if [ -z "$KMS_KEY_ID" ]; then
  KMS_KEY_ID=abjpbk9cdck54cg3er5u
fi  

set -eou pipefail

SECRET_ID="$1"
NAME_PREFIX="$2"

YC_TOKEN=$(yc --endpoint=api.cloud.yandex.net:443 iam create-token)

function kms_encrypt {
  base64 | jq -Rns 'inputs|{plaintext:.}' \
  | curl --fail -Ss -H "Authorization: Bearer $YC_TOKEN" -d @- \
    "https://kms.yandex/kms/v1/keys/${KMS_KEY_ID}:encrypt"
}

KEYS=$(yav get version "${SECRET_ID}" --json | jq -r '.value|keys[]')
CRT_KEY=$(grep -E '.*c.*r.*te?$' <<< "$KEYS")
KEY_KEY=$(grep -E 'key$' <<< "$KEYS")

yav get version "${SECRET_ID}" -o "${CRT_KEY}" > "files/${NAME_PREFIX}.pem"
yav get version "${SECRET_ID}" -o "${KEY_KEY}" | kms_encrypt > "files/${NAME_PREFIX}_key.json"

echo "Updated cert:"
echo
openssl x509 -in "files/${NAME_PREFIX}.pem" -serial -dates -issuer -ext subjectAltName -nocert
