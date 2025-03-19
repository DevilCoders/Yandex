#!/bin/bash

set -e

yum -y install epel-release
yum -y install cloud-utils-growpart dracut-modules-growroot

cat > /etc/cloud/cloud.cfg.d/00_Ec2.cfg <<EOF
datasource:
 Ec2:
  strict_id: false
EOF

cat > /etc/modprobe.d/blacklist-nouveau.conf <<EOF
# Use nvidia drivers for GPU
blacklist nouveau
blacklist lbm-nouveau
alias nouveau off
alias lbm-nouveau off
EOF

yum -y upgrade
