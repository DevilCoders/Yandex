#!/bin/bash
set -xe
CVS_VERSION="1.11.23"
CVS_ARCHIVE_URL="https://proxy.sandbox.yandex-team.ru/3129996495"
CVS_ARCHIVE_MD5="0213ea514e231559d6ff8f80a34117f0"

mkdir /data/CVSROOT -p || true
mkdir /data/locks || true
mkdir /data/tmp || true
mkdir /var/www/check || true
chmod 777 /data/locks /data/tmp || true
mkdir /opt/CVSROOT -p
mount -a
apt-get update > /dev/null || true
apt-get install apache2 apache2-suexec-pristine libio-compress-perl -y --force-yes || true
a2disconf other-vhosts-access-log
a2enmod ssl suexec cgid authnz_ldap headers

/etc/init.d/sssd stop || true
systemctl stop sssd || true

echo >&2 "Adding groups and users..."
groupadd -g 20001 dpt_yandex || true
groupadd -g 20044 dpt_yandex_mnt_noc || true
groupadd -g 20052 dpt_yandex_mnt_security || true

groupadd -g 520 cvsweb || true
useradd -m -d /var/www/cvs -s /bin/false -u 520 -g 520 -G 20001,20044,20052 cvsweb || true
usermod -a -G 20001,20044,20052 cvsweb || true
chown -R cvsweb:cvsweb /var/www/cvs || true

groupadd -g 509 racktables || true
useradd -m -u 509 -g 509 -G 20001,20044 racktables || true
usermod -a -G 20001,20044 racktables || true
chown -R racktables:racktables /home/racktables || true

groupadd -g 517 nocdeploy || true
useradd -m -u 517 -g 517 -G 20001,20044 nocdeploy || true
usermod -a -G 20001,20044 nocdeploy || true
chown -R nocdeploy:nocdeploy /home/nocdeploy || true

#groupadd -g _ nocrobot
#useradd -M -u _ -g _ -G 20001,20044,20052 nocrobot
#chown -R nocrobot:dpt_virtual_robots /home/nocrobot
#chown -R 31940:22073 /home/nocrobot	# nocrobot:dpt_virtual_robots

groupadd -g 505 dns-robot || true
useradd -m -u 505 -g 505 -G 20001,20044 dns-robot || true
usermod -a -G 20001,20044 dns-robot || true
chown -R dns-robot:dns-robot /home/dns-robot || true

for f in /etc/group /etc/gshadow; do
	sed -i -re 's/^(dpt_yandex:.*)$/\1,buildfarm,robot-vasya,robot-perry,robot-dynfw-dev,robot-dynfw-imports,robot-golem-svn,robot-dns,ttmgmt,robot-frodo,splunk-runner,robot-crt/' $f
	sed -i -re 's/^(dpt_yandex_mnt_noc:.*)$/\1,buildfarm,robot-vasya,robot-dynfw-dev,robot-dynfw-imports,robot-golem-svn,robot-dns,ttmgmt,robot-frodo,splunk-runner/' $f
	sed -i -re 's/^(dpt_yandex_mnt_security:.*)$/\1,robot-golem-svn,robot-perry,robot-frodo,robot-crt,lytboris,robot-dynfw-imports/' $f
done || true

/etc/init.d/sssd start || true
systemctl start sssd || true

apt-get update > /dev/null
apt-get install wget build-essential gcc -y --force-yes
cd /root
if test -e cvs-$CVS_VERSION; then
    mv -v cvs-$CVS_VERSION cvs-$CVS_VERSION-backup-$(date +%FT%T)
fi
if ! [[ "$(md5sum cvs-$CVS_VERSION.tar.bz2 2>/dev/null)" == "$CVS_ARCHIVE_MD5  cvs-$CVS_VERSION.tar.bz2" ]]; then
    wget -O cvs-$CVS_VERSION.tar.bz2  $CVS_ARCHIVE_URL
fi
tar -xjvf cvs-$CVS_VERSION.tar.bz2
cd cvs-$CVS_VERSION/
patch -i ../cvs-$CVS_VERSION-getline64.patch -p1
patch -i ../cvs-$CVS_VERSION-reduce-locksleep.patch -p1
patch -i ../cvs-$CVS_VERSION-logmsg.patch -p1
patch -i ../cvs-$CVS_VERSION-version.patch -p1
./configure --prefix=/usr
make -j12
make install
