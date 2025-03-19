#!/usr/bin/env bash

set -ex

SOLOMON_VERSION="1:15.6"
PUSH_CLIENT_VERSION="6.69.4"
VERSION=$(date +%Y%m%d%H%M%S)-$(svn info --show-item revision)

read -p "Are you sure redeploy root-kms-config (y/n)?" CONT
if [ "$CONT" = "y" ]; then
    sed "s/<VERSION>/${VERSION}/" ${ROOT_DIR}/tmp/control > ${ROOT_DIR}/root-kms-config/DEBIAN/control
    dpkg-deb --root-owner-group --build ${ROOT_DIR}/root-kms-config

    pssh run "sudo apt-get install yandex-solomon-agent-bin=${SOLOMON_VERSION}" ${KMS_HOST_NAME}
    pssh run "sudo apt-get install yandex-push-client=${PUSH_CLIENT_VERSION}" ${KMS_HOST_NAME}

    pssh run "[ -d root-kms-config ] || mkdir root-kms-config" ${KMS_HOST_NAME}
    pssh scp "${ROOT_DIR}/root-kms-config.deb" "$KMS_HOST_NAME:/home/$USER/root-kms-config"
    pssh run "sudo apt -y --allow-downgrades install ./root-kms-config/root-kms-config.deb" ${KMS_HOST_NAME}

    pssh run "sudo systemctl enable solomon-agent.service; sudo systemctl daemon-reload" ${KMS_HOST_NAME}
    pssh run "sudo systemctl enable push-client.service; sudo systemctl daemon-reload" ${KMS_HOST_NAME}
    read -p "Do you want to restart solomon-agent (y/n)?" CONT
    if [[ "$CONT" = "y" ]]; then
        pssh run "sudo systemctl restart solomon-agent.service" ${KMS_HOST_NAME}
    fi
    read -p "Do you want to restart push-client (y/n)?" CONT
    if [[ "$CONT" = "y" ]]; then
        pssh run "sudo systemctl restart push-client.service" ${KMS_HOST_NAME}
    fi
fi
