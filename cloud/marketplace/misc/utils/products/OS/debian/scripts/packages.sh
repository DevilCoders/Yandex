#!/bin/bash -e

PACKAGES="
acpid
apt-file
bind9-host
bzip2
curl
dnsutils
htop
nmon
ntp
rsync
slurm
sudo
tcpdump
unzip
vim
cloud-utils
cloud-init
cloud-guest-utils
"
apt-get -y install --no-install-recommends $PACKAGES
