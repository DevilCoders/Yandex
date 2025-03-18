# -*- coding: utf-8 -*-
"""
Base transport for getting instances from gencfg
"""

import copy

from src.groupmap import TGroupToTagMapping


class Instance(object):
    """Instance for balancer"""

    __slots__ = (
        # data fields
        'hostname',  # instnace host name (fqdn)
        'port',
        'power',  # instances power (in gencfg units)
        'location',  # instance location
        'dc',  # instance dc
        'domain',  # instance host domain (all after dot in fqdn)
        'ipv4addr',
        'ipv6addr',
        # hbf mtn info
        'hbf_mtn_hostname',
        'hbf_mtn_ipv6addr',
        # tags info
        'tags',
        # auxiliarily field set when needed
        'cached_ip',
    )

    def __init__(self, hostname, port, power=None, dc=None, location=None, domain=None, ipv4addr=None,
                 ipv6addr=None, hbf_mtn_hostname=None, hbf_mtn_ipv6addr=None, tags=None):
        self.hostname = hostname
        self.port = port
        self.power = power
        self.dc = dc
        self.location = location
        self.domain = domain
        self.ipv4addr = ipv4addr
        self.ipv6addr = ipv6addr
        self.hbf_mtn_hostname = hbf_mtn_hostname
        self.hbf_mtn_ipv6addr = hbf_mtn_ipv6addr
        if tags is None:
            self.tags = set()
        else:
            self.tags = set(copy.copy(tags))

    def __eq__(self, other):
        return (
            self.hostname == other.hostname and
            self.port == other.port and
            self.power == other.power and
            self.dc == other.dc and
            self.location == other.location and
            self.domain == other.domain and
            self.ipv4addr == other.ipv4addr and
            self.ipv6adddr == other.ipv6addr
        )

    def __repr__(self):
        return (
            '{0.hostname}:{0.port} (power: {0.power}, dc: {0.dc}, '
            'location: {0.location}, domain: {0.domain}, '
            'ipv4addr: {0.ipv4addr}, ipv6addr: {0.ipv6addr})'.format(self)
        )


class InstanceDbTransport(object):
    """
    Base transport for getting instances from gencfg
    """

    def __init__(self, trunk_fallback=True):
        self.mapping = TGroupToTagMapping(
            trunk_fallback=trunk_fallback,
        )
        self.trunk_fallback = trunk_fallback

    def get_group_instances(self, group_name, gencfg_version=None):
        """
        :param str group_name:
        :rtype: list of Instance
        """
        raise NotImplementedError()

    def get_intlookup_instances(self, intlookup_name, instance_type=None, gencfg_version=None):
        """
        :param intlookup_name: existing intlookup name, e.g. intlookup-msk-imgs.py
        :param instance_type: base or int for specific type, None for all instances
        :return:
        """
        raise NotImplementedError()

    def describe_gencfg_version(self, gencfg_version=None):
        """
        Returns a string that uniquely identifies commit of given gencfg version
        and which as human friendly as it's possible (tries to make it based on nearest tag name).

        :param gencfg_version:
        :rtype: str
        """
        raise NotImplementedError()

    def resolve_host(self, host_name, families, use_curdb=True, gencfg_version=None):
        """
        Retruns resolved host name

        :param host_name: fqdn host name to resolve, e. g. ws2-200.yandex.ru
        :param families: list of tcp proto versions. Valid values: [4, 6], [6, 4], [4], [6]
        :param use_curdb: do not perform resolving if host already resolved in gencfg (option is on by default)
        :param gencfg_version
        :rtype: str
        """
        raise NotImplementedError()

    def close_session(self):
        """
        Close connection to gencfg api to avoid race in reading from socket
        """
        raise NotImplementedError()

    def name(self):
        """Return transport name"""
        raise NotImplementedError()
