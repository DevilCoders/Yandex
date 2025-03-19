#!/bin/bash

eth0_ipv6_address="$(/sbin/ip -6 add list dev eth0 | awk '/global/ {print $2}' | cut -f1 -d/)"
if [ "$eth0_ipv6_address" = "" ]; then
    exit 0
fi

# It is important to have on eth0 metric less than on eth1
/sbin/ip -6 route add 2a02:6b8::/32 dev eth0 metric 512
/sbin/ip -6 route add 2a0d:d6c0::/29 dev eth0 metric 512
/sbin/ip -6 route add 2a11:f740::/29 dev eth0 metric 512

# We route marked packets to be sent from needed interface.
# Packets are marked in `ip6tables -t mange -A OUTPUT`,
# see /etc/ferm/conf.d/30_user_net_routes.conf.
# More details could be found on wiki:
# https://wiki.yandex-team.ru/mdb/internal/operations/network/
/sbin/ip -6 rule add priority 4096 fwmark 4 table 4
/sbin/ip -6 route add default dev eth0 table 4

# We can't expect that all firewall rules will be ready from image
# since we don't have IPv6 when we build images.
# So we need a hack to get minimal needed network configuration
# for working salt during initial setup. On all other reboots
# ferm will fix these rules if needed.
# These rules are needed for routing traffic to contrail dns through eth0.
/sbin/ip6tables -t nat -A POSTROUTING -o eth0 -d 2a02:6b8::/32 -j SNAT --to-source ${eth0_ipv6_address}
/sbin/ip6tables -t nat -A POSTROUTING -o eth0 -d 2a0d:d6c0::/29 -j SNAT --to-source ${eth0_ipv6_address}
/sbin/ip6tables -t nat -A POSTROUTING -o eth0 -d 2a11:f740::/29 -j SNAT --to-source ${eth0_ipv6_address}

eth0_project_id=$(/usr/bin/python3 -c "import ipaddress; ip = ipaddress.ip_address(\"${eth0_ipv6_address}\"); first96bits = int(ip) // 2**32; project_id = first96bits % 2**32; print(hex(project_id))")
if [ "$eth0_project_id" = "" ]; then
    exit 1
fi
/sbin/ip6tables -t mangle -A OUTPUT -m u32 --u32 "0x20&0xffffffff=${eth0_project_id}" -j MARK --set-xmark 4
