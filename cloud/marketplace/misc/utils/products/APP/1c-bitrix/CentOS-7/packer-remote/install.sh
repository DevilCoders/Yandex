#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

echo "Install packages ..."
yum -y update
yum -y install wget vim iptables ip6tables

sed -i "s/SELINUX=\(enforcing\|permissive\)/SELINUX=disabled/" /etc/selinux/config

set +e
COUNTER=0
while [[ $COUNTER -le 50 ]]; 
do
    wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 -t 5 http://repos.1c-bitrix.ru/yum/bitrix-env.sh -O /root/bitrix-env.sh && break
    sleep 5
    (( COUNTER++ ))
done
set -e

chmod +x /root/bitrix-env.sh
generate_print_credentials;
reboot
