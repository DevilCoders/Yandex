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
python3-pip
telnet
atop
docker-ce
ubuntu-keyring
"
apt-get -y install --no-install-recommends $PACKAGES

pip3 install setuptools
pip3 install --upgrade pip==9.0.3
pip3 install docker

apt-get -y install python-pip

pip2 install setuptools
pip2 install --upgrade pip==9.0.3
pip2 install docker
