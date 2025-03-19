#!/bin/bash
#set -e
MAX_TASKS=${1:-600}
DELAY=${2:-30}
OVERLOAD_DELAY=${3:-60}
SA_ID="yc.resource-manager.clouds-cleaner"
KEY_ID="yc.resource-manager.clouds-cleaner"
PRIVATE_KEY_PATH="/usr/share/yc-secrets/64453231-8667-11eb-83f1-4bb33d4cf1a2/clouds-cleaner.key/1/clouds-cleaner.key"
DATABASE=/pre-prod_global/iam
ENDPOINT_SUFFIX=cloud-preprod.yandex.net

YDB_ENDPOINT="grpcs://ydb-iam.${ENDPOINT_SUFFIX}:2136"
TOKEN_SERVICE_ENDPOINT="ts.private-api.${ENDPOINT_SUFFIX}:4282"
RMCP_ENDPOINT="rm.private-api.${ENDPOINT_SUFFIX}:4284"
TASK_TABLE=hardware/default/resource_manager/task_processor/tasks

function delete_cloud() {
  DELETE_AFTER=$(date -d "+3 days" -u --iso-8601=seconds)

  JWT=$(/usr/local/bin/jwt-generator --key-id ${KEY_ID?} --service-account-id ${SA_ID} --private-key-file ${PRIVATE_KEY_PATH?})
  CLEANER_IAM_TOKEN=$(/usr/local/bin/grpcurl -d "{\"jwt\":\"${JWT?}\"}" ${TOKEN_SERVICE_ENDPOINT?} yandex.cloud.priv.iam.v1.IamTokenService.Create | jq -r .iam_token)

  /usr/local/bin/grpcurl \
        -H "X-Request-Id: $(uuidgen -t)" \
        -H "Authorization: Bearer ${CLEANER_IAM_TOKEN?}" \
        -d "{\"cloud_id\": \"$1\",\"delete_after\": \"${DELETE_AFTER?}\"}" \
        ${RMCP_ENDPOINT?} yandex.cloud.priv.resourcemanager.v1.CloudService.Delete
  echo "$1"
}

function active_tasks() {
  AWAIT_TS=$(date -d "$date +15minutes" +%s000)
  echo $(ydb -e ${YDB_ENDPOINT} -d ${DATABASE} --token-file /var/lib/autodelete-token scripting yql --script \
    "SELECT count(*) AS cnt FROM [${TASK_TABLE}]
     WHERE id BETWEEN ('projection-action/' || pool || '/0')
     AND ('projection-action/' || pool || '/' || DateTime::ToString(DateTime2::FromMilliseconds(Cast(${AWAIT_TS} as Uint64))))" \
    | grep -o '[0-9]*')
}

while IFS= read -r line; do
  while [[ $(active_tasks) -gt ${MAX_TASKS} ]]; do
    echo waiting...
    sleep ${OVERLOAD_DELAY}
  done
  delete_cloud "$line"
  sleep ${DELAY}
done

