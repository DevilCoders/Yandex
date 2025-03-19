#!/bin/bash

. /usr/local/sbin/autodetect_active_eth

[ -n "$default_iface" ] || exit 1

cat <<EOF > /etc/network/interfaces
auto lo $default_iface
iface lo inet loopback

iface $default_iface inet6 auto
  privext 0
  mtu 8950
  ya-slb6-tun yes
  ya-slb-tun yes
EOF
