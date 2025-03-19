#!/bin/bash
set -e

ENV_ID=$1
PATH_TO_DELETE=$2

BASE_DIR="$(dirname $(dirname $0))"
CONFIG_FILE="${BASE_DIR}/config.json"
USAGE="usage: IAM_TOKEN=... $0 <ENVIRONMENT> <PATH_TO_DELETE>"

if [[ -z "${ENV_ID}" ]]; then
    echo "Environment required!"
    echo ${USAGE}
    exit 1
fi

JSON_ENV_ID=$(jq ".environments[] | select(.id == \"${ENV_ID}\")" ${CONFIG_FILE})

if [[ -z ${JSON_ENV_ID} ]]; then
    echo "Environment must be one of: [ $(jq -r '[.environments[].id] | join(" | ")' ${CONFIG_FILE}) ]"
    echo ${USAGE}
    exit 1
fi

if [[ -z ${IAM_TOKEN} ]]; then
    IAM_TOKEN=$(yc --profile=${ENV_ID} iam create-token)
fi

PROJECT_ID=$(echo ${JSON_ENV_ID} | jq -r ".projectId")
SOLOMON_ENDPOINT=$(echo ${JSON_ENV_ID} | jq -r ".endpoint")

curl --silent -X DELETE \
    -H "Authorization: Bearer ${IAM_TOKEN}" \
    https://${SOLOMON_ENDPOINT}/api/v2/projects/${PROJECT_ID}/${PATH_TO_DELETE}

echo
