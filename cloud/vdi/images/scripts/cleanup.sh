#!/bin/bash 

set -eux

echo "==> SSH server keys"
rm -f /etc/ssh/*_key*

echo "==> machine ID"
rm -f /etc/machine-id
rm -f /var/lib/dbus/machine-id
touch /etc/machine-id
ln -s /etc/machine-id /var/lib/dbus/machine-id

echo "==> dhcp leases"
if [ -d "/var/lib/dhcp" ]; then
    rm -rf /var/lib/dhcp/*
fi

echo "==> tmp"
rm -rf /tmp/*

echo "==> apt chache"
apt -y autoremove --purge
apt -y clean
apt -y autoclean

unset HISTFILE
echo "==> bash history"
rm -f /root/.bash_history
rm -rf /root/.ssh/*
rm -f /root/*

echo "==> log files"
find /var/log -type f -delete
journalctl --rotate
journalctl --vacuum-time=1s

echo "==> disable root logon"
sed -i 's/.*PermitRootLogin.*/#PermitRootLogin No/g' /etc/ssh/sshd_config

echo "==> root password"
passwd -d root
passwd -l root

echo "==> remove ubuntu user"
userdel -f ubuntu
rm -rf /home/ubuntu

echo "==> sudoers"
rm -f /etc/sudoers.d/90-cloud-init-users
