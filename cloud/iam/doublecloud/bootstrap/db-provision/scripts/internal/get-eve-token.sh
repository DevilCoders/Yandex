#!/bin/bash
set -e

TS_URL=$1

if [[ -z ${TS_URL} ]]; then
    TS_URL=ts.private-api.eu-central-1.aws.datacloud.net
fi

#JWT=$(jwt-generator -k eve -s eve -p ~/temp/aws-prod/eve.key)
#JWT=$(jwt-generator -k yc.iam.sync -s yc.iam.sync -p ~/temp/aws-prod/yc.iam.sync.key)
JWT=$(jwt-generator -k eve -s eve -p ~/temp/aws-preprod/datacloud-yc/eve.key)
#JWT=$(jwt-generator -k yc.iam.sync -s yc.iam.sync -p ~/temp/aws-preprod/datacloud-yc/yc.iam.sync.key)
#JWT=$(jwt-generator -k yc.iam.sync -s yc.iam.sync -p ~/temp/aws-preprod/yc.iam.sync.key)

grpcurl -insecure -d "{\"jwt\": \"${JWT?}\"}" ${TS_URL?}:4282 yandex.cloud.priv.iam.v1.IamTokenService/Create | jq -r '.iam_token'
