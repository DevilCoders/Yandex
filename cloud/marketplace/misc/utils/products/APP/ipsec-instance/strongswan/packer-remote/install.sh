#!/bin/bash

set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
echo "Install packages ..."

apt-get -y update

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections

apt-get -q -y install iptables iptables-persistent strongswan

echo -e "net.ipv4.ip_forward = 1" >> /etc/sysctl.conf
echo -e "net.ipv4.conf.all.accept_redirects = 1" >> /etc/sysctl.conf
echo -e "net.ipv4.conf.all.send_redirects = 1" >> /etc/sysctl.conf

sysctl -p /etc/sysctl.conf
systemctl restart systemd-networkd

systemctl start strongswan 
systemctl enable strongswan

netfilter-persistent save

generate_print_credentials;
on_install;
print_msg;
