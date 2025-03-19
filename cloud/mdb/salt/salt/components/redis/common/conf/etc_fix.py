#!/usr/bin/env python3

import ipaddress
import logging
import netifaces
import os
import socket


IFACE = 'eth0'
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
ETC_HOSTS = '/etc/hosts'
ETC_HOSTS_TMP = '/etc/hosts_tmp'

LOG = logging.getLogger('etc-fix')


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


def update_ip_in_etc_hosts():
    ip = get_ip()
    hostname = socket.gethostname()
    with open(ETC_HOSTS) as fobj:
        lines = fobj.readlines()
    host_record_lines, non_host_record_lines = [], []
    [(non_host_record_lines, host_record_lines)[hostname in line].append(line) for line in lines]
    if len(host_record_lines) == 1 and ip in host_record_lines[0]:
        LOG.info('Current ip %s is in %s, nothing to do, exiting...', ip, ETC_HOSTS)
        return

    new_line = "{}\t\t{}".format(ip, hostname)
    LOG.info('Ip not found in %s, adding line: %s', ETC_HOSTS, new_line)
    new_lines = non_host_record_lines
    new_lines.append(new_line)

    new_data = "\n".join(line.strip() for line in new_lines) + "\n"
    with open(ETC_HOSTS_TMP, 'w') as fobj:
        fobj.write(new_data)
    os.rename(ETC_HOSTS_TMP, ETC_HOSTS)
    LOG.info('Ip changed successfully, exiting...')


if __name__ == '__main__':
    logging.basicConfig(
        level=logging.DEBUG,
        format='%(asctime)s [%(levelname)s] %(name)s: %(message)s')
    try:
        update_ip_in_etc_hosts()
    except Exception as exc:
        LOG.exception(exc)
