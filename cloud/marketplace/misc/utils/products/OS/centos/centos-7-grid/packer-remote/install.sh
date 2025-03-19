#!/bin/bash

set -eu

yum -y update
yum -y upgrade
yum -y install gdisk

cat > /etc/modprobe.d/blacklist-nouveau.conf <<EOF
# Use nvidia drivers for GPU
blacklist nouveau
blacklist lbm-nouveau
alias nouveau off
alias lbm-nouveau off
EOF

# grid part

URL="https://storage.yandexcloud.net/8tyoswphf7rct9ggdmepy32awkembilw/nvidia-vgpu-kvm/440.53/NVIDIA-Linux-x86_64-440.56-grid.run"
NVIDIA_DRIVER_PATH="/tmp/NVIDIA-Linux-x86_64-440.56-grid.run"

yum -y groupinstall "Development Tools"
yum -y install kernel-devel kernel-headers kernel-source

trap "rm -f $NVIDIA_DRIVER_PATH" EXIT
curl "$URL" -o "$NVIDIA_DRIVER_PATH"
chmod +x "$NVIDIA_DRIVER_PATH"

$NVIDIA_DRIVER_PATH --ui=none --silent --disable-nouveau