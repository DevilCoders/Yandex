#!/bin/bash
set -e

CLOUD_ID=$1;
ROLE=${2:-admin}
PROFILE=${3:-default}
shift

if [[ -z "${CLOUD_ID}" ]]; then
  echo "Usage: $0 <CLOUD_ID> [role=admin] [profile=default] uids.txt"
  exit
fi

function grant_role() {
resource_type=$1
resource_id=$2
role=$3
subject_id=$4
ycp --profile=${PROFILE} --format=json resource-manager ${resource_type} update-access-bindings --async -r - <<REQ
resource_id: ${resource_id}
private_call: true
access_binding_deltas:
  - action: ADD
    access_binding:
      role_id: ${role}
      subject:
        id: ${subject_id}
        type: userAccount
REQ
}

function create_folder() {
cloud_id=$1
folder_name=$2
existing_folder=$(
  echo "{filter: \"name='${folder_name}'\", cloud_id: \"${cloud_id}\"}" |
  ycp --profile=${PROFILE} --format=json resource-manager folder list -r - |
  jq -rc .[])
if [[ -z $existing_folder ]]; then
  echo "{cloud_id: ${cloud_id}, description: \"Personal folder for {$folder_name}\", name: \"${folder_name}\"}" |
  ycp --profile=${PROFILE} --format=json resource-manager folder create -r -
else
  echo $existing_folder
fi
}

function get_or_create_subject_id() {
  login=$1
  subject_id=$(echo "passport_users: [login: \"${login}\"]" |
    ycp iam --profile=${PROFILE} --format=json yandex-passport-user-account add-user-accounts -r - |
    jq -rc .valid_users[].id)
  echo ${subject_id}
}

function invite_passport_user() {
login=$1
ycp --profile=${PROFILE} --format=json organization-manager invite create -r - <<REQ
organization_id: ${ORG_ID}
invites:
  - yandex_passport_user_account:
      login: ${login}
REQ
}

function add_user() {
  # USER
  login=$1
  subject_id=$(get_or_create_subject_id ${login})
  _=$(invite_passport_user ${login})
  _=$(grant_role "cloud" ${CLOUD_ID} "resource-manager.clouds.member" ${subject_id})

  #FOLDER
  folder_name=$(sed -e 's/[^a-z0-9-]/-/g' <<< ${login})
  folder_id=$(create_folder ${CLOUD_ID} ${folder_name} |
    jq -rc .id)
  _=$(grant_role "folder" ${folder_id} "${ROLE}" ${subject_id})

  echo "UID: $1 FOLDER: ${folder_id} USER ${subject_id}"
}

ORG_ID=$(echo "cloud_id: \"${CLOUD_ID}\"" | ycp --profile=${PROFILE} --format=json resource-manager cloud get -r - | jq -rc .organization_id)
while IFS=" " read -r -a line; do
  login=${line[1]}
  add_user $login
done
