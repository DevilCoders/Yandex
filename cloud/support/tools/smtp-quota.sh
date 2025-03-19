#!/bin/bash

if [ -n "$1" ]
then
cid=$1
token=`yc iam create-token`
echo -e "increasing quota for \e[1;32m$cid\e[0m cloud:"
curl -X PATCH -H "X-YaCloud-SubjectToken: $token" -H "content-type: application/json" -H "content-type: application/json" -d '{"metric":{"name":"external-smtp-direct-address-count","limit":"'"$2"'"}}' "https://iaas.private-api.cloud.yandex.net/compute/external/v1/quota/$cid/metric" 
else
        echo 'Enter cloud-id and count'
fi
