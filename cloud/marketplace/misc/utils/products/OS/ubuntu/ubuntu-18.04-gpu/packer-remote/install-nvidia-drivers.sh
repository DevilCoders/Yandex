#!/bin/bash

set -e

echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

DISTR="ubuntu1804"
ARCH="x86_64"
REPOBASE="https://developer.download.nvidia.com/compute/cuda/repos/${DISTR}/${ARCH}"
TMPDIR="/tmp"

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true

wget --retry-connrefused --waitretry=1 --read-timeout=10 --timeout=10 ${REPOBASE}/cuda-${DISTR}.pin -P ${TMPDIR}
mv ${TMPDIR}/cuda-${DISTR}.pin /etc/apt/preferences.d/cuda-repository-pin-600
apt-key adv --fetch-keys ${REPOBASE}/7fa2af80.pub
add-apt-repository "deb ${REPOBASE}/ /"
apt-get update
apt-get -y install cuda

systemctl daemon-reload
systemctl enable nvidia-persistenced.service
