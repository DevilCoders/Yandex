#!/bin/bash

set -e

yum -y install epel-release
yum groupinstall -y "Development Tools"
yum install -y wget dkms kernel-devel kernel-headers

DISTR="rhel8"
ARCH="x86_64"
REPOBASE="https://developer.download.nvidia.com/compute/cuda/repos/${DISTR}/${ARCH}"
dnf config-manager --add-repo "${REPOBASE}/cuda-${DISTR}.repo"
dnf -y module install nvidia-driver:latest-dkms
dnf -y install cuda cuda-drivers
dnf -y update
yum clean all

systemctl daemon-reload
systemctl enable nvidia-persistenced.service
