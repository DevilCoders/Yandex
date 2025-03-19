#!/bin/bash

if [ -n "$1" ]
then
cid=$1
token=`yc iam create-token`
fid=`yc resource-manager folder list --cloud-id=$cid --format=json | jq -r '.[].id'`
for id in $fid; do
        echo -e "increasing quota for \e[1;32m$id\e[0m folder:"
        curl -X PATCH -H "X-YaCloud-SubjectToken: $token" -H "content-type: application/json" -H "content-type: application/json" -d '{"metric":{"name":"active-operation-count","limit":"'"$2"'"}}' "https://iaas.private-api.cloud.yandex.net/compute/external/v1/folderQuota/$id/metric" ; done
else
        echo 'Enter cloud-id and count'
fi
