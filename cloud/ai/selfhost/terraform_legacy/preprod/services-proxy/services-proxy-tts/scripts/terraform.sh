#!/bin/sh

YAV_TOKEN=`cat ~/yav_token`
if [ -z "$YC_SERVICE_ACCOUNT_KEY_FILE" ]
then
    export YC_SERVICE_ACCOUNT_KEY_FILE=".yc_sa_key"
    ya vault get version sec-01djmfwpjn2a5hwzn7gcm2mmcz -o service-key-services-proxy-deployer > $YC_SERVICE_ACCOUNT_KEY_FILE
fi

exec terraform $* --var yandex_token=${YAV_TOKEN}
