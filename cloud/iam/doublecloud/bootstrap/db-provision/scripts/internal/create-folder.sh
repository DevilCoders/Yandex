#!/bin/bash
set -e

IAM_TOKEN=${1?}
FOLDER_ID=${2?}
FOLDER_NAME=${3?}
FOLDER_DESCRIPTION=${4?}
CLOUD_ID=${5?}
RMCP_URL=${6?}

grpcurl -insecure -H "Authorization: Bearer ${IAM_TOKEN?}" -d @ ${RMCP_URL?}:4284 yandex.cloud.priv.resourcemanager.v1.FolderService/Create <<REQ
{
  "id": "${FOLDER_ID?}",
  "name": "${FOLDER_NAME?}",
  "description": "${FOLDER_DESCRIPTION?}",
  "cloud_id": "${CLOUD_ID?}"
}
REQ
