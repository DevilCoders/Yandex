#!/bin/bash

set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
echo "Install packages ..."

apt-get -y update

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections

apt-get -q -y install iptables iptables-persistent

apt update && apt -y install ca-certificates wget net-tools
wget -qO - https://as-repository.openvpn.net/as-repo-public.gpg | apt-key add -
echo "deb http://as-repository.openvpn.net/as/debian bionic main">/etc/apt/sources.list.d/openvpn-as-repo.list
apt update && apt -y install openvpn-as

systemctl enable netfilter-persistent

systemctl stop openvpnas
systemctl disable openvpnas

netfilter-persistent save

generate_print_credentials;
on_install;
print_msg;
