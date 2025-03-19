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

apt-get -y update
apt-get -y install --no-install-recommends $PACKAGES

apt-get -y upgrade
apt-get -y install apt-transport-https

sed -i "s@#FallbackNTP.*@FallbackNTP=ntp0.NL.net clock.isc.org ntp2.vniiftri.ru ntps1-1.cs.tu-berlin.de ntp.ix.ru@" /etc/systemd/timesyncd.conf

echo 'datasource_list: [ NoCloud, GCE, Ec2, None ]' | tee /etc/cloud/cloud.cfg.d/90_dpkg.cfg

cat > /etc/cloud/cloud.cfg.d/00_Ec2.cfg <<EOF
datasource:
 Ec2:
  strict_id: false
EOF

locale-gen en_US.UTF-8
update-grub

cat > /etc/cloud/cloud.cfg.d/95-yandex-cloud.cfg <<EOF
# Yandex.Cloud repo configuration
manage_etc_hosts: true

system_info:
   package_mirrors:
     - arches: [i386, amd64]
       failsafe:
         primary: http://deb.debian.org/debian
         security: http://security.debian.org/
       search:
         primary:
           - http://mirror.yandex.ru/debian/
EOF
