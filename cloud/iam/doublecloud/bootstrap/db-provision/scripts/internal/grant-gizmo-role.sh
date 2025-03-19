#!/bin/bash
set -e

IAM_TOKEN=${1?}

SA_ID=${2?}
ROLE_ID=${3?}

IAMCP_URL=${4?}

grpcurl -insecure -H "Authorization: Bearer ${IAM_TOKEN?}" -d @ ${IAMCP_URL?}:4283 yandex.cloud.priv.iam.v1.GizmoService/UpdateAccessBindings <<REQ
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
  ]
}
REQ