#!/bin/bash

set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

echo "Install packages ..."

systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
apt-get -y update
apt-get install -y gnupg curl iptables

gpg_key_url="https://packages.gitlab.com/gitlab/gitlab-ce/gpgkey"
apt_config_url="https://packages.gitlab.com/install/repositories/gitlab/gitlab-ce/config_file.list?os=ubuntu&dist=bionic&source=script"

apt_source_path="/etc/apt/sources.list.d/gitlab_gitlab-ce.list"

echo -n "Installing $apt_source_path..."

# create an apt config file for this repository
curl -sSf "${apt_config_url}" > $apt_source_path

echo -n "Importing packagecloud gpg key... "
# import the gpg key
curl -L "${gpg_key_url}" | apt-key add -

apt-get update
EXTERNAL_URL="http://gitlab.example.com" apt-get -y install gitlab-ce

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections

apt-get -q -y install iptables-persistent

generate_print_credentials;
on_install;
print_msg;
