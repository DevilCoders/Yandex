#!/bin/bash
set -e

IAM_TOKEN=${1?}
SA_ID=${2?}
SA_NAME=${3?}
SA_DESCRIPTION=${4?}
FOLDER_ID=${5?}
IAMCP_URL=${6?}

grpcurl -insecure -H "Authorization: Bearer ${IAM_TOKEN?}" -d @ ${IAMCP_URL?}:4283 yandex.cloud.priv.iam.v1.ServiceAccountService/Create <<REQ
{
  "id": "${SA_ID?}",
  "name": "${SA_NAME?}",
  "description": "${SA_DESCRIPTION?}",
  "folder_id": "${FOLDER_ID?}"
}
REQ
