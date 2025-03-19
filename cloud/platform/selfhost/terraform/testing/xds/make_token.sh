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
  echo '  files/secrets/name.json'
  exit 0
  ;;
esac

set -eou pipefail

SECRET_ID="$1"
NAME="$2"
YAV_KEY="$3"

KMS_KEY_ID=dq8giijioj54087lhjq6

YC_TOKEN=$(ycp iam iam-token create -r=<(echo "{\"yandex_passport_oauth_token\": \"$(yc config get token)\"}") --profile testing | yq '.iam_token' -r)

function kms_encrypt {
  base64 | jq -Rns 'inputs|{plaintext:.}' \
  | curl --fail -Ss -H "Authorization: Bearer $YC_TOKEN" -d @- \
    "https://kms.cloud-testing.yandex.net/kms/v1/keys/${KMS_KEY_ID}:encrypt"
}

ya vault get version "${SECRET_ID}" -o "${YAV_KEY}" | kms_encrypt > "files/secrets/${NAME}.json"
