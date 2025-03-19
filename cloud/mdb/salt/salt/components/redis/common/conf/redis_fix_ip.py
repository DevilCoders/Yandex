#!/usr/bin/env python3

import os
import netifaces
import ipaddress

IFACE = 'eth0'
CONFIG_PATH="/etc/redis/cluster.conf"
CONFIG_COPY_PATH="/etc/redis/cluster.conf.cpy"
RESERVED_IP = {
    netifaces.AF_INET: [
        ipaddress.IPv4Network('127.0.0.0/8'),
        ipaddress.IPv4Network('169.254.0.0/16'),
    ],
    netifaces.AF_INET6: [
        # https://tools.ietf.org/html/rfc4291#section-2.4
        ipaddress.IPv6Network('fe80::/10')
    ],
}

AF_IP_OBJ = {
    netifaces.AF_INET: ipaddress.IPv4Address,
    netifaces.AF_INET6: ipaddress.IPv6Address,
}

REDIS_PORT = {{ salt.mdb_redis.get_redis_notls_port() }}
SHARDED_PORT = {{ salt.mdb_redis.get_sharded_notls_port() }}


def is_reserved_ip(af, address):
    for net in RESERVED_IP[af]:
        if AF_IP_OBJ[af](address) in net:
            return True
    return False


def get_ip(iface=IFACE):
    """
    Tries IPv4 first, then IPv6.
    Returns IP for iface if successful.
    """
    if not iface in netifaces.interfaces():
        raise RuntimeError(f'Unable to find interface: {iface}')

    ifaddresses = netifaces.ifaddresses(iface)
    for af in (netifaces.AF_INET, netifaces.AF_INET6):
        if af not in ifaddresses:
            # No current AF on iface (e.g. no IP4 in Porto)
            continue
        for address_data in ifaddresses[af]:
            address = address_data['addr']
            if not is_reserved_ip(af, address):
                return address
    RuntimeError(f'No suitable addresses found on {iface}')


def update_ip():
    # two ports to rule all the data
    fqdn = get_ip() + ':{}@{}'.format(REDIS_PORT, SHARDED_PORT)
    lines = []
    if os.path.isfile(CONFIG_PATH):
        flag = True
        with open(CONFIG_PATH) as cluster:
            for line in cluster:
                if line.find('myself') != -1:
                    split = line.split(' ')
                    split[1] = fqdn
                    line = ' '.join(split)
                    flag = False
                lines.append(line)
        if flag:
            RuntimeError(f'Unable to find myself in cluster.conf')
        with open(CONFIG_COPY_PATH, 'w') as conf:
            for line in lines:
                conf.write(line)
        os.rename(CONFIG_COPY_PATH, CONFIG_PATH)


if __name__ == '__main__':
    update_ip()
