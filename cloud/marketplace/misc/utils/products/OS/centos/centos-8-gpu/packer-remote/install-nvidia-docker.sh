#!/bin/bash

set -e

yum install ca-certificates curl yum-utils
yum remove -y docker docker-client docker-client-latest docker-common docker-latest docker-latest-logrotate docker-logrotate docker-engine

DOCKER_GPG_KEY=/tmp/docker-gpg-key
NVIDIA_DOCKER_GPG_KEY=/tmp/docker-gpg-key
curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o "$DOCKER_GPG_KEY"
rpm --import "$DOCKER_GPG_KEY"
rm -f "$DOCKER_GPG_KEY"
yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
curl -fsSL https://nvidia.github.io/nvidia-docker/gpgkey -o "$NVIDIA_DOCKER_GPG_KEY"
rpm --import "$NVIDIA_DOCKER_GPG_KEY"
rm -f "$NVIDIA_DOCKER_GPG_KEY"
distribution=$(. /etc/os-release; echo $ID$VERSION_ID)
curl -fsSL https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.repo | tee /etc/yum.repos.d/nvidia-docker.repo
yum update -y

yum install -y docker-ce docker-ce-cli containerd.io
yum install -y nvidia-container-toolkit nvidia-docker2

mkdir -p /etc/systemd/system/docker.service.d/
cat <<EOF > /etc/systemd/system/docker.service.d/override.conf
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd \\
 --default-shm-size=1G \\
 --host=fd:// \\
 --storage-driver=overlay2
LimitMEMLOCK=infinity
LimitSTACK=67108864
EOF

systemctl daemon-reload
systemctl restart docker
