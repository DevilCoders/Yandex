#!/bin/bash

set -e

echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

DISTR="debian10"
ARCH="x86_64"
REPOBASE="https://developer.download.nvidia.com/compute/cuda/repos/${DISTR}/${ARCH}"
TMPDIR="/tmp"

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true

apt-get -y install gnupg software-properties-common
apt-key adv --fetch-keys ${REPOBASE}/7fa2af80.pub
add-apt-repository "deb ${REPOBASE}/ /"
add-apt-repository contrib
apt-get update
apt-get -y install cuda

systemctl daemon-reload
systemctl enable nvidia-persistenced.service
