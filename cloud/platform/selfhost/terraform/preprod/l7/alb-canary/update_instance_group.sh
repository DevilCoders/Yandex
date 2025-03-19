#!/usr/bin/env bash
set -eou pipefail

INSTANCE_GROUP_ID=amcqjf3sp50sv09s54jv
FOLDER_ID=aoe4lof1sp0df92r6l8j

yc compute instance-group update \
  --id="${INSTANCE_GROUP_ID}" \
  --endpoint=api.cloud-preprod.yandex.net:443 \
  --folder-id="${FOLDER_ID}" \
  --file=<(yq . ig-spec.tpl.yaml | \
    jq --arg x "$(cat ./files/xds-server.pem)" '.instance_template.metadata.xds_server_cert_crt=$x' | \
    jq --arg x "$(cat ./files/xds-server-key.json)" '.instance_template.metadata.xds_server_cert_key_kms=$x' | \
    jq --arg x "$(cat ./files/alb-server.pem)" '.instance_template.metadata.alb_server_cert_crt=$x' | \
    jq --arg x "$(cat ./files/alb-server-key.json)" '.instance_template.metadata.alb_server_cert_key_kms=$x' | \
    jq --arg x "$(cat ./files/xds-server.pem)" '.instance_template.metadata.server_cert_crt=$x' | \
    jq --arg x "$(cat ./files/xds-server-key.json)" '.instance_template.metadata.server_cert_key_kms=$x' | \
    jq --arg x "$(cat ./files/xds-client.pem)" '.instance_template.metadata.client_cert_crt=$x' | \
    jq --arg x "$(cat ./files/xds-client-key.json)" '.instance_template.metadata.client_cert_key_kms=$x' | \
    jq --arg x "$(cat ./files/envoy.tpl.yaml)" '.instance_template.metadata.envoy_config=$x' | \
    jq --arg x "$(cat ./files/default_ssh_keys.txt)" '.instance_template.metadata."ssh-keys"=$x' | \
    yq -y .)
