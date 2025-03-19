#!/bin/bash

set -e

# Docker CE
apt-get -y update
apt-get -y install ca-certificates curl software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
add-apt-repository -y "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
apt-get -y remove docker docker-engine docker.io containerd runc
apt-get -y update
apt-get -y install docker-ce docker-ce-cli containerd.io

distribution=$(. /etc/os-release; echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list > /etc/apt/sources.list.d/nvidia-docker.list
apt-get -y update
apt-get -y install -y nvidia-container-toolkit nvidia-docker2
# usermod -aG docker $USER

mkdir -p /etc/systemd/system/docker.service.d/
cat <<EOF > /etc/systemd/system/docker.service.d/override.conf
[Service]
ExecStart=
ExecStart=/usr/bin/dockerd \\
 --default-shm-size="1G" \\
 --host=fd:// \\
 --storage-driver=overlay2
LimitMEMLOCK=infinity
LimitSTACK=67108864
EOF

systemctl daemon-reload
systemctl restart docker
