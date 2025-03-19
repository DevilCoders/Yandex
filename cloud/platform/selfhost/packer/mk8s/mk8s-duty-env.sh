#!/bin/bash
set -x

# install kubectl (of 1.14.6 version for the current mk8s backend's k8s version 1.13)
curl -Lo kubectl https://storage.yandexcloud.net/mk8s/binaries/v1.14.6/kubectl-v1.14.6 && \
chmod +x kubectl && \
mv kubectl /usr/local/bin

# install yc
curl https://storage.yandexcloud.net/yandexcloud-yc/install.sh | bash

# install ycp
# TODO(skipor): remove when added to base: https://st.yandex-team.ru/CLOUD-34280
curl https://s3.mds.yandex.net/mcdev/ycp/install.sh | bash

echo "alias k=kubectl" >> /root/.bashrc
echo "alias d=docker" >> /root/.bashrc
echo "alias j=journalctl" >> /root/.bashrc
echo "alias s=systemctl" >> /root/.bashrc
echo "alias c=crictl" >> /root/.bashrc

