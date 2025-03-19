#!/bin/bash -eux

CLEANUP_PAUSE=${CLEANUP_PAUSE:-0}
echo "==> Pausing for ${CLEANUP_PAUSE} seconds..."
sleep ${CLEANUP_PAUSE}

sudo yum clean all

# Unique SSH keys will be generated on first boot
echo "==> Removing SSH server keys"
rm -f /etc/ssh/*_key*

# Unique machine ID will be generated on first boot
rm -f /etc/machine-id
rm -f /var/lib/dbus/machine-id
touch /etc/machine-id
ln -s /etc/machine-id /var/lib/dbus/machine-id

echo "==> Cleaning up leftover dhcp leases"
find /var/lib/dhclient/ -name *.lease -type f -print0 | sudo xargs -0 /bin/rm -f

echo "==> Cleaning up tmp"
rm -rf /tmp/*

# Remove Bash history
unset HISTFILE
rm -f /root/.bash_history
rm -rf /root/.ssh/*
rm -f /root/*

# Clean sudoers
rm -f /etc/sudoers.d/90-cloud-init-users

# Clean up log files
echo "==> Purging log files"
find /var/log -type f -delete

# Deny SSH root login
sed -i 's/.*PermitRootLogin.*/#PermitRootLogin No/g' /etc/ssh/sshd_config

# Clear root password
echo "==> Cleaning root password"
passwd -d root
passwd -l root

# Remove centos user
echo "==> Removing centos user"
userdel -f centos
rm -rf /home/centos