#!/bin/bash
set -ex

# Bootstrap a bootstrap host
# usage: bootstrap.sh ${fqdn} ${environment}
# requirements: pssh, yc-secret-cli, jq

fqdn="$1"
environment="$2"
BB_KEY_ID="01e0a45a-8626-11ea-81d5-1269bd65bbaa"
[[ "${environment}" == "testing" ]] && BB_KEY_ID="baab8d1f-6c3c-11eb-948c-c6c44f0e411e"
[[ "${environment}" == "preprod" ]] && BB_KEY_ID="e82ecfc4-855f-11eb-9790-56c1508db55c"
BB_KEY_NAME="robot-yc-bitbucket.key"
BB_KEY_VERSION="1"
SECRETS_PATH="/usr/share/yc-secrets"
BB_KEY_PATH="${SECRETS_PATH}/${BB_KEY_ID}/${BB_KEY_NAME}/${BB_KEY_VERSION}/${BB_KEY_NAME}"

yc-secret-cli --profile="${environment}" hostgroup list-hosts conductor_cloud_bootstrap | grep "${fqdn}" || yc-secret-cli --profile="${environment}" hostgroup add-host conductor_cloud_bootstrap --fqdn "${fqdn}"
pssh run "sudo yc-secret-agent" "${fqdn}"
pssh run "sudo cp ${BB_KEY_PATH} /root/.ssh/robot-yc-bitbucket.key" "${fqdn}"
pssh run "sudo chown 600 /root/.ssh/robot-yc-bitbucket.key" "${fqdn}"
pssh run "sudo apt-get -y install salt-yandex-components=2017.7.2-yandex1" "${fqdn}"
pssh run "sudo GIT_SSH_COMMAND='ssh -o StrictHostKeyChecking=no -i /root/.ssh/robot-yc-bitbucket.key' git clone ssh://git@bb.yandex-team.ru/cloud/management-salt.git /srv/salt" "${fqdn}"
pssh run "sudo salt-call --local --file-root /srv/salt/ --pillar-root /srv/salt/pillar state.highstate" "${fqdn}"
