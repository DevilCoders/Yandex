#!/bin/bash
set -e

IAM_TOKEN=${1?}
FEDERATION_ID=${2?}
ORG_ID=${3?}
FEDERATION_NAME=${4?}
FEDERATION_DESCRIPTION=${5?}
FEDERATION_ISSUER=${6?}
FEDERATION_SSO_URL=${7?}
ORG_URL=${8?}

grpcurl -insecure -H "Authorization: Bearer ${IAM_TOKEN?}" -d @ ${ORG_URL?}:4290 yandex.cloud.priv.organizationmanager.v1.saml.FederationService/Create <<REQ
{
  "id": "${FEDERATION_ID?}",
  "organization_id": "${ORG_ID}",
  "name": "${FEDERATION_NAME?}",
  "description": "${FEDERATION_DESCRIPTION?}",
  "cookie_max_age": "12h",
  "auto_create_account_on_login": false,
  "issuer": "${FEDERATION_ISSUER?}",
  "sso_binding": "POST",
  "sso_url": "${FEDERATION_SSO_URL?}",
  "case_insensitive_name_ids": true
}
REQ
