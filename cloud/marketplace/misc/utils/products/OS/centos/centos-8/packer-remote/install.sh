#!/bin/bash

set -e

yum -y update
yum -y upgrade

yum -y install gdisk

mkdir /var/log/journal
systemd-tmpfiles --create --prefix /var/log/journal
systemctl restart systemd-journald

cat > /etc/modprobe.d/blacklist-nouveau.conf <<EOF
# Use nvidia drivers for GPU
blacklist nouveau
blacklist lbm-nouveau
alias nouveau off
alias lbm-nouveau off
EOF

