#!/bin/bash
set -e

IAM_TOKEN=${1?}

SA_ID=${2?}
ROLE_ID=${3?}
RESOURCE_TYPE=${4?}
RESOURCE_ID=${5?}

IAMCP_URL=${6?}

grpcurl -insecure -H "Authorization: Bearer ${IAM_TOKEN?}" -d @ ${IAMCP_URL?}:4283 yandex.cloud.priv.iam.v1.AccessBindingService/UpdateAccessBindings <<REQ
{
  "access_binding_deltas": [
    {
      "action": "ADD",
      "access_binding": {
        "role_id": "${ROLE_ID?}",
        "subject": {
          "id": "${SA_ID?}",
          "type": "serviceAccount"
        }
      }
    }
  ],
  "resource_path": [
    {
      "type": "${RESOURCE_TYPE?}",
      "id": "${RESOURCE_ID?}"
    }
  ],
  "private_call": true
}
REQ
