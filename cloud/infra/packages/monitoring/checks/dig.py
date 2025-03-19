#!/usr/bin/env python3

import logging
import os
import sys
import configparser

# NOTE: package dnspython
import dns.resolver
import dns.name

logger = logging.getLogger(__name__)
CONFIG = {}


class MonrunException(Exception):
    def __init__(self, stage, wrapped_exc):
        self.stage = stage
        self.wrapped_exc = wrapped_exc

    def __str__(self):
        return "stage %s failed %s %s" % (self.stage,
                                          self.wrapped_exc.__class__.__name__,
                                          self.wrapped_exc)


def m_exit(status, message):
    """
    Monrun output function
    """
    print("PASSIVE-CHECK:dig;{0};{1}".format(status, message))
    sys.exit(0)


def gen_config():
    global CONFIG
    config_path = 'dig.conf'

    if not os.path.exists(config_path):
        m_exit(1, 'config file does not exist')

    config = configparser.ConfigParser()
    config.read(config_path)
    CONFIG['YANDEX_NS_SERVERS'] = [item.strip()
                                   for item in config.get('dig', 'yandex_ns_servers').split(',')]
    CONFIG['COMMON_YANDEX_HOSTNAME'] = config.get(
        'dig', 'common_yandex_hostname')
    # CONFIG['CLOUD_NS_SERVERS'] = [item.strip() for item in config.get('dig', 'cloud_ns_servers').split(',')]
    # CONFIG['PUBLIC_ZONE'] = config.get('dig', 'public_zone')
    return CONFIG


def dig(host, ns_servers, ttype="AAAA", timeout=None):
    logger.info("resolve %s %s via %s", host, ttype, ns_servers)
    resolver = dns.resolver.Resolver(configure=False)
    resolver.nameservers = ns_servers
    if timeout is not None:
        resolver.timeout = timeout
    try:
        resolver.query(host, ttype)
    except Exception as err:
        raise MonrunException("dig %s %s" % (host, ttype), err)


def resolve_nameservers(ns_servers, timeout=None):
    logger.info("resolve nameservers IPs: %s", ns_servers)
    resolver = dns.resolver.Resolver(configure=True)
    if timeout is not None:
        resolver.timeout = timeout
    answer = []
    ttype = "AAAA"
    if not isinstance(ns_servers, list):
        list(ns_servers)
    for ns_servername in ns_servers:
        logger.info("resolve %s for %s", ttype, ns_servername)
        try:
            ans = resolver.query(ns_servername, ttype,
                                 raise_on_no_answer=False)
        except dns.exception.DNSException as err:
            raise MonrunException(ns_servername, err)
        answer.extend((str(i) for i in ans))
    if not answer:
        raise MonrunException("resolve_nameservers",
                              Exception("empty IPS result"))
    return answer


def main():
    TIMEOUT = 5
    try:
        # dig common yandex name via common yandex's NS servers
        yandex_ns_servers = resolve_nameservers(
            CONFIG['YANDEX_NS_SERVERS'], timeout=TIMEOUT)
        dig(CONFIG['COMMON_YANDEX_HOSTNAME'],
            yandex_ns_servers, timeout=TIMEOUT)

        # dig common yandex name via our NS servers
        # cloud_ns_servers = resolve_nameservers(CONFIG['CLOUD_NS_SERVERS'], timeout=TIMEOUT)
        # dig(CONFIG['COMMON_YANDEX_HOSTNAME'], cloud_ns_servers, timeout=TIMEOUT)

        # dig(CONFIG['PUBLIC_ZONE'], cloud_ns_servers, ttype="SOA", timeout=TIMEOUT)
    except Exception as err:
        m_exit(2, str(err))
    else:
        m_exit(0, "OK")


if __name__ == "__main__":
    if os.getenv("DEBUG"):
        handler = logging.StreamHandler()
        logger.addHandler(handler)
        logger.setLevel(logging.DEBUG)
        handler.setLevel(logging.DEBUG)
    gen_config()
    main()
