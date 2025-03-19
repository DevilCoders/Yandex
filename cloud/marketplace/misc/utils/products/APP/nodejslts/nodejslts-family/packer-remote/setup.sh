#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-cms-iptables.sh

echo "Adding users to nvm group"
for user in /home/*
do
    usermod -aG nvm $(basename "$user")
done

rm $0;
rm /etc/cron.d/first-boot

flush_welcome_body;
generate_iptables;
netfilter-persistent save
print_msg;
