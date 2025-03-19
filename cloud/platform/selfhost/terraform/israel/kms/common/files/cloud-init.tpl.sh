#!/bin/bash

set -ex

# Enable this script to be run on each boot.
sed -i "s/ - scripts-user/ - [scripts-user, always]/g" /etc/cloud/cloud.cfg

# Use /etc/hosts proxy for local-lb
sed -i "/${local_lb_fqdn}/d" /etc/hosts
echo "${local_lb_addr} ${local_lb_fqdn}" >> /etc/hosts

# Use ns-cache
rm /etc/resolv.conf
echo "nameserver 2a02:6b8::1:1" > /etc/resolv.conf
