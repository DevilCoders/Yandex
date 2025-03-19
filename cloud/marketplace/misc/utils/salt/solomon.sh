#!/bin/bash
source ./pillar/env.sh

cli_params="--api-url ${YC_API_URL} --cloud ${YC_CLOUD} --oauth-token ${OAUTH_TOKEN} --folder ${YC_FOLDER} --identity-public-api-url $YC_IAM_URL --identity-private-api-url $YC_IAM_PRIV_URL"
valid_names=`(for i in ${instances[@]}; do echo $i | awk '{print $1}'; done) | xargs echo | sed 's/ /|/g'`

list=`yc-cli instance list -f value -c name $cli_params | grep -P "^(${valid_names})\$"`

for node in $list
do
# `grep 2a` is to filter ipv6 on df
    node_info=`yc-cli instance show -f json $node $cli_params`
    ip=`echo ${node_info} | jq ".network_interfaces[0].primaryV6Address.address" | sed 's/"//g'`


     hosts+=",{
          \"urlPattern\": \"\\[$ip\\]\",
          \"ranges\": \"\",
          \"dc\": \"\",
          \"labels\": [
            \"env=test\"
          ]
        }"
done

dt=$(date -u +"%Y-%m-%dT%H:%M:%S.000Z")
curl -X GET \
      H 'Content-Type: application/json' \
      -H 'Accept: application/json' \
      -H "Authorization: OAuth ${SOLOMON_TOKEN}" \
      -o /tmp/solomon.json \
      http://solomon.yandex.net/api/v2/projects/yc-marketplace/clusters/yc-marketplace_${ENV}

cat > /tmp/solomon.json << EOF
{
  "id": "yc-marketplace_${ENV}",
  "name": "${ENV}",
  "projectId": "yc-marketplace",
  "hosts": [
    ${hosts:1}
  ],
  "version": 0,
  "createdAt": "$dt",
  "updatedAt": "$dt",
  "createdBy": "nikthespirit",
  "updatedBy": "nikthespirit"
}
EOF


curl -X PUT \
      -H 'Content-Type: application/json' \
      -H 'Accept: application/json' \
      -H "Authorization: OAuth ${SOLOMON_TOKEN}" \
      -d @/tmp/solomon.json \
      http://solomon.yandex.net/api/v2/projects/yc-marketplace/clusters/yc-marketplace_${ENV}

rm /tmp/solomon.json
