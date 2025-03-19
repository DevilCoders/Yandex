#!/bin/bash
set -e

CLOUD_FOLDER_ID="yc.iam.service"

RMCP_URL=$1

if [[ -z ${RMCP_URL} ]]; then
    echo "usage: $0 [RMCP_URL]"
    RMCP_URL=rm.private-api.eu-central-1.aws.datacloud.net
    TS_URL=ts.private-api.eu-central-1.aws.datacloud.net
else
    TS_URL=${RMCP_URL?}
fi

IAM_TOKEN=$(internal/get-eve-token.sh ${TS_URL})

echo Create cloud
internal/create-cloud.sh "${IAM_TOKEN?}" "${CLOUD_FOLDER_ID?}" "yc.organization-manager.yandex" "iam-service" "" "${RMCP_URL?}" | jq

echo sleep 10
sleep 10

echo Create folder
internal/create-folder.sh "${IAM_TOKEN?}" "${CLOUD_FOLDER_ID?}" "iam-service" "" "${CLOUD_FOLDER_ID?}" "${RMCP_URL?}" | jq
