#!/bin/bash

# If eth0 has IPv6, it is important to have on eth1 metric higher than on eth0
/sbin/ip -6 route add 2a02:6b8::/32 dev eth1 metric 1024
/sbin/ip -6 route add 2a0d:d6c0::/29 dev eth1 metric 1024
/sbin/ip -6 route add 2a11:f740::/29 dev eth1 metric 1024

# We route marked packets to be sent from needed interface.
# Packets are marked in `ip6tables -t mange -A OUTPUT`,
# see /etc/ferm/conf.d/30_user_net_routes.conf.
# More details could be found on wiki:
# https://wiki.yandex-team.ru/mdb/internal/operations/network/
/sbin/ip -6 rule add priority 8192 fwmark 8 table 8
/sbin/ip -6 route add default dev eth1 table 8

# We can't expect that all firewall rules will be ready from image
# since we don't have IPv6 when we build images.
# So we need a hack to get minimal needed network configuration
# for working salt during initial setup. On all other reboots
# ferm will fix these rules if needed.
/sbin/ip6tables -t mangle -I OUTPUT 1 -j MARK --set-mark 8
eth1_ipv6_address="$(/sbin/ip -6 add list dev eth1 | awk '/global/ {print $2}' | cut -f1 -d/)"
/sbin/ip6tables -t nat -A POSTROUTING -o eth1 -d 2a02:6b8::/32 -j SNAT --to-source ${eth1_ipv6_address}
/sbin/ip6tables -t nat -A POSTROUTING -o eth1 -d 2a0d:d6c0::/29 -j SNAT --to-source ${eth1_ipv6_address}
/sbin/ip6tables -t nat -A POSTROUTING -o eth1 -d 2a11:f740::/29 -j SNAT --to-source ${eth1_ipv6_address}
