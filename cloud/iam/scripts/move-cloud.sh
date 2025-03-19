#!/bin/bash
set -e
CLOUD_ID=$1
ORG_ID=$2
PROFILE=${3:-default}

if [[ -z ${ORG_ID} ]]; then
  echo "Usage: $0 <cloud_id> <org_id> [profile=default]"
  exit 0
fi


SA_IAM_TOKEN=$(ycp --profile=${PROFILE} --format=json iam iam-token create-for-service-account -r - \
  <<< "service_account_id: yc.iam.service-account" | jq -rc .iam_token)

CLOUD_OWNERS=$(YC_IAM_TOKEN=${SA_IAM_TOKEN} ycp --profile=${PROFILE} --format json \
  resource-manager cloud list-access-bindings -r - <<< "resource_id: ${CLOUD_ID}" |
  jq -rc '.[] | select(.role_id == "resource-manager.clouds.owner") | .subject.id')
echo "cloud owners: [${CLOUD_OWNERS}]"


ORG_OWNERS=$(YC_IAM_TOKEN=${SA_IAM_TOKEN} ycp --profile=${PROFILE} --format json \
  organization-manager organization list-access-bindings -r - <<< "resource_id: ${ORG_ID}" |
  jq -rc '.[] | select(.role_id == "organization-manager.organizations.owner") | .subject.id')
echo "org owners: [${ORG_OWNERS}]"

if [[ "${CLOUD_OWNERS}" = "${ORG_OWNERS}" ]]; then
  CONFIRM=y
else
  read -p "Continue (y/n)?" CONFIRM
fi

if [[ "$CONFIRM" = "y" ]]; then
  echo Облако перемещено:
  YC_IAM_TOKEN=${SA_IAM_TOKEN} ycp --profile=${PROFILE} \
    resource-manager cloud move -r - <<< "{cloud_id: ${CLOUD_ID}, organization_id: ${ORG_ID}}"
fi
