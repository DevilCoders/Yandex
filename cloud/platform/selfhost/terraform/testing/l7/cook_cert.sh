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
  echo '  name_preifx.pem'
  echo '  name_preifx_key.json'
  exit 0
  ;;
esac

set -eou pipefail

SECRET_ID="$1"
NAME_PREFIX="$2"

KMS_KEY_ID=dq89nnq4tmpubm559l8d

function kms_encrypt {
  $(dirname $0)/kms_encrypt.sh $KMS_KEY_ID --profile=testing
}

KEYS=$(yav get version "${SECRET_ID}" --json | jq -r '.value|keys[]')
CRT_KEY=$(grep -E '.*c.*r.*te?$' <<< "$KEYS")
KEY_KEY=$(grep -E 'key$' <<< "$KEYS")

yav get version "${SECRET_ID}" -o "${CRT_KEY}" > "${NAME_PREFIX}.pem"
yav get version "${SECRET_ID}" -o "${KEY_KEY}" | kms_encrypt > "${NAME_PREFIX}_key.json"

echo "Updated cert:"
echo
openssl x509 -in "${NAME_PREFIX}.pem" -serial -dates -issuer -ext subjectAltName -nocert
