#!/bin/bash

set -e

yum -y install epel-release
yum groupinstall -y "Development Tools"
yum install -y wget dkms kernel-devel kernel-headers

DISTR="rhel7"
ARCH="x86_64"
REPOBASE="https://developer.download.nvidia.com/compute/cuda/repos/${DISTR}/${ARCH}"
yum-config-manager --add-repo "${REPOBASE}/cuda-${DISTR}.repo"
yum -y install nvidia-driver-latest-dkms cuda cuda-drivers
yum -y update
yum clean all

systemctl daemon-reload
systemctl enable nvidia-persistenced.service
