#!/bin/bash

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-webauth.sh
source /opt/yc-marketplace/yc-cms-iptables.sh

rm $0;
rm /etc/cron.d/first-boot

update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
flush_welcome_body

status_code=$(curl --write-out %{http_code} --silent --output /dev/null "http://169.254.169.254/latest/meta-data/public-ipv4")

if [[ "$status_code" -ne 404 ]] ; then
  IP=$(curl http://169.254.169.254/latest/meta-data/public-ipv4)
else
  IP=$(curl http://169.254.169.254/latest/meta-data/local-ipv4)
fi

sed -i "s@external_url.*@external_url 'http://${IP}'@" /etc/gitlab/gitlab.rb

gitlab-ctl reconfigure


generate_iptables
netfilter-persistent save

run_print_credentials

print_msg;

rm -rf /opt/yc-marketplace/assets

echo "yc-setup: Done"

