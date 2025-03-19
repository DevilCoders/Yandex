#!/bin/bash

set -e

yum -y update
yum -y upgrade

mkdir /var/log/journal
systemd-tmpfiles --create --prefix /var/log/journal
systemctl restart systemd-journald
