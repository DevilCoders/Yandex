#!/usr/bin/env bash
set -eou pipefail

INSTANCE_GROUP_ID="cl14gtcf0qibrnjvuhfu"

FOLDER_ID="b1gjdkej39hkol2v3kue"

FILES=$(jq -n \
    --arg platformHTTPChecks "$(cat ../common/platform-http-checks.json)" \
    --arg jaegerAgentConfig "$(cat ../common/jaeger-agent.yaml)" \
    --arg sdsConfig "$(cat ../common/sds.yaml)" \
    --arg caBundle "$(cat ../common/allCAs.pem)" \
    --arg serverCertificatePEM "$(cat ./files/server.pem)" \
    --arg serverCertificateKey "$(cat ./files/server_key.json)" \
    --arg clientCertificatePEM "$(cat ./files/client.pem)" \
    --arg clientCertificateKey "$(cat ./files/client_key.json)" \
    '{
        "/juggler-bundle/platform-http-checks.json": $platformHTTPChecks,
        "/etc/jaeger-agent/jaeger-agent-config.yaml": $jaegerAgentConfig,
        "/etc/l7/configs/sds/sds.yaml": $sdsConfig,
        "/etc/l7/configs/envoy/ssl/certs/allCAs.pem": $caBundle,
        "/etc/l7/configs/envoy/ssl/certs/frontend.crt": $serverCertificatePEM,
        "/etc/l7/configs/envoy/ssl/private/frontend.key": $serverCertificateKey,
        "/etc/l7/configs/envoy/ssl/certs/xds-client.crt": $clientCertificatePEM,
        "/etc/l7/configs/envoy/ssl/private/xds-client.key": $clientCertificateKey
    }')

yc compute instance-group update --id="${INSTANCE_GROUP_ID}" \
  --endpoint=api.cloud.yandex.net:443 \
  --folder-id="${FOLDER_ID}" \
  --file=<(yq . ig-spec.tpl.yaml | \
    jq --arg x "$(cat ./files/solomon-token.json)" '.instance_template.metadata.solomon_token_kms=$x' | \
    jq --arg x "$(cat ./files/envoy.tpl.yaml)" '.instance_template.metadata.envoy_config=$x' | \
    jq --arg x "$(cat ../common/user_data.yaml)" '.instance_template.metadata."user-data"=$x' | \
    jq --arg x "$(cat ../common/config-dumper-endpoints.json)" '.instance_template.metadata."config-dumper-endpoints"=$x' | \
    jq --arg x "$(cat ../common/default_ssh_keys.txt)" '.instance_template.metadata."ssh-keys"=$x' | \
    jq --arg x "$FILES" '.instance_template.metadata.files=$x' | \
    yq -y .)
