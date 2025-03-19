#!/usr/bin/env bash

set -eo pipefail

if [ -z "$KMS_KEY_ID" ]; then
  KMS_KEY_ID=e5m9omljprgsqd0jj5se
fi  

{
  echo plaintext: $(base64 --wrap=0)
  echo key_id: $KMS_KEY_ID
} | ycp kms symmetric-crypto encrypt -r- --format=json --profile=gpn
