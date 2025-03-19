#!/bin/bash
set -e
ORG_ID=${1}
PROFILE=${2:default}

CLOUDS=$(ycp --profile=${PROFILE} --format=json \
  resource-manager cloud list --organization-id ${ORG_ID?})

if [[ "[]" != $CLOUDS ]]; then
  echo "Organization $ORG_ID has clouds: $CLOUDS"
  exit 1
fi

FEDERATIONS=$(ycp --profile=${PROFILE} --format=json \
  organization-manager saml federation list --organization-id ${ORG_ID?})

if [[ "[]" != $FEDERATIONS ]]; then
  echo "Organization $ORG_ID has federations: $FEDERATIONS"
  exit 1
fi

export SA_IAM_TOKEN=$(ycp --profile=${PROFILE} --format=json \
  iam iam-token create-for-service-account -r - \
  <<< "service_account_id: yc.iam.service-account" | jq -rc .iam_token)

YC_IAM_TOKEN=$SA_IAM_TOKEN ycp iam --profile prod access-binding set-access-bindings -r - <<REQ
private_call: true
resource_path:
  - type: organization-manager.organization
    id: $ORG_ID
access_bindings:
  - role_id: organization-manager.organizations.owner
    subject:
      type: serviceAccount
      id: yc.iam.sync
REQ

YC_IAM_TOKEN=$SA_IAM_TOKEN ycp iam --profile prod access-binding list-access-bindings -r - <<REQ
private_call: true
resource_type: organization-manager.organization
resource_id: $ORG_ID
REQ

echo "WARN!!! Don't forget to fix the quota usage: step 4 in https://wiki.yandex-team.ru/cloud/iamrm/duty/faq/#udalitorganizaciju"
echo "(or simply automate the step too ;))"
