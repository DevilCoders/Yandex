#!/usr/bin/env bash
set -eou pipefail

INSTANCE_GROUP_ID="df2tvtgausmobep621l4"
FOLDER_ID=batmvm4btt1d2eidsuqf

ALB_AWS_ACCESS_KEY_ID=$(ya vault get version "sec-01cxcwpq2hwygkxrqq6v73077q" -o access-key-id)

ycp --profile=testing microcosm instance-group update --id="${INSTANCE_GROUP_ID}" \
  --request=<(yq . ig-spec.tpl.yaml | \
    jq --arg x "${ALB_AWS_ACCESS_KEY_ID}" '.instance_template.metadata.alb_aws_access_key_id=$x' | \
    jq --arg x "$(cat ./files/secrets/alb-aws-secret-key.json)" '.instance_template.metadata.alb_aws_secret_access_key=$x' | \
    jq --arg x "$(cat ./files/secrets/alb-server.pem)" '.instance_template.metadata.alb_server_cert_crt=$x' | \
    jq --arg x "$(cat ./files/secrets/alb-server_key.json)" '.instance_template.metadata.alb_server_cert_key_kms=$x' | \
    jq --arg x "$(cat ./files/bootstrap.yaml)" '.instance_template.metadata."k8s-runtime-bootstrap-yaml"=$x' | \
    jq --arg x "$(cat ./files/alb.tpl.yaml)" '.instance_template.metadata."alb-api-config"=$x' | \
    jq --arg x "$(cat ./files/metrics-agent.yaml)" '.instance_template.metadata."metricsagent-config"=$x' | \
    jq --arg x "$(cat ./files/user_data.yaml)" '.instance_template.metadata."user-data"=$x' | \
    jq --arg x "$(cat ./files/default_ssh_keys.txt)" '.instance_template.metadata."ssh-keys"=$x' | \
    jq --arg x "$(cat ./files/allCAs.pem)" '.instance_template.metadata."yandex-ca"=$x' | \
    # jq --arg x "$(cat ./files/push-client.yaml)" '.instance_template.metadata."push-client-config"=$x' | \
    yq -y .)

echo "IG updated successfuly"
ycp --profile=testing microcosm instance-group list-instances --id="${INSTANCE_GROUP_ID}"
