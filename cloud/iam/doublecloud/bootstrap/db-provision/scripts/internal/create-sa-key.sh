#!/bin/bash
set -e

IAM_TOKEN=${1?}
SA_ID=${2?}
KEY_DESCRIPTION=${3?}
IAMCP_URL=${4?}

# YAV_SECRET_ID=sec-01fgv4qyqec73vqcd2adp5636s # datacloud-aws-prod-sa-keys
# YAV_SECRET_ID=sec-01f4hrse1cnjzkr85v3tns9vqg # datacloud-aws-preprod-sa-keys
YAV_SECRET_ID=sec-01f60a2f98w9725kz4darw2shk # datacloud-yc-sa-keys
SA_KEY_PUB=$(internal/get-public-key.sh ${SA_ID?} ${YAV_SECRET_ID?})

grpcurl -insecure -H "Authorization: Bearer ${IAM_TOKEN?}" -d @ ${IAMCP_URL?}:4283 yandex.cloud.priv.iam.v1.KeyService/Create <<REQ
{
  "key_id": "${SA_ID?}",
  "service_account_id": "${SA_ID?}",
  "key_algorithm": "RSA_4096",
  "public_key": "${SA_KEY_PUB?}"
}
REQ

#   "description": "${KEY_DESCRIPTION?}, https://yav.yandex-team.ru/secret/${YAV_SECRET_ID?}"
