#!/bin/bash
set -e
test -z $OAUTH_TOKEN && echo "\$OAUTH_TOKEN not set https://oauth.yandex-team.ru/authorize?response_type=token&client_id=577125be67bb4415ab1d781d19434545" && exit 1
test -z $SERVICE_NAME && echo "\$SERVICE_NAME dns prefix is not set, should be specified in short form e.g. as|ts|oauth " && exit 1

# https://abc.yandex-team.ru/services/yc_iam_dev/
ABC_ID=2090


create_certificate() {
  suffix=$1
  type=$2
  ttl_days=${3:-365}
  src_crt_path="./src_cert.pem"

  hosts=(${SERVICE_NAME//,/ })
  for i in ${!hosts[@]}; do
    hosts[i]="${hosts[i]}".$suffix
  done
  dest_file=$hosts.pem
  hosts=$(tr ' ' ',' <<< ${hosts[@]})

  response=$(curl -s \
    -H "Authorization: OAuth $OAUTH_TOKEN" \
    -H "Content-Type: application/json" \
    -X POST -d "{\"type\": \"$type\", \"hosts\": \"${hosts}\", \"desired_ttl_days\": ${ttl_days}, \"abc_service\": $ABC_ID}"\
    "https://crt.cloud.yandex.net/api/certificate") 
  download_url=$(jq -rc '.download2' <<< "${response}")
  if [[ ${download_url} == "null" ]]; then
    echo ${response}
    exit
  fi
  curl -s -H "Authorization: OAuth $OAUTH_TOKEN" $download_url > $src_crt_path
   
  cat $src_crt_path | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' > $dest_file
  openssl pkcs8 -topk8 -in $src_crt_path -inform pem -outform pem -nocrypt  >> $dest_file
  rm $src_crt_path
  echo "server pem-certificate is generated at path $dest_file"
}

create_certificate cloud.yandex-team.ru ca
create_certificate prestable.cloud-internal.yandex.net ca
create_certificate dev.cloud-internal.yandex.net ca

create_certificate private-api.cloud-testing.yandex.net ca
create_certificate private-api.cloud-preprod.yandex.net ca
create_certificate private-api.cloud.yandex.net ca 380

create_certificate private-api.private-testing.yandexcloud.net ca
create_certificate private-api.gpn.yandexcloud.net gpn_ca 380

