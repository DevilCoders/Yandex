#!/bin/bash
set -e

IAM_TOKEN=${1?}
CLOUD_ID=${2?}
ORGANIZATION_ID=${3?}
CLOUD_NAME=${4?}
CLOUD_DESCRIPTION=${5?}
RMCP_URL=${6?}

grpcurl -insecure -H "Authorization: Bearer ${IAM_TOKEN?}" -d @ ${RMCP_URL?}:4284 yandex.cloud.priv.resourcemanager.v1.CloudService/Create <<REQ
{
  "id": "${CLOUD_ID?}",
  "organization_id": "${ORGANIZATION_ID}",
  "name": "${CLOUD_NAME?}",
  "description": "${CLOUD_DESCRIPTION?}"
}
REQ
