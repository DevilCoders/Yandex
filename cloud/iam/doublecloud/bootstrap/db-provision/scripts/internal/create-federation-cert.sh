#!/bin/bash
set -e

IAM_TOKEN=${1?}
FEDERATION_ID=${2?}
CERTIFICATE_NAME=${3?}
CERTIFICATE_DATA=${4?}
ORG_URL=${5?}

grpcurl -insecure -H "Authorization: Bearer ${IAM_TOKEN?}" -d @ ${ORG_URL?}:4290 yandex.cloud.priv.organizationmanager.v1.saml.CertificateService/Create <<REQ
{
  "federation_id": "${FEDERATION_ID?}",
  "name": "${CERTIFICATE_NAME?}",
  "data": "${CERTIFICATE_DATA?}"
}
REQ
