#!/bin/bash

set -eux

echo "==> ubuntu desktop" # ubuntu-desktop package will disable ufw
DEBIAN_FRONTEND=noninteractive apt install ubuntu-desktop -y

echo "==> allow not only console users in Xwrapper"
sed -i 's/allowed_users=console/allowed_users=anybody/' /etc/X11/Xwrapper.config

echo "==> unity pkla tweaks"
tee /etc/polkit-1/localauthority/50-local.d/45-allow-colord.pkla > /dev/null <<EOF
[Allow Colord all Users]
Identity=unix-user:*
Action=org.freedesktop.color-manager.create-device;org.freedesktop.color-manager.create-profile;org.freedesktop.color-manager.delete-device;org.freedesktop.color-manager.delete-profile;org.freedesktop.color-manager.modify-device;org.freedesktop.color-manager.modify-profile
ResultAny=no
ResultInactive=no
ResultActive=yes
EOF

tee /etc/polkit-1/localauthority/50-local.d/46-allow-admin.pkla > /dev/null <<EOF
[user admin]
Identity=unix-user:*
Action=org.gnome.controlcenter.user-accounts.administration
ResultAny=auth_admin_keep
ResultInactive=no
ResultActive=no
EOF

tee /etc/polkit-1/localauthority/50-local.d/47-allow-package-management.pkla > /dev/null <<EOF
[Allow Package Management all Users]
Identity=unix-user:*
Action=org.freedesktop.packagekit.system-sources-refresh
ResultAny=yes
ResultInactive=yes
ResultActive=yes
EOF

tee /etc/polkit-1/localauthority/50-local.d/48-allow-networkd.pkla > /dev/null <<EOF
[Allow Network Control all Users]
Identity=unix-user:*
Action=org.freedesktop.NetworkManager.network-control
ResultAny=no
ResultInactive=no
ResultActive=yes
EOF

tee /etc/polkit-1/localauthority/50-local.d/49-allow-shutdown.pkla > /dev/null <<EOF
[Disable Shutdown, etc.]
Identity=unix-user:*
Action=org.freedesktop.login1.reboot;org.freedesktop.login1.reboot-multiple-sessions;org.freedesktop.login1.power-off;org.freedesktop.login1.power-off-multiple-sessions;org.freedesktop.login1.suspend;org.freedesktop.login1.suspend-multiple-sessions;org.freedesktop.login1.hibernate;org.freedesktop.login1.hibernate-multiple-sessions
ResultAny=yes
ResultInactive=yes
ResultActive=yes
EOF
