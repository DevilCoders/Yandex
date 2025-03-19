#!/bin/bash
set -e

SERVICE_NAME_FOR_ID=$1
SERVICE_NAME_FOR_NAME=$2
SERVICE_NAME_FOR_DESCRIPTION=$3

USAGE="usage: $0 <SERVICE_NAME_FOR_ID> <SERVICE_NAME_FOR_NAME> <SERVICE_NAME_FOR_DESCRIPTION> [RMCP_URL] [IAMCP_URL]"

if [[ -z ${SERVICE_NAME_FOR_ID} ]] || [[ -z ${SERVICE_NAME_FOR_NAME} ]] || [[ -z ${SERVICE_NAME_FOR_DESCRIPTION} ]]; then
    echo ${USAGE}
    exit 1
fi

CLOUD_FOLDER_ID="yc.iam.service"

# XXX Folder must be same as cloud. Because of it must represents the Project at dataclouds
#FOLDER_ID="yc.iam.${SERVICE_NAME_FOR_ID?}"
#FOLDER_NAME="${SERVICE_NAME_FOR_NAME?}"
#FOLDER_DESCRIPTION="IAM ${SERVICE_NAME_FOR_DESCRIPTION?} folder"

SA_ID="yc.iam.${SERVICE_NAME_FOR_ID?}"
SA_NAME="yc-iam-${SERVICE_NAME_FOR_NAME?}"
SA_DESCRIPTION="IAM ${SERVICE_NAME_FOR_DESCRIPTION?} service account"
KEY_DESCRIPTION="IAM ${SERVICE_NAME_FOR_DESCRIPTION?} service account key"

RMCP_URL=$4
IAMCP_URL=$5

if [[ -z ${RMCP_URL} ]]; then
    echo ${USAGE}
    RMCP_URL=rm.private-api.eu-central-1.aws.datacloud.net
    IAMCP_URL=iam.private-api.eu-central-1.aws.datacloud.net
    TS_URL=ts.private-api.eu-central-1.aws.datacloud.net
else
    TS_URL=${RMCP_URL?}
fi

IAM_TOKEN=$(internal/get-eve-token.sh ${TS_URL?})

# XXX At dataclouds service SA need to place into common folder together. Because of there is only one folder at cloud.
#echo Create folder
#internal/create-folder.sh "${IAM_TOKEN?}" "${FOLDER_ID?}" "${FOLDER_NAME?}" "${FOLDER_DESCRIPTION?}" "yc.iam.serviceCloud" "${RMCP_URL?}" | jq

echo Create SA
internal/create-sa.sh "${IAM_TOKEN?}" "${SA_ID?}" "${SA_NAME?}" "${SA_DESCRIPTION?}" "${CLOUD_FOLDER_ID?}" "${IAMCP_URL?}" | jq

echo Create SA key
internal/create-sa-key.sh "${IAM_TOKEN?}" "${SA_ID?}" "${KEY_DESCRIPTION?}" "${IAMCP_URL?}" | jq
