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
python-google-compute-engine
python3-google-compute-engine
google-compute-engine-oslogin
gce-compute-image-packages
"
apt-get -y install --no-install-recommends $PACKAGES

pip3 install setuptools
pip3 install --upgrade pip==9.0.3
pip3 install docker
