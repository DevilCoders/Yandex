#!/bin/bash
#set -x
set -e

FEDERATION_ID=$1;
PROFILE=${2:-default}
shift

if [[ -z "${FEDERATION_ID}" ]]; then
  echo "Usage: $0 <FEDERATION_ID> [profile]"
  exit
fi

function list-users() {
  federation_id=$1

  ycp --profile=${PROFILE} --format json organization-manager saml federation list-user-accounts ${federation_id} |
    jq -rc '.[] | {name_id: .saml_user_account.name_id, id: .id}' | sort
}

function get-user-bindings() {
  subject_id=$1

  ycp --profile=${PROFILE} --format json \
    iam backoffice access-binding list-by-subject --subject-id ${subject_id} |
    jq -c '[ .[] | select(.role_id | contains("internal") | not)]'
}

USERS=$(list-users ${FEDERATION_ID})

for user in $USERS
do
  id=$(jq -rc .id <<< ${user})
  bindings=$(get-user-bindings ${id})
  echo "{\"user\": ${user}, \"bindings\": ${bindings}}"
done
