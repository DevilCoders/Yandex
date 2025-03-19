#!/bin/bash
set -e

SA_ID="yc.iam.sync"
SA_NAME="yc-iam-sync"
SA_DESCRIPTION="IAM sync-account Used to sync access bindings"
KEY_DESCRIPTION="IAM ${SA_ID?} service account key"
FOLDER_ID="yc.iam.service"

IAMCP_URL=$1

if [[ -z ${IAMCP_URL} ]]; then
    IAMCP_URL=iam.private-api.eu-central-1.aws.datacloud.net
    TS_URL=ts.private-api.eu-central-1.aws.datacloud.net
else
    TS_URL=${IAMCP_URL?}
fi

IAM_TOKEN=$(internal/get-eve-token.sh ${TS_URL?})

echo Create SA
internal/create-sa.sh "${IAM_TOKEN?}" "${SA_ID?}" "${SA_NAME?}" "${SA_DESCRIPTION?}" "${FOLDER_ID?}" "${IAMCP_URL?}" | jq

echo Create SA key
internal/create-sa-key.sh "${IAM_TOKEN?}" "${SA_ID?}" "${KEY_DESCRIPTION?}" "${IAMCP_URL?}" | jq
