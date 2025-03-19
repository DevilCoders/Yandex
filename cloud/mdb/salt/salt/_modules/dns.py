# coding: utf-8
import socket
import logging


__salt__ = {}
log = logging.getLogger(__name__)


def __virtual__():
    return True


def has_ipv6_address(fqdn):
    try:
        addrs = socket.getaddrinfo(fqdn, 0, socket.AF_INET6)
        return len(addrs) > 0
    except Exception as e:
        log.warn("Can't resolve fqdn {0} via ipv6. Error message: {1}".format(fqdn, e))
        return False


def has_ipv4_address(fqdn):
    try:
        addrs = socket.getaddrinfo(fqdn, 0, socket.AF_INET)
        return len(addrs) > 0
    except Exception as e:
        log.warn("Can't resolve fqdn {0} via ipv4. Error message: {1}".format(fqdn, e))
        return False
