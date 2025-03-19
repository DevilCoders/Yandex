#!/bin/bash
set -e

IAM_TOKEN=${1?}
ORG_ID=${2?}
ORG_NAME=${3?}
ORG_DESCRIPTION=${4?}
ORG_URL=${5?}

grpcurl -insecure -H "Authorization: Bearer ${IAM_TOKEN?}" -d @ ${ORG_URL?}:4290 yandex.cloud.priv.organizationmanager.v1.OrganizationService/Create <<REQ
{
  "id": "${ORG_ID?}",
  "name": "${ORG_NAME?}",
  "title": "${ORG_NAME?}",
  "description": "${ORG_DESCRIPTION?}"
}
REQ
