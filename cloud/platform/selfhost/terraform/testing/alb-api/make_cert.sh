#!/usr/bin/env bash

case "$1" in
"help"|"--help"|"-h"|"")
  echo "Take cert/key from a secret and store them into files."
  echo 
  echo "Usage:"
  echo
  echo "  $0 sec-id name_prefix"
  echo
  echo 'Will create:'
  echo
  echo '  files/secrets/name_prefix.pem'
  echo '  files/secrets/name_prefix_key.json'
  exit 0
  ;;
esac

set -eou pipefail

SECRET_ID="$1"
NAME_PREFIX="$2"

KMS_KEY_ID=dq8elo74a2agikavfbng

YC_TOKEN=$(ycp iam iam-token create -r=<(echo "{\"yandex_passport_oauth_token\": \"$(yc config get token)\"}") --profile testing | yq '.iam_token' -r)

function kms_encrypt {
  base64 | jq -Rns 'inputs|{plaintext:.}' \
  | curl --fail -Ss -H "Authorization: Bearer $YC_TOKEN" -d @- \
    "https://kms.cloud-testing.yandex.net/kms/v1/keys/${KMS_KEY_ID}:encrypt"
}

KEYS=$(ya vault get version "${SECRET_ID}" --json | jq -r '.value|keys[]')
CRT_KEY=$(grep -E '.*c.*r.*te?$' <<< "$KEYS")
KEY_KEY=$(grep -E 'key$' <<< "$KEYS")

ya vault get version "${SECRET_ID}" -o "${CRT_KEY}" > "files/secrets/${NAME_PREFIX}.pem"
ya vault get version "${SECRET_ID}" -o "${KEY_KEY}" | kms_encrypt > "files/secrets/${NAME_PREFIX}_key.json"

echo "Updated cert:"
echo
openssl x509 -in "files/${NAME_PREFIX}.pem" -serial -dates -issuer -ext subjectAltName -nocert
