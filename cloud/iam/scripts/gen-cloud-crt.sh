#!/bin/bash
set -e

FOLDER_ID=${1}
SERVICE_NAME=${2}
PROFILE=${3:-default}

if [[ -z $SERVICE_NAME ]]; then
 echo "Usage: $0 <folder-id> <prefix> [profile]"
 echo "Example: $0 yc.iam.access-service-folder as,as-s3,as-monitoring prod"
 exit 0
fi

create_certificate() {
  suffix=$1
  hosts=(${SERVICE_NAME//,/ })
  name=$hosts
  for i in ${!hosts[@]}; do
    hosts[i]="${hosts[i]}".$suffix
  done
  dest_file=$hosts.pem
  hosts=$(tr ' ' ',' <<< ${hosts[@]})

  request="{ \
    folder_id: ${FOLDER_ID}, \
    provider: INTERNAL_CA, \
    name: ${name}, \
    description: \"${SERVICE_NAME} for ${suffix}\", \
    domains: [ ${hosts} ] }"
  response=$(ycp --profile=${PROFILE} --format=json certificatemanager v1 certificate request-new -r - <<< "${request}")
  id=$(jq -rc .id <<< "${response}")
  status=$(jq -rc .status <<< "${response}")
  while [[ "${status}" == "VALIDATING" ]]; do
    # Waiting for a phantom operaton certificatemanager v1 certificate update to complete
    sleep 5
    response=$(ycp --profile=${PROFILE} --format=json certificatemanager v1 certificate get ${id})
    status=$(jq -rc .status <<< "${response}")
  done

  content=$(ycp --profile=${PROFILE} --format=json certificatemanager v1 certificate-content get -r - <<< "certificate_id: ${id}")
  jq -rc '.certificate_chain + [.private_key] | join("")' <<< "${content}" > ${dest_file}
  
  echo "Certificate ${id} saved to ${dest_file}"
}

create_certificate cloud.yandex-team.ru
create_certificate prestable.cloud-internal.yandex.net
create_certificate dev.cloud-internal.yandex.net

create_certificate private-api.cloud-testing.yandex.net
create_certificate private-api.cloud-preprod.yandex.net
create_certificate private-api.cloud.yandex.net
