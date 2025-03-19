#!/bin/bash

set -e

cat > /etc/cloud/cloud.cfg.d/95-yandex-cloud.cfg <<EOF
# Yandex.Cloud repo configuration
manage_etc_hosts: true

EOF

dnf -y update

