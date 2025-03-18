# -*- coding: utf-8 -*-
import re
import socket
import logging
import requests

from sepelib.yandex.gencfg import GencfgClient

from src.transports.base_db_transport import InstanceDbTransport, Instance
from src.groupmap import TGroupToTagMapping
from src import retry


SLBS_URL = "http://api.gencfg.yandex-team.ru/trunk/slbs"


class GencfgApiTransport(InstanceDbTransport):
    """
    Transport for getting instances from gencfg via API
    """

    class TFakeDb(object):
        class TSlbs(object):
            """Slbs (RX-401)"""

            class TSlbInfo(object):
                def __init__(self, fqdn, ips):
                    self.fqdn = fqdn
                    self.ips = ips

            def __init__(self):

                @retry.retries(6, delay=6, backoff=2)
                def get_slbs():
                    # delays(s): 6, 12, 24, 48, 96, 192
                    return requests.get(SLBS_URL).json()
                result = get_slbs()

                self.data = {
                    x['fqdn']: GencfgApiTransport.TFakeDb.TSlbs.TSlbInfo(x['fqdn'], x['ips'])
                    for x in result['slbs']
                }

            def get(self, domain):
                return self.data[domain]

        def __init__(self):
            self.slbs = GencfgApiTransport.TFakeDb.TSlbs()

    def __init__(self, url=None, req_timeout=None, attempts=None, trunk_fallback=True):
        super(GencfgApiTransport, self).__init__(
            trunk_fallback=trunk_fallback,
        )

        self._gencfg_client = GencfgClient(url=url, req_timeout=req_timeout, attempts=attempts)
        self.instances_cache = dict()
        self.ip_cache = dict()
        self.__db = None

    @property
    def db(self):
        if self.__db is None:
            self.__db = GencfgApiTransport.TFakeDb()

        return self.__db

    def _instances_from_sepelib_instances(self, instances):
        return [Instance(
            hostname=str(instance.hostname),
            port=instance.port,
            power=instance.power,
            dc=str(instance.dc),
            location=str(instance.location),
            domain=str(instance.domain),
            ipv4addr=str(instance.ipv4addr) if instance.ipv4addr is not None else None,
            ipv6addr=str(instance.ipv6addr) if instance.ipv6addr is not None else None,
            hbf_mtn_hostname=str(instance.hbf_mtn_hostname),
            hbf_mtn_ipv6addr=str(instance.hbf_mtn_ipv6addr),
            tags=instance.tags,
        ) for instance in instances]

    def _convert_tag_name(self, tagname):
        if tagname == "trunk":
            return "trunk"

        m = re.match(r'stable-(\d+)-r(\d+)', tagname)
        if m is not None:
            return "tags/stable-{}-r{}".format(m.group(1), m.group(2))

        raise Exception("Can not convert tag <{}>".format(tagname))

    def get_group_instances(self, group_name, gencfg_version=None):
        """
        :param str group_name:
        :rtype: list of Instance
        """
        del gencfg_version

        if len(group_name.split(':')) > 1:
            group_name, tag = group_name.split(':')
        else:
            tag = self.mapping.gettag(TGroupToTagMapping.ETypes.GROUP, group_name)

        cache_key = (TGroupToTagMapping.ETypes.GROUP, group_name, tag)
        if cache_key in self.instances_cache:
            return self.instances_cache[cache_key]

        # SEARCH-7087 proves that gencfg API can return 404 in some cases,
        # but group exists and retry helps
        @retry.retries(5, delay=5, backoff=2)
        def list_group_instances():
            return self._gencfg_client.list_group_instances(group_name, self._convert_tag_name(tag))

        try:
            instances = list_group_instances()
        except requests.exceptions.HTTPError as exc:
            raise Exception(
                "Finally got HTTPError when getting instances for group <{}> "
                "in tag <{}>: <{}> (response content <{}>)".format(
                    group_name, tag, str(exc), exc.response.content,
                )
            )

        instances = self._instances_from_sepelib_instances(instances)

        if not instances:
            logging.error("Empty instances list for %s", group_name)

        self.instances_cache[cache_key] = instances
        return instances

    def get_intlookup_instances(self, intlookup_name, instance_type=None, gencfg_version=None):
        """
        :param intlookup_name: existing intlookup name, e.g. intlookup-msk-imgs.py
        :param instance_type: base or int for specific type, None for all instances
        :return:
        """

        tag = self.mapping.gettag(TGroupToTagMapping.ETypes.INTLOOKUP, self.group_name)

        return self._instances_from_sepelib_instances(
            self._gencfg_client.list_intlookup_instances(intlookup_name, instance_type, tag)
        )

    def describe_gencfg_version(self, gencfg_version=None):
        return ''

    def resolve_host(self, host, families, use_curdb=True, gencfg_version=None):
        if use_curdb:
            for family in families:
                if family == 4 and host.ipv4addr is not None:
                    return host.ipv4addr
                if family == 6 and host.ipv6addr is not None:
                    return host.ipv6addr

        for family in families:
            retval = self.ip_cache.get((host.hostname, family), None)
            if retval is not None:
                return retval

        # Add kosher retries (see MINOTAUR-774)
        @retry.retries(3, delay=10, backoff=2)
        def resolve_host_impl(family):
            sock_family = socket.AF_INET if family == 4 else socket.AF_INET6
            resolved = socket.getaddrinfo(host.hostname, None, sock_family)
            ips = set(ip[4][0] for ip in resolved)
            self.ip_cache[(host.hostname, family)] = list(ips)[0]
            return list(ips)[0]

        saved_exception = None
        for family in families:
            assert (family in [4, 6])
            if "127.0.0." in host.hostname and family == 6:
                # we'll definitely catch an error here, it is slow, so just don't try
                continue

            try:
                logging.warning("Resolving [v%s] %s", family, host.hostname)
                return resolve_host_impl(family)
            except Exception as exc:
                logging.error("Failed to resolve [v%s] %s", family, host.hostname)
                saved_exception = exc

        raise Exception(
            "Got exception of type <{type}> while resolving host <{host}> with protos <{protos}>: {message}".format(
                type=type(saved_exception),
                host=host.hostname,
                protos=",".join(map(lambda x: str(x), families)),
                message=str(saved_exception),
            )
        )

    def close_session(self):
        """
        Close connection to gencfg api to avoid race in reading from socket
        """
        self._gencfg_client._session.close()

    def name(self):
        return "gencfg_api"
