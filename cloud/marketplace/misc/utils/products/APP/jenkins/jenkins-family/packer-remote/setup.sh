#!/bin/bash

apt_wait () {
  while sudo fuser /var/lib/dpkg/lock >/dev/null 2>&1 ; do
    sleep 1
  done
  while sudo fuser /var/lib/apt/lists/lock >/dev/null 2>&1 ; do
    sleep 1
  done
  if [ -f /var/log/unattended-upgrades/unattended-upgrades.log ]; then
    while sudo fuser /var/log/unattended-upgrades/unattended-upgrades.log >/dev/null 2>&1 ; do
      sleep 1
    done
  fi
}

set -ev

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-webauth.sh
source /opt/yc-marketplace/yc-cms-iptables.sh

rm $0;
rm /etc/cron.d/first-boot

while [ ! -f /var/lib/cloud/instance/boot-finished ]; do
  echo 'Waiting for cloud-init...'
  sleep 5 
done

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
apt-get -y update

update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
flush_welcome_body

apt_wait
apt-get install -y jenkins

generate_iptables
iptables -A INPUT -p tcp --dport 8080 -j ACCEPT # Jenkins web port
ip6tables -A INPUT -p tcp --dport 8080 -j ACCEPT # Jenkins
netfilter-persistent save

systemctl enable nginx
systemctl start nginx
run_print_credentials

print_msg;

echo "yc-setup: Done"
